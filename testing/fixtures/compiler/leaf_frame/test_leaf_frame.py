#!/usr/bin/env python3
"""The direct-page frame of leaf functions and the NMI: the same leaf runs in
the main loop and in the NMI callback, each side counts its wrong results.
Assumes `make` produced leaf_frame.sfc (see main.c for what is at stake)."""
from __future__ import annotations
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402
ROM = HERE / "leaf_frame.sfc"
ASM = HERE / "main.c.asm"
# little-endian words
CASES = [("done", "EFBE"),            # the run reached its end
         ("runs_main", "1800"),       # 24 turns of the main loop
         ("runs_nmi", "1800"),        # and the callback ran on each of them
         ("bad_main", "0000"), ("bad_nmi", "0000"),
         ("want_main", "681B"), ("want_nmi", "2816")]
def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    fails = 0
    # the premise: leaf() really keeps its temps in the direct page
    uses = ASM.read_text().count("tcc__lf") if ASM.is_file() else 0
    ok = uses > 0
    print(("PASS " if ok else "FAIL ") + f"leaf() uses tcc__lf ({uses} operands)")
    fails += not ok
    luna = find_luna()
    for name, want in CASES:
        ok, detail = assert_mem(luna, ROM, 4_000_000, [(name, want)])
        print(("PASS " if ok else "FAIL ") + name + ("" if ok else f": {detail}"))
        fails += not ok
    return 1 if fails else 0
if __name__ == "__main__":
    sys.exit(run())
