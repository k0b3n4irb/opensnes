#!/usr/bin/env python3
"""Golden tests of opensnes-level.

The data outputs (.m16 / .q16 / .c16 / .b16 / .o16) must equal tmx2snes's
goldens byte for byte: the converter is the same code (tools/tmx2snes/src/level.c).
The glue (.inc, _data.as) and the entities header are the family's own.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-level", __file__)
TMX = Path(__file__).resolve().parents[2] / "tmx2snes" / "tests" / "golden"
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
sys.exit(g.report())
