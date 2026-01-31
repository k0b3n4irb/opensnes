#!/usr/bin/env python3
"""
Convert SNES 4bpp tiles to PNG with actual SNES palette
Usage: python3 view_tiles_color.py tiles2.dat palette.dat
"""

import sys
from PIL import Image

def load_snes_palette(filename):
    """Load SNES 15-bit palette and convert to RGB"""
    with open(filename, 'rb') as f:
        data = f.read()

    palette = []
    for i in range(0, len(data), 2):
        if i + 1 >= len(data):
            break
        # SNES: 0BBBBBGG GGGRRRRR (little endian)
        word = data[i] | (data[i + 1] << 8)
        r = (word & 0x1F) << 3
        g = ((word >> 5) & 0x1F) << 3
        b = ((word >> 10) & 0x1F) << 3
        palette.append((r, g, b))

    return palette

def decode_4bpp_tile(data, offset):
    """Decode one 8x8 4bpp SNES tile (32 bytes)"""
    pixels = []
    for row in range(8):
        bp0 = data[offset + row * 2]
        bp1 = data[offset + row * 2 + 1]
        bp2 = data[offset + row * 2 + 16]
        bp3 = data[offset + row * 2 + 17]

        row_pixels = []
        for bit in range(7, -1, -1):
            pixel = ((bp0 >> bit) & 1) | \
                    (((bp1 >> bit) & 1) << 1) | \
                    (((bp2 >> bit) & 1) << 2) | \
                    (((bp3 >> bit) & 1) << 3)
            row_pixels.append(pixel)
        pixels.append(row_pixels)
    return pixels

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 view_tiles_color.py tiles.dat palette.dat")
        sys.exit(1)

    tile_file = sys.argv[1]
    pal_file = sys.argv[2]

    # Load palette
    full_palette = load_snes_palette(pal_file)
    print(f"Loaded {len(full_palette)} colors from {pal_file}")

    # Use sprite palette (colors 128-143 for palette 0)
    sprite_pal = full_palette[128:144] if len(full_palette) > 143 else full_palette[:16]
    print(f"Sprite palette 0 colors: {sprite_pal[:4]}...")

    with open(tile_file, 'rb') as f:
        data = f.read()

    num_tiles = len(data) // 32
    print(f"File: {tile_file}, {len(data)} bytes, {num_tiles} tiles")

    # Create image with labels
    tiles_per_row = 6
    rows = (num_tiles + tiles_per_row - 1) // tiles_per_row

    tile_size = 16  # Scale up 2x for visibility
    spacing = 4
    cell_size = tile_size + spacing

    img_width = tiles_per_row * cell_size + spacing
    img_height = rows * (cell_size + 12) + spacing  # Extra space for numbers

    img = Image.new('RGB', (img_width, img_height), (0, 0, 80))  # Dark blue background

    for tile_idx in range(num_tiles):
        col = tile_idx % tiles_per_row
        row = tile_idx // tiles_per_row

        tile_x = spacing + col * cell_size
        tile_y = spacing + row * (cell_size + 12)

        pixels = decode_4bpp_tile(data, tile_idx * 32)

        # Draw scaled tile (2x)
        for y, prow in enumerate(pixels):
            for x, pixel in enumerate(prow):
                if pixel < len(sprite_pal):
                    color = sprite_pal[pixel]
                else:
                    color = (255, 0, 255)  # Magenta for out of range
                # 2x scale
                for dy in range(2):
                    for dx in range(2):
                        img.putpixel((tile_x + x*2 + dx, tile_y + y*2 + dy), color)

    output = tile_file.replace('.dat', '_color.png')
    img.save(output)
    print(f"Saved: {output}")

if __name__ == '__main__':
    main()
