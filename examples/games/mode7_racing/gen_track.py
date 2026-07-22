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

def main():
    random.seed(7)
    img = Image.new("RGB", (W, H))
    px = img.load()

    # grass base with mild texture (few distinct colors — kind to the
    # 256-color quantization AND to tile dedup)
    greens = [(34, 110, 34), (30, 102, 30), (38, 118, 38)]
    for y in range(H):
        for x in range(W):
            px[x, y] = greens[(x // 8 + y // 8 + ((x ^ y) >> 6)) % 3]

    draw = ImageDraw.Draw(img)

    # road: stamp discs along the center path
    road = (90, 90, 96)
    edge = (200, 200, 210)
    steps = 4000
    pts = [center_path(i / steps) for i in range(steps)]
    for (x, y) in pts:
        draw.ellipse([x - ROAD_HALF, y - ROAD_HALF,
                      x + ROAD_HALF, y + ROAD_HALF], fill=road)
    # edge lines: thinner white band re-stamped then road again inside
    for (x, y) in pts:
        draw.ellipse([x - ROAD_HALF, y - ROAD_HALF,
                      x + ROAD_HALF, y + ROAD_HALF],
                     outline=edge, width=3)
    for (x, y) in pts:
        draw.ellipse([x - (ROAD_HALF - 4), y - (ROAD_HALF - 4),
                      x + (ROAD_HALF - 4), y + (ROAD_HALF - 4)], fill=road)

    # start/finish checker line across the road at t=0 (top straight)
    sx, sy = center_path(0.0)
    for oy in range(-ROAD_HALF + 4, ROAD_HALF - 3):
        for ox in range(-6, 6):
            c = (240, 240, 240) if ((ox // 6) + (oy // 6)) % 2 == 0 else (16, 16, 16)
            px[int(sx + ox), int(sy + oy)] = c

    # wall ring at the border (2 tiles thick, dark red)
    wall = (120, 30, 30)
    draw.rectangle([0, 0, W - 1, 15], fill=wall)
    draw.rectangle([0, H - 16, W - 1, H - 1], fill=wall)
    draw.rectangle([0, 0, 15, H - 1], fill=wall)
    draw.rectangle([W - 16, 0, W - 1, H - 1], fill=wall)

    # gfx4snes needs an INDEXED png; the track uses ~10 colors anyway
    img.convert("P", palette=Image.ADAPTIVE, colors=64).save(RES / "track.png")

    # class map: sample the tile center -> road/grass/wall byte
    classes = bytearray()
    for ty in range(128):
        for tx in range(128):
            r, g, b = px[tx * TILE + 4, ty * TILE + 4]
            if r > 100 and g < 60:
                classes.append(2)               # wall
            elif abs(r - g) < 30 and r > 60:
                classes.append(0)               # road / line / edge
            else:
                classes.append(1)               # grass
    (RES / "track_class.bin").write_bytes(bytes(classes))
    road_n = classes.count(0)
    print(f"track.png 1024x1024 + track_class.bin "
          f"(road {road_n}, grass {classes.count(1)}, wall {classes.count(2)})")


if __name__ == "__main__":
    main()
