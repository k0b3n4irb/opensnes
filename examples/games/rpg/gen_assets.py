#!/usr/bin/env python3
"""Generate every RPG asset from a Tiled map (the source of truth).

Outputs in res/:
  tileset.png            — 16 unique 8x8 terrain tiles (gfx4snes input)
  town.tmj               — the Tiled map: terrain + per-tile collision
                           + an Entities object layer (spawn/npc/chest)
  town_map.bin           — SNES 64x64 tilemap (4 quadrant pages) from the .tmj
  town_collision.bin     — 64x64 bytes, 0 walkable / 1 blocked, from the .tmj
  entities.inc           — C defines for the entity tile positions
  hero.png               — 8-frame walk sheet (4 dirs x 2)
  uibox.png              — 3x3 bordered dialog-box tileset

EDIT THE MAP IN TILED: open res/town.tmj, edit terrain / drag the
Entities objects, then re-run this script (it re-reads the .tmj and
regenerates only the binaries) and `make`.

Run:  python3 gen_assets.py            # regenerate .tmj too (from scratch)
      python3 gen_assets.py --keep-map # keep an edited town.tmj, rebuild bins
"""
from pathlib import Path
import json
import sys

from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
RES = HERE / "res"
RES.mkdir(exist_ok=True)

MAP_W = MAP_H = 64            # tiles
TILE = 8

# --- terrain tile table: (name, color, blocked) --------------------
# Tile index in the tileset == index here. Tiled gid = index + 1.
TILES = [
    ("grass",   (56, 132, 48),  0),
    ("grass2",  (48, 120, 44),  0),
    ("path",    (168, 140, 96), 0),
    ("wall",    (176, 148, 112), 1),
    ("roof",    (168, 64, 56),  1),
    ("door",    (64, 40, 24),   0),
    ("water",   (48, 96, 176),  1),
    ("tree",    (32, 88, 34),   1),
    ("fence",   (120, 92, 60),  1),
    ("chest",   (150, 96, 40),  0),   # walkable: you step on it
    ("flower",  (72, 148, 60),  0),
    ("sand",    (196, 176, 128), 0),
    # the blue-roofed house is the one you can walk into
    ("roof2",   (64, 92, 176),  1),
    ("door2",   (64, 40, 24),   0),   # walkable: stepping on it enters
]

# The interior is a SCENE of its own: its own tileset, its own palette,
# its own map, collision and entities. Nothing is shared with the town,
# so neither has to give up colours for the other.
INTERIOR_TILES = [
    ("floor",   (152, 112, 72), 0),
    ("floor2",  (140, 102, 66), 0),
    ("iwall",   (96, 76, 108),  1),
    ("bed",     (216, 216, 232), 1),
    ("pillow",  (240, 240, 250), 1),
    ("table",   (132, 92, 52),  1),
    ("chair",   (104, 72, 40),  1),
    ("rug",     (160, 62, 70),  0),
    ("shelf",   (116, 84, 48),  1),
    ("window",  (110, 170, 220), 1),
    ("exit",    (74, 54, 34),   0),   # the mat: step on it to leave
]
NAME2IDX = {n: i for i, (n, _, _) in enumerate(TILES)}
INAME2IDX = {n: i for i, (n, _, _) in enumerate(INTERIOR_TILES)}

HOUSE_W = HOUSE_H = 32           # the interior is exactly one screen
# The blue-roofed house in the town, and the door you walk into.
HOUSE = (41, 40)
DOOR = (HOUSE[0] + 1, HOUSE[1] + 2)
# Interior: where you appear, where the mat is, and who greets you.
HOUSE_SPAWN = (16, 20)
HOUSE_EXIT = (16, 21)
HOUSE_NPC = ((16, 8), "WELCOME TO MY HOME!")

SPAWN = (31, 34)
CHEST = (31, 28)
# Villagers. Each carries its own line in the map (a `text` property on
# the Tiled object), so adding one is a map edit, not a code edit.
NPCS = [
    ((34, 31), "WELCOME, TRAVELER!"),
    ((28, 33), "HEY! HOW ARE YOU?"),
]


