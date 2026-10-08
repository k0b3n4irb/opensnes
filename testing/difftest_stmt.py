#!/usr/bin/env python3
"""difftest_stmt.py — differential test of the C compiler on small programs.

The statement side of testing/difftest.py: where that one compares single
integer expressions, this one generates small functions — locals and
parameters, global arrays, a struct reached by name, through a pointer and
in an array, counted loops in four spellings, a pointer walking an array,
if / else, compound assignments, ++ and -- — compiles them with cc65816,
runs them on luna and compares a checksum of everything the function left
behind (its variables, the arrays, the struct fields) with the value an
interpreter in this script computes.

The interpreter evaluates every expression with difftest.py's model of C's
integer rules, the one clang checks under a 16-bit-int target, so the
arithmetic is the checked one; what is new here is control flow and memory,
kept simple enough to read: assignment converts to the left type, a counted
loop runs its count, an index is masked to the array's size. A program
whose run would meet undefined behaviour (signed overflow, a shift count
out of range) is thrown away and another is drawn.

    python3 testing/difftest_stmt.py                    # the gate: pinned functions and fixed seeds
    python3 testing/difftest_stmt.py --seeds 100-499    # hunt
    python3 testing/difftest_stmt.py --seeds 7 --keep   # keep build/difftest/stmt_7
    python3 testing/difftest_stmt.py --cc /tmp/old/cc65816

A failing program is reduced: statements are removed one at a time for as
long as the result still differs, and what is left is printed.

Exit 0 when every checksum matches, 1 on a mismatch, 2 when a ROM does not
build or finish.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import copy
import json
import os
import random
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import difftest as D  # noqa: E402
from lib import find_luna  # noqa: E402

# The gate: the pinned functions below, twelve seeds (5, 8, 10 and 12 stopped
# the compiler on `a[x & 7]` with a long x), and the seeds that showed the
# two other defects of 2026-10-08 first (51, 2630).
GATE_SEEDS = ["pinned"] + list(range(1, 13)) + [51, 2630]

# Hand-written functions for the defects this test found, reduced. Fixed-width
# types and no operation that depends on the width of int, so the expected
# values are the ones a host compiler gives (checked 2026-10-08).
PINNED_GLOBALS = """s8 pin_a[8] = { 7, 3, 5, 3, -2, 1, -86, 4 };
u32 pin_g = 1;
s32 pin_x = 0x00050003;
u8 pin_t[8] = { 10, 11, 12, 13, 14, 15, 16, 17 };
"""
PINNED = [
    # phi moves were emitted one after the other: `prev` read the new `cur`
    ("the value saved before an update, in a for loop",
     "u32 p0(s32 p) { u32 v = pin_g; s16 i; for (i = 0; i < 2; i++) { p = v; v += 5; } return p; }",
     "p0(5)", 0x00000006),
    ("the same in a while loop",
     "u32 p1(s32 p) { u32 v = pin_g; s16 i = 2; while (i--) { p = v; v += 5; } return p; }",
     "p1(5)", 0x00000006),
    ("the same on 16 bits, around an inner pointer walk",
     "u32 p2(u16 p) { u16 v = pin_g; s16 i = 2; s8 *w; while (i--) { p = v; "
     "for (w = pin_a; w != pin_a + 8; w++) v += *w; } return p; }",
     "p2(5)", 0x0000FFC0),
    ("two variables swapped in a loop",
     "u32 p3(void) { u16 a = pin_g, b = 2, t; u8 k; for (k = 0; k < 3; k++) { t = a; a = b; b = t; } "
     "return ((u32)a << 8) | b; }",
     "p3()", 0x00000201),
    ("three 32-bit variables rotated in a loop",
     "u32 p4(void) { u32 a = pin_g, b = 0x20000, c = 0x30003, t; u8 k; "
     "for (k = 0; k < 4; k++) { t = a; a = b; b = c; c = t; } return a + b * 3 + c * 5; }",
     "p4()", 0x000B000E),
    ("two swaps in the same loop",
     "u32 p5(void) { u16 a = pin_g, b = 2, c = 7, d = 9, t; u8 k; for (k = 0; k < 3; k++) "
     "{ t = a; a = b; b = t; t = c; c = d; d = t; } return ((u32)a << 12) | (b << 8) | (c << 4) | d; }",
     "p5()", 0x00002197),
    ("Fibonacci",
     "u32 p6(void) { u16 f0 = 0, f1 = pin_g, t; u8 k; for (k = 0; k < 10; k++) "
     "{ t = f0 + f1; f0 = f1; f1 = t; } return f0; }",
     "p6()", 0x00000037),
    # the Kl invariant stopped the build: and / or / xor / neg computed a high half declared dead
    ("a byte array indexed by a masked long",
     "u32 p7(void) { pin_t[pin_x & 7] = 99; return pin_t[3] + pin_t[(pin_x | 4) & 7] + pin_t[(pin_x ^ 1) & 7]; }",
     "p7()", 99 + 17 + 12),
]
PROGS_PER_ROM = 10
TYPES = D.TYPES
UNSIGNED = [t for t in TYPES if t not in D.SIGNED]
FNV_PRIME = 16777619


class UB(Exception):
    """The program would execute undefined behaviour: not a test."""


# ------------------------------------------------------------ expressions --
# ('k', type, value)        a literal
# ('v', name)               a scalar variable (local, parameter or counter)
# ('i', array, index)       array[index]
# ('d', array, index)       *(array + index)
# ('f', base, field)        base is 's', 'p' (ps->) or ('a', index) for sa[index]
# ('c', type, e) ('u', op, e) ('b', op, l, r) ('t', c, a, b)

class Prog:
    """Declarations of one test function, and its statements."""

    def __init__(self):
        self.vars = {}        # name -> type (locals and counters)
        self.params = []      # names of the locals passed as parameters
        self.init = {}        # name -> initial value
        self.arrays = {}      # name -> (type, [values])
        self.fields = []      # [(name, type)] of the struct
        self.sinit = {}       # field -> initial value of `s`
        self.sainit = []      # two dicts: initial values of sa[0], sa[1]
        self.counters = []    # names never assigned by a generated statement
        self.body = []

    def ftype(self, name):
        return dict(self.fields)[name]


def leaf(t, v):
    return D.Node("leaf", t, D.wrap(v, t), leaf=0)


def checked(node, *inputs):
    """difftest's builders insert a cast or a mask where C would be undefined;
    here that means the program is not a test."""
    if any(k is not i for k, i in zip(node.kids, inputs)):
        raise UB
    return node


def ev(e, P, env):
    """Evaluate to a difftest Node (type and value)."""
    k = e[0]
    if k == "k":
        return leaf(e[1], e[2])
    if k == "v":
        return leaf(P.vars[e[1]], env[e[1]])
    if k in ("i", "d"):
        t, _ = P.arrays[e[1]]
        idx = ev(e[2], P, env).value
        if not 0 <= idx < len(env[e[1]]):
            raise UB
        return leaf(t, env[e[1]][idx])
    if k == "f":
        return leaf(P.ftype(e[2]), struct_of(e[1], P, env)[e[2]])
    if k == "c":
        return D.cast(e[1], ev(e[2], P, env))
    if k == "u":
        a = ev(e[2], P, env)
        return checked(D.unary(e[1], a), a)
    if k == "b":
        if e[1] in ("&&", "||"):                  # the right side may not run
            a = ev(e[2], P, env)
            if (a.value != 0) == (e[1] == "||"):
                return leaf("s16", int(e[1] == "||"))
            return leaf("s16", int(ev(e[3], P, env).value != 0))
        a, b = ev(e[2], P, env), ev(e[3], P, env)
        return checked(D.binary(e[1], a, b), a, b)
    if k == "t":
        c = ev(e[1], P, env)
        t = D.common(ty(e[2], P), ty(e[3], P))
        return leaf(t, ev(e[2] if c.value else e[3], P, env).value)
    raise AssertionError(e)


def struct_of(base, P, env):
    if base in ("s", "p"):
        return env["s"]
    idx = ev(base[1], P, env).value
    if not 0 <= idx < 2:
        raise UB
    return env["sa"][idx]


def ty(e, P):
    """Static type (C's rules do not look at values)."""
    k = e[0]
    if k == "k":
        return e[1]
    if k == "v":
        return P.vars[e[1]]
    if k in ("i", "d"):
        return P.arrays[e[1]][0]
    if k == "f":
        return P.ftype(e[2])
    if k == "c":
        return e[1]
    if k == "u":
        return "s16" if e[1] == "!" else D.promote(ty(e[2], P))
    if k == "b":
        op = e[1]
        if op in ("&&", "||", "==", "!=", "<", "<=", ">", ">="):
            return "s16"
        if op in ("<<", ">>"):
            return D.promote(ty(e[2], P))
        return D.common(ty(e[2], P), ty(e[3], P))
    return D.common(ty(e[2], P), ty(e[3], P))


def rd(e):
    """C text."""
    k = e[0]
    if k == "k":
        return D.literal(e[1], e[2])
    if k == "v":
        return e[1]
    if k == "i":
        return f"{e[1]}[{rd(e[2])}]"
    if k == "d":
        return f"(*({e[1]} + ({rd(e[2])})))"
    if k == "f":
        base = {"s": "s.", "p": "ps->"}.get(e[1]) if isinstance(e[1], str) else f"sa[{rd(e[1][1])}]."
        return base + e[2]
    if k == "c":
        return f"(({e[1]})({rd(e[2])}))"
    if k == "u":
        return f"({e[1]}({rd(e[2])}))"
    if k == "b":
        return f"(({rd(e[2])}) {e[1]} ({rd(e[3])}))"
    return f"(({rd(e[1])}) ? ({rd(e[2])}) : ({rd(e[3])}))"


# -------------------------------------------------------------- generator --

def konst(t, v):
    return ("k", t, D.wrap(v, t))


def masked(e, n):
    """An index into n elements (n a power of two)."""
    return ("b", "&", e, konst("s16", n - 1))


def gen_lvalue(rng, P):
    pick = rng.random()
    if pick < 0.40:
        return ("v", rng.choice([n for n in P.vars if n not in P.counters]))
    if pick < 0.65:
        name = rng.choice(list(P.arrays))
        n = len(P.arrays[name][1])
        return (rng.choice("iid"), name, masked(gen_expr(rng, P, 1), n))
    base = rng.choice(["s", "p", ("a", masked(gen_expr(rng, P, 1), 2))])
    return ("f", base, rng.choice(P.fields)[0])


def gen_expr(rng, P, depth):
    if depth == 0 or rng.random() < 0.2:
        pick = rng.random()
        if pick < 0.25:
            t = rng.choice(TYPES)
            return konst(t, D.interesting(rng, t))
        if pick < 0.45 and P.counters:
            return ("v", rng.choice(P.counters))
        return gen_lvalue(rng, P) if depth else ("v", rng.choice(list(P.vars)))
    k = rng.random()
    if k < 0.12:
        return ("c", rng.choice(TYPES), gen_expr(rng, P, depth - 1))
    if k < 0.22:
        op = rng.choice("-~!")
        a = gen_expr(rng, P, depth - 1)
        if op == "-" and D.promote(ty(a, P)) in D.SIGNED and D.BITS[ty(a, P)] >= 16:
            a = ("c", D.unsigned_of(D.promote(ty(a, P))), a)
        return ("u", op, a)
    if k < 0.30:
        return ("t", gen_expr(rng, P, depth - 1), gen_expr(rng, P, depth - 1),
                gen_expr(rng, P, depth - 1))
    return gen_binary(rng, P, rng.choice(D.BIN_OPS), gen_expr(rng, P, depth - 1),
                      gen_expr(rng, P, depth - 1))


def gen_binary(rng, P, op, l, r):
    """Keep the operation defined whatever the values turn out to be."""
    lt, rt = ty(l, P), ty(r, P)
    if op in ("<<", ">>"):
        pl = D.promote(lt)
        r = ("b", "&", r, konst("s16", D.BITS[pl] - 1))
        if op == "<<" and pl in D.SIGNED:
            l = ("c", D.unsigned_of(pl), l)
    elif op in ("/", "%"):
        r = ("b", "|", ("b", "&", r, konst("s16", 0x7C)), konst("s16", 2))   # 2..126
        if rng.random() < 0.3 and D.common(lt, ty(r, P)) in D.SIGNED:
            r = ("u", "-", r)                                               # -126..-2
    elif op in ("+", "-", "*"):
        ct = D.common(lt, rt)
        narrow = D.BITS[lt] == 8 and D.BITS[rt] == 8
        if ct in D.SIGNED and (op == "*" or not narrow):
            l = ("c", D.unsigned_of(ct), l)
    return ("b", op, l, r)


def gen_block(rng, P, depth, n):
    return [gen_stmt(rng, P, depth) for _ in range(n)]


def gen_stmt(rng, P, depth):
    pick = rng.random()
    free = [c for c in ("i", "j") if c not in P.counters]
    if depth > 0 and pick < 0.16:
        return ("if", gen_expr(rng, P, 2), gen_block(rng, P, depth - 1, rng.randint(1, 2)),
                gen_block(rng, P, depth - 1, rng.randint(0, 2)))
    if depth > 0 and pick < 0.34 and free:
        c = free[0]
        P.counters.append(c)
        body = gen_block(rng, P, depth - 1, rng.randint(1, 3))
        P.counters.remove(c)
        return ("for", rng.choice("ABCD"), c, rng.randint(1, 5), body)
    if pick < 0.42:
        acc = rng.choice([n for n, t in P.vars.items() if t in UNSIGNED and n not in P.counters])
        return ("walk", rng.choice(list(P.arrays)), acc, rng.choice("+^|-"), rng.random() < 0.4)
    lv = gen_lvalue(rng, P)
    lt = ty(lv, P)
    if pick < 0.52 and lt not in ("s16", "s32"):
        return ("inc", rng.choice(["++", "--"]), rng.random() < 0.5, lv)
    if pick < 0.68:
        ops = ["&", "|", "^", ">>"] + (["+", "-", "*", "<<"] if lt in UNSIGNED else [])
        op = rng.choice(ops)
        e = gen_expr(rng, P, 2)
        if op in ("<<", ">>"):
            e = ("b", "&", e, konst("s16", D.BITS[D.promote(lt)] - 1))
        return ("op", op, lv, e)
    return ("set", lv, gen_expr(rng, P, rng.choice([1, 2, 2, 3])))


def gen_prog(rng) -> Prog:
    P = Prog()
    for n in range(rng.randint(3, 5)):
        P.vars[f"v{n}"] = rng.choice(TYPES)
    P.vars["u"] = rng.choice(UNSIGNED)            # always one unsigned accumulator
    for name, t in P.vars.items():
        P.init[name] = D.interesting(rng, t)
    P.params = [n for n in P.vars if rng.random() < 0.4]
    for c in ("i", "j"):
        P.vars[c] = rng.choice(["u8", "u16", "s16"])
        P.init[c] = 0
    for n in range(rng.randint(1, 2)):
        t = rng.choice(TYPES)
        P.arrays[f"a{n}"] = (t, [D.interesting(rng, t) for _ in range(rng.choice([4, 8]))])
    P.fields = [(f"f{n}", rng.choice(TYPES)) for n in range(rng.randint(2, 4))]
    P.sinit = {f: D.interesting(rng, t) for f, t in P.fields}
    P.sainit = [{f: D.interesting(rng, t) for f, t in P.fields} for _ in range(2)]
    P.body = gen_block(rng, P, 2, rng.randint(3, 6))
    return P


# ------------------------------------------------------------ interpreter --

def store(lv, value, P, env):
    k = lv[0]
    if k == "v":
        env[lv[1]] = D.wrap(value, P.vars[lv[1]])
    elif k in ("i", "d"):
        idx = ev(lv[2], P, env).value
        if not 0 <= idx < len(env[lv[1]]):
            raise UB
        env[lv[1]][idx] = D.wrap(value, P.arrays[lv[1]][0])
    else:
        struct_of(lv[1], P, env)[lv[2]] = D.wrap(value, P.ftype(lv[2]))


def run_block(block, P, env):
    for st in block:
        k = st[0]
        if k == "set":
            # C evaluates the two sides in no fixed order; neither has a side effect
            store(st[1], ev(st[2], P, env).value, P, env)
        elif k == "op":
            store(st[2], ev(("b", st[1], st[2], st[3]), P, env).value, P, env)
        elif k == "inc":
            store(st[3], ev(("b", "+" if st[1] == "++" else "-", st[3], konst("s16", 1)), P, env).value, P, env)
        elif k == "if":
            run_block(st[2] if ev(st[1], P, env).value else st[3], P, env)
        elif k == "walk":
            _, arr, acc, op, write = st
            for n in range(len(env[arr])):
                elem = ("i", arr, konst("s16", n))
                store(("v", acc), ev(("b", op, ("v", acc), elem), P, env).value, P, env)
                if write:
                    store(elem, ev(("v", acc), P, env).value, P, env)
        else:
            _, form, c, count, body = st
            ct = P.vars[c]
            if form == "A":                       # for (c = 0; c < K; c++)
                env[c] = 0
                while env[c] < count:
                    run_block(body, P, env)
                    env[c] = D.wrap(env[c] + 1, ct)
            elif form == "B":                     # for (c = K; c > 0; c--)
                env[c] = count
                while env[c] > 0:
                    run_block(body, P, env)
                    env[c] = D.wrap(env[c] - 1, ct)
            elif form == "C":                     # c = 0; do { } while (++c < K);
                env[c] = 0
                while True:
                    run_block(body, P, env)
                    env[c] = D.wrap(env[c] + 1, ct)
                    if not env[c] < count:
                        break
            else:                                 # c = K; while (c--) { }
                env[c] = count
                while True:
                    was = env[c]
                    env[c] = D.wrap(env[c] - 1, ct)
                    if not was:
                        break
                    run_block(body, P, env)


def expected(P) -> int:
    env = dict(P.init)
    for name, (_, vals) in P.arrays.items():
        env[name] = list(vals)
    env["s"] = dict(P.sinit)
    env["sa"] = [dict(d) for d in P.sainit]
    run_block(P.body, P, env)
    P.final = [D.wrap(v, "u32") for v in state_values(P, env)]
    h = 0x811C9DC5
    for value in P.final:
        h = ((h ^ value) * FNV_PRIME) & 0xFFFFFFFF
    return h


def state_values(P, env):
    for name in P.vars:
        yield env[name]
    for name in P.arrays:
        yield from env[name]
    for f, _ in P.fields:
        yield env["s"][f]
    for d in env["sa"]:
        for f, _ in P.fields:
            yield d[f]


# ------------------------------------------------------------------ C text --

def c_block(block, ind):
    out = []
    pad = "    " * ind
    for st in block:
        k = st[0]
        if k == "set":
            out.append(f"{pad}{rd(st[1])} = {rd(st[2])};")
        elif k == "op":
            out.append(f"{pad}{rd(st[2])} {st[1]}= {rd(st[3])};")
        elif k == "inc":
            out.append(f"{pad}{rd(st[3])}{st[1]};" if st[2] else f"{pad}{st[1]}{rd(st[3])};")
        elif k == "if":
            out.append(f"{pad}if ({rd(st[1])}) {{")
            out += c_block(st[2], ind + 1)
            if st[3]:
                out.append(f"{pad}}} else {{")
                out += c_block(st[3], ind + 1)
            out.append(f"{pad}}}")
        elif k == "walk":
            _, arr, acc, op, write = st
            n = "ARRLEN_" + arr
            out.append(f"{pad}for (pw_{arr} = {arr}; pw_{arr} != {arr} + {n}; pw_{arr}++) {{")
            out.append(f"{pad}    {acc} {op}= *pw_{arr};")
            if write:
                out.append(f"{pad}    *pw_{arr} = {acc};")
            out.append(f"{pad}}}")
        else:
            _, form, c, count, body = st
            head = {"A": f"for ({c} = 0; {c} < {count}; {c}++) {{",
                    "B": f"for ({c} = {count}; {c} > 0; {c}--) {{",
                    "C": f"{c} = 0; do {{",
                    "D": f"{c} = {count}; while ({c}--) {{"}[form]
            out.append(pad + head)
            out += c_block(body, ind + 1)
            out.append(pad + (f"}} while (++{c} < {count});" if form == "C" else "}"))
    return out


def c_prog(P, n: int, label: str, exp: int, probe: bool = False) -> tuple:
    """(file-scope text, call expression) of program n."""
    g = f"t{n}_"
    ren = lambda s: s                                  # names are local to the function
    out = [f"/* {label}: expect 0x{exp:08X} */"]
    out.append(f"struct S{n} {{ " + " ".join(f"{t} {f};" for f, t in P.fields) + " };")
    body = []
    for name, (t, vals) in P.arrays.items():
        out.append(f"{t} {g}{name}[{len(vals)}] = {{ {', '.join(str(v) for v in vals)} }};")
    out.append(f"struct S{n} {g}s = {{ {', '.join(str(P.sinit[f]) for f, _ in P.fields)} }};")
    out.append(f"struct S{n} {g}sa[2] = {{ " + ", ".join(
        "{ " + ", ".join(str(d[f]) for f, _ in P.fields) + " }" for d in P.sainit) + " };")
    params = ", ".join(f"{P.vars[p]} {p}" for p in P.params) or "void"
    out.append(f"u32 t{n}({params}) {{")
    for name, t in P.vars.items():
        if name not in P.params:
            body.append(f"    {t} {name} = {P.init[name]};")
    body.append(f"    struct S{n} *ps = &{g}s;")
    body.append("    u32 h = 0x811C9DC5UL;")
    for name, (t, vals) in P.arrays.items():
        body.append(f"    {t} *pw_{name};")
    text = "\n".join(c_block(P.body, 1))
    for name, (t, vals) in P.arrays.items():
        text = text.replace("ARRLEN_" + name, str(len(vals)))
    # the globals carry the test's prefix; the statements name them bare
    defs = [f"#define {name} {g}{name}" for name in list(P.arrays) + ["s", "sa"]]
    undefs = [f"#undef {name}" for name in list(P.arrays) + ["s", "sa"]]
    mix = []
    for name in P.vars:
        mix.append(f"(u32){name}")
    for name, (t, vals) in P.arrays.items():
        mix += [f"(u32){name}[{k}]" for k in range(len(vals))]
    mix += [f"(u32)s.{f}" for f, _ in P.fields]
    mix += [f"(u32)sa[{k}].{f}" for k in range(2) for f, _ in P.fields]
    hash_lines = [f"    h = (h ^ {m}) * {FNV_PRIME}UL;" for m in mix]
    if probe:                 # every piece of state, one by one, for a failing program
        hash_lines = [f"    probe[{k}] = {m};" for k, m in enumerate(mix)] + hash_lines
        P.probe_names = [m[5:] for m in mix]
    out = out[:-1] + defs + [out[-1]] + body + [text] + hash_lines + ["    return h;", "}"] + undefs
    call = f"t{n}({', '.join(D.literal(P.vars[p], P.init[p]) for p in P.params)})"
    return "\n".join(out), call


# ----------------------------------------------------------- build and run --

class Case:
    def __init__(self, P, label):
        self.P, self.label = P, label
        self.expected = expected(P)        # raises UB


def make_cases(seed: int) -> list:
    rng = random.Random(f"stmt-{seed}")
    cases = []
    while len(cases) < PROGS_PER_ROM:
        P = gen_prog(rng)
        try:
            cases.append(Case(P, f"seed {seed} program {len(cases)}"))
        except UB:
            continue
    return cases


def run_batch(cases: list, name: str, luna: str):
    d = D.WORK / name
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    texts, calls = [], []
    for n, c in enumerate(cases):
        text, call = c_prog(c.P, n, c.label, c.expected)
        texts.append(text + "\n")
        calls.append(f"    r[{n}] = {call};")
    src = ["#include <snes.h>", "", f"u32 r[{len(cases)}];", "u16 done;", ""] + texts
    src += ["int main(void) {", "    consoleInit();"] + calls
    src += [f"    done = 0x{D.DONE_MAGIC:04X};", "    while (1) { WaitForVBlank(); }", "    return 0;", "}", ""]
    (d / "main.c").write_text("\n".join(src))
    (d / "Makefile").write_text(D.MAKEFILE.format(repo=D.REPO))
    env = {k: v for k, v in os.environ.items() if k not in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL")}
    try:
        proc = subprocess.run(["make", "-s", "-C", str(d)] + D.MAKE_ARGS, capture_output=True,
                              text=True, env=env, timeout=D.BUILD_TIMEOUT)
    except subprocess.TimeoutExpired:
        return f"the build of {d} did not end in {D.BUILD_TIMEOUT} s (a compiler that loops?)"
    rom = d / "difftest.sfc"
    if proc.returncode != 0 or not rom.is_file():
        return f"build failed in {d}:\n" + (proc.stdout + proc.stderr)[-1500:]
    steps = 4_000_000
    while True:
        out = subprocess.run([luna, "state", "-n", str(steps), "--peek", f"r:{len(cases) * 4:x}",
                              "--peek", "done:2", "--out", "-", str(rom)], capture_output=True, text=True)
        try:
            peeks = json.loads(out.stdout)["peeks"]
        except (ValueError, KeyError):
            return f"luna gave no state for {rom}:\n{out.stderr[-800:]}"
        if int.from_bytes(bytes.fromhex(peeks[1]["bytes_hex"]), "little") == D.DONE_MAGIC:
            break
        if steps >= 128_000_000:
            return f"{rom} never reached the end of main (done != 0x{D.DONE_MAGIC:04X})"
        steps *= 4
    raw = bytes.fromhex(peeks[0]["bytes_hex"])
    return [int.from_bytes(raw[i:i + 4], "little") for i in range(0, len(cases) * 4, 4)]


def probe_case(case: Case, luna: str, name: str) -> list:
    """Which variables are wrong: the program again, writing each one out."""
    d = D.WORK / name
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    text, call = c_prog(case.P, 0, case.label, case.expected, probe=True)
    n = len(case.P.final)
    src = ["#include <snes.h>", "", f"u32 probe[{n}];", "u32 r[1];", "u16 done;", "", text, "",
           "int main(void) {", "    consoleInit();", f"    r[0] = {call};",
           f"    done = 0x{D.DONE_MAGIC:04X};", "    while (1) { WaitForVBlank(); }", "    return 0;", "}", ""]
    (d / "main.c").write_text("\n".join(src))
    (d / "Makefile").write_text(D.MAKEFILE.format(repo=D.REPO))
    env = {k: v for k, v in os.environ.items() if k not in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL")}
    proc = subprocess.run(["make", "-s", "-C", str(d)] + D.MAKE_ARGS, capture_output=True, text=True, env=env)
    if proc.returncode != 0:
        return ["  (the probe did not build)"]
    out = subprocess.run([luna, "state", "-n", "16000000", "--peek", f"probe:{n * 4:x}", "--out", "-",
                          str(d / "difftest.sfc")], capture_output=True, text=True)
    try:
        raw = bytes.fromhex(json.loads(out.stdout)["peeks"][0]["bytes_hex"])
    except (ValueError, KeyError):
        return ["  (the probe gave no state)"]
    lines = []
    for k, (nm, exp) in enumerate(zip(case.P.probe_names, case.P.final)):
        got = int.from_bytes(raw[k * 4:k * 4 + 4], "little")
        if got != exp:
            lines.append(f"  wrong: {nm} = 0x{got:08X}, expected 0x{exp:08X}")
    return lines or ["  (every variable is right when read one by one: the checksum itself differs)"]


def run_pinned(luna: str):
    d = D.WORK / "stmt_pinned"
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    src = ["#include <snes.h>", "", f"u32 r[{len(PINNED)}];", "u16 done;", "", PINNED_GLOBALS]
    src += [f"/* {label} */\n{text}\n" for label, text, _, _ in PINNED]
    src += ["int main(void) {", "    consoleInit();"]
    src += [f"    r[{k}] = {call};" for k, (_, _, call, _) in enumerate(PINNED)]
    src += [f"    done = 0x{D.DONE_MAGIC:04X};", "    while (1) { WaitForVBlank(); }", "    return 0;", "}", ""]
    (d / "main.c").write_text("\n".join(src))
    (d / "Makefile").write_text(D.MAKEFILE.format(repo=D.REPO))
    env = {k: v for k, v in os.environ.items() if k not in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL")}
    proc = subprocess.run(["make", "-s", "-C", str(d)] + D.MAKE_ARGS, capture_output=True, text=True, env=env)
    if proc.returncode != 0:
        return "pinned", 2, [f"pinned: build failed in {d}:\n" + (proc.stdout + proc.stderr)[-1200:]]
    out = subprocess.run([luna, "state", "-n", "8000000", "--peek", f"r:{len(PINNED) * 4:x}", "--peek", "done:2",
                          "--out", "-", str(d / "difftest.sfc")], capture_output=True, text=True)
    try:
        peeks = json.loads(out.stdout)["peeks"]
    except (ValueError, KeyError):
        return "pinned", 2, ["pinned: luna gave no state"]
    if int.from_bytes(bytes.fromhex(peeks[1]["bytes_hex"]), "little") != D.DONE_MAGIC:
        return "pinned", 2, ["pinned: the ROM never reached the end of main"]
    raw = bytes.fromhex(peeks[0]["bytes_hex"])
    msgs = []
    for k, (label, text, _, exp) in enumerate(PINNED):
        got = int.from_bytes(raw[k * 4:k * 4 + 4], "little")
        if got != exp:
            msgs += [f"MISMATCH pinned: {label}: expected 0x{exp:08X}, got 0x{got:08X}", "    " + text]
    return "pinned", (1 if msgs else 0), msgs


def without(block, path):
    """A copy of the block with the statement at `path` removed."""
    block = copy.deepcopy(block)
    cur = block
    for step in path[:-1]:
        cur = cur[step[0]][step[1]]
    del cur[path[-1]]
    return block


def stmt_paths(block, prefix=()):
    for n, st in enumerate(block):
        yield prefix + (n,)
        if st[0] == "if":
            yield from stmt_paths(st[2], prefix + ((n, 2),))
            yield from stmt_paths(st[3], prefix + ((n, 3),))
        elif st[0] == "for":
            yield from stmt_paths(st[4], prefix + ((n, 4),))


def reduce_case(case: Case, luna: str, name: str) -> Case:
    """Drop statements for as long as the program still fails."""
    best, rnd = case, 0
    while True:
        cands = []
        for path in stmt_paths(best.P.body):
            P = copy.copy(best.P)
            P.body = without(best.P.body, path)
            try:
                cands.append(Case(P, case.label + " (reduced)"))
            except UB:
                pass
        hit = None
        for i in range(0, len(cands), PROGS_PER_ROM):
            chunk = cands[i:i + PROGS_PER_ROM]
            res = run_batch(chunk, f"{name}_reduce{rnd}_{i // PROGS_PER_ROM}", luna)
            if isinstance(res, str):
                return best
            hit = next((c for c, got in zip(chunk, res) if got != c.expected), None)
            if hit:
                break
        if not hit:
            return best
        best, rnd = hit, rnd + 1


def run_seed(seed, luna: str, keep: bool):
    if seed == "pinned":
        return run_pinned(luna)
    cases = make_cases(seed)
    name = f"stmt_{seed}"
    res = run_batch(cases, name, luna)
    if isinstance(res, str):
        return seed, 2, [f"seed {seed}: {res}"]
    msgs = []
    for c, got in zip(cases, res):
        if got != c.expected:
            msgs.append(f"MISMATCH {c.label}: expected 0x{c.expected:08X}, got 0x{got:08X}")
            small = reduce_case(c, luna, name)
            msgs.append(f"  reduced to {sum(1 for _ in stmt_paths(small.P.body))} statement(s):")
            msgs += ["    " + ln for ln in c_prog(small.P, 0, small.label, small.expected)[0].splitlines()]
            msgs.append("    call: " + c_prog(small.P, 0, small.label, small.expected)[1])
            msgs += probe_case(small, luna, name + "_probe")
    if not msgs and not keep:
        shutil.rmtree(D.WORK / name, ignore_errors=True)
    return seed, (1 if msgs else 0), msgs


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--seeds", help="e.g. 5 or 1,2,9 or 100-163 (default: the gate seeds)")
    ap.add_argument("--keep", action="store_true", help="keep the generated projects of passing seeds")
    ap.add_argument("--jobs", type=int, default=int(os.environ.get("LUNA_JOBS", os.cpu_count() or 1)))
    ap.add_argument("--cc", help="path of another cc65816 to test (e.g. one built before a change)")
    args = ap.parse_args()
    if args.cc:
        D.MAKE_ARGS.append(f"CC={Path(args.cc).resolve()}")
    seeds = D.parse_seeds(args.seeds) if args.seeds else GATE_SEEDS
    luna = find_luna()
    worst = failed = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        for seed, code, msgs in pool.map(lambda s: run_seed(s, luna, args.keep), seeds):
            worst = max(worst, code)
            failed += code != 0
            for m in msgs:
                print(m)
    print(f"difftest-stmt: {len(seeds) - failed}/{len(seeds)} seeds clean, "
          f"{sum(len(PINNED) if x == 'pinned' else PROGS_PER_ROM for x in seeds)} programs")
    return worst


if __name__ == "__main__":
    sys.exit(main())
