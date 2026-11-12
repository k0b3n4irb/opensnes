#!/usr/bin/env bash
# Sourced by mark-tests-passed.sh and pre-commit-check.sh: the name of the
# "tests passed" marker of ONE working tree for today.
#
# The marker used to be one file per day for the whole machine: a green run
# in one worktree opened the commit gate of every other (2026-11-12: three
# compiler commits went through on a marker another session had left; noted
# as an open point by an audit of 2026-10-17). The tree is the git toplevel
# of the directory the command ran in (the hook input's "cwd"), else the
# project's directory.
#
# Needs $INPUT (the hook's JSON). Sets MARKER.
tests_marker() {
    local dir top key
    dir=$(printf '%s' "$INPUT" | python3 -c '
import sys, json
try:
    print(json.load(sys.stdin).get("cwd", ""))
except Exception:
    print("")
' 2>/dev/null) || dir=""
    [ -d "$dir" ] || dir="${CLAUDE_PROJECT_DIR:-$PWD}"
    top=$(git -C "$dir" rev-parse --show-toplevel 2>/dev/null) || top="$dir"
    key=$(printf '%s' "$top" | sha1sum 2>/dev/null | cut -c1-12)
    MARKER="/tmp/opensnes_tests_passed_$(date +%Y-%m-%d)_${key:-unknown}"
}
