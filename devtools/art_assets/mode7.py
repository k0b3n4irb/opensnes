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
