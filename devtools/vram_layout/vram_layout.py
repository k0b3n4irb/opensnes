"""VRAM layout generator (OR-Tools CP-SAT) — opt-in, build-TIME, reproducible.

Reads a `vram.spec` (see vram_spec.py), computes base word-addresses that satisfy
the SDK's hardware alignment, never overlap, fit in 64 KB, and minimise the VRAM
high-water mark, then emits the example's `vram_map.h`. The committed header is
what the build consumes — ortools is NOT a build dependency. Re-run this only when
the spec changes; the build gate (`make lint-vram`, pure stdlib) re-validates the
committed header against the spec on every CI run.

Usage:
  python3 vram_layout.py --emit examples/basics/collision_demo   # (re)generate vram_map.h
  python3 vram_layout.py --emit DIR --keep-low NAME              # pin a region's base low
  python3 vram_layout.py                                          # demo on collision_demo

Solver is pinned single-threaded with a fixed seed → identical output every run
(reproducible builds; same-spec ⇒ same header).
"""
from __future__ import annotations

import sys
from pathlib import Path

from ortools.sat.python import cp_model

sys.path.insert(0, str(Path(__file__).resolve().parent))
from vram_spec import VRAM_WORDS, Region, emit_header, parse_spec, validate  # noqa: E402


def solve(regions, forbid_bases=None, pin=None):
    """Return ({region.name: base}, highwater) minimising high-water, or None.

    pin: {name: base} to fix a region (e.g. keep an obj sheet at its origin).
    forbid_bases: {name: set(addresses)} to exclude.
    """
    m = cp_model.CpModel()
    forbid_bases, pin = forbid_bases or {}, pin or {}
    bases, intervals, ends = {}, [], []
    for r in regions:
        base = m.NewIntVar(0, VRAM_WORDS - r.size, f"base_{r.name}")
        if r.name in pin:
            m.Add(base == pin[r.name])
        else:
            k = m.NewIntVar(0, (VRAM_WORDS - r.size) // r.align, f"k_{r.name}")
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


def emit(example_dir, pin=None):
    d = Path(example_dir)
    spec = d / "vram.spec"
    regions = parse_spec(spec)
    result = solve(regions, pin=pin)
    if result is None:
        sys.exit(f"{spec}: INFEASIBLE — assets do not fit under the constraints")
    bases, hw = result
    ok, issues = validate({r.define: bases[r.name] for r in regions}, regions)
    if not ok:
        sys.exit("solver produced an invalid layout (bug):\n  " + "\n  ".join(issues))
    out = d / "vram_map.h"
    emit_header(out, bases, regions, spec_name=spec.name)
    print(f"wrote {out}  (high-water 0x{hw:04X} words / {hw * 2} bytes)")
    for r in sorted(regions, key=lambda r: bases[r.name]):
        print(f"  {r.define:<16} 0x{bases[r.name]:04X}  ({r.kind}, {r.size}w)")
    return 0


def main(argv) -> int:
    if "--emit" in argv:
        i = argv.index("--emit")
        example_dir = argv[i + 1]
        pin = {}
        if "--keep-low" in argv:
            pin[argv[argv.index("--keep-low") + 1]] = 0x0000
        # --pin name=0xADDR (repeatable): freeze a region at an exact base —
        # used when generalising an existing example (keep its addresses, just
        # name + validate them), so the render is byte-identical.
        while "--pin" in argv:
            i = argv.index("--pin")
            name, _, addr = argv[i + 1].partition("=")
            pin[name] = int(addr, 0)
            del argv[i:i + 2]
        return emit(example_dir, pin=pin or None)
    # No args: demo on collision_demo's spec.
    demo = Path(__file__).resolve().parents[2] / "examples" / "basics" / "collision_demo"
    if (demo / "vram.spec").is_file():
        return emit(demo)
    print("usage: vram_layout.py --emit <example_dir> [--keep-low <region>]")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
