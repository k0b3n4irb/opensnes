#!/usr/bin/env python3
"""Golden tests for opensnes-text (bitmap fonts; the 1.x successor of font2snes).

The family's contract: reproduce the absorbed tool's golden suite byte for
byte. The tiles and the grey palette of font.png, at 2 and 4 bpp, are
compared against font2snes's own goldens (the fixture is a copy of its
128x48 grey sheet); font2snes's C-array header has no successor — the
family ships .incbin glue — so the .inc and _data.as have goldens of their
own. font_indexed.png is the same sheet as a 2-bit palette PNG (indices
0..3), so the indexed path must give font2snes's tiles too.

Run:  python3 tools/opensnes-text/tests/run_golden.py
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-text", __file__)
FNT = g.here.parents[1] / "font2snes" / "tests" / "golden"
font4 = (g.fixtures / "font.png").read_bytes()   # font2snes's 4 bpp golden is named font4

g.expect_outputs("font font.png == font2snes font.png font.pic (2 bpp tiles + grey palette)", ["font", "-q", "font.png"],
                 copy=["font.png"], outputs=["font.pic", "font.pal"], want_dir=FNT)
g.expect_outputs("font --bpp 4 == font2snes -b 4", ["font", "-q", "--bpp", "4", "font4.png"],
                 write={"font4.png": font4}, outputs=["font4.pic", "font4.pal"], want_dir=FNT)
g.expect_outputs("font font_indexed.png (a 2-bit palette PNG): the same tiles", ["font", "-q", "font_indexed.png"],
                 copy=["font_indexed.png"], outputs=["font_indexed.pic"])
g.expect_outputs("font font.png: the glue (.inc, _data.as)", ["font", "-q", "font.png"],
                 copy=["font.png"], outputs=["font.inc", "font_data.as"])
g.expect_outputs("font --bpp 4 --save writes font.png.toml", ["font", "-q", "--bpp", "4", "--save", "font.png"],
                 copy=["font.png"], outputs=["font.png.toml"])
g.expect_stdout("font --json", ["font", "--json", "font.png"], golden="font.json", copy=["font.png"])
g.expect_stdout("inspect --json", ["inspect", "--json", "font.png", "font_indexed.png", "short.png"], golden="inspect.json",
                copy=["font.png", "font_indexed.png", "short.png"])
g.expect_refused("font short.png (8 cells, not 96)", ["font", "short.png"], copy=["short.png"],
                 needles=["exactly 96 glyphs"], nothing_written=["short.pic"])
g.expect_refused("font --bpp 3", ["font", "--bpp", "3", "font.png"], copy=["font.png"], needles=["2 or 4"], nothing_written=["font.pic"])
g.expect_refused("font --bpp 2 font16.png (indices up to 15)", ["font", "--bpp", "2", "font16.png"], copy=["font16.png"],
                 needles=["2 bpp holds 4 colours"], nothing_written=["font16.pic"])
g.expect_outputs("font --bpp 4 font16.png (a 4-bit palette PNG, indices 0/5/10/15) == font2snes -b 4", ["font", "-q", "--bpp", "4", "font4.png"],
                 write={"font4.png": (g.fixtures / "font16.png").read_bytes()}, outputs=["font4.pic"], want_dir=FNT)
sys.exit(g.report())
