#!/usr/bin/env python3
"""Golden-output tests for font2snes (gaps review P4, 2026-09-14).

Each case copies the fixture font PNG into a temp dir, runs font2snes and
byte-compares every produced file against golden/. Output is deterministic
(no timestamps), so a diff is a behaviour change: a regression, or an
intentional change to re-golden after reviewing the diff.

Fixture: fixtures/font.png — a synthetic 128x48 sheet (16 x 6 cells of
8x8, the 96 glyphs of ASCII 32-127) generated once by PIL: every cell has
a distinct bit pattern and one of four grey levels, so both 2bpp planes
(and all four 4bpp levels) carry data and no two glyphs dedupe.

Run:  python3 tools/font2snes/tests/run_golden.py
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
TOOL = REPO / "bin" / "font2snes"

# (fixture, flags, output argument, files byte-compared against golden/)
CASES = [
    ("font.png", [], "font.pic", ["font.pic", "font.pal"]),            # 2bpp binary + its 4-colour palette
    ("font.png", ["-b", "4"], "font4.pic", ["font4.pic", "font4.pal"]),  # 4bpp binary + 16-colour palette
    ("font.png", ["-c"], "font.h", ["font.h"]),                        # C header
]


def run_case(fixture: str, flags: list[str], out_arg: str, outputs: list[str]) -> list[str]:
    errs = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        shutil.copy(HERE / "fixtures" / fixture, work / fixture)
        proc = subprocess.run([str(TOOL), *flags, fixture, out_arg],
                              cwd=work, capture_output=True, text=True, timeout=60)
        if proc.returncode != 0:
            return [f"exit {proc.returncode}: {(proc.stderr or proc.stdout).strip()[:200]}"]
        for out in outputs:
            got, want = work / out, HERE / "golden" / out
            if not got.is_file():
                errs.append(f"{out}: not produced")
            elif not filecmp.cmp(got, want, shallow=False):
                errs.append(f"{out}: differs from golden ({got.stat().st_size} vs {want.stat().st_size} bytes)")
    return errs


def main() -> int:
    if not TOOL.is_file():
        sys.exit(f"ERROR: {TOOL} not found — build the tools (make tools) first")
    failed = 0
    for fixture, flags, out_arg, outputs in CASES:
        errs = run_case(fixture, flags, out_arg, outputs)
        label = f"{fixture} [{' '.join(flags) or 'no flags'}] -> {out_arg}"
        if errs:
            failed += 1
            print(f"  FAIL {label}: " + "; ".join(errs))
        else:
            print(f"  PASS {label} ({len(outputs)} outputs match)")
    print(f"\nfont2snes golden: {len(CASES) - failed}/{len(CASES)} ok")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
