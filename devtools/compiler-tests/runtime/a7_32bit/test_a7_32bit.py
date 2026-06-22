#!/usr/bin/env python3
"""Runtime-correctness check for A7 (32-bit Kl codegen).

Builds nothing — assumes `make` produced a7_32bit.sfc. Resolves each result
global from the .sym and asserts its WRAM value via `luna state --assert`. This
is the gate the A7 chantier needs (static C->ASM checks can't prove runtime
correctness of 32-bit arithmetic).
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "tools" / "luna-test" / "probes"))
from lib import find_luna, sym_of, assert_mem  # noqa: E402

ROM = HERE / "a7_32bit.sfc"
STEPS = 1_000_000

# (global, width-bytes, expected-value)
CASES = [
    ("r_add",   4, 0x00010000),
    ("r_sub",   4, 0x0000FFFF),
    ("r_mul",   4, 0x00012340),
    ("r_mulhi", 4, 0x00100000),
    ("r_shl",   4, 0x00100000),
    ("r_shr",   4, 0x08000000),
    ("r_sar",   4, 0xFFFFFFF0),
    ("r_div",   4, 0x00010000),
    ("r_mod",   4, 0x00000003),
    ("r_and",   4, 0x000F0F00),
    ("r_or",    4, 0x0FFFFF0F),
    ("r_xor",   4, 0x0FF0F00F),
    ("r_cmp",   2, 0x0001),
    ("r_sdiv",  4, 0xFFFFFFF0),
    ("r_smod",  4, 0xFFFFFFFF),
    ("r_slt",   2, 0x0001),
    ("r_sgt",   2, 0x0000),
    ("r_sar8",  4, 0xFFFFFFFF),
]


def le_bytes(value: int, width: int) -> str:
    return "".join(f"{(value >> (8 * i)) & 0xFF:02X}" for i in range(width))


# Known-failing cases (xfail) — documented A7 bugs not yet fixed. Remove an entry
# when its fix lands; an unexpected PASS (XPASS) then flags the stale xfail.
#
# (empty) — the Osar fold bug found in Phase 0 is FIXED: `compiler/qbe/fold.c`
# now folds `Osar` with 32-bit signed semantics (w65816 Kl is 32-bit), so
# arithmetic-shift-right of a negative 32-bit constant sign-extends correctly
# (r_sar, r_sar8). Signed Kl div/mod/compare were verified to go through the
# runtime path (not folded), so they were never affected.
KNOWN_FAIL: set[str] = set()


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    real_fails = 0
    for name, width, want in CASES:
        bank, off = sym_of(ROM, name)
        ok, detail = assert_mem(luna, ROM, STEPS, [(bank, off, le_bytes(want, width))])
        if name in KNOWN_FAIL:
            if ok:
                print(f"  XPASS {name} == 0x{want:0{width*2}X}  ← fixed! remove from KNOWN_FAIL")
                real_fails += 1
            else:
                print(f"  XFAIL {name} == 0x{want:0{width*2}X}  (known A7 bug, see header)")
        elif ok:
            print(f"  PASS  {name} == 0x{want:0{width*2}X}")
        else:
            print(f"  FAIL  {name} == 0x{want:0{width*2}X}  [{detail}]")
            real_fails += 1
    passed = sum(1 for n, _, _ in CASES if n not in KNOWN_FAIL)
    print(f"\nA7 32-bit runtime: {passed - real_fails}/{passed} ok"
          f" (+{len(KNOWN_FAIL)} xfail: {', '.join(sorted(KNOWN_FAIL))})")
    return 1 if real_fails else 0


if __name__ == "__main__":
    sys.exit(run())
