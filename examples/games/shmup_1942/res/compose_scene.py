#!/usr/bin/env python3
"""Compose scene.png — the 256×256 displayable screen — from ground.png.

ground.png is a 12×7 grid of 16×16 source tiles from Kenney's Pixel Shmup
pack. The only borderless self-tilable tile is `(6,2)` — pure grass. The
adjacent `(6,5)` is grass with a small brown "worn patch" decoration,
also self-contained, usable as occasional variation. Everything else in
the pack carries autotile borders (cream-sand frame around grass, or
brown road-on-grass autotile) and produces seams when tiled across a
full screen.

Composition algorithm (deterministic via fixed PRNG seed):

  1. Pure-grass fill on the whole 16×16 macro-tile grid.
  2. A handful of "worn patch" (6,5) tiles sparsely placed for texture
     variety.
  3. Tree CLUSTERS, not single trees. Cluster centres come from
     Poisson-disk-style min-distance rejection sampling so they look
     placed by erosion / seeding, not by a human picking spots. Each
     centre gets 2-5 trees with offsets drawn from a tight gaussian.
  4. Two red tent enemy bases at hand-picked asymmetric positions
     (these are level design, not procedural).
  5. Bushes and wooden rings scattered with the same min-distance
     rejection sampler, but lower density.

Re-run after editing the SEED or the parameters below; gfx4snes picks
up the new scene.png on the next `make`.
"""

from __future__ import annotations

import random
from pathlib import Path
from subprocess import check_output

from PIL import Image

HERE = Path(__file__).resolve().parent
SRC = HERE / "ground.png"
OUT = HERE / "scene.png"

TILE = 16
COLS = ROWS = 256 // TILE  # 16×16 macro grid

# Fixed seed — same composition on every regeneration. Bump if the layout
# needs to change drastically (e.g. new biome).
SEED = 7

# Hand-picked enemy base positions (level design, not procedural). They sit
# off the centre line in different quadrants to avoid mirror symmetry.
TENT_POSITIONS = [(3, 4), (10, 11)]


def tile(src: Image.Image, r: int, c: int) -> Image.Image:
    """Crop a 16×16 source tile from ground.png at (row, col) of its 12×7 grid."""
    return src.crop((c * TILE, r * TILE, (c + 1) * TILE, (r + 1) * TILE))


def reserved_cells() -> set[tuple[int, int]]:
    """Cells occupied by the deterministic tent stacks — keep clear of placement."""
    cells = set()
    for (r0, c) in TENT_POSITIONS:
        for dr in range(3):  # roof + body + flag
            cells.add((r0 + dr, c))
    return cells


def min_distance_sample(
    n_target: int,
    min_dist: float,
    rng: random.Random,
    forbidden: set[tuple[int, int]],
    margin: int = 0,
    max_attempts: int = 400,
) -> list[tuple[int, int]]:
    """Reject-sample up to `n_target` integer cells with pairwise distance ≥ min_dist.

    Falls back to fewer than `n_target` if the placement is too dense; that
    keeps the algorithm bounded.
    """
    accepted: list[tuple[int, int]] = []
    attempts = 0
    while len(accepted) < n_target and attempts < max_attempts:
        attempts += 1
        r = rng.randint(margin, ROWS - 1 - margin)
        c = rng.randint(margin, COLS - 1 - margin)
        if (r, c) in forbidden:
            continue
        ok = True
        for (rr, cc) in accepted:
            if (r - rr) ** 2 + (c - cc) ** 2 < min_dist * min_dist:
                ok = False
                break
        if ok:
            accepted.append((r, c))
    return accepted


def build() -> None:
    rng = random.Random(SEED)
    src = Image.open(SRC).convert("RGB")

    GRASS = tile(src, 6, 2)
    WORN = tile(src, 6, 5)            # grass with small brown patch
    BUSH = tile(src, 0, 0)
    TREE_S = tile(src, 1, 0)
    TREE_L = tile(src, 2, 0)
    TENT_TOP = tile(src, 3, 0)
    TENT_BOT = tile(src, 4, 0)
    TENT_FLAG = tile(src, 5, 0)
    RING = tile(src, 6, 3)

    scene = Image.new("RGB", (256, 256))

    # 1) Pure-grass fill across the whole screen
    for r in range(ROWS):
        for c in range(COLS):
            scene.paste(GRASS, (c * TILE, r * TILE))

    reserved = reserved_cells()
    # Track every placed object so subsequent samplers avoid stacking
    placed: set[tuple[int, int]] = set(reserved)

    # 2) Worn-grass patches — sparse, breaks the regular texture
    worn_cells = min_distance_sample(
        n_target=4,
        min_dist=4.5,
        rng=rng,
        forbidden=placed,
    )
    for (r, c) in worn_cells:
        scene.paste(WORN, (c * TILE, r * TILE))
        placed.add((r, c))

    # 3) Tree clusters — Poisson-disk centres, each centre seeds 2-5 trees
    #    with small gaussian offsets. Min cluster distance is large; trees
    #    within a cluster sit on adjacent or near-adjacent cells.
    cluster_centres = min_distance_sample(
        n_target=5,
        min_dist=5.0,
        rng=rng,
        forbidden=placed,
        margin=1,
    )
    for (cr, cc) in cluster_centres:
        n_trees = rng.randint(2, 5)
        cluster_cells: list[tuple[int, int]] = []
        attempts = 0
        while len(cluster_cells) < n_trees and attempts < 30:
            attempts += 1
            # Tight gaussian offset, clamped to a 3×3 neighbourhood
            dr = max(-2, min(2, round(rng.gauss(0, 0.9))))
            dc = max(-2, min(2, round(rng.gauss(0, 0.9))))
            r, c = cr + dr, cc + dc
            if not (0 <= r < ROWS and 0 <= c < COLS):
                continue
            if (r, c) in placed:
                continue
            cluster_cells.append((r, c))
            placed.add((r, c))
            t = rng.choice([TREE_S, TREE_S, TREE_L])  # small trees more common
            scene.paste(t, (c * TILE, r * TILE))

    # 4) Tent enemy bases — deterministic, asymmetric placement
    for (r0, c) in TENT_POSITIONS:
        scene.paste(TENT_TOP, (c * TILE, r0 * TILE))
        scene.paste(TENT_BOT, (c * TILE, (r0 + 1) * TILE))
        scene.paste(TENT_FLAG, (c * TILE, (r0 + 2) * TILE))

    # 5) Wooden rings — visual breaks, treated as small "landing pads"
    ring_cells = min_distance_sample(
        n_target=3,
        min_dist=5.5,
        rng=rng,
        forbidden=placed,
        margin=1,
    )
    for (r, c) in ring_cells:
        scene.paste(RING, (c * TILE, r * TILE))
        placed.add((r, c))

    # 6) Bushes — final pass, fills in remaining open ground
    bush_cells = min_distance_sample(
        n_target=6,
        min_dist=3.5,
        rng=rng,
        forbidden=placed,
    )
    for (r, c) in bush_cells:
        scene.paste(BUSH, (c * TILE, r * TILE))
        placed.add((r, c))

    # Quantise to indexed PNG for gfx4snes
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
