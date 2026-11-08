#!/usr/bin/env python3
"""A divide or remainder of two constants is folded: fold16() and fold32()
call no division helper and every result equals the value written by hand;
edges() keeps INT_MIN / -1, INT_MIN % -1 (both widths) and a zero divisor as
run-time calls, and the helpers' INT_MIN results are checked. Assumes `make`
produced const_div.sfc (see main.c for what is at stake)."""
from __future__ import annotations
import re
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402
ROM = HERE / "const_div.sfc"
ASM = HERE / "main.c.asm"
HELPER = re.compile(r"\tjsl (__s?div16|__s?mod16|tcc_[us]divmod32)\b")


def le(vals: list[int], size: int) -> str:
    """Little-endian hex of each value, two's complement at `size` bytes."""
    mask = (1 << (8 * size)) - 1
    return "".join((v & mask).to_bytes(size, "little").hex().upper() for v in vals)


# The expected values, written by hand (main.c gives each one beside its
# expression), not computed by Python's operators: Python floors a signed
# division where C truncates it.
CASES = [
    ("ures", le([5, 1471, 1, 21845, 255, 5714, 2, 7], 2)),
    ("sres", le([-3, -1, -3, 1, 3, -1, -5, -1471, -16384, -2, -32767, -32768], 2)),
    ("lres", le([14285, 5, 0x55555555, 5], 4)),
    ("slres", le([-14285, -5, -14285, -2147483647, -1073741824, -2], 4)),
    ("edge16", le([-32768, 0], 2)),
    ("edge32", le([-2147483648, 0], 4)),
    ("done", "EFBE"),
]

# function -> the helper calls it must contain, in order (empty: none)
CALLS = {
    "fold16": [],
    "fold32": [],
    "edges": ["__sdiv16", "__smod16", "tcc_sdivmod32", "tcc_sdivmod32", "__div16"],
}


def body(text: str, fn: str) -> str | None:
    head = f"\n{fn}:\n"
    if head not in text:
        return None
    return text.split(head, 1)[1].split(".ENDS", 1)[0]


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    fails = 0
    text = ASM.read_text() if ASM.is_file() else ""
    for fn, want in CALLS.items():
        b = body(text, fn)
        got = HELPER.findall(b) if b is not None else None
        ok = got == want
        print(("PASS " if ok else "FAIL ") + f"{fn}() division helpers {want}"
              + ("" if ok else f": got {got}"))
        fails += not ok
    luna = find_luna()
    for name, want in CASES:
        ok, detail = assert_mem(luna, ROM, 1_000_000, [(name, want)])
        print(("PASS " if ok else "FAIL ") + name + ("" if ok else f": {detail}"))
        fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
