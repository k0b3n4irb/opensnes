"""The Rue basse and the raccoon's den, for demos/rpg.

Imported by build.py. Everything the example's res/ holds is written here:

- tileset.png and town.tmj: the town, 512x512, composed on a grid of 16x16
  metatiles from the ground of assets/neo-lumen/tiles/ (PixelLab: two Wang
  sets, road to sidewalk and canal to sidewalk, and their variants, the bridge,
  the alleys) and the buildings and props of assets/neo-lumen/town/ (PixelLab,
  the city's 15 colours), then cut into its distinct 8x8 tiles. The sheet is
  the Tiled tileset (32 tiles a row, one tile per distinct look and
  collision), the map names them, and each tile carries its `attribute`
  (FF00 solid, 0 walkable): the build's opensnes-level writes the VRAM map,
  the collision bytes and the entities from it;
- interior.png and house.tmj: the den (assets/neo-lumen/backgrounds/den.png),
  one screen in the top-left corner of a 64x64 map, its floor plan below;
- town_fixed.pal, interior_fixed.pal: the city's palette, imposed on both;
- hero.png (Kit's walk, 15 frames of 32x32) and one 32x32 sheet per
  resident, each on its own 15 colours (res/palettes.toml plans the slots);
- uibox.png: the 9-slice frame of the HUD and the dialog box, and its icons.

Positions are in metatiles (16 px, two map cells) unless they say cells.
"""
from __future__ import annotations

import json
import random
import struct
from pathlib import Path

from PIL import Image

CITY = ["#080810", "#101828", "#182838", "#203048", "#281838", "#402858", "#604080", "#8060a8",
        "#183840", "#205058", "#307078", "#48a0a0", "#a02860", "#e05090", "#f0b050"]
INK, NIGHT, DUSK, SLATE, PLUM, MAUVE, VIOLET, LILAC, DEEP, TEAL, SEA, AQUA, WINE, PINK, AMBER = range(15)

MT = 16                     # a metatile, in pixels
W = H = 32                  # the town, in metatiles: 512x512, the 64x64 map
ROOM_W, ROOM_H = 16, 14     # the den, in metatiles: one screen


def rgb(h: str) -> tuple:
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


PAL = [rgb(c) for c in CITY]


# --- the ground, from PixelLab: two Wang sets placed by the corners, variants on the full tiles --
# tiles/wang_road_walk.png and tiles/wang_canal_walk.png: 16 tiles each, tile NW<<3 | NE<<2 |
# SW<<1 | SE at (k % 4, k / 4), 1 = the sidewalk (the upper terrain). The two share the same
# sidewalk, pixel for pixel (the second was made with the first's full tile as reference).
# walk_variants.png, road_variants.png: full tiles of either, in the same style; the road's
# last one is the centre line. bridge_alley_rail.png: the bridge deck (bare, its railing left,
# its railing right), the alley floor (two), the sidewalk with its guard rail.
WALK_WEIGHTS = [10, 6, 6, 1, 1, 1]               # plain slabs, then a crack, a crack, a puddle
ROAD_WEIGHTS = [30, 24, 1, 1, 1, 1, 1]           # plain asphalt, then tracks, cracks, a puddle, a grate
CENTRE_ROW = 9                                   # the street's centre line, a dash every other metatile


def strip(art: Path, name: str) -> list[Image.Image]:
    im = Image.open(art / "tiles" / name).convert("RGB")
    return [im.crop((x, 0, x + MT, MT)) for x in range(0, im.width, MT)]


