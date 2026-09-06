#!/usr/bin/env python3
"""Runtime gate for B2 (C objects in bank $7E).

Assumes `make` produced b2_far_ram.sfc. Each CASE is one matrix cell of
main.c; a FAIL names the emit path that still drops the bank byte. The
bank-0 control cells (c0_*) must never fail.
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "tools" / "luna-test" / "probes"))
from lib import find_luna, assert_mem  # noqa: E402

ROM = HERE / "b2_far_ram.sfc"
STEPS = 1_000_000

# (global, width-bytes, expected)
CASES = [
    ("r_dir8",  1, 0x5A),        ("r_dir16", 2, 0xBEEF),   ("r_dir32", 4, 0x12345678),
    ("r_idx8",  1, 0xC3),        ("r_idx16", 2, 0xCAFE),
    ("r_ptr8",  1, 0x77),        ("r_ptr16", 2, 0xD00D),   ("r_ptr32", 4, 0xA5A5F00F),
    ("r_par8",  1, 0x42),        ("r_parl8", 1, 0x69),
    ("r_fld8",  1, 0x21),        ("r_fld16", 2, 0x4321),   ("r_fld32", 4, 0x0BADF00D),
    ("r_idx32", 4, 0xDEADBEEF),
    ("r_pfld16", 2, 0x7777),     ("r_pfld32", 4, 0x89ABCDEF),
    ("r_walk",  2, 360),
    ("r_init",  2, 0x4444),      ("r_init8", 2, 0x00A7),
    ("r_zero",  2, 0x0100),
    ("r_hi",    2, 0x007E),
    ("c0_dir8", 1, 0x5A),        ("c0_dir16", 2, 0xBEEF),  ("c0_idx8", 1, 0xC3),
    ("c0_ptr8", 1, 0x77),
]

# Cells known to fail on the current compiler. Empty since B2 Phase 2
# (2026-09-06): every far access form is bank-honouring — symbol-direct
# `lda.l/sta.l sym`, `sym[idx]` as absolute long indexed, runtime pointers
# through `[tcc__r9]` (+`,y` for base+offset decompositions), initialised
# far data via the bank byte in the init record, and the boot zero-fill of
# $7E:2000-$FFFF. A regression here names the emit path that dropped the
# bank; an XPASS flags a stale entry.
KNOWN_FAIL: set[str] = set()


def le_bytes(value: int, width: int) -> str:
    return "".join(f"{(value >> (8 * i)) & 0xFF:02X}" for i in range(width))


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    real_fails = 0
    for name, width, want in CASES:
        ok, detail = assert_mem(luna, ROM, STEPS, [(name, le_bytes(want, width))])
        tag = "PASS " if ok else "FAIL "
        if name in KNOWN_FAIL:
            tag = "XPASS" if ok else "XFAIL"
            if ok:
                real_fails += 1
        elif not ok:
            real_fails += 1
        extra = f"  [{detail.strip().splitlines()[-1] if detail else ''}]" if not ok else ""
        print(f"  {tag} {name} == 0x{want:0{width*2}X}{extra}")
    total = len(CASES) - len(KNOWN_FAIL)
    print(f"\nB2 far-RAM runtime: {total - real_fails}/{total} ok"
          f" (+{len(KNOWN_FAIL)} xfail)")
    return 1 if real_fails else 0


if __name__ == "__main__":
    sys.exit(run())
