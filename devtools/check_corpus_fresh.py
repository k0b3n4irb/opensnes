#!/usr/bin/env python3
"""Corpus freshness guard (issue #105).

Incremental example builds have twice produced ROMs whose WRAM streams
differ from a clean build of the same sources (aim_target, 2026-07-12,
deterministic 811b15ed... vs clean c6ffcd01...). Baselines captured from
such a tree get rejected by CI (which always builds clean). Until the
micro-mechanism is fully root-caused, this guard converts the failure
mode from 'silent wrong baselines' into a loud error: `make tests`
refuses to run if any example .sfc predates the newest lib/toolchain
build output.

One mechanism is now known (2026-09-13, review C2): after a compiler
change, `make clean-examples && make examples` recompiles the examples
with the new compiler but links them against lib objects the OLD compiler
produced (nothing in lib/ changed, so make keeps them). The ROMs are then
a mix no clean build can reproduce, and every baseline captured from
them fails CI. So the guard also refuses a lib whose objects predate the
toolchain binaries: the fix for that is a full `make clean && make`.

Exit 0 = corpus fresh; 1 = stale (with the fix-it command).
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def newest_mtime(*globs: str) -> tuple[float, Path | None]:
    best, who = 0.0, None
    for g in globs:
        for f in ROOT.glob(g):
            m = f.stat().st_mtime
            if m > best:
                best, who = m, f
    return best, who


def main() -> int:
    # Toolchain timestamps come from the submodule build trees, not bin/:
    # every top-level make re-copies bin/ (the install step is unconditional),
    # so bin/ says when the toolchain was last installed, not built.
    lib_m, lib_f = newest_mtime("lib/build/**/*.o", "lib/build/**/*.asm",
                                "compiler/qbe/qbe", "compiler/cproc/cproc-qbe",
                                "compiler/wla-dx/binaries/wla-65816",
                                "compiler/wla-dx/binaries/wlalink")
    if lib_m == 0.0:
        print("corpus-fresh: no lib build outputs found — build the SDK first")
        return 0
    # The lib itself must have been compiled by the current toolchain: a
    # lib object older than any compiler binary was produced by a previous
    # compiler and would be linked into every example. The timestamps are
    # the submodules' own build outputs, not bin/: `make compiler` re-copies
    # bin/ on every top-level make (the install step is not conditional),
    # so bin/ mtimes say when the toolchain was last INSTALLED, the build
    # trees say when it was last BUILT.
    tool_m, tool_f = newest_mtime("compiler/qbe/qbe", "compiler/cproc/cproc-qbe",
                                  "compiler/wla-dx/binaries/wla-65816",
                                  "compiler/wla-dx/binaries/wlalink")
    stale_lib = [o.relative_to(ROOT) for o in ROOT.glob("lib/build/**/*.o")
                 if o.stat().st_mtime < tool_m]
    if stale_lib:
        print(f"STALE LIB: {len(stale_lib)} lib object(s) predate the newest "
              f"toolchain binary ({tool_f.relative_to(ROOT)}).")
        for o in stale_lib[:6]:
            print(f"  {o}")
        if len(stale_lib) > 6:
            print(f"  ... and {len(stale_lib) - 6} more")
        print("They were compiled by the previous compiler and get linked into")
        print("every example; a corpus built on them matches no clean build.")
        print("FIX:  make clean && make")
        return 1
    stale = []
    for sfc in ROOT.glob("examples/**/*.sfc"):
        if sfc.stat().st_mtime < lib_m:
            stale.append(sfc.relative_to(ROOT))
    if stale:
        print(f"STALE CORPUS: {len(stale)} example ROM(s) predate the newest "
              f"lib/toolchain output ({lib_f.relative_to(ROOT)}).")
        for s in stale[:8]:
            print(f"  {s}")
        if len(stale) > 8:
            print(f"  ... and {len(stale) - 8} more")
        print("Incremental trees have produced ROMs that differ from clean")
        print("builds (issue #105) — baselines captured now could be wrong.")
        print("FIX:  make clean-examples && make examples")
        return 1
    print(f"corpus-fresh: OK ({lib_f.relative_to(ROOT)} older than every example ROM)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
