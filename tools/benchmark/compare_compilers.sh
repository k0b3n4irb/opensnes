#!/usr/bin/env bash
# compare_compilers.sh — Benchmark OpenSNES vs PVSnesLib compiler output
#
# Compiles bench_functions.c with both toolchains, runs cyclecount.py --json
# on each output, and displays a side-by-side comparison table.
#
# Requirements:
#   OPENSNES_HOME  — path to OpenSNES SDK root (default: script's grandparent)
#   PVSNESLIB_HOME — path to PVSnesLib root (default: ~/workspace/pvsneslib)
#
# Usage:
#   tools/benchmark/compare_compilers.sh
#   PVSNESLIB_HOME=/path/to/pvsneslib tools/benchmark/compare_compilers.sh

set -euo pipefail

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# Always resolve from script location to avoid stale env vars
OPENSNES_HOME="$(cd "$SCRIPT_DIR/../.." && pwd)"
PVSNESLIB_HOME="${PVSNESLIB_HOME:-$HOME/workspace/pvsneslib}"

BENCH_SRC="${OPENSNES_HOME}/tests/benchmark/bench_functions.c"
CYCLECOUNT="${OPENSNES_HOME}/tools/cyclecount/cyclecount.py"

CC65816="${OPENSNES_HOME}/bin/cc65816"
TCC816="${PVSNESLIB_HOME}/devkitsnes/bin/816-tcc"
OPT816="${PVSNESLIB_HOME}/devkitsnes/tools/816-opt"

TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

# ---------------------------------------------------------------------------
# Validate
# ---------------------------------------------------------------------------
missing=0
for tool in "$CC65816" "$TCC816" "$OPT816"; do
    if [ ! -x "$tool" ]; then
        echo "ERROR: not found or not executable: $tool" >&2
        missing=1
    fi
done
if [ ! -f "$BENCH_SRC" ]; then
    echo "ERROR: benchmark source not found: $BENCH_SRC" >&2
    missing=1
fi
if [ ! -f "$CYCLECOUNT" ]; then
    echo "ERROR: cyclecount.py not found: $CYCLECOUNT" >&2
    missing=1
fi
if [ "$missing" -eq 1 ]; then
    exit 1
fi

# ---------------------------------------------------------------------------
# Compile
# ---------------------------------------------------------------------------
echo "Compiling with OpenSNES (cc65816)..."
"$CC65816" "$BENCH_SRC" -o "$TMPDIR/opensnes.asm" 2>/dev/null

echo "Compiling with PVSnesLib (816-tcc)..."
"$TCC816" -Wall -c "$BENCH_SRC" -o "$TMPDIR/pvs_raw.ps" 2>/dev/null

echo "Optimizing with 816-opt..."
# 816-opt prints optimization stats to stdout before the asm; redirect stderr
"$OPT816" "$TMPDIR/pvs_raw.ps" > "$TMPDIR/pvs_opt.asm" 2>/dev/null

# ---------------------------------------------------------------------------
# Analyze
# ---------------------------------------------------------------------------
echo "Analyzing cycle counts..."
python3 "$CYCLECOUNT" --json "$TMPDIR/pvs_raw.ps"    > "$TMPDIR/pvs_raw.json"
python3 "$CYCLECOUNT" --json "$TMPDIR/pvs_opt.asm"   > "$TMPDIR/pvs_opt.json"
python3 "$CYCLECOUNT" --json "$TMPDIR/opensnes.asm"   > "$TMPDIR/opensnes.json"

# ---------------------------------------------------------------------------
# Build comparison table
# ---------------------------------------------------------------------------
python3 - "$TMPDIR/pvs_raw.json" "$TMPDIR/pvs_opt.json" "$TMPDIR/opensnes.json" <<'PYEOF'
import json
import sys

def load(path):
    with open(path) as f:
        return json.load(f)

pvs_raw = load(sys.argv[1])
pvs_opt = load(sys.argv[2])
opensnes = load(sys.argv[3])

# Collect all function names in order (use OpenSNES order, which matches source order)
all_funcs = list(opensnes['functions'].keys())
# Add any PVSnesLib-only functions
for name in pvs_opt['functions']:
    if name not in all_funcs:
        all_funcs.append(name)

# Column widths
max_name = max(len(n) for n in all_funcs) if all_funcs else 8
max_name = max(max_name, 8)

print()
print("=" * (max_name + 52))
print("  OpenSNES vs PVSnesLib Compiler Benchmark")
print("=" * (max_name + 52))
print()
header = f"  {'FUNCTION':<{max_name}}  {'PVS_RAW':>8}  {'PVS_OPT':>8}  {'OPENSNES':>8}  {'vs_OPT':>8}"
print(header)
sep = f"  {chr(0x2500) * max_name}  {chr(0x2500) * 8}  {chr(0x2500) * 8}  {chr(0x2500) * 8}  {chr(0x2500) * 8}"
print(sep)

total_raw = 0
total_opt = 0
total_ons = 0
wins = 0
losses = 0

for name in all_funcs:
    c_raw = pvs_raw['functions'].get(name, {}).get('cycles', 0)
    c_opt = pvs_opt['functions'].get(name, {}).get('cycles', 0)
    c_ons = opensnes['functions'].get(name, {}).get('cycles', 0)

    total_raw += c_raw
    total_opt += c_opt
    total_ons += c_ons

    if c_opt > 0:
        delta = (c_ons - c_opt) / c_opt * 100
        delta_str = f"{delta:+.1f}%"
        if c_ons < c_opt:
            wins += 1
        elif c_ons > c_opt:
            losses += 1
    elif c_ons > 0:
        delta_str = "NEW"
    else:
        delta_str = "-"

    raw_str = str(c_raw) if c_raw > 0 else "-"
    opt_str = str(c_opt) if c_opt > 0 else "-"
    ons_str = str(c_ons) if c_ons > 0 else "-"

    print(f"  {name:<{max_name}}  {raw_str:>8}  {opt_str:>8}  {ons_str:>8}  {delta_str:>8}")

print(sep)

if total_opt > 0:
    total_delta = (total_ons - total_opt) / total_opt * 100
    total_delta_str = f"{total_delta:+.1f}%"
else:
    total_delta_str = "-"

print(f"  {'TOTAL':<{max_name}}  {total_raw:>8}  {total_opt:>8}  {total_ons:>8}  {total_delta_str:>8}")
print()
print(f"  Functions: {len(all_funcs)} | "
      f"OpenSNES wins: {wins} | PVSnesLib+opt wins: {losses} | Tie: {len(all_funcs) - wins - losses}")
print()
PYEOF
