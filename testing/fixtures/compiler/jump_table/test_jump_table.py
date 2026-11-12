#!/usr/bin/env python3
"""A dense switch is a jump table (L2): every shape main.c holds, taken or
refused, swept over a range of values and folded into a checksum, against
the same C computed here; one sweep also runs in the NMI callback against
the main loop. Assumes `make` produced jump_table.sfc (see main.c)."""
from __future__ import annotations
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402
ROM = HERE / "jump_table.sfc"
ASM = HERE / "main.c.asm"
W = 0xFFFF


def s16(x):
    x &= W
    return x - 0x10000 if x & 0x8000 else x


class M:
    g = 0
    st = 0


def dense(v):
    return 11 * (v + 1) if v < 8 else 99


def based(v):
    if 10 <= v <= 14:
        M.g = v - 9
    return M.g


def negative(v):                 # v signed
    return {-3: 1, -2: 2, -1: 3, 0: 4, 1: 5, 2: 6}.get(v, 0)


def negu(v):
    return {0xFFFD: 1, 0xFFFE: 2, 0xFFFF: 3, 0: 4, 1: 5}.get(v, 0)


def bytes_(v):
    M.st = v & 0xFF
    M.st = {0: 3, 1: 0, 2: 5, 3: 1, 4: 2}.get(M.st, M.st)
    return M.st


def setter(v, r):
    r = {1: 10, 2: 20, 4: 40, 5: 50, 6: 60}.get(v, r)
    return (r + v) & W


def mixed(v):
    if v in (2, 3, 4):
        return 6
    return {6: 1, 8: 9, 9: 2}.get(v, 100)


def after(v):
    r = {3: 1, 4: 2, 5: 3, 6: 4, 7: 5}.get(v, 6)
    return (r * 100 + v) & W


def few(v):
    return {0: 5, 1: 6, 2: 7}.get(v, 0)


def sparse(v):
    return {0: 1, 100: 2, 200: 3, 300: 4, 400: 5}.get(v, 0)


def wide(v):
    return {0: 1, 1: 2, 2: 3, 3: 4, 70000: 5, 5: 6}.get(v, 0)


def cluster(v):
    if v < 8:
        return v + 1
    return {1000: 9, 2000: 10}.get(v, 0)


def inloop(seed):
    tab = [(seed + i * 3) & 7 for i in range(8)]
    for i in range(8):
        t = tab[i]
        tab[i] = {0: 5, 1: 9, 2: 1, 3: 0, 5: t + i}.get(t, 2)
    s = 0
    for i in range(8):
        s = (s * 3 + tab[i]) & W
    return s


def counted(n):
    s = 0
    for i in range(n):
        c = i & 7
        if c == 0:
            s += 1
        elif c == 1:
            s += 10
        elif c == 2:
            s ^= 0x55
        elif c == 3:
            s <<= 1
        elif c == 4:
            s -= 3
        else:
            s += i
        s &= W
    return s


def fold(s, x):
    return (s * 31 + x) & W


FAR_VALS = [2000, 1999, 2001, 32767, -32768, -1, -2, -3, -4, 70]


def sweep(which):
    s = 0
    for v in range(-20, 1101):
        u = v & W
        x = None
        if which == 0: x = dense(u)
        elif which == 1: x = based(u)
        elif which == 2: x = negative(v)
        elif which == 3: x = negu(u)
        elif which == 4: x = bytes_(v)
        elif which == 5: x = setter(u, (v * 7) & W)
        elif which == 6: x = mixed(u)
        elif which == 7: x = after(u)
        elif which == 8: x = few(u)
        elif which == 9: x = sparse(u)
        elif which == 10: x = wide(v & 0xFFFFFFFF)
        elif which == 11: x = cluster(u)
        elif which == 12:
            if v & 63 == 0: x = inloop(u)
        elif which == 13:
            if v & 63 == 0: x = counted(u & 127)
        else: x = wide((70000 + v) & 0xFFFFFFFF)
        if x is not None:
            s = fold(s, x)
    for v in FAR_VALS:
        u = v & W
        s = fold(s, (dense(u) + negative(v) + negu(u) + cluster(u)) & W)
    return s


def sweep_small():
    s = 0
    for v in range(-3, 10):
        u = v & W
        s = fold(s, dense(u))
        s = fold(s, negative(v))
        s = fold(s, mixed(u))
    return fold(s, counted(9))


def hexw(x):
    return x.to_bytes(2, "little").hex().upper()


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    fails = 0
    # the premise: the dense switches are tables, the others trees
    text = ASM.read_text() if ASM.is_file() else ""
    def body(fn):
        return text.split(f"\n{fn}:\n", 1)[-1].split(".ENDS", 1)[0] if f"\n{fn}:\n" in text else ""
    for fn, want in [("dense", True), ("based", True), ("negative", True), ("negu", True),
                     ("bytes", True), ("setter", True), ("mixed", True), ("after", True),
                     ("inloop", True), ("counted", True), ("few", False), ("sparse", False),
                     ("wide", False)]:
        ok = ("jmp (@jtt." in body(fn)) == want
        print(("PASS " if ok else "FAIL ") + f"{fn}() {'is' if want else 'is not'} a jump table")
        fails += not ok
    M.g = M.st = 0
    want = [sweep(k) for k in range(15)]
    cases = [("res", "".join(hexw(x) for x in want)),
             ("want_nmi", hexw(sweep_small())),
             ("done", "EFBE"),
             ("runs_main", "1000"),       # 16 turns of the main loop
             ("runs_nmi", "1000"),        # and 16 runs of the callback
             ("bad_main", "0000"), ("bad_nmi", "0000")]
    # one run to the end (the tree twin, QBE_NO_JUMP_TABLE=1, needs more
    # than 30 million steps): every value read at that point
    luna = find_luna()
    ok, detail = assert_mem(luna, ROM, 80_000_000, cases)
    for name, w in cases:
        bad = f"${name}" in detail or f" {name} " in detail
        print(("FAIL " if not ok and bad else "PASS ") + name)
    if not ok:
        print(detail)
        fails += 1
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
