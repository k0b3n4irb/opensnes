#!/usr/bin/env python3
"""PPU resource-budget report — VRAM / CGRAM / OAM usage per example.

The linker-side budgets (bank $00 ROM, C RAM band) are checked by
`devtools/symmap/symmap.py` post-link. This is their PPU-side twin: it turns
the "budget before you draw" worksheet in `docs/craft/planning.md` into a live
number, by asking luna how much VRAM / CGRAM / OAM each example actually fills.

What it measures: luna's `ppu.{vram,cgram,oam}_non_zero` counters at a captured
scene — i.e. how much of each resource holds NON-ZERO data. That is a *footprint*
(a lower bound on allocation, since a region of zero-value tiles reads as unused),
not the linker's reserved layout, and it is a single snapshot (the loaded scene),
not the game's peak. It is the right order-of-magnitude gut-check for "does my
scene fit," and it is honest about what it is.

Limits (see docs/craft/planning.md):
  VRAM  65536 bytes  ·  CGRAM 256 colours  ·  OAM 544 bytes (128 sprites)

Usage:
  python3 tools/luna-test/budget.py                 # whole corpus, sorted
  python3 tools/luna-test/budget.py --only mode2    # one example (substring)
  python3 tools/luna-test/budget.py --warn 90       # flag >= 90% of any limit
  python3 tools/luna-test/budget.py mygame.sfc      # your own ROM(s)
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from luna_runner import (  # noqa: E402  (reuse the harness's helpers)
    find_luna,
    discover_example_roms,
    example_key,
    load_manifest,
)

VRAM_MAX = 65536   # bytes
CGRAM_MAX = 256    # colours
OAM_MAX = 544      # bytes (128 * 4 low table + 32 high table)


def steps_for(key: str, manifest: dict) -> int:
    """Warm-up instructions before the snapshot — respect per-example overrides
    (audio needs a longer warm-up; self-animating examples list two points —
    take the later/more-loaded one)."""
    entry = manifest.get("examples", {}).get(key, {})
    steps = entry.get("steps", manifest.get("default_steps", 3_000_000))
    if isinstance(steps, list):
        return max(steps) if steps else 3_000_000
    return int(steps)


def measure(luna: str, rom: Path, steps: int) -> dict | None:
    """Return {'vram','cgram','oam'} non-zero counts, or None on failure."""
    proc = subprocess.run(
        [luna, "state", "-n", str(steps), "--out", "-", str(rom)],
        capture_output=True, text=True, timeout=300,
    )
    try:
        ppu = json.loads(proc.stdout)["ppu"]
    except (json.JSONDecodeError, KeyError):
        return None
    return {
        "vram": ppu.get("vram_non_zero", 0),
        "cgram": ppu.get("cgram_non_zero", 0),
        "oam": ppu.get("oam_non_zero", 0),
    }


def bar(pct: float, width: int = 10) -> str:
    filled = int(round(pct / 100 * width))
    return "#" * min(filled, width) + "." * (width - min(filled, width))


def main() -> int:
    ap = argparse.ArgumentParser(description="PPU VRAM/CGRAM/OAM budget report")
    ap.add_argument("--only", metavar="SUBSTR",
                    help="restrict to examples whose key contains SUBSTR")
    ap.add_argument("--warn", type=float, default=90.0, metavar="PCT",
                    help="flag any resource at >= PCT%% of its limit (default 90)")
    ap.add_argument("rom", nargs="*", type=Path,
                    help="one or more .sfc ROMs to measure instead of the corpus "
                         "(point it at your own game)")
    args = ap.parse_args()

    luna = find_luna()
    manifest = load_manifest()
    if args.rom:
        roms = []
        for r in args.rom:
            if not r.is_file():
                print(f"not a file: {r}", file=sys.stderr)
                return 1
            roms.append(r)
    else:
        roms = discover_example_roms()
        if args.only:
            roms = [r for r in roms if args.only in example_key(r)]
    if not roms:
        print("no matching examples", file=sys.stderr)
        return 1

    def label(rom: Path) -> str:
        try:
            return example_key(rom)          # ".../examples/foo/bar" -> "foo/bar"
        except ValueError:
            return rom.stem                  # arbitrary ROM: just its name

    rows = []
    for rom in roms:
        key = label(rom)
        m = measure(luna, rom, steps_for(key, manifest))
        if m is None:
            print(f"  ??  {key} (could not read PPU state)", file=sys.stderr)
            continue
        vpct = 100 * m["vram"] / VRAM_MAX
        cpct = 100 * m["cgram"] / CGRAM_MAX
        opct = 100 * m["oam"] / OAM_MAX
        rows.append((key, m, vpct, cpct, opct, max(vpct, cpct, opct)))

    rows.sort(key=lambda r: r[2], reverse=True)  # busiest VRAM first

    print("PPU budget — non-zero footprint at the captured scene (lower bound).")
    print(f"limits: VRAM {VRAM_MAX} B · CGRAM {CGRAM_MAX} col · OAM {OAM_MAX} B\n")
    print(f"  {'example':<34} {'VRAM':>13} {'CGRAM':>9} {'OAM':>9}   flag")
    print(f"  {'-'*34} {'-'*13} {'-'*9} {'-'*9}   ----")
    flagged = 0
    for key, m, vpct, cpct, opct, worst in rows:
        flag = "  <-- >%d%%" % args.warn if worst >= args.warn else ""
        if flag:
            flagged += 1
        print(f"  {key:<34} "
              f"{m['vram']:>6} {vpct:>4.0f}% "
              f"{m['cgram']:>3} {cpct:>3.0f}% "
              f"{m['oam']:>3} {opct:>3.0f}%{flag}")

    if rows:
        peak = max(rows, key=lambda r: r[2])
        print(f"\n{len(rows)} example(s) measured. "
              f"Peak VRAM: {peak[0]} at {peak[2]:.0f}% "
              f"({peak[1]['vram']}/{VRAM_MAX} B). "
              f"{flagged} at/over {args.warn:.0f}% of a limit.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
