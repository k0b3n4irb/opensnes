#!/usr/bin/env python3
"""Golden tests of opensnes-level.

The data outputs (.m16 / .q16 / .c16 / .b16 / .o16) must equal tmx2snes's
goldens byte for byte: the converter is the same code (tools/lib/tmx2snes/src/level.c).
The glue (.inc, _data.as) and the entities header are the family's own.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-level", __file__)
TMX = Path(__file__).resolve().parents[2] / "lib" / "tmx2snes" / "tests" / "golden"
INPUTS = ["town.tmj", "tileset.map"]
ALL = ["convert", "-q", "--tileset", "tileset.map", "--entities", "--quadrant", "--collision", "town.tmj"]

g.expect_outputs("convert --entities --quadrant --collision == tmx2snes -e -Q -C (the data)", ALL,
                 copy=INPUTS, outputs=["BG1.q16", "BG1.c16", "BG1.m16", "town.b16", "town.o16"], want_dir=TMX)
g.expect_outputs("convert: the glue (.inc, _data.as) and the entities header", ALL,
                 copy=INPUTS, outputs=["town.inc", "town_data.as", "town_entities.inc"])
g.expect_outputs("convert (no flags) == tmx2snes (no flags)", ["convert", "-q", "--tileset", "tileset.map", "town.tmj"],
                 copy=INPUTS, outputs=["BG1.m16", "town.b16", "town.o16"], want_dir=TMX)
g.expect_stdout("convert --json", ["convert", "--json", "--tileset", "tileset.map", "--entities", "town.tmj"], golden="convert.json", copy=INPUTS)
g.expect_stdout("inspect", ["inspect", "town.tmj"], golden="inspect.txt", copy=["town.tmj"])
g.expect_refused("refused: no --tileset", ["convert", "town.tmj"], copy=INPUTS, needles=["no --tileset"])


def _rotated(m):
    m["layers"][0]["data"][0] |= 0x20000000


def _big_gid(m):
    m["layers"][0]["data"][0] = 1100


def _two_tilesets(m):
    m["tilesets"].append({"firstgid": 17, "name": "second", "image": "second.png",
                          "imagewidth": 64, "imageheight": 8, "tilewidth": 8,
                          "tileheight": 8, "tilecount": 8, "columns": 8,
                          "margin": 0, "spacing": 0})
    m["layers"][0]["data"][0] = 17


town = json.loads((g.fixtures / "town.tmj").read_text(encoding="utf-8"))
for name, patch, expect in [("rotated tile", _rotated, "is rotated"),
                            ("tile id past 1024", _big_gid, "uses id 1100"),
                            ("second tileset", _two_tilesets, "one tileset per map")]:
    bad = json.loads(json.dumps(town))
    patch(bad)
    g.expect_refused(f"refused: {name}", ["convert", "--tileset", "tileset.map", "bad.tmj"], copy=["tileset.map"],
                     write={"bad.tmj": json.dumps(bad)}, needles=[expect])

# The .q16 is written in VRAM tiles, through the tileset's table: once the
# converter has merged duplicate tiles, a Tiled tile id is not a VRAM tile.
# Until 2026-10-23 the quadrant map wrote the Tiled id, which equals the VRAM
# tile only while the table is the identity (the fixture's is, on the tiles
# its map uses); a 32-wide picture of 822 tiles came out as garbage.
def _quadrant_through_table():
    import struct
    import tempfile
    table = [((n * 7 + 3) % 16) for n in range(1024)]
    table[2] |= 0x4000                      # stored mirrored by the converter
    table[5] |= 0x0C00                      # in palette bank 3
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["convert", "-q", "--tileset", "perm.map", "--quadrant", "town.tmj"], work,
                     copy=["town.tmj"], write={"perm.map": struct.pack("<1024H", *table)})
        err = g.failure(proc)
        if err:
            return [err]
        q = (work / "BG1.q16").read_bytes()
        data = town["layers"][0]["data"]
        errs = []
        for cy in range(64):
            for cx in range(64):
                gid = data[cy * 64 + cx]
                page = (cy // 32) * 2 + cx // 32
                got = struct.unpack_from("<H", q, (page * 1024 + (cy % 32) * 32 + cx % 32) * 2)[0]
                t = (gid & 0x0FFFFFFF) - 1
                want = 0 if not gid else table[t] & 0xDFFF
                if gid & 0x80000000:
                    want ^= 0x4000
                if gid & 0x40000000:
                    want ^= 0x8000
                if got & 0xDFFF != want and len(errs) < 3:
                    errs.append(f"cell ({cx},{cy}): tile id {t} -> {got:#06x}, the table gives {want:#06x}")
        return errs


town = json.loads((g.fixtures / "town.tmj").read_text(encoding="utf-8"))
g.check("convert --quadrant: the map goes through the tileset's table", "every cell is the table's entry",
        _quadrant_through_table)

# The layer's name names the output file: a map from someone else must not
# choose where the tool writes, nor overflow the path buffer (audit 07-1).
for name, lname in [("layer name with ..", "../escape/PWN"),
                    ("layer name with a slash", "sub/BG1"),
                    ("layer name with a backslash", "sub\\BG1"),
                    ("layer name with a drive", "C:BG1"),
                    ("empty layer name", ""),
                    ("layer name of 1000 characters", "B" * 1000)]:
    bad = json.loads(json.dumps(town))
    bad["layers"][0]["name"] = lname
    g.expect_refused(f"refused: {name}", ["convert", "--tileset", "tileset.map", "bad.tmj"], copy=["tileset.map"],
                     write={"bad.tmj": json.dumps(bad)}, needles=["is not a plain file name"])
sys.exit(g.report())
