#!/usr/bin/env python3
"""G1 idioms I1 (narrow sign-extension chain), I2 (scaled induction
variable), I3 (loop rotation), I4 (induction reversal) and I5 (the
two-entry loop), compiler/qbe/idiom.c.

Checks, in order:
  1. the ROM `make` produced (the switch off, the default): every value
     against a model of the C (main.c says what each result is), but
     r_i5hi: its first read is at an index whose double is 17 bits, out
     of the C object, and the backend's short form `lda.l big,x` drops
     bit 16 of it (known); it is checked with the switch on;
  2. rebuilt with OPENSNES_G1_IDIOMS=1: the pass says it rewrote every
     function of main.c it should, with the idiom it should
     (OPENSNES_G1_TRACE), and every value is the same;
  3. the provoked faults, each rebuilt with OPENSNES_G1_FAULT=N as well,
     each must turn at least one value of its own idiom red:
     =1 (I1) a logical shift in place of the arithmetic one, so the high
     word is 0 where it should be the sign mask;
     =2 (I2) the entry value scaled at 16 bits, bit 16 of 2*v0 lost when
     v0 >= $8000 (the range assumption the rule refuses);
     =3 (I3) the trip count not looked at: i3_zero's loop, which never
     runs, is rotated and its body runs once;
     =4 (I4) the trip count computed with the signed compares read as
     unsigned: i4_neg's counter from -5 to 5 counts 0 trips;
     =5 (I5) the second form of the peeled first load reads $S, not
     $S+$10000: the carry out of the scaling dropped, r_i5hi red;
  the ROM is rebuilt as `make` builds it at the end (`--no-faults` skips
  parts 2 and 3).
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402

ROM = HERE / "g1_idioms.sfc"
STEPS = 4_000_000
M32 = 0xFFFFFFFF
# (function, idiom) -> rewrites expected when the switch is on
WANT_RW = {("path", "I1"): 1, ("chain", "I1"): 2, ("dom", "I1"): 1,
           **{(f"kf{k}", "I1"): 1 for k in range(1, 16)},
           **{(f"i2_{n}", "I2"): 1
              for n in ("walk", "wide", "free", "sub", "exit", "const")},
           **{(f"i3_{n}", "I3"): 1
              for n in ("count", "idx", "neg", "step", "wrap", "down", "one")},
           ("i3_nest", "I3"): 2, ("main", "I3"): 7,
           ("i3_count", "I4"): 1,
           **{(f"i4_{n}", "I4"): 1
              for n in ("neg", "step", "down", "wrap", "le", "brk", "zero")},
           **{(f"i4_{n}", "I3"): 1
              for n in ("neg", "step", "down", "wrap", "le")},
           **{(f"i5_{n}", "I2"): 1
              for n in ("path", "rom", "do", "wide", "const", "plain")},
           **{(f"i5_{n}", "I4"): 1
              for n in ("path", "rom", "wide", "const", "plain")},
           **{(f"i5_{n}", "I5"): 1 for n in ("path", "rom", "do")}}
# fault -> the values it must turn red (one at least)
FAULT_HITS = {"1": {"r_path3", "r_k", "r_chain", "r_dom"}, "2": {"r_i2"},
              "3": {"r_i3"}, "4": {"r_i4"}, "5": {"r_i5hi"}}
# values the switch-off build is not checked on (see the docstring)
OFF_SKIP = {"r_i5hi"}


def s16(x: int) -> int:
    x &= 0xFFFF
    return x - 0x10000 if x & 0x8000 else x


def mix(acc: int, f: int) -> int:
    return (((acc << 1) | (acc >> 31)) ^ f) & M32


def vals() -> list[int]:
    v = [s16(i * 257 + 0x35) for i in range(256)]
    return v + [-32768, -1, 0, 1, 0x7FFF, -2, -32767, 0x4000]


def le(v: int, n: int) -> str:
    return (v & ((1 << (8 * n)) - 1)).to_bytes(n, "little").hex().upper()


def F(i: int) -> int:
    return s16(i * 0x9E37 + 0x51)


def i2_model() -> list[int]:
    """main.c's r_i2[]: each walk reads F(idx) at the full index (the FAR
    pointer's 24-bit sum b + 2*idx, put() filled exactly the indices read)."""
    def walk(acc, idx, step, n, nxt):
        for _ in range(n):
            acc = mix(acc, F(idx) & M32)
            idx = nxt(idx, step) & 0xFFFF
        return mix(acc, idx)

    def exit_(idx, key, n):
        acc = 0x00070000
        while n:
            s = F(idx)
            if s == s16(key):
                break
            acc = mix(acc, s & M32)
            idx = (idx + 3) & 63
            n -= 1
        return mix(acc, idx) ^ n

    w63 = lambda i, s: (i + s) & 63
    return [
        walk(0x30000, 3, 5, 64, w63),
        walk(0x30000, 2, 0xFFFF, 70, w63),
        walk(0x30000, 0x8003, 5, 20, w63),
        walk(0x30000, 0x8005, 1, 0, w63),
        walk(0x40000, 0x8010, 7, 40, lambda i, s: (i + s) & 0x803F),
        walk(0x50000, 5, 0x8000, 6, lambda i, s: i + s),
        walk(0x50000, 0x8002, 0xFFFF, 18, lambda i, s: i + s),
        walk(0x50000, 0x7FF4, 1, 30, lambda i, s: i + s),
        walk(0x60000, 1, 3, 50, lambda i, s: (i - s) & 63),
        exit_(4, F(19), 64),
        exit_(4, 0x1234, 30),
        walk(0x80000, 7, 9, 40, w63),
        exit_(0x8001, F(0x8001), 10),
    ]


