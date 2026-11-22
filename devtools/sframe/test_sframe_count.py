#!/usr/bin/env python3
"""The faulted run of L4 S0: generated programs whose answer is known, counted by sframe_count.py.

Each case is compiled by bin/cc65816 with QBE_SFRAME_DUMP=1 (and, for some, a hand-written assembly file), then
counted without the SDK's assembly. The count must refuse a real re-entry under the models that can see it, and
accept the program without re-entry; with the unknown-edge rule broken (SFRAME_COUNT_FAULT=no_unknown) the case
whose re-entry goes through an opaque assembly routine must pass, which proves the case sees that rule.

usage: python3 devtools/sframe/test_sframe_count.py      (needs bin/cc65816; `make count-sframe`)
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
import sframe_count as S  # noqa: E402

CC = os.path.join(ROOT, "bin", "cc65816")

# f keeps `a` across its calls (a stack temp), and has two call sites (one would be inlined whole)
F_CALLS_X = "static int f(int a){ int r = x(a); return r + x(a + 1) + a; }\nint e0(int a){ return f(a) + f(a + 2); }\n"
CASES = {
    # 1a: f -> B.x -> A.e0 -> f, B in C
    "cross_c": {"a.c": "extern int x(int);\n" + F_CALLS_X,
                "b.c": "extern int e0(int);\nint x(int a){ if (a > 100) return a; return e0(a + 50); }\n"},
    # 1b: the same through an assembly routine that jumps through a pointer (opaque: UNKNOWN)
    "cross_asm_indirect": {"a.c": "extern int x(int);\n" + F_CALLS_X,
                           "b.asm": ".SECTION \".text.x\" SUPERFREE\nx:\n\tjml [x_ptr]\n.ENDS\n"},
    # 1c: the same through an assembly routine that calls e0 by name (a known edge)
    "cross_asm_named": {"a.c": "extern int x(int);\n" + F_CALLS_X,
                        "b.asm": ".SECTION \".text.x\" SUPERFREE\nx:\n\tjsl e0\n\trtl\n.ENDS\n"},
    # 2: Fable's shape in one file, no re-entry of f: f -> e1 -> h; f and h must not overlap
    "same_file": {"a.c": "extern int g(int);\n"
                         "static int h(int a){ int r = g(a); return r + g(a + 3) + a; }\n"
                         "int e1(int a){ return h(a) + h(a + 1); }\n"
                         "static int f(int a){ int r = e1(a); return r + e1(a + 1) + a; }\n"
                         "int e0(int a){ return f(a) + f(a + 2); }\n",
                  "b.c": "int g(int a){ return a + 1; }\n"},
    # 2b: re-entry inside the file is a cycle of the file's graph: f is not a candidate
    "same_file_cycle": {"a.c": "int e1(int a);\n"
                               "static int f(int a){ int r = e1(a); return r + e1(a + 1) + a; }\n"
                               "int e1(int a){ if (a > 9) return a; return f(a + 1) + f(a + 2); }\n"},
    # 3: through a pointer: f -> *fp -> e0 -> f
    "indirect": {"a.c": "int e0(int a);\nint (*fp)(int) = e0;\n"
                        "static int f(int a){ int r = fp(a); return r + fp(a + 1) + a; }\n"
                        "int e0(int a){ if (a > 9) return a; return f(a) + f(a + 2); }\n"},
    # 4: no re-entry
    "none": {"a.c": "extern int g(int);\nstatic int f(int a){ int r = g(a); return r + g(a + 1) + a; }\n"
                    "int e0(int a){ return f(a) + f(a + 2); }\n",
             "b.c": "int g(int a){ return a * 3; }\n"},
}

# expected: model -> 'refused' | 'accepted' | 'nocand' for f (and for same_file, the no-overlap check)
EXPECT = {
    "cross_c": {"coarse": "refused", "closure": "refused", "m2": "refused"},
    "cross_asm_indirect": {"coarse": "refused", "closure": "refused", "m2": "accepted"},
    "cross_asm_named": {"coarse": "refused", "closure": "refused", "m2": "accepted"},
    "same_file": {"coarse": "accepted", "closure": "accepted", "m2": "accepted"},
    "same_file_cycle": {"coarse": "nocand", "closure": "nocand", "m2": "nocand"},
    "indirect": {"coarse": "refused", "closure": "refused", "m2": "accepted"},
    "none": {"coarse": "accepted", "closure": "accepted", "m2": "accepted"},
}
# with SFRAME_COUNT_FAULT=no_unknown
EXPECT_FAULT = {
    "cross_asm_indirect": {"closure": "accepted"},   # the case sees the unknown-edge rule
    "cross_asm_named": {"closure": "refused"},       # a named edge is not an unknown one
}


def build(name, files, d):
    for fn, txt in files.items():
        with open(os.path.join(d, fn), "w") as fh:
            fh.write(txt)
        if fn.endswith(".c"):
            env = dict(os.environ, QBE_SFRAME_DUMP="1")
            r = subprocess.run([CC, os.path.join(d, fn), "-o", os.path.join(d, fn + ".asm")], env=env,
                               capture_output=True, text=True)
            if r.returncode:
                raise SystemExit(f"{name}: {fn} does not compile: {r.stderr}")


def verdict(o, model, fn):
    r = o["models"][model]
    full = [n for n in list(r["accepted"]) + r["refused_names"] if n.split(".")[0] == fn]
    if not full:
        return "nocand", None
    n = full[0]
    return ("refused" if n in r["refused_names"] else "accepted"), n


def main():
    if not os.access(CC, os.X_OK):
        raise SystemExit("bin/cc65816 is missing: build the SDK first (make)")
    bad = 0
    with tempfile.TemporaryDirectory(prefix="sframe_count_") as tmp:
        for name, files in CASES.items():
            d = os.path.join(tmp, name)
            os.makedirs(d)
            build(name, files, d)
            for fault, table in (("", EXPECT), ("no_unknown", EXPECT_FAULT)):
                if name not in table:
                    continue
                S.FAULT = fault
                o = S.analyse([d], None, [d], None, name)
                for model, want in table[name].items():
                    got, full = verdict(o, model, "f")
                    ok = got == want
                    if ok and got == "refused" and full not in o["models"][model]["refused_paths"]:
                        ok = False
                    path = " -> ".join(o["models"][model]["refused_paths"].get(full, [])) if got == "refused" else ""
                    print(f"{'ok ' if ok else 'BAD'} {name:20s} {model:8s} fault={fault or '-':10s} f {got}"
                          + (f" ({path})" if path else ""))
                    bad += not ok
                if name == "same_file" and not fault:
                    for model in ("coarse", "closure", "m2"):
                        pl = o["models"][model]["place"]["temps"]
                        off, sz = pl["offsets"], pl["sizes"]
                        fs = [n for n in off if n.startswith("f.")]
                        hs = [n for n in off if n.startswith("h.")]
                        ok = bool(fs and hs) and sz[fs[0]] > 0 and off[hs[0]] >= off[fs[0]] + sz[fs[0]]
                        print(f"{'ok ' if ok else 'BAD'} {name:20s} {model:8s} h placed past f: f@{off.get(fs[0]) if fs else '?'}"
                              f"+{sz.get(fs[0]) if fs else '?'}, h@{off.get(hs[0]) if hs else '?'}")
                        bad += not ok
    S.FAULT = ""
    print("sframe_count fixtures:", "FAIL" if bad else "PASS")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
