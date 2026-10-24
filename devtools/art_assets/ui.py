"""The small sprites drawn by PixelLab (assets/neo-lumen/ui/), made ready for a 4bpp OBJ palette.

Imported by build.py and shmup.py. Each picture is indexed on its own
colours: 0 transparent, then its colours by brightness. Two adjustments,
both mechanical: a cursor is moved so that its tip is the top-left pixel
(the mouse example draws the sprite at the pointer), and a sight can get a
ring of ink round every lit pixel so that it reads over bright lines.
"""
from __future__ import annotations

from pathlib import Path

from PIL import Image

INK = (8, 8, 16)


def ui_sprite(art: Path, name: str, shift: tuple[int, int] = (0, 0), outline: bool = False) -> Image.Image:
    src = Image.open(art / "ui" / name).convert("RGBA")
    w, h = src.size
    lit = {}
    for y in range(h):
        for x in range(w):
            r, g, b, a = src.getpixel((x, y))
            if a >= 128 and 0 <= x + shift[0] < w and 0 <= y + shift[1] < h:
                lit[(x + shift[0], y + shift[1])] = (r, g, b)
    if outline:
        ring = {(x + dx, y + dy) for x, y in lit for dx in (-1, 0, 1) for dy in (-1, 0, 1)}
        for p in ring - set(lit):
            if 0 <= p[0] < w and 0 <= p[1] < h:
                lit[p] = INK
    colours = sorted(set(lit.values()), key=sum)
    assert len(colours) <= 15, (name, len(colours))
    out = Image.new("P", (w, h), 0)
    out.putpalette([0, 0, 0] + [v for c in colours for v in c] + [0] * (765 - 3 * len(colours)))
    for (x, y), c in lit.items():
        out.putpixel((x, y), colours.index(c) + 1)
    return out
