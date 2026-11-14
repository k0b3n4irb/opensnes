#!/usr/bin/env python3
"""The label of a copied block, cut where the assembler reads a size suffix:
three levels of `static inline` copies whose cut labels end in `.b`, `.w`,
`.l` (repaired by the inliner), run in the main loop and in the NMI callback,
against the same C computed here. Assumes `make` produced
inline_label_cut.sfc (see main.c for what is at stake)."""
from __future__ import annotations
import re
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402
ROM = HERE / "inline_label_cut.sfc"
ASM = HERE / "main.c.asm"
W = 0xFFFF


def bitsum(x):
    s = 0
    while x:
        if (x & 1) and s < 900:
            s = (s + x) & W
        x >>= 1
    return s


def pairer(x):
    return (bitsum(x) + bitsum(x ^ 0x55)) & W


def f(x):
    return pairer(x) ^ pairer((x + 3) & W)


def run_(x):
    return (f(x) + f((x * 7) & W)) & W


def sweep(start, n):
    acc = 0
    for i in range(n):
        acc = (acc * 3 + run_((start + i * 97) & W)) & W
    return acc


def le(v):
    return f"{v & 0xFF:02X}{v >> 8:02X}"


CASES = [("done", "EFBE"),                 # the run reached its end
         ("runs_main", le(16)),
         ("bad_main", "0000"), ("bad_nmi", "0000"),
         ("sum_main", le(sweep(0, 64))),
         ("want_nmi", le(sweep(5, 4))),
         ("sum_nmi", le(sweep(5, 4)))]


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    fails = 0
    # the premise: the third level's cut fell after .b, .w and .l, and each
    # such label was repaired (a trailing `_`)
    text = ASM.read_text() if ASM.is_file() else ""
    ends = set(re.findall(r"^@\S+(\.[bwlBWL])_:$", text, re.M))
    ok = ends == {".b", ".w", ".l"}
    print(("PASS " if ok else "FAIL ") + f"repaired cut labels end in {sorted(ends)}")
    fails += not ok
    bad = re.findall(r"^@\S+\.[bwlBWL]:$", text, re.M)
    ok = not bad
    print(("PASS " if ok else "FAIL ") + f"no label left on a size suffix ({len(bad)})")
    fails += not ok
    luna = find_luna()
    ok, detail = assert_mem(luna, ROM, 6_000_000, CASES)
    print(("PASS " if ok else "FAIL ") + "values against the host model"
          + ("" if ok else f": {detail}"))
    fails += not ok
    ok, detail = assert_mem(luna, ROM, 6_000_000, [("runs_nmi", "0000")])
    ok = not ok                          # the callback ran at least once
    print(("PASS " if ok else "FAIL ") + "the NMI callback ran")
    fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
