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
    on_path = shutil.which("luna")
    if on_path:
        return on_path
    vendored = HERE / "vendor" / f"luna-{LUNA_VERSION}-linux-{os.uname().machine}" / "luna"
    if vendored.is_file():
        return str(vendored)
    sys.exit(
        "ERROR: luna binary not found. Set $LUNA_BIN, put `luna` on PATH, or "
        f"vendor it under tools/luna-test/vendor/. Expected luna {LUNA_VERSION}."
    )


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


def main() -> int:
    ap = argparse.ArgumentParser(description="Luna visual-regression runner (Phase 1 prototype)")
    ap.add_argument("--update", action="store_true", help="(re)write baselines instead of comparing")
    ap.add_argument("--only", metavar="SUBSTR", help="restrict to labels containing SUBSTR")
    ap.add_argument("--list", action="store_true", help="print the manifest and exit")
    args = ap.parse_args()
    if args.list:
        for e in MANIFEST:
            print(f"  {e['label']:32} {e['rom']}  (-n {e['steps']})")
        return 0
    return run(args.update, args.only)


if __name__ == "__main__":
    sys.exit(main())
