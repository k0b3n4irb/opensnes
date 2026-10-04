#!/usr/bin/env python3
"""Generate the Mode 7 race track: res/track.png (1024x1024, 128x128
tiles of 8x8) for gfx4snes -M 7, plus res/track_class.bin — a 128x128
byte map (one byte per tile: 0 = road, 1 = grass, 2 = wall) the game
reads for collision/surface.

The circuit is a rounded rectangle with a chicane on the top straight,
drawn as a thick road band around a center path, textured grass
outside, a start/finish checker line, and a solid wall ring at the map
border (the game also hard-clamps position, the wall makes the edge
visible).

Run from this directory:  python3 gen_track.py
Deterministic (fixed seed) so the committed assets are reproducible.
"""
from pathlib import Path
import math
import random

from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
RES = HERE / "res"
RES.mkdir(exist_ok=True)

W = H = 1024                    # pixels; 128x128 tiles
TILE = 8

# --- the center path of the circuit (parametric rounded rect + chicane)
def center_path(t):
    """t in [0,1) -> (x, y) on the circuit center line."""
    # rounded rectangle centered at (512, 512)
    rx, ry, r = 320, 240, 120   # half-extents and corner radius
    cx, cy = 512, 512
    per_straight_x = 2 * (rx - r)
    per_straight_y = 2 * (ry - r)
    per_corner = math.pi * r / 2
    total = 2 * per_straight_x + 2 * per_straight_y + 4 * per_corner
    d = t * total

    segs = [
        ("s", per_straight_x, (cx - (rx - r), cy - ry), (1, 0)),      # top, ->
        ("c", per_corner, (cx + (rx - r), cy - ry + r), -90),          # top-right corner
        ("s", per_straight_y, (cx + rx, cy - (ry - r)), (0, 1)),       # right, v
        ("c", per_corner, (cx + (rx - r), cy + (ry - r)), 0),          # bottom-right
        ("s", per_straight_x, (cx + (rx - r), cy + ry), (-1, 0)),      # bottom, <-
        ("c", per_corner, (cx - (rx - r), cy + (ry - r)), 90),         # bottom-left
        ("s", per_straight_y, (cx - rx, cy + (ry - r)), (0, -1)),      # left, ^
        ("c", per_corner, (cx - (rx - r), cy - (ry - r)), 180),        # top-left
    ]
    for kind, length, origin, arg in segs:
        if d <= length:
            if kind == "s":
                dx, dy = arg
                x = origin[0] + dx * d
                y = origin[1] + dy * d
            else:
                a0 = math.radians(arg)
                a = a0 + (d / length) * (math.pi / 2)
                x = origin[0] + r * math.cos(a)
                y = origin[1] + r * math.sin(a)
            # chicane: push the top straight outward around its middle
            if 380 < x < 644 and y < 400:
                bump = 60 * math.exp(-((x - 512) / 70.0) ** 2)
                y += bump
            return x, y
        d -= length
    return center_path(0)


ROAD_HALF = 40                  # road half-width in pixels

# Everything below is painted per 8x8 TILE, not per pixel: a Mode 7 map
# addresses 256 tiles, and the per-pixel discs, anti-aliased edges and
# checker line made 406 distinct ones — gfx4snes wrapped the index modulo
# 256 without a word until 2026-10-04, so 150 map entries showed the wrong
# tile. The road mask is still computed at full resolution from the same
# centre path; each tile then gets one of a few patterns (solid road, grass,
# wall, an edge band on the sides facing grass, the checker line).
GREENS = [(34, 110, 34), (30, 102, 30), (38, 118, 38)]
ROAD = (90, 90, 96)
EDGE = (200, 200, 210)
WALL = (120, 30, 30)
N = W // TILE                   # 128 tiles a side


def main():
    random.seed(7)

    # full-resolution road mask: stamp discs along the centre path
    mask = Image.new("L", (W, H), 0)
    mdraw = ImageDraw.Draw(mask)
    steps = 4000
    pts = [center_path(i / steps) for i in range(steps)]
    for (x, y) in pts:
        mdraw.ellipse([x - ROAD_HALF, y - ROAD_HALF,
                       x + ROAD_HALF, y + ROAD_HALF], fill=255)
    mpx = mask.load()

    # tile classes: 0 = road, 1 = grass, 2 = wall (2-tile ring at the border)
    cls = [[1] * N for _ in range(N)]
    for ty in range(N):
        for tx in range(N):
            if ty < 2 or ty >= N - 2 or tx < 2 or tx >= N - 2:
                cls[ty][tx] = 2
                continue
            road_px = sum(1 for y in range(TILE) for x in range(TILE)
                          if mpx[tx * TILE + x, ty * TILE + y])
            cls[ty][tx] = 0 if road_px * 2 >= TILE * TILE else 1

    # start/finish: the tile column of the path's origin, over the road tiles
    sx, sy = center_path(0.0)
    start_tx = int(sx) // TILE
    checker = set()
    for ty in range(N):
        if cls[ty][start_tx] == 0 and abs(ty * TILE + 4 - sy) <= ROAD_HALF:
            checker.add((start_tx, ty))

    img = Image.new("RGB", (W, H))
    px = img.load()
    draw = ImageDraw.Draw(img)

    def is_grass(tx, ty):
        return 0 <= tx < N and 0 <= ty < N and cls[ty][tx] == 1

    for ty in range(N):
        for tx in range(N):
            x0, y0 = tx * TILE, ty * TILE
            c = cls[ty][tx]
            if c == 2:
                draw.rectangle([x0, y0, x0 + 7, y0 + 7], fill=WALL)
            elif c == 1:
                g = GREENS[(tx + ty + ((x0 ^ y0) >> 6)) % 3]
                draw.rectangle([x0, y0, x0 + 7, y0 + 7], fill=g)
            else:
                draw.rectangle([x0, y0, x0 + 7, y0 + 7], fill=ROAD)
                if (tx, ty) in checker:
                    for y in range(TILE):
                        for x in range(TILE):
                            on = ((x // 4) + (y // 4) + ty) % 2 == 0
                            px[x0 + x, y0 + y] = (240, 240, 240) if on else (16, 16, 16)
                    continue
                # a 3-pixel edge band on each side that faces grass
                if is_grass(tx, ty - 1):
                    draw.rectangle([x0, y0, x0 + 7, y0 + 2], fill=EDGE)
                if is_grass(tx, ty + 1):
                    draw.rectangle([x0, y0 + 5, x0 + 7, y0 + 7], fill=EDGE)
                if is_grass(tx - 1, ty):
                    draw.rectangle([x0, y0, x0 + 2, y0 + 7], fill=EDGE)
                if is_grass(tx + 1, ty):
                    draw.rectangle([x0 + 5, y0, x0 + 7, y0 + 7], fill=EDGE)

    # gfx4snes needs an INDEXED png; the track uses ~8 colors
    img.convert("P", palette=Image.ADAPTIVE, colors=64).save(RES / "track.png")

    # class map straight from the tile classes (the game reads one byte per tile)
    classes = bytearray(cls[ty][tx] for ty in range(N) for tx in range(N))
    (RES / "track_class.bin").write_bytes(bytes(classes))
    print(f"track.png 1024x1024 + track_class.bin "
          f"(road {classes.count(0)}, grass {classes.count(1)}, wall {classes.count(2)})")


if __name__ == "__main__":
    main()
