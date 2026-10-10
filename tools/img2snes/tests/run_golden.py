#!/usr/bin/env python3
"""Golden-output tests for img2snes (gaps review P4, 2026-09-14).

The quantiser is deterministic (no random seeds), so a diff is a behaviour
change. Fixture: fixtures/rgb.png — a synthetic 64x32 RGB image generated
once by PIL: a two-axis colour gradient plus two flat rectangles, so
quantisation to 16 colours has to merge shades and keep the flat regions.

Run:  python3 tools/img2snes/tests/run_golden.py
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("img2snes", __file__)
# (flags, output file byte-compared against golden/)
for flags, out in [(["-c", "16"], "rgb_16.png"),                   # 16-colour quantisation
                   (["-c", "4", "--round-snes"], "rgb_4_snes.png")]:  # 4 colours, palette rounded to BGR555
    g.expect_outputs(f"rgb.png [{' '.join(flags)}] -> {out}",
                     ["-q", "-i", "rgb.png", "-o", out, *flags], copy=["rgb.png"], outputs=[out])
sys.exit(g.report())
