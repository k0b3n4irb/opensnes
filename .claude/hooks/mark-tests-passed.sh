#!/usr/bin/env bash
# Post-test marker hook for Claude Code
# Creates a daily marker file when the OpenSNES test suite completes successfully.
#
# Watched commands : `node ... run-all-tests.mjs ...` (canonical test runner)
# Success patterns : `ALL CHECKS PASSED` (output of run-all-tests.mjs)
#
# Legacy patterns (run_tests.sh / validate_examples.sh / "All tests passed" /
# "ALL VALIDATIONS PASSED") are still matched for backwards compatibility,
# even though those scripts no longer ship with the repo.

# No set -euo pipefail — this hook must NEVER crash.

INPUT=$(cat 2>/dev/null) || INPUT=""

# Quick exit if not about a test command
case "$INPUT" in
    *run-all-tests.mjs*|*run_tests.sh*|*validate_examples.sh*) ;;
    *)
        echo '{"decision": "allow"}'
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

# Check for success indicators (current + legacy)
if echo "$OUTPUT" | grep -qE 'ALL CHECKS PASSED|All tests passed|ALL VALIDATIONS PASSED' 2>/dev/null; then
    TODAY=$(date +%Y-%m-%d)
    MARKER="/tmp/opensnes_tests_passed_${TODAY}"
    echo "$(date '+%H:%M:%S') tests passed" >> "$MARKER" 2>/dev/null
fi

echo '{"decision": "allow"}'
exit 0
