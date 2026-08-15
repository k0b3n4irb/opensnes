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
from lib import find_luna, assert_mem  # noqa: E402

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
# The A6 gap this matrix was built to track — far DEREF of bank-$02 data
# (byte/word/param/phi; hi2 is the pointer VALUE, not a deref) — is now CLOSED
# on the pinned toolchain. Every cell is a green regression guard; KNOWN_FAIL
# is empty.
#
# Closure history:
#   - l2_0 (Kl long load) was already bank-aware at the A1 baseline (qbe
#     1884a20); its long-load emit path was the model the other forms copied.
#   - b2_0/b2_7/w2_0/w2_2/fp2 (byte/word/param DEREF) closed with the A6+A7
#     far-pointer ABI; the KNOWN_FAIL set then went stale (a stale entry only
#     prints "promote", never fails, so it lingered). Promoted out 2026-08-13.
#   - ph2 (`pp = cond ? p2 : p0; ph2 = pp[5]`) closed 2026-08-13 by the
#     phi-bank-drop fix in qbe emitphimoves — the conditional dropped the
#     selected pointer's bank byte. See
#     .claude/notes/tech/ternary_addr_const_bank_drop.md.
#
# TRAP (2026-07-04, kept as method): b2_0/b2_7/fp2 were once promoted out on a
# STALE local a6_farptr.sfc (matrix green while the corpus regressed, never
# merged). Before trusting an XPASS here, `make clean` this directory first
# (`make tests` does) AND confirm the full corpus is green — both were done for
# the 2026-08-13 promotion.
KNOWN_FAIL = set()


def le(v, w):
    return "".join(f"{(v >> (8 * i)) & 0xFF:02X}" for i in range(w))


def run():
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run make)")
    luna = find_luna()
    regress, xfail, xpass = [], [], []
    for name, w, want, bank, form in CASES:
        ok, _ = assert_mem(luna, ROM, STEPS, [(name, le(want, w))])
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
