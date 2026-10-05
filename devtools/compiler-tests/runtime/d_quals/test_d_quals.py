#!/usr/bin/env python3
"""Runtime gate for the qualifier / width miscompilations of 2026-10-03.

Assumes `make` produced d_quals.sfc. Each CASE is one cell of main.c; a FAIL
names the construct the compiler got wrong again (A_compiler.md, defects 1
to 4). The near_* and r_copy_p cells are controls.
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing" / "probes"))
from lib import find_luna, assert_mem  # noqa: E402

ROM = HERE / "d_quals.sfc"
STEPS = 1_000_000

# (global, width-bytes, expected)
CASES = [
    # 1. ++ / -- on FAR objects store back to bank $7E
    ("r_inc_dir",     2, 12),     ("r_inc_idx",  2, 6),       ("r_dec_fld", 2, 1),
    ("r_inc_long",    2, 1),      ("r_inc_near", 2, 12),
    # 2. whole-struct copy: every byte of a 4-aligned member
    ("r_copy_lo",     2, 0x3344), ("r_copy_hi",  2, 0x1122),  ("r_copy_k",  2, 0x5566),
    ("r_copy_p",      2, 0x444C),
    # 3. = {0} writes both halves of a 4-aligned object
    ("r_zero_arr",    2, 0),      ("r_zero_struct", 2, 0),
    # 4. bit-fields read with the object's bank
    ("r_bits_far",    2, 222),    ("r_bits_const0", 2, 222),  ("r_bits_const1", 2, 6),
    ("r_bits_near",   2, 222),
    ("r_done",        2, 0xBEEF),
]


def le_bytes(value: int, width: int) -> str:
    return "".join(f"{(value >> (8 * i)) & 0xFF:02X}" for i in range(width))


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    fails = 0
    for name, width, want in CASES:
        ok, detail = assert_mem(luna, ROM, STEPS, [(name, le_bytes(want, width))])
        extra = f"  [{detail.strip().splitlines()[-1] if detail else ''}]" if not ok else ""
        print(f"  {'PASS' if ok else 'FAIL'} {name} == 0x{want:0{width*2}X}{extra}")
        fails += not ok
    print(f"\nD qualifiers runtime: {len(CASES) - fails}/{len(CASES)} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
