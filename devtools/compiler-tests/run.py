#!/usr/bin/env python3
"""Compiler C→ASM pattern checks (re-homed into the repo).

These guard cc65816 codegen: each `cases/<name>.c` is compiled to assembly with
`bin/cc65816` and its output is matched against declarative rules in
`cases/<name>.checks`. They are pure compile-time checks — no emulator — and were
previously the "60 compiler tests" inside the removed opensnes-emu submodule
(JS port of the older `tests/compiler/run_tests.sh`). Re-homed here, dependency-
free, alongside the other devtools linters.

Run:  python3 devtools/compiler-tests/run.py        # all cases that have .checks
      python3 devtools/compiler-tests/run.py --only tail_call
      python3 devtools/compiler-tests/run.py --list  # cases missing a .checks (TODO)

.checks DSL (one directive per line; '#' comments and blank lines ignored):
    present <regex>                 ASM must contain regex
    absent  <regex>                 ASM must NOT contain regex
    count   <N> <regex>             exactly N matches in the ASM
    in <func>: present <regex>      regex present within that function's body
    in <func>: absent  <regex>      regex absent within that function's body
    section <sym>: present <regex>   the .SECTION/.RAMSECTION line of <sym> matches
    section <sym>: absent  <regex>   ...does not match

Exit 0 = all pass, 1 = any failure / compile error.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
CASES = Path(__file__).resolve().parent / "cases"
CC = REPO_ROOT / "bin" / "cc65816"


def compile_asm(src: Path) -> str:
    with tempfile.NamedTemporaryFile(suffix=".asm", delete=False) as tf:
        out = Path(tf.name)
    proc = subprocess.run([str(CC), str(src), "-o", str(out)],
                          capture_output=True, text=True, timeout=60)
    if not out.is_file() or out.stat().st_size == 0:
        raise RuntimeError(f"compile failed: {(proc.stderr or proc.stdout).strip()[:300]}")
    return out.read_text()


def func_body(asm: str, name: str) -> str:
    """From `name:` label to the next `.ENDS` (the section close)."""
    lines = asm.splitlines()
    start = next((i for i, l in enumerate(lines) if re.match(rf"^{re.escape(name)}:", l)), -1)
    if start < 0:
        return ""
    out = []
    for l in lines[start:]:
        out.append(l)
        if ".ENDS" in l:
            break
    return "\n".join(out)


def section_of(asm: str, sym: str) -> str:
    """The nearest .SECTION/.RAMSECTION line above `sym:`."""
    lines = asm.splitlines()
    idx = next((i for i, l in enumerate(lines) if re.match(rf"^{re.escape(sym)}:", l)), -1)
    if idx < 0:
        return ""
    for i in range(idx - 1, -1, -1):
        if re.search(r"\.(SECTION|RAMSECTION)", lines[i]):
            return lines[i]
    return ""


def apply_check(asm: str, line: str) -> str | None:
    """Return an error string if the directive fails, else None."""
    m = re.match(r"in\s+(\S+):\s*(present|absent)\s+(.+)", line)
    if m:
        fn, mode, pat = m.group(1), m.group(2), m.group(3)
        body = func_body(asm, fn)
        if not body:
            return f"function '{fn}' not found"
        found = re.search(pat, body) is not None
        if mode == "present" and not found:
            return f"in {fn}: missing /{pat}/"
        if mode == "absent" and found:
            return f"in {fn}: unexpected /{pat}/"
        return None
    m = re.match(r"section\s+(\S+):\s*(present|absent)\s+(.+)", line)
    if m:
        sym, mode, pat = m.group(1), m.group(2), m.group(3)
        sec = section_of(asm, sym)
        if not sec:
            return f"symbol '{sym}' not found"
        found = re.search(pat, sec) is not None
        if mode == "present" and not found:
            return f"section of {sym} (/{sec.strip()}/) missing /{pat}/"
        if mode == "absent" and found:
            return f"section of {sym} (/{sec.strip()}/) unexpectedly matches /{pat}/"
        return None
    m = re.match(r"count\s+(\d+)\s+(.+)", line)
    if m:
        n, pat = int(m.group(1)), m.group(2)
        got = len(re.findall(pat, asm))
        return None if got == n else f"count /{pat}/ = {got}, expected {n}"
    m = re.match(r"(present|absent)\s+(.+)", line)
    if m:
        mode, pat = m.group(1), m.group(2)
        found = re.search(pat, asm) is not None
        if mode == "present" and not found:
            return f"missing /{pat}/"
        if mode == "absent" and found:
            return f"unexpected /{pat}/"
        return None
    return f"unparseable directive: {line!r}"


def run(only: str | None) -> int:
    checks = sorted(CASES.glob("*.checks"))
    passed = failed = 0
    for cf in checks:
        name = cf.stem
        if only and only not in name:
            continue
        src = CASES / f"{name}.c"
        if not src.is_file():
            print(f"  FAIL {name}: missing {src.name}")
            failed += 1
            continue
        try:
            asm = compile_asm(src)
        except RuntimeError as e:
            print(f"  FAIL {name}: {e}")
            failed += 1
            continue
        errs = []
        for raw in cf.read_text().splitlines():
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            e = apply_check(asm, line)
            if e:
                errs.append(e)
        if errs:
            print(f"  FAIL {name}: " + "; ".join(errs))
            failed += 1
        else:
            print(f"  PASS {name}")
            passed += 1
    print(f"\nCompiler checks: {passed} passed, {failed} failed")
    return 1 if failed else 0


def main() -> int:
    ap = argparse.ArgumentParser(description="cc65816 C→ASM pattern checks")
    ap.add_argument("--only", metavar="SUBSTR")
    ap.add_argument("--list", action="store_true",
                    help="list fixtures that still lack a .checks file (TODO to port)")
    args = ap.parse_args()
    if args.list:
        cs = {p.stem for p in CASES.glob("*.c")}
        done = {p.stem for p in CASES.glob("*.checks")}
        todo = sorted(cs - done)
        print(f"{len(done)}/{len(cs)} fixtures have .checks; {len(todo)} TODO:")
        for t in todo:
            print(f"  {t}")
        return 0
    if not CC.is_file():
        sys.exit(f"ERROR: {CC} not found — build the toolchain (make compiler) first")
    return run(args.only)


if __name__ == "__main__":
    sys.exit(main())
