#!/usr/bin/env python3
"""Compose scene.png — a procedurally-generated archipelago for shmup_1942.

The Kenney *Pixel Shmup* tile pack ships two autotiles for the water/land
transition: a simple 3×3 rectangular one (rows 0-2 cols 7-9) and a richer
15-tile bank in rows 3-5 cols 7-11 that contains the 4 convex corners,
4 cardinal edges, the centre, plus 6 connector/concave pieces. The
richer bank can render organic, non-rectangular coastlines — what the
official Kenney preview render shows.

Algorithm
=========

  1. **Cellular automata** generates a binary land/water mask on the
     16×16 macro grid. We start with ~45% random fill, force a 1-cell
     water border, then iterate the classic cave-generation rule
     (a cell stays/becomes land if ≥5 of its 8 neighbours are land for
     existing land, ≥6 to flip to land). After 4 iterations small
     pockets of either biome get smoothed out.

  2. **Catalog decode**: each of the 15 tiles in the sand-in-water bank
     gets its 4 mid-edge pixels sampled and classified as land or
     water. The resulting 4-bit pattern (N, E, S, W) becomes the
     lookup key. Where multiple tiles share a pattern we keep all of
     them and pick one at random per cell — that gives natural-looking
     edge variety without explicit decoration sprinkling.

  3. **Resolver pass**: for every LAND cell in the mask, the four
     cardinal neighbours are inspected. Each neighbour that is also
     LAND sets the corresponding bit; the resulting pattern is looked
     up in the catalog to pick the right rendering tile.

  4. **Grass layer**: each connected land component that's at least
     4×4 internal cells gets a smaller rectangular grass area dropped
     inside it (1-tile sand beach margin) using the existing 3×3
     grass-in-sand autotile.

  5. **Dirt + tents**: each grass area large enough hosts a 3×3-to-5×4
     dirt clearing with a tent stack in its centre column.

  6. **Decorations**: trees clustered on the grass interior near the
     beach (Poisson-disk min-distance sampling), bushes/rings sparse.

PRNG seed is fixed (SEED) so the output is reproducible. Bump SEED to
redraw.
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
COLS = ROWS = 256 // TILE
SEED = 17


# ============================================================================
# Tile catalog
# ============================================================================

WATER_FILL = (0, 6)
GRASS_FILL = (6, 2)
SAND_INTERIOR = (6, 8)   # pure orange/sand fill, for land cells with no water neighbour

# 15-tile bank, sand-in-water (rows 3-5, cols 7-11) — used as a wang-style
# autotile resolved from neighbour patterns at runtime.
SAND_BANK = [(r, c) for r in range(3, 6) for c in range(7, 12)]

# 3×3 grass-in-sand autotile (rows 0-2, cols 1-3)
GRASS_IN_SAND = {
    "TL": (0, 1), "T": (0, 2), "TR": (0, 3),
    "L":  (1, 1), "C": (1, 2), "R":  (1, 3),
    "BL": (2, 1), "B": (2, 2), "BR": (2, 3),
}

# 3×3 dirt-on-grass autotile (rows 3-5, cols 1-3)
DIRT_IN_GRASS = {
    "TL": (3, 1), "T": (3, 2), "TR": (3, 3),
    "L":  (4, 1), "C": (4, 2), "R":  (4, 3),
    "BL": (5, 1), "B": (5, 2), "BR": (5, 3),
}

# Decoration sprites (placed individually, not by autotile)
BUSH      = (0, 0)
TREE_S    = (1, 0)
TREE_L    = (2, 0)
TENT_TOP  = (3, 0)
TENT_BOT  = (4, 0)
TENT_FLAG = (5, 0)
RING      = (6, 3)


# ============================================================================
# Helpers
# ============================================================================

def tile(src: Image.Image, r: int, c: int) -> Image.Image:
    return src.crop((c * TILE, r * TILE, (c + 1) * TILE, (r + 1) * TILE))


def is_landish(rgb: tuple[int, int, int]) -> bool:
    """Classify a pixel sample as land (orange/sand/brown/grass) or water
    (cyan). Water in the Kenney pack is RGB(223,246,245)-ish — very bright
    with b ≥ g > r; everything else (sand 240/220/180, desert 220/130/80,
    grass 130/200/80) has r as the largest or near-largest channel."""
    r, g, b = rgb
    # Water signature: b is the largest channel AND the tile is bright.
    if b >= r and b >= g - 5 and b > 200 and (b - r) >= 15:
        return False
    return True


def decode_sand_bank(src: Image.Image) -> dict[int, list[tuple[int, int]]]:
    """Build a NESW → list-of-tiles catalog by sampling each bank tile's
    four mid-edge pixels. Returns a dict keyed by 4-bit ints
    where bit3=N, bit2=E, bit1=S, bit0=W; bit set ⇔ that edge is land."""
    cat: dict[int, list[tuple[int, int]]] = {}
    for (r, c) in SAND_BANK:
        x0, y0 = c * TILE, r * TILE
        n = src.getpixel((x0 + TILE // 2, y0 + 1))
        e = src.getpixel((x0 + TILE - 2, y0 + TILE // 2))
        s = src.getpixel((x0 + TILE // 2, y0 + TILE - 2))
        w = src.getpixel((x0 + 1,         y0 + TILE // 2))
        pat = (
            (1 if is_landish(n) else 0) << 3 |
            (1 if is_landish(e) else 0) << 2 |
            (1 if is_landish(s) else 0) << 1 |
            (1 if is_landish(w) else 0)
        )
        cat.setdefault(pat, []).append((r, c))
    return cat


def autotile_key(dr: int, dc: int, h: int, w: int) -> str:
    row = "T" if dr == 0 else ("B" if dr == h - 1 else "")
    col = "L" if dc == 0 else ("R" if dc == w - 1 else "")
    if not row and not col:
        return "C"
    return row + col if (row and col) else (row or col)


def render_rect(scene, src, autotile, r0, c0, w, h):
    for dr in range(h):
        for dc in range(w):
            key = autotile_key(dr, dc, h, w)
            scene.paste(tile(src, *autotile[key]),
                        ((c0 + dc) * TILE, (r0 + dr) * TILE))


def rect_cells(r0, c0, w, h):
    return {(r0 + dr, c0 + dc) for dr in range(h) for dc in range(w)}


def rect_interior(r0, c0, w, h):
    if w < 3 or h < 3:
        return set()
    return {(r0 + dr, c0 + dc) for dr in range(1, h - 1) for dc in range(1, w - 1)}


# ============================================================================
# Cellular automata noise
# ============================================================================

def generate_land_mask(rng, fill_pct=0.52, iters=4):
    """16×16 land/water mask via cellular automata. Outer 1-cell border is
    forced water. Returns 2D list of ints (1=land, 0=water)."""
    grid = [
        [1 if rng.random() < fill_pct else 0 for _ in range(COLS)]
        for _ in range(ROWS)
    ]
    # Force water border so islands never touch the screen edge
    for c in range(COLS):
        grid[0][c] = 0
        grid[ROWS - 1][c] = 0
    for r in range(ROWS):
        grid[r][0] = 0
        grid[r][COLS - 1] = 0

    for _ in range(iters):
        new = [[0] * COLS for _ in range(ROWS)]
        for r in range(ROWS):
            for c in range(COLS):
                if r == 0 or r == ROWS - 1 or c == 0 or c == COLS - 1:
                    new[r][c] = 0
                    continue
                count = 0
                for dr in (-1, 0, 1):
                    for dc in (-1, 0, 1):
                        if dr == 0 and dc == 0:
                            continue
                        nr, nc = r + dr, c + dc
                        if 0 <= nr < ROWS and 0 <= nc < COLS:
                            count += grid[nr][nc]
                # Cave-gen rule
                if grid[r][c] == 1:
                    new[r][c] = 1 if count >= 4 else 0
                else:
                    new[r][c] = 1 if count >= 5 else 0
        grid = new
    # Strip tiny 1-cell speckles (a single isolated land cell looks wrong
    # rendered without a proper "isolated" tile in the bank)
    for r in range(1, ROWS - 1):
        for c in range(1, COLS - 1):
            if grid[r][c] == 1:
                neigh = grid[r - 1][c] + grid[r + 1][c] + grid[r][c - 1] + grid[r][c + 1]
                if neigh == 0:
                    grid[r][c] = 0
    return grid


def connected_components(mask):
    """Flood-fill on the land cells; returns list of cell-sets."""
    seen = [[False] * COLS for _ in range(ROWS)]
    out: list[set[tuple[int, int]]] = []
    for r in range(ROWS):
        for c in range(COLS):
            if mask[r][c] == 0 or seen[r][c]:
                continue
            stack = [(r, c)]
            comp: set[tuple[int, int]] = set()
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

def poisson_disk(rng, n_target, min_dist, forbidden, allowed=None, margin=0, max_attempts=600):
    out: list[tuple[int, int]] = []
    tries = 0
    while len(out) < n_target and tries < max_attempts:
        tries += 1
        r = rng.randint(margin, ROWS - 1 - margin)
        c = rng.randint(margin, COLS - 1 - margin)
        if (r, c) in forbidden:
            continue
        if allowed is not None and (r, c) not in allowed:
            continue
        if any((r - rr) ** 2 + (c - cc) ** 2 < min_dist * min_dist for (rr, cc) in out):
            continue
        out.append((r, c))
    return out


def build() -> None:
    rng = random.Random(SEED)
    src = Image.open(SRC).convert("RGB")
    sand_catalog = decode_sand_bank(src)

    scene = Image.new("RGB", (256, 256))

    # Layer 0: water everywhere
    water_tile = tile(src, *WATER_FILL)
    for r in range(ROWS):
        for c in range(COLS):
            scene.paste(water_tile, (c * TILE, r * TILE))

    # Layer 1: cellular-automata land mask → autotile-resolved sand rendering
    mask = generate_land_mask(rng)

    # The 15-tile bank covers patterns 0b0000..0b1101 except 0b0111 and
    # 0b1110 — there's no "all-land interior" tile (0b1111) and the two
    # T-junctions whose missing arm faces W or E are absent. We handle:
    #   - 0b1111 (all 4 neighbours land) → pure sand fill (6,8)
    #   - 0b0111 (W only is water)        → fall back to 0b0101 (E+W strip
    #                                       extended; visually the closest)
    #   - 0b1110 (E only is water)        → fall back to 0b0101
    interior_tile = SAND_INTERIOR
    pattern_fallbacks = {
        0b0111: 0b0101,
        0b1110: 0b0101,
    }

    land_cells: set[tuple[int, int]] = set()
    for r in range(ROWS):
        for c in range(COLS):
            if mask[r][c] != 1:
                continue
            land_cells.add((r, c))
            n = mask[r - 1][c] if r > 0          else 0
            s = mask[r + 1][c] if r < ROWS - 1   else 0
            w = mask[r][c - 1] if c > 0          else 0
            e = mask[r][c + 1] if c < COLS - 1   else 0
            pat = (n << 3) | (e << 2) | (s << 1) | w
            if pat == 0b1111:
                t = interior_tile
            else:
                lookup_pat = pattern_fallbacks.get(pat, pat)
                candidates = sand_catalog.get(lookup_pat)
                if not candidates:
                    t = interior_tile
                else:
                    t = rng.choice(candidates)
            scene.paste(tile(src, *t), (c * TILE, r * TILE))

    # Layer 2: grass rectangles inside large connected land components
    grass_specs: list[tuple[int, int, int, int]] = []
    grass_cells: set[tuple[int, int]] = set()
    for comp in connected_components(mask):
        if len(comp) < 12:
            continue
        rs = [r for (r, _) in comp]
        cs = [c for (_, c) in comp]
        r_min, r_max = min(rs), max(rs)
        c_min, c_max = min(cs), max(cs)
        bw = c_max - c_min + 1
        bh = r_max - r_min + 1
        if bw < 5 or bh < 5:
            continue
        # Grass rectangle inside the land component. We only require the
        # rectangle itself to be on land — the cream-sand border of the
        # grass-in-sand autotile handles the visual transition. We try
        # progressively smaller sizes if a big one doesn't fit, so an
        # irregular component still gets some grass.
        for gw_try in range(min(bw, 6), 2, -1):
            for gh_try in range(min(bh, 5), 2, -1):
                placed = False
                for _ in range(30):
                    gr0 = rng.randint(r_min, r_max - gh_try + 1)
                    gc0 = rng.randint(c_min, c_max - gw_try + 1)
                    cells_needed = rect_cells(gr0, gc0, gw_try, gh_try)
                    if cells_needed <= comp:
                        grass_specs.append((gr0, gc0, gw_try, gh_try))
                        grass_cells |= cells_needed
                        render_rect(scene, src, GRASS_IN_SAND, gr0, gc0, gw_try, gh_try)
                        placed = True
                        break
                if placed:
                    break
            else:
                continue
            break

    # Layer 3: dirt clearings inside grass, with a tent each
    dirt_cells: set[tuple[int, int]] = set()
    forbidden = set()
    for (gr0, gc0, gw, gh) in grass_specs:
        # Dirt clearing must fit inside grass with at least 1-cell grass
        # border on all sides → needs grass ≥ 5×5. Smaller grass areas
        # just stay pure grass.
        if gw < 5 or gh < 5:
            continue
        # Maximum dirt size: gw-2, gh-2 (leaves a 1-cell grass border).
        max_dw, max_dh = gw - 2, gh - 2
        for _ in range(15):
            dw = rng.randint(3, min(5, max_dw))
            dh = rng.randint(3, min(4, max_dh))
            dr0 = rng.randint(gr0 + 1, gr0 + gh - dh - 1)
            dc0 = rng.randint(gc0 + 1, gc0 + gw - dw - 1)
            new_cells = rect_cells(dr0, dc0, dw, dh)
            if new_cells & dirt_cells:
                continue
            dirt_cells |= new_cells
            render_rect(scene, src, DIRT_IN_GRASS, dr0, dc0, dw, dh)
            # Tent in centre column. Needs 3 vertical cells.
            tc = dc0 + dw // 2
            tr = dr0
            if dh >= 3:
                scene.paste(tile(src, *TENT_TOP),  (tc * TILE, (tr + 0) * TILE))
                scene.paste(tile(src, *TENT_BOT),  (tc * TILE, (tr + 1) * TILE))
                scene.paste(tile(src, *TENT_FLAG), (tc * TILE, (tr + 2) * TILE))
                forbidden |= {(tr, tc), (tr + 1, tc), (tr + 2, tc)}
            break

    # Layer 4: decorations on grass-only cells (grass minus dirt minus tents)
    grass_only = grass_cells - dirt_cells - forbidden
    # Trees: 5 clusters, each 2-4 trees
    tree_centres = poisson_disk(rng, 5, 3.5, forbidden, grass_only)
    for (cr, cc) in tree_centres:
        for _ in range(rng.randint(2, 4)):
            for _ in range(20):
                dr = max(-2, min(2, round(rng.gauss(0, 0.9))))
                dc = max(-2, min(2, round(rng.gauss(0, 0.9))))
                r, c = cr + dr, cc + dc
                if (r, c) not in grass_only or (r, c) in forbidden:
                    continue
                t = rng.choice([TREE_S, TREE_S, TREE_L])
                scene.paste(tile(src, *t), (c * TILE, r * TILE))
                forbidden.add((r, c))
                break

    # Bushes: 4 scattered
    for (r, c) in poisson_disk(rng, 4, 3.0, forbidden, grass_only):
        scene.paste(tile(src, *BUSH), (c * TILE, r * TILE))
        forbidden.add((r, c))

    # Wooden rings: 1-2
    for (r, c) in poisson_disk(rng, 2, 5.0, forbidden, grass_only):
        scene.paste(tile(src, *RING), (c * TILE, r * TILE))
        forbidden.add((r, c))

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
