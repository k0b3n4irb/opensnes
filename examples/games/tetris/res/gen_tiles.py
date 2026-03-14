#!/usr/bin/env python3
"""Generate tiles.png for Tetris — 8x8 tiles, 4bpp indexed (16 colors).

Tile layout:
  Row 0 (tiles 0-15):   empty, 7 colored blocks (I,O,T,S,Z,L,J), 3 border, 3 spare, ghost, spare
  Row 1 (tiles 16-31):  wide block tiles (16x8, left+right halves)
  Row 2-5 (tiles 32-95): ASCII font from breakout (space=32, 0-9=48-57, A-Z=65-90)
  Row 6 (tiles 96-111):  remaining (mostly unused)

Font tiles extracted from breakout's tiles1.dat (4bpp).
Uses palette 1 colors: 0=transparent, 11=white body, 12=blue shadow.
"""

from PIL import Image
import os

# --- 16-color palette (SNES BGR555 order doesn't matter for PNG; we specify RGB) ---
PALETTE = [
    (0x2b, 0x45, 0x55), # 0:  Background — deep blue-gray (#2b4555)
    (0x80, 0xbe, 0xdb), # 1:  I piece — light blue (#80bedb)
    (0xf6, 0xd7, 0x14), # 2:  O piece — bright yellow (#f6d714)
    (0x71, 0x61, 0xde), # 3:  T piece — purple (#7161de)
    (0x0b, 0xdb, 0x52), # 4:  S piece — neon green (#0bdb52)
    (0xe7, 0x18, 0x61), # 5:  Z piece — bright magenta (#e71861)
    (0xdc, 0x6f, 0x0f), # 6:  L piece — orange (#dc6f0f)
    (0x13, 0x8e, 0x7d), # 7:  J piece — teal (#138e7d)
    (0x18, 0x10, 0x10), # 8:  Block bevel dark (#181010)
    (0xf5, 0xf5, 0xf5), # 9:  Block bevel light (#f5f5f5)
    (0x61, 0x6b, 0x6d), # 10: Dark gray (#616b6d)
    (0xa9, 0xb3, 0x99), # 11: Sage green (#a9b399)
    (0xff, 0xa9, 0x95), # 12: Coral pink (#ffa995)
    (0xc6, 0x92, 0x4f), # 13: Tan/brown (#c6924f)
    (0x88, 0x58, 0x18), # 14: Dark brown (#885818)
    (0x8d, 0x02, 0x2e), # 15: Deep maroon (#8d022e)
]

# Image: 16 tiles wide (128px) x 7 rows (56px)
W, H = 128, 112  # 16 tiles * 7 rows = 112 tiles total (plenty for ASCII)
img = Image.new('P', (W, H), 0)

# Set palette
pal_flat = []
for r, g, b in PALETTE:
    pal_flat.extend([r, g, b])
pal_flat.extend([0, 0, 0] * (256 - len(PALETTE)))
img.putpalette(pal_flat)

pixels = img.load()


def fill_tile(tx, ty, color_idx):
    """Fill an 8x8 tile at grid position (tx, ty) with a single color."""
    for y in range(8):
        for x in range(8):
            pixels[tx * 8 + x, ty * 8 + y] = color_idx


def draw_block(tx, ty, color_idx):
    """Draw a 3D beveled block tile at grid (tx, ty)."""
    x0, y0 = tx * 8, ty * 8
    # Fill with main color
    for y in range(8):
        for x in range(8):
            pixels[x0 + x, y0 + y] = color_idx

    # Light bevel (top + left edge, 1px)
    for x in range(8):
        pixels[x0 + x, y0] = 9       # top row = light
    for y in range(8):
        pixels[x0, y0 + y] = 9       # left col = light

    # Dark bevel (bottom + right edge, 1px)
    for x in range(8):
        pixels[x0 + x, y0 + 7] = 8   # bottom row = dark
    for y in range(8):
        pixels[x0 + 7, y0 + y] = 8   # right col = dark

    # Corner adjustments
    pixels[x0, y0] = 9               # top-left = light
    pixels[x0 + 7, y0] = 9           # top-right = light (top wins)
    pixels[x0, y0 + 7] = 8           # bottom-left = dark (bottom wins)
    pixels[x0 + 7, y0 + 7] = 8       # bottom-right = dark


