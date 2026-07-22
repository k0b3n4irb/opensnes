#!/usr/bin/env python3
"""Generate the Mode 7 flying terrain: res/terrain.png (1024x1024) for
gfx4snes -M 7, plus res/terrain_class.bin — 128x128 bytes (one per
tile: 0 = field, 1 = water, 2 = landing pad).

A patchwork of fields crossed by a river with a lake, and three
landing pads (striped helipad-style rectangles) the game's objective
counts. Deterministic (fixed seed) — committed assets are
reproducible.

Run from this directory:  python3 gen_terrain.py
"""
from pathlib import Path
import math
import random

from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
RES = HERE / "res"
RES.mkdir(exist_ok=True)

W = H = 1024
TILE = 8

PADS = [(200, 200), (780, 320), (460, 800)]     # pad centers (pixels)
PAD_W, PAD_H = 96, 64


def main():
    random.seed(11)
    img = Image.new("RGB", (W, H))
    px = img.load()
    draw = ImageDraw.Draw(img)

    # patchwork fields: 64px parcels in a few greens/yellows
    field_colors = [(52, 120, 40), (66, 134, 44), (108, 128, 36),
                    (44, 108, 36), (86, 140, 52)]
    for py in range(0, H, 64):
        for pxx in range(0, W, 64):
            c = field_colors[(pxx // 64 * 7 + py // 64 * 13) % len(field_colors)]
            draw.rectangle([pxx, py, pxx + 63, py + 63], fill=c)
    # parcel borders (hedges)
    hedge = (30, 78, 26)
    for v in range(0, W, 64):
        draw.line([(v, 0), (v, H - 1)], fill=hedge, width=2)
        draw.line([(0, v), (W - 1, v)], fill=hedge, width=2)

    # river: a sine sweep top-to-bottom, widening into a lake
    water = (36, 76, 160)
    for y in range(H):
        cx = 620 + int(140 * math.sin(y / 170.0))
        hw = 26 + (40 if 380 < y < 560 else 0)   # lake bulge
        draw.line([(cx - hw, y), (cx + hw, y)], fill=water, width=1)

    # landing pads: dark slab, white border, stripe chevrons
    for (cx, cy) in PADS:
        x0, y0 = cx - PAD_W // 2, cy - PAD_H // 2
        x1, y1 = cx + PAD_W // 2, cy + PAD_H // 2
        draw.rectangle([x0, y0, x1, y1], fill=(70, 70, 76))
        draw.rectangle([x0, y0, x1, y1], outline=(230, 230, 235), width=4)
        for sx in range(x0 + 16, x1 - 12, 24):
            draw.line([(sx, y0 + 8), (sx + 10, y1 - 8)],
                      fill=(230, 230, 235), width=4)

    img.convert("P", palette=Image.ADAPTIVE, colors=64).save(RES / "terrain.png")

    # class map from tile centers
    classes = bytearray()
    for ty in range(128):
        for tx in range(128):
            x, y = tx * TILE + 4, ty * TILE + 4
            r, g, b = px[x, y]
            on_pad = any(abs(x - cx) <= PAD_W // 2 and abs(y - cy) <= PAD_H // 2
                         for (cx, cy) in PADS)
            if on_pad:
                classes.append(2)
            elif b > 120 and b > r + 40:
                classes.append(1)               # water
            else:
                classes.append(0)               # field
    (RES / "terrain_class.bin").write_bytes(bytes(classes))
    print(f"terrain.png + terrain_class.bin (pads {classes.count(2)}, "
          f"water {classes.count(1)}, field {classes.count(0)})")


if __name__ == "__main__":
    main()
