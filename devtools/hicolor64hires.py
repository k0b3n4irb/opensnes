#!/usr/bin/env python3
"""hicolor64hires.py — per-tile-row palettes x pseudo-hires converter.

Reimplements krom (Peter Lemon)'s SNESBGPAL64tilerowHiRes.py contract
(HiColor64PerTileRowPseudoHiRes demo): a 512x224 image is quantized per
128x8 segment (15 colors + reserved black), then split by COLUMN PARITY —
odd columns become BG1 tiles (main screen), even columns BG2 tiles (sub
screen). Under SETINI pseudo-hires the PPU interleaves sub/main pixels
back into 512 horizontal pixels; the H-IRQ CGRAM stream reloads the
shared 64-color row palettes exactly as in hicolor_1792.

Outputs: <prefix>_bg1.pic, <prefix>_bg2.pic (28672 B each), <prefix>.pal
(3584 B). Usage: hicolor64hires.py input.png output_prefix
"""
import sys
import struct
import PIL.Image

COLORS, METHOD, KMEANS = 15, 0, 3


def write_pal(segment, out):
    palette = segment.getpalette()[: COLORS * 3]
    out.write(struct.pack('H', 0))
    for i in range(COLORS):
        r, g, b = palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]
        out.write(struct.pack('H', ((b & 0xF8) << 7) | ((g & 0xF8) << 2) | ((r & 0xF8) >> 3)))


def write_tile(segment, tilenum, parity, out):
    """8x8 tile from one column-parity of a 128x8 segment (krom's
    convert_tile1/2: start offset = tile*16 + parity, x-step 2)."""
    pixels = segment.getdata()
    tile = []
    i = tilenum * 16 + parity
    for _y in range(8):
        for _x in range(8):
            tile.append(pixels[i] + 1)
            i += 2
        i += 112
    planes = bytearray(32)
    for y in range(8):
        b = [0, 0, 0, 0]
        for x in range(8):
            c = tile[(y << 3) + x]
            for p in range(4):
                b[p] |= ((c >> p) & 1) << (7 - x)
        planes[y*2], planes[y*2+1], planes[16+y*2], planes[16+y*2+1] = b
    out.write(bytes(planes))


def main(argv):
    infile, prefix = argv
    img = PIL.Image.open(infile).convert('RGB')
    if img.size != (512, 224):
        sys.exit(f'error: input must be 512x224, got {img.size}')
    with open(prefix + '.pal', 'wb') as outpal, \
         open(prefix + '_bg1.pic', 'wb') as out1, \
         open(prefix + '_bg2.pic', 'wb') as out2:
        for row in range(28):
            for seg in range(4):
                s = img.crop((seg * 128, row * 8, (seg + 1) * 128, row * 8 + 8))
                s = s.quantize(colors=COLORS, method=METHOD, kmeans=KMEANS)
                write_pal(s, outpal)
                for t in range(8):
                    write_tile(s, t, 1, out1)  # odd cols -> BG1 (main)
                    write_tile(s, t, 0, out2)  # even cols -> BG2 (sub)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1:])
