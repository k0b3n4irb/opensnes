#!/usr/bin/env python3
"""Compose scene.png — procedurally-generated land-with-water-channels
and organic dirt patches.

Matches the layout of Kenney's *Pixel Shmup* preview render:
GRASS is the base biome, WATER carves into it from off-screen, with a
sand beach at every grass/water boundary. Inside the grass, organic
DIRT patches read as bare-earth clearings; their tents are bombable
targets in S5.

Algorithm
=========

  1. Pure-grass fill everywhere.

  2. **Water CA** (cellular automata, ~30 % fill, 4 iters). Two random
     sides get a forced water margin before smoothing so water always
     connects to the off-screen "ocean". The 15-tile sand-in-water
     autotile (rows 3-5 cols 7-11) resolves the coast.

  3. **Dirt CA** (smaller fill, ~12 %, on grass interior only). The
     bank lacks a curved dirt-in-grass autotile, so we **derive one at
     runtime** by pixel-recolouring the 15-tile water bank:
       - sand main         → dirt main
       - sand transition   → dirt highlight
       - water cyan        → grass green
     The pattern lookup (NESW → tile) is identical to the water pass;
     only the rendered tile images differ.

  4. **Tents** placed on the largest dirt patches (one per cluster).

  5. **Decorations** (Poisson-disk min-distance sampling):
     trees biased toward grass cells near water edges; bushes and
     wooden rings sparse on the open interior.

Bank-completion trick: the 15-tile bank is missing two T-junctions
(0b0111 = water/grass only to N, 0b1110 = only to W). We derive them
at runtime by flipping the symmetric existing tiles (0b1101 vertically
flipped, 0b1011 horizontally flipped). All 16 patterns are then
covered.

SEED = 17, water fill 0.30 was picked from a 10-seed comparison sweep —
it gives water carving in from N+W with grass dominant elsewhere, the
layout closest to the Kenney preview.
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
# 32×64 macro grid → 256×512 px scene = SC_32x64 tilemap (2 screens tall).
# Vertical scroll cycles through the full 512 px before wrapping; that's
# ~8.5 s of unique terrain at 60 fps with 1 px/frame.
COLS = 16
ROWS = 32
SEED = 17


# ============================================================================
# Tile catalog
# ============================================================================

WATER_FILL = (0, 6)
GRASS_FILL = (6, 2)
SAND_INTERIOR = (6, 8)

SAND_BANK = [(r, c) for r in range(3, 6) for c in range(7, 12)]

BUSH      = (0, 0)
TREE_S    = (1, 0)
TREE_L    = (2, 0)
TENT_TOP  = (3, 0)
TENT_BOT  = (4, 0)
TENT_FLAG = (5, 0)
RING      = (6, 3)

# Colour mapping for water→dirt recolour. Derived from histograms of
# both autotiles in ground.png.
WATER_TO_DIRT_COLOR_MAP = {
    (203, 129, 94):  (224, 142, 103),  # sand main → dirt main
    (224, 142, 103): (244, 172, 102),  # sand transition → dirt highlight
    (244, 172, 102): (244, 172, 102),  # shared light tone — stays
    (180, 114, 83):  (224, 142, 103),  # dark sand variant → dirt main
    (223, 246, 245): (182, 213, 60),   # water cyan → grass green
}


# ============================================================================
# Tile helpers
# ============================================================================

def tile(src: Image.Image, r: int, c: int) -> Image.Image:
    return src.crop((c * TILE, r * TILE, (c + 1) * TILE, (r + 1) * TILE))


def is_landish(rgb: tuple[int, int, int]) -> bool:
    r, g, b = rgb
    if b >= r and b >= g - 5 and b > 200 and (b - r) >= 15:
        return False
    return True


def recolour(t: Image.Image, mapping: dict) -> Image.Image:
    """Per-pixel colour swap using a fixed RGB→RGB mapping. Unknown
    colours pass through unchanged (none expected in the bank tiles)."""
    out = t.copy()
    px = out.load()
    for y in range(t.height):
        for x in range(t.width):
            c = px[x, y]
            if c in mapping:
                px[x, y] = mapping[c]
    return out


def decode_sand_bank(src: Image.Image) -> dict[int, list[Image.Image]]:
    """Catalog the 15-tile water bank by NESW pattern (bit set ⇒ that
    edge is LAND). Derive missing T-junctions by mirroring."""
    cat: dict[int, list[Image.Image]] = {}
    for (r, c) in SAND_BANK:
        x0, y0 = c * TILE, r * TILE
        edges = (
            src.getpixel((x0 + TILE // 2, y0 + 1)),
            src.getpixel((x0 + TILE - 2, y0 + TILE // 2)),
            src.getpixel((x0 + TILE // 2, y0 + TILE - 2)),
            src.getpixel((x0 + 1,         y0 + TILE // 2)),
        )
        bits = sum(
            (1 if is_landish(p) else 0) << (3 - i)
            for i, p in enumerate(edges)
        )
        cat.setdefault(bits, []).append(tile(src, r, c))

    if 0b0111 not in cat and 0b1101 in cat:
        cat[0b0111] = [t.transpose(Image.FLIP_TOP_BOTTOM) for t in cat[0b1101]]
    if 0b1110 not in cat and 0b1011 in cat:
        cat[0b1110] = [t.transpose(Image.FLIP_LEFT_RIGHT) for t in cat[0b1011]]
    return cat


def build_dirt_catalog(sand_catalog: dict) -> dict:
    """Re-colour every tile in the sand catalog to produce the
    dirt-in-grass equivalent. Same pattern keys."""
    return {
        pat: [recolour(t, WATER_TO_DIRT_COLOR_MAP) for t in tiles]
        for pat, tiles in sand_catalog.items()
    }


# ============================================================================
# Cellular automata
# ============================================================================

def cellular_automata(rng, fill_pct, iters, alive_threshold=4, birth_threshold=5,
                     seed_edges=None):
    """Generic CA pass returning a 0/1 grid.

    seed_edges: optional list of "N"/"S"/"E"/"W" forcing those edges to be
    alive before smoothing (used by the water pass to connect to off-screen
    ocean).
    """
    grid = [
        [1 if rng.random() < fill_pct else 0 for _ in range(COLS)]
        for _ in range(ROWS)
    ]
    if seed_edges:
        for side in seed_edges:
            if side == "N":
                for c in range(COLS):
                    grid[0][c] = 1
                    if rng.random() < 0.7:
                        grid[1][c] = 1
            elif side == "S":
                for c in range(COLS):
                    grid[ROWS - 1][c] = 1
                    if rng.random() < 0.7:
                        grid[ROWS - 2][c] = 1
            elif side == "W":
                for r in range(ROWS):
                    grid[r][0] = 1
                    if rng.random() < 0.7:
                        grid[r][1] = 1
            else:  # E
                for r in range(ROWS):
                    grid[r][COLS - 1] = 1
                    if rng.random() < 0.7:
                        grid[r][COLS - 2] = 1

    for _ in range(iters):
        new = [[0] * COLS for _ in range(ROWS)]
        for r in range(ROWS):
            for c in range(COLS):
                count = 0
                for dr in (-1, 0, 1):
                    for dc in (-1, 0, 1):
                        if dr == 0 and dc == 0:
                            continue
                        nr, nc = r + dr, c + dc
                        if 0 <= nr < ROWS and 0 <= nc < COLS:
                            count += grid[nr][nc]
                if grid[r][c] == 1:
                    new[r][c] = 1 if count >= alive_threshold else 0
                else:
                    new[r][c] = 1 if count >= birth_threshold else 0
        grid = new

    # Strip single-cell speckles
    for r in range(ROWS):
        for c in range(COLS):
            if grid[r][c] != 1:
                continue
            n = sum(
                grid[r + dr][c + dc]
                for dr in (-1, 0, 1)
                for dc in (-1, 0, 1)
                if (dr != 0 or dc != 0) and 0 <= r + dr < ROWS and 0 <= c + dc < COLS
            )
            if n == 0:
                grid[r][c] = 0
    return grid


def generate_water_mask(rng, fill_pct=0.32, iters=4):
    """Water mask. Two random sides get a forced edge seed pre-CA, then
    after smoothing we RE-APPLY the edge seed so the screen always shows
    visible water on at least one side (CA tends to erode small forced
    margins to nothing otherwise)."""
    sides = rng.sample(["N", "S", "E", "W"], 2)
    grid = cellular_automata(rng, fill_pct, iters, seed_edges=sides)
    # Re-seed: make sure the chosen sides keep at least their outermost
    # row/col as water (so visible water actually reaches the screen edge)
    for side in sides:
        if side == "N":
            for c in range(COLS):
                if rng.random() < 0.85:
                    grid[0][c] = 1
        elif side == "S":
            for c in range(COLS):
                if rng.random() < 0.85:
                    grid[ROWS - 1][c] = 1
        elif side == "W":
            for r in range(ROWS):
                if rng.random() < 0.85:
                    grid[r][0] = 1
        else:  # E
            for r in range(ROWS):
                if rng.random() < 0.85:
                    grid[r][COLS - 1] = 1
    return grid


def generate_dirt_mask(rng, grass_interior_set, fill_pct=0.28, iters=3):
    """CA mask for dirt patches. We generate everywhere, then mask to
    grass-interior cells (which excludes water + coast). The 'alive' rule
    is more permissive so patches stay small but connected."""
    raw = cellular_automata(rng, fill_pct, iters,
                            alive_threshold=3, birth_threshold=4)
    out = [[0] * COLS for _ in range(ROWS)]
    for r in range(ROWS):
        for c in range(COLS):
            if raw[r][c] == 1 and (r, c) in grass_interior_set:
                out[r][c] = 1
    # Speckle removal again on the masked result
    for r in range(ROWS):
        for c in range(COLS):
            if out[r][c] != 1:
                continue
            n = sum(
                out[r + dr][c + dc]
                for dr in (-1, 0, 1)
                for dc in (-1, 0, 1)
                if (dr != 0 or dc != 0) and 0 <= r + dr < ROWS and 0 <= c + dc < COLS
            )
            if n == 0:
                out[r][c] = 0
    return out


# ============================================================================
# Geometry / sampling helpers
# ============================================================================

def poisson_disk(rng, n_target, min_dist, forbidden, allowed=None, max_attempts=600):
    out: list[tuple[int, int]] = []
    tries = 0
    while len(out) < n_target and tries < max_attempts:
        tries += 1
        r = rng.randint(0, ROWS - 1)
        c = rng.randint(0, COLS - 1)
        if (r, c) in forbidden:
            continue
        if allowed is not None and (r, c) not in allowed:
            continue
        if any((r - rr) ** 2 + (c - cc) ** 2 < min_dist * min_dist for (rr, cc) in out):
            continue
        out.append((r, c))
    return out


def distance_to_mask(mask):
    """BFS distance from every cell to the nearest mask=1 cell."""
    INF = 9999
    dist = [[INF] * COLS for _ in range(ROWS)]
    frontier = []
    for r in range(ROWS):
        for c in range(COLS):
            if mask[r][c] == 1:
                dist[r][c] = 0
                frontier.append((r, c))
    head = 0
    while head < len(frontier):
        r, c = frontier[head]
        head += 1
        for dr, dc in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nr, nc = r + dr, c + dc
            if 0 <= nr < ROWS and 0 <= nc < COLS and dist[nr][nc] > dist[r][c] + 1:
                dist[nr][nc] = dist[r][c] + 1
                frontier.append((nr, nc))
    return dist


def connected_components(mask):
    seen = [[False] * COLS for _ in range(ROWS)]
    out = []
    for r in range(ROWS):
        for c in range(COLS):
            if mask[r][c] == 0 or seen[r][c]:
                continue
            stack = [(r, c)]
            comp = set()
            while stack:
                rr, cc = stack.pop()
                if seen[rr][cc] or mask[rr][cc] == 0:
                    continue
                seen[rr][cc] = True
                comp.add((rr, cc))
                for dr, dc in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    nr, nc = rr + dr, cc + dc
                    if 0 <= nr < ROWS and 0 <= nc < COLS and not seen[nr][nc]:
                        stack.append((nr, nc))
            out.append(comp)
    return out


# ============================================================================
# Composition
# ============================================================================

def resolve_pattern(r, c, mask, catalog, src_fallback_tile):
    """Look up a tile from the catalog based on cardinal neighbour states
    in `mask`. Off-screen neighbours count as BASE biome (not water/dirt)
    to keep the screen border clean — no artificial sand band along
    edges that don't actually touch water."""
    def biome_at(rr, cc):
        if 0 <= rr < ROWS and 0 <= cc < COLS:
            return mask[rr][cc] == 1
        return False  # off-screen = base biome (no artificial frame)
    n_other = biome_at(r - 1, c)
    s_other = biome_at(r + 1, c)
    e_other = biome_at(r, c + 1)
    w_other = biome_at(r, c - 1)
    # pat bit = 1 ⇒ edge is BASE biome (not other)
    pat = (
        (0 if n_other else 1) << 3 |
        (0 if e_other else 1) << 2 |
        (0 if s_other else 1) << 1 |
        (0 if w_other else 1)
    )
    candidates = catalog.get(pat)
    return candidates


