#!/usr/bin/env bash
# Post-test marker hook for Claude Code
# Creates a daily marker file when the OpenSNES test suite completes successfully.
#
# Watched command  : `make tests` (the luna suite).
# Success pattern  : `ALL CHECKS PASSED` (emitted by the `tests` make target
#                    only after coverage + visual regression + probes all pass).

# No set -euo pipefail — this hook must NEVER crash.

INPUT=$(cat 2>/dev/null) || INPUT=""

# Quick exit if not about a test command
case "$INPUT" in
    *make\ tests*|*make\ \ tests*|*luna_runner.py*|*run_all.py*) ;;
    *)
        exit 0
        ;;
esac

# Extract the tool output from tool_response. Its shape has varied across
# Claude Code releases (a {stdout, stderr} object, a bare string, a content
# list), so accept all of them; if none yields the banner, scan the raw JSON
# as a last resort — the banner survives JSON escaping verbatim. Missing it
# is the failure mode that matters: on 2026-09-04/05 two green `make tests`
# runs left no marker and the pre-commit gate blocked validated commits.
OUTPUT=$(printf '%s' "$INPUT" | python3 -c '
import sys, json
try:
    d = json.load(sys.stdin)
    r = d.get("tool_response", d.get("tool_result", ""))
    parts = []
    if isinstance(r, dict):
        for k in ("stdout", "output", "stderr"):
            v = r.get(k)
            if isinstance(v, str):
                parts.append(v)
        c = r.get("content")
        if isinstance(c, list):
            parts += [x.get("text", "") for x in c if isinstance(x, dict)]
    elif isinstance(r, list):
        parts += [x.get("text", "") for x in r if isinstance(x, dict)]
    elif isinstance(r, str):
        parts.append(r)
    print("\n".join(parts))
except Exception:
    print("")
' 2>/dev/null) || OUTPUT=""

# The raw scan leaves the COMMAND out: a command that merely names the banner
# (a grep for it, a test of this hook) is not a green run. Seen 2026-11-12:
# a command quoting the banner marked its own tree. If the input is not JSON
# the whole of it is scanned, as before.
RAW=$(printf '%s' "$INPUT" | python3 -c '
import sys, json
s = sys.stdin.read()
try:
    d = json.loads(s)
    d.pop("tool_input", None)
    print(json.dumps(d))
except Exception:
    print(s)
' 2>/dev/null) || RAW="$INPUT"

# Check for the success banner emitted by `make tests`
if printf '%s' "$OUTPUT" | grep -qE 'ALL CHECKS PASSED' 2>/dev/null \
   || printf '%s' "$RAW" | grep -qE 'ALL CHECKS PASSED' 2>/dev/null; then
    # One marker per working tree and per day (tree-key.sh says why).
    . "$(dirname "$0")/tree-key.sh" 2>/dev/null && tests_marker \
        && echo "$(date '+%H:%M:%S') tests passed" >> "$MARKER" 2>/dev/null
fi

exit 0
