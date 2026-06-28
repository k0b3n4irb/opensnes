#!/usr/bin/env python3
"""A6 far-pointer MATRIX runtime test (Tier 2, Phase A — A1).

Each cell is a pointer DEREF crossed by {byte, word, long, param/RSlot, phi} ×
{bank $00, bank $02}. Bank-$02 deref cells exercise the A6 far gap (KNOWN_FAIL
until the far-deref codegen lands); bank-$00 cells + the pointer-VALUE high half
must already pass. As Phase A turns a bank-$02 cell green it XPASSes — promote it
out of KNOWN_FAIL. The run fails ONLY on a regression of an expected-green cell,
so it is a safe gate while Phase A is in flight.

Run: cd devtools/compiler-tests/runtime/a6_farptr && make && python3 test_a6_farptr.py
"""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[3] / "tools" / "luna-test" / "probes"))
from lib import find_luna, sym_of, assert_mem  # noqa: E402

ROM = HERE / "a6_farptr.sfc"
STEPS = 1_000_000

# (name, width_bytes, expected, bank, form)
CASES = [
    ("b0_0", 1, 0xA0,       0, "byte"),  ("b0_7", 1, 0xA7,       0, "byte"),
    ("w0_0", 2, 0xA1A0,     0, "word"),  ("l0_0", 4, 0xA3A2A1A0, 0, "long"),
    ("fp0",  1, 0xA3,       0, "param"), ("hi0",  2, 0x0000,     0, "ptr-hi"),
    ("b2_0", 1, 0x11,       2, "byte"),  ("b2_7", 1, 0x88,       2, "byte"),
    ("w2_0", 2, 0x2211,     2, "word"),  ("w2_2", 2, 0x4433,     2, "word"),
    ("l2_0", 4, 0x44332211, 2, "long"),
    ("fp2",  1, 0x44,       2, "param"), ("ph2",  1, 0x66,       2, "phi"),
    ("hi2",  2, 0x0002,     2, "ptr-hi"),   # pointer VALUE carries bank → passes pre-A6
]
# The A6 gap: far DEREF of bank-$02 data. (hi2 is the pointer value, not a deref.)
# FINDING (A1 baseline): l2_0 — the Kl (u32) load — is ALREADY bank-aware on qbe
# 1884a20 (reads bank $02 correctly) while byte/word/param/phi are not. So the
# long-load emit path is the working model to replicate for the other forms in A2.
KNOWN_FAIL = {"b2_0", "b2_7", "w2_0", "w2_2", "fp2", "ph2"}


def le(v, w):
    return "".join(f"{(v >> (8 * i)) & 0xFF:02X}" for i in range(w))


def run():
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run make)")
    luna = find_luna()
    regress, xfail, xpass = [], [], []
    for name, w, want, bank, form in CASES:
        b, o = sym_of(ROM, name)
        ok, _ = assert_mem(luna, ROM, STEPS, [(b, o, le(want, w))])
        if name in KNOWN_FAIL:
            (xpass if ok else xfail).append(name)
            note = "  <- XPASS: promote out of KNOWN_FAIL" if ok else "  (A6 gap, xfail)"
        else:
            if not ok:
                regress.append(name)
            note = "" if ok else "  <- REGRESSION"
        print(f"  [{' ' if ok else 'X'}] bank{bank} {form:<6} {name:<5} == 0x{want:0{w*2}X}{note}")
    green = [c[0] for c in CASES if c[0] not in KNOWN_FAIL]
    print(f"\nA6 matrix: {len(green) - len(regress)}/{len(green)} expected-green ok; "
          f"{len(xfail)} xfail (A6 far gap), {len(xpass)} XPASS")
    return 1 if regress else 0


if __name__ == "__main__":
    sys.exit(run())
