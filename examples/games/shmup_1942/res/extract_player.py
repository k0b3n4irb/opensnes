#!/usr/bin/env python3
"""Extract the player ship from Kenney's `ships_packed.png` 4×6 grid of
32×32 ships and re-export it as `player.png` for gfx4snes.

The pack lays the ships out in 4 columns × 6 rows (24 total). `ship_0000`
sits at the top-left corner — a big blue chasseur with a clear central
silhouette, good readability at SNES sprite size. We pick that as the
player ship for the shmup.

gfx4snes interprets the first colour of the indexed-PNG palette as
transparent for sprites. The Kenney background is a dark blue
(`#1d2b53`-ish) that occupies the whole sprite's outer frame, so it
maps to palette index 0 naturally after quantisation. The visible ship
silhouette uses the remaining colours.
"""

from pathlib import Path
from subprocess import check_output

from PIL import Image

HERE = Path(__file__).resolve().parent
SRC = HERE / "ships_packed.png"
OUT = HERE / "player.png"

TILE = 32

def build():
    src = Image.open(SRC).convert("RGB")
    # ship_0000 = top-left ship in the 4×6 grid
    ship = src.crop((0, 0, TILE, TILE))

    # Force the background colour (top-left pixel — the dark-blue frame
    # surrounding every ship in the Kenney pack) to land at palette
    # index 0, which is how gfx4snes flags transparency for sprites.
    bg_colour = ship.getpixel((0, 0))

    # Step 1: convert to an adaptive 16-colour palette
    quant = ship.convert(
        "P",
        palette=Image.Palette.ADAPTIVE,
        colors=16,
        dither=Image.Dither.NONE,
    )
    pal_flat = quant.getpalette()[:48]
    # Step 2: find where the bg colour landed
    bg_idx = None
    for i in range(16):
        if (pal_flat[i * 3], pal_flat[i * 3 + 1], pal_flat[i * 3 + 2]) == bg_colour:
            bg_idx = i
            break
    if bg_idx is None:
        raise SystemExit(f"could not locate bg colour {bg_colour} in quantised palette")
    # Step 3: rebuild palette with bg first, swap pixel indices accordingly
    pixels = list(quant.getdata())
    # Mapping: old_idx → new_idx. Swap bg_idx ↔ 0; everything else shifts
    # by one slot in the affected range.
    swap = {bg_idx: 0, 0: bg_idx}
    new_pixels = [swap.get(p, p) for p in pixels]
    new_palette = list(pal_flat)
    # Swap palette RGB triples
    a, b = bg_idx * 3, 0
    new_palette[b:b+3], new_palette[a:a+3] = pal_flat[a:a+3], pal_flat[b:b+3]
    # Pad palette to 768 entries (PIL requirement for full P mode)
    new_palette += [0] * (768 - len(new_palette))

    out = Image.new("P", quant.size)
    out.putpalette(new_palette)
    out.putdata(new_pixels)
    out.save(OUT)


if __name__ == "__main__":
    if not SRC.exists():
        raise SystemExit(f"missing {SRC}")
    build()
    info = check_output(
        ["identify", "-format", "%wx%h  unique=%k\n", str(OUT)]
    ).decode().strip()
    print(f"{OUT.name}: {info}")
