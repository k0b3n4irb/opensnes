#!/usr/bin/env python3
"""Static PPU asset-budget report — VRAM / CGRAM weight of converted graphics.

This is the *build-time, static* twin of `tools/luna-test/budget.py`. Where that
one asks luna how much VRAM/CGRAM a running scene actually fills (a runtime
footprint, a lower bound), this one weighs the **converted asset files on disk**
before the ROM ever runs — the "budget before you draw" worksheet from
`docs/craft/craft_planning` turned into a number, with no emulator involved.

It reads only the gfx4snes / tmx2snes pipeline outputs:

    .pic / .pc7   tile data      -> VRAM (bytes)
    .map / .mp7   tilemap data   -> VRAM (bytes)
    .pal          palette        -> CGRAM (2 bytes per colour)

and totals them per example against the hard limits:

    VRAM 65536 bytes   ·   CGRAM 256 colours

What this is NOT: it does not know the game's *runtime* VRAM layout. It is an
inventory — the total weight of an example's converted graphics — and thus an
*upper bound* on what those assets could occupy if loaded at once. A streaming
game legitimately ships more asset bytes than fit in VRAM (it loads a window at
a time), so a total over 64 KB is a flag to think, not a failure. For "what is
actually resident in a scene," run `make budget` (the luna runtime measure).
The two are complementary: this bounds the assets you built; that measures the
scene you loaded.

Because it over-counts (streaming, non-simultaneous loads, LZ77-compressed .pic
that expand in VRAM), it is a **report with a soft warning**, never a build
gate — unlike the linker-side bank-$00 / RAM ratchets in symmap.py.

Usage:
  python3 devtools/asset_budget.py                  # whole corpus, sorted heaviest first
  python3 devtools/asset_budget.py --only mode1     # examples whose path contains "mode1"
  python3 devtools/asset_budget.py --warn 80        # flag examples >= 80% of any limit
  python3 devtools/asset_budget.py examples/games/rpg
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

VRAM_LIMIT = 65536      # bytes
CGRAM_LIMIT = 256       # colours

TILE_EXT = {".pic", ".pc7"}
MAP_EXT = {".map", ".mp7"}
PAL_EXT = {".pal"}


class Budget:
    """VRAM/CGRAM weight of one example's converted assets."""

    def __init__(self, name: str):
        self.name = name
        self.tiles = 0      # bytes
        self.maps = 0       # bytes
        self.pal_bytes = 0  # bytes (2 per colour)
        self.files = 0

    def add(self, path: Path) -> None:
        ext = path.suffix.lower()
        size = path.stat().st_size
        if ext in TILE_EXT:
            self.tiles += size
        elif ext in MAP_EXT:
            self.maps += size
        elif ext in PAL_EXT:
            self.pal_bytes += size
        else:
            return
        self.files += 1

    @property
    def vram(self) -> int:
        return self.tiles + self.maps

    @property
    def colours(self) -> int:
        return self.pal_bytes // 2

    @property
    def worst_pct(self) -> float:
        return max(100.0 * self.vram / VRAM_LIMIT,
                   100.0 * self.colours / CGRAM_LIMIT)


def scan_example(example_dir: Path) -> Budget:
    """Total every converted asset found under an example directory."""
    b = Budget(str(example_dir))
    for path in example_dir.rglob("*"):
        if path.is_file() and path.suffix.lower() in (TILE_EXT | MAP_EXT | PAL_EXT):
            b.add(path)
    return b


def find_examples(root: Path) -> list[Path]:
    """Every directory holding a main.c (one example each)."""
    return sorted({p.parent for p in root.rglob("main.c")})


def kb(n: int) -> str:
    return f"{n / 1024:.1f}K"


def main() -> int:
    ap = argparse.ArgumentParser(description="Static PPU asset-budget report")
    ap.add_argument("paths", nargs="*", type=Path,
                    help="example dirs to scan (default: examples/ corpus)")
    ap.add_argument("--only", metavar="SUBSTR",
                    help="only examples whose path contains SUBSTR")
    ap.add_argument("--warn", type=float, default=80.0, metavar="PCT",
                    help="flag examples at >= PCT%% of any limit (default 80)")
    ap.add_argument("--oneline", action="store_true",
                    help="compact one-line-per-example output for post-build use "
                         "(silent when an example has no converted assets)")
    args = ap.parse_args()

    repo = Path(__file__).resolve().parent.parent
    if args.paths:
        example_dirs = [p if p.is_absolute() else repo / p for p in args.paths]
    else:
        example_dirs = find_examples(repo / "examples")
    if args.only:
        example_dirs = [d for d in example_dirs if args.only in str(d)]

    budgets = []
    for d in example_dirs:
        b = scan_example(d)
        b.name = str(d.relative_to(repo)) if d.is_relative_to(repo) else str(d)
        b.name = b.name.replace("examples/", "")
        if b.files:
            budgets.append(b)

    # Compact per-build line (post-link instrument, like the bank-$00/RAM lines).
    # Silent for asset-less examples so audio/text builds add no noise, and no
    # warning marker: an inventory over 100% is legitimate (streaming, per-scene
    # or per-scanline palette swaps), so a per-build alarm would cry wolf.
    if args.oneline:
        for b in budgets:
            vpct = 100.0 * b.vram / VRAM_LIMIT
            cpct = 100.0 * b.colours / CGRAM_LIMIT
            print(f"[ASSETS] VRAM {kb(b.vram)}/64K ({vpct:.0f}%) · "
                  f"CGRAM {b.colours}/256 ({cpct:.0f}%)")
        return 0

    if not budgets:
        print("no converted assets found "
              "(build the examples first: `make examples`)")
        return 0

    budgets.sort(key=lambda b: b.vram, reverse=True)

    print(f"{'example':<34} {'VRAM':>12} {'CGRAM':>11}   breakdown")
    print(f"{'':<34} {'/64K':>12} {'/256':>11}")
    print("-" * 90)
    flagged = 0
    for b in budgets:
        vpct = 100.0 * b.vram / VRAM_LIMIT
        cpct = 100.0 * b.colours / CGRAM_LIMIT
        mark = " !" if b.worst_pct >= args.warn else ""
        vram_col = f"{kb(b.vram):>6} {vpct:4.0f}%"
        cgram_col = f"{b.colours:>4} {cpct:4.0f}%"
        detail = f"tiles {kb(b.tiles)}, maps {kb(b.maps)}, pal {b.colours}c"
        print(f"{b.name:<34} {vram_col:>12} {cgram_col:>11}   {detail}{mark}")
        if b.worst_pct >= args.warn:
            flagged += 1

    print("-" * 90)
    print(f"{len(budgets)} examples with assets · "
          f"VRAM limit 64K · CGRAM limit 256 colours")
    if flagged:
        print(f"! {flagged} at >= {args.warn:.0f}% of a limit — an inventory upper "
              f"bound; see `make budget` for the runtime footprint.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
