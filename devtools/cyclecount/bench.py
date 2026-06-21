#!/usr/bin/env python3
"""Compiler cycle-count benchmark (re-homed off opensnes-emu).

Compiles `bench_functions.c` with `bin/cc65816`, runs `cyclecount.py` for a
per-function cycle estimate, and compares against a committed baseline — a
codegen cycle-regression guard. This was `opensnes-emu/test/run-benchmark.mjs`
(removed with the submodule); re-homed here, dependency-free.

  make bench                                   # compare vs baseline (exit 1 on regression)
  python3 devtools/cyclecount/bench.py --update  # re-baseline (after an intentional change)

Thresholds (as before): total > +5%, OR per-function > +25% AND > +50 abs cycles.

NOTE (feature L5 upgrade): `cyclecount.py` is a *static* estimate. luna v0.3.0's
`--cpu-trace` gives ground-truth cycles; cross-checking the estimate against a
ROM-harness `--cpu-trace` run is the planned upgrade (see README).
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
CC = REPO / "bin" / "cc65816"
SRC = HERE / "bench_functions.c"
BASELINE = HERE / "bench_baseline.json"


def measure() -> dict[str, int]:
    with tempfile.NamedTemporaryFile(suffix=".asm", delete=False) as tf:
        asm = Path(tf.name)
    r = subprocess.run([str(CC), str(SRC), "-o", str(asm)],
                       capture_output=True, text=True, timeout=60)
    if not asm.is_file() or asm.stat().st_size == 0:
        sys.exit(f"compile failed: {(r.stderr or r.stdout).strip()[:300]}")
    j = subprocess.run([sys.executable, str(HERE / "cyclecount.py"), "--json", str(asm)],
                       capture_output=True, text=True)
    fns = json.loads(j.stdout)["functions"]
    return {k: v["cycles"] for k, v in fns.items()}


def main() -> int:
    ap = argparse.ArgumentParser(description="cc65816 cycle-count benchmark")
    ap.add_argument("--update", action="store_true", help="(re)write the baseline")
    args = ap.parse_args()
    if not CC.is_file():
        sys.exit(f"ERROR: {CC} not found — build the toolchain (make compiler) first")
    cur = measure()
    cur_total = sum(cur.values())

    if args.update:
        BASELINE.write_text(json.dumps(dict(sorted(cur.items())), indent=2) + "\n")
        print(f"baseline written: {len(cur)} functions, {cur_total} total cycles")
        return 0

    if not BASELINE.is_file():
        sys.exit("no baseline — run with --update first")
    base = json.loads(BASELINE.read_text())
    base_total = sum(base.values())

    fails = []
    for name in sorted(cur):
        b = base.get(name)
        if b is None:
            print(f"  NEW  {name}: {cur[name]} cycles (not in baseline)")
            continue
        d = cur[name] - b
        if d > 0 and d > 0.25 * b and d > 50:
            fails.append(f"{name}: {b}→{cur[name]} (+{d}, +{100 * d / b:.0f}%)")
    td = cur_total - base_total
    if base_total and td > 0.05 * base_total:
        fails.append(f"TOTAL: {base_total}→{cur_total} (+{td}, +{100 * td / base_total:.0f}%)")

    if fails:
        print("CYCLE REGRESSION:")
        for f in fails:
            print("  " + f)
        print("(intentional? re-baseline with --update and record why in the commit)")
        return 1
    print(f"OK: {len(cur)} functions, {cur_total} total cycles "
          f"(baseline {base_total}), no regression")
    return 0


if __name__ == "__main__":
    sys.exit(main())
