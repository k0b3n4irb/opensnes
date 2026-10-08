#!/usr/bin/env python3
"""difftest_stmt.py — differential test of the C compiler on small programs.

The statement side of testing/difftest.py: where that one compares single
integer expressions, this one generates small functions — locals,
parameters and global scalars, global arrays in one and two dimensions, a
struct reached by name, through a pointer and in an array, bit-fields,
counted loops in four spellings with break and continue, a pointer walking
an array, if / else, switch with fall-through, compound assignments, ++ and
--, stores through a pointer to a pointer, and calls to helper functions
generated with it (pure ones, recursive ones, ones that take an array, a
struct pointer, or a pointer to write through, and two behind a table of
function pointers), a union read through its other members, array members
of the struct, static locals, and loops and skips written with goto —
compiles them with cc65816,
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

# The gate: the pinned functions below, twelve seeds, and the seeds that
# showed a defect first on 2026-10-08, whatever the generator has become
# since (42, 51, 62, 2630, 3797; 336 and 2085 for the functions of more than
# 256 temporaries, which no hand-written function here reproduces).
GATE_SEEDS = ["pinned"] + list(range(1, 13)) + [42, 51, 62, 336, 2085, 2630, 3797]

# Hand-written functions for the defects this test found, reduced. Fixed-width
# types and no operation that depends on the width of int, so the expected
# values are the ones a host compiler gives (checked 2026-10-08).
PINNED_GLOBALS = """u16 pin_d[4] = { [2] = 7 };
struct PinB { unsigned int b0 : 2; unsigned int b1 : 2; u8 pad; };
struct PinB pin_b = { 0, 0, 1 };
u16 pin_after = 0x234;
u16 pin_z;
s8 pin_a[8] = { 7, 3, 5, 3, -2, 1, -86, 4 };
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
    # what the first moves of a branch left in A was believed to be there on the other way out
    ("a do-while with a break that is not taken",
     "u32 p8(void) { u8 j = 0; do { if (pin_z) break; } while (++j < 3); return j; }",
     "p8()", 3),
    # a zero-fill before the first value of an initialised object was not emitted:
    # the object came out short and every init record after it was read shifted
    ("initialisers that start with implicit zeros, and the object after them",
     "u32 p9(void) { return ((u32)pin_d[2] << 16) | ((u16)pin_b.pad << 12) | pin_after; }",
     "p9()", 0x00071234),
    # the low half lived in A only and `lda #0` for the high half overwrote it
    ("a condition returned as a 32-bit value",
     "u32 p10(u32 x) { return x ? 1L : (15L & x); }",
     "p10(0x00070000)", 1),
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
# ('f', base, field)        base is 's', 'p' (ps->), 'q' (a helper's q->) or ('a', index) for sa[index]
# ('m', matrix, i, j)       matrix[i][j]
# ('bf', field)             bf.field, a bit-field
# ('fa', base, field, i)    an element of an array member of the struct
# ('un', member, i)         a member of the union: 'l', or w[i], b[i], sw[i]
# ('fcall', sel, args)      fpt[sel](args): a call through a table of function pointers
# ('call', k, args)         helper k called with these arguments
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
        self.gvars = []       # names of the scalars declared at file scope
        self.mats = {}        # name -> (type, [[values] * 4] * 2)
        self.bits = []        # [(name, signed, width)] of the bit-field struct
        self.binit = {}       # bit-field -> initial value
        self.helpers = []     # Helper objects
        self.afields = []     # [(name, type)]: array members of the struct, 4 elements each
        self.union = False    # a union of a u32, two u16, four u8 and two s16 is declared
        self.uinit = 0
        self.statics = []     # locals declared `static`
        self.fptab = None     # (rtype, [(name, type)], [expr, expr]): two functions behind a table
        self.nlabel = 0
        self.ctx = []         # generator only: 'loop' / 'switch' nesting
        self.body = []

    def ftype(self, name):
        return dict(self.fields)[name]

    def atype(self, name):
        return dict(self.afields)[name]

    def btype(self, name):
        _, signed, width = next(b for b in self.bits if b[0] == name)
        return "u16" if (not signed and width == 16) else "s16"