# ---------------------------------------------------------------- tileset
# A BG palette is 16 colours, colour 0 being the transparent backdrop.
# These tilesets are drawn DIRECTLY in those 16 slots rather than drawn
# in RGB and quantised afterwards: with a quantiser, adding one tile
# re-derives the whole palette and every existing tile shifts hue (the
# town's paths turned pink the first time the blue roof was added).
# Explicit slots make the art additive.
TOWN_PAL = [
    (255, 0, 255),      # 0  transparent (backdrop; never used in a tile)
    (48, 120, 44),      # 1  grass dark
    (56, 132, 48),      # 2  grass
    (86, 160, 70),      # 3  grass highlight
    (32, 88, 34),       # 4  tree
    (168, 140, 96),     # 5  path
    (200, 178, 132),    # 6  path highlight / sand
    (64, 40, 24),       # 7  wood dark / doors
    (120, 92, 60),      # 8  wood / fence
    (176, 148, 112),    # 9  wall
    (168, 64, 56),      # 10 red roof
    (120, 40, 36),      # 11 red roof shade
    (64, 92, 176),      # 12 blue roof
    (40, 60, 128),      # 13 blue roof shade
    (48, 96, 176),      # 14 water
    (240, 210, 70),     # 15 gold
]

INTERIOR_PAL = [
    (255, 0, 255),      # 0  transparent
    (140, 102, 66),     # 1  floor dark
    (152, 112, 72),     # 2  floor
    (170, 128, 84),     # 3  floor highlight
    (88, 62, 34),       # 4  wood dark
    (132, 92, 52),      # 5  wood
    (168, 120, 68),     # 6  wood highlight
    (96, 76, 108),      # 7  wall
    (72, 56, 82),       # 8  wall shade
    (240, 240, 250),    # 9  linen
    (180, 180, 200),    # 10 linen shade
    (196, 88, 96),      # 11 fabric
    (160, 62, 70),      # 12 fabric shade
    (150, 210, 245),    # 13 daylight
    (70, 100, 130),     # 14 window frame
    (238, 206, 126),    # 15 brass
]

# Each tile is (fill, [(x, y, colour) …]) in PALETTE INDICES, drawn into
# an 8x8 cell. Lines are expanded to points so the art stays declarative.
def _hline(x0, x1, y, c):
    return [(x, y, c) for x in range(x0, x1 + 1)]


def _vline(x, y0, y1, c):
    return [(x, y, c) for y in range(y0, y1 + 1)]


def _box(x0, y0, x1, y1, c):
    return [(x, y, c) for y in range(y0, y1 + 1) for x in range(x0, x1 + 1)]


TOWN_ART = {
    "grass":  (2, [(2, 2, 3), (5, 5, 3)]),
    "grass2": (1, [(3, 1, 3), (6, 4, 3)]),
    "path":   (5, [(1, 4, 6), (6, 2, 6)]),
    "wall":   (9, _hline(0, 7, 3, 8) + _vline(3, 0, 3, 8)),
    "roof":   (10, _hline(0, 7, 2, 11) + _hline(0, 7, 5, 11)),
    "door":   (7, _box(1, 1, 6, 7, 8) + [(5, 4, 15)]),
    "door2":  (7, _hline(1, 6, 1, 8) + _vline(1, 1, 7, 8) + _vline(6, 1, 7, 8)),
    "water":  (14, _hline(1, 4, 2, 12) + _hline(3, 6, 5, 12)),
    "tree":   (2, _box(1, 0, 6, 4, 4) + _box(2, 5, 5, 5, 4) + _box(3, 6, 4, 7, 7)),
    "fence":  (8, _box(1, 1, 6, 6, 6) + _hline(1, 6, 3, 7)),
    "chest":  (8, _box(1, 2, 6, 7, 8) + _hline(1, 6, 2, 7) + _hline(1, 6, 3, 7)
                  + [(3, 5, 15), (4, 5, 15)]),
    "flower": (2, [(2, 2, 3), (5, 5, 3), (4, 3, 15), (3, 4, 15)]),
    "sand":   (6, [(2, 3, 5), (5, 6, 5)]),
    "roof2":  (12, _hline(0, 7, 2, 13) + _hline(0, 7, 5, 13)),
}

