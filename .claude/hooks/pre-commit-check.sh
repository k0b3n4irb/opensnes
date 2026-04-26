#!/usr/bin/env bash
# Pre-commit hook for Claude Code
# Blocks `git commit` if the OpenSNES test suite has not been run today.
#
# Input  (stdin)  : JSON { "tool_name": "Bash", "tool_input": { "command": "..." } }
# Output (stdout) : JSON { "decision": "allow"|"block", "reason": "..." }
#
# The marker file /tmp/opensnes_tests_passed_YYYY-MM-DD is created automatically
# by `mark-tests-passed.sh` when the test runner reports `ALL CHECKS PASSED`.
# It can also be touched manually to bypass for trivial changes (docs, comments).

# No set -euo pipefail — this hook must NEVER crash.

INPUT=$(cat 2>/dev/null) || INPUT=""

# Quick exit if not a git commit command
case "$INPUT" in
    *git\ commit*|*git\ \ commit*) ;;
    *)
        echo '{"decision": "allow"}'
        exit 0
        ;;
esac

TODAY=$(date +%Y-%m-%d)
MARKER="/tmp/opensnes_tests_passed_${TODAY}"

if [[ -f "$MARKER" ]]; then
    echo '{"decision": "allow"}'
    exit 0
fi

# Tests not run today — block
cat <<'EOF'
{"decision": "block", "reason": "BLOCKED: OpenSNES test suite has not been run today.\n\nRun the test suite first:\n\n  cd tools/opensnes-emu && node test/run-all-tests.mjs --quick\n\nIf all checks pass, the marker /tmp/opensnes_tests_passed_$(date +%Y-%m-%d) is created automatically.\n\nTo bypass for trivial changes (docs, comments), touch the marker manually:\n\n  touch /tmp/opensnes_tests_passed_$(date +%Y-%m-%d)\n\nSee CONTRIBUTING.md for the full testing workflow."}
EOF
exit 0