class Helper:
    """A function generated beside the test function and called by it.

    pure  RT h(T0 a, T1 b...)        { return EXPR(a, b...); }
    rec   RT h(u8 n, RT acc)         { if (n == 0) return acc; return h(n - 1, EXPR(acc, n)); }
    recn  RT h(u8 n, RT acc)         { if (n == 0) return acc; rv = h(n - 1, acc); return EXPR(rv, n); }
    sum   u32 h(T *p, u8 n)          { fold the n elements p points at }
    set   void h(T *p, T v)          { *p = v; }
    fld   RT h(struct S *q)          { return EXPR(q->fields); }
    """

    def __init__(self, kind, rtype, params, expr=None):
        self.kind, self.rtype, self.params, self.expr = kind, rtype, params, expr
        self.scope = Prog()
        self.scope.vars = dict(params)


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
    if k == "m":
        i, j = ev(e[2], P, env).value, ev(e[3], P, env).value
        if not (0 <= i < 2 and 0 <= j < 4):
            raise UB
        return leaf(P.mats[e[1]][0], env[e[1]][i][j])
    if k == "bf":
        return leaf(P.btype(e[1]), env["bf"][e[1]])
    if k == "fa":
        i = ev(e[3], P, env).value
        if not 0 <= i < 4:
            raise UB
        return leaf(P.atype(e[2]), struct_of(e[1], P, env)[e[2]][i])
    if k == "un":
        t, off, size = union_slot(e, P, env)
        return leaf(t, int.from_bytes(env["un"][off:off + size], "little"))
    if k == "fcall":
        rt, params, bodies = P.fptab
        which = ev(e[1], P, env).value
        if which not in (0, 1):
            raise UB
        vals = [D.wrap(ev(a, P, env).value, t) for a, (_, t) in zip(e[2], params)]
        return leaf(rt, D.wrap(ev(bodies[which], P.fpscope, dict(zip((n for n, _ in params), vals))).value, rt))
    if k == "call":
        return leaf(P.helpers[e[1]].rtype, call_helper(P.helpers[e[1]], e[2], P, env))
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


def call_helper(h, args, P, env) -> int:
    """The value a helper returns (already converted to its return type)."""
    if h.kind == "sum":
        t, total = P.arrays[args[0]][0], 0
        for v in env[args[0]]:                      # t = (t << 1) ^ (u32)*p++
            total = ((total << 1) ^ D.wrap(v, "u32")) & 0xFFFFFFFF
        return total
    if h.kind == "fld":
        return D.wrap(ev(h.expr, h.scope, {"q": struct_of(args[0], P, env)}).value, h.rtype)
    vals = [D.wrap(ev(a, P, env).value, t) for a, (_, t) in zip(args, h.params)]
    if h.kind == "pure":
        return D.wrap(ev(h.expr, h.scope, dict(zip((n for n, _ in h.params), vals))).value, h.rtype)
    n, acc = vals
    if h.kind == "rec":
        while n:
            acc = D.wrap(ev(h.expr, h.scope, {"n": n, "acc": acc}).value, h.rtype)
            n -= 1
        return acc
    for step in range(1, n + 1):                    # recn: innermost call first
        acc = D.wrap(ev(h.expr, h.scope, {"n": step, "rv": acc}).value, h.rtype)
    return acc


UNION_MEMBERS = {"l": ("u32", 4, 1), "w": ("u16", 2, 2), "b": ("u8", 1, 4), "sw": ("s16", 2, 2)}


def union_slot(e, P, env):
    """(type, byte offset, size) of a union member access; little-endian."""
    t, size, count = UNION_MEMBERS[e[1]]
    i = ev(e[2], P, env).value if e[1] != "l" else 0
    if not 0 <= i < count:
        raise UB
    return t, i * size, size


