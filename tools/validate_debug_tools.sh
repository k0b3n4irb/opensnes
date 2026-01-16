#!/bin/bash
#
# validate_debug_tools.sh - Validate OpenSNES debugging tools
#
# This script tests the debugging tools against known-good and known-bad
# test fixtures to ensure they work correctly.
#
# Usage:
#   ./validate_debug_tools.sh           # Run all tests
#   ./validate_debug_tools.sh --verbose # Show detailed output
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SYMMAP="$SCRIPT_DIR/symmap/symmap.py"
FIXTURES_DIR="$SCRIPT_DIR/debug-fixtures"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Counters
PASS=0
FAIL=0
SKIP=0

VERBOSE=0
if [[ "$1" == "-v" || "$1" == "--verbose" ]]; then
    VERBOSE=1
fi

echo -e "${BOLD}=== Validating OpenSNES Debug Tools ===${NC}"
echo ""

# Check tools exist
if [[ ! -f "$SYMMAP" ]]; then
    echo -e "${RED}ERROR: symmap not found at $SYMMAP${NC}"
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo -e "${RED}ERROR: python3 not found${NC}"
    exit 1
fi

echo -e "${CYAN}[TEST] symmap on clean fixtures...${NC}"

# Test 1: symmap must PASS on clean fixtures
for dir in "$FIXTURES_DIR"/clean/*/; do
    if [[ ! -d "$dir" ]]; then
        continue
    fi

    name=$(basename "$dir")
    sym_file=$(find "$dir" -name "*.sym" -type f | head -n1)

    if [[ -z "$sym_file" ]]; then
        echo -e "  ${YELLOW}SKIP${NC}: $name (no .sym file found)"
        ((SKIP++))
        continue
    fi

    output=$(python3 "$SYMMAP" --check-overlap --no-color "$sym_file" 2>&1)
    exit_code=$?

    if [[ $exit_code -eq 0 ]]; then
        echo -e "  ${GREEN}PASS${NC}: $name"
        ((PASS++))
        if [[ $VERBOSE -eq 1 ]]; then
            echo "$output" | sed 's/^/       /'
        fi
    else
        echo -e "  ${RED}FAIL${NC}: $name (should not have errors)"
        echo "$output" | sed 's/^/       /'
        ((FAIL++))
    fi
done

echo ""
echo -e "${CYAN}[TEST] symmap on broken fixtures...${NC}"

# Test 2: symmap must CATCH broken fixtures
for dir in "$FIXTURES_DIR"/broken/*/; do
    if [[ ! -d "$dir" ]]; then
        continue
    fi

    name=$(basename "$dir")
    sym_file=$(find "$dir" -name "*.sym" -type f | head -n1)
    expected_file="$dir/expected_error.txt"

    if [[ -z "$sym_file" ]]; then
        echo -e "  ${YELLOW}SKIP${NC}: $name (no .sym file found)"
        ((SKIP++))
        continue
    fi

    if [[ ! -f "$expected_file" ]]; then
        echo -e "  ${YELLOW}SKIP${NC}: $name (no expected_error.txt)"
        ((SKIP++))
        continue
    fi

    expected=$(cat "$expected_file")
    output=$(python3 "$SYMMAP" --check-overlap --no-color "$sym_file" 2>&1)
    exit_code=$?

    # Check that it failed (exit code != 0) AND contains expected error
    if [[ $exit_code -ne 0 ]] && echo "$output" | grep -q "$expected"; then
        echo -e "  ${GREEN}PASS${NC}: $name (caught: $expected)"
        ((PASS++))
        if [[ $VERBOSE -eq 1 ]]; then
            echo "$output" | sed 's/^/       /'
        fi
    elif [[ $exit_code -eq 0 ]]; then
        echo -e "  ${RED}FAIL${NC}: $name (did not detect error - exit code 0)"
        echo "$output" | sed 's/^/       /'
        ((FAIL++))
    else
        echo -e "  ${RED}FAIL${NC}: $name (did not contain: $expected)"
        echo "$output" | sed 's/^/       /'
        ((FAIL++))
    fi
done

echo ""
echo -e "${BOLD}=== Results ===${NC}"
echo -e "${GREEN}Passed: $PASS${NC}"
echo -e "${RED}Failed: $FAIL${NC}"
if [[ $SKIP -gt 0 ]]; then
    echo -e "${YELLOW}Skipped: $SKIP${NC}"
fi

echo ""
if [[ $FAIL -gt 0 ]]; then
    echo -e "${RED}${BOLD}VALIDATION FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}${BOLD}VALIDATION PASSED${NC}"
    exit 0
fi
