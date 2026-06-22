"""VRAM layout solver (PILOT) — CP-SAT auto-placement of SNES VRAM regions.

Given the VRAM regions an example needs (BG tilesets, tilemaps, sprite tiles)
with their sizes in words, compute base word-addresses that:
  - satisfy the SDK's hardware alignment constraints,
  - never overlap,
  - fit in 64 KB ($8000 words),
minimising the VRAM high-water mark. It also *validates* a hand-coded layout — a
linter for the #1 VRAM debugging bug (silent tile/map/sprite overlap that the PPU
ignores with no error).

Alignment granularities are taken straight from the SDK source, so the model
can't drift from what the lib actually programs:
  bg_char  base / 0x1000  (background.c bgSetGfxPtr → BG12NBA = vramAddr>>12)
  bg_map   base / 0x400   (background.c bgSetMapPtr → BGnSC   = (vramAddr>>8)&0xFC)
  obj      base / 0x2000  (sprite.h OBJ_BASE        = (vram_addr>>13)&0x07)

Build-TIME, OPT-IN tool. Requires `ortools` (deliberately NOT a build
dependency — run this to (re)generate a layout and commit the result; the build
stays stdlib-only). Solver is pinned single-threaded + fixed seed so the output
is reproducible (a hard requirement for this project's deterministic builds).
"""
from __future__ import annotations

import sys
from dataclasses import dataclass

from ortools.sat.python import cp_model

VRAM_WORDS = 0x8000  # 64 KB / 2 bytes per word
ALIGN = {"bg_char": 0x1000, "bg_map": 0x400, "obj": 0x2000}


@dataclass(frozen=True)
class Region:
    name: str
    kind: str   # bg_char | bg_map | obj
    size: int   # words occupied

    @property
    def align(self) -> int:
        return ALIGN[self.kind]


def solve(regions, forbid_bases=None):
    """Return ({name: base}, highwater) minimising the high-water mark, or None."""
    m = cp_model.CpModel()
    forbid_bases = forbid_bases or {}
    bases, intervals, ends = {}, [], []
    for r in regions:
        nslots = (VRAM_WORDS - r.size) // r.align
        k = m.NewIntVar(0, nslots, f"k_{r.name}")
        base = m.NewIntVar(0, VRAM_WORDS - r.size, f"base_{r.name}")
        m.Add(base == r.align * k)
        for b in forbid_bases.get(r.name, ()):
            m.Add(base != b)
        end = m.NewIntVar(0, VRAM_WORDS, f"end_{r.name}")
        m.Add(end == base + r.size)
        intervals.append(m.NewIntervalVar(base, r.size, end, f"iv_{r.name}"))
        bases[r.name] = base
        ends.append(end)
    m.AddNoOverlap(intervals)
    hw = m.NewIntVar(0, VRAM_WORDS, "highwater")
    m.AddMaxEquality(hw, ends)
    m.Minimize(hw)

    s = cp_model.CpSolver()
    s.parameters.random_seed = 0
    s.parameters.num_search_workers = 1        # deterministic / reproducible
    if s.Solve(m) not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        return None
    return {n: s.Value(b) for n, b in bases.items()}, s.Value(hw)


def validate(layout, regions):
    """Linter: check alignment + fit + no overlap. Returns (ok, [issues])."""
    issues = []
    byname = {r.name: r for r in regions}
    for name, base in layout.items():
        r = byname[name]
        if base % r.align:
            issues.append(f"{name}: base 0x{base:04X} not aligned to 0x{r.align:X}")
        if base + r.size > VRAM_WORDS:
            issues.append(f"{name}: 0x{base:04X}+0x{r.size:X} overruns VRAM")
    items = sorted((base, base + byname[n].size, n) for n, base in layout.items())
    for (a0, a1, an), (b0, b1, bn) in zip(items, items[1:]):
        if a1 > b0:
            issues.append(f"overlap: {an}(0x{a0:04X}-0x{a1:04X}) ∩ {bn}(0x{b0:04X}-0x{b1:04X})")
    return (not issues), issues


def _show(title, layout, regions):
    print(f"  {title}")
    hw = max(b + next(r.size for r in regions if r.name == n) for n, b in layout.items())
    for n, b in sorted(layout.items(), key=lambda kv: kv[1]):
        r = next(r for r in regions if r.name == n)
        print(f"    {n:<12} 0x{b:04X}..0x{b + r.size:04X}  ({r.kind}, align 0x{r.align:X}, {r.size}w)")
    print(f"    high-water: 0x{hw:04X} words ({hw * 2} bytes)")


# ---- PILOT target: examples/basics/collision_demo --------------------------
# Real regions from its main.c: BG tiles (2bpp empty+wall, 8w each), 32x32
# tilemap (1024w), sprite tiles (4bpp player+enemy, 16w each).
COLLISION_DEMO = [
    Region("bg_tiles", "bg_char", 0x10),
    Region("bg_tilemap", "bg_map", 0x400),
    Region("obj_tiles", "obj", 0x20),
]
HAND_CODED = {"bg_tiles": 0x0000, "bg_tilemap": 0x0400, "obj_tiles": 0x4000}


def main() -> int:
    regs = COLLISION_DEMO
    print("collision_demo VRAM layout pilot\n")

    ok, issues = validate(HAND_CODED, regs)
    print(f"[1] linter on the hand-coded layout: {'VALID' if ok else 'INVALID'}")
    for i in issues:
        print(f"      ! {i}")
    _show("hand-coded:", HAND_CODED, regs)

    auto, hw = solve(regs)
    print(f"\n[2] solver (minimise high-water): {'OK' if auto else 'INFEASIBLE'}")
    _show("auto:", auto, regs)
    hand_hw = max(b + next(r.size for r in regs if r.name == n) for n, b in HAND_CODED.items())
    saved = hand_hw - hw
    print(f"    vs hand-coded high-water 0x{hand_hw:04X} → reclaims {saved} words "
          f"({saved * 2} bytes)" if saved > 0 else "    (matches hand-coded compactness)")

    # Force a relocation to prove the solver re-packs correctly under a constraint.
    moved, _ = solve(regs, forbid_bases={"bg_tiles": {0x0000}})
    print("\n[3] solver with bg_tiles forbidden at 0x0000 (forced relocation):")
    _show("relocated:", moved, regs)

    ok2, issues2 = validate(auto, regs)
    return 0 if (ok and ok2 and not issues2) else 1


if __name__ == "__main__":
    sys.exit(main())