def struct_of(base, P, env):
    if base in ("s", "p"):
        return env["s"]
    if base == "q":
        return env["q"]
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
    if k == "m":
        return P.mats[e[1]][0]
    if k == "bf":
        return P.btype(e[1])
    if k == "fa":
        return P.atype(e[2])
    if k == "un":
        return UNION_MEMBERS[e[1]][0]
    if k == "fcall":
        return P.fptab[0]
    if k == "call":
        return P.helpers[e[1]].rtype
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
        base = {"s": "s.", "p": "ps->", "q": "q->"}.get(e[1]) if isinstance(e[1], str) else f"sa[{rd(e[1][1])}]."
        return base + e[2]
    if k == "m":
        return f"{e[1]}[{rd(e[2])}][{rd(e[3])}]"
    if k == "bf":
        return "bf." + e[1]
    if k == "fa":
        return rd(("f", e[1], e[2])) + f"[{rd(e[3])}]"
    if k == "un":
        return "un.l" if e[1] == "l" else f"un.{e[1]}[{rd(e[2])}]"
    if k == "fcall":
        return f"fpt[{rd(e[1])}]({', '.join(rd(a) for a in e[2])})"
    if k == "call":
        return f"h{e[1]}({', '.join(rd_arg(a) for a in e[2])})"
    if k == "c":
        return f"(({e[1]})({rd(e[2])}))"
    if k == "u":
        return f"({e[1]}({rd(e[2])}))"
    if k == "b":
        return f"(({rd(e[2])}) {e[1]} ({rd(e[3])}))"
    return f"(({rd(e[1])}) ? ({rd(e[2])}) : ({rd(e[3])}))"


def rd_arg(a):
    """A call argument: an expression, an array name, or a struct designator."""
    if isinstance(a, str):
        return {"s": "&s", "p": "ps"}.get(a, a)
    if a[0] == "a":
        return f"&sa[{rd(a[1])}]"
    return rd(a)


# -------------------------------------------------------------- generator --

def konst(t, v):
    return ("k", t, D.wrap(v, t))


def masked(e, n):
    """An index into n elements (n a power of two)."""
    return ("b", "&", e, konst("s16", n - 1))


def gen_lvalue(rng, P, addressable=False):
    """addressable: something `&` may be applied to (not a bit-field)."""
    pick = rng.random()
    if pick < 0.36:
        return ("v", rng.choice([n for n in P.vars if n not in P.counters]))
    if pick < 0.44 and P.mats:
        return ("m", rng.choice(list(P.mats)), masked(gen_expr(rng, P, 1), 2), masked(gen_expr(rng, P, 1), 4))
    if pick < 0.52 and P.bits and not addressable:
        return ("bf", rng.choice(P.bits)[0])
    if pick < 0.58 and P.union:
        m = rng.choice(list(UNION_MEMBERS))
        return ("un", m, masked(gen_expr(rng, P, 1), UNION_MEMBERS[m][2]))
    if pick < 0.64 and P.afields:
        base = rng.choice(["s", "p", ("a", masked(gen_expr(rng, P, 1), 2))])
        return ("fa", base, rng.choice(P.afields)[0], masked(gen_expr(rng, P, 1), 4))
    if not P.arrays:                               # a helper's scope: parameters only
        return ("v", rng.choice(list(P.vars)))
    if pick < 0.70:
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
    if k < 0.03 and P.fptab:
        return ("fcall", masked(gen_expr(rng, P, depth - 1), 2),
                [gen_expr(rng, P, depth - 1) for _ in P.fptab[1]])
    if k < 0.10 and any(h.kind != "set" for h in P.helpers):
        return gen_call(rng, P, depth - 1)
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


def gen_call(rng, P, depth):
    k = rng.choice([n for n, h in enumerate(P.helpers) if h.kind != "set"])
    h = P.helpers[k]
    if h.kind == "sum":
        return ("call", k, [h.array, konst("u8", len(P.arrays[h.array][1]))])
    if h.kind == "fld":
        return ("call", k, [rng.choice(["s", "p", ("a", masked(gen_expr(rng, P, 1), 2))])])
    if h.kind in ("rec", "recn"):
        return ("call", k, [konst("u8", rng.randint(0, 5)), gen_expr(rng, P, depth)])
    return ("call", k, [gen_expr(rng, P, depth) for _ in h.params])


def gen_helpers(rng, P):
    """One to three helpers. A helper's body is generated in its own scope:
    it sees its parameters only, so a call has no side effect."""
    for _ in range(rng.randint(1, 3)):
        kind = rng.choice(["pure", "pure", "rec", "recn", "sum", "set", "fld"])
        rt = rng.choice(TYPES)
        if kind == "pure":
            h = Helper(kind, rt, [(f"x{n}", rng.choice(TYPES)) for n in range(rng.randint(1, 4))])
            h.expr = gen_expr(rng, h.scope, 2)
        elif kind in ("rec", "recn"):
            rt = rng.choice(UNSIGNED)        # an accumulator: unsigned arithmetic only
            h = Helper(kind, rt, [("n", "u8"), ("acc", rt)])
            h.scope.vars = {"n": "u8", "acc" if kind == "rec" else "rv": rt}
            h.expr = gen_expr(rng, h.scope, 2)
        elif kind == "sum":
            h = Helper(kind, "u32", [])
            h.array = rng.choice(list(P.arrays))
        elif kind == "set":
            h = Helper(kind, rng.choice(TYPES), [])
        else:
            h = Helper(kind, rt, [])
            h.scope.fields = P.fields
            h.scope.vars = {}
            h.expr = gen_fld_expr(rng, P, 2)
        P.helpers.append(h)


