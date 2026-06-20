#!/usr/bin/env python3
"""Luna-backed visual-regression runner (Phase 1 prototype).

Part of the migration off snes9x-WASM + Mesen2 onto luna
(see /tmp/luna_migration_report_2026-06-20.md and
.claude/notes/chantiers/luna_migration.md).

What it does
------------
For each example in MANIFEST it drives the headless `luna` CLI to render a
deterministic framebuffer, then:
  * `--update`  : writes the baseline (PNG + SHA-256 + provenance) to
                  tools/luna-test/baselines/.
  * default     : re-renders and compares the framebuffer SHA-256 against the
                  stored baseline → pass/fail (the runner computes the
                  verdict; luna has no --assert flag yet, decision #3).

Why hashing the PNG is sound: luna's headless framebuffer render is
byte-deterministic run-to-run (verified in the Phase 0 spike — identical PNG
bytes across two runs), so a SHA-256 of the PNG is a stable, drift-free
regression key. The PNG itself is kept alongside for human diffing
(decision #1: "both" — hash gate + PNG debug).

luna binary resolution order: $LUNA_BIN, then `luna` on PATH, then the
vendored extract under tools/luna-test/vendor/.

Usage
-----
    python3 tools/luna-test/luna_runner.py --update          # (re)baseline all
    python3 tools/luna-test/luna_runner.py                   # compare all
    python3 tools/luna-test/luna_runner.py --only mapscroll  # one label substring
    python3 tools/luna-test/luna_runner.py --list            # show the manifest

Exit code: 0 = all pass, 1 = at least one mismatch / error.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tomllib
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
HERE = Path(__file__).resolve().parent
BASELINE_DIR = HERE / "baselines"
LUNA_VERSION = "v0.2.0"  # pinned (decision #4)

# Phase 1 first batch — representative coverage across the axes that matter:
# basic render, sprites, scrolling map (our B1 bank-$02 case), audio driver,
# and the two enhancement chips snes9x cannot run.
#   steps = CPU instructions before capture. Any fixed value is fine for a
#   deterministic baseline; pick one past boot/scene-setup for that example.
MANIFEST = [
    {"label": "text_hello_world",      "rom": "examples/text/hello_world/hello_world.sfc",      "steps": 1_000_000},
    {"label": "maps_mapscroll",        "rom": "examples/maps/mapscroll/mapscroll.sfc",          "steps": 3_000_000},
    {"label": "graphics_sprites_metasprite", "rom": "examples/graphics/sprites/metasprite/metasprite.sfc", "steps": 2_000_000},
    {"label": "audio_snesmod_music",   "rom": "examples/audio/snesmod_music/music.sfc",         "steps": 4_000_000},
    {"label": "memory_superfx_hello",  "rom": "examples/memory/superfx_hello/superfx_hello.sfc","steps": 3_000_000},
    {"label": "memory_sa1_hello",      "rom": "examples/memory/sa1_hello/sa1_hello.sfc",        "steps": 3_000_000},
]


def find_luna() -> str:
    env = os.environ.get("LUNA_BIN")
    if env and Path(env).is_file():
        return env
    installed = HERE / "bin" / "luna"  # scripts/install-luna.sh target
    if installed.is_file():
        return str(installed)
    on_path = shutil.which("luna")
    if on_path:
        return on_path
    vendored = HERE / "vendor" / f"luna-{LUNA_VERSION}-linux-{os.uname().machine}" / "luna"
    if vendored.is_file():
        return str(vendored)
    sys.exit(
        "ERROR: luna binary not found. Run scripts/install-luna.sh, set $LUNA_BIN, "
        f"or put `luna` on PATH. Expected luna {LUNA_VERSION}."
    )


def load_manifest() -> dict:
    """Per-example overrides from manifest.toml (default_steps + [examples.*])."""
    path = HERE / "manifest.toml"
    if not path.is_file():
        return {"default_steps": 3_000_000, "examples": {}}
    with path.open("rb") as f:
        m = tomllib.load(f)
    m.setdefault("default_steps", 3_000_000)
    m.setdefault("examples", {})
    return m


def example_key(rom: Path) -> str:
    """Example path relative to examples/ (the dir holding main.c)."""
    return str(rom.parent.relative_to(REPO_ROOT / "examples"))


def liveness(state: dict) -> tuple[bool, str]:
    """Is the machine actually *running* (not just rendering)?

    'renders' != 'works': a big PNG can be a crashed/hung/forced-blank frame.
    The robust running signal is the NMI/VBlank handshake advancing and the CPU
    not halted. Catches crashes, hangs and dead-NMI; does NOT catch a ROM that
    runs but waits on an unmodelled device (e.g. Mouse/Super Scope DETECT) —
    that's handled by the manifest's input_dependent flag, not here.
    """
    sch = state.get("scheduler", {})
    cpu = state.get("cpu", {})
    frames = sch.get("frame_count", 0)
    nmis = sch.get("nmis_serviced", 0)
    if cpu.get("stopped"):
        return False, "CPU stopped (STP)"
    if frames <= 0:
        return False, "no PPU frames advanced"
    if nmis <= 0:
        return False, "no NMIs serviced (VBlank handshake dead)"
    # NOTE: frames - nmis is the boot offset (frames before NMI was enabled),
    # which varies by example init (audio drivers load in forced blank; GSU
    # compute) — it is NOT lag, so we do not gate on the ratio. Catching a
    # frozen-after-boot ROM would need a two-snapshot delta (nmis advanced
    # between N1 and N2) — noted as future hardening.
    return True, f"live ({frames}f/{nmis}nmi)"


def discover_example_roms() -> list[Path]:
    """Canonical corpus = one ROM per example *that has a main.c* (N_corpus=56).

    Discovering via main.c (not a loose `*.sfc` glob) excludes stale build
    residue like the source-less examples/graphics/effects/hdma_gradient/ that
    inflated an earlier `.sfc` count to 57. One .sfc is expected per example dir.
    """
    roms: list[Path] = []
    for main_c in sorted(REPO_ROOT.glob("examples/**/main.c")):
        sfcs = sorted(main_c.parent.glob("*.sfc"))
        if sfcs:
            roms.append(sfcs[0])
    return roms


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    h.update(path.read_bytes())
    return h.hexdigest()


def render(luna: str, rom: Path, steps: int, out_png: Path) -> None:
    """Render a deterministic framebuffer PNG for `rom` after `steps` instructions."""
    out_png.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run(
        [luna, "run", "-n", str(steps), "--screenshot", str(out_png), str(rom)],
        capture_output=True, text=True, timeout=300,
    )
    if proc.returncode != 0 or not out_png.is_file():
        raise RuntimeError(f"luna run failed for {rom.name}: {proc.stderr.strip()[:400]}")


def run(update: bool, only: str | None) -> int:
    luna = find_luna()
    BASELINE_DIR.mkdir(parents=True, exist_ok=True)
    manifest_path = BASELINE_DIR / "baselines.json"
    db = json.loads(manifest_path.read_text()) if manifest_path.is_file() else {}

    failures, count = 0, 0
    for entry in MANIFEST:
        label = entry["label"]
        if only and only not in label:
            continue
        count += 1
        rom = REPO_ROOT / entry["rom"]
        if not rom.is_file():
            print(f"  SKIP  {label}: ROM missing ({entry['rom']}) — build it first")
            failures += 1
            continue

        baseline_png = BASELINE_DIR / f"{label}.png"
        if update:
            render(luna, rom, entry["steps"], baseline_png)
            digest = sha256_file(baseline_png)
            db[label] = {
                "sha256": digest,
                "steps": entry["steps"],
                "rom_sha256": sha256_file(rom),
                "luna_version": LUNA_VERSION,
            }
            print(f"  BASELINE  {label}  sha256={digest[:16]}…  ({baseline_png.stat().st_size} B)")
        else:
            ref = db.get(label)
            if not ref:
                print(f"  MISS  {label}: no baseline — run with --update first")
                failures += 1
                continue
            actual_png = Path("/tmp/luna-test-actual") / f"{label}.png"
            try:
                render(luna, rom, ref["steps"], actual_png)
            except RuntimeError as e:
                print(f"  ERROR {label}: {e}")
                failures += 1
                continue
            digest = sha256_file(actual_png)
            if digest == ref["sha256"]:
                print(f"  PASS  {label}")
            else:
                print(f"  FAIL  {label}: sha256 {digest[:16]}… != baseline {ref['sha256'][:16]}…")
                print(f"        actual PNG kept at {actual_png} for diff")
                failures += 1

    if update:
        manifest_path.write_text(json.dumps(db, indent=2, sort_keys=True) + "\n")
        print(f"\nWrote {manifest_path.relative_to(REPO_ROOT)} ({count} entries).")

    print(f"\n{'UPDATE' if update else 'COMPARE'}: {count - failures}/{count} ok"
          + (f", {failures} failed" if failures else ""))
    return 1 if failures else 0


def render_state(luna: str, rom: Path, steps: int, png: Path) -> dict:
    """Run `luna state` → return parsed EmulatorState JSON (+ write a PNG)."""
    png.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run(
        [luna, "state", "-n", str(steps), "--out", "-", "--screenshot", str(png), str(rom)],
        capture_output=True, text=True, timeout=300,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"luna state failed: {proc.stderr.strip()[:300]}")
    return json.loads(proc.stdout)


def coverage(luna: str) -> int:
    """Whole-corpus headless pass: does luna *run* every OpenSNES example?

    Upgrade over the old PNG-size heuristic ('renders' != 'works'): each ROM is
    checked for real LIVENESS from `luna state` (NMI/VBlank handshake advancing,
    CPU not halted). Examples whose device input luna can't drive (manifest
    input_dependent, gap G4) are reported as boot+visual only, not a clean pass.
    """
    out_dir = Path("/tmp/luna-test-corpus")
    out_dir.mkdir(parents=True, exist_ok=True)
    manifest = load_manifest()
    default_steps = manifest["default_steps"]
    roms = discover_example_roms()  # canonical N_corpus (via main.c, skips residue)
    rows, ok, inputdep, dead, fail = [], 0, 0, 0, 0
    for rom in roms:
        key = example_key(rom)
        cfg = manifest["examples"].get(key, {})
        steps = cfg.get("steps", default_steps)
        png = out_dir / f"{key.replace('/', '_')}.png"
        try:
            state = render_state(luna, rom, steps, png)
        except Exception as e:  # noqa: BLE001 — bench-style panic-safety
            fail += 1
            rows.append((key, "FAIL", str(e)[:80]))
            print(f"  {'FAIL':9} {key}: {str(e)[:80]}")
            continue
        live, why = liveness(state)
        if not live:
            dead += 1
            status = "DEAD"
        elif cfg.get("input_dependent"):
            inputdep += 1
            status = "INPUT-DEP"
        else:
            ok += 1
            status = "OK"
        rows.append((key, status, why))
        print(f"  {status:9} {key}  ({why})")

    report = HERE / "CORPUS_COVERAGE.md"
    lines = [
        "# Luna corpus coverage (whole-suite headless liveness pass)",
        "",
        f"luna {LUNA_VERSION} · `luna state -n <steps>` per ROM · {len(roms)} ROMs · "
        f"**{ok} OK, {inputdep} INPUT-DEP, {dead} DEAD, {fail} FAIL**",
        "",
        "> Liveness from `luna state` (NMI/VBlank advancing, CPU not halted) — not "
        "a PNG-size heuristic. **INPUT-DEP** = runs+renders but its device input "
        "(Mouse/Super Scope, gap G4) is unmodelled → boot+visual only, *not* a "
        "clean functional pass. **DEAD** = ran but not live (crash/hang). "
        "**FAIL** = luna errored. PNGs: `/tmp/luna-test-corpus/`.",
        "",
        "| Example | Status | Detail |",
        "|---|---|---|",
    ]
    lines += [f"| `{l}` | {s} | {d} |" for l, s, d in rows]
    report.write_text("\n".join(lines) + "\n")
    print(f"\nCoverage: {ok} OK / {inputdep} INPUT-DEP / {dead} DEAD / {fail} FAIL "
          f"of {len(roms)}.")
    print(f"Report: {report.relative_to(REPO_ROOT)}")
    return 1 if (dead or fail) else 0


def main() -> int:
    ap = argparse.ArgumentParser(description="Luna visual-regression runner (Phase 1 prototype)")
    ap.add_argument("--update", action="store_true", help="(re)write baselines instead of comparing")
    ap.add_argument("--only", metavar="SUBSTR", help="restrict to labels containing SUBSTR")
    ap.add_argument("--list", action="store_true", help="print the manifest and exit")
    ap.add_argument("--coverage", action="store_true",
                    help="run EVERY built example ROM and write a compatibility report")
    args = ap.parse_args()
    if args.list:
        for e in MANIFEST:
            print(f"  {e['label']:32} {e['rom']}  (-n {e['steps']})")
        return 0
    if args.coverage:
        return coverage(find_luna())
    return run(args.update, args.only)


if __name__ == "__main__":
    sys.exit(main())
