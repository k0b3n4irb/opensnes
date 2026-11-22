#!/usr/bin/env python3
"""A 32-bit mask of a 32-bit 0 / 1 chosen by a one-bit test: the ROM builds
(the compiler stopped on gvn's assoccon assertion before 2026-11-22) and every
value, high half included, is `a & 1` (see main.c). Assumes `make` produced
assoc_widen.sfc."""
from __future__ import annotations
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402
ROM = HERE / "assoc_widen.sfc"

AV = [0, 1, 2, 3, 0x8001, 0xFFFF]


def le32(vals: list[int]) -> str:
    return "".join(v.to_bytes(4, "little").hex().upper() for v in vals)


BITS = le32([a & 1 for a in AV])
CASES = [
    ("r_and", BITS),
    ("r_high", BITS),
    ("r_neg", BITS),
    ("score", le32([sum(a & 1 for a in AV)])),
    ("done", "EFBE"),
]


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    fails = 0
    for name, want in CASES:
        ok, detail = assert_mem(luna, ROM, 1_000_000, [(name, want)])
        print(("PASS " if ok else "FAIL ") + name + ("" if ok else f": {detail}"))
        fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