def gen_fld_expr(rng, P, depth):
    """An expression over the fields of *q."""
    if depth == 0 or rng.random() < 0.25:
        if rng.random() < 0.2:
            t = rng.choice(TYPES)
            return konst(t, D.interesting(rng, t))
        return ("f", "q", rng.choice(P.fields)[0])
    scope = Prog()
    scope.fields = P.fields
    return gen_binary(rng, scope, rng.choice(D.BIN_OPS), gen_fld_expr(rng, P, depth - 1),
                      gen_fld_expr(rng, P, depth - 1))


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
    if depth > 0 and pick < 0.14:
        return ("if", gen_expr(rng, P, 2), gen_block(rng, P, depth - 1, rng.randint(1, 2)),
                gen_block(rng, P, depth - 1, rng.randint(0, 2)))
    if depth > 0 and pick < 0.30 and free:
        c = free[0]
        P.counters.append(c)
        P.ctx.append("loop")
        body = gen_block(rng, P, depth - 1, rng.randint(1, 3))
        P.ctx.pop()
        P.counters.remove(c)
        return ("for", rng.choice("ABCD"), c, rng.randint(1, 5), body)
    if depth > 0 and pick < 0.33 and free:
        # a loop made of a label and a goto: no break or continue inside
        c = free[0]
        P.counters.append(c)
        P.ctx.append("goto")
        body = gen_block(rng, P, depth - 1, rng.randint(1, 3))
        P.ctx.pop()
        P.counters.remove(c)
        P.nlabel += 1
        return ("for", "G", c, rng.randint(1, 5), body, P.nlabel)
    if depth > 0 and pick < 0.355:
        P.ctx.append("goto")
        body = gen_block(rng, P, depth - 1, rng.randint(1, 2))
        P.ctx.pop()
        P.nlabel += 1
        return ("gskip", gen_expr(rng, P, 2), body, P.nlabel)
    if depth > 0 and pick < 0.39:
        labels = rng.sample(range(8), rng.randint(2, 4))
        P.ctx.append("switch")
        arms = [[lab, gen_block(rng, P, depth - 1, rng.randint(1, 2)), rng.random() < 0.3]
                for lab in labels]
        if rng.random() < 0.7:               # a default arm, anywhere
            arms.insert(rng.randint(0, len(arms)), [None, gen_block(rng, P, depth - 1, 1), rng.random() < 0.3])
        P.ctx.pop()
        return ("sw", masked(gen_expr(rng, P, 2), 8), arms)
    if pick < 0.42 and P.ctx and P.ctx[-1] == "loop":
        return (rng.choice(["brk", "cont"]), gen_expr(rng, P, 2))
    if pick < 0.46:
        sets = [n for n, h in enumerate(P.helpers) if h.kind == "set"]
        cands = [lv for lv in (gen_lvalue(rng, P, True) for _ in range(6))]
        if sets and rng.random() < 0.5:
            k = rng.choice(sets)
            cands = [lv for lv in cands if ty(lv, P) == P.helpers[k].rtype]
            if cands:
                return ("hset", k, cands[0], gen_expr(rng, P, 2))
        else:
            return ("pp", ty(cands[0], P), cands[0], gen_expr(rng, P, 2))
    if pick < 0.50:
        acc = rng.choice([n for n, t in P.vars.items() if t in UNSIGNED and n not in P.counters])
        return ("walk", rng.choice(list(P.arrays)), acc, rng.choice("+^|-"), rng.random() < 0.4)
    lv = gen_lvalue(rng, P)
    lt = ty(lv, P)
    if pick < 0.58 and lt not in ("s16", "s32"):
        return ("inc", rng.choice(["++", "--"]), rng.random() < 0.5, lv)
    if pick < 0.72:
        ops = ["&", "|", "^", ">>"] + (["+", "-", "*", "<<"] if lt in UNSIGNED and lv[0] != "bf" else [])
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
    P.params = [n for n in P.vars if rng.random() < 0.4]       # before the globals join P.vars
    for n in range(rng.randint(0, 2)):                 # scalars at file scope
        P.vars[f"g{n}"] = rng.choice(TYPES)
        P.init[f"g{n}"] = D.interesting(rng, P.vars[f"g{n}"])
        P.gvars.append(f"g{n}")
    for c in ("i", "j"):
        P.vars[c] = rng.choice(["u8", "u16", "s16"])
        P.init[c] = 0
    for n in range(rng.randint(1, 2)):
        t = rng.choice(TYPES)
        P.arrays[f"a{n}"] = (t, [D.interesting(rng, t) for _ in range(rng.choice([4, 8]))])
    P.fields = [(f"f{n}", rng.choice(TYPES)) for n in range(rng.randint(2, 4))]
    P.sinit = {f: D.interesting(rng, t) for f, t in P.fields}
    P.sainit = [{f: D.interesting(rng, t) for f, t in P.fields} for _ in range(2)]
    if rng.random() < 0.6:
        t = rng.choice(TYPES)
        P.mats["m0"] = (t, [[D.interesting(rng, t) for _ in range(4)] for _ in range(2)])
    if rng.random() < 0.6:
        for n in range(rng.randint(2, 4)):
            signed, width = rng.random() < 0.4, rng.randint(1, 12)
            P.bits.append((f"b{n}", signed, max(width, 2) if signed else width))
        P.binit = {name: bit_wrap(rng.getrandbits(16), signed, width) for name, signed, width in P.bits}
    if rng.random() < 0.5:
        P.afields = [(f"d{n}", rng.choice(TYPES)) for n in range(rng.randint(1, 2))]
        for d in [P.sinit] + P.sainit:
            for f, t in P.afields:
                d[f] = [D.interesting(rng, t) for _ in range(4)]
    if rng.random() < 0.5:
        P.union, P.uinit = True, rng.getrandbits(32)
    P.statics = [n for n in P.vars if n not in P.params and n not in P.gvars
                 and n not in ("i", "j") and rng.random() < 0.2]
    if rng.random() < 0.5:
        params = [(f"y{n}", rng.choice(TYPES)) for n in range(rng.randint(1, 3))]
        P.fpscope = Prog()
        P.fpscope.vars = dict(params)
        P.fptab = (rng.choice(TYPES), params, [gen_expr(rng, P.fpscope, 2) for _ in range(2)])
    gen_helpers(rng, P)
    P.body = gen_block(rng, P, 2, rng.randint(3, 6))
    return P


