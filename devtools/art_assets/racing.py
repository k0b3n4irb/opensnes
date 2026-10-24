"""The Mode 7 race: a circuit on the rooftops round a lagoon, and three hover-bikes.

Imported by build.py. The ground is 1024x1024 in at most 256 distinct 8x8
tiles, laid out like the city (mode7.py) from two Wang tilesets of 16x16
tiles from PixelLab, placed by the corners of a vertex grid every 16 px:

- tiles/wang_track_gravel.png: the asphalt and the rooftop gravel, a red and
  white curb between them;
- tiles/wang_gravel_lagoon.png: the same gravel and the lagoon's water, a
  quay between them.

The circuit's centre line is the rounded rectangle with a chicane that the
example always had (the start line is still at (312, 272)). A vertex is
asphalt within ROAD_HALF of it, water in the infield further than LAGOON_GAP
from it and on the map's border, gravel elsewhere; so the curb and the quay
never meet in one tile. The start line and two boost pads are drawn here on
the asphalt, and the rooftop props (props/roof_*.png, 32x32 from PixelLab)
are scattered on the gravel, on the 16-px grid so that the gravel behind each
copy is the same and a prop costs its 16 tiles once.

Beside the picture: the class of each 8x8 tile (0 road, 1 gravel, 2 water or
a prop, 3 boost), and the two lanes the rivals ride, 256 points each with their
heading.

The bikes (characters/racebike_*.png, 32x32 from above, pointing up) share
one palette. A rival is seen at any angle from the camera, so its bike is
turned here in 22.5-degree steps from 0 to 90 degrees; the sprite's flips
give the other twelve directions. The turn is RotSprite's: the sprite is
enlarged 8 times with Scale2x, turned without filtering and brought back
by the commonest colour of each 8x8 block, which keeps one-pixel lines whole.
"""
from __future__ import annotations

import math
import random
from pathlib import Path

from PIL import Image, ImageDraw

W = 1024
CELLS = 64                       # 16-px cells across the map
ROAD_HALF = 40                   # asphalt within this distance of the centre line
LAGOON_GAP = 96                  # the infield's water starts this far from it
LANE = 18                        # the rivals ride this far either side of it
WAYPOINTS = 256
CHECKER = ((240, 240, 240), (16, 16, 16))
AMBER = (240, 176, 80)
MAGENTA = (224, 80, 144)
PROPS = ["helipad", "aircon", "tank", "solar", "aircon_violet", "dish"]
PROP_STEP = 6                    # cells between two props, before the jitter
BOOSTS = [0.35, 0.60]            # where the pads lie, as a fraction of the lap: the right and the bottom straight


def center_path(t: float) -> tuple[float, float]:
    """t in [0, 1) -> (x, y) on the circuit's centre line, clockwise from the start line."""
    rx, ry, r = 320, 240, 120    # half-extents and corner radius
    cx, cy = 512, 512
    sx, sy = 2 * (rx - r), 2 * (ry - r)
    corner = math.pi * r / 2
    d = t * (2 * sx + 2 * sy + 4 * corner)
    segs = [
        ("s", sx, (cx - (rx - r), cy - ry), (1, 0)),
        ("c", corner, (cx + (rx - r), cy - ry + r), -90),
        ("s", sy, (cx + rx, cy - (ry - r)), (0, 1)),
        ("c", corner, (cx + (rx - r), cy + (ry - r)), 0),
        ("s", sx, (cx + (rx - r), cy + ry), (-1, 0)),
        ("c", corner, (cx - (rx - r), cy + (ry - r)), 90),
        ("s", sy, (cx - rx, cy + (ry - r)), (0, -1)),
        ("c", corner, (cx - (rx - r), cy - (ry - r)), 180),
    ]
    for kind, length, origin, arg in segs:
        if d <= length:
            if kind == "s":
                x, y = origin[0] + arg[0] * d, origin[1] + arg[1] * d
            else:
                a = math.radians(arg) + (d / length) * (math.pi / 2)
                x, y = origin[0] + r * math.cos(a), origin[1] + r * math.sin(a)
            if 380 < x < 644 and y < 400:            # the chicane on the top straight
                y += 60 * math.exp(-((x - 512) / 70.0) ** 2)
            return x, y
        d -= length
    return center_path(0)


def stamp_mask(pts, radius: int) -> Image.Image:
    m = Image.new("L", (W, W), 0)
    d = ImageDraw.Draw(m)
    for x, y in pts:
        d.ellipse([x - radius, y - radius, x + radius, y + radius], fill=255)
    return m


