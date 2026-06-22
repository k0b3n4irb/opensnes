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

# Extract stdout from tool_response (the actual field name in Claude Code hooks)
OUTPUT=$(echo "$INPUT" | python3 -c "
import sys, json
try:
    d = json.load(sys.stdin)
    r = d.get('tool_response', d.get('tool_result', {}))
    print(r.get('stdout', '') or r.get('output', ''))
except:
    print('')
" 2>/dev/null) || OUTPUT=""

# Check for the success banner emitted by `make tests`
if echo "$OUTPUT" | grep -qE 'ALL CHECKS PASSED' 2>/dev/null; then
    TODAY=$(date +%Y-%m-%d)
    MARKER="/tmp/opensnes_tests_passed_${TODAY}"
    echo "$(date '+%H:%M:%S') tests passed" >> "$MARKER" 2>/dev/null
fi

exit 0