def bit_wrap(v: int, signed: bool, width: int) -> int:
    """v stored in a bit-field of that width."""
    v &= (1 << width) - 1
    if signed and v >> (width - 1):
        v -= 1 << width
    return v


# ------------------------------------------------------------ interpreter --

class Brk(Exception):
    pass


class Cont(Exception):
    pass


def store(lv, value, P, env):
    k = lv[0]
    if k == "v":
        env[lv[1]] = D.wrap(value, P.vars[lv[1]])
    elif k in ("i", "d"):
        idx = ev(lv[2], P, env).value
        if not 0 <= idx < len(env[lv[1]]):
            raise UB
        env[lv[1]][idx] = D.wrap(value, P.arrays[lv[1]][0])
    elif k == "m":
        i, j = ev(lv[2], P, env).value, ev(lv[3], P, env).value
        if not (0 <= i < 2 and 0 <= j < 4):
            raise UB
        env[lv[1]][i][j] = D.wrap(value, P.mats[lv[1]][0])
    elif k == "bf":
        _, signed, width = next(b for b in P.bits if b[0] == lv[1])
        env["bf"][lv[1]] = bit_wrap(value, signed, width)
    elif k == "fa":
        i = ev(lv[3], P, env).value
        if not 0 <= i < 4:
            raise UB
        struct_of(lv[1], P, env)[lv[2]][i] = D.wrap(value, P.atype(lv[2]))
    elif k == "un":
        t, off, size = union_slot(lv, P, env)
        env["un"][off:off + size] = (value & ((1 << (8 * size)) - 1)).to_bytes(size, "little")
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
        elif k in ("pp", "hset"):                 # a store through a pointer: the same store
            store(st[-2], ev(st[-1], P, env).value, P, env)
        elif k == "gskip":                        # if (c) goto L; body; L: ;
            if not ev(st[1], P, env).value:
                run_block(st[2], P, env)
        elif k == "brk":
            if ev(st[1], P, env).value:
                raise Brk
        elif k == "cont":
            if ev(st[1], P, env).value:
                raise Cont
        elif k == "sw":
            sel = ev(st[1], P, env).value
            arms = st[2]
            at = next((n for n, a in enumerate(arms) if a[0] == sel), None)
            if at is None:
                at = next((n for n, a in enumerate(arms) if a[0] is None), None)
            while at is not None and at < len(arms):
                run_block(arms[at][1], P, env)
                at = at + 1 if arms[at][2] else None
        elif k == "walk":
            _, arr, acc, op, write = st
            for n in range(len(env[arr])):
                elem = ("i", arr, konst("s16", n))
                store(("v", acc), ev(("b", op, ("v", acc), elem), P, env).value, P, env)
                if write:
                    store(elem, ev(("v", acc), P, env).value, P, env)
        else:
            form, c, count, body = st[1:5]
            ct = P.vars[c]
            if form == "G":                       # c = K; L: body; if (--c > 0) goto L;
                env[c] = count
                while True:
                    run_block(body, P, env)
                    env[c] = D.wrap(env[c] - 1, ct)
                    if not env[c] > 0:
                        break
                continue
            def once():
                """The body; False when it left through `break`. `continue`
                goes on to the loop's own step, as in C."""
                try:
                    run_block(body, P, env)
                except Cont:
                    pass
                except Brk:
                    return False
                return True

            if form == "A":                       # for (c = 0; c < K; c++)
                env[c] = 0
                while env[c] < count and once():
                    env[c] = D.wrap(env[c] + 1, ct)
            elif form == "B":                     # for (c = K; c > 0; c--)
                env[c] = count
                while env[c] > 0 and once():
                    env[c] = D.wrap(env[c] - 1, ct)
            elif form == "C":                     # c = 0; do { } while (++c < K);
                env[c] = 0
                while once():
                    env[c] = D.wrap(env[c] + 1, ct)
                    if not env[c] < count:
                        break
            else:                                 # c = K; while (c--) { }
                env[c] = count
                while True:
                    was = env[c]
                    env[c] = D.wrap(env[c] - 1, ct)
                    if not was or not once():
                        break


