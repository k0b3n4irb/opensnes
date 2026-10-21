"""The Mode 7 city: a ground seen from above and a skyline over its horizon.

Imported by build.py. The ground is 1024x1024 in at most 256 distinct 8x8
tiles (a Mode 7 map names one byte per tile), so it is not a picture cut
down but a city laid out from pieces:

- two Wang tilesets of 16x16 tiles from PixelLab (tiles/wang_street_roof.png,
  tiles/wang_street_canal.png), placed by the corners of a vertex grid:
  streets every 128 px, the roofs between them, a canal each way;
- the round plaza, cut from the aerial view (backgrounds/aerial.png) at its
  full size, on an esplanade of four blocks at the centre of the map, with
  neon lines drawn here leading to it;
- helipads and a skylight from the same aerial view, one on most roofs.

The map wraps: its last vertex row and column are its first.

The sky is the mid-distance towers (backgrounds/skyline_mid.png) at half
size, each half followed by its mirror so it wraps at 512, over a night
gradient. It is a 4bpp layer that reads CGRAM 0-15, the ground's first
sixteen colours, so the ground's palette starts with the sky's.
"""
from __future__ import annotations

import math
import random
from pathlib import Path

from PIL import Image, ImageOps

CELLS = 64                    # 16-px cells across the map
BLOCK = 8                     # cells per block, street included
INK = (8, 8, 16)              # colour 0: the backdrop shows there, the same colour
AMBER = (240, 176, 80)
MAGENTA = (224, 80, 144)
GRADIENT = [(16, 16, 40), (32, 24, 56)]   # the night above the towers, then near them
HORIZON = 96                  # sky rows above the ground in mode7/perspective


def bayer4(x: int, y: int) -> int:
    return [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]][y & 3][x & 3]


def grid() -> list[list[str]]:
    """The terrain at each vertex: l the street, u a roof, c the canal."""
    n = CELLS + 1
    v = [["l" if x % BLOCK == 0 or y % BLOCK == 0 else "u" for x in range(n)] for y in range(n)]
    for y in range(n):                       # the canal down: water on 16-17, streets either side
        v[y][15] = v[y][18] = "l"
        v[y][16] = v[y][17] = "c"
    for x in range(n):                       # and across, on 48-49
        for y in (47, 50):
            if v[y][x] != "c":
                v[y][x] = "l"
        v[48][x] = v[49][x] = "c"
    for y in range(25, 40):                  # the esplanade: the four blocks round the centre
        for x in range(25, 40):
            v[y][x] = "l"
    for i in range(n):                       # wrap
        v[CELLS][i] = v[0][i]
        v[i][CELLS] = v[i][0]
    return v


