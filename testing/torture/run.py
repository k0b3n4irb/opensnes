#!/usr/bin/env python3
"""gcc.c-torture/execute on the target: C conformance, executed on luna.

difftest.py checks integer expressions and small programs against a model; this
runs the programs of a real conformance suite. Each test of gcc's
`gcc.c-torture/execute` is a complete C program that calls abort() when the
compiler got something wrong and exits 0 otherwise. Here each one is compiled by
cc65816, built into a ROM with a small runtime (rt.c: abort, exit, mem*, str*)
and run on luna; the result is read from WRAM.

The tests are GPL-3.0 and are NEVER committed: `--fetch` clones them at the
pinned commit (GCC_SHA) into build/torture/. What is committed is ours: this
script, the runtime, the headers of shim/, and expected.txt, the status of every
test at that commit.

Statuses
  PASS     built and exited 0 (or returned 0 from main)
  FAIL     built, then abort(), a non-zero exit, or no end within the step budget
  NOBUILD  compiled, then did not link / assemble / fit
  REFUSED  cc65816 refused it (a feature the SDK says it does not handle:
           long long, va_start, asm, struct by value, VLA, K&R, ...)

The ratchet (expected.txt): a test expected PASS that no longer passes fails the
run. A test that improved is reported; `--update` rewrites the file.

  python3 testing/torture/run.py --fetch          # clone the tests (once)
  python3 testing/torture/run.py                  # run everything, compare
  python3 testing/torture/run.py --only 20000113-1,pr58943
  python3 testing/torture/run.py --update         # re-capture after an intended change
Exit 0 = nothing that passed regressed, 1 = a regression, 2 = usage / missing tests.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
GCC_REPO = "https://github.com/gcc-mirror/gcc.git"
GCC_SHA = "a74179d08523b1028479752dba7d321bcdb01d81"
SUBDIR = "gcc/testsuite/gcc.c-torture/execute"
DEFAULT_DIR = REPO / "build" / "torture"
EXPECTED = HERE / "expected.txt"
STEPS = 150_000_000          # a slow test (naive mem*) needs more than 8 M instructions

# Why a test is expected to fail although the SDK is not at fault: written by hand,
# one line per test, in expected.txt (third column).

MAKEFILE = """\
OPENSNES := {sdk}
TARGET   := t.sfc
ROM_NAME := TORTURE
USE_LIB  := 1
LIB_MODULES := console
CSRC := t.c rt.c
CFLAGS := -I{shim}
include $(OPENSNES)/make/common.mk
"""

REFUSAL_TAGS = [
    (r"long long|64-bit", "long-long"),
    (r"va_start|va_arg|variadic", "varargs"),
    (r"undeclared identifier: asm|asm", "asm"),
    (r"struct (returns|parameters|pa)|parc|argv|by value", "struct-by-value"),
    (r"variable-length", "vla"),
    (r"long double|_Complex|float", "float"),
    (r"no type in parameter|no type specifier|expected declaration", "k-and-r"),
    (r"GNU attribute", "gnu-attribute"),
    (r"invalid UTF-8", "invalid-utf8"),
    (r"function definition not allowed", "nested-function"),
]


def find_luna():
    p = REPO / "testing" / "bin" / "luna"
    return str(p) if p.is_file() else shutil.which("luna")


def fetch(dest):
    dest.mkdir(parents=True, exist_ok=True)
    g = dest / "gcc"
    if not (g / ".git").is_dir():
        subprocess.run(["git", "init", "-q", str(g)], check=True)
        subprocess.run(["git", "-C", str(g), "remote", "add", "origin", GCC_REPO], check=True)
    have = subprocess.run(["git", "-C", str(g), "rev-parse", "--verify", "-q", "HEAD"],
                          capture_output=True, text=True).stdout.strip()
    if have != GCC_SHA:
        for attempt in range(3):
            r = subprocess.run(["git", "-C", str(g), "fetch", "-q", "--depth", "1", "--filter=blob:none",
                                "origin", GCC_SHA])
            if r.returncode == 0:
                break
        else:
            sys.exit("torture: could not fetch gcc at " + GCC_SHA)
        subprocess.run(["git", "-C", str(g), "sparse-checkout", "set", "--no-cone", "/" + SUBDIR + "/*.c"], check=True)
        subprocess.run(["git", "-C", str(g), "checkout", "-q", "--detach", GCC_SHA], check=True)
    print(f"torture: tests at {GCC_SHA[:10]} in {g / SUBDIR}")


def refusal_tag(text):
    for rx, tag in REFUSAL_TAGS:
        if re.search(rx, text, re.I):
            return tag
    return "other"


def one(args):
    name, src, work, luna, sdk = args
    d = work / name
    d.mkdir(parents=True, exist_ok=True)
    # bytes, not text: some tests hold a raw 0xFF in a string literal, and a text round trip
    # with replacement turns it into three bytes (20000227-1)
    (d / "t.c").write_bytes((HERE / "prelude.h").read_bytes() + (src / f"{name}.c").read_bytes())
    # 1. does cc65816 take it? (fast; a refusal is a documented limit, not a defect)
    r = subprocess.run([str(sdk / "bin" / "cc65816"), str(d / "t.c"), "-o", str(d / "t.asm"),
                        "-I", str(HERE / "shim"), "-I", str(sdk / "lib" / "include")],
                       capture_output=True, text=True)
    if r.returncode != 0:
        msg = (r.stdout + r.stderr)
        first = next((l for l in msg.splitlines() if re.search(r"error|unhandled|not (yet )?supported|Assertion", l)), "?")
        shutil.rmtree(d, ignore_errors=True)
        return name, "REFUSED", refusal_tag(first)
    # 2. build the ROM
    shutil.copy(HERE / "rt.c", d / "rt.c")
    (d / "Makefile").write_text(MAKEFILE.format(sdk=sdk, shim=HERE / "shim"))
    r = subprocess.run(["make", "-s", "-C", str(d), "SKIP_LINT=1"], capture_output=True, text=True)
    if r.returncode != 0 or not (d / "t.sfc").is_file():
        shutil.rmtree(d, ignore_errors=True)
        return name, "NOBUILD", ""
    # 3. run
    out = d / "o.json"
    subprocess.run([luna, "state", str(d / "t.sfc"), "--sym", str(d / "t.sym"), "-n", str(STEPS),
                    "--peek", "torture_result:2", "--out", str(out)], capture_output=True, text=True)
    try:
        j = json.loads(out.read_text())
        hx = []

        def walk(o):
            if isinstance(o, dict):
                if "bytes_hex" in o:
                    hx.append(o["bytes_hex"])
                for v in o.values():
                    walk(v)
            elif isinstance(o, list):
                for v in o:
                    walk(v)
        walk(j)
        val = int.from_bytes(bytes.fromhex(hx[-1]), "little")
        cpu = j.get("cpu", {})
    except Exception:
        shutil.rmtree(d, ignore_errors=True)
        return name, "FAIL", "no-result"
    if val == 0xC0DE:
        status, why = "PASS", ""
    elif val == 0xDEAD:
        status, why = "FAIL", "abort"
    elif val != 0:
        status, why = "FAIL", "exit%04X" % val
    elif cpu.get("stopped"):
        a = cpu.get("a", 1) & 0xFFFF
        status, why = ("PASS", "") if a == 0 else ("FAIL", "ret%04X" % a)
    else:
        status, why = "FAIL", "hang"
    shutil.rmtree(d, ignore_errors=True)
    return name, status, why


def load_expected():
    exp = {}
    if EXPECTED.is_file():
        for l in EXPECTED.read_text().splitlines():
            if not l.strip() or l.startswith("#"):
                continue
            p = l.split(None, 2)
            exp[p[0]] = (p[1], p[2] if len(p) > 2 else "")
    return exp


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--fetch", action="store_true", help="clone the tests at the pinned commit and stop")
    ap.add_argument("--dir", default=os.environ.get("TORTURE_DIR", str(DEFAULT_DIR)))
    ap.add_argument("--only", help="comma-separated test names")
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("-v", "--verbose", action="store_true", help="one line per test")
    ap.add_argument("--update", action="store_true", help="rewrite expected.txt from this run")
    args = ap.parse_args()
    dest = Path(args.dir)
    if args.fetch:
        fetch(dest)
        return 0
    src = dest / "gcc" / SUBDIR
    if not src.is_dir():
        print(f"torture: no tests in {src}; run with --fetch first", file=sys.stderr)
        return 2
    luna = find_luna()
    if not luna:
        print("torture: luna not found (scripts/install-luna.sh)", file=sys.stderr)
        return 2
    names = sorted(p.stem for p in src.glob("*.c"))
    if args.only:
        want = set(args.only.split(","))
        names = [n for n in names if n in want]
    work = Path(tempfile.mkdtemp(prefix="torture-", dir=str(dest)))
    results = {}
    try:
        with ThreadPoolExecutor(max_workers=args.jobs) as ex:
            for n, s, why in ex.map(one, [(n, src, work, luna, REPO) for n in names]):
                results[n] = (s, why)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    c = Counter(s for s, _ in results.values())
    if args.verbose:
        for n in sorted(results):
            print(f"  {n} {results[n][0]} {results[n][1]}")
    print(f"torture: {len(results)} tests: " + ", ".join(f"{k} {c[k]}" for k in ("PASS", "FAIL", "NOBUILD", "REFUSED") if c[k]))
    refused = Counter(w for s, w in results.values() if s == "REFUSED")
    if refused:
        print("  refused by cc65816: " + ", ".join(f"{t} {n}" for t, n in refused.most_common()))
    exp = load_expected()
    if args.update:
        old = exp
        lines = ["# status of gcc.c-torture/execute at " + GCC_SHA[:10] + " on the target (testing/torture/run.py --update)",
                 "# name STATUS reason; a test expected PASS must keep passing"]
        for n in sorted(results):
            s, why = results[n]
            note = old[n][1].split(" | ", 1)[1] if n in old and " | " in old[n][1] else ""
            if note:
                why = (why + " | " + note).strip()   # keep the hand-written reason of a known failure
            lines.append(f"{n} {s}" + (f" {why}" if why else ""))
        EXPECTED.write_text("\n".join(lines) + "\n")
        print(f"torture: wrote {EXPECTED.relative_to(REPO)} ({len(results)} tests)")
        return 0
    if not exp:
        print("torture: no expected.txt: run with --update first", file=sys.stderr)
        return 2
    regress = [n for n, (s, _) in results.items() if exp.get(n, ("", ""))[0] == "PASS" and s != "PASS"]
    better = [n for n, (s, _) in results.items() if s == "PASS" and exp.get(n, ("", ""))[0] not in ("PASS", "")]
    new = [n for n in results if n not in exp]
    for n in sorted(regress):
        print(f"  REGRESSION {n}: expected PASS, now {results[n][0]} {results[n][1]}")
    if better:
        print(f"  improved ({len(better)}): " + " ".join(sorted(better)[:12]) + (" ..." if len(better) > 12 else "") + "  (--update)")
    if new:
        print(f"  not in expected.txt ({len(new)}): " + " ".join(sorted(new)[:12]))
    print("torture: " + ("REGRESSION" if regress else "no regression") + f" ({c['PASS']} pass)")
    return 1 if regress else 0


if __name__ == "__main__":
    sys.exit(main())