def draw_breakout_tile(tx, ty, pattern):
    """Draw an 8x8 tile from a pixel pattern (list of 8 strings, each 8 hex digits).
    Pixel values 0-7 used with palette 1 at runtime (grey gradient)."""
    x0, y0 = tx * 8, ty * 8
    for y in range(8):
        for x in range(8):
            pixels[x0 + x, y0 + y] = int(pattern[y][x], 16)


# Breakout border patterns (extracted from breakout tiles1.dat)
# Pixel indices 0-7 map to grey gradient via palette 1 at runtime
BORDER_CORNER = [   # Tile 1 from breakout — top-left corner
    "00077777",
    "00722222",
    "07221111",
    "72213333",
    "72133555",
    "72135566",
    "72135644",
    "72135647",
]

BORDER_HORIZ = [    # Tile 6 from breakout — horizontal bar
    "77777777",
    "22222222",
    "11111111",
    "33333333",
    "55555555",
    "66666666",
    "55555555",
    "77777777",
]

BORDER_VERT = [     # Tile 8 from breakout — vertical bar
    "72135647",
    "72135647",
    "72135647",
    "72135647",
    "72135647",
    "72135647",
    "72135647",
    "72135647",
]


def decode_4bpp_tile(data, tile_idx):
    """Decode one 8x8 4bpp SNES tile from raw data → 8x8 pixel array."""
    offset = tile_idx * 32
    px = [[0]*8 for _ in range(8)]
    for row in range(8):
        bp0 = data[offset + row*2]
        bp1 = data[offset + row*2 + 1]
        bp2 = data[offset + 16 + row*2]
        bp3 = data[offset + 16 + row*2 + 1]
        for col in range(8):
            bit = 7 - col
            px[row][col] = (((bp0 >> bit) & 1) | (((bp1 >> bit) & 1) << 1) |
                            (((bp2 >> bit) & 1) << 2) | (((bp3 >> bit) & 1) << 3))
    return px


def load_breakout_font():
    """Load font tiles from breakout's tiles1.dat.
    Returns dict mapping ASCII char → 8x8 pixel array.
    Breakout font: digits at tiles 38-47, letters A-Z at tiles 55-80.
    Uses colors 0 (transparent), 11 (white body), 12 (blue shadow)."""
    dat_path = os.path.join(os.path.dirname(__file__), '..', '..', 'breakout', 'res', 'tiles1.dat')
    data = open(dat_path, 'rb').read()
    font = {}
    # Digits 0-9
    for d in range(10):
        font[chr(48 + d)] = decode_4bpp_tile(data, 38 + d)
    # Letters A-Z
    for i in range(26):
        font[chr(65 + i)] = decode_4bpp_tile(data, 55 + i)
    # Space = empty (all zeros)
    font[' '] = [[0]*8 for _ in range(8)]
    return font


BREAKOUT_FONT = load_breakout_font()


def draw_char(tx, ty, ch):
    """Draw a breakout font character at tile grid position (tx, ty)."""
    x0, y0 = tx * 8, ty * 8
    glyph = BREAKOUT_FONT.get(ch)
    if glyph is None:
        return
    for row in range(8):
        for col in range(8):
            if glyph[row][col]:
                pixels[x0 + col, y0 + row] = glyph[row][col]


# === ROW 0: Tile 0 = empty, Tiles 1-7 = colored blocks, Tiles 8-13 = border ===

# Tile 0: empty (already background color 0)

# Tiles 1-7: colored block tiles
for i in range(7):
    draw_block(i + 1, 0, i + 1)

# Tiles 8-10: breakout-style border (3 tiles + SNES flip flags = full frame)
# Tile 8: corner (TL; flip for TR/BL/BR)
draw_breakout_tile(8, 0, BORDER_CORNER)
# Tile 9: horizontal bar (top/bottom)
draw_breakout_tile(9, 0, BORDER_HORIZ)
# Tile 10: vertical bar (left; hflip for right)
draw_breakout_tile(10, 0, BORDER_VERT)
# Tiles 11-13: unused (border uses flip flags now)

# Tile 14: ghost piece (Phase 2) - dimmer version of a block
x0, y0 = 14 * 8, 0
for y in range(8):
    for x in range(8):
        pixels[x0 + x, y0 + y] = 14
# Add subtle edges
for x in range(8):
    pixels[x0 + x, y0] = 9
    pixels[x0 + x, y0 + 7] = 8
for y in range(8):
    pixels[x0, y0 + y] = 9
    pixels[x0 + 7, y0 + y] = 8

