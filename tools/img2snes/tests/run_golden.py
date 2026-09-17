#!/usr/bin/env python3
"""Golden-output tests for img2snes (gaps review P4, 2026-09-14).

Each case copies the fixture PNG into a temp dir, runs img2snes and
byte-compares the produced indexed PNG against golden/. The quantiser is
deterministic (no random seeds), so a diff is a behaviour change: a
regression, or an intentional change to re-golden after reviewing.

Fixture: fixtures/rgb.png — a synthetic 64x32 RGB image generated once by
PIL: a two-axis colour gradient plus two flat rectangles, so quantisation
to 16 colours has to merge shades and keep the flat regions.

Run:  python3 tools/img2snes/tests/run_golden.py
"""
from __future__ import annotations

import filecmp
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TOOL = REPO / "bin" / "img2snes"

# (fixture, flags, output file byte-compared against golden/)
CASES = [
    ("rgb.png", ["-c", "16"], "rgb_16.png"),                 # 16-colour quantisation
    ("rgb.png", ["-c", "4", "--round-snes"], "rgb_4_snes.png"),  # 4 colours, palette rounded to BGR555
]


def run_case(fixture: str, flags: list[str], out: str) -> list[str]:
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        shutil.copy(HERE / "fixtures" / fixture, work / fixture)
        proc = subprocess.run([str(TOOL), "-q", "-i", fixture, "-o", out, *flags],
                              cwd=work, capture_output=True, text=True, timeout=60)
        if proc.returncode != 0:
            return [f"exit {proc.returncode}: {(proc.stderr or proc.stdout).strip()[:200]}"]
        got, want = work / out, HERE / "golden" / out
        if not got.is_file():
            return [f"{out}: not produced"]
        if not filecmp.cmp(got, want, shallow=False):
            return [f"{out}: differs from golden ({got.stat().st_size} vs {want.stat().st_size} bytes)"]
    return []


def main() -> int:
    if not TOOL.is_file():
        sys.exit(f"ERROR: {TOOL} not found — build the tools (make tools) first")
    failed = 0
    for fixture, flags, out in CASES:
        errs = run_case(fixture, flags, out)
        label = f"{fixture} [{' '.join(flags)}] -> {out}"
        if errs:
            failed += 1
            print(f"  FAIL {label}: " + "; ".join(errs))
        else:
            print(f"  PASS {label}")
    print(f"\nimg2snes golden: {len(CASES) - failed}/{len(CASES)} ok")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
