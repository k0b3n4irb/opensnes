#!/usr/bin/env python3
"""Runtime gate: C objects in WRAM bank $7F, placed by assembly.

Assumes `make` produced far_bank7f.sfc. Each CASE is one cell of main.c: a
global in bank 0 holding what the compiler read back from a `.RAMSECTION ...
BANK $7F SLOT 3` object declared `extern FAR`. A FAIL names the form that
dropped or shared the bank byte.
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402

ROM = HERE / "far_bank7f.sfc"
STEPS = 3_000_000   # the overlap loops walk 35 KB four times

# (global, width-bytes, expected)
CASES = [
    # (a) direct symbol
    ("r_a8", 1, 0x5A), ("r_a16", 2, 0xBEEF), ("r_a32", 4, 0x12345678),
    # (b) symbol[idx], volatile index; r_bhi is byte 0x8400 of the object
    ("r_b8", 1, 0xC3), ("r_b16", 2, 0xCAFE), ("r_bhi", 2, 0xD00D),
    ("r_b32", 4, 0xDEADBEEF),
    # (c) run-time volatile FAR pointer
    ("r_c8", 1, 0x77), ("r_c16", 2, 0xA11A), ("r_chi", 2, 0xB22B),
    ("r_c32", 4, 0xA5A5F00F),
    # (d) pointer as a parameter
    ("r_d8", 1, 0x42), ("r_d16", 2, 0x6969), ("r_dhi", 2, 0x3C3C),
    ("r_d32", 4, 0x82063DE2),
    # (e) memcpy from ROM, memset over part of it
    ("r_e_c0", 1, 0x11), ("r_e_c15", 1, 0xF2), ("r_e_before", 1, 0x22),
    ("r_e_s0", 1, 0xEE), ("r_e_s3", 1, 0xEE), ("r_e_after", 1, 0x77),
    ("r_e_w0", 2, 0x2211), ("r_e_w7", 2, 0xF2F1),
    # (f) a $7E and a $7F object at the same offset, one function
    ("r_f_lo", 2, 0xF705), ("r_f_hi", 2, 0xE705),
    ("r_f_lo2", 2, 0xE704), ("r_f_hi2", 2, 0xF706),
    # (g) bank-0 control
    ("r_g16", 2, 0x7E7F),
    # two objects one after the other do not overlap; the banks as C sees them
    ("r_ovl_a", 2, 0), ("r_ovl_b", 2, 0), ("r_ovl_c", 2, 0), ("r_ord", 2, 1),
    ("r_bank_w", 2, 0x7F), ("r_bank_x", 2, 0x7F), ("r_bank_e", 2, 0x7E),
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
        print(f"  {'PASS ' if ok else 'FAIL '} {name} == 0x{want:0{width*2}X}{extra}")
        fails += 0 if ok else 1
    print(f"\nfar_bank7f: {len(CASES) - fails}/{len(CASES)} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