def expected(P) -> int:
    env = dict(P.init)
    for name, (_, vals) in P.arrays.items():
        env[name] = list(vals)
    env["s"] = copy.deepcopy(P.sinit)
    env["sa"] = copy.deepcopy(P.sainit)
    env["un"] = bytearray(P.uinit.to_bytes(4, "little"))
    for name, (_, rows) in P.mats.items():
        env[name] = [list(r) for r in rows]
    env["bf"] = dict(P.binit)
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
    for name in P.mats:
        for row in env[name]:
            yield from row
    for name, _, _ in P.bits:
        yield env["bf"][name]
    for d in [env["s"]] + env["sa"]:
        for f, _ in P.afields:
            yield from d[f]
    if P.union:
        yield int.from_bytes(env["un"], "little")


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
        elif k == "pp":
            out.append(f"{pad}{{ {st[1]} *q1 = &{rd(st[2])}; {st[1]} **q2 = &q1; **q2 = {rd(st[3])}; }}")
        elif k == "hset":
            out.append(f"{pad}h{st[1]}(&{rd(st[2])}, {rd(st[3])});")
        elif k == "gskip":
            out.append(f"{pad}if ({rd(st[1])}) goto L{st[3]};")
            out += c_block(st[2], ind)
            out.append(f"{pad}L{st[3]}: ;")
        elif k in ("brk", "cont"):
            out.append(f"{pad}if ({rd(st[1])}) {'break' if k == 'brk' else 'continue'};")
        elif k == "sw":
            out.append(f"{pad}switch ({rd(st[1])}) {{")
            for lab, blk, falls in st[2]:
                out.append(f"{pad}{'default' if lab is None else 'case ' + str(lab)}:")
                out += c_block(blk, ind + 1)
                out.append(f"{pad}    {'/* falls through */' if falls else 'break;'}")
            out.append(f"{pad}}}")
        elif k == "walk":
            _, arr, acc, op, write = st
            n = "ARRLEN_" + arr
            out.append(f"{pad}for (pw_{arr} = {arr}; pw_{arr} != {arr} + {n}; pw_{arr}++) {{")
            out.append(f"{pad}    {acc} {op}= *pw_{arr};")
            if write:
                out.append(f"{pad}    *pw_{arr} = {acc};")
            out.append(f"{pad}}}")
        elif k == "for" and st[1] == "G":
            _, _, c, count, body, lab = st
            out.append(f"{pad}{c} = {count};")
            out.append(f"{pad}L{lab}: ;")
            out += c_block(body, ind + 1)
            out.append(f"{pad}if (--{c} > 0) goto L{lab};")
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


