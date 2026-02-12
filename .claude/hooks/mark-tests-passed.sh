#!/usr/bin/env bash
# Post-test marker hook for Claude Code
# Creates a daily marker file when tests complete successfully
#
# Input: JSON on stdin with { "tool_name": "Bash", "tool_input": { "command": "..." }, "tool_result": { "exit_code": N, ... } }
# Output: JSON on stdout (always allow, this is observational)
#
# Detects successful execution of run_tests.sh or validate_examples.sh
# and creates a marker file that pre-commit-check.sh looks for.

set -euo pipefail

# Read the tool input from stdin
INPUT=$(cat)

# Extract command and exit code from JSON
COMMAND=$(echo "$INPUT" | python3 -c "import sys, json; print(json.load(sys.stdin).get('tool_input', {}).get('command', ''))" 2>/dev/null || echo "")
STDOUT=$(echo "$INPUT" | python3 -c "import sys, json; print(json.load(sys.stdin).get('tool_result', {}).get('stdout', ''))" 2>/dev/null || echo "")

# Check if this was a test command that succeeded
IS_TEST_CMD=false
if echo "$COMMAND" | grep -qE 'run_tests\.sh|validate_examples\.sh'; then
    IS_TEST_CMD=true
fi

if [[ "$IS_TEST_CMD" == "true" ]]; then
    # Check for success indicators in output
    # run_tests.sh prints "PASSED" / "All N tests passed"
    # validate_examples.sh prints "All examples validated"
    if echo "$STDOUT" | grep -qiE 'pass|passed|validated|all.*tests.*passed|all.*examples.*validated'; then
        TODAY=$(date +%Y-%m-%d)
        MARKER="/tmp/opensnes_tests_passed_${TODAY}"
        echo "$COMMAND" >> "$MARKER"
        echo "$(date '+%H:%M:%S') - Tests passed" >> "$MARKER"
    fi
fi

# Always allow (this is a PostToolUse hook - observational only)
echo '{"decision": "allow"}'
exit 0
