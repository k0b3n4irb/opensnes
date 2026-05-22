#!/usr/bin/env python3
"""Compose scene.png — the 256×256 displayable screen — from ground.png.

ground.png is a 12×7 grid of 16×16 source tiles from Kenney's Pixel Shmup
pack. None of them is a self-tilable "pure grass" except (6,2), which is
the only tile without an autotile border. The rest are pieces of the
"grass-enclosed-by-sand" or "brown-road-on-grass" autotile families and
look wrong when tiled across a full screen.

This script assembles a coherent S1 scene:
  - Pure grass (6,2) fills the whole 16×16 macro-tile grid
  - A handful of trees ((1,0) small, (2,0) larger) scattered off-grid
  - Two red tent assemblies — roof (3,0) + body (4,0) + flag (5,0)
    stacked vertically — as enemy bases
  - Bushes (0,0) in corners + wooden rings (6,3) for visual breaks

Output: scene.png, 256×256, ~12 unique colours (fits one SNES palette
with 4 slots free).

Re-run after editing the tile assignments below; gfx4snes will pick up
the new scene.png on the next `make`.
"""

from pathlib import Path
from subprocess import check_output

from PIL import Image

HERE = Path(__file__).resolve().parent
SRC = HERE / "ground.png"
OUT = HERE / "scene.png"

TILE = 16

def tile(src, r, c):
    return src.crop((c * TILE, r * TILE, (c + 1) * TILE, (r + 1) * TILE))


def build():
    src = Image.open(SRC).convert("RGB")

    G = tile(src, 6, 2)             # pure grass — the only borderless tile
    BUSH = tile(src, 0, 0)
    TREE_S = tile(src, 1, 0)
    TREE_L = tile(src, 2, 0)
    TENT_TOP = tile(src, 3, 0)      # red tent roof
    TENT_BOT = tile(src, 4, 0)      # red tent body
    TENT_FLAG = tile(src, 5, 0)     # red flag
    RING = tile(src, 6, 3)          # wooden ring (single-tile decor)

    W = H = 256
    COLS = ROWS = 16
    scene = Image.new("RGB", (W, H))

    # Pure-grass fill across the whole screen
    for r in range(ROWS):
        for c in range(COLS):
            scene.paste(G, (c * TILE, r * TILE))

    # Trees — sprinkled in a non-grid pattern
    trees = [
        (1, 2, TREE_L), (1, 12, TREE_S),
        (3, 6, TREE_S), (3, 9, TREE_L),
        (4, 14, TREE_S),
        (7, 1, TREE_L), (7, 13, TREE_S),
        (9, 3, TREE_S),
        (10, 8, TREE_S),
        (11, 11, TREE_L),
        (13, 5, TREE_L), (14, 14, TREE_S),
    ]
    for (r, c, t) in trees:
        scene.paste(t, (c * TILE, r * TILE))

    # Two red tent assemblies as enemy bases (3-tile vertical stack)
    def tent(r0, c):
        scene.paste(TENT_TOP, (c * TILE, (r0 + 0) * TILE))
        scene.paste(TENT_BOT, (c * TILE, (r0 + 1) * TILE))
        scene.paste(TENT_FLAG, (c * TILE, (r0 + 2) * TILE))

    tent(4, 5)
    tent(11, 10)

    # Bushes in corners + a couple wooden rings for visual variety
    for (r, c) in [(0, 0), (0, 15), (15, 0), (15, 15), (5, 11), (12, 4)]:
        scene.paste(BUSH, (c * TILE, r * TILE))
    for (r, c) in [(8, 13), (2, 7)]:
        scene.paste(RING, (c * TILE, r * TILE))

    # Quantise to an indexed PNG so gfx4snes consumes it directly
    out = scene.quantize(colors=64, method=Image.Quantize.MEDIANCUT)
    out.save(OUT)


if __name__ == "__main__":
    if not SRC.exists():
        raise SystemExit(f"missing {SRC}")
    build()
    info = check_output(
        ["identify", "-format", "%wx%h  unique=%k\n", str(OUT)]
    ).decode().strip()
    print(f"{OUT.name}: {info}")
