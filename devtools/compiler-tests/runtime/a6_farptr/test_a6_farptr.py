#!/usr/bin/env python3
"""A6 far-pointer runtime check: deref of a pointer to BANK 2 data.

Proves the A6 remaining gap: the pointer VALUE keeps its bank byte (r_ptrhi),
but dereferencing it reads bank $00 (the `lda.l $0000,x` codegen). The deref
cases are xfail until A6 Phase 1 lowers pointer derefs as 24-bit.
"""
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[3] / "tools" / "luna-test" / "probes"))
from lib import find_luna, sym_of, assert_mem  # noqa: E402
ROM = HERE / "a6_farptr.sfc"; STEPS = 1_000_000
CASES = [("r_ptrhi", 2, 0x0002), ("r_d0", 1, 0x11), ("r_d3", 1, 0x44), ("r_d7", 1, 0x88)]
# Deref through a far pointer is bank-$00-hardcoded (emit.c `lda.l $0000,x`);
# fixed in A6 Phase 1. r_ptrhi (the pointer value's bank byte) already works.
KNOWN_FAIL = {"r_d0", "r_d3", "r_d7"}
def le(v, w): return "".join(f"{(v>>(8*i))&0xFF:02X}" for i in range(w))
def run():
    if not ROM.is_file(): sys.exit(f"ROM missing: {ROM} (run make)")
    luna = find_luna(); real = 0
    for name, w, want in CASES:
        b, o = sym_of(ROM, name)
        ok, det = assert_mem(luna, ROM, STEPS, [(b, o, le(want, w))])
        if name in KNOWN_FAIL:
            print(f"  {'XPASS' if ok else 'XFAIL'} {name} == 0x{want:0{w*2}X}"
                  + ("  ← fixed! remove from KNOWN_FAIL" if ok else "  (A6 deref gap)"))
            real += 1 if ok else 0
        else:
            print(f"  {'PASS' if ok else 'FAIL'}  {name} == 0x{want:0{w*2}X}" + ("" if ok else f"  [{det}]"))
            real += 0 if ok else 1
    passed = sum(1 for n,_,_ in CASES if n not in KNOWN_FAIL)
    print(f"\nA6 far-ptr: {passed-real}/{passed} ok (+{len(KNOWN_FAIL)} xfail: deref gap)")
    return 1 if real else 0
sys.exit(run())