def c_helper(h, k: int, n: int) -> str:
    rt = h.rtype
    if h.kind == "pure":
        return f"{rt} h{k}({', '.join(f'{t} {nm}' for nm, t in h.params)}) {{ return {rd(h.expr)}; }}"
    if h.kind == "rec":
        return (f"{rt} h{k}(u8 n, {rt} acc) {{ if (n == 0) return acc; "
                f"return h{k}(n - 1, {rd(h.expr)}); }}")
    if h.kind == "recn":
        return (f"{rt} h{k}(u8 n, {rt} acc) {{ {rt} rv; if (n == 0) return acc; "
                f"rv = h{k}(n - 1, acc); return {rd(h.expr)}; }}")
    if h.kind == "sum":
        return (f"u32 h{k}({h.elem} *p, u8 n) {{ u32 t = 0; "
                f"while (n--) {{ t = (t << 1) ^ (u32)*p++; }} return t; }}")
    if h.kind == "set":
        return f"void h{k}({rt} *p, {rt} v) {{ *p = v; }}"
    return f"{rt} h{k}(struct S{n} *q) {{ return {rd(h.expr)}; }}"


def c_prog(P, n: int, label: str, exp: int, probe: bool = False) -> tuple:
    """(file-scope text, call expression) of program n."""
    g = f"t{n}_"
    out = [f"/* {label}: expect 0x{exp:08X} */"]
    out.append(f"struct S{n} {{ " + " ".join(f"{t} {f};" for f, t in P.fields)
               + "".join(f" {t} {f}[4];" for f, t in P.afields) + " };")
    names = []                                         # file-scope names the statements use bare
    for name in P.gvars:
        out.append(f"{P.vars[name]} {g}{name} = {P.init[name]};")
        names.append(name)
    for name, (t, vals) in P.arrays.items():
        out.append(f"{t} {g}{name}[{len(vals)}] = {{ {', '.join(str(v) for v in vals)} }};")
        names.append(name)
    for name, (t, rows) in P.mats.items():
        out.append(f"{t} {g}{name}[2][4] = {{ " + ", ".join(
            "{ " + ", ".join(str(v) for v in r) + " }" for r in rows) + " };")
        names.append(name)
    def sinit(d):
        parts = [str(d[f]) for f, _ in P.fields]
        parts += ["{ " + ", ".join(str(v) for v in d[f]) + " }" for f, _ in P.afields]
        return "{ " + ", ".join(parts) + " }"

    out.append(f"struct S{n} {g}s = {sinit(P.sinit)};")
    out.append(f"struct S{n} {g}sa[2] = {{ {sinit(P.sainit[0])}, {sinit(P.sainit[1])} }};")
    names += ["s", "sa"]
    if P.union:
        out.append(f"union U{n} {{ u32 l; u16 w[2]; u8 b[4]; s16 sw[2]; }};")
        out.append(f"union U{n} {g}un = {{ {P.uinit}UL }};")
        names.append("un")
    if P.bits:
        out.append(f"struct B{n} {{ " + " ".join(
            f"{'signed' if sg else 'unsigned'} int {nm} : {w};" for nm, sg, w in P.bits) + " u8 pad; };")
        out.append(f"struct B{n} {g}bf = {{ {', '.join(str(P.binit[nm]) for nm, _, _ in P.bits)}, 1 }};")
        names.append("bf")
    names += [f"h{k}" for k in range(len(P.helpers))]
    if P.fptab:
        names += ["fp0", "fp1", "fpt"]
    out += [f"#define {name} {g}{name}" for name in names]
    if P.fptab:
        rt, params, bodies = P.fptab
        sig = ", ".join(f"{t} {nm}" for nm, t in params)
        for k, body in enumerate(bodies):
            out.append(f"{rt} fp{k}({sig}) {{ return {rd(body)}; }}")
        out.append(f"{rt} (*fpt[2])({', '.join(t for _, t in params)}) = {{ fp0, fp1 }};")
    for k, h in enumerate(P.helpers):
        if h.kind == "sum":
            h.elem = P.arrays[h.array][0]
        out.append(c_helper(h, k, n))
    params = ", ".join(f"{P.vars[p]} {p}" for p in P.params) or "void"
    out.append(f"u32 t{n}({params}) {{")
    for name, t in P.vars.items():
        if name not in P.params and name not in P.gvars:
            out.append(f"    {'static ' if name in P.statics else ''}{t} {name} = {P.init[name]};")
    out.append(f"    struct S{n} *ps = &s;")
    out.append("    u32 h = 0x811C9DC5UL;")
    for name, (t, vals) in P.arrays.items():
        out.append(f"    {t} *pw_{name};")
    text = "\n".join(c_block(P.body, 1))
    for name, (t, vals) in P.arrays.items():
        text = text.replace("ARRLEN_" + name, str(len(vals)))
    if text:
        out.append(text)
    mix = [f"(u32){name}" for name in P.vars]
    for name, (t, vals) in P.arrays.items():
        mix += [f"(u32){name}[{k}]" for k in range(len(vals))]
    mix += [f"(u32)s.{f}" for f, _ in P.fields]
    mix += [f"(u32)sa[{k}].{f}" for k in range(2) for f, _ in P.fields]
    for name in P.mats:
        mix += [f"(u32){name}[{i}][{j}]" for i in range(2) for j in range(4)]
    mix += [f"(u32)bf.{nm}" for nm, _, _ in P.bits]
    for base in ["s", "sa[0]", "sa[1]"]:
        mix += [f"(u32){base}.{f}[{k}]" for f, _ in P.afields for k in range(4)]
    if P.union:
        mix.append("(u32)un.l")
    if probe:                 # every piece of state, one by one, for a failing program
        out += [f"    probe[{k}] = {m};" for k, m in enumerate(mix)]
        P.probe_names = [m[5:] for m in mix]
    out += [f"    h = (h ^ {m}) * {FNV_PRIME}UL;" for m in mix]
    out += ["    return h;", "}"]
    out += [f"#undef {name}" for name in names]
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
        if steps >= 16_000_000:
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
    if all(b == 0 for b in raw):
        return ["  (the program never reached its end: nothing was written out)"]
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


