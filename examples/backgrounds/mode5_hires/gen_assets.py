#!/usr/bin/env python3
"""Regenerate res/text.{pic,map,pal} + res/source.png (needs Pillow+numpy).

Renders a 512x448 text page and converts it to BG Mode 5 assets:
4bpp 8x8 subtiles emitted in left/right pairs (Mode 5's 16x8 tiles read
characters N and N+1), 32x64 tilemap (two stacked 32x32 screens — 64
tile rows x 8 = 512 lines of vertical space; interlace maps tile texel
rows 1:1 to hi-res lines, so a full 448-line page needs 56 rows).
Generated output is committed (wavetable/hicolor precedent).
"""
from PIL import Image, ImageDraw
import numpy as np, struct

W, H = 512, 448
img = Image.new('P', (W, H), 0)
img.putpalette([0,0,0, 255,255,255, 120,200,255, 255,200,60] + [0,0,0]*252)
d = ImageDraw.Draw(img)
def text2(x, y, c, t):
    # 2x2-thick strokes: readable both at true 512 AND in a 256-wide
    # downsample view (1px strokes average to half-intensity mush —
    # the 1px BANDS below stay thin on purpose as the acid test).
    for dx in (0, 1):
        for dy in (0, 1):
            d.text((x+dx, y+dy), t, fill=c)

for x, y, c, t in [
    (8,  10, 1, "OPENSNES HI-RES TEXT — BG MODE 5 + INTERLACE (512 x 448)"),
    (8,  34, 2, "Every glyph on this page is 8 pixels wide: a 256-wide mode"),
    (8,  50, 2, "would have to draw each character in 4 pixels."),
    (8,  82, 1, "!\"#$%&'()*+,-./0123456789:;<=>?@"),
    (8,  98, 1, "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"),
    (8, 114, 1, "abcdefghijklmnopqrstuvwxyz{|}~"),
    (8, 146, 3, "videoSetInterlace(1);"),
    (8, 162, 3, "setMainScreen(LAYER_BG1); setSubScreen(LAYER_BG1);"),
    (8, 194, 2, "1-pixel vertical stripes (white/black then cyan/gold):"),
    (8, 330, 2, "1-pixel checkerboard:"),
    (8, 424, 1, "Technique: krom (Peter Lemon), InterlaceFont — art: OpenSNES"),
]:
    text2(x, y, c, t)
a = np.array(img)
a[210:250, :] = np.tile(np.array([1,0], dtype=np.uint8), W//2)
a[264:304, :] = np.tile(np.array([2,0,3,0], dtype=np.uint8), W//4)
yy, xx = np.mgrid[352:408, 8:504]
a[352:408, 8:504] = ((xx+yy) % 2).astype(np.uint8)

def tile4bpp(block):
    out = bytearray(32)
    for y in range(8):
        b0=b1=b2=b3=0
        for x in range(8):
            c = int(block[y,x])
            b0 |= (c&1)<<(7-x); b1 |= ((c>>1)&1)<<(7-x)
            b2 |= ((c>>2)&1)<<(7-x); b3 |= ((c>>3)&1)<<(7-x)
        out[y*2],out[y*2+1],out[16+y*2],out[16+y*2+1] = b0,b1,b2,b3
    return bytes(out)

tilelist, pairs = [], {}
tilemap = np.zeros((64, 32), dtype=np.uint16)
for ty in range(64):
    for tx in range(32):
        if ty*8 < H:
            L = a[ty*8:(ty+1)*8, tx*16:tx*16+8]
            R = a[ty*8:(ty+1)*8, tx*16+8:tx*16+16]
        else:
            L = R = np.zeros((8,8), np.uint8)
        k = (L.tobytes(), R.tobytes())
        if k not in pairs:
            pairs[k] = len(tilelist)
            tilelist.append(tile4bpp(L)); tilelist.append(tile4bpp(R))
        tilemap[ty, tx] = pairs[k]
open('res/text.pic','wb').write(b''.join(tilelist))
open('res/text.map','wb').write(tilemap.astype('<u2').tobytes())
pal = bytearray(32)
for i,(r,g,b) in enumerate([(0,0,0),(255,255,255),(120,200,255),(255,200,60)]):
    struct.pack_into('<H', pal, i*2, ((b&0xF8)<<7)|((g&0xF8)<<2)|((r&0xF8)>>3))
open('res/text.pal','wb').write(bytes(pal))
Image.fromarray(a, mode='P').convert('RGB').save('res/source.png')
print('tiles:', len(tilelist), '| map 32x64 | ok')
