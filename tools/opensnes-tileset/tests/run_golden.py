#!/usr/bin/env python3
"""Golden tests for opensnes-tileset (background pictures; the 1.x successor of gfx4snes -m).

The family's contract: reproduce the absorbed tool's golden suite byte for
byte. The bg case compares against gfx4snes's goldens; the two banks
cases run gfx4snes's pixel oracle (decode .pic/.pal/.map and compare every
pixel's colour with the source, with and without --rearrange — the two
silent defects of 2026-10-05); the refusals name their limit. Fixtures are
copies of gfx4snes's.

Run:  python3 tools/opensnes-tileset/tests/run_golden.py
"""
from __future__ import annotations

import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-tileset", __file__)
GFX = g.here.parents[1] / "gfx4snes" / "tests" / "golden"

g.expect_outputs("convert bg.png --colors 16 == gfx4snes -s 8 -o 16 -u 16 -p -m (the data)", ["convert", "-q", "--colors", "16", "bg.png"],
                 copy=["bg.png"], outputs=["bg.pic", "bg.pal", "bg.map"], want_dir=GFX)
g.expect_outputs("convert bg.png: the glue in asset.h's naming (.inc with DECLARE_BG_ASSET, _data.as)", ["convert", "-q", "--colors", "16", "bg.png"],
                 copy=["bg.png"], outputs=["bg.inc", "bg_data.as"])


def _read_pixels(path: Path):
    pal, rows = [], []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("palette "):
            pal = [tuple(int(h[i:i + 2], 16) for i in (0, 2, 4)) for h in line.split()[1:]]
        elif line.startswith("row "):
            rows.append([int(v) for v in line.split()[1:]])
    return pal, rows


def roundtrip(argv):
    """Every pixel of the converted picture decodes to the source's colour (gfx4snes's oracle)."""
    def run():
        pal, rows = _read_pixels(g.fixtures / "banks.pixels")
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            proc = g.run(argv, work, copy=["banks.png"])
            if g.failure(proc):
                return [g.failure(proc)]
            pic = (work / "banks.pic").read_bytes()
            palb = (work / "banks.pal").read_bytes()
            cols = [struct.unpack_from("<H", palb, i * 2)[0] for i in range(len(palb) // 2)]
            mp = (work / "banks.map").read_bytes()
            entries = [struct.unpack_from("<H", mp, i * 2)[0] for i in range(len(mp) // 2)]

        def rgb(c):
            return ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3)

        def tile(n):
            t = pic[n * 32:(n + 1) * 32]
            out = [[0] * 8 for _ in range(8)]
            for y in range(8):
                b0, b1, b2, b3 = t[y * 2], t[y * 2 + 1], t[16 + y * 2], t[16 + y * 2 + 1]
                for x in range(8):
                    bit = 7 - x
                    out[y][x] = (((b0 >> bit) & 1) | (((b1 >> bit) & 1) << 1)
                                 | (((b2 >> bit) & 1) << 2) | (((b3 >> bit) & 1) << 3))
            return out

        width = len(rows[0]) // 8
        bad = 0
        for tx in range(width):
            e = entries[tx]
            tn, pb, hf, vf = e & 0x3FF, (e >> 10) & 7, (e >> 14) & 1, (e >> 15) & 1
            t = tile(tn)
            for y in range(8):
                for x in range(8):
                    idx = t[7 - y if vf else y][7 - x if hf else x]
                    src = rows[y][tx * 8 + x]
                    if idx == 0 and src % 16 == 0:
                        continue
                    want = tuple((v >> 3) << 3 for v in pal[src])
                    if rgb(cols[pb * 16 + idx]) != want:
                        bad += 1
        return [f"{bad} pixel(s) decode to a colour that is not the source's"] if bad else []
    return run


g.check("convert banks.png --colors 48: every pixel round-trips", "pixel oracle", roundtrip(["convert", "-q", "--colors", "48", "banks.png"]))
g.check("convert banks.png --colors 48 --rearrange: every pixel round-trips", "pixel oracle after the rearrangement",
        roundtrip(["convert", "-q", "--colors", "48", "--rearrange", "banks.png"]))
g.expect_stdout("convert --json", ["convert", "--json", "--colors", "16", "bg.png"], golden="convert.json", copy=["bg.png"])
g.expect_stdout("inspect --json", ["inspect", "--json", "bg.png", "banks.png"], golden="inspect.json", copy=["bg.png", "banks.png"])
g.expect_outputs("convert --save writes bg.png.toml", ["convert", "-q", "--colors", "16", "--flip", "--save", "bg.png"],
                 copy=["bg.png"], outputs=["bg.png.toml"])
g.expect_refused("convert toomany_bg.png (1056 distinct tiles)", ["convert", "--no-palette", "toomany_bg.png"],
                 copy=["toomany_bg.png"], needles=["1024 at most"])
g.expect_refused("convert toomany_m7.png --mode 7 (tile 256 in a byte)", ["convert", "--bpp", "8", "--mode", "7", "--no-palette", "toomany_m7.png"],
                 copy=["toomany_m7.png"], needles=["exceeds 255"])
g.expect_refused("convert mixed_map.png (two palette banks in a tile)", ["convert", "--colors", "32", "mixed_map.png"],
                 copy=["mixed_map.png"], needles=["palette banks"])
g.expect_refused("convert --size 12", ["convert", "--size", "12", "bg.png"], copy=["bg.png"], needles=["8 or 16"], nothing_written=["bg.pic"])
g.expect_refused("convert --mode 2 (the map format is 1, 5, 6 or 7)", ["convert", "--mode", "2", "bg.png"], copy=["bg.png"], needles=["1 (Modes 0 to 4), 5, 6 or 7"])
sys.exit(g.report())
