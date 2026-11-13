#!/usr/bin/env python3
"""A leaf above 16 words keeps its costliest temps in the direct page and the
rest on the stack: three such leaves are checked against values computed
here, and one runs in the main loop and in the NMI callback, each side
counting its wrong results. Assumes `make` produced leaf_split.sfc (see
main.c for what is at stake)."""
from __future__ import annotations
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402
ROM = HERE / "leaf_split.sfc"
ASM = HERE / "main.c.asm"
M = 0xFFFFFFFF
KTAB = [0x00010003, 0x7FFF0001, 0x0000FFFF, 0x12345678,
        0xFEDCBA98, 0x00008000, 0x80000000, 0x0F0F0F0F,
        11, 22, 33, 44, 55, 66, 77, 88] + [0] * 48


def model(nvars: int, a: int, b: int, n: int) -> list[int]:
    """The C loop of leafN: v[0] += ktab[n & 63], then v[k] += v[k - 1]."""
    v = [a, b] + [(a + k) & M for k in range(2, nvars)]
    while n:
        n -= 1
        v[0] = (v[0] + KTAB[n & 63]) & M
        for k in range(1, nvars):
            v[k] = (v[k] + v[k - 1]) & M
    return v


def hexwords(vals: list[int]) -> str:
    return "".join(x.to_bytes(4, "little").hex().upper() for x in vals)


CASES = [("res17", hexwords(model(4, 0x00010000, 0x0000FFFF, 50))),
         ("res21", hexwords(model(5, 0x7FFFFFFF, 3, 70))),
         ("res25", hexwords(model(6, 0xFFFF0001, 0x00012345, 129))),
         ("want_nmi", hexwords(model(5, 0x00ABCDEF, 0xFFFFFFF0, 37))),
         ("done", "EFBE"),            # the run reached its end
         ("runs_main", "1800"),       # 24 turns of the main loop
         ("runs_nmi", "1800"),        # and the callback ran on each of them
         ("bad_main", "0000"), ("bad_nmi", "0000")]


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    fails = 0
    # the premise: leaf21 and leaf25 are split, part in tcc__lf, part on the
    # stack; leaf17 fits the 20 words of tcc__lf whole since 2026-11-13; leaf21
    # and leaf21n (run by the NMI on its own page) use the words added
    # (+32..+39), so the NMI page's 168 bytes are exercised
    text = ASM.read_text() if ASM.is_file() else ""
    for fn, split in (("leaf17", False), ("leaf21", True), ("leaf25", True), ("leaf21n", True)):
        body = text.split(f"\n{fn}:\n", 1)[-1].split(".ENDS", 1)[0] if f"\n{fn}:\n" in text else ""
        ok = "tcc__lf" in body and ("\ttas" in body) == split
        what = "splits its frame (tcc__lf and a stack frame)" if split else "keeps its whole frame in tcc__lf"
        print(("PASS " if ok else "FAIL ") + f"{fn}() {what}")
        fails += not ok
    for fn in ("leaf21", "leaf21n"):
        body = text.split(f"\n{fn}:\n", 1)[-1].split(".ENDS", 1)[0] if f"\n{fn}:\n" in text else ""
        ok = any(f"tcc__lf+{n}" in body for n in (32, 34, 36, 38))
        print(("PASS " if ok else "FAIL ") + f"{fn}() uses the words added to tcc__lf (+32..+39)")
        fails += not ok
    luna = find_luna()
    for name, want in CASES:
        ok, detail = assert_mem(luna, ROM, 4_000_000, [(name, want)])
        print(("PASS " if ok else "FAIL ") + name + ("" if ok else f": {detail}"))
        fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
