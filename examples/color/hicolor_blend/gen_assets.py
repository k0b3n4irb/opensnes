#!/usr/bin/env python3
"""Regenerate the channel-split blend assets (needs Pillow + numpy).

BG1 (main): the image's G+B channels, 240 colors at CGRAM 16-255,
8bpp Mode 3 tiles (indices offset by +16 so CGRAM 0-15 stays free).
BG2 (sub): the R channel, 15 levels at CGRAM 1-15 (palette 0, index 0
transparent -> backdrop, which is in the ADD mask). Color math ADDs
them back into full RGB — krom's HiColor3840 recipe.
"""
import numpy as np, struct
from PIL import Image

MW, MH = 128, 112
yy, xx = np.mgrid[0:MH, 0:MW].astype(float)
cx, cy = 64.0, 56.0
r = np.hypot(xx-cx, yy-cy)
a = np.arctan2(yy-cy, xx-cx)
hue = (r / 12.0 + 1.5 * (1.0 + np.sin(3 * a))) % 6.0  # sin: continuous across the arctan2 branch cut
c = np.clip(np.stack([np.abs(hue-3)-1, 2-np.abs(hue-2), 2-np.abs(hue-4)]), 0, 1) * 255
img = np.transpose(c, (1, 2, 0))
shade = 0.6 + 0.4*np.clip(1 - r/90.0, 0, 1)
img = (img * shade[..., None]).clip(0, 255).astype(np.uint8)
big = np.tile(img, (2, 2, 1))
Image.fromarray(big).save('res/source.png')

gb = big.copy(); gb[..., 0] = 0
q = Image.fromarray(gb).quantize(colors=240, kmeans=3)
gb_idx = np.array(q)                     # 0..239
gb_pal = q.getpalette()[:240*3]
red = (big[..., 0].astype(int) * 15 // 255).astype(np.uint8)  # 0..15

def snes_col(rgb):
    r_, g_, b_ = rgb
    return ((b_ & 0xF8) << 7) | ((g_ & 0xF8) << 2) | ((r_ & 0xF8) >> 3)

def tile8bpp(block):  # 8x8 of 0..255 -> 64 bytes (4 plane pairs)
    out = bytearray(64)
    for pair in range(4):
        for y in range(8):
            b0 = b1 = 0
            for x in range(8):
                v = int(block[y, x])
                b0 |= ((v >> (pair*2)) & 1) << (7 - x)
                b1 |= ((v >> (pair*2 + 1)) & 1) << (7 - x)
            out[pair*16 + y*2] = b0
            out[pair*16 + y*2 + 1] = b1
    return bytes(out)

def tile4bpp(block):
    out = bytearray(32)
    for y in range(8):
        b = [0]*4
        for x in range(8):
            v = int(block[y, x])
            for p in range(4):
                b[p] |= ((v >> p) & 1) << (7 - x)
        out[y*2], out[y*2+1], out[16+y*2], out[16+y*2+1] = b
    return bytes(out)

def build(indices, tilefn):
    tiles, order, tmap = {}, [], np.zeros((28, 32), np.uint16)
    for ty in range(28):
        for tx in range(32):
            blk = indices[ty*8:(ty+1)*8, tx*8:(tx+1)*8]
            k = blk.tobytes()
            if k not in tiles:
                tiles[k] = len(order); order.append(tilefn(blk))
            tmap[ty, tx] = tiles[k]
    return b''.join(order), tmap.astype('<u2').tobytes()

gb_pic, gb_map = build(gb_idx + 16, tile8bpp)   # CGRAM 16-255
r_pic, r_map = build(red, tile4bpp)             # palette 0, idx 0 transparent
open('res/gb.pic','wb').write(gb_pic); open('res/gb.map','wb').write(gb_map)
open('res/r.pic','wb').write(r_pic);  open('res/r.map','wb').write(r_map)

pal = bytearray(512)
for i in range(240):
    struct.pack_into('<H', pal, (16+i)*2, snes_col(gb_pal[i*3:i*3+3]))
for i in range(16):     # red ramp at 0-15 (index 0 unused/transparent)
    struct.pack_into('<H', pal, i*2, (i * 31 // 15) & 0x1F)
open('res/blend.pal','wb').write(bytes(pal))
print('gb.pic %d gb.map %d r.pic %d r.map %d pal %d' %
      (len(gb_pic), len(gb_map), len(r_pic), len(r_map), len(pal)))