# Tile 15: spare (leave as background)

# === ROW 1 (tiles 16-31): wide block tiles (16x8, left+right halves) ===
# Each game cell on BG1 uses 2 tiles side-by-side for a 16x8 pixel block.
# Tile N*2+16 = left half, N*2+17 = right half (N=0..6 for 7 piece colors).


def draw_wide_block_left(tx, ty, color_idx):
    """Left half of a 16x8 wide block: bevel on top, left, bottom only."""
    x0, y0 = tx * 8, ty * 8
    for y in range(8):
        for x in range(8):
            pixels[x0 + x, y0 + y] = color_idx
    # Light bevel: top row + left column
    for x in range(8):
        pixels[x0 + x, y0] = 9
    for y in range(8):
        pixels[x0, y0 + y] = 9
    # Dark bevel: bottom row only (right edge is interior seam)
    for x in range(8):
        pixels[x0 + x, y0 + 7] = 8
    # Corners
    pixels[x0, y0] = 9          # top-left: light
    pixels[x0, y0 + 7] = 8      # bottom-left: dark


def draw_wide_block_right(tx, ty, color_idx):
    """Right half of a 16x8 wide block: bevel on top, right, bottom only."""
    x0, y0 = tx * 8, ty * 8
    for y in range(8):
        for x in range(8):
            pixels[x0 + x, y0 + y] = color_idx
    # Light bevel: top row only (left edge is interior seam)
    for x in range(8):
        pixels[x0 + x, y0] = 9
    # Dark bevel: right column + bottom row
    for y in range(8):
        pixels[x0 + 7, y0 + y] = 8
    for x in range(8):
        pixels[x0 + x, y0 + 7] = 8
    # Corners
    pixels[x0 + 7, y0] = 9      # top-right: light wins
    pixels[x0 + 7, y0 + 7] = 8  # bottom-right: dark


# Tiles 16-29: wide block pairs for 7 piece colors
for i in range(7):
    draw_wide_block_left(i * 2, 1, i + 1)
    draw_wide_block_right(i * 2 + 1, 1, i + 1)

# Tiles 30-31: wide ghost piece
draw_wide_block_left(14, 1, 14)
draw_wide_block_right(15, 1, 14)

# === ROWS 2-6 (tiles 32-111): ASCII font ===
# Tile index = ASCII code (space=32, '0'=48, 'A'=65, etc.)
# Row 2: tiles 32-47 (space through '/')
# Row 3: tiles 48-63 ('0' through '?')
# Row 4: tiles 64-79 ('@' through 'O')
# Row 5: tiles 80-95 ('P' through '_')

for ascii_code in range(32, 96):
    ch = chr(ascii_code)
    tile_idx = ascii_code
    tx = tile_idx % 16
    ty = tile_idx // 16
    draw_char(tx, ty, ch)

img.save('tiles.png')
print(f"Generated tiles.png ({W}x{H}, {len(PALETTE)} colors)")

# =============================================================================
# Generate font2bpp.bin — raw 2bpp SNES tile data for BG3 message overlay
# =============================================================================
# 96 tiles (indices 0-95), 16 bytes per 2bpp tile = 1536 bytes.
# Tiles 0-31: empty. Tiles 32-95: ASCII font (space through '_').
#
# 2bpp SNES tile format: 8 rows per tile, 2 bytes per row.
#   byte 0 = bitplane 0, byte 1 = bitplane 1.
#   Pixel value = (bp1_bit << 1) | bp0_bit.
# Font uses pixel value 1 only (bp0=data, bp1=0).
#
# The 5x7 font is centered at pixel offset (2,1) in the 8x8 tile.
# Font bitmap bit 4 = leftmost column → tile bit 5 (after <<1 shift).

font_data = bytearray()

for tile_idx in range(96):
    ch = chr(tile_idx) if 32 <= tile_idx <= 95 else None
    glyph = BREAKOUT_FONT.get(ch) if ch else None

    for y in range(8):
        bp0 = 0
        if glyph:
            for x in range(8):
                if glyph[y][x]:  # any non-zero pixel = "on" in 2bpp
                    bp0 |= (1 << (7 - x))
        bp1 = 0
        font_data.append(bp0)
        font_data.append(bp1)

with open('font2bpp.bin', 'wb') as f:
    f.write(font_data)

print(f"Generated font2bpp.bin ({len(font_data)} bytes, {len(font_data) // 16} tiles)")
