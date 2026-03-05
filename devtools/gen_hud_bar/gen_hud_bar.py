#!/usr/bin/env python3
"""Generate HUD health bar sprite sheet for SNES (48x8, 6 tiles of 8x8).

Tile layout (left to right):
  0: back_left   (empty left cap)
  1: back_mid    (empty middle)
  2: back_right  (empty right cap)
  3: fill_left   (filled left cap)
  4: fill_mid    (filled middle)
  5: fill_right  (filled right cap)

Output: res/hud_bar.png (48x8, indexed 16-color)

If Kenney RPG Expansion PNGs are found in the same directory, they are
composited. Otherwise, a procedural bar is generated.
"""

import sys
import os

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("ERROR: Pillow is required. Install with: pip install Pillow", file=sys.stderr)
    sys.exit(1)

# SNES BGR555 palette (used by gfx4snes)
# Color 0: transparent (black)
# Color 1: dark gray background (bar back)
# Color 2: dark green border/shadow
# Color 3: bright green fill
# Color 4: medium green highlight
PALETTE = [
    (0, 0, 0),        # 0: transparent
    (48, 48, 48),      # 1: dark gray (bar back fill)
    (24, 80, 24),      # 2: dark green (border)
    (56, 184, 56),     # 3: bright green (bar fill)
    (40, 136, 40),     # 4: medium green (fill shadow)
    (32, 32, 32),      # 5: darker gray (back border)
]
# Pad to 16 colors
while len(PALETTE) < 16:
    PALETTE.append((0, 0, 0))


def draw_back_left(draw, x):
    """Empty bar left cap at tile position x."""
    # Border
    draw.rectangle([x + 1, 1, x + 7, 6], fill=1, outline=5)
    # Round left edge
    draw.line([x, 2, x, 5], fill=5)
    draw.point([x + 1, 1], fill=0)
    draw.point([x + 1, 6], fill=0)


def draw_back_mid(draw, x):
    """Empty bar middle segment at tile position x."""
    draw.rectangle([x, 1, x + 7, 6], fill=1)
    draw.line([x, 1, x + 7, 1], fill=5)
    draw.line([x, 6, x + 7, 6], fill=5)


def draw_back_right(draw, x):
    """Empty bar right cap at tile position x."""
    draw.rectangle([x, 1, x + 6, 6], fill=1, outline=5)
    # Round right edge
    draw.line([x + 7, 2, x + 7, 5], fill=5)
    draw.point([x + 6, 1], fill=0)
    draw.point([x + 6, 6], fill=0)


def draw_fill_left(draw, x):
    """Filled bar left cap at tile position x."""
    # Background shape first
    draw_back_left(draw, x)
    # Green fill inside
    draw.rectangle([x + 2, 2, x + 7, 5], fill=3)
    draw.line([x + 1, 3, x + 1, 4], fill=3)
    # Top highlight
    draw.line([x + 2, 2, x + 7, 2], fill=4)


def draw_fill_mid(draw, x):
    """Filled bar middle segment at tile position x."""
    draw_back_mid(draw, x)
    # Green fill
    draw.rectangle([x, 2, x + 7, 5], fill=3)
    # Top highlight
    draw.line([x, 2, x + 7, 2], fill=4)


def draw_fill_right(draw, x):
    """Filled bar right cap at tile position x."""
    draw_back_right(draw, x)
    # Green fill inside
    draw.rectangle([x, 2, x + 5, 5], fill=3)
    draw.line([x + 6, 3, x + 6, 4], fill=3)
    # Top highlight
    draw.line([x, 2, x + 5, 2], fill=4)


def main():
    # Output path
    if len(sys.argv) > 1:
        outdir = sys.argv[1]
    else:
        outdir = os.path.join(os.path.dirname(__file__), '..', 'projects', 'rpg', 'res')

    outpath = os.path.join(outdir, 'hud_bar.png')

    # Create 48x8 indexed image
    img = Image.new('P', (48, 8), 0)

    # Set palette
    flat_pal = []
    for r, g, b in PALETTE:
        flat_pal.extend([r, g, b])
    # Pad to 256 colors
    while len(flat_pal) < 768:
        flat_pal.extend([0, 0, 0])
    img.putpalette(flat_pal)

    draw = ImageDraw.Draw(img)

    # Draw 6 tiles
    draw_back_left(draw, 0)
    draw_back_mid(draw, 8)
    draw_back_right(draw, 16)
    draw_fill_left(draw, 24)
    draw_fill_mid(draw, 32)
    draw_fill_right(draw, 40)

    os.makedirs(outdir, exist_ok=True)
    img.save(outpath)
    print(f"[HUD] Generated {outpath} (48x8, 6 tiles)")


if __name__ == '__main__':
    main()