def verdicts(cases: list, name: str, luna: str):
    """True per case that fails. A ROM that does not finish (a program that
    crashes takes the others with it) is run again one program at a time; a
    program that does not finish alone fails. A build error is returned as text."""
    res = run_batch(cases, name, luna)
    if not isinstance(res, str):
        return [got != c.expected for c, got in zip(cases, res)]
    if "never reached" not in res:
        return res
    if len(cases) == 1:
        return [True]
    out = []
    for k, c in enumerate(cases):
        one = verdicts([c], f"{name}_one{k}", luna)
        if isinstance(one, str):
            return one
        out += one
    return out


def without(block, path):
    """A copy of the block with the statement at `path` removed."""
    block = copy.deepcopy(block)
    cur = block
    for step in path[:-1]:
        for key in step:
            cur = cur[key]
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
        elif st[0] == "gskip":
            yield from stmt_paths(st[2], prefix + ((n, 2),))
        elif st[0] == "sw":
            for a, arm in enumerate(st[2]):
                yield from stmt_paths(arm[1], prefix + ((n, 2, a, 1),))


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
            res = verdicts(chunk, f"{name}_reduce{rnd}_{i // PROGS_PER_ROM}", luna)
            if isinstance(res, str):
                return best
            hit = next((c for c, bad in zip(chunk, res) if bad), None)
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
    res = verdicts(cases, name, luna)
    if isinstance(res, str):
        return seed, 2, [f"seed {seed}: {res}"]
    msgs = []
    for c, bad in zip(cases, res):
        if bad:
            msgs.append(f"MISMATCH {c.label}: expected 0x{c.expected:08X} (a wrong checksum, or the program never returned)")
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
