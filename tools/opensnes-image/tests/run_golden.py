#!/usr/bin/env python3
"""Golden tests for opensnes-image (HiColor pictures and the Mode 7 perspective tables).

`perspective` with its defaults (48 angles, 224 lines, zoom 80) must give
krom (Peter Lemon)'s Perspective demo tables byte for byte — golden/
perspective_{cos,sin,nsin}.hdma are those bytes, which the example
`mode7/perspective_rotate` shipped verbatim until the tool regenerated them
(32256 of 32256 entries are the rounded formula; the nearest entry to a
rounding boundary is 0.0005 away, so no libm can flip one). `hicolor` has
no byte-exact ancestor (hicolor64.py quantized with Pillow): its golden is
the tool's own output on a synthetic 256x224 scene, and the pixel oracle
checks every tile's colours decode to a palette entry of its own segment.

Run:  python3 tools/opensnes-image/tests/run_golden.py
"""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-image", __file__)
PERSPECTIVE_TOML = 'tool = "opensnes-image"\n\n[perspective]\n'

g.expect_outputs("perspective (defaults) == krom's Perspective tables", ["perspective", "-q", "perspective.toml"],
                 write={"perspective.toml": PERSPECTIVE_TOML},
                 outputs=["perspective_cos.hdma", "perspective_sin.hdma", "perspective_nsin.hdma", "perspective.inc", "perspective_data.as"])
g.expect_outputs("perspective --angles 4 --lines 8 --zoom 2 --save", ["perspective", "-q", "--angles", "4", "--lines", "8", "--zoom", "2", "--save", "small.toml"],
                 outputs=["small.toml", "small_cos.hdma", "small_sin.hdma", "small_nsin.hdma", "small.inc"])
g.expect_stdout("perspective --json", ["perspective", "--json", "perspective.toml"], golden="perspective.json")
g.expect_refused("perspective --zoom 200 (beyond a 16-bit term)", ["perspective", "--zoom", "200", "perspective.toml"],
                 needles=["1 to 127"], nothing_written=["perspective_cos.hdma"])
g.expect_refused("perspective without a settings file", ["perspective", "scene.png"], copy=["scene.png"], needles=["one settings file"])

g.expect_outputs("hicolor scene.png: tiles, palettes, glue", ["hicolor", "-q", "scene.png"], copy=["scene.png"],
                 outputs=["scene.pic", "scene.pal", "scene.inc", "scene_data.as"])
g.expect_outputs("hicolor --passes 0 --save writes scene.png.toml", ["hicolor", "-q", "--passes", "0", "--save", "scene.png"], copy=["scene.png"],
                 outputs=["scene.png.toml"])
g.expect_stdout("hicolor --json", ["hicolor", "--json", "scene.png"], golden="hicolor.json", copy=["scene.png"])
g.expect_stdout("inspect --json", ["inspect", "--json", "scene.png"], golden="inspect.json", copy=["scene.png"])
g.expect_refused("hicolor of a 128x48 picture", ["hicolor", "font.png"],
                 write={"font.png": (g.here.parents[1] / "opensnes-text" / "tests" / "fixtures" / "font.png").read_bytes()},
                 needles=["256x224"], nothing_written=["font.pic"])


def hicolor_oracle():
    """Every pixel of every tile names a colour of its own segment's palette, and the
    layout is krom's: 28 rows x 4 segments x 8 tiles, 16 colours per segment, black first."""
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["hicolor", "-q", "scene.png"], work, copy=["scene.png"])
        if g.failure(proc):
            return [g.failure(proc)]
        pic = (work / "scene.pic").read_bytes()
        pal = (work / "scene.pal").read_bytes()
    errs = []
    if len(pic) != 28 * 4 * 8 * 32:
        errs.append(f"scene.pic is {len(pic)} bytes, not 28672")
    if len(pal) != 28 * 4 * 32:
        errs.append(f"scene.pal is {len(pal)} bytes, not 3584")
    if errs:
        return errs
    bad_black = sum(1 for s in range(112) if pal[s * 32:s * 32 + 2] != b"\0\0")
    if bad_black:
        errs.append(f"{bad_black} segment palettes do not start with black")
    zero_pixels = 0
    for t in range(896):
        tile = pic[t * 32:(t + 1) * 32]
        for y in range(8):
            b0, b1, b2, b3 = tile[y * 2], tile[y * 2 + 1], tile[16 + y * 2], tile[16 + y * 2 + 1]
            for x in range(8):
                bit = 7 - x
                idx = ((b0 >> bit) & 1) | (((b1 >> bit) & 1) << 1) | (((b2 >> bit) & 1) << 2) | (((b3 >> bit) & 1) << 3)
                if idx == 0:
                    zero_pixels += 1
    if zero_pixels:
        errs.append(f"{zero_pixels} pixels use index 0, the reserved black")
    return errs


g.check("hicolor scene.png: layout and index oracle", "28x4x8 tiles, black at 0, no pixel at 0", hicolor_oracle)
sys.exit(g.report())
