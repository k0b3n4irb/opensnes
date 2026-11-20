"""scrolling/tile_ring: an archipelago with more 8x8 characters than VRAM holds.

Imported by build.py, and runnable alone: `python3 tile_ring.py` writes
res/archipelago.png, `--check` says whether the committed one is what it writes.

Everything is invented here, from a fixed seed. The picture is 1280x896
pixels (160x112 characters), indexed, two palettes:

- palette 1, the sea: a swell drawn on a 32x16 motif whose right half is the
  left half mirrored, so opensnes-tilering keeps a handful of characters for
  the whole sea (the mirror merge at work);
- palette 0, the islands: blobs with a noisy shore, a sand rim and a
  grass-and-rock texture from value noise, so that nearly every land
  character is distinct. Their number and size set the figures the example
  is about: well over 1,024 distinct characters in the whole map, and a
  worst screen that a ring of 768 slots holds.
"""
from __future__ import annotations

import argparse
import math
import random
import sys
from io import BytesIO
from pathlib import Path

from PIL import Image

W, H = 1280, 896
SEED = 20261120

# palette 0 (land) then palette 1 (sea), 16 colours each, colour 0 unused
# palette 0: grass 1-5, sand 6-7, rock 8-10, and 11-15 the sea's colours, for
# the shore characters, which hold land and sea (one palette per character)
LAND = ["000000", "1d3b1f", "2b5a2d", "3d7a3b", "56a04a", "7cbd5e",
        "c9b47e", "e3d39a", "6d6458", "948a7b", "c2baa9",
        "10295a", "173873", "2b5ea8", "5a8fd1", "a9cdee"]
SEA = ["000000", "0b1e3d", "10295a", "173873", "1f4a8f", "2b5ea8", "3d74bf",
       "5a8fd1", "7eaee0", "a9cdee", "d5e8f8", "0b1e3d", "0b1e3d", "0b1e3d",
       "0b1e3d", "0b1e3d"]


def _lattice(rng: random.Random, step: int) -> tuple[int, list[list[float]]]:
    cols, rows = W // step + 2, H // step + 2
    return step, [[rng.random() for _ in range(cols)] for _ in range(rows)]


def _sample(lat: tuple[int, list[list[float]]], x: int, y: int) -> float:
    step, g = lat
    gx, gy = x / step, y / step
    ix, iy = int(gx), int(gy)
    fx, fy = gx - ix, gy - iy
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    a = g[iy][ix] + (g[iy][ix + 1] - g[iy][ix]) * fx
    b = g[iy + 1][ix] + (g[iy + 1][ix + 1] - g[iy + 1][ix]) * fx
    return a + (b - a) * fy


SEA_ON_LAND = {2: 11, 3: 12, 5: 13, 7: 14, 9: 15}


def _sea(x: int, y: int) -> int:
    """The swell (sea colour 2-9): a 32x16 motif, its right half the left half mirrored."""
    mx, my = x % 32, y % 16
    if mx >= 16:
        mx = 31 - mx
    crest = 6 + round(3 * math.sin(mx * math.pi / 8))
    if my == crest:
        return 9
    if my == crest + 1:
        return 7
    if (mx * 7 + my * 3) % 11 == 0:
        return 5
    return 3 if my < 8 else 2


def build_tile_ring() -> dict[str, bytes]:
    rng = random.Random(SEED)
    coarse, fine, shore = _lattice(rng, 24), _lattice(rng, 5), _lattice(rng, 16)
    # (centre x, centre y, radius) in pixels: six islands, their shores at
    # least 24 pixels apart
    islands = []
    for _ in range(400):
        if len(islands) == 6:
            break
        r = rng.randint(40, 84)
        cx, cy = rng.randint(r, W - r), rng.randint(r, H - r)
        if all(math.hypot(cx - x, cy - y) > r + q + 24 for x, y, q in islands):
            islands.append((cx, cy, r))
    img = Image.new("P", (W, H))
    img.putpalette(sum(([int(c[i:i + 2], 16) for i in (0, 2, 4)] for c in LAND + SEA), []))
    px = img.load()
    land = [[None] * W for _ in range(H)]          # distance to the shore, None = sea
    for y in range(H):
        for x in range(W):
            for cx, cy, r in islands:
                d = math.hypot(x - cx, y - cy) / r + 0.35 * (_sample(shore, x, y) - 0.5)
                if d < 1.0:
                    land[y][x] = d
                    break
    for cy in range(0, H, 8):
        for cx in range(0, W, 8):
            coast = any(land[y][x] is not None for y in range(cy, cy + 8) for x in range(cx, cx + 8))
            for y in range(cy, cy + 8):
                for x in range(cx, cx + 8):
                    edge = land[y][x]
                    if edge is None:
                        s = _sea(x, y)
                        px[x, y] = SEA_ON_LAND[s] if coast else 16 + s
                    elif edge > 0.9:
                        px[x, y] = 6 + (1 if (x * 13 + y * 7) % 5 == 0 else 0)     # sand
                    else:
                        v = 0.65 * _sample(coarse, x, y) + 0.35 * _sample(fine, x, y)
                        v += (((x * 73856093) ^ (y * 19349663)) % 97) / 97.0 * 0.12   # grain
                        if v > 0.78:
                            px[x, y] = 8 + min(2, int((v - 0.78) * 14))            # rock
                        else:
                            px[x, y] = 1 + min(4, int(v * 6.5))                    # grass
    out = BytesIO()
    img.save(out, format="PNG", optimize=False)
    return {"archipelago.png": out.getvalue()}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--check", action="store_true", help="compare with the committed file instead of writing it")
    args = ap.parse_args()
    path = Path(__file__).resolve().parents[2] / "examples" / "scrolling" / "tile_ring" / "res" / "archipelago.png"
    data = build_tile_ring()["archipelago.png"]
    if args.check:
        if not path.is_file() or path.read_bytes() != data:
            print(f"  DIFFERS {path}")
            return 1
        return 0
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return 0


if __name__ == "__main__":
    sys.exit(main())
