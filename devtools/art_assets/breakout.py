"""Neon Breaker: the bar's arcade cabinet, for games/breakout.

Imported by build.py. The game addresses its tiles by number, so the sheet
keeps the numbering of the game it was ported from, and everything is
drawn here in that layout:

- tiles.png, 128 4bpp tiles in a 16x8 sheet, shared by BG1 (VRAM $1000) and
  the sprites' second name table, which reads the same words: 0 blank,
  1 the cabinet's corner, 2-7 its top edge, 8 its sides (the right
  ones are the left ones mirrored by the map), 9-12 a
  speaker grille on the sides, 13-14 a brick (colours 13-15 of whichever
  BG palette the map entry names: one palette per brick colour), 15-17 the
  paddle (left end, middle, right end; the middle drawn twice, the second
  mirrored), 18-19 its shadow, 20 the ball, 21 its shadow, 22-117 the font
  (ASCII 32-127, colours 10-12), 118-119 the lives icon. The sheet's PNG
  palette is the whole CGRAM image (256 colours): BG palettes 0-7, then the
  sprites' palette 0 at 128;
- back.png, BG3's thirteen 2bpp tiles: 0 blank, 1-12 a circuit board in
  2x2 cells, three cells and their lit variants;
- bg1map.bin, the cabinet and the HUD labels (32x32, BG1);
- bg2map.bin, the board under the playfield (32x32, BG3, palette 4: the
  game adds 1 to the palette under each brick for its shadow);
- backpal.bin, seven levels of BG3 colours, CGRAM 16-23 (palette 4 and its
  shadow, palette 5), the game copies one set per level.
"""
from __future__ import annotations

import random
import struct

from PIL import Image


def hexrgb(h: str) -> tuple:
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


def bgr15(h: str) -> int:
    r, g, b = (v >> 3 for v in hexrgb(h))
    return r | g << 5 | b << 10


# BG palette 0: the cabinet 1-7, the lives icon 8-9, the labels' font 10-12
CABINET = ["#080810", "#101828", "#282438", "#403858", "#686088", "#b8b0d0", "#e05090", "#f8a0d0",
           "#48a0a0", "#b0f8ff", "#101828", "#48d0e0", "#e8f8ff"]
# the font in palettes 1 (numbers) and 2 (SCORE): shadow, body, highlight
FONTS = {1: ["#101828", "#f0b050", "#fff0c0"], 2: ["#101828", "#e05090", "#f8b0d8"]}
# one brick colour per BG palette 0-7: dark edge (13), body (14), light (15)
BRICKS = [["#686088", "#b8b0d0", "#ffffff"], ["#803818", "#f0b050", "#fff0c0"],
          ["#702050", "#e05090", "#f8b0d8"], ["#185868", "#48d0e0", "#c0fcff"],
          ["#402858", "#8060a8", "#c8b0f0"], ["#186038", "#40d070", "#c0ffd0"],
          ["#681828", "#e03838", "#ffb0a0"], ["#205058", "#48a0a0", "#a8f0e8"]]
# the sprites' palette 0 (CGRAM 128-143)
OBJ = ["#000000", "#ffffff", "#080810", "#c0fcff", "#48d0e0", "#2890a8", "#e05090", "#8060a8",
       "#403858", "#b8b0d0", "#f8a0d0"]
# BG3, one set per level: the board (1-3) and its shadow (5-7)
LEVELS = [("#101830", "#182444", "#24345c"), ("#1c1030", "#281844", "#38205c"),
          ("#0c2024", "#103034", "#18444c"), ("#24101c", "#341828", "#4c2038"),
          ("#102414", "#18341c", "#204c28"), ("#241c0c", "#342810", "#4c3818"),
          ("#181820", "#24242c", "#34343c")]


def palette() -> list[tuple]:
    pal = [(0, 0, 0)] * 256
    for i, c in enumerate(CABINET):
        pal[i] = hexrgb(c)
    for p, cols in FONTS.items():
        for i, c in enumerate(cols):
            pal[p * 16 + 10 + i] = hexrgb(c)
    for p, cols in enumerate(BRICKS):
        for i, c in enumerate(cols):
            pal[p * 16 + 13 + i] = hexrgb(c)
    for i, c in enumerate(OBJ):
        pal[128 + i] = hexrgb(c)
    # CGRAM 16-23 (BG3's palettes 4 and 5): the first level's set, which the
    # game copies only from the second level on
    words = struct.unpack("<8H", backpal()[:16])
    for i, w in enumerate(words):
        pal[16 + i] = ((w & 31) << 3, (w >> 5 & 31) << 3, (w >> 10 & 31) << 3)
    return pal


