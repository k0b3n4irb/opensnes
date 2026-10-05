#!/usr/bin/env python3
"""Golden-output tests for tmx2snes.

What the cases pin:

  - the `-Q` quadrant tilemap. A SNES 64x64 background is four 32x32
    pages; the golden is byte-identical to the hand-written Python
    converter `examples/games/rpg/gen_assets.py` used before this flag
    existed, which is what makes it a meaningful reference.
  - the `-e` entity header: object types become macro prefixes, custom
    properties tables, and a lone object of its type also gets scalars.
  - the `-C` per-cell collision grid, identical to the RPG's hand-written
    converter in 4094 of 4096 cells (the two villagers' own tiles are game
    logic, which the tool deliberately does not block).
  - that a pretty-printed .tmj parses at all (issue #125): the fixture is
    indented, which cute_tiled cannot read without the minify pass.
  - three refusals (2026-10-05, build audit S15), each a patch of town.tmj:
    a rotated tile (Tiled's diagonal flip, bit 29), a tile id past the 1024
    the SNES tilemap addresses, a second tileset. Until then the id was
    masked onto another tile, the rotation dropped, the second tileset read
    as the first — three silent wrong maps.

Fixture provenance: fixtures/town.tmj and fixtures/tileset.map are the
RPG template's own map (`examples/games/rpg/res/`), original work.

Run:  python3 tools/tmx2snes/tests/run_golden.py
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("tmx2snes", __file__)
INPUTS = ["town.tmj", "tileset.map"]
# (extra flags, expected outputs — all byte-compared)
CASES = [
    (["-e", "-Q", "-C"], ["BG1.q16", "BG1.c16", "town.inc", "BG1.m16", "town.b16", "town.o16"]),
    ([], ["BG1.m16", "town.b16", "town.o16"]),   # no flags: the historical outputs untouched
]
for flags, outputs in CASES:
    g.expect_outputs(f"town.tmj [{' '.join(flags) or 'no flags'}]", [*flags, *INPUTS],
                     copy=INPUTS, outputs=outputs)


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
    g.expect_refused(f"refused: {name}", ["bad.tmj", "tileset.map"], copy=["tileset.map"],
                     write={"bad.tmj": json.dumps(bad)}, needles=[expect])
sys.exit(g.report())