INTERIOR_ART = {
    "floor":  (2, _hline(0, 7, 0, 1) + [(4, 4, 3)]),
    "floor2": (1, _hline(0, 7, 0, 2) + [(2, 5, 3)]),
    "iwall":  (7, _hline(0, 7, 6, 8) + _vline(3, 0, 6, 8)),
    "bed":    (9, _hline(0, 7, 0, 11) + _hline(0, 7, 1, 11) + _hline(0, 7, 7, 10)),
    "pillow": (9, _box(1, 2, 6, 5, 9) + _hline(1, 6, 6, 10)),
    "table":  (5, _box(0, 0, 7, 2, 6) + _box(2, 5, 5, 7, 4)),
    "chair":  (5, _box(1, 0, 6, 3, 6) + _box(2, 4, 5, 7, 4)),
    "rug":    (12, _box(1, 1, 6, 6, 11) + [(3, 3, 15), (4, 4, 15)]),
    "shelf":  (5, _box(0, 1, 7, 2, 4) + _box(1, 3, 2, 6, 11)
                  + _box(4, 3, 5, 6, 13)),
    "window": (14, _box(1, 1, 6, 6, 13) + _vline(3, 1, 6, 14)),
    "exit":   (4, _box(1, 2, 6, 6, 5) + _hline(1, 6, 4, 6)),
}


def write_pal(palette, out):
    """Emit the palette as a raw SNES .pal (BGR555, little-endian).

    Committed and passed to gfx4snes with -c, so the converter must use
    THIS palette: any colour that creeps into the art without being
    declared here fails the build instead of silently re-deriving all 16
    slots and shifting every existing tile's hue.
    """
    data = bytearray()
    for (r, g, b) in palette:
        v = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3)
        data += bytes((v & 0xFF, v >> 8))
    (RES / out).write_bytes(bytes(data))
    print(f"{out} ({len(palette)} colours, imposed on the converter)")


