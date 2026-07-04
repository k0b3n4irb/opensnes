#!/usr/bin/env python3
"""Runtime assertions for lib/source functions (libtest.sfc).

Builds nothing — assumes `make` produced libtest.sfc. Resolves each result
global from the .sym and asserts its WRAM value via `luna state --assert`.
This is the execution gate the library lacks: examples only exercise lib
functions transitively, and the visual-regression hash can't see a wrong
return value that doesn't change pixels.

Vectors covered (see main.c):
  - math: div16/mod16 (incl. divisor-0 contract and the 65535/1 worst
    case of the old O(quotient) loop), mul16, sqrt16
  - text: cursor_y wrap — the tilemapBuffer overflow guard
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "tools" / "luna-test" / "probes"))
from lib import find_luna, sym_of, assert_mem  # noqa: E402

ROM = HERE / "libtest.sfc"
STEPS = 1_000_000

# (global, width-bytes, expected-value)
CASES = [
    ("r_div_a",    2, 14),
    ("r_mod_a",    2, 2),
    ("r_div_max",  2, 65535),
    ("r_div_zero", 2, 0),
    ("r_mod_zero", 2, 0),
    ("r_mul",      2, 5535),
    ("r_sqrt",     2, 12),
    ("s_map_width", 1, 32),
    ("s_cursor_y",  1, 8),
    ("r_done",     2, 0xBEEF),
]


def le_bytes(value: int, width: int) -> str:
    return "".join(f"{(value >> (8 * i)) & 0xFF:02X}" for i in range(width))


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    fails = 0
    for name, width, want in CASES:
        bank, off = sym_of(ROM, name)
        ok, detail = assert_mem(luna, ROM, STEPS, [(bank, off, le_bytes(want, width))])
        if ok:
            print(f"  PASS  {name} == 0x{want:0{width*2}X}")
        else:
            print(f"  FAIL  {name} == 0x{want:0{width*2}X}  [{detail}]")
            fails += 1
    print(f"\nLib runtime assertions: {len(CASES) - fails}/{len(CASES)} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
