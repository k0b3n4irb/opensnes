#!/usr/bin/env python3
"""Extract the player and enemy ships from Kenney's `ships_packed.png`.

The pack lays out 24 ships in a 4×6 grid of 32×32 sprites. We pick:
  - #0  (row 0, col 0) — big blue chasseur → `player.png`
  - #9  (row 2, col 1) — medium red chasseur → `enemy.png`

Each sprite is exported as a 16-colour indexed PNG with the dark-blue
Kenney background forced to palette index 0, because gfx4snes treats
that index as transparent for sprites. PIL's adaptive quantizer puts
the background at an arbitrary index, so the script does an explicit
palette swap to put it at slot 0.
"""

from pathlib import Path
from subprocess import check_output

from PIL import Image

HERE = Path(__file__).resolve().parent
SRC = HERE / "ships_packed.png"

TILE = 32


def extract(index: int, out_path: Path) -> None:
    """Crop ship at `index` (row-major in the 4×6 grid) and quantise to
    16 colours with the background at palette index 0."""
    src = Image.open(SRC).convert("RGB")
    row, col = divmod(index, 4)
    ship = src.crop((col * TILE, row * TILE, (col + 1) * TILE, (row + 1) * TILE))

    bg_colour = ship.getpixel((0, 0))

    quant = ship.convert(
        "P",
        palette=Image.Palette.ADAPTIVE,
        colors=16,
        dither=Image.Dither.NONE,
    )
    pal_flat = quant.getpalette()[:48]

    bg_idx = None
    for i in range(16):
        if (pal_flat[i * 3], pal_flat[i * 3 + 1], pal_flat[i * 3 + 2]) == bg_colour:
            bg_idx = i
            break
    if bg_idx is None:
        raise SystemExit(f"could not locate bg colour {bg_colour} in quantised palette")

    pixels = list(quant.getdata())
    swap = {bg_idx: 0, 0: bg_idx}
    new_pixels = [swap.get(p, p) for p in pixels]

    new_palette = list(pal_flat)
    a, b = bg_idx * 3, 0
    new_palette[b:b + 3], new_palette[a:a + 3] = pal_flat[a:a + 3], pal_flat[b:b + 3]
    new_palette += [0] * (768 - len(new_palette))

    out = Image.new("P", quant.size)
    out.putpalette(new_palette)
    out.putdata(new_pixels)
    out.save(out_path)


def info(p: Path) -> str:
    return check_output(
        ["identify", "-format", "%wx%h  unique=%k", str(p)]
    ).decode().strip()


if __name__ == "__main__":
    if not SRC.exists():
        raise SystemExit(f"missing {SRC}")
    for (index, name) in [(0, "player.png"), (9, "enemy.png")]:
        out_path = HERE / name
        extract(index, out_path)
        print(f"{name}: {info(out_path)}")