def gen_tileset(tiles=None, art=None, palette=None, out="tileset.png"):
    """One unique 8x8 tile per terrain type, in a 16-wide grid.

    Drawn straight into a fixed 16-colour palette — see TOWN_PAL.
    """
    tiles = TILES if tiles is None else tiles
    art = TOWN_ART if art is None else art
    palette = TOWN_PAL if palette is None else palette

    rows = (len(tiles) + 15) // 16
    img = Image.new("P", (16 * TILE, rows * TILE), 0)
    flat = []
    for c in palette:
        flat += list(c)
    flat += [0] * (768 - len(flat))
    img.putpalette(flat)

    px = img.load()
    for i, (name, _, _) in enumerate(tiles):
        x0, y0 = (i % 16) * TILE, (i // 16) * TILE
        fill, points = art[name]
        for y in range(TILE):
            for x in range(TILE):
                px[x0 + x, y0 + y] = fill
        for (x, y, c) in points:
            px[x0 + x, y0 + y] = c
    img.save(RES / out)
    print(f"{out} ({len(tiles)} tiles, fixed 16-colour palette)")


# ------------------------------------------------------------- tiled map
def build_terrain():
    """Return a MAP_H x MAP_W grid of tile indices (the town layout)."""
    g = NAME2IDX["grass"]
    g2 = NAME2IDX["grass2"]
    grid = [[g if (x + y) % 2 else g2 for x in range(MAP_W)]
            for y in range(MAP_H)]

    def rect(x0, y0, x1, y1, name):
        t = NAME2IDX[name]
        for y in range(max(0, y0), min(MAP_H, y1 + 1)):
            for x in range(max(0, x0), min(MAP_W, x1 + 1)):
                grid[y][x] = t

    # fence border (1 tile ring)
    rect(0, 0, MAP_W - 1, 0, "fence")
    rect(0, MAP_H - 1, MAP_W - 1, MAP_H - 1, "fence")
    rect(0, 0, 0, MAP_H - 1, "fence")
    rect(MAP_W - 1, 0, MAP_W - 1, MAP_H - 1, "fence")

    # cross roads (2 tiles wide) through the middle
    rect(8, 31, MAP_W - 9, 32, "path")
    rect(31, 8, 32, MAP_H - 9, "path")

    # pond (top-right)
    rect(46, 10, 55, 17, "water")
    rect(45, 12, 45, 15, "water")
    rect(56, 12, 56, 15, "water")

    # three houses: 4x3 wall block with a roof row and a door.
    # The third one has a BLUE roof and a walkable door — that is the
    # visual cue that you can go in. Its interior is house.tmj.
    def house(x, y, roof="roof", door="door"):
        rect(x, y, x + 3, y, roof)
        rect(x, y + 1, x + 3, y + 2, "wall")
        rect(x + 1, y + 2, x + 1, y + 2, door)
    house(14, 37)
    house(40, 18)
    house(HOUSE[0], HOUSE[1], "roof2", "door2")

    # tree clusters
    for (tx, ty) in [(12, 16), (13, 17), (16, 18), (11, 19),
                     (48, 45), (49, 46), (52, 44), (46, 48),
                     (17, 50), (18, 51), (15, 52)]:
        rect(tx, ty, tx, ty, "tree")

    # flower patches
    for (fx, fy) in [(24, 24), (25, 25), (38, 26), (39, 27), (22, 40)]:
        rect(fx, fy, fx, fy, "flower")

    # chest tile (walkable — you step on it to open)
    rect(CHEST[0], CHEST[1], CHEST[0], CHEST[1], "chest")

    # sand patch around the spawn so the start reads clearly
    rect(SPAWN[0] - 1, SPAWN[1], SPAWN[0] + 1, SPAWN[1], "sand")
    return grid


def build_interior():
    """The inside of the blue house: one screen, no scrolling."""
    f, f2 = INAME2IDX["floor"], INAME2IDX["floor2"]
    grid = [[f if (x + y) % 2 else f2 for x in range(HOUSE_W)]
            for y in range(HOUSE_H)]

    def rect(x0, y0, x1, y1, name):
        t = INAME2IDX[name]
        for y in range(max(0, y0), min(HOUSE_H, y1 + 1)):
            for x in range(max(0, x0), min(HOUSE_W, x1 + 1)):
                grid[y][x] = t

    # The room is bounded so that the whole of it stays ABOVE the dialog
    # box (rows 22-27): you must be able to see yourself when the host
    # greets you on the way in.
    rect(0, 0, HOUSE_W - 1, 3, "iwall")           # thick back wall
    rect(0, 22, HOUSE_W - 1, HOUSE_H - 1, "iwall")   # front wall
    rect(0, 0, 1, HOUSE_H - 1, "iwall")
    rect(HOUSE_W - 2, 0, HOUSE_W - 1, HOUSE_H - 1, "iwall")

    rect(6, 1, 8, 2, "window")                    # two windows, back wall
    rect(23, 1, 25, 2, "window")
    rect(13, 2, 18, 3, "shelf")                   # a dresser between them

    rect(3, 5, 6, 5, "pillow")                    # bed, top-left corner
    rect(3, 6, 6, 9, "bed")

    rect(13, 13, 18, 14, "table")                 # table, four chairs
    rect(13, 12, 18, 12, "chair")
    rect(13, 15, 18, 15, "chair")
    rect(11, 13, 12, 14, "chair")
    rect(19, 13, 20, 14, "chair")

    rect(24, 6, 28, 10, "rug")                    # a rug in the corner

    rect(HOUSE_EXIT[0] - 1, HOUSE_EXIT[1],
         HOUSE_EXIT[0] + 1, HOUSE_EXIT[1], "exit")
    return grid


def gen_tmj(grid):
    """Write a real Tiled map (editable in Tiled) with per-tile collision
    properties and an Entities object layer."""
    tiles_props = []
    for i, (name, _, blocked) in enumerate(TILES):
        tiles_props.append({
            "id": i,
            "properties": [
                {"name": "attribute", "type": "string",
                 "value": "FF00" if blocked else "0"},
                {"name": "palette", "type": "string", "value": "0"},
                {"name": "priority", "type": "string", "value": "0"},
                {"name": "terrain", "type": "string", "value": name},
            ],
        })

    data = []
    for y in range(MAP_H):
        for x in range(MAP_W):
            data.append(grid[y][x] + 1)      # Tiled gids are 1-based

    tmj = {
        "compressionlevel": -1, "infinite": False,
        "width": MAP_W, "height": MAP_H, "tilewidth": TILE, "tileheight": TILE,
        "orientation": "orthogonal", "renderorder": "right-down",
        "tiledversion": "1.10.2", "type": "map", "version": "1.10",
        "nextlayerid": 3, "nextobjectid": 3 + len(NPCS),
        "layers": [
            {"id": 1, "name": "BG1", "type": "tilelayer",
             "width": MAP_W, "height": MAP_H, "x": 0, "y": 0,
             "opacity": 1, "visible": True, "data": data},
            {"id": 2, "name": "Entities", "type": "objectgroup",
             "x": 0, "y": 0, "opacity": 1, "visible": True,
             "draworder": "topdown",
             "objects": [
                 {"id": 1, "name": "spawn", "type": "spawn", "rotation": 0,
                  "x": SPAWN[0] * TILE, "y": SPAWN[1] * TILE,
                  "width": TILE, "height": TILE, "visible": True},
                 *[{"id": 2 + i, "name": f"villager{i}", "type": "npc",
                    "rotation": 0, "x": pos[0] * TILE, "y": pos[1] * TILE,
                    "width": TILE, "height": TILE, "visible": True,
                    "properties": [{"name": "text", "type": "string",
                                    "value": line}]}
                   for i, (pos, line) in enumerate(NPCS)],
                 {"id": 2 + len(NPCS), "name": "chest", "type": "chest",
                  "rotation": 0,
                  "x": CHEST[0] * TILE, "y": CHEST[1] * TILE,
                  "width": TILE, "height": TILE, "visible": True},
             ]},
        ],
        "tilesets": [{
            "firstgid": 1, "name": "tileset", "image": "tileset.png",
            "imagewidth": 16 * TILE, "imageheight": TILE,
            "tilewidth": TILE, "tileheight": TILE,
            "tilecount": 16, "columns": 16, "margin": 0, "spacing": 0,
            "tiles": tiles_props,
        }],
    }
    # Indented on purpose: this file is meant to be opened and edited in
    # Tiled, and read in a diff. tmx2snes minifies before parsing since
    # issue #125 (its parser used to choke on any whitespace).
    (RES / "town.tmj").write_text(json.dumps(tmj, indent=1))
    print("town.tmj (editable in Tiled: terrain + collision + entities)")


def gen_house_tmj(grid):
    """The interior as its own Tiled map — same conventions as the town:
    an `attribute` property carries collision, the Entities layer carries
    the spawn, the exit mat and the villager (with its greeting)."""
    tiles_props = [{
        "id": i,
        "properties": [
            {"name": "attribute", "type": "string",
             "value": "FF00" if blocked else "0"},
            {"name": "palette", "type": "string", "value": "0"},
            {"name": "priority", "type": "string", "value": "0"},
            {"name": "terrain", "type": "string", "value": name},
        ],
    } for i, (name, _, blocked) in enumerate(INTERIOR_TILES)]

    data = [grid[y][x] + 1 for y in range(HOUSE_H) for x in range(HOUSE_W)]
    (npos, nline) = HOUSE_NPC
    tmj = {
        "backgroundcolor": "#000000", "compressionlevel": -1,
        "infinite": False,
        "width": HOUSE_W, "height": HOUSE_H,
        "tilewidth": TILE, "tileheight": TILE,
        "orientation": "orthogonal", "renderorder": "right-down",
        "tiledversion": "1.10.2", "type": "map", "version": "1.10",
        "nextlayerid": 3, "nextobjectid": 4,
        "layers": [
            {"id": 1, "name": "BG1", "type": "tilelayer",
             "width": HOUSE_W, "height": HOUSE_H, "x": 0, "y": 0,
             "opacity": 1, "visible": True, "data": data},
            {"id": 2, "name": "Entities", "type": "objectgroup",
             "x": 0, "y": 0, "opacity": 1, "visible": True,
             "draworder": "topdown",
             "objects": [
                 {"id": 1, "name": "spawn", "type": "spawn", "rotation": 0,
                  "x": HOUSE_SPAWN[0] * TILE, "y": HOUSE_SPAWN[1] * TILE,
                  "width": TILE, "height": TILE, "visible": True},
                 {"id": 2, "name": "host", "type": "npc", "rotation": 0,
                  "x": npos[0] * TILE, "y": npos[1] * TILE,
                  "width": TILE, "height": TILE, "visible": True,
                  "properties": [{"name": "text", "type": "string",
                                  "value": nline}]},
                 {"id": 3, "name": "exit", "type": "exit", "rotation": 0,
                  "x": HOUSE_EXIT[0] * TILE, "y": HOUSE_EXIT[1] * TILE,
                  "width": TILE, "height": TILE, "visible": True},
             ]},
        ],
        "tilesets": [{
            "firstgid": 1, "name": "interior", "image": "interior.png",
            "imagewidth": 16 * TILE,
            "imageheight": ((len(INTERIOR_TILES) + 15) // 16) * TILE,
            "tilewidth": TILE, "tileheight": TILE,
            "tilecount": 16, "columns": 16, "margin": 0, "spacing": 0,
            "tiles": tiles_props,
        }],
    }
    (RES / "house.tmj").write_text(json.dumps(tmj, indent=1))
    print("house.tmj (the interior, editable in Tiled)")


def convert_house_tmj():
    """house.tmj -> a flat 32x32 SNES tilemap + collision + entity defines."""
    tmj = json.loads((RES / "house.tmj").read_text())
    w, h = tmj["width"], tmj["height"]
    layer = next(l for l in tmj["layers"] if l["type"] == "tilelayer")
    ts = tmj["tilesets"][0]
    first = ts["firstgid"]

    blocked = {}
    for t in ts.get("tiles", []):
        attr = next((p["value"] for p in t.get("properties", [])
                     if p["name"] == "attribute"), "0")
        blocked[t["id"]] = 1 if attr != "0" else 0

    tmap = bytearray()
    coll = bytearray(w * h)
    for i, gid in enumerate(layer["data"]):
        idx = gid - first
        tmap += bytes((idx & 0xFF, (idx >> 8) & 0x03))
        coll[i] = blocked.get(idx, 0)

    ents, npc = {}, None
    for l in tmj["layers"]:
        if l["type"] == "objectgroup":
            for o in l["objects"]:
                pos = (o["x"] // TILE, o["y"] // TILE)
                if o["type"] == "npc":
                    npc = (pos, next((p["value"] for p in o.get("properties", [])
                                      if p["name"] == "text"), "..."))
                else:
                    ents[o["type"]] = pos
    if npc:
        coll[npc[0][1] * w + npc[0][0]] = 1     # the host blocks its tile

    (RES / "house_map.bin").write_bytes(bytes(tmap))
    (RES / "house_collision.bin").write_bytes(bytes(coll))
    print(f"house_map.bin + house_collision.bin (blocked {sum(coll)}/{w*h})")
    return ents, npc


# ------------------------------------------------- tmj -> SNES binaries
def convert_tmj():
    """Read the Tiled map and emit the SNES tilemap, the collision map
    and the entity positions. This is the step to re-run after editing
    the map in Tiled."""
    tmj = json.loads((RES / "town.tmj").read_text())
    w, h = tmj["width"], tmj["height"]
    layer = next(l for l in tmj["layers"] if l["type"] == "tilelayer")
    ts = tmj["tilesets"][0]
    first = ts["firstgid"]

    blocked = {}
    for t in ts.get("tiles", []):
        attr = next((p["value"] for p in t.get("properties", [])
                     if p["name"] == "attribute"), "0")
        blocked[t["id"]] = 1 if attr != "0" else 0

    # SNES 64x64 tilemap = four 32x32 quadrant pages (TL, TR, BL, BR)
    tmap = bytearray()
    for qy in range(2):
        for qx in range(2):
            for ty in range(32):
                for tx in range(32):
                    gx, gy = qx * 32 + tx, qy * 32 + ty
                    idx = layer["data"][gy * w + gx] - first
                    tmap += bytes((idx & 0xFF, (idx >> 8) & 0x03))
    (RES / "town_map.bin").write_bytes(bytes(tmap))

    coll = bytearray(w * h)
    for i, gid in enumerate(layer["data"]):
        coll[i] = blocked.get(gid - first, 0)
    (RES / "town_collision.bin").write_bytes(bytes(coll))

    ents = {}
    npcs = []
    for l in tmj["layers"]:
        if l["type"] == "objectgroup":
            for o in l["objects"]:
                pos = (o["x"] // TILE, o["y"] // TILE)
                if o["type"] == "npc":
                    line = next((p["value"] for p in o.get("properties", [])
                                 if p["name"] == "text"), "...")
                    npcs.append((pos, line))
                else:
                    ents[o["type"]] = pos

    # An NPC blocks its tile (you talk to it face-to-face); a chest does
    # not (you step onto it). Entity-driven, straight from the map.
    for (nx, ny), _ in npcs:
        coll[ny * w + nx] = 1
    (RES / "town_collision.bin").write_bytes(bytes(coll))

    inc = ["/* Generated from res/town.tmj by gen_assets.py — do not edit. */"]
    for key, macro in (("spawn", "SPAWN"), ("chest", "CHEST")):
        tx, ty = ents.get(key, (0, 0))
        inc.append(f"#define {macro}_TX {tx}")
        inc.append(f"#define {macro}_TY {ty}")
    inc.append("")
    inc.append(f"#define NPC_COUNT {len(npcs)}")
    # Same macro shape `tmx2snes -e` emits, so moving this example onto
    # the tool is a Makefile change rather than a code change.
    inc.append("#define NPC_FIELDS \\\n    u8 tx; u8 ty; const char *text;")
    rows = ", \\\n    ".join(
        '{ %d, %d, "%s" }' % (pos[0], pos[1], line) for pos, line in npcs)
    inc.append("#define NPC_TABLE { \\\n    " + rows + " \\\n}")
    # the interior scene's own entities, from house.tmj
    hents, hnpc = convert_house_tmj()
    hx, hy = hents.get("spawn", (16, 24))
    ex, ey = hents.get("exit", (16, 25))
    inc.append("")
    inc.append(f"#define DOOR_TX {DOOR[0]}")
    inc.append(f"#define DOOR_TY {DOOR[1]}")
    inc.append(f"#define HOUSE_SPAWN_TX {hx}")
    inc.append(f"#define HOUSE_SPAWN_TY {hy}")
    inc.append(f"#define HOUSE_EXIT_TX {ex}")
    inc.append(f"#define HOUSE_EXIT_TY {ey}")
    inc.append(f"#define HOUSE_NPC_TX {hnpc[0][0]}")
    inc.append(f"#define HOUSE_NPC_TY {hnpc[0][1]}")
    inc.append('#define HOUSE_NPC_LINE "%s"' % hnpc[1])
    (RES / "entities.inc").write_text("\n".join(inc) + "\n")
    print(f"town_map.bin + town_collision.bin (blocked {sum(coll)}/{w*h}) "
          f"+ entities.inc {ents} npcs={[p for p, _ in npcs]}")


# ------------------------------------------------------------------ hero
def _quantize_with_transparent(img, out_path):
    q = img.convert("P", palette=Image.ADAPTIVE, colors=15)
    src = list(q.get_flattened_data())
    ncol = max(src) + 1
    flat = q.getpalette()
    pal = [tuple(flat[i * 3:i * 3 + 3]) for i in range(ncol)]
    mag = min(range(ncol),
              key=lambda i: (pal[i][0] - 255) ** 2 + pal[i][1] ** 2
              + (pal[i][2] - 255) ** 2)
    remap, nxt, newpal = {}, 1, [255, 0, 255]
    for i in range(ncol):
        if i == mag:
            remap[i] = 0
        else:
            remap[i] = nxt
            newpal += list(pal[i])
            nxt += 1
    newpal += [0, 0, 0] * (16 - nxt)
    out = Image.new("P", img.size)
    out.putdata(bytes(remap[v] for v in src))
    out.putpalette(newpal)
    out.save(out_path)


def gen_hero():
    SKIN = (240, 200, 160); HAIR = (96, 56, 24); SHIRT = (56, 96, 200)
    PANTS = (48, 48, 72); SHOE = (32, 24, 16)
    img = Image.new("RGB", (128, 16), (255, 0, 255))
    d = ImageDraw.Draw(img)

    def frame(ox, facing, step):
        d.rectangle([ox + 5, 1, ox + 10, 5], fill=SKIN)
        if facing == "up":
            d.rectangle([ox + 5, 1, ox + 10, 3], fill=HAIR)
        elif facing == "down":
            d.rectangle([ox + 5, 1, ox + 10, 2], fill=HAIR)
            d.point((ox + 6, 4), fill=(0, 0, 0)); d.point((ox + 9, 4), fill=(0, 0, 0))
        else:
            d.rectangle([ox + 5, 1, ox + 10, 2], fill=HAIR)
            d.point((ox + 6 if facing == "left" else ox + 9, 4), fill=(0, 0, 0))
        d.rectangle([ox + 4, 6, ox + 11, 11], fill=SHIRT)
        l0, l1 = (12, 14) if step == 0 else (14, 12)
        d.rectangle([ox + 5, 12, ox + 7, l0], fill=PANTS)
        d.rectangle([ox + 8, 12, ox + 10, l1], fill=PANTS)
        d.rectangle([ox + 5, l0 - 1, ox + 7, l0], fill=SHOE)
        d.rectangle([ox + 8, l1 - 1, ox + 10, l1], fill=SHOE)

    f = 0
    for facing in ("down", "up", "left", "right"):
        for step in range(2):
            frame(f * 16, facing, step)
            f += 1
    _quantize_with_transparent(img, RES / "hero.png")
    print("hero.png (8 frames)")


def gen_npc_palette():
    """The NPC reuses the hero's TILES with a different palette: green
    tunic, grey hair, darker skin. Emitted as a raw 16-color SNES
    palette matching the hero palette's slot order."""
    hero = Image.open(RES / "hero.png")
    pal = hero.getpalette()[:16 * 3]
    # map hero colors -> npc colors (same slots, recolored)
    # Deliberately NOT green: a green villager disappears against the
    # grass. Purple/red reads instantly on every terrain tile.
    swap = {
        (56, 96, 200): (176, 64, 152),     # shirt  blue -> magenta robe
        (48, 48, 72): (88, 40, 80),        # pants  -> dark plum
        (96, 56, 24): (236, 220, 140),     # hair   -> blond
        (240, 200, 160): (216, 172, 132),  # skin   -> darker
    }
    out = bytearray()
    for i in range(16):
        r, g, b = pal[i * 3:i * 3 + 3]
        r, g, b = swap.get((r, g, b), (r, g, b))
        # SNES 15-bit BGR
        v = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3)
        out += bytes((v & 0xFF, v >> 8))
    (RES / "npc.pal").write_bytes(bytes(out))
    print("npc.pal (hero tiles, recolored)")


# --------------------------------------------------------------- ui box
def gen_uibox():
    """The 9-slice dialog border PLUS the HUD icons, in one 4x3 sheet.

    Laid out so the 9-slice keeps its raster order in the first three
    columns and the fourth column carries the icons:

        TL  T   TR  heart
        L   C   R   heart-empty
        BL  B   BR  coin
    """
    FILL = (40, 32, 72); LIGHT = (120, 108, 190)
    DARK = (20, 16, 44); EDGE = (200, 196, 240)
    img = Image.new("RGB", (32, 24), (255, 0, 255))
    d = ImageDraw.Draw(img)
    # the 3x3 border block
    d.rectangle([0, 0, 23, 23], fill=FILL)
    d.line([(0, 0), (23, 0)], fill=LIGHT); d.line([(0, 0), (0, 23)], fill=LIGHT)
    d.line([(1, 1), (22, 1)], fill=LIGHT); d.line([(1, 1), (1, 22)], fill=LIGHT)
    d.line([(23, 0), (23, 23)], fill=DARK); d.line([(0, 23), (23, 23)], fill=DARK)
    d.line([(22, 1), (22, 22)], fill=DARK); d.line([(1, 22), (22, 22)], fill=DARK)
    d.rectangle([3, 3, 20, 20], outline=EDGE)

    def heart(x0, y0, body, edge):
        pts = [(2, 2), (3, 1), (4, 1), (5, 2), (6, 2),
               (1, 2), (1, 3), (2, 4), (3, 5), (4, 6), (5, 5), (6, 4), (6, 3)]
        d.rectangle([x0, y0, x0 + 7, y0 + 7], fill=FILL)
        for (px, py) in [(2, 2), (3, 2), (4, 2), (5, 2), (1, 3), (2, 3),
                         (3, 3), (4, 3), (5, 3), (6, 3), (2, 4), (3, 4),
                         (4, 4), (5, 4), (3, 5), (4, 5), (3, 6)]:
            d.point((x0 + px, y0 + py), fill=body)
        for (px, py) in [(1, 2), (6, 2), (0, 3), (7, 3), (1, 5), (6, 5)]:
            d.point((x0 + px, y0 + py), fill=edge)

    heart(24, 0, (232, 64, 72), (150, 30, 40))          # full
    heart(24, 8, (72, 60, 96), (48, 40, 68))            # empty
    # coin
    d.rectangle([24, 16, 31, 23], fill=FILL)
    d.ellipse([25, 17, 30, 22], fill=(240, 200, 72), outline=(180, 140, 30))
    d.line([(27, 18), (27, 21)], fill=(180, 140, 30))
    _quantize_with_transparent(img, RES / "uibox.png")
    print("uibox.png (9-slice border + HUD icons)")


if __name__ == "__main__":
    keep = "--keep-map" in sys.argv
    gen_tileset()
    write_pal(TOWN_PAL, "town_fixed.pal")
    gen_tileset(INTERIOR_TILES, INTERIOR_ART, INTERIOR_PAL,
                "interior.png")
    write_pal(INTERIOR_PAL, "interior_fixed.pal")
    if not keep or not (RES / "town.tmj").exists():
        gen_tmj(build_terrain())
    else:
        print("town.tmj kept (edited in Tiled)")
    if not keep or not (RES / "house.tmj").exists():
        gen_house_tmj(build_interior())
    else:
        print("house.tmj kept (edited in Tiled)")
    convert_tmj()
    gen_hero()
    gen_npc_palette()
    gen_uibox()