def backpal() -> bytes:
    out = b""
    for cols in LEVELS:
        shade = [tuple(v * 3 // 5 for v in hexrgb(c)) for c in cols]
        words = [0] + [bgr15(c) for c in cols] + [0] + [bgr15("#%02x%02x%02x" % s) for s in shade]
        out += struct.pack("<8H", *words)
    return out


def _put(sheet: Image.Image, n: int, rows: list[str], key: dict[str, int]) -> None:
    x0, y0 = (n % 16) * 8, (n // 16) * 8
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            sheet.putpixel((x0 + x, y0 + y), key[ch])


def tiles(font: dict[str, list[int]]) -> Image.Image:
    sheet = Image.new("P", (128, 64), 0)
    sheet.putpalette([v for c in palette() for v in c])
    k = {".": 0, "0": 1, "1": 2, "2": 3, "3": 4, "4": 5, "m": 6, "n": 7, "t": 8, "g": 9}
    # the top edge: a steel beam with a magenta neon tube; 2-6 and 7 are the
    # same beam with a bolt at a different place, so the repeats do not show
    beam = ["11111111", "22222222", "33333333", "44444444", "33333333", "mmmmmmmm", "nnnnnnnn", "mmmmmmmm"]
    for n, bolt in zip(range(2, 8), (1, 6, 3, 0, 4, 2)):
        rows = list(beam)
        rows[2] = rows[2][:bolt] + "4" + rows[2][bolt + 1:]
        _put(sheet, n, rows, k)
    # the sides (the right one is the map's mirror of the left), and a
    # speaker grille on them (9-12, then 9 again)
    side = "12343mnm"
    _put(sheet, 8, [side] * 8, k)
    _put(sheet, 9, ["12443mnm"] * 8, k)
    for n in (10, 11, 12):
        _put(sheet, n, ["12443mnm", "1t443mnm", "12443mnm", "1g443mnm", "12443mnm", "1t443mnm", "12443mnm", "12443mnm"], k)
    # the corner: the beam above its diagonal, the side below it, rounded
    corner = [[(beam[y][x] if x >= y else side[x]) for x in range(8)] for y in range(8)]
    for x, y in ((0, 0), (1, 0), (0, 1)):
        corner[y][x] = "."
    _put(sheet, 1, ["".join(r) for r in corner], k)
    # the brick, 16x8 in two tiles: light top and left, dark bottom and right
    b = {".": 0, "d": 13, "b": 14, "l": 15}
    _put(sheet, 13, ["llllllll", "lbbbbbbb", "lbbbbbbb", "lbbbbbbb", "lbbbbbbb", "lbbbbbbb", "lddddddd", "dddddddd"], b)
    _put(sheet, 14, ["lllllllc".replace("c", "d"), "bbbbbbbd", "bbbbbbbd", "bbbbbbbd", "bbbbbbbd", "bbbbbbbd", "dddddddd", "dddddddd"], b)
    # the paddle: cyan emitters at the ends, a violet body with a magenta stripe
    o = {".": 0, "W": 1, "s": 2, "C": 3, "c": 4, "e": 5, "m": 6, "v": 7, "V": 8, "h": 9, "p": 10}
    _put(sheet, 15, ["..eccchh", ".ecCCvhh", "ecCWCvvv", "ecWWCmpp", "ecCWCmmm", "ecCCCvvv", ".eccVVVV", "..eeVVVV"], o)
    _put(sheet, 16, ["hhhhhhhh", "hhhhhhhh", "vvvvvvvv", "pppppppp", "mmmmmmmm", "vvvvvvvv", "VVVVVVVV", "VVVVVVVV"], o)
    _put(sheet, 17, ["hhccce..", "hhvCCce.", "vvvCWCce", "ppmCWWce", "mmmCWCce", "vvvCCCce", "VVVVcce.", "VVVVee.."], o)
    _put(sheet, 18, ["..ssssss", ".sssssss", "ssssssss", "ssssssss", "ssssssss", "ssssssss", ".sssssss", "..ssssss"], o)
    _put(sheet, 19, ["ssssssss"] * 8, o)
    _put(sheet, 20, ["..eeee..", ".ecCCce.", "ecCWWCce", "ecWWWWce", "ecWWWCce", "ecCWCCce", ".ecccce.", "..eeee.."], o)
    _put(sheet, 21, ["..ssss..", ".ssssss.", "ssssssss", "ssssssss", "ssssssss", "ssssssss", ".ssssss.", "..ssss.."], o)
    # the font: the library's glyphs, a highlight on their top row, a shadow below right
    for code in range(33, 127):
        g = font.get(chr(code))
        if not g:
            continue
        n = code - 10
        x0, y0 = (n % 16) * 8, (n // 16) * 8
        on = {(x, y) for y in range(8) for x in range(8) if g[y] >> (7 - x) & 1}
        for x, y in on:
            if (x + 1, y + 1) not in on and x < 7 and y < 7:
                sheet.putpixel((x0 + x + 1, y0 + y + 1), 10)
        for x, y in on:
            sheet.putpixel((x0 + x, y0 + y), 12 if (x, y - 1) not in on else 11)
    # the lives icon: a small paddle, 16x8
    i = {".": 0, "4": 5, "3": 4, "2": 3, "t": 8, "g": 9, "m": 6, "n": 7}
    _put(sheet, 118, ["........", "........", "..tt3333", ".tgg2222", ".tggnnnn", "..tt2222", "........", "........"], i)
    _put(sheet, 119, ["........", "........", "3333tt..", "2222ggt.", "nnnnggt.", "2222tt..", "........", "........"], i)
    return sheet


def back() -> Image.Image:
    """BG3's tiles: three 16x16 cells of a circuit board and their lit variants."""
    im = Image.new("P", (13 * 8, 8), 0)
    im.putpalette([0, 0, 0, 40, 60, 90, 60, 80, 120, 90, 120, 170] + [0] * 756)
    cells = [
        ["1111111111111111", "1111111111111111", "1122222222221111", "1121111111121111",
         "1121111111121111", "1121111111121111", "1121111111122221", "1121111111111121",
         "1121111111111121", "1121111111111121", "1122222111111121", "1111112111111121",
         "1111112111111121", "1111112222222221", "1111111111111111", "1111111111111111"],
        ["1111111211111111", "1111111211111111", "1111111211111111", "1111111211111111",
         "1111111211111111", "1111112221111111", "1111122122111111", "2222221112222222",
         "1111122122111111", "1111112221111111", "1111111211111111", "1111111211111111",
         "1111111211111111", "1111111211111111", "1111111211111111", "1111111211111111"],
        ["1111111111111111", "1111111111111111", "1112211111122111", "1112211111122111",
         "1111111111111111", "1111111111111111", "1111111111111111", "1111111111111111",
         "1111111111111111", "1111111111111111", "1111111111111111", "1111111111111111",
         "1112211111122111", "1112211111122111", "1111111111111111", "1111111111111111"],
    ]
    # tiles 1-4, 5-8, 9-12: one cell each, its four quarters; the lit
    # variant is the same cell with its pads in colour 3, drawn by the map
    # through the cell's own tiles (3 cells x 4 quarters = 12 tiles)
    for c, cell in enumerate(cells):
        for q in range(4):
            n = 1 + c * 4 + q
            for y in range(8):
                for x in range(8):
                    v = int(cell[(q >> 1) * 8 + y][(q & 1) * 8 + x])
                    if c == 2 and v == 2:
                        v = 3                       # the third cell's pads are lit
                    im.putpixel((n * 8 + x, y), v)
    return im


def bg1map() -> bytes:
    """The cabinet and the HUD labels, as the game expects them."""
    m = [0] * 1024
    flip = 0x4000                               # H-flip: the right side mirrors the left
    m[32 * 1 + 1] = 1
    m[32 * 1 + 22] = 1 | flip
    for x in range(2, 22):
        m[32 * 1 + x] = 2 + (x * 7) % 6
    for y in range(2, 28):
        tile = 8
        if 23 <= y <= 27:
            tile = (9, 10, 11, 12, 9)[y - 23]
        m[32 * y + 1] = tile
        m[32 * y + 22] = tile | flip

    def label(s: str, x: int, y: int, pal: int) -> None:
        for i, ch in enumerate(s):
            m[32 * y + x + i] = (ord(ch) - 10 if ch != " " else 0) | pal << 10

    label("HIGH", 24, 2, 0)
    label("SCORE", 25, 3, 0)
    label("50000", 25, 4, 1)
    label("SCORE", 24, 6, 2)
    label("0", 29, 7, 1)
    m[32 * 9 + 25], m[32 * 9 + 26] = 118, 119
    label("04", 28, 9, 1)
    label("ROUND", 25, 22, 0)
    label("1", 29, 23, 1)
    return struct.pack("<1024H", *m)


def bg2map() -> bytes:
    """The board: a 32x32 map of 16x16 cells, palette 4."""
    rng = random.Random(21)
    m = [0] * 1024
    for cy in range(16):
        for cx in range(16):
            c = rng.choice((0, 0, 1, 2))
            for q in range(4):
                x, y = cx * 2 + (q & 1), cy * 2 + (q >> 1)
                m[32 * y + x] = (1 + c * 4 + q) | 4 << 10
    return struct.pack("<1024H", *m)


def build_breakout(font: dict[str, list[int]]) -> dict[str, Image.Image | bytes]:
    return {"tiles": tiles(font), "back": back(), "bg1map": bg1map(), "bg2map": bg2map(),
            "backpal": backpal()}
