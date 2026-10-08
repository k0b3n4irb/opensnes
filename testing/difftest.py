#!/usr/bin/env python3
"""difftest.py — differential test of the C compiler on integer expressions.

Random C integer expressions, each compiled by cc65816 and executed on luna
in four shapes, and compared with the value C gives them:

  v  operands read from initialised globals
  c  operands written as literals (the compiler folds them at build time)
  f  operands received as function parameters (the calling convention)
  l  operands copied to locals first (stack slots)

The expected value comes from a model of C's integer rules for this target
(char 8, short and int 16, long 32: promotions, usual arithmetic
conversions, wrapping) written below. The model is itself checked: every
literal shape is given to clang as `_Static_assert(expr == value)` under
`--target=avr`, whose int and long have the same widths — a model mistake
is reported as such and never as a compiler defect. Expressions are built
free of undefined behaviour (signed overflow, division by zero, shift
counts out of range): where one would occur the generator inserts a cast
or a mask and evaluates again.

    python3 testing/difftest.py                    # the gate: pinned expressions and fixed seeds
    python3 testing/difftest.py --seeds 1000-1063  # hunt with other seeds
    python3 testing/difftest.py --seeds 7 --keep   # keep build/difftest/seed_7
    python3 testing/difftest.py --cc /tmp/old/cc65816   # another compiler (bisecting)

A failing expression is reduced: its sub-expressions are run as tests of
their own and the smallest failing one is printed with its C source, ready
for a file under devtools/compiler-tests/cases/.

Exit 0 when every result matches, 1 on a mismatch, 2 when the model and
clang disagree or a ROM does not build or finish.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import random
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
sys.path.insert(0, str(HERE))
from lib import find_luna  # noqa: E402

WORK = REPO / "build" / "difftest"
# The gate: the pinned expressions, eight seeds, and the seeds that showed
# each defect of 2026-10-08 first (2, 8, 57, 116, 126, 134, 829, 5485, 5728).
GATE_SEEDS = ["pinned"] + list(range(1, 9)) + [57, 116, 126, 134, 829, 5485, 5728]
TESTS_PER_ROM = 24
SHAPES = "vcfl"
DONE_MAGIC = 0x600D
BUILD_TIMEOUT = 120
MAKE_ARGS: list = []      # --cc: another cc65816 (a build of an older compiler, to bisect)

# ---------------------------------------------------------------- C model --

BITS = {"s8": 8, "u8": 8, "s16": 16, "u16": 16, "s32": 32, "u32": 32}
SIGNED = {"s8", "s16", "s32"}
CTYPE = {"s8": "signed char", "u8": "unsigned char", "s16": "short",
         "u16": "unsigned short", "s32": "long", "u32": "unsigned long"}
TYPES = list(BITS)


def wrap(v: int, t: str) -> int:
    """v converted to type t (modulo; two's complement for signed)."""
    b = BITS[t]
    v &= (1 << b) - 1
    if t in SIGNED and v >> (b - 1):
        v -= 1 << b
    return v


def fits(v: int, t: str) -> bool:
    return wrap(v, t) == v


def promote(t: str) -> str:
    return "s16" if BITS[t] < 16 else t


def unsigned_of(t: str) -> str:
    return {"s16": "u16", "s32": "u32"}.get(t, t)


def common(a: str, b: str) -> str:
    """Usual arithmetic conversions with a 16-bit int and a 32-bit long."""
    a, b = promote(a), promote(b)
    if a == b:
        return a
    if "u32" in (a, b):
        return "u32"
    if "s32" in (a, b):
        return "s32"          # long holds every unsigned int value
    return "u16"              # s16 with u16


class Node:
    """An expression: kind, type, value, children; a leaf has an index."""

    def __init__(self, kind, typ, value, kids=(), op="", leaf=-1):
        self.kind, self.type, self.value = kind, typ, value
        self.kids, self.op, self.leaf = tuple(kids), op, leaf

    def size(self) -> int:
        return 1 + sum(k.size() for k in self.kids)

    def subtrees(self):
        yield self
        for k in self.kids:
            yield from k.subtrees()

    def leaves(self):
        if self.kind == "leaf":
            yield self
        for k in self.kids:
            yield from k.leaves()


def literal(t: str, v: int) -> str:
    """A C literal of exactly type t on a 16-bit-int target."""
    if t == "s16":
        return "(-32767 - 1)" if v == -32768 else (f"({v})" if v < 0 else f"{v}")
    if t == "u16":
        return f"{v}U"
    if t == "s32":
        return "(-2147483647L - 1)" if v == -2147483648 else (f"({v}L)" if v < 0 else f"{v}L")
    if t == "u32":
        return f"{v}UL"
    return f"(({t}){v})"


def const(v: int) -> Node:
    return Node("const", "s16", v)


def cast(t: str, e: Node) -> Node:
    return Node("cast", t, wrap(e.value, t), [e], op=t)


def unary(op: str, e: Node) -> Node:
    if op == "!":
        return Node("un", "s16", int(e.value == 0), [e], op)
    t = promote(e.type)
    v = wrap(e.value, t)
    r = ~v if op == "~" else -v
    if t in SIGNED and not fits(r, t):          # -INT_MIN
        return unary(op, cast(unsigned_of(t), e))
    return Node("un", t, wrap(r, t), [e], op)


def binary(op: str, l: Node, r: Node) -> Node:
    if op in ("&&", "||"):
        v = (l.value != 0 and r.value != 0) if op == "&&" else (l.value != 0 or r.value != 0)
        return Node("bin", "s16", int(v), [l, r], op)
    if op in ("<<", ">>"):
        t = promote(l.type)
        n = wrap(r.value, promote(r.type))
        if n < 0 or n >= BITS[t]:
            return binary(op, l, binary("&", r, const(BITS[t] - 1)))
        a = wrap(l.value, t)
        if op == ">>":
            return Node("bin", t, a >> n, [l, r], op)
        if t in SIGNED and (a < 0 or not fits(a << n, t)):
            return binary(op, cast(unsigned_of(t), l), r)
        return Node("bin", t, wrap(a << n, t), [l, r], op)
    t = common(l.type, r.type)
    a, b = wrap(l.value, t), wrap(r.value, t)
    if op in ("==", "!=", "<", "<=", ">", ">="):
        v = {"==": a == b, "!=": a != b, "<": a < b, "<=": a <= b, ">": a > b, ">=": a >= b}[op]
        return Node("bin", "s16", int(v), [l, r], op)
    if op in ("/", "%"):
        if b == 0:
            return binary(op, l, binary("|", r, const(1)))
        q = abs(a) // abs(b)
        if (a < 0) != (b < 0):
            q = -q
        if t in SIGNED and not fits(q, t):      # INT_MIN / -1
            return binary(op, cast(unsigned_of(t), l), r)
        return Node("bin", t, wrap(q if op == "/" else a - q * b, t), [l, r], op)
    v = {"+": a + b, "-": a - b, "*": a * b, "&": a & b, "|": a | b, "^": a ^ b}[op]
    if t in SIGNED and not fits(v, t):
        return binary(op, cast(unsigned_of(t), l), r)
    return Node("bin", t, wrap(v, t), [l, r], op)


def ternary(c: Node, a: Node, b: Node) -> Node:
    t = common(a.type, b.type)
    return Node("tern", t, wrap(a.value if c.value else b.value, t), [c, a, b])


# -------------------------------------------------------------- generator --

BIN_OPS = ["+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>",
           "==", "!=", "<", "<=", ">", ">=", "&&", "||"]


def interesting(rng: random.Random, t: str) -> int:
    b = BITS[t]
    pick = rng.random()
    if pick < 0.45:
        edge = [0, 1, 2, 3, 7, 8, 15, 16, 127, 128, 255, 256, (1 << (b - 1)) - 1,
                1 << (b - 1), (1 << b) - 1, (1 << b) - 2, 0x55 << (b - 8), 0xAA << (b - 8)]
        return wrap(rng.choice(edge), t)
    if pick < 0.6:
        return wrap(1 << rng.randrange(b), t)
    if pick < 0.75:
        return wrap(rng.randrange(-9, 10), t)
    return wrap(rng.getrandbits(b), t)


def gen(rng: random.Random, depth: int, leaves: list) -> Node:
    if depth == 0 or (depth < 3 and rng.random() < 0.25) or len(leaves) >= 6:
        if leaves and (len(leaves) >= 6 or rng.random() < 0.15):
            return rng.choice(leaves)                  # reuse an operand
        t = rng.choice(TYPES)
        n = Node("leaf", t, interesting(rng, t), leaf=len(leaves))
        leaves.append(n)
        return n
    k = rng.random()
    if k < 0.12:
        return cast(rng.choice(TYPES), gen(rng, depth - 1, leaves))
    if k < 0.22:
        return unary(rng.choice("-~!"), gen(rng, depth - 1, leaves))
    if k < 0.30:
        return ternary(gen(rng, depth - 1, leaves), gen(rng, depth - 1, leaves),
                       gen(rng, depth - 1, leaves))
    l = gen(rng, depth - 1, leaves)
    r = gen(rng, depth - 1, leaves)
    return binary(rng.choice(BIN_OPS), l, r)


def render(e: Node, shape: str, tid: int) -> str:
    if e.kind == "const":
        return str(e.value)
    if e.kind == "leaf":
        if shape == "c":
            return literal(e.type, e.value)
        return {"v": f"g{tid}_{e.leaf}", "f": f"p{e.leaf}", "l": f"a{e.leaf}"}[shape]
    k = [render(x, shape, tid) for x in e.kids]
    if e.kind == "cast":
        return f"(({e.op})({k[0]}))"
    if e.kind == "un":
        return f"({e.op}({k[0]}))"
    if e.kind == "tern":
        return f"(({k[0]}) ? ({k[1]}) : ({k[2]}))"
    return f"(({k[0]}) {e.op} ({k[1]}))"


class Test:
    def __init__(self, expr: Node, label: str):
        self.expr, self.label = expr, label
        self.leaves = sorted({id(n): n for n in expr.leaves()}.values(), key=lambda n: n.leaf)
        self.expected = wrap(expr.value, "u32")

    def source(self, tid: int) -> str:
        """The four functions of this test, and its globals."""
        out = [f"/* {self.label}: expect 0x{self.expected:08X} ({self.expr.type}) */"]
        for n in self.leaves:
            out.append(f"{n.type} g{tid}_{n.leaf} = {n.value};")
        params = ", ".join(f"{n.type} p{n.leaf}" for n in self.leaves) or "void"
        locs = " ".join(f"{n.type} a{n.leaf} = g{tid}_{n.leaf};" for n in self.leaves)
        out.append(f"u32 v{tid}(void) {{ return (u32)({render(self.expr, 'v', tid)}); }}")
        out.append(f"u32 c{tid}(void) {{ return (u32)({render(self.expr, 'c', tid)}); }}")
        out.append(f"u32 f{tid}({params}) {{ return (u32)({render(self.expr, 'f', tid)}); }}")
        out.append(f"u32 l{tid}(void) {{ {locs} return (u32)({render(self.expr, 'l', tid)}); }}")
        return "\n".join(out)

    def calls(self, tid: int) -> str:
        args = ", ".join(f"g{tid}_{n.leaf}" for n in self.leaves)
        b = tid * len(SHAPES)
        return (f"    r[{b}] = v{tid}(); r[{b + 1}] = c{tid}(); "
                f"r[{b + 2}] = f{tid}({args}); r[{b + 3}] = l{tid}();")


def pinned_tests() -> list:
    """The expressions that found a compiler defect, reduced (2026-10-08).

    Kept as trees so that they run in the four shapes and go through the
    clang check like the random ones."""
    def pin(label, build):
        leaves: list = []

        def L(t, v):
            n = Node("leaf", t, wrap(v, t), leaf=len(leaves))
            leaves.append(n)
            return n
        return Test(build(L), "pinned: " + label)

    def shifted(L):
        x, y = L("s32", -2147483648), L("s32", 544398009)
        return ternary(binary(">>", cast("s16", x), binary(">", y, y)), L("s16", 1), L("s16", 2))

    return [
        # QBE folded Kl at 64 bits: the negation is -4294967291 there, 5 in 32 bits
        pin("a folded 32-bit negation, compared",
            lambda L: binary("==", unary("-", L("u32", 4294967291)), L("s16", 5))),
        # ... and left a 16-bit product uncut: 0xAA0000 is 0 in 16 bits
        pin("a 16-bit product that wraps to 0, as a condition",
            lambda L: binary("&&", binary("*", L("s16", -22016), L("u16", 256)), L("s16", 1))),
        pin("a 32-bit shift that wraps to 0, as a condition",
            lambda L: binary("&&", binary("<<", L("u32", 512), L("s16", 31)), L("u16", 512))),
        # cproc compared the right operand of && with a 16-bit compare
        pin("a long whose low word is 0, right of &&",
            lambda L: binary("&&", L("u32", 1), L("u32", 0x02A80000))),
        pin("the same, right of ||",
            lambda L: binary("||", L("u8", 0), L("s32", 0x00010000))),
        # jnz read 32 bits of a temp that held a 16-bit condition
        pin("(s16) of a long whose low word is 0, as a condition",
            lambda L: ternary(cast("s16", L("s32", 0x00100000)), L("s16", 1), L("s16", 2))),
        pin("the same through a shift by a comparison that folds to 0", shifted),
        pin("(u16) of a long constant expression, as a condition",
            lambda L: binary("&&", cast("u16", binary("-", L("s32", -2147483647), L("s16", 1))),
                             L("s8", -6))),
        # cproc's evaluator returned an operand of || and && instead of 0 or 1
        pin("|| of two constants is 1, not its left operand",
            lambda L: ternary(binary("-", binary("||", L("u16", 43732), L("u32", 642511729)),
                                     binary("<", L("u8", 15), L("u32", 642511729))),
                              L("s16", 7), L("s16", 0))),
        pin("&& of two constants is 1, not its right operand",
            lambda L: ternary(binary("-", binary("&&", L("s16", 5), L("s16", 7)), L("s16", 1)),
                              L("s16", 7), L("s16", 0))),
    ]
    # Two more defects of that day have no pin here and are held by their
    # seed in GATE_SEEDS: a function left without a stack frame (seed 8) and
    # a qbe that never returned on a 31-bit constant multiply (seed 5728).


def make_tests(seed: int) -> list:
    rng = random.Random(seed)
    tests = []
    for i in range(TESTS_PER_ROM):
        leaves: list = []
        tests.append(Test(gen(rng, rng.choice([2, 3, 3, 4]), leaves), f"seed {seed} test {i}"))
    return tests


# ---------------------------------------------------------- model vs clang --

def check_model(tests: list) -> list:
    """Return the labels clang (16-bit int target) refuses; None = no clang."""
    if not shutil.which("clang"):
        return None
    src = ["typedef signed char s8; typedef unsigned char u8; typedef short s16;",
           "typedef unsigned short u16; typedef long s32; typedef unsigned long u32;",
           "_Static_assert(sizeof(int) == 2 && sizeof(long) == 4, \"widths\");"]
    for i, t in enumerate(tests):
        src.append(f"_Static_assert((u32)({render(t.expr, 'c', 0)}) == {t.expected}UL, \"T{i}\");")
    proc = subprocess.run(["clang", "--target=avr", "-fsyntax-only", "-w", "-x", "c", "-"],
                          input="\n".join(src), capture_output=True, text=True)
    if proc.returncode == 0:
        return []
    if "unknown target" in proc.stderr or "widths" in proc.stderr:
        return None
    import re
    bad = [tests[int(m)].label for m in re.findall(r'"T(\d+)"', proc.stderr)]
    return bad or ["(clang failed without naming a test)\n" + proc.stderr[:2000]]


# ----------------------------------------------------------- build and run --

MAKEFILE = """OPENSNES := {repo}
TARGET   := difftest.sfc
ROM_NAME := DIFFTEST
USE_LIB  := 1
LIB_MODULES := console sprite dma background
BANK0_FAIL_THRESHOLD := 0
# Generated expressions trip style warnings by design (a shift used as a
# condition, a comparison that is always true): the lint is for people.
SKIP_LINT := 1
CSRC := main.c
include $(OPENSNES)/make/common.mk
"""


def write_project(tests: list, name: str) -> Path:
    d = WORK / name
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    n = len(tests) * len(SHAPES)
    body = ["#include <snes.h>", "", f"u32 r[{n}];", "u16 done;", ""]
    body += [t.source(i) + "\n" for i, t in enumerate(tests)]
    body += ["int main(void) {", "    consoleInit();"]
    body += [t.calls(i) for i, t in enumerate(tests)]
    body += [f"    done = 0x{DONE_MAGIC:04X};", "    while (1) { WaitForVBlank(); }", "    return 0;", "}", ""]
    (d / "main.c").write_text("\n".join(body))
    (d / "Makefile").write_text(MAKEFILE.format(repo=REPO))
    return d


def run_batch(tests: list, name: str, luna: str):
    """Build and run one ROM. Returns (results per test per shape) or an error string."""
    d = write_project(tests, name)
    env = {k: v for k, v in os.environ.items() if k not in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL")}
    try:
        proc = subprocess.run(["make", "-s", "-C", str(d)] + MAKE_ARGS, capture_output=True, text=True,
                              env=env, timeout=BUILD_TIMEOUT)
    except subprocess.TimeoutExpired:
        return f"the build of {d} did not end in {BUILD_TIMEOUT} s (a compiler that loops?)"
    rom = d / "difftest.sfc"
    if proc.returncode != 0 or not rom.is_file():
        return f"build failed in {d}:\n" + (proc.stdout + proc.stderr)[-1500:]
    n = len(tests) * len(SHAPES)
    steps = 2_000_000
    while True:
        out = subprocess.run([luna, "state", "-n", str(steps), "--peek", f"r:{n * 4:x}",
                              "--peek", "done:2", "--out", "-", str(rom)],
                             capture_output=True, text=True)
        try:
            peeks = json.loads(out.stdout)["peeks"]
        except (ValueError, KeyError):
            return f"luna gave no state for {rom}:\n{out.stderr[-800:]}"
        if int.from_bytes(bytes.fromhex(peeks[1]["bytes_hex"]), "little") == DONE_MAGIC:
            break
        if steps >= 64_000_000:
            return f"{rom} never reached the end of main (done != 0x{DONE_MAGIC:04X})"
        steps *= 4
    raw = bytes.fromhex(peeks[0]["bytes_hex"])
    vals = [int.from_bytes(raw[i:i + 4], "little") for i in range(0, n * 4, 4)]
    return [vals[i:i + len(SHAPES)] for i in range(0, n, len(SHAPES))]


def reduce_failure(test: Test, shapes: str, luna: str, name: str) -> str:
    """Run every sub-expression as its own test; describe the smallest failing one."""
    subs, seen = [], set()
    for s in sorted(test.expr.subtrees(), key=lambda e: e.size()):
        if s.kind in ("leaf", "const"):
            continue
        key = render(s, "c", 0)
        if key not in seen:
            seen.add(key)
            subs.append(Test(s, f"{test.label} (sub-expression)"))
    for i in range(0, len(subs), TESTS_PER_ROM):
        chunk = subs[i:i + TESTS_PER_ROM]
        res = run_batch(chunk, f"{name}_reduce{i // TESTS_PER_ROM}", luna)
        if isinstance(res, str):
            return "  (reduction failed: " + res.splitlines()[0] + ")"
        for t, got in zip(chunk, res):
            bad = [SHAPES[k] for k, g in enumerate(got) if g != t.expected]
            if bad:
                lines = [f"  smallest failing sub-expression ({t.expr.size()} nodes, shapes {''.join(bad)}):"]
                lines += ["    " + ln for ln in t.source(0).splitlines()]
                lines.append("    got: " + ", ".join(f"{SHAPES[k]}=0x{g:08X}" for k, g in enumerate(got)))
                return "\n".join(lines)
    return "  (no sub-expression fails alone: the whole expression is the reproducer)"


def run_seed(seed, luna: str, keep: bool):
    """One ROM: the random tests of a seed, or the pinned ones (seed "pinned")."""
    tests = pinned_tests() if seed == "pinned" else make_tests(seed)
    name = f"seed_{seed}"
    model = check_model(tests)
    if model:
        return seed, 2, [f"seed {seed}: the model and clang disagree on {', '.join(model)}"], model is None
    res = run_batch(tests, name, luna)
    if isinstance(res, str):
        return seed, 2, [f"seed {seed}: {res}"], model is None
    msgs = []
    for i, (t, got) in enumerate(zip(tests, res)):
        bad = "".join(SHAPES[k] for k, g in enumerate(got) if g != t.expected)
        if bad:
            msgs.append(f"MISMATCH {t.label}, shapes {bad}: expected 0x{t.expected:08X}, got "
                        + ", ".join(f"{SHAPES[k]}=0x{g:08X}" for k, g in enumerate(got)))
            msgs += ["    " + ln for ln in t.source(i).splitlines()]
            msgs.append(reduce_failure(t, bad, luna, name))
    if not msgs and not keep:
        shutil.rmtree(WORK / name, ignore_errors=True)
    return seed, (1 if msgs else 0), msgs, model is None


def parse_seeds(spec: str) -> list:
    out = []
    for part in spec.split(","):
        if part == "pinned":
            out.append(part)
        elif "-" in part:
            a, b = part.split("-", 1)
            out += range(int(a), int(b) + 1)
        else:
            out.append(int(part))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--seeds", help="e.g. 5 or 1,2,9 or 100-163 or pinned (default: the gate)")
    ap.add_argument("--keep", action="store_true", help="keep the generated projects of passing seeds")
    ap.add_argument("--jobs", type=int, default=int(os.environ.get("LUNA_JOBS", os.cpu_count() or 1)))
    ap.add_argument("--cc", help="path of another cc65816 to test (e.g. one built before a change)")
    args = ap.parse_args()
    if args.cc:
        MAKE_ARGS.append(f"CC={Path(args.cc).resolve()}")
    seeds = parse_seeds(args.seeds) if args.seeds else GATE_SEEDS
    luna = find_luna()
    worst, no_clang, failed = 0, False, 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        for seed, code, msgs, unchecked in pool.map(lambda s: run_seed(s, luna, args.keep), seeds):
            worst, no_clang = max(worst, code), no_clang or unchecked
            failed += code != 0
            for m in msgs:
                print(m)
    total = sum(len(pinned_tests()) if x == "pinned" else TESTS_PER_ROM for x in seeds)
    note = " (model NOT checked: no clang with an avr target here)" if no_clang else ""
    print(f"difftest: {len(seeds) - failed}/{len(seeds)} seeds clean, {total} expressions "
          f"in {len(SHAPES)} shapes{note}")
    return worst


if __name__ == "__main__":
    sys.exit(main())
