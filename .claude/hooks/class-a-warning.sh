#!/usr/bin/env bash
# PostToolUse hook: warn on Class A file modifications
# Triggers on Edit/Write to compiler/ or templates/ files

INPUT=$(cat 2>/dev/null) || INPUT=""

FILE_PATH=$(echo "$INPUT" | python3 -c "
import sys, json
try:
    d = json.load(sys.stdin)
    ti = d.get('tool_input', {})
    print(ti.get('file_path', '') or ti.get('path', ''))
except:
    print('')
" 2>/dev/null) || FILE_PATH=""

case "$FILE_PATH" in
    *templates/crt0.asm*)
        echo "CLASS A WARNING: crt0.asm modified — consult .claude/rules/nmi_audit.md before commit. Requires: make clean && make + full test suite + Mesen2 on ALL examples."
        ;;
    *templates/*)
        echo "CLASS A WARNING: template modified — requires: make clean && make + full test suite + Mesen2."
        ;;
    *compiler/*)
        echo "CLASS A WARNING: compiler modified — requires: make clean && make + full test suite + Mesen2 on ALL examples."
        ;;
esac

exit 0