def wang(sheet: Image.Image) -> dict[int, Image.Image]:
    """The 16 tiles of a sheet, keyed NW<<3 | NE<<2 | SW<<1 | SE (1 = the upper terrain)."""
    return {k: sheet.crop(((k % 4) * 16, (k // 4) * 16, (k % 4) * 16 + 16, (k // 4) * 16 + 16)) for k in range(16)}


def grid(pts) -> list[list[str]]:
    """The terrain at each vertex: a asphalt, g gravel, w water."""
    road = stamp_mask(pts, ROAD_HALF).load()
    near = stamp_mask(pts, LAGOON_GAP).load()
    inside = Image.new("L", (W, W), 0)
    ImageDraw.Draw(inside).polygon(pts, fill=255)
    inside = inside.load()
    n = CELLS + 1
    v = [["g"] * n for _ in range(n)]
    for y in range(n):
        for x in range(n):
            px, py = min(x * 16, W - 1), min(y * 16, W - 1)
            if x < 2 or y < 2 or x > CELLS - 2 or y > CELLS - 2:
                v[y][x] = "w"
            elif road[px, py]:
                v[y][x] = "a"
            elif inside[px, py] and not near[px, py]:
                v[y][x] = "w"
    return v


def chevron(direction: tuple[int, int]) -> list[tuple[int, int, tuple]]:
    """A 32x32 boost pad: two chevrons pointing along `direction`, as offsets from its centre."""
    fx, fy = direction
    out = []
    for along in range(-16, 16):
        for across in range(-16, 16):
            for tip in (-6, 6):                    # two chevrons, one behind the other
                d = along - tip + abs(across) // 2
                if 0 <= d < 4 and abs(across) < 14:
                    colour = AMBER if d < 2 else MAGENTA
                    out.append((fx * along - fy * across, fy * along + fx * across, colour))
    return out


def build_track(art: Path) -> dict:
    """track.png (1024x1024, Mode 7), the 128x128 class map, the rivals' lanes."""
    track = wang(Image.open(art / "tiles" / "wang_track_gravel.png").convert("RGB"))
    lagoon = wang(Image.open(art / "tiles" / "wang_gravel_lagoon.png").convert("RGB"))
    pts = [center_path(i / 4000) for i in range(4000)]
    v = grid(pts)
    out = Image.new("RGB", (W, W))
    for y in range(CELLS):
        for x in range(CELLS):
            c = (v[y][x], v[y][x + 1], v[y + 1][x], v[y + 1][x + 1])
            assert not ("a" in c and "w" in c), (x, y)
            if "w" in c:
                key = sum((t == "w") << b for t, b in zip(c, (3, 2, 1, 0)))
                out.paste(lagoon[key], (x * 16, y * 16))
            else:
                key = sum((t == "g") << b for t, b in zip(c, (3, 2, 1, 0)))
                out.paste(track[key], (x * 16, y * 16))

    # each 8x8 tile takes the class of the vertex it is nearest
    cls = [[{"a": 0, "g": 1, "w": 2}[v[(ty + 1) // 2][(tx + 1) // 2]] for tx in range(128)] for ty in range(128)]

    # the props, on gravel with a cell of gravel round them; they are solid
    props = [Image.open(art / "props" / f"roof_{n}.png").convert("RGBA") for n in PROPS]
    rng = random.Random(11)
    for gy in range(2, CELLS - 2, PROP_STEP):
        for gx in range(2, CELLS - 2, PROP_STEP):
            x, y = gx + rng.randrange(3), gy + rng.randrange(3)
            k = rng.randrange(len(props))
            if x + 3 > CELLS or y + 3 > CELLS or any(
                    v[j][i] != "g" for j in range(y - 1, y + 4) for i in range(x - 1, x + 4)):
                continue
            out.paste(props[k], (x * 16, y * 16), props[k])
            for ty in range(2 * y, 2 * y + 4):
                for tx in range(2 * x, 2 * x + 4):
                    cls[ty][tx] = 2
    # the start line: a two-tile checker band across the asphalt at x = 312, one tile
    # short of the curbs (a whole tile of checker is one tile, wherever it lies)
    sx, sy = center_path(0.0)
    for ty in range(128):
        for tx in (int(sx) // 8 - 1, int(sx) // 8):
            if abs(ty * 8 + 4 - sy) < ROAD_HALF and all(cls[ty + j][tx] == 0 for j in (-1, 0, 1)):
                for y in range(8):
                    for x in range(8):
                        out.putpixel((tx * 8 + x, ty * 8 + y), CHECKER[((x // 4) + (y // 4) + ty) % 2])

    # two boost pads, pointing the way the race goes
    for t in BOOSTS:
        (x0, y0), (x1, y1) = center_path(t), center_path(t + 0.002)
        f = (round((x1 - x0) / math.hypot(x1 - x0, y1 - y0)), round((y1 - y0) / math.hypot(x1 - x0, y1 - y0)))
        cx, cy = int(x0) // 16 * 16, int(y0) // 16 * 16
        for dx, dy, p in chevron(f):
            out.putpixel((cx + dx, cy + dy), p)
        for ty in range((cy - 16) // 8, (cy + 16) // 8):
            for tx in range((cx - 16) // 8, (cx + 16) // 8):
                assert cls[ty][tx] == 0, (tx, ty)
                cls[ty][tx] = 3

    colours = sorted(set(out.getdata()))
    tiles = {out.crop((x, y, x + 8, y + 8)).tobytes() for y in range(0, W, 8) for x in range(0, W, 8)}
    assert len(tiles) <= 256, f"{len(tiles)} tiles: a Mode 7 map names 256"
    lut = {c: i for i, c in enumerate(colours)}
    png = Image.new("P", (W, W), 0)
    png.putpalette([c for rgb in colours for c in rgb] + [0] * (768 - 3 * len(colours)))
    png.putdata([lut[p] for p in out.getdata()])
    return {"track": png, "classes": bytes(c for row in cls for c in row), "lanes": lanes(),
            "_tiles": len(tiles), "_colours": len(colours)}


def lanes() -> tuple[bytes, bytes]:
    """The rivals' two lanes: x, y (u16) of 256 points each, then their headings (u8, 64 = east)."""
    xy, heading = bytearray(), bytearray()
    for side in (-1, 1):
        for i in range(WAYPOINTS):
            (x0, y0), (x1, y1) = center_path(i / WAYPOINTS), center_path((i + 0.5) / WAYPOINTS)
            dx, dy = x1 - x0, y1 - y0
            n = math.hypot(dx, dy)
            x, y = x0 - side * LANE * dy / n, y0 + side * LANE * dx / n   # to the left (-1) or the right
            xy += round(x).to_bytes(2, "little") + round(y).to_bytes(2, "little")
            # forward = (sin h, -cos h) on a 0-255 circle, the game's unit
            heading.append(round(math.atan2(dx, -dy) * 128 / math.pi) & 0xFF)
    return bytes(xy), bytes(heading)


def scale2x(px: list[list[int]]) -> list[list[int]]:
    h, w = len(px), len(px[0])
    out = [[0] * (2 * w) for _ in range(2 * h)]
    for y in range(h):
        for x in range(w):
            e = px[y][x]
            b = px[y - 1][x] if y else e
            h_ = px[y + 1][x] if y + 1 < h else e
            d = px[y][x - 1] if x else e
            f = px[y][x + 1] if x + 1 < w else e
            if b != h_ and d != f:
                e0, e1 = (d if d == b else e), (f if b == f else e)
                e2, e3 = (d if d == h_ else e), (f if h_ == f else e)
            else:
                e0 = e1 = e2 = e3 = e
            out[2 * y][2 * x], out[2 * y][2 * x + 1] = e0, e1
            out[2 * y + 1][2 * x], out[2 * y + 1][2 * x + 1] = e2, e3
    return out


def rotsprite(im: Image.Image, degrees: float) -> Image.Image:
    """`im` (P, 0 transparent) turned clockwise by `degrees`, the RotSprite way."""
    px = [[im.getpixel((x, y)) for x in range(im.width)] for y in range(im.height)]
    for _ in range(3):
        px = scale2x(px)
    n = len(px)
    big = [[0] * n for _ in range(n)]
    a = math.radians(degrees)
    c, s = math.cos(a), math.sin(a)
    half = n / 2
    for y in range(n):
        for x in range(n):
            # the source pixel that lands here: turn back by `degrees`
            u, v = x + 0.5 - half, y + 0.5 - half
            sx, sy = int(c * u + s * v + half), int(-s * u + c * v + half)
            if 0 <= sx < n and 0 <= sy < n:
                big[y][x] = px[sy][sx]
    out = im.copy()
    for y in range(im.height):
        for x in range(im.width):
            block = [big[8 * y + j][8 * x + i] for j in range(8) for i in range(8)]
            opaque = [p for p in block if p]
            out.putpixel((x, y), max(set(opaque), key=opaque.count) if len(opaque) * 2 >= len(block) else 0)
    return out


def bikes(art: Path) -> Image.Image:
    """The bike sheet, 128x96: Kit's bike, then each rival's turned 0, 22.5, 45, 67.5 and 90 degrees."""
    names = ["kit", "white", "crimson"]
    srcs = [Image.open(art / "characters" / f"racebike_{n}.png").convert("RGBA") for n in names]
    colours = sorted({p[:3] for im in srcs for p in im.getdata() if p[3] >= 128})
    assert len(colours) <= 15, len(colours)

    def indexed(im):
        out = Image.new("P", im.size, 0)
        out.putdata([colours.index(p[:3]) + 1 if p[3] >= 128 else 0 for p in im.getdata()])
        return out

    frames = [indexed(srcs[0])]
    for rival in srcs[1:]:
        base = indexed(rival)
        frames += [base] + [rotsprite(base, 22.5 * k) for k in range(1, 4)] + [base.transpose(Image.Transpose.ROTATE_270)]
    sheet = Image.new("P", (128, 96), 0)
    sheet.putpalette([0, 0, 0] + [v for c in colours for v in c] + [0] * (765 - 3 * len(colours)))
    for i, f in enumerate(frames):
        sheet.paste(f, ((i % 4) * 32, (i // 4) * 32))
    return sheet
