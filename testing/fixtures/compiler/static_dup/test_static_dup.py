#!/usr/bin/env python3
"""Two sources, one static name each: the link succeeds and each file reads
its own `k` and `tag()` (cproc emits file-scope statics as name.<source>)."""
from __future__ import annotations
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402
ROM = HERE / "static_dup.sfc"
CASES = [("k.main", "1111"), ("k.other", "2222"), ("r_main", "BBBB"), ("r_other", "DDDD")]
def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna(); fails = 0
    for name, want in CASES:
        ok, detail = assert_mem(luna, ROM, 1_000_000, [(name, want)])
        print(("PASS " if ok else "FAIL ") + name + ("" if ok else f": {detail}"))
        fails += not ok
    return 1 if fails else 0
if __name__ == "__main__":
    sys.exit(run())
