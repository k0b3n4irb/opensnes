#!/usr/bin/env python3
"""Compiler C→ASM pattern checks (re-homed into the repo).

These guard cc65816 codegen: each `cases/<name>.c` is compiled to assembly with
`bin/cc65816` and its output is matched against declarative rules in
`cases/<name>.checks`. They are pure compile-time checks — no emulator — and were
previously the "60 compiler tests" inside the removed opensnes-emu submodule
(JS port of the older `tests/compiler/run_tests.sh`). Re-homed here, dependency-
free, alongside the other devtools linters.

Every `cases/<name>.c` is compiled — a fixture without a `.checks` file still
runs as a compile-only case (a cproc/QBE crash or empty output fails it). The
number of unchecked fixtures is RATCHETED by MAX_UNCHECKED below: adding a new
fixture without assertions fails the run. Port a fixture, then lower the
constant — never raise it (same pattern as BANK0_FAIL_THRESHOLD).

Run:  python3 devtools/compiler-tests/run.py        # all cases (checked + compile-only)
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

`cases/negative/<name>.c` + `<name>.expect` pin C the toolchain must REFUSE
(variadic functions, struct by value, inline asm): the compile must fail and
stderr must contain the `.expect` text. A refusal that turns into a compile is
reported as a failure too — the feature landed, promote the fixture.

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
NEGATIVE = CASES / "negative"
CC = REPO_ROOT / "bin" / "cc65816"

# Ratchet on fixtures lacking a .checks file. 56 of 66 cases predate the
# .checks DSL and run compile-only. Porting a fixture lowers this number;
# it must NEVER go up — a new fixture ships with its assertions.
MAX_UNCHECKED = 55  # new fixtures ship WITH checks; test_function_ptr got its .checks 2026-09-11


def compile_result(src: Path) -> tuple[bool, str]:
    """(compiled?, stderr+stdout) — for the negative fixtures."""
    with tempfile.NamedTemporaryFile(suffix=".asm", delete=False) as tf:
        out = Path(tf.name)
    proc = subprocess.run([str(CC), f"-I{REPO_ROOT / 'lib' / 'include'}",
                           str(src), "-o", str(out)],
                          capture_output=True, text=True, timeout=60)
    ok = out.is_file() and out.stat().st_size > 0 and proc.returncode == 0
    return ok, (proc.stderr or "") + (proc.stdout or "")


def run_negative(only: str | None) -> tuple[int, int]:
    """Refusal fixtures: must NOT compile, and must say why. Returns (pass, fail)."""
    passed = failed = 0
    for src in sorted(NEGATIVE.glob("*.c")):
        name = f"negative/{src.stem}"
        if only and only not in name:
            continue
        expect_file = src.with_suffix(".expect")
        want = expect_file.read_text().strip() if expect_file.is_file() else ""
        compiled, msg = compile_result(src)
        if compiled:
            print(f"  FAIL {name}: compiled — the feature landed? promote the fixture to cases/")
            failed += 1
        elif want and want not in msg:
            print(f"  FAIL {name}: refused, but stderr lacks {want!r}: {msg.strip()[:200]}")
            failed += 1
        else:
            print(f"  PASS {name} (refused: {want or 'any error'})")
            passed += 1
    return passed, failed


def compile_asm(src: Path) -> str:
    with tempfile.NamedTemporaryFile(suffix=".asm", delete=False) as tf:
        out = Path(tf.name)
    # SDK include path: fixtures may use <snes/*.h> (e.g. test_metasprite).
    proc = subprocess.run([str(CC), f"-I{REPO_ROOT / 'lib' / 'include'}",
                           str(src), "-o", str(out)],
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
    sources = sorted(CASES.glob("*.c"))
    passed = failed = compile_only = unchecked = 0
    for src in sources:
        name = src.stem
        cf = CASES / f"{name}.checks"
        if not cf.is_file():
            unchecked += 1
        if only and only not in name:
            continue
        try:
            asm = compile_asm(src)
        except RuntimeError as e:
            print(f"  FAIL {name}: {e}")
            failed += 1
            continue
        if not cf.is_file():
            # Compile-only tier: no assertions yet, but a toolchain crash or
            # empty output on this fixture still fails the suite.
            compile_only += 1
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
    neg_pass, neg_fail = run_negative(only)
    failed += neg_fail
    print(f"\nCompiler checks: {passed} passed, {failed} failed"
          f" (+{compile_only} compile-only; {unchecked}/{MAX_UNCHECKED} unchecked ratchet;"
          f" {neg_pass}/{neg_pass + neg_fail} refusals pinned)")
    if only is None and unchecked > MAX_UNCHECKED:
        print(f"ERROR: {unchecked} fixtures lack a .checks file, ratchet allows "
              f"{MAX_UNCHECKED}. New fixtures ship with assertions — port the "
              f"fixture or write its .checks (see --list).")
        return 1
    if only is None and unchecked < MAX_UNCHECKED:
        print(f"NOTE: unchecked count dropped to {unchecked} — lower MAX_UNCHECKED "
              f"in run.py to lock in the progress.")
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
