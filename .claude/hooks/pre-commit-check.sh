#!/usr/bin/env bash
# Pre-commit hook for Claude Code
# Blocks git commit if tests haven't been run today
#
# Input: JSON on stdin with { "tool_name": "Bash", "tool_input": { "command": "..." } }
# Output: JSON on stdout with { "decision": "allow"|"block", "reason": "..." }
#
# This hook intercepts Bash tool calls and checks if the command is a git commit.
# If so, it verifies that tests have been run (marker file exists for today).

set -euo pipefail

# Read the tool input from stdin
INPUT=$(cat)

# Extract the command from JSON
COMMAND=$(echo "$INPUT" | python3 -c "import sys, json; print(json.load(sys.stdin).get('tool_input', {}).get('command', ''))" 2>/dev/null || echo "")

# Only intercept git commit commands
if ! echo "$COMMAND" | grep -qE '^\s*git\s+commit\b'; then
    # Not a git commit - allow
    echo '{"decision": "allow"}'
    exit 0
fi

# Check for test marker file
TODAY=$(date +%Y-%m-%d)
MARKER="/tmp/opensnes_tests_passed_${TODAY}"

if [[ -f "$MARKER" ]]; then
    # Tests were run today - allow the commit
    echo '{"decision": "allow", "reason": "Tests passed today (marker: '"$MARKER"')"}'
    exit 0
fi

# Tests not run - block the commit
cat <<EOF
{"decision": "block", "reason": "BLOCKED: Tests have not been run today. Run these commands first:\n\n  ./tests/compiler/run_tests.sh\n  OPENSNES_HOME=\$(pwd) ./tests/examples/validate_examples.sh --quick\n\nSee .claude/TESTING_PROTOCOL.md for the full validation protocol."}
EOF
exit 0