def wang(sheet: Image.Image) -> dict[int, Image.Image]:
    """The 16 tiles of a sheet, keyed NW<<3 | NE<<2 | SW<<1 | SE (1 = the upper terrain)."""
    return {k: sheet.crop(((k % 4) * 16, (k // 4) * 16, (k % 4) * 16 + 16, (k // 4) * 16 + 16)) for k in range(16)}


def disk(src: Image.Image, cx: int, cy: int, r: int) -> list[tuple[int, int, tuple]]:
    """The pixels of `src` within `r` of (cx, cy), as offsets from the disk's top left."""
    return [(x + r, y + r, src.getpixel((cx + x, cy + y)))
            for y in range(-r, r) for x in range(-r, r) if x * x + y * y < r * r]


def rect(src: Image.Image, box: tuple[int, int, int, int]) -> list[tuple[int, int, tuple]]:
    x0, y0, x1, y1 = box
    return [(x - x0, y - y0, src.getpixel((x, y))) for y in range(y0, y1) for x in range(x0, x1)]


def ground_rgb(art: Path) -> Image.Image:
    roof = wang(Image.open(art / "tiles" / "wang_street_roof.png").convert("RGB"))
    canal = wang(Image.open(art / "tiles" / "wang_street_canal.png").convert("RGB"))
    aerial = Image.open(art / "backgrounds" / "aerial.png").convert("RGB")
    v = grid()
    out = Image.new("RGB", (CELLS * 16, CELLS * 16))
    for y in range(CELLS):
        for x in range(CELLS):
            c = (v[y][x], v[y][x + 1], v[y + 1][x], v[y + 1][x + 1])
            assert not ("u" in c and "c" in c), (x, y)
            upper = "c" if "c" in c else "u"
            key = sum((t == upper) << b for t, b in zip(c, (3, 2, 1, 0)))
            out.paste((canal if upper == "c" else roof)[key], (x * 16, y * 16))

    def stamp(pixels, x0, y0):
        for x, y, p in pixels:
            out.putpixel((x0 + x, y0 + y), p)

    # the plaza at the centre of the map, the esplanade's neon round it
    r = 40
    stamp(disk(aerial, 256, 256, r), 512 - r, 512 - r)
    lo, hi = 24 * 16 + 8, 40 * 16 - 8        # the esplanade's street, 8 px in from its curbs
    for t in range(lo, hi):
        for w in (0, 1):
            for x, y in ((t, lo + w), (t, hi - 1 - w), (lo + w, t), (hi - 1 - w, t)):
                out.putpixel((x, y), MAGENTA)
            if not 512 - r - 2 < t < 512 + r + 2:
                out.putpixel((t, 511 + w), AMBER)
                out.putpixel((511 + w, t), AMBER)

    # one piece on most roofs, centred on the roof and on the 16-px grid
    pieces = [disk(aerial, 25, 22, 20), disk(aerial, 489, 23, 20), rect(aerial, (200, 90, 232, 118))]
    rng = random.Random(3)
    for by in range(CELLS // BLOCK):
        for bx in range(CELLS // BLOCK):
            k = rng.randrange(4)
            if bx in (3, 4) and by in (3, 4) or k == 3:
                continue
            roofcells = [(x, y) for y in range(by * BLOCK, by * BLOCK + BLOCK) for x in range(bx * BLOCK, bx * BLOCK + BLOCK)
                         if v[y][x] == v[y][x + 1] == v[y + 1][x] == v[y + 1][x + 1] == "u"]
            xs = [x for x, _ in roofcells]
            ys = [y for _, y in roofcells]
            w = max(px for px, _, _ in pieces[k]) + 1
            h = max(py for _, py, _ in pieces[k]) + 1
            cx = (min(xs) + max(xs) + 1) * 8 - w // 2
            cy = (min(ys) + max(ys) + 1) * 8 - h // 2
            stamp(pieces[k], cx // 16 * 16, cy // 16 * 16)
    return out


def half_size(im: Image.Image) -> Image.Image:
    """Half size, each pixel the commonest opaque colour of its 2x2 (transparent if most are)."""
    out = Image.new("RGBA", (im.width // 2, im.height // 2), (0, 0, 0, 0))
    for y in range(out.height):
        for x in range(out.width):
            px = [im.getpixel((2 * x + i, 2 * y + j)) for j in (0, 1) for i in (0, 1)]
            opaque = [p[:3] for p in px if p[3] >= 128]
            if len(opaque) >= 2:
                out.putpixel((x, y), max(opaque, key=opaque.count) + (255,))
    return out


def snap(c: tuple) -> tuple:
    return tuple(v & 0xF8 for v in c[:3])


def sky_rgb(art: Path) -> tuple[Image.Image, list[tuple]]:
    """The 512x256 sky in RGB, and its colours (INK first, then the gradient, then the towers)."""
    mid = half_size(Image.open(art / "backgrounds" / "skyline_mid.png").convert("RGBA"))
    towers = Image.new("RGBA", (512, mid.height), (0, 0, 0, 0))
    towers.paste(mid, (0, 0))
    towers.paste(ImageOps.mirror(mid), (256, 0))
    # 13 colours for the towers, the 15-bit grid
    opaque = Image.new("RGB", (512, mid.height), INK)
    opaque.paste(towers, (0, 0), towers)
    q = opaque.quantize(13, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    qp = q.getpalette()
    tones = [snap(tuple(qp[i:i + 3])) for i in range(0, len(qp) - 2, 3)]
    used = sorted({i for _, i in q.getcolors()})
    colours = [INK] + GRADIENT + [c for c in dict.fromkeys(tones[i] for i in used) if c not in (INK, *GRADIENT)]
    assert len(colours) <= 16, len(colours)
    out = Image.new("RGB", (512, 256), INK)
    top = HORIZON - mid.height
    for y in range(HORIZON):
        for x in range(512):
            # three bands down to the horizon, an ordered dither between them
            f = y * 2 * 16 // HORIZON
            band = min(2, f // 16 + (bayer4(x, y) < f % 16))
            out.putpixel((x, y), ([INK] + GRADIENT)[band])
            ty = y - top
            if 0 <= ty < mid.height and towers.getpixel((x, ty))[3] >= 128:
                out.putpixel((x, y), tones[q.getpixel((x, ty))])
    return out, colours


def indexed(rgb: Image.Image, colours: list[tuple]) -> Image.Image:
    lut = {c: i for i, c in enumerate(colours)}
    out = Image.new("P", rgb.size, 0)
    out.putpalette([v for c in colours for v in c] + [0] * (768 - 3 * len(colours)))
    out.putdata([lut[p] for p in rgb.getdata()])
    return out


def build_city(art: Path) -> dict[str, Image.Image]:
    """ground.png (1024x1024, Mode 7) and sky.png (512x256, 16 colours) on one palette."""
    sky, colours = sky_rgb(art)
    ground = ground_rgb(art)
    colours = colours + sorted(set(ground.getdata()) - set(colours))
    tiles = {ground.crop((x, y, x + 8, y + 8)).tobytes() for y in range(0, 1024, 8) for x in range(0, 1024, 8)}
    assert len(tiles) <= 256, f"{len(tiles)} tiles: a Mode 7 map names 256"
    return {"ground": indexed(ground, colours), "sky": indexed(sky, colours[:16]), "_tiles": len(tiles),
            "_colours": len(colours)}


def glider(art: Path) -> Image.Image:
    """Kit's delivery glider from above (characters/glider.png), indexed: 0 transparent, then its colours."""
    im = Image.open(art / "characters" / "glider.png").convert("RGBA")
    colours = sorted({p[:3] for p in im.getdata() if p[3] >= 128})
    assert len(colours) <= 15, len(colours)
    out = Image.new("P", im.size, 0)
    out.putpalette([0, 0, 0] + [v for c in colours for v in c] + [0] * (765 - 3 * len(colours)))
    out.putdata([colours.index(p[:3]) + 1 if p[3] >= 128 else 0 for p in im.getdata()])
    return out


# mode7/extbg: the city seen from above with the elevated metro over it. The
# viaducts are the high half of the plane (bit 7 set): their colours sit at
# indices 1-13, so that 0x81-0x8D read as colours 1-13 on EXTBG's BG2 and, as
# BG1 reads all eight bits, as CGRAM 129-141, which the palette repeats; the
# city's own colours start at 16. The window shown is 256x224 at 1:1; the
# rest of the 1024x1024 plane repeats it.
EXTBG_WINDOW = (160, 400)               # the city crop: two roof blocks, a canal, a cross street
DECKS = (24, 144, 208)                  # the viaducts' left edges, on the 8-px grid
DECK_W = 24
DECK = {"hi": "#b8b0d0", "con": "#8880a8", "mid": "#686088", "lo": "#403858", "ballast": "#282438",
        "sleeper": "#504868", "rail": "#a8c8d8", "glint": "#e8f8ff", "amber": "#f0b050",
        "roof": "#585070", "stripe": "#e05090", "window": "#f8d888", "head": "#ffffff"}


def hexrgb(h: str) -> tuple:
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


def extbg_plane(art: Path) -> tuple[Image.Image, int]:
    """plane.png for mode7/extbg (1024x1024, indexed as above) and its count of distinct tiles."""
    city = ground_rgb(art).crop((*EXTBG_WINDOW, EXTBG_WINDOW[0] + 256, EXTBG_WINDOW[1] + 224))
    deck = {k: hexrgb(v) for k, v in DECK.items()}
    tones = sorted(set(city.getdata()), key=sum)

    def shade(c: tuple) -> tuple:
        return tones[max(0, tones.index(c) - 4)]

    low = city.copy()
    high = {}                                       # (x, y) -> a deck colour
    for x0 in DECKS:
        for y in range(224):
            for x in range(x0 + DECK_W, x0 + DECK_W + 8):   # the shadow, light from the upper left
                low.putpixel((x, y), shade(city.getpixel((x, y))))
            for dx in range(DECK_W):
                if dx == 0:
                    c = "hi"
                elif dx == DECK_W - 1:
                    c = "lo"
                elif dx in (1, DECK_W - 2):
                    c = "amber" if dx == 1 and y % 32 == 16 else "con"
                elif dx in (2, DECK_W - 3, 11, 12):
                    c = "mid"                        # the walkways and the centre beam
                elif dx in (5, 8, 15, 18):
                    c = "glint" if y % 16 == 3 else "rail"
                else:
                    c = "sleeper" if y % 4 == 0 else "ballast"
                high[(x0 + dx, y)] = c
    tx = DECKS[1] + 3                               # a three-car train on the middle viaduct
    for car in range(3):
        ty = 20 + car * 46
        for y in range(ty, ty + 42):
            for x in range(tx, tx + 8):
                edge = x in (tx, tx + 7) or y in (ty, ty + 41)
                c = "lo" if edge else "roof"
                if not edge and x in (tx + 1, tx + 6) and (y - ty) % 6 < 4 and 3 < y - ty < 38:
                    c = "window"
                if not edge and x in (tx + 3, tx + 4):
                    c = "stripe"
                if car == 0 and y == ty + 1 and x in (tx + 2, tx + 5):
                    c = "head"
                high[(x, y)] = c

    names = list(DECK)
    citycols = [INK] + [c for c in tones if c != INK]
    assert len(names) <= 15 and len(citycols) - 1 <= 112, (len(names), len(citycols))
    lut = {c: (0 if i == 0 else 15 + i) for i, c in enumerate(citycols)}
    window = Image.new("P", (256, 224), 0)
    for y in range(224):
        for x in range(256):
            k = high.get((x, y))
            window.putpixel((x, y), 0x81 + names.index(k) if k else lut[low.getpixel((x, y))])
    pal = [(0, 0, 0)] * 256
    for c, i in lut.items():
        pal[i] = c
    for i, k in enumerate(names):
        pal[1 + i] = pal[0x81 + i] = deck[k]
    plane = Image.new("P", (1024, 1024), 0)
    for y in range(0, 1024, 224):
        for x in range(0, 1024, 256):
            plane.paste(window, (x, y))
    plane.putpalette([v for c in pal for v in c])
    tiles = {window.crop((x, y, x + 8, y + 8)).tobytes() for y in range(0, 224, 8) for x in range(0, 256, 8)}
    assert len(tiles) <= 256, len(tiles)
    return plane, len(tiles)


def orb() -> Image.Image:
    """The energy orb (O6), eight 16x16 frames of a pulse in a 128-px row (one VRAM tile row), indexed: 0 transparent."""
    cols = ["#000000", "#ffffff", "#c0fcff", "#58d8e8", "#2890a8", "#e05090", "#f8a0d0"]
    out = Image.new("P", (128, 16), 0)
    out.putpalette([v for c in cols for v in hexrgb(c)] + [0] * (768 - 3 * len(cols)))
    for f in range(8):
        g = (0, 1, 2, 3, 4, 3, 2, 1)[f] / 4                 # the glow breathes out and back
        for y in range(16):
            for x in range(16):
                r = math.hypot(x - 7.5, y - 7.5) + ((x - 7.5) + (y - 7.5)) * 0.12   # lit from the upper left
                if r <= 1.6 + g:
                    c = 1
                elif r <= 3.2 + g:
                    c = 2
                elif r <= 4.6 + g:
                    c = 3
                elif r <= 5.6 + g * 0.5:
                    c = 4
                elif r <= 7.6 and (x + y + f) % 2 == 0:
                    c = 5 if (x * 5 + y * 3 + f * 4) % 7 else 6     # the halo, dithered, a spark here and there
                else:
                    c = 0
                out.putpixel((f * 16 + x, y), c)
    return out
