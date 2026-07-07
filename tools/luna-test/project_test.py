#!/usr/bin/env python3
"""Project-level test runner — `make test` for user game projects.

Runs the tests declared in a project's `test/manifest.toml` against the
project's ROM using the same pinned luna binary and the same techniques as
the SDK's own harness (fbhash visual baselines, WRAM asserts by symbol
name, the WDM/SNES_ASSERT oracle). Baselines are project-local:
`test/baselines.json` plus one PNG per capture point for human diffing.

Manifest format (TOML):

    # Optional; used when a test declares no `steps`.
    default_steps = 3_000_000

    [tests.boot]
    steps = 3_000_000              # scalar or list (multi-point capture)

    [tests.walk_right]
    steps = 3_000_000
    input = "30:0x100,90:0"        # joypad script, frame:hex (hold RIGHT 30-89)
    assert = ["player_x = 8600"]   # symbol = little-endian hex bytes in WRAM

Semantics per test:
  - `assert` entries run through `luna state --assert NAME=HEX` — NAME is
    resolved from the ROM's .sym by luna (v1.7.0+), HEX is the raw
    little-endian byte string (an s16 of 134 → "8600").
  - Without `input`, every steps point is also a visual checkpoint:
    fbhash is compared against test/baselines.json, and the in-ROM
    SNES_ASSERT/WDM channel is a free failure oracle.
  - With `input`, the visual baseline is skipped (luna's fbhash lives on
    `luna run`, which takes no input script) — the asserts are the oracle,
    and a screenshot is still written for the human.

Exit 0 = all tests pass; 1 = any failure. `--update` (or `make
test-update`) rewrites the baselines from the current ROM.
"""
from __future__ import annotations

import argparse
import json
import sys
import tomllib
from pathlib import Path

# The SDK harness lives next to this file — reuse its luna resolution and
# manifest helpers instead of duplicating them.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from luna_runner import LUNA_VERSION, find_luna, render, sha256_file, steps_points  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent / "probes"))
from lib import assert_mem  # noqa: E402


def load_project_manifest(test_dir: Path) -> dict:
    path = test_dir / "manifest.toml"
    if not path.is_file():
        sys.exit(f"ERROR: {path} not found — create it to declare tests "
                 "(see docs/GETTING_STARTED.md, 'Test your game').")
    with path.open("rb") as f:
        m = tomllib.load(f)
    if not m.get("tests"):
        sys.exit(f"ERROR: {path} declares no [tests.*] table.")
    m.setdefault("default_steps", 3_000_000)
    return m


def parse_asserts(entries: list[str]) -> list[tuple[str, str]]:
    """'player_x = 8600' → ('player_x', '8600'). Bytes are little-endian hex."""
    specs = []
    for e in entries:
        name, _, hexb = e.partition("=")
        name, hexb = name.strip(), hexb.strip().lower()
        if not name or not hexb or len(hexb) % 2:
            sys.exit(f"ERROR: bad assert spec {e!r} — expected 'symbol = hexbytes' "
                     "with an even number of hex digits (little-endian).")
        specs.append((name, hexb))
    return specs


def run_tests(rom: Path, test_dir: Path, update: bool) -> int:
    luna = find_luna()
    manifest = load_project_manifest(test_dir)
    baseline_path = test_dir / "baselines.json"
    baselines = (json.loads(baseline_path.read_text())
                 if baseline_path.is_file() else {})
    new_baselines: dict = {}
    failed = 0

    # `make test` must not clobber the human-facing baseline PNGs in test/:
    # actual captures land in test/actual/ (gitignored); --update writes test/.
    capture_dir = test_dir if update else test_dir / "actual"

    for name, spec in manifest["tests"].items():
        points = steps_points(spec.get("steps", manifest["default_steps"]))
        input_script = spec.get("input")
        asserts = parse_asserts(spec.get("assert", []))
        problems: list[str] = []

        if input_script is None:
            # Visual checkpoints: fbhash per point + the WDM oracle.
            entry = []
            for i, steps in enumerate(points):
                png = capture_dir / (f"{name}.png" if i == 0 else f"{name}@{steps}.png")
                fbhash, wdm_fired = render(luna, rom, steps, png)
                if wdm_fired:
                    problems.append(f"@{steps}: in-ROM SNES_ASSERT fired (see "
                                    f"{png.with_suffix('.wdm.txt')})")
                entry.append({"steps": steps, "fbhash": fbhash})
                if not update:
                    known = (baselines.get(name) or [{}] * len(points))
                    want = known[i].get("fbhash") if i < len(known) else None
                    if want is None:
                        problems.append(f"@{steps}: no baseline — run 'make test-update'")
                    elif want != fbhash:
                        problems.append(f"@{steps}: fbhash {fbhash} != baseline {want} "
                                        f"(actual: {png})")
            new_baselines[name] = entry
        else:
            # Input-driven: screenshot for the human, no fbhash baseline
            # (luna's fbhash lives on `run`, which takes no input script).
            png = capture_dir / f"{name}.png"
            png.parent.mkdir(parents=True, exist_ok=True)
            ok, detail = assert_mem(luna, rom, points[-1], [],
                                    input_script=input_script,
                                    extra=["--screenshot", str(png)])
            if not ok:
                problems.append(f"run failed: {detail}")

        if asserts:
            ok, detail = assert_mem(luna, rom, points[-1], asserts,
                                    input_script=input_script)
            if not ok:
                problems.append(detail)

        if problems and not update:
            failed += 1
            print(f"  FAIL  {name}: " + "; ".join(problems))
        elif update and problems:
            # Baselines are still written, but a WDM fire or failed assert
            # during --update means the new baseline captures a broken state.
            print(f"  UPDATE  {name}  (WARNING: " + "; ".join(problems) + ")")
        else:
            print(f"  {'UPDATE' if update else 'PASS'}  {name}")

    if update:
        meta = {"luna_version": LUNA_VERSION, "rom_sha256": sha256_file(rom)}
        baseline_path.write_text(
            json.dumps({"_meta": meta, **new_baselines}, indent=2, sort_keys=True)
            + "\n")
        print(f"wrote {baseline_path}")
        return 0

    total = len(manifest["tests"])
    print(f"\nTESTS: {total - failed}/{total} ok" + (f", {failed} failed" if failed else ""))
    return 1 if failed else 0


def main() -> int:
    ap = argparse.ArgumentParser(description="OpenSNES project test runner (luna)")
    ap.add_argument("--rom", required=True, help="the project ROM (.sfc)")
    ap.add_argument("--update", action="store_true",
                    help="rewrite test/baselines.json and the PNGs from the current ROM")
    ap.add_argument("--project", default=".", help="project root (default: cwd)")
    args = ap.parse_args()

    project = Path(args.project).resolve()
    rom = (project / args.rom).resolve()
    if not rom.is_file():
        sys.exit(f"ERROR: ROM not found: {rom} — build the project first (make).")
    return run_tests(rom, project / "test", args.update)


if __name__ == "__main__":
    sys.exit(main())
