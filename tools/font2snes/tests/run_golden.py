#!/usr/bin/env python3
"""Golden-output tests for font2snes (gaps review P4, 2026-09-14).

Fixture: fixtures/font.png — a synthetic 128x48 sheet (16 x 6 cells of
8x8, the 96 glyphs of ASCII 32-127) generated once by PIL: every cell has
a distinct bit pattern and one of four grey levels, so both 2bpp planes
(and all four 4bpp levels) carry data and no two glyphs dedupe.

Run:  python3 tools/font2snes/tests/run_golden.py
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("font2snes", __file__)
# (flags, output argument, files byte-compared against golden/)
CASES = [
    ([], "font.pic", ["font.pic", "font.pal"]),            # 2bpp binary + its 4-colour palette
    (["-b", "4"], "font4.pic", ["font4.pic", "font4.pal"]),  # 4bpp binary + 16-colour palette
    (["-c"], "font.h", ["font.h"]),                        # C header
]
for flags, out_arg, outputs in CASES:
    g.expect_outputs(f"font.png [{' '.join(flags) or 'no flags'}] -> {out_arg}",
                     [*flags, "font.png", out_arg], copy=["font.png"], outputs=outputs)
sys.exit(g.report())