def build() -> None:
    rng = random.Random(SEED)
    src = Image.open(SRC).convert("RGB")
    sand_catalog = decode_sand_bank(src)
    dirt_catalog = build_dirt_catalog(sand_catalog)

    scene = Image.new("RGB", (COLS * TILE, ROWS * TILE))

    # Layer 0: pure grass everywhere
    grass_tile = tile(src, *GRASS_FILL)
    for r in range(ROWS):
        for c in range(COLS):
            scene.paste(grass_tile, (c * TILE, r * TILE))

    # Layer 1: water mask + sand beach autotile
    water_mask = generate_water_mask(rng)
    water_tile = tile(src, *WATER_FILL)
    sand_interior_tile = tile(src, *SAND_INTERIOR)

    coast_cells: set[tuple[int, int]] = set()
    for r in range(ROWS):
        for c in range(COLS):
            if water_mask[r][c] == 1:
                scene.paste(water_tile, (c * TILE, r * TILE))
                continue
            # Land cell: is any cardinal neighbour water (in-bounds only)?
            def w_at(rr, cc):
                if 0 <= rr < ROWS and 0 <= cc < COLS:
                    return water_mask[rr][cc] == 1
                return False  # off-screen never counted as water here
            if not any(w_at(r + dr, c + dc) for (dr, dc) in ((-1, 0), (1, 0), (0, -1), (0, 1))):
                continue
            coast_cells.add((r, c))
            candidates = resolve_pattern(r, c, water_mask, sand_catalog, sand_interior_tile)
            if not candidates:
                scene.paste(sand_interior_tile, (c * TILE, r * TILE))
                continue
            scene.paste(rng.choice(candidates), (c * TILE, r * TILE))

    # Grass interior = all land cells not on the coast
    grass_interior: set[tuple[int, int]] = {
        (r, c) for r in range(ROWS) for c in range(COLS)
        if water_mask[r][c] == 0 and (r, c) not in coast_cells
    }

    dist_water = distance_to_mask(water_mask)

    # Layer 2: dirt mask + recoloured-autotile rendering. Only on grass
    # interior cells whose distance to water is ≥ 2 (avoid sand/dirt
    # touching, which has no defined transition tile).
    dirt_eligible = {(r, c) for (r, c) in grass_interior if dist_water[r][c] >= 2}
    dirt_mask = generate_dirt_mask(rng, dirt_eligible)
    # Drop dirt cells that ended up next to a water cell (CA can sneak
    # past the eligible mask via single-cell neighbours)
    for r in range(ROWS):
        for c in range(COLS):
            if dirt_mask[r][c] != 1:
                continue
            for (dr, dc) in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                nr, nc = r + dr, c + dc
                if 0 <= nr < ROWS and 0 <= nc < COLS and water_mask[nr][nc] == 1:
                    dirt_mask[r][c] = 0
                    break

    dirt_cells: set[tuple[int, int]] = set()
    for r in range(ROWS):
        for c in range(COLS):
            if dirt_mask[r][c] != 1:
                continue
            dirt_cells.add((r, c))
            candidates = resolve_pattern(r, c, dirt_mask, dirt_catalog, None)
            if not candidates:
                # Pure-grass-around fallback: use a recoloured 0b0000
                # (no land edges = all-grass border around dirt) if it
                # exists, else just leave the grass tile in place.
                fallback = dirt_catalog.get(0b0000) or dirt_catalog.get(0b1111)
                if fallback:
                    scene.paste(rng.choice(fallback), (c * TILE, r * TILE))
                continue
            scene.paste(rng.choice(candidates), (c * TILE, r * TILE))

    # Layer 3: tents on the largest dirt clusters. Place a tent at the
    # interior of each connected component ≥ 4 cells, where the centre
    # column has 3 vertical dirt cells in a row.
    placed_decor: set[tuple[int, int]] = set()
    dirt_components = connected_components(dirt_mask)
    dirt_components.sort(key=len, reverse=True)
    # On a 32-row scene we want more tents (each comes into view as the
    # player scrolls). Cap at 5 and only require ≥3 cells per cluster.
    for comp in dirt_components[:5]:
        if len(comp) < 3:
            continue
        # Find a column with 3 vertical dirt cells
        cols_with_3 = []
        for c in {cc for (_, cc) in comp}:
            col_rows = sorted([r for (r, cc) in comp if cc == c])
            for i in range(len(col_rows) - 2):
                if col_rows[i + 1] == col_rows[i] + 1 and col_rows[i + 2] == col_rows[i] + 2:
                    cols_with_3.append((c, col_rows[i]))
                    break
        if not cols_with_3:
            continue
        (tc, tr) = rng.choice(cols_with_3)
        scene.paste(tile(src, *TENT_TOP),  (tc * TILE, (tr + 0) * TILE))
        scene.paste(tile(src, *TENT_BOT),  (tc * TILE, (tr + 1) * TILE))
        scene.paste(tile(src, *TENT_FLAG), (tc * TILE, (tr + 2) * TILE))
        placed_decor |= {(tr, tc), (tr + 1, tc), (tr + 2, tc)}

    # Layer 4: tree clusters biased toward coast-adjacent grass
    near_coast = {
        (r, c) for (r, c) in grass_interior
        if dist_water[r][c] in (1, 2, 3)
        and (r, c) not in placed_decor and (r, c) not in dirt_cells
    }
    cluster_centres = poisson_disk(rng, 7, 2.5, placed_decor | dirt_cells, near_coast)
    for (cr, cc) in cluster_centres:
        for _ in range(rng.randint(2, 4)):
            for _ in range(15):
                dr = max(-2, min(2, round(rng.gauss(0, 0.9))))
                dc = max(-2, min(2, round(rng.gauss(0, 0.9))))
                r, c = cr + dr, cc + dc
                if (r, c) not in grass_interior:
                    continue
                if (r, c) in placed_decor or (r, c) in dirt_cells:
                    continue
                t = rng.choice([TREE_S, TREE_S, TREE_L])
                scene.paste(tile(src, *t), (c * TILE, r * TILE))
                placed_decor.add((r, c))
                break

    # Layer 5: bushes scattered
    allowed_misc = grass_interior - placed_decor - dirt_cells
    for (r, c) in poisson_disk(rng, 5, 2.0, placed_decor | dirt_cells, allowed_misc):
        scene.paste(tile(src, *BUSH), (c * TILE, r * TILE))
        placed_decor.add((r, c))

    # Layer 6: 1-2 wooden rings
    for (r, c) in poisson_disk(rng, 2, 5.0, placed_decor | dirt_cells, allowed_misc):
        scene.paste(tile(src, *RING), (c * TILE, r * TILE))
        placed_decor.add((r, c))

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
