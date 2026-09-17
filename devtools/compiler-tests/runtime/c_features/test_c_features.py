#!/usr/bin/env python3
"""Runtime-correctness check of C features (gaps review C2, 2026-09-13).

Builds nothing — assumes `make` produced c_features.sfc. Resolves each result
global from the .sym and asserts its WRAM value via `luna state --assert`.
main.c documents every expected value next to its global; this file is the
list that gates `make tests`.
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "tools" / "luna-test" / "probes"))
from lib import find_luna, assert_mem  # noqa: E402

ROM = HERE / "c_features.sfc"
STEPS = 1_000_000

# (global, width-bytes, expected-value)
CASES = [
    # switch
    ("r_sw_dense",   2, 70),
    ("r_sw_sparse",  2, 3),
    ("r_sw_default", 2, 99),
    ("r_sw_fall",    2, 12),
    ("r_sw_neg",     2, 33),
    ("r_sw_32",      2, 5),
    ("r_sw_loop",    2, 6),
    # function pointers
    ("r_fp_const",   2, 11),
    ("r_fp_ram",     2, 49),
    ("r_fp_cb",      2, 42),
    ("r_fp_struct",  2, 16),
    ("r_fp_eq",      2, 3),
    # bit-fields, enum
    ("r_bf_sum",     2, 222),
    ("r_bf_ovf",     2, 1),
    ("r_bf_c",       2, 200),
    ("r_enum",       2, 206),
    ("r_enum_sw",    2, 2),
    # goto
    ("r_goto_fwd",    2, 1),
    ("r_goto_back",   2, 4),
    ("r_goto_nested", 2, 23),
    # recursion
    ("r_fact",   2, 720),
    ("r_fib",    2, 55),
    ("r_mutual", 2, 1),
    # 32-bit with runtime operands
    ("r_mul32",      4, 0x01234500),
    ("r_mul32_wrap", 4, 0xFFFFFFFE),
    ("r_div32",      4, 0x0000FFFF),
    ("r_mod32",      4, 0x00002345),
    ("r_smul32",     4, 0xFFFB6C20),
    ("r_sdiv32",     4, 0xFFFFC833),
    ("r_smod32",     4, 0xFFFFFFFB),
    ("r_shl_var",    4, 0x468A0000),
    ("r_shr_var",    4, 0x00000001),
    ("r_sar_var",    4, 0xFFFFFFFF),
    ("r_shl1",       4, 0xFFFFFFFE),
    ("r_cmp32",      2, 5),
    ("r_scmp32",     2, 7),
    ("r_eq32",       2, 6),
    ("r_jnz32",      2, 1),
    ("r_lnot32",     2, 6),
    # widths, promotions, truncations
    ("r_widen_u",   4, 120000),
    ("r_widen_s",   4, 0xFFFFFFF9),
    ("r_mul16to32", 4, 0xFFFEA070),
    ("r_trunc16",   2, 0x2345),
    ("r_trunc8",    1, 0x45),
    ("r_promote8",  2, 300),
    ("r_s8ext",     2, 0xFFF1),
    ("r_u16wrap",   2, 0),
    ("r_sdiv16",    2, 0xFFFD),
    ("r_smod16",    2, 0xFFFF),
    ("r_mixcmp",    2, 0),
    ("r_scmp16",    2, 15),
    ("r_sbr16",     2, 1),
    ("r_ucmp16",    2, 7),
    ("r_sizes",     2, 0x2442),
    # control flow, side effects, memory
    ("r_shortcirc",  2, 0x0001),
    ("r_ternary",    2, 3),
    ("r_dowhile",    2, 5),
    ("r_comma",      2, 7),
    ("r_ptr_rmw",    2, 10),
    ("r_postinc",    2, 0x0504),
    ("r_str",        2, 0x45),
    ("r_2d",         2, 23),
    ("r_struct_arr", 2, 200),
    ("r_unary",      2, 21),
]


def le_bytes(value: int, width: int) -> str:
    return "".join(f"{(value >> (8 * i)) & 0xFF:02X}" for i in range(width))


# Known-failing cases (xfail). Empty: the four miscompilations this ROM found on
# its first run (2026-09-13 — bit-field reads, variable-count 32-bit shifts,
# signed 32-bit compares, s16->s32 sign extension) were fixed in the same
# chantier (KNOWN_LIMITATIONS.md, "Four silent miscompilations"). An entry
# added here must name its KNOWN_LIMITATIONS entry; an XPASS flags a stale one.
KNOWN_FAIL: set[str] = set()


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    real_fails = 0
    for name, width, want in CASES:
        ok, detail = assert_mem(luna, ROM, STEPS, [(name, le_bytes(want, width))])
        if name in KNOWN_FAIL:
            if ok:
                print(f"  XPASS {name} == 0x{want:0{width*2}X}  <- fixed! remove from KNOWN_FAIL")
                real_fails += 1
            else:
                print(f"  XFAIL {name} == 0x{want:0{width*2}X}")
        elif ok:
            print(f"  PASS  {name} == 0x{want:0{width*2}X}")
        else:
            print(f"  FAIL  {name} == 0x{want:0{width*2}X}  [{detail}]")
            real_fails += 1
    checked = sum(1 for n, _, _ in CASES if n not in KNOWN_FAIL)
    print(f"\nC-feature runtime: {checked - real_fails}/{checked} ok"
          f" (+{len(KNOWN_FAIL)} xfail)")
    return 1 if real_fails else 0


if __name__ == "__main__":
    sys.exit(run())