def wang(art: Path, name: str) -> dict[int, Image.Image]:
    im = Image.open(art / "tiles" / name).convert("RGB")
    return {k: im.crop(((k % 4) * MT, (k // 4) * MT, (k % 4) * MT + MT, (k // 4) * MT + MT)) for k in range(16)}


def vertex_terrain(grid) -> list[list[str]]:
    """The terrain at each metatile corner: the canal where any of the four cells round it is
    water, else the road where any is street, else the sidewalk. The curb and the quay so
    fall inside the sidewalk's cells, half a metatile from the street; the bridge, drawn over
    its own cells, meets the street's end with no water between them."""
    v = [["walk"] * (W + 1) for _ in range(H + 1)]
    for vy in range(H + 1):
        for vx in range(W + 1):
            near = {grid[y][x] for y in (vy - 1, vy) for x in (vx - 1, vx) if 0 <= y < H and 0 <= x < W}
            if "water" in near:
                v[vy][vx] = "water"
            elif "street" in near:
                v[vy][vx] = "street"
    return v


def ground_rgb(art: Path, grid) -> Image.Image:
    """The town's ground, 512x512 in RGB. The variants come from one generator with a fixed
    seed, walked over the town row by row: the town is the same at every build, and the
    details spread (one seed per metatile, consecutive seeds, bunched them)."""
    road, canal = wang(art, "wang_road_walk.png"), wang(art, "wang_canal_walk.png")
    walk_v, road_v = strip(art, "walk_variants.png"), strip(art, "road_variants.png")
    deck, rail_l, rail_r, alley, alley2, guard = strip(art, "bridge_alley_rail.png")
    v = vertex_terrain(grid)
    rng = random.Random(8150)
    out = Image.new("RGB", (W * MT, H * MT))
    for my in range(H):
        for mx in range(W):
            c = (v[my][mx], v[my][mx + 1], v[my + 1][mx], v[my + 1][mx + 1])
            if "water" in c and "street" in c:           # where the lane meets the bridge: a curb
                c = tuple("walk" if t == "water" else t for t in c)
            water = "water" in c
            key = sum((t == "walk") << b for t, b in zip(c, (3, 2, 1, 0)))
            tile = (canal if water else road)[key]
            if key == 15:
                tile = rng.choices(walk_v, WALK_WEIGHTS)[0]
            elif key == 0 and not water:
                tile = road_v[-1] if (my == CENTRE_ROW and mx % 2 == 0) else rng.choices(road_v[:-1], ROAD_WEIGHTS)[0]
            k = grid[my][mx]
            if k == "bridge":
                tile = rail_l if grid[my][mx - 1] == "water" else rail_r if grid[my][mx + 1] == "water" else deck
            elif k == "alley":
                tile = alley if (mx + my) % 2 else alley2
            elif k == "rail":
                tile = guard
            out.paste(tile, (mx * MT, my * MT))
    return out


# --- the town -------------------------------------------------------------------------------
# (piece, metatile x, metatile y, solid metatiles of its footprint: None = all of them;
# a tuple of rows (from the piece's top) = only those)
TOWN_PIECES = [
    ("residential", 1, 0, None), ("noodle_bar", 8, 0, None), ("garage", 15, 0, None),
    ("junk_shop", 22, 0, None),
    ("vending", 28, 4, None), ("crates", 30, 4, None), ("lamp", 6, 4, (2,)), ("lamp", 20, 4, (2,)),
    ("lamp", 27, 3, (2,)),
    ("apartment", 2, 14, None), ("residential", 8, 14, None), ("residential", 19, 14, None),
    ("junk_shop", 25, 14, None),
    ("lockers", 18, 12, None), ("crates", 0, 12, None), ("lamp", 12, 11, (2,)),
    ("lamp", 0, 19, (2,)), ("lamp", 24, 19, (2,)),
    ("guard_post", 19, 26, None), ("crates", 24, 28, None), ("vending", 10, 27, None),
    ("lamp", 12, 25, (2,)), ("crates", 27, 26, None), ("lamp", 4, 25, (2,)), ("lamp", 28, 28, (2,)),
    ("crates", 2, 28, None), ("lockers", 6, 28, None), ("vending", 8, 28, None),
]
DOOR = (4, 19)            # the apartment's door, a metatile of its bottom row: walkable, it enters
TOWN_ENTITIES = [         # (type, metatile x, metatile y, properties)
    ("spawn", 11, 9, {}),
    ("npc", 11, 6, {"text": "NOODLES ARE HOT TONIGHT.", "look": 0}),
    ("npc", 18, 6, {"text": "YOUR BIKE? TOMORROW, KIT.", "look": 1}),
    ("npc", 24, 6, {"text": "CHIPS, CABLES. ALL CHEAP.", "look": 2}),
    ("npc", 16, 27, {"text": "NO DELIVERIES PAST HERE.", "look": 3}),
    ("locker", 18, 12, {}),
    ("door", DOOR[0], DOOR[1], {}),
]


def town_terrain() -> list[list[str]]:
    g = [["walk"] * W for _ in range(H)]
    for my in range(H):
        for mx in range(W):
            if 8 <= my <= 11 or (14 <= mx <= 17 and 12 <= my <= 21):
                g[my][mx] = "street"
            elif 22 <= my <= 25:
                g[my][mx] = "bridge" if 14 <= mx <= 17 else "water"
            elif my <= 4 and mx in (0, 7, 14, 21):
                g[my][mx] = "alley"
            elif my == 31:
                g[my][mx] = "rail"
    for mx in range(14, 18):                          # the lane meets the bridge across the promenade
        g[20][mx] = g[21][mx] = "street"
    return g


SOLID_GROUND = {"water", "alley", "rail"}


def load_piece(art: Path, name: str) -> Image.Image:
    p = art / "town" / f"{name}.png"
    return Image.open(p).convert("RGBA")


def nearest(c):
    return min(range(len(PAL)), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, PAL[i])))


def compose_town(art: Path):
    """The town as colour indices (512x512) and its solid metatiles."""
    grid = town_terrain()
    g = ground_rgb(art, grid)
    lut = {c: nearest(c) for c in set(g.getdata())}
    img = [[lut[g.getpixel((x, y))] for x in range(W * MT)] for y in range(H * MT)]
    solid = [[grid[my][mx] in SOLID_GROUND for mx in range(W)] for my in range(H)]
    cache = {}
    for name, mx, my, rows in TOWN_PIECES:
        im = cache.setdefault(name, load_piece(art, name))
        for y in range(im.height):
            for x in range(im.width):
                p = im.getpixel((x, y))
                if p[3] >= 128:
                    img[my * MT + y][mx * MT + x] = nearest(p[:3])
        tw, th = im.width // MT, im.height // MT
        for r in range(th):
            if rows is None or r in rows:
                for c in range(tw):
                    solid[my + r][mx + c] = True
    solid[DOOR[1]][DOOR[0]] = False
    return img, solid


# --- the den ----------------------------------------------------------------------------------
# its floor plan, a metatile a character: '#' solid, '.' floor; the mat is EXIT
ROOM_PLAN = [
    "################",
    "################",
    "################",
    "################",
    "################",
    "################",
    "################",
    "#######....#####",
    "##.........#####",
    "###........#####",
    "#####......#####",
    "######.....#####",
    "#######.########",
    "################",
]
HOUSE_ENTITIES = [
    ("arrival", 7, 11, {}),
    ("mat", 7, 12, {}),
    ("host", 6, 8, {"text": "HEY KIT. MIND THE CABLES.", "look": 4}),
]


def compose_room(art: Path):
    im = Image.open(art / "backgrounds" / "den.png").convert("RGB")
    amber = PAL[AMBER]
    img = [[0] * 256 for _ in range(256)]
    for y in range(224):
        for x in range(256):
            p = im.getpixel((x, y))
            img[y][x] = INK if p == amber and (x < 22 or x > 233 or y < 20 or y > 203) else nearest(p)
    solid = [[x >= ROOM_W or y >= ROOM_H or ROOM_PLAN[y][x] == "#" for x in range(W)] for y in range(H)]
    return img, solid


# --- cut into tiles: the sheet, the Tiled map -------------------------------------------------
def cut(img, solid, cells_w, cells_h):
    """The distinct (8x8 look, collision) pairs in first-seen order, and the cell -> index map."""
    tiles, index, cells = [], {}, []
    for cy in range(cells_h):
        for cx in range(cells_w):
            look = tuple(tuple(img[cy * 8 + y][cx * 8:cx * 8 + 8]) for y in range(8))
            key = (look, solid[cy // 2][cx // 2])
            if key not in index:
                index[key] = len(tiles)
                tiles.append(key)
            cells.append(index[key])
    return tiles, cells


def sheet(tiles) -> Image.Image:
    """32 tiles a row: the converter writes the picture's map in rows of 32 entries, and
    opensnes-level reads the tile n of a Tiled tileset as the map's entry n."""
    rows = (len(tiles) + 31) // 32
    im = Image.new("P", (256, rows * 8), 0)
    im.putpalette([v for c in PAL for v in c] + [0] * (768 - 3 * len(PAL)))
    for n, (look, _) in enumerate(tiles):
        for y in range(8):
            for x in range(8):
                im.putpixel(((n % 32) * 8 + x, (n // 32) * 8 + y), look[y][x])
    return im


def tmj(name: str, image: str, layer: str, tiles, cells, entities) -> bytes:
    props = [{"id": n, "properties": [{"name": "attribute", "type": "string", "value": "FF00" if s else "0"}]}
             for n, (_, s) in enumerate(tiles)]
    objects = []
    for i, (kind, mx, my, pr) in enumerate(entities):
        o = {"id": i + 1, "name": f"{kind}{i}", "type": kind, "rotation": 0, "x": mx * MT, "y": my * MT,
             "width": 8, "height": 8, "visible": True}
        if pr:
            o["properties"] = [{"name": k, "type": "int" if isinstance(v, int) else "string", "value": v}
                               for k, v in pr.items()]
        objects.append(o)
    rows = (len(tiles) + 31) // 32
    m = {"compressionlevel": -1, "infinite": False, "width": 64, "height": 64, "tilewidth": 8,
         "tileheight": 8, "orientation": "orthogonal", "renderorder": "right-down",
         "tiledversion": "1.10.2", "type": "map", "version": "1.10", "nextlayerid": 3,
         "nextobjectid": len(objects) + 1,
         "tilesets": [{"firstgid": 1, "name": name, "image": image, "imagewidth": 256,
                       "imageheight": rows * 8, "tilewidth": 8, "tileheight": 8,
                       "tilecount": len(tiles), "columns": 32, "margin": 0, "spacing": 0,
                       "tiles": props}],
         "layers": [{"id": 1, "name": layer, "type": "tilelayer", "width": 64, "height": 64, "x": 0, "y": 0,
                     "opacity": 1, "visible": True, "data": [c + 1 for c in cells]},
                    {"id": 2, "name": "Entities", "type": "objectgroup", "draworder": "topdown", "x": 0,
                     "y": 0, "opacity": 1, "visible": True, "objects": objects}]}
    return (json.dumps(m, indent=1) + "\n").encode()


def pal_bytes() -> bytes:
    words = [(r >> 3) | (g >> 3) << 5 | (b >> 3) << 10 for r, g, b in PAL] + [0]
    return struct.pack("<16H", *words)


# --- the characters ---------------------------------------------------------------------------
RESIDENTS = ["owl", "frog", "rat", "pangolin", "raccoon"]   # the `look` property's order


def indexed(frames: list[Image.Image], cols: int, rows: int) -> Image.Image:
    colours = sorted({p[:3] for f in frames for p in f.getdata() if p[3] >= 128}, key=sum)
    if len(colours) > 15:
        q = Image.new("RGB", (32 * len(frames), 32))
        for i, f in enumerate(frames):
            q.paste(f.convert("RGB"), (32 * i, 0))
        colours = sorted({c for c in q.quantize(15, method=Image.Quantize.MEDIANCUT).convert("RGB").getdata()},
                         key=sum)
    im = Image.new("P", (32 * cols, 32 * rows), 0)
    im.putpalette([0, 0, 0] + [v for c in colours for v in c] + [0] * (765 - 3 * len(colours)))
    for n, f in enumerate(frames):
        for y in range(32):
            for x in range(32):
                p = f.getpixel((x, y))
                if p[3] >= 128:
                    k = min(range(len(colours)), key=lambda j: sum((a - b) ** 2 for a, b in zip(p[:3], colours[j])))
                    im.putpixel(((n % cols) * 32 + x, (n // cols) * 32 + y), k + 1)
    return im


def hero(art: Path) -> Image.Image:
    """Kit's walk (characters/topdown/kit_walk_sheet.png): down, up, right, five frames each,
    four to a 128-px row, so frame f starts at OBJ tile (f % 4) * 4 + (f / 4) * 64."""
    walk = Image.open(art / "characters" / "topdown" / "kit_walk_sheet.png").convert("RGBA")
    frames = [walk.crop((0, 32 * i, 32, 32 * i + 32)) for i in range(15)]
    return indexed(frames, 4, 4)


def resident(art: Path, who: str) -> Image.Image:
    return indexed([Image.open(art / "characters" / "topdown" / f"{who}_south.png").convert("RGBA")], 1, 1)


# --- the HUD and the dialog box -------------------------------------------------------------
UI = ["#000000", "#101828", "#307078", "#48a0a0", "#b0f8ff", "#a02860", "#e05090", "#ffd0f0",
      "#f0b050", "#806020", "#203048"]


def uibox() -> Image.Image:
    """A 4x3 sheet of 8x8 tiles: the 9-slice frame in the first three columns (a cyan
    terminal border, a night fill), a heart, an empty heart and a credit chip in the fourth."""
    im = Image.new("P", (32, 24), 0)
    im.putpalette([v for c in UI for v in rgb(c)] + [0] * (768 - 3 * len(UI)))

    def frame(x, y, ox, oy):
        edge = {0: 0, 1: 1, 2: 2}
        gx = 0 if ox == 0 and x < 3 else 2 if ox == 2 and x > 4 else 1
        gy = 0 if oy == 0 and y < 3 else 2 if oy == 2 and y > 4 else 1
        d = min(x if ox == 0 else 7 - x if ox == 2 else 9, y if oy == 0 else 7 - y if oy == 2 else 9)
        if gx != 1 or gy != 1:
            if ox == 0 and oy == 0 and x + y < 2 or ox == 2 and oy == 0 and (7 - x) + y < 2 \
                    or ox == 0 and oy == 2 and x + (7 - y) < 2 or ox == 2 and oy == 2 and (7 - x) + (7 - y) < 2:
                return 0
        del edge
        return {0: 2, 1: 3, 2: 4}.get(d, 1) if d <= 2 else 1
    for oy in range(3):
        for ox in range(3):
            for y in range(8):
                for x in range(8):
                    im.putpixel((ox * 8 + x, oy * 8 + y), frame(x, y, ox, oy))
    heart = ["........", ".##.##..", "#++#++#.", "#+++++#.", ".#+++#..", "..#+#...", "...#....", "........"]
    for row, (shape, full) in enumerate(((heart, True), (heart, False))):
        for y in range(8):
            for x in range(8):
                ch = shape[y][x]
                c = 0 if ch == "." else (5 if ch == "#" else (6 if full else 10))
                if full and ch == "+" and (x, y) in ((1, 2), (2, 2)):
                    c = 7
                im.putpixel((24 + x, row * 8 + y), c)
    chip = ["..####..", ".#++++#.", "#+#++#+#", "#++##++#", "#++##++#", "#+#++#+#", ".#++++#.", "..####.."]
    for y in range(8):
        for x in range(8):
            ch = chip[y][x]
            im.putpixel((24 + x, 16 + y), 0 if ch == "." else 9 if ch == "#" else 8)
    return im


def png(im: Image.Image) -> bytes:
    import io
    buf = io.BytesIO()
    im.save(buf, format="PNG", optimize=False)
    return buf.getvalue()


def build_rpg(art: Path) -> dict[str, bytes]:
    out: dict[str, bytes] = {}
    img, solid = compose_town(art)
    tiles, cells = cut(img, solid, 64, 64)
    out["tileset.png"] = png(sheet(tiles))
    out["town.tmj"] = tmj("tileset", "tileset.png", "street", tiles, cells, TOWN_ENTITIES)
    room, rsolid = compose_room(art)
    big = [[INK] * 512 for _ in range(512)]
    for y in range(256):
        big[y][:256] = room[y]
    rtiles, rcells = cut(big, rsolid, 64, 64)
    out["interior.png"] = png(sheet(rtiles))
    out["house.tmj"] = tmj("interior", "interior.png", "den", rtiles, rcells, HOUSE_ENTITIES)
    out["town_fixed.pal"] = out["interior_fixed.pal"] = pal_bytes()
    out["hero.png"] = png(hero(art))
    for who in RESIDENTS:
        out[f"{who}.png"] = png(resident(art, who))
    out["uibox.png"] = png(uibox())
    out["_report"] = f"town {len(tiles)} tiles, den {len(rtiles)} tiles".encode()
    return out


def preview(art: Path) -> Image.Image:
    """The town in RGB, for a look before the build."""
    img, _ = compose_town(art)
    im = Image.new("RGB", (512, 512))
    im.putdata([PAL[c] for row in img for c in row])
    return im
