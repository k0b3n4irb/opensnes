#!/bin/bash
# =============================================================================
# OpenSNES Unit Tests Runner
# =============================================================================
# Builds and validates all unit tests in the tests/unit/ directory.
#
# Usage:
#   ./tests/unit/run_unit_tests.sh           # Run all tests
#   ./tests/unit/run_unit_tests.sh -v        # Verbose output
# =============================================================================

set -uo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DERIVED_HOME="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Auto-detect OPENSNES_HOME if not set or invalid
if [ -z "${OPENSNES_HOME:-}" ] || [ ! -f "${OPENSNES_HOME}/tools/symmap/symmap.py" ]; then
    export OPENSNES_HOME="$DERIVED_HOME"
fi

SYMMAP="$OPENSNES_HOME/tools/symmap/symmap.py"

VERBOSE=0
if [ "${1:-}" = "-v" ] || [ "${1:-}" = "--verbose" ]; then
    VERBOSE=1
fi

echo "========================================"
echo "OpenSNES Unit Tests"
echo "========================================"
echo "OPENSNES_HOME: $OPENSNES_HOME"
echo ""

TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0

# Find all unit test directories
for test_dir in "$SCRIPT_DIR"/*/; do
    if [ ! -f "$test_dir/Makefile" ]; then
        continue
    fi

    test_name=$(basename "$test_dir")
    TOTAL=$((TOTAL + 1))

    echo -n "Testing $test_name... "

    # Check if test is disabled
    if grep -q '\[SKIP\]' "$test_dir/Makefile" 2>/dev/null; then
        echo -e "${YELLOW}[SKIP]${NC}"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    # Build the test
    BUILD_LOG="/tmp/unittest_${test_name}.log"
    if make -C "$test_dir" > "$BUILD_LOG" 2>&1; then
        # Check for memory overlaps
        SFC_FILE=$(find "$test_dir" -maxdepth 1 -name "*.sfc" -type f 2>/dev/null | head -1)
        if [ -n "$SFC_FILE" ]; then
            SYM_FILE="${SFC_FILE%.sfc}.sym"
            if [ -f "$SYM_FILE" ] && [ -f "$SYMMAP" ]; then
                if python3 "$SYMMAP" --check-overlap "$SYM_FILE" > /dev/null 2>&1; then
                    echo -e "${GREEN}[PASS]${NC}"
                    PASSED=$((PASSED + 1))
                else
                    echo -e "${RED}[FAIL]${NC} (memory collision)"
                    FAILED=$((FAILED + 1))
                    if [ $VERBOSE -eq 1 ]; then
                        python3 "$SYMMAP" --check-overlap "$SYM_FILE"
                    fi
                fi
            else
                echo -e "${GREEN}[PASS]${NC}"
                PASSED=$((PASSED + 1))
            fi
        else
            echo -e "${GREEN}[PASS]${NC}"
            PASSED=$((PASSED + 1))
        fi
    else
        echo -e "${RED}[FAIL]${NC} (build failed)"
        FAILED=$((FAILED + 1))
        if [ $VERBOSE -eq 1 ]; then
            cat "$BUILD_LOG"
        fi
    fi
done

echo ""
echo "========================================"
echo "Unit Test Summary"
echo "========================================"
echo "Total:   $TOTAL"
echo -e "Passed:  ${GREEN}$PASSED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "Failed:  ${RED}$FAILED${NC}"
fi
if [ $SKIPPED -gt 0 ]; then
    echo -e "Skipped: ${YELLOW}$SKIPPED${NC}"
fi
echo "========================================"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo -e "${RED}Some tests failed${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}All tests passed${NC}"
exit 0
