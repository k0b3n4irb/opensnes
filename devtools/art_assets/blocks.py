"""The Arcade's well: data containers stacked in a cabinet, for games/blocks.

Imported by build.py. The game addresses its tiles by number, so the sheet
keeps the layout the game was written for, and everything is drawn here:

- tiles.png, 96 4bpp tiles in a 16x6 sheet, shared by BG1 (the well) and
  BG2 (the HUD): 0 blank, 1-7 a data container of each piece's colour
  (palette 0, colours 1-7, with the shared edges 8-9 and the grooves 10),
  8-10 the cabinet's frame, corner, top edge and side, the other three
  corners and edges being the map's flips (palette 1, colours 1-7), 14 the
  ghost container (colour 14), 32-95 the city's font at its ASCII codes
  (the library's 8x8 font: body 11, shadow 12, in whichever of palettes
  1-5 the HUD names). The sheet's PNG palette is palette 0; palettes 1-5
  are the tables of data.asm, BORDER and LABELS below;
- font2bpp.bin, BG3's 2bpp font, 96 tiles at their ASCII codes (0-31
  blank): the same glyphs, body colour 1 (the colour main.c cycles), a
  shadow in colour 2.
"""
from __future__ import annotations

from PIL import Image


def hexrgb(h: str) -> tuple:
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


def bgr15(h: str) -> int:
    r, g, b = (v >> 3 for v in hexrgb(h))
    return r | g << 5 | b << 10


# palette 0, the sheet's PNG palette: 0 the well (the HDMA gradient replaces
# it), 1-7 the containers (I O T S Z L J), 8 their dark edge, 9 their lit
# edge, 10 their grooves, 14 the ghost
CONTAINERS = ["#48d0e0", "#f0b050", "#8060a8", "#40d070", "#e05090", "#e07038", "#48a0a0"]
SHEET = (["#101830"] + CONTAINERS + ["#080810", "#e8f8ff", "#203048", "#000000", "#000000", "#000000",
                                     "#282438", "#000000"])
# palette 1 (data.asm border_pal): the cabinet 1-7 as in breakout, 10 the
# messages' shadow (BG3 palette 6 is CGRAM 24-27), 11-12 the HUD's white font
BORDER = {1: "#101828", 2: "#282438", 3: "#403858", 4: "#686088", 5: "#b8b0d0", 6: "#e05090", 7: "#f8a0d0",
          9: "#e8f8ff", 10: "#080810", 11: "#e8f8ff", 12: "#283850"}
# palettes 2-5 (data.asm): the labels SCORE, LEVEL, LINES, NEXT: body, shadow
LABELS = [("#e05090", "#401028"), ("#48d0e0", "#103040"), ("#a888d8", "#281838"), ("#f0b050", "#402810")]
# the colours main.c cycles on PRESS START and GAME OVER (msg_colors)
MESSAGE = ["#e8f8ff", "#e05090", "#f0b050", "#40d070", "#48d0e0", "#a888d8", "#e07038", "#48a0a0"]

# the frame, from the outside in: a steel beam with a magenta neon tube (breakout's cabinet)
BEAM = [2, 3, 4, 5, 4, 6, 7, 6]

CONTAINER = ["99999998",
             "9b#b#bb8",
             "9b#b#b98",
             "9b#b#bb8",
             "9b#b#bb8",
             "9b#b#bb8",
             "9b#b#bb8",
             "88888888"]


def _put(sheet: Image.Image, n: int, px: list[list[int]]) -> None:
    x0, y0 = (n % 16) * 8, (n // 16) * 8
    for y in range(8):
        for x in range(8):
            if px[y][x]:
                sheet.putpixel((x0 + x, y0 + y), px[y][x])


def _glyph(rows: list[int], body: int, shadow: int) -> list[list[int]]:
    """A glyph of the font (eight rows of bits, bit 7 the left pixel) with its
    shadow one pixel right and one down."""
    px = [[0] * 8 for _ in range(8)]
    for y in range(8):
        for x in range(8):
            if rows[y] >> (7 - x) & 1:
                px[y][x] = body
            elif y and x and rows[y - 1] >> (8 - x) & 1:
                px[y][x] = shadow
    return px


def tiles(font: dict[str, list[int]]) -> Image.Image:
    sheet = Image.new("P", (128, 96), 0)
    sheet.putpalette([v for c in SHEET for v in hexrgb(c)] + [0] * (768 - 3 * len(SHEET)))
    for n in range(1, 8):                       # the containers: two grooves, a lit diode
        key = {"9": 9, "8": 8, "b": n, "#": 10}
        _put(sheet, n, [[key[ch] for ch in row] for row in CONTAINER])
    corner = [[BEAM[min(x, y)] for x in range(8)] for y in range(8)]
    for x, y in ((0, 0), (1, 0), (0, 1)):
        corner[y][x] = 0                        # the cabinet's rounded corner
    _put(sheet, 8, corner)
    _put(sheet, 9, [[BEAM[y]] * 8 for y in range(8)])
    _put(sheet, 10, [list(BEAM) for _ in range(8)])
    _put(sheet, 14, [[9 if y == 0 or x == 0 else 8 if y == 7 or x == 7 else 14 for x in range(8)]
                     for y in range(8)])
    for code in range(32, 96):
        _put(sheet, code, _glyph(font[chr(code)], 11, 12))
    return sheet


def font2bpp(font: dict[str, list[int]]) -> bytes:
    out = bytearray(16 * 96)
    for code in range(32, 96):
        px = _glyph(font[chr(code)], 1, 2)
        for y in range(8):
            out[code * 16 + 2 * y] = sum(1 << (7 - x) for x in range(8) if px[y][x] & 1)
            out[code * 16 + 2 * y + 1] = sum(1 << (7 - x) for x in range(8) if px[y][x] & 2)
    return bytes(out)


def build_blocks(font: dict[str, list[int]]) -> dict[str, object]:
    return {"tiles": tiles(font), "font2bpp": font2bpp(font)}