def i3_model() -> list[int]:
    """main.c's r_i3[]: the loops of the C, run as written."""
    def loop(acc, i, cont, step, f=lambda i: i, last=True):
        while cont(i):
            acc = mix(acc, f(i) & M32)
            i = (i + step) & 0xFFFF
        return mix(acc, f(i) & M32) if last else acc

    vs = vals()
    acc = 0x00090000
    for _ in range(64):
        acc = (acc * 3 + 0x1234) & M32
    r = [acc]
    acc = 0x000A0000
    for i in range(10):
        acc = mix(acc, vs[i] & M32)
    r.append(mix(acc, 10))
    r.append(loop(0x000B0000, 0xFFFB, lambda i: s16(i) < 5, 1, s16))
    r.append(loop(0x000C0000, 3, lambda i: i < 100, 7))
    r.append(loop(0x000D0000, 0xFFF0, lambda i: i != 0x10, 1))
    r.append(loop(0x000E0000, 20, lambda i: i > 0, 0xFFFF))
    r.append(loop(0x000F0000, 7, lambda i: i < 8, 1))
    acc = 0x00100000
    for i in range(4):
        for j in range(3):
            acc = mix(acc, i * 16 + j)
    r.append(acc)
    acc = 0x00110000
    i = 0
    while i < 50 and vs[i] != vs[12]:
        acc = mix(acc, i)
        i += 1
    r.append(mix(acc, i))
    r.append(loop(0x00120000, 5, lambda i: i < 3, 1))
    return r


def i4_model() -> list[int]:
    """main.c's r_i4[]: each loop's body run its trip count."""
    def runs(acc, n, f):
        for _ in range(n):
            acc = (acc * 3 + f) & M32
        return acc

    acc = 0x00250000
    for _ in range(40):
        if acc & 0xFF == 0x3A:
            break
        acc = (acc * 3 + 0x1D) & M32
    return [runs(0x00200000, 10, 0x55), runs(0x00210000, 33, 0x66),
            runs(0x00220000, 10, 0x77), runs(0x00230000, 32, 0x88),
            runs(0x00240000, 8, 0x99), acc, runs(0x00260000, 0, 0xAA)]


def i5_model() -> tuple[list[int], list[int]]:
    """main.c's r_i5[] and r_i5hi[]: each loop reads F(idx) at the full
    index (big filled by put(), tab5[i] = F(i)); i5_do returns acc."""
    def walk(acc, idx, step, n, mask=63, last=True):
        for _ in range(n):
            acc = mix(acc, F(idx) & M32)
            idx = (idx + step) & mask
        return mix(acc, idx) if last else acc

    lo = [walk(0x300000, 3, 5, 40), walk(0x300000, 0x7FFF, 1, 40),
          walk(0x310000, 3, 5, 64), walk(0x310000, 60, 0xFFFF, 64),
          walk(0x320000, 7, 5, 30, last=False),
          walk(0x320000, 2, 5, 1, last=False),
          walk(0x330000, 0x10, 7, 20, 0x803F) ^ walk(0x340000, 9, 11, 20)]
    vs = vals()
    acc, idx = 0x350000, 100
    for _ in range(20):
        acc = mix(acc, vs[idx] & M32)
        idx = (idx + 9) & 127
    lo.append(mix(acc, idx))
    hi = [walk(0x300000, 0x8003, 5, 40),
          walk(0x320000, 0x8010, 5, 9, last=False),
          walk(0x320000, 0x803F, 5, 1, last=False)]
    return lo, hi


