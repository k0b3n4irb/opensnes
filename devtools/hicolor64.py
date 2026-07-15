#!/usr/bin/env python3
"""hicolor64.py — per-tile-row palette converter for the HiColor technique.

Converts a 256x224 image into the exact asset contract of krom (Peter
Lemon)'s HiColor64PerTileRow demo, whose converter
(GFX/SNESBGPAL64tilerow.py in his repo) this tool reimplements — credit
where due:

  - .pic : 4bpp planar tiles, SEQUENTIAL (no dedup) — 28 tile rows x 32
           tiles x 32 bytes = 28672 bytes. Tiles are emitted per 64x8
           screen-quarter segment (8 tiles), 4 segments per row.
  - .pal : 28 rows x 4 segments x 16 colors x 2 bytes = 3584 bytes.
           Each 64x8 segment is quantized to 15 colors; sub-palette
           index 0 is black (the technique streams whole palettes, so
           there is no transparency to preserve).

Every segment gets its OWN 16 colors, reloaded per scanline by the
H-IRQ CGRAM stream at runtime: 4 x 16 = 64 colors per tile row, 28 rows
= 1792 colors on screen from a 4bpp background.

Usage: hicolor64.py input.png output_prefix
       (writes output_prefix.pic and output_prefix.pal)
"""
import sys
import struct
import PIL.Image

COLORS = 15   # colors per segment (index 0 reserved = black)
METHOD = 0    # PIL quantize: median cut
KMEANS = 3


def write_pal(segment, out):
    """SNES BGR555 palette: black at index 0, then the 15 quantized colors."""
    palette = segment.getpalette()[: COLORS * 3]
    out.write(struct.pack('H', 0))
    for i in range(COLORS):
        r, g, b = palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]
        out.write(struct.pack('H', ((b & 0xF8) << 7) | ((g & 0xF8) << 2) | ((r & 0xF8) >> 3)))


def write_tile(segment, tilenum, out):
    """One 8x8 tile of the 64x8 segment as 4bpp planar (32 bytes)."""
    pixels = segment.getdata()
    tile = []
    i = tilenum * 8
    for _y in range(8):
        for _x in range(8):
            tile.append(pixels[i] + 1)  # +1: index 0 is the reserved black
            i += 1
        i += 56  # next row within the 64px-wide segment
    planes = bytearray(32)
    for y in range(8):
        b1 = b2 = b3 = b4 = 0
        for x in range(8):
            c = tile[(y << 3) + x]
            b1 |= (c & 1) << (7 - x)
            b2 |= ((c >> 1) & 1) << (7 - x)
            b3 |= ((c >> 2) & 1) << (7 - x)
            b4 |= ((c >> 3) & 1) << (7 - x)
        planes[y * 2], planes[y * 2 + 1] = b1, b2
        planes[y * 2 + 16], planes[y * 2 + 17] = b3, b4
    out.write(bytes(planes))


def main(argv):
    infile, prefix = argv
    img = PIL.Image.open(infile).convert('RGB')
    if img.size != (256, 224):
        sys.exit(f'error: input must be 256x224, got {img.size[0]}x{img.size[1]}')
    with open(prefix + '.pal', 'wb') as outpal, open(prefix + '.pic', 'wb') as outpic:
        for row in range(224 // 8):
            for seg in range(4):
                segment = img.crop((seg * 64, row * 8, (seg + 1) * 64, row * 8 + 8))
                segment = segment.quantize(colors=COLORS, method=METHOD, kmeans=KMEANS)
                write_pal(segment, outpal)
                for t in range(8):
                    write_tile(segment, t, outpic)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1:])
