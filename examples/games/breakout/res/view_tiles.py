#!/usr/bin/env python3
"""
Convert SNES 4bpp tiles to PNG image for viewing
Usage: python3 view_tiles.py tiles2.dat
"""

import sys
from PIL import Image

# Simple grayscale palette for visualization
PALETTE = [(i * 17, i * 17, i * 17) for i in range(16)]

def decode_4bpp_tile(data, offset):
    """Decode one 8x8 4bpp SNES tile (32 bytes)"""
    pixels = []
    for row in range(8):
        # Each row: 2 bytes for bitplanes 0-1, then 2 bytes for bitplanes 2-3
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
    if len(sys.argv) < 2:
        print("Usage: python3 view_tiles.py tiles2.dat")
        sys.exit(1)

    filename = sys.argv[1]
    with open(filename, 'rb') as f:
        data = f.read()

    num_tiles = len(data) // 32
    print(f"File: {filename}")
    print(f"Size: {len(data)} bytes")
    print(f"Tiles: {num_tiles}")

    # Create image: 6 tiles per row
    tiles_per_row = 6
    rows = (num_tiles + tiles_per_row - 1) // tiles_per_row

    img_width = tiles_per_row * 10  # 8px tile + 2px spacing
    img_height = rows * 10

    img = Image.new('RGB', (img_width, img_height), (40, 40, 40))

    for tile_idx in range(num_tiles):
        tile_x = (tile_idx % tiles_per_row) * 10
        tile_y = (tile_idx // tiles_per_row) * 10

        pixels = decode_4bpp_tile(data, tile_idx * 32)

        for y, row in enumerate(pixels):
            for x, pixel in enumerate(row):
                color = PALETTE[pixel]
                img.putpixel((tile_x + x, tile_y + y), color)

    output = filename.replace('.dat', '_tiles.png')
    img.save(output)
    print(f"Saved: {output}")
    print(f"\nTile layout (left to right, top to bottom):")
    for row in range(rows):
        start = row * tiles_per_row
        end = min(start + tiles_per_row, num_tiles)
        print(f"  Row {row}: tiles {start}-{end-1}")

if __name__ == '__main__':
    main()