def model() -> list[tuple[str, str]]:
    vs = vals()
    p0, p3 = 0x10000, 0x20000
    for s in vs:
        p0 = (p0 + s) & M32
        p3 = (p3 + (s >> 1)) & M32
        p3 = mix(p3, p0)
    rk = [0]
    for k in range(1, 16):
        acc = 0
        for s in vs:
            acc = mix(acc, (s & M32) ^ (((s >> k) & M32) << 1 & M32))
        rk.append(acc)
    acc = 0
    for s in vs:
        a = s >> 1
        b = a >> 3
        acc = mix(acc, (s + a * 16 - b * 512) & M32)
    r_chain = acc
    acc = 0
    for s in vs[::7]:
        d = s & M32
        for _ in range(3):
            d = mix(d, (s >> 2) & M32)
            d = (d + s) & M32
        acc = mix(acc, d)
    return [
        ("r_path0", le(p0, 4)),
        ("r_path3", le(p3, 4)),
        ("r_k", "".join(le(x, 4) for x in rk)),
        ("r_chain", le(r_chain, 4)),
        ("r_dom", le(acc, 4)),
        ("r_i2", "".join(le(x, 4) for x in i2_model())),
        ("r_i3", "".join(le(x, 4) for x in i3_model())),
        ("r_i4", "".join(le(x, 4) for x in i4_model())),
        ("r_i5", "".join(le(x, 4) for x in i5_model()[0])),
        ("r_i5hi", "".join(le(x, 4) for x in i5_model()[1])),
        ("done", "EFBE"),
    ]


def values(luna: str, skip=frozenset()) -> dict[str, bool]:
    return {name: assert_mem(luna, ROM, STEPS, [(name, want)])[0]
            for name, want in model() if name not in skip}


def build(env_extra: dict[str, str]) -> str:
    env = {k: v for k, v in os.environ.items() if not k.startswith("OPENSNES_G1")}
    env.update(env_extra)
    subprocess.run(["make", "-s", "-C", str(HERE), "clean"], env=env, check=True,
                   stdout=subprocess.DEVNULL)
    p = subprocess.run(["make", "-s", "-C", str(HERE)], env=env, check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    return p.stderr


def rewrites(trace: str) -> dict[tuple[str, str], int]:
    """`g1idioms: <fn>: <idiom> rewrote <n>` lines"""
    got = {}
    for line in trace.splitlines():
        if line.startswith("g1idioms: ") and " rewrote " in line:
            fn, rest = line[len("g1idioms: "):].split(": ", 1)
            idiom, _, n = rest.split(" ")
            got[(fn, idiom)] = int(n)
    return got


def report(res: dict[str, bool], tag: str) -> int:
    fails = 0
    for name, ok in res.items():
        print(("PASS " if ok else "FAIL ") + f"{tag}{name}")
        fails += not ok
    return fails


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    fails = report(values(luna, OFF_SKIP), "switch off: ")
    for name in sorted(OFF_SKIP):
        print(f"SKIP switch off: {name} (checked with the switch on)")
    if "--no-faults" in sys.argv or fails:
        return 1 if fails else 0
    try:
        got = rewrites(build({"OPENSNES_G1_IDIOMS": "1", "OPENSNES_G1_TRACE": "1"}))
        ok = got == WANT_RW
        n1 = sum(v for (_, i), v in got.items() if i == "I1")
        n2 = sum(v for (_, i), v in got.items() if i == "I2")
        n3 = sum(v for (_, i), v in got.items() if i == "I3")
        n4 = sum(v for (_, i), v in got.items() if i == "I4")
        n5 = sum(v for (_, i), v in got.items() if i == "I5")
        print(("PASS " if ok else "FAIL ")
              + f"I1, I2, I3, I4, I5 rewrite every function they should"
              f" ({n1} + {n2} + {n3} + {n4} + {n5} sites)")
        if not ok:
            print(f"  got {sorted(got.items())}")
        fails += not ok
        fails += report(values(luna), "switch on: ")
        for fault, hits in FAULT_HITS.items():
            build({"OPENSNES_G1_IDIOMS": "1", "OPENSNES_G1_FAULT": fault})
            res = values(luna)
            red = {n for n, v in res.items() if not v}
            seen = bool(red & hits)
            print(("PASS " if seen else "FAIL ")
                  + f"OPENSNES_G1_FAULT={fault} turns values red ({', '.join(sorted(red))})")
            fails += not seen
    finally:
        build({})                       # the ROM `make` builds, back
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
