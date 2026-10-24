"""demos/shmup: Kit's glider over the city at night, against the security's hunters.

Imported by build.py. The ground is the Mode 7 city's (mode7.py) laid out
again on a strip of 256x512 that wraps vertically, so the auto-scroll loops:

- the same two Wang tilesets of 16x16 tiles (tiles/wang_street_roof.png,
  tiles/wang_street_canal.png), placed by the corners of a vertex grid:
  streets every 128 px, roofs between them, a canal down the middle;
- the round plaza from the aerial view (backgrounds/aerial.png) on an
  esplanade of one block, helipads and a skylight on most roofs.

The sprites: Kit's glider (characters/glider.png), the hunter
(characters/hunter.png; nose up, the game flips it to dive), and Kit's shot,
drawn here.
"""
from __future__ import annotations

import random
from pathlib import Path

from PIL import Image

from mode7 import disk, glider, rect, wang

W, H = 16, 32                 # the strip, in 16-px cells: 256x512
BLOCK = 8                     # cells per block, street included


def grid() -> list[list[str]]:
    """The terrain at each vertex: l the street, u a roof, c the canal."""
    v = [["l" if x % BLOCK == 0 or y % BLOCK == 0 else "u" for x in range(W + 1)] for y in range(H + 1)]
    for y in range(H + 1):                   # the canal down: water on 7-8, streets either side
        v[y][6] = v[y][9] = "l"
        v[y][7] = v[y][8] = "c"
    for y in range(17, 24):                  # the esplanade: the right block of the third row
        for x in range(10, 16):
            v[y][x] = "l"
    v[H] = list(v[0])                        # wrap: the last vertex row is the first
    return v


def ground(art: Path) -> tuple[Image.Image, int]:
    """scene.png (256x512, indexed on its own colours by brightness) and its distinct 8x8 tiles."""
    roof = wang(Image.open(art / "tiles" / "wang_street_roof.png").convert("RGB"))
    canal = wang(Image.open(art / "tiles" / "wang_street_canal.png").convert("RGB"))
    aerial = Image.open(art / "backgrounds" / "aerial.png").convert("RGB")
    v = grid()
    out = Image.new("RGB", (W * 16, H * 16))
    for y in range(H):
        for x in range(W):
            c = (v[y][x], v[y][x + 1], v[y + 1][x], v[y + 1][x + 1])
            assert not ("u" in c and "c" in c), (x, y)
            upper = "c" if "c" in c else "u"
            key = sum((t == upper) << b for t, b in zip(c, (3, 2, 1, 0)))
            out.paste((canal if upper == "c" else roof)[key], (x * 16, y * 16))

    def stamp(pixels, x0, y0):
        for x, y, p in pixels:
            out.putpixel((x0 + x, y0 + y), p)

    r = 40                                   # the plaza, centred on its esplanade
    stamp(disk(aerial, 256, 256, r), 208 - r, 320 - r)
    pieces = [disk(aerial, 25, 22, 20), disk(aerial, 489, 23, 20), rect(aerial, (200, 90, 232, 118))]
    rng = random.Random(5)
    for by in range(H // BLOCK):
        for bx in range(W // BLOCK):
            k = rng.randrange(4)
            if (bx, by) == (1, 2) or k == 3:
                continue
            cells = [(x, y) for y in range(by * BLOCK, by * BLOCK + BLOCK) for x in range(bx * BLOCK, bx * BLOCK + BLOCK)
                     if v[y][x] == v[y][x + 1] == v[y + 1][x] == v[y + 1][x + 1] == "u"]
            xs = [x for x, _ in cells]
            ys = [y for _, y in cells]
            w = max(px for px, _, _ in pieces[k]) + 1
            h = max(py for _, py, _ in pieces[k]) + 1
            cx = (min(xs) + max(xs) + 1) * 8 - w // 2
            cy = (min(ys) + max(ys) + 1) * 8 - h // 2
            stamp(pieces[k], cx // 16 * 16, cy // 16 * 16)

    colours = sorted(set(out.getdata()), key=sum)
    assert len(colours) <= 16, len(colours)
    lut = {c: i for i, c in enumerate(colours)}
    im = Image.new("P", out.size, 0)
    im.putpalette([v for c in colours for v in c] + [0] * (768 - 3 * len(colours)))
    im.putdata([lut[p] for p in out.getdata()])
    tiles = {out.crop((x, y, x + 8, y + 8)).tobytes() for y in range(0, H * 16, 8) for x in range(0, W * 16, 8)}
    return im, len(tiles)


def hunter(art: Path) -> Image.Image:
    """The security's hunter from above (characters/hunter.png), indexed: 0 transparent."""
    im = Image.open(art / "characters" / "hunter.png").convert("RGBA")
    colours = sorted({p[:3] for p in im.getdata() if p[3] >= 128})
    assert len(colours) <= 15, len(colours)
    out = Image.new("P", im.size, 0)
    out.putpalette([0, 0, 0] + [v for c in colours for v in c] + [0] * (765 - 3 * len(colours)))
    out.putdata([colours.index(p[:3]) + 1 if p[3] >= 128 else 0 for p in im.getdata()])
    return out


# Kit's shot: an aqua bolt pointing up, in the 8x8 the game's collision box
# assumes at the centre of its 16x16. 1 the core, 2 the bolt, 3 its rim.
BOLT = ["................", "................", "................", ".......22.......",
        "......2112......", "......2112......", ".....321123.....", ".....321123.....",
        ".....321123.....", "......3223......", "......3223......", ".......33.......",
        "................", "................", "................", "................"]


def bolt() -> Image.Image:
    im = Image.new("P", (16, 16), 0)
    im.putpalette([0, 0, 0, 232, 248, 255, 112, 224, 224, 42, 128, 144] + [0] * 756)
    im.putdata([0 if ch == "." else int(ch) for row in BOLT for ch in row])
    return im


def build_shmup(art: Path) -> dict[str, Image.Image]:
    scene, tiles = ground(art)
    return {"scene": scene, "player": glider(art), "enemy": hunter(art), "bullet": bolt(), "_tiles": tiles}
