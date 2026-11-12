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
        exit 0
        ;;
esac

# One marker per working tree and per day (tree-key.sh says why).
MARKER=""
. "$(dirname "$0")/tree-key.sh" 2>/dev/null && tests_marker

if [[ -n "$MARKER" && -f "$MARKER" ]]; then
    exit 0
fi

# Tests not run today in this working tree — block
cat <<EOF
{"decision": "block", "reason": "BLOCKED: the OpenSNES test suite has not been run today in THIS working tree.\n\nRun the test suite first (single tool: luna):\n\n  make tests\n\nWhen it prints 'ALL CHECKS PASSED' the marker of this tree is created automatically. A green run in another worktree does not count.\n\nTo bypass for trivial changes (docs, comments), touch this tree's marker:\n\n  touch ${MARKER:-/tmp/opensnes_tests_passed_DATE_TREE}\n\nSee CONTRIBUTING.md for the full testing workflow."}
EOF
exit 0
