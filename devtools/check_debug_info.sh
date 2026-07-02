#!/usr/bin/env bash
#
# check_debug_info.sh — CI coverage for the Cooper source-level debug path.
#
# The debug info (dbgloc `; @cline`, typed `; @dbglocal`, the `.dbg` sidecar)
# is emitted ONLY under `CC65816_G` so a normal build stays byte-identical to a
# non-debug compiler. Without a test, either half can silently regress:
#   * the debug metadata could leak into a normal build (perturbing release
#     codegen — this actually happened, see qbe bd929f5 / cproc 7b3200b), or
#   * the `-g` emission could break and Cooper would lose debugging with no signal.
#
# This asserts both halves on one representative example. Needs the toolchain
# already built in $OPENSNES_HOME/bin (CI runs it after `make release`).
#
# Usage:  OPENSNES_HOME=/path/to/opensnes devtools/check_debug_info.sh
set -euo pipefail

ROOT="${OPENSNES_HOME:-$(cd "$(dirname "$0")/.." && pwd)}"
EX="$ROOT/examples/basics/aim_target"   # has typed locals + a fixed-point struct
ASM="$EX/main.c.asm"
DBG="$EX/main.c.dbg"

fail() { echo "check_debug_info: FAIL — $1" >&2; exit 1; }
# grep -c prints the count AND exits 1 when it is zero; normalise to a bare int.
clines() {
    local n
    [ -f "$ASM" ] || { echo 0; return; }
    n="$( { grep -c '@cline' "$ASM" 2>/dev/null || true; } )"
    echo "${n:-0}"
}

# 1. A normal build must emit NO debug metadata (byte-identical release codegen).
#    `make clean` doesn't sweep the .dbg sidecar, so remove it explicitly to be
#    sure we're testing what THIS build writes, not a leftover from a -g run.
make -C "$EX" clean >/dev/null
rm -f "$DBG"
OPENSNES_HOME="$ROOT" make -C "$EX" >/dev/null
[ "$(clines)" -eq 0 ] || fail "normal build emitted @cline (must be gated behind CC65816_G)"
[ ! -f "$DBG" ]       || fail "normal build wrote a .dbg sidecar (must be gated behind CC65816_G)"
echo "OK: normal build emits no debug metadata"

# 2. A CC65816_G=1 build must emit @cline + typed @dbglocal + the .dbg sidecar.
make -C "$EX" clean >/dev/null
CC65816_G=1 OPENSNES_HOME="$ROOT" make -C "$EX" >/dev/null
n="$(clines)"
[ "$n" -gt 0 ]              || fail "-g build emitted no @cline line markers"
grep -q '@dbglocal' "$ASM" || fail "-g build emitted no @dbglocal typed locals"
[ -s "$DBG" ]              || fail "-g build wrote no (or empty) .dbg sidecar"
echo "OK: -g build emits $n @cline + @dbglocal + .dbg sidecar"

make -C "$EX" clean >/dev/null
echo "check_debug_info: PASS — Cooper -g debug-info path verified"
