#!/usr/bin/env python3
"""Run the upstream test suites of cproc, QBE and wla-dx on the fork's binaries.

Gaps review item H1 (2026-09-13). Each submodule ships its own suite, and none
had ever run on the OpenSNES forks: a PIN bump was validated by SHA only. This
runner drives the three suites as upstream wrote them and grades every test
against a known-fail ratchet in devtools/toolchain-suites/:

  - a test not listed there that fails  -> regression, exit 1;
  - a listed test that passes           -> XPASS, exit 1 (shrink the list);
  - a listed test that fails            -> expected.

What each suite proves on this fork:

  cproc   compiler/cproc/runtests semantics: cproc-qbe's QBE IL diffed against
          expected files written for the host ABI. The fork's 16-bit int and
          rodata sectioning make 107 of 170 expected outputs unmatchable
          (cproc_known_fail.txt explains); the 63 that match are the ratchet.
  qbe     compiler/qbe/tools/test.sh: every test/*.ssa compiled for the HOST
          target, linked with cc, executed (test.sh skips the ones marked
          for another target). The only execution test of the shared
          middle-end passes the fork patched. Expected: every test green.
  wla-dx  the tests/{65816,spc-700,superfx} projects assembled and linked with
          the fork's binaries, bytes checked by byte_tester. Expected 31/32:
          base_test_1 asserts the .BASE-on-RAMSECTION semantics the fork's one
          wla-dx patch reverses on purpose.

The binaries are the ones the submodule builds leave in place (make compiler):
compiler/cproc/cproc-qbe, compiler/qbe/qbe, compiler/wla-dx/binaries/*. Run
under ASAN_OPTIONS / UBSAN_OPTIONS as make test-sanitizers does, the suites
also become a sanitizer workload (that is how CI runs them).
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
COMPILER = ROOT / "compiler"
KNOWN = ROOT / "devtools" / "toolchain-suites"


def known_fail(name: str) -> set[str]:
    path = KNOWN / f"{name}_known_fail.txt"
    out: set[str] = set()
    for line in path.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            out.add(line)
    return out


def grade(suite: str, results: dict[str, bool], expected_fail: set[str]) -> int:
    """Print the verdict of one suite and return the number of problems."""
    passed = [t for t, ok in results.items() if ok]
    failed = [t for t, ok in results.items() if not ok]
    regressions = sorted(t for t in failed if t not in expected_fail)
    xpass = sorted(t for t in passed if t in expected_fail)
    stale = sorted(t for t in expected_fail if t not in results)
    print(f"{suite}: {len(passed)} pass, {len(failed)} fail "
          f"({len(failed) - len(regressions)} expected), "
          f"{len(regressions)} regression(s), {len(xpass)} XPASS")
    for t in regressions:
        print(f"  REGRESSION {t}: fails and is not in {suite}_known_fail.txt")
    for t in xpass:
        print(f"  XPASS {t}: passes — remove it from {suite}_known_fail.txt")
    for t in stale:
        print(f"  note: {t} is listed in {suite}_known_fail.txt but the suite has no such test")
    return len(regressions) + len(xpass)


# --------------------------------------------------------------------------
# cproc — mirrors compiler/cproc/runtests, one result per test
# --------------------------------------------------------------------------

def run_cproc() -> dict[str, bool]:
    cdir = COMPILER / "cproc"
    ccqbe = cdir / "cproc-qbe"
    if not ccqbe.exists():
        ccqbe = ROOT / "bin" / "cproc-qbe"
    if not ccqbe.exists():
        sys.exit("cproc: no cproc-qbe binary (make compiler first)")
    results: dict[str, bool] = {}
    with tempfile.TemporaryDirectory() as tmp:
        got = Path(tmp) / "got"
        for src in sorted((cdir / "test").glob("*.c")):
            stem = src.with_suffix("")
            arch = stem.name.split("+", 1)[1] if "+" in stem.name else "x86_64-sysv"
            rel = f"test/{src.name}"   # the path runtests passes; it shows in -E output
            if stem.with_suffix(".qbe").exists():
                want = stem.with_suffix(".qbe")
                cmd = [str(ccqbe), "-t", arch, "-o", str(got), rel]
            elif stem.with_suffix(".pp").exists():
                want = stem.with_suffix(".pp")
                cmd = [str(ccqbe), "-t", arch, "-E", "-o", str(got), rel]
            else:
                continue
            got.unlink(missing_ok=True)
            proc = subprocess.run(cmd, cwd=cdir, capture_output=True)
            ok = proc.returncode == 0 and got.exists() and got.read_bytes() == want.read_bytes()
            results[src.name] = ok
    return results


# --------------------------------------------------------------------------
# QBE — tools/test.sh all, parsed per test
# --------------------------------------------------------------------------

def run_qbe() -> dict[str, bool]:
    qdir = COMPILER / "qbe"
    qbe = qdir / "qbe"
    if not qbe.exists():
        sys.exit("qbe: no qbe binary (make compiler first)")
    env = dict(os.environ, bin=str(qbe))
    proc = subprocess.run(["sh", "tools/test.sh", "all"], cwd=qdir, env=env,
                          capture_output=True, text=True)
    results: dict[str, bool] = {}
    current = None
    for line in proc.stdout.splitlines():
        m = re.match(r"^(\S+\.ssa)\.\.\.", line)
        if m:
            current = m.group(1)
            results[current] = True
        if current and re.search(r"\[[^\]]*fail\]", line):
            results[current] = False
    if not results:
        print(proc.stdout[-2000:], proc.stderr[-2000:], file=sys.stderr)
        sys.exit("qbe: tools/test.sh produced no results (missing cc?)")
    return results


# --------------------------------------------------------------------------
# wla-dx — the 65816 / spc-700 / superfx projects with byte_tester
# --------------------------------------------------------------------------

PLATFORMS = ("65816", "spc-700", "superfx")


def run_wla_dx() -> dict[str, bool]:
    wdir = COMPILER / "wla-dx"
    binaries = wdir / "binaries"
    for b in ("wla-65816", "wla-spc700", "wla-superfx", "wlalink"):
        if not (binaries / b).exists():
            sys.exit(f"wla-dx: {binaries / b} missing (make compiler first)")
    # byte_tester is not part of the four targets compiler/Makefile builds
    bt = wdir / "byte_tester"
    if subprocess.run(["make", "-s", "-C", str(bt)], capture_output=True).returncode != 0:
        sys.exit("wla-dx: cannot build byte_tester")
    shutil.copy2(bt / "byte_tester", binaries / "byte_tester")
    env = dict(os.environ)
    env["PATH"] = f"{binaries}{os.pathsep}{env.get('PATH', '')}"
    env["WLAVALGRIND"] = ""   # the makefiles prefix every tool with it
    results: dict[str, bool] = {}
    for plat in PLATFORMS:
        for tdir in sorted((wdir / "tests" / plat).iterdir()):
            if not tdir.is_dir() or tdir.name.startswith("_") or not (tdir / "makefile").exists():
                continue
            name = f"{plat}/{tdir.name}"
            ok = True
            subprocess.run(["make", "-s", "clean"], cwd=tdir, env=env, capture_output=True)
            if subprocess.run(["make", "-s"], cwd=tdir, env=env, capture_output=True).returncode != 0:
                ok = False
            elif (tdir / "testsfile").exists():
                if subprocess.run(["byte_tester", "testsfile"], cwd=tdir, env=env,
                                  capture_output=True).returncode != 0:
                    ok = False
            subprocess.run(["make", "-s", "clean"], cwd=tdir, env=env, capture_output=True)
            results[name] = ok
    return results


SUITES = {"cproc": run_cproc, "qbe": run_qbe, "wla-dx": run_wla_dx}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--only", choices=sorted(SUITES), help="run one suite")
    args = ap.parse_args()
    problems = 0
    for name, fn in SUITES.items():
        if args.only and name != args.only:
            continue
        results = fn()
        problems += grade(name, results, known_fail(name.replace("-", "_")))
    if problems:
        print(f"TOOLCHAIN SUITES: {problems} problem(s)")
        return 1
    print("TOOLCHAIN SUITES: OK (every deviation from upstream is a listed, explained one)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
