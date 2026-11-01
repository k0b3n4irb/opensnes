"""Neon Breaker: the bar's arcade cabinet, for games/breakout.

Imported by build.py. The game addresses its tiles by number, so the sheet
keeps the numbering of the game it was ported from, and everything is
drawn here in that layout:

- tiles.png, 128 4bpp tiles in a 16x8 sheet, shared by BG1 (VRAM $1000) and
  the sprites' second name table, which reads the same words: 0 blank,
  1 the cabinet's corner, 2-7 its top beam (six tiles that repeat without a
  seam), 8-11 its side (four tiles that repeat without a seam, the right
  side is the left one mirrored by the map), 12 unused, 13-14 a brick (its dark edge, body and
  light are colours 13-15 and its glint colour 12 of whichever BG palette the
  map entry names, one palette per brick colour; its top row is empty, the joint
  between two rows of bricks), 15-17 the
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
from pathlib import Path

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
BRICKS = [["#6a6a88", "#c8c8e0", "#ffffff"], ["#b05010", "#f89020", "#ffd070"],
          ["#a02068", "#f050a8", "#ffa0d8"], ["#0870a0", "#18c8f0", "#a0f0ff"],
          ["#503098", "#9068e0", "#d0b8ff"], ["#107028", "#28d048", "#a0ffb0"],
          ["#a01020", "#f03038", "#ff9890"], ["#988008", "#f0d820", "#fff890"]]
# the sprites' palette 0 (CGRAM 128-143)
OBJ = ["#000000", "#ffffff", "#080810", "#c0fcff", "#48d0e0", "#2890a8", "#e05090", "#8060a8",
       "#403858", "#b8b0d0", "#f8a0d0", "#686088", "#282438", "#d8d0f0", "#0a0e1a"]
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
        if p > 2:
            pal[p * 16 + 12] = (255, 255, 255)      # the glint: 0 has the cabinet's white, 1 and 2 the fonts'
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
        shade = [tuple(v * 4 // 5 for v in hexrgb(c)) for c in cols]
        words = [0] + [bgr15(c) for c in cols] + [0] + [bgr15("#%02x%02x%02x" % s) for s in shade]
        out += struct.pack("<8H", *words)
    return out


def _put(sheet: Image.Image, n: int, rows: list[str], key: dict[str, int]) -> None:
    x0, y0 = (n % 16) * 8, (n // 16) * 8
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            sheet.putpixel((x0 + x, y0 + y), key[ch])


def brick() -> list[list[int]]:
    """The brick, 16x8 in two tiles: an outline and a shade in the dark colour (13), a bevel in
    the light one (15) on its top and left, a glint (12) and the body (14); its top row is
    empty, the joint between two rows. Drawn here."""
    rows = ["................", ".DDDDDDDDDDDDDD.", "DLLLLLLLLLLLLLLD", "DLWWBBBBBBBBBBBD",
            "DLWBBBBBBBBBBBBD", "DLBBBBBBBBBBBBBD", "DBBBBBBBBBBBBBBD", ".DDDDDDDDDDDDDD."]
    k = {".": 0, "D": 13, "B": 14, "L": 15, "W": 12}
    return [[k[c] for c in r] for r in rows]


# The cabinet's drawing alphabet, to CABINET's colour numbers: 0 the ink, 1-4 the steel from
# dark to light, w a white glint, c / C the cyan of the light channel, t teal
CAB_INDEX = {".": 0, "0": 1, "1": 2, "2": 3, "3": 4, "4": 5, "m": 6, "n": 7, "t": 8, "C": 9, "c": 11, "w": 12}


def cabinet_wall() -> list[list[str]]:
    """The cabinet's side, 8x32 (four tiles, meeting top to bottom): dark steel with a channel of
    cyan light down its middle, and a bracket across it every 16 pixels. Drawn here."""
    rows = []
    for r in range(32):
        k = r % 16
        if k < 3:                                   # the bracket: a groove, a bevel, a glint
            rows.append(["01111110", "04444320", "04wwww20"][k])
        elif k == 3:
            rows.append("02111120")
        else:                                       # the channel, its two lights alternating
            rows.append("0421cc20" if k % 2 else "0421Cc20")
    g = [list(r) for r in rows]
    for y in range(32):
        if y % 16 in (1, 2):
            g[y][2], g[y][5] = "4", "3"
    return g


def cabinet_beam() -> list[list[str]]:
    """The cabinet's top beam, 48x8 (six tiles, meeting end to start): the same channel of light
    along it, and a bracket every 16 pixels. Drawn here."""
    g = [list(r) for r in ("0" * 48, "4" * 48, "3" * 48, "1" * 48, "cC" * 24, "c" * 48, "2" * 48, "0" * 48)]
    lit = ["0", "4", "w", "3", "3", "2", "1", "0"]
    plain = ["0", "4", "4", "3", "3", "2", "1", "0"]
    for p in range(3):
        for y in range(1, 7):
            for x in range(p * 16, p * 16 + 4):
                g[y][x] = lit[y] if x in (p * 16 + 1, p * 16 + 2) else plain[y]
    return g


def tiles(art: Path, font: dict[str, list[int]]) -> Image.Image:
    sheet = Image.new("P", (128, 64), 0)
    sheet.putpalette([v for c in palette() for v in c])
    # the top beam (2-7) and the side (8-11); the right side is the left one mirrored by the map
    beam, wall = cabinet_beam(), cabinet_wall()
    for n in range(6):
        for y in range(8):
            for x in range(8):
                sheet.putpixel((((2 + n) % 16) * 8 + x, y), CAB_INDEX[beam[y][n * 8 + x]])
    for n in range(4):
        for y in range(8):
            for x in range(8):
                sheet.putpixel((((8 + n) % 16) * 8 + x, (8 + n) // 16 * 8 + y), CAB_INDEX[wall[n * 8 + y][x]])
    # the corner: the beam above its diagonal, the side below it, rounded
    for y in range(8):
        for x in range(8):
            v = CAB_INDEX[beam[y][x] if x >= y else wall[y][x]]
            if (x, y) in ((0, 0), (1, 0), (0, 1)):
                v = 0
            sheet.putpixel((8 + x, y), v)
    # the brick, 16x8 in two tiles (13 its left half, 14 its right)
    for y, row in enumerate(brick()):
        for x, v in enumerate(row):
            sheet.putpixel(((13 + x // 8) % 16 * 8 + x % 8, (13 // 16) * 8 + y), v)
    # the paddle: cyan emitters at the ends, a violet body with a magenta stripe
    o = {".": 0, "W": 1, "s": 2, "C": 3, "c": 4, "e": 5, "m": 6, "v": 7, "V": 8, "h": 9, "p": 10,
         "S": 11, "D": 12, "A": 13, "z": 14}
    # the paddle, 32x8 from three tiles (15 its left end, 16 its middle, drawn once and
    # mirrored by the second sprite, 17 its right end, the left one mirrored): a one-colour
    # outline (s, the shadow's ink), the light from above (A to D down the body), a channel of
    # cyan light along it and a magenta diode at each end, like the cabinet's
    half = ["...sssssssssssss", "..sAAAAAAAAAAAAA", ".shhhhhhhhhhhhhh", ".shhpmCCCCCCCCCC",
            ".sSSmmcccccccccc", ".sSSSSSSSSSSSSSS", "..sDDDDDDDDDDDDD", "...sssssssssssss"]
    _put(sheet, 15, [r[:8] for r in half], o)
    _put(sheet, 16, [r[8:] for r in half], o)
    _put(sheet, 17, [r[:8][::-1] for r in half], o)
    # the paddle's shadow: its silhouette in a colour close to the board's (z, 14), the right
    # end being the left one flipped by its sprite
    _put(sheet, 18, ["...zzzzz", "..zzzzzz", ".zzzzzzz", ".zzzzzzz", ".zzzzzzz", ".zzzzzzz", "..zzzzzz", "...zzzzz"], o)
    _put(sheet, 19, ["zzzzzzzz"] * 8, o)
    # the ball: an outline, the light from the top left, a shade at the bottom right
    _put(sheet, 20, ["..ssss..", ".sCCcces", "sCWWcces", "sCWccces", "sccccees", "sccceees", ".seeees.", "..ssss.."], o)
    _put(sheet, 21, ["..zzzz..", ".zzzzzz.", "zzzzzzzz", "zzzzzzzz", "zzzzzzzz", "zzzzzzzz", ".zzzzzz.", "..zzzz.."], o)
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
        m[32 * 1 + x] = 2 + (x - 2) % 6
    for y in range(2, 28):
        tile = 8 + (y - 2) % 4
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
    """The board: a 32x32 map of 16x16 cells, palette 4, under the playfield only (cells 1-10 across,
    tiles 2-21, from the beam down): the score panel keeps the backdrop. Most cells are the quiet one (two pairs of
    dots on the board's colour), a few the cross and the outline, so the circuit is a texture."""
    rng = random.Random(21)
    m = [0] * 1024
    for cy in range(1, 16):
        for cx in range(1, 11):
            c = rng.choice((2, 2, 2, 2, 2, 1, 1, 0))
            for q in range(4):
                x, y = cx * 2 + (q & 1), cy * 2 + (q >> 1)
                m[32 * y + x] = (1 + c * 4 + q) | 4 << 10
    return struct.pack("<1024H", *m)


def build_breakout(art: Path, font: dict[str, list[int]]) -> dict[str, Image.Image | bytes]:
    return {"tiles": tiles(art, font), "back": back(), "bg1map": bg1map(), "bg2map": bg2map(),
            "backpal": backpal()}
