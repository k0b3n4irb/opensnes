#!/bin/bash
#==============================================================================
# OpenSNES Black Screen Test Runner
#==============================================================================
#
# Runs all example ROMs through Mesen and checks for black screens.
# A black screen usually indicates a broken/non-working ROM.
#
# Usage:
#   ./run_black_screen_tests.sh /path/to/Mesen
#   ./run_black_screen_tests.sh /path/to/Mesen --verbose
#   ./run_black_screen_tests.sh /path/to/Mesen --save-screenshots
#
# Requirements:
#   - Mesen emulator with Lua support
#   - All examples must be built first (make examples)
#
#==============================================================================

set -eo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
EXAMPLES_DIR="$PROJECT_ROOT/examples"
LUA_SCRIPT="$SCRIPT_DIR/black_screen_test.lua"
RESULT_FILE="/tmp/opensnes_test_result.txt"
SCREENSHOT_FILE="/tmp/opensnes_test.png"
TIMEOUT_SECONDS=10

# Temp file for capturing output (macOS fallback needs this)
OUTPUT_FILE="/tmp/opensnes_mesen_output.txt"

# macOS compatible timeout function that captures output
run_with_timeout_capture() {
    local timeout=$1
    shift
    local cmd="$@"

    # Clear output file
    > "$OUTPUT_FILE"

    # Try gtimeout first (GNU coreutils), then timeout, then fallback
    if command -v gtimeout &>/dev/null; then
        gtimeout "$timeout" $cmd > "$OUTPUT_FILE" 2>&1
        return $?
    elif command -v timeout &>/dev/null; then
        timeout "$timeout" $cmd > "$OUTPUT_FILE" 2>&1
        return $?
    else
        # macOS fallback: run in background with output to file, kill after timeout
        $cmd > "$OUTPUT_FILE" 2>&1 &
        local pid=$!
        local count=0
        while kill -0 $pid 2>/dev/null && [ $count -lt $timeout ]; do
            sleep 1
            count=$((count + 1))
        done
        if kill -0 $pid 2>/dev/null; then
            kill -9 $pid 2>/dev/null
            wait $pid 2>/dev/null
            return 124  # timeout exit code
        fi
        wait $pid
        return $?
    fi
}

# Options
VERBOSE=0
SAVE_SCREENSHOTS=0
SCREENSHOTS_DIR=""

#------------------------------------------------------------------------------
# Parse arguments
#------------------------------------------------------------------------------
if [ $# -lt 1 ]; then
    echo "Usage: $0 /path/to/Mesen [--verbose] [--save-screenshots]"
    echo ""
    echo "Options:"
    echo "  --verbose          Show detailed output for each test"
    echo "  --save-screenshots Save screenshots to tests/screenshots/"
    exit 1
fi

MESEN_PATH="$1"
shift

while [ $# -gt 0 ]; do
    case "$1" in
        --verbose)
            VERBOSE=1
            ;;
        --save-screenshots)
            SAVE_SCREENSHOTS=1
            SCREENSHOTS_DIR="$SCRIPT_DIR/screenshots"
            mkdir -p "$SCREENSHOTS_DIR"
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
    shift
done

# Validate Mesen path
if [ ! -x "$MESEN_PATH" ]; then
    echo -e "${RED}ERROR: Mesen not found or not executable: $MESEN_PATH${NC}"
    exit 1
fi

# Validate Lua script
if [ ! -f "$LUA_SCRIPT" ]; then
    echo -e "${RED}ERROR: Lua script not found: $LUA_SCRIPT${NC}"
    exit 1
fi

#------------------------------------------------------------------------------
# Find all example ROMs
#------------------------------------------------------------------------------
ROMS=$(find "$EXAMPLES_DIR" -name "*.sfc" | sort)
TOTAL=$(echo "$ROMS" | wc -l | tr -d ' ')

if [ "$TOTAL" -eq 0 ]; then
    echo -e "${RED}ERROR: No .sfc files found in $EXAMPLES_DIR${NC}"
    echo "Run 'make examples' first to build the ROMs."
    exit 1
fi

#------------------------------------------------------------------------------
# Pre-flight: verify Mesen can start
#------------------------------------------------------------------------------
echo "--- Mesen pre-flight check ---"
echo "Binary: $MESEN_PATH"
echo "File type: $(file "$MESEN_PATH" 2>/dev/null | head -1)"
echo "Lua script: $LUA_SCRIPT"

# Try running Mesen briefly to check for missing libraries or startup errors
FIRST_ROM=$(echo "$ROMS" | head -1)
echo "Testing with: $FIRST_ROM"
timeout 15 "$MESEN_PATH" --testrunner --enablestdout "$FIRST_ROM" "$LUA_SCRIPT" > /tmp/opensnes_preflight.txt 2>&1
PREFLIGHT_EXIT=$?
echo "Pre-flight exit code: $PREFLIGHT_EXIT (124=timeout, 0=PASS, 1=FAIL)"
echo "--- Pre-flight output (first 30 lines) ---"
head -30 /tmp/opensnes_preflight.txt 2>/dev/null || echo "(no output)"
echo "--- Pre-flight output size: $(wc -c < /tmp/opensnes_preflight.txt 2>/dev/null || echo 0) bytes ---"

# Check if result was produced
if grep -q "BLACKTEST_RESULT:" /tmp/opensnes_preflight.txt 2>/dev/null; then
    echo "Pre-flight: Mesen testrunner is working!"
else
    echo "WARNING: Mesen did not produce expected output."
    echo "Full output:"
    cat /tmp/opensnes_preflight.txt 2>/dev/null || echo "(empty)"
fi
echo "-------------------------------"

#------------------------------------------------------------------------------
# Run tests
#------------------------------------------------------------------------------
echo "========================================"
echo "OpenSNES Black Screen Test"
echo "========================================"
echo ""
echo "Mesen: $MESEN_PATH"
echo "Examples: $TOTAL ROMs"
echo "Timeout: ${TIMEOUT_SECONDS}s per ROM"
echo ""
echo "----------------------------------------"

PASSED=0
FAILED=0
ERRORS=0
FAILED_ROMS=""
FIRST_ERROR_DUMPED=0

for ROM in $ROMS; do
    ROM_NAME=$(basename "$ROM" .sfc)
    ROM_DIR=$(basename "$(dirname "$ROM")")

    # Clean up previous result
    rm -f "$RESULT_FILE" "$SCREENSHOT_FILE"

    # Run Mesen with timeout
    if [ "$VERBOSE" -eq 1 ]; then
        echo -e "${CYAN}Testing: $ROM_DIR/$ROM_NAME${NC}"
    fi

    # Run Mesen in testrunner mode with timeout (output captured to file)
    # Syntax: Mesen --testrunner --enablestdout <rom> <lua_script>
    run_with_timeout_capture "$TIMEOUT_SECONDS" "$MESEN_PATH" --testrunner --enablestdout "$ROM" "$LUA_SCRIPT"
    MESEN_EXIT=$?
    OUTPUT=$(cat "$OUTPUT_FILE" 2>/dev/null || true)

    # Check result: prefer stdout output, fallback to exit code
    RESULT=""
    if echo "$OUTPUT" | grep -q "BLACKTEST_RESULT:"; then
        RESULT=$(echo "$OUTPUT" | grep "BLACKTEST_RESULT:" | tail -1 | sed 's/BLACKTEST_RESULT://')
    elif [ "$MESEN_EXIT" -eq 0 ]; then
        # emu.stop(0) = PASS (exit code 0, no stdout needed)
        RESULT="PASS:exit code 0"
    elif [ "$MESEN_EXIT" -eq 1 ]; then
        # emu.stop(1) = FAIL (exit code 1)
        RESULT="FAIL:exit code 1 (black screen)"
    elif [ -f "$RESULT_FILE" ]; then
        RESULT=$(cat "$RESULT_FILE")
    fi

    # Process result
    if [ -n "$RESULT" ]; then
        if echo "$RESULT" | grep -q "^PASS"; then
            PASSED=$((PASSED + 1))
            if [ "$VERBOSE" -eq 1 ]; then
                echo -e "  ${GREEN}[PASS]${NC} $RESULT"
            else
                echo -e "${GREEN}[PASS]${NC} $ROM_DIR/$ROM_NAME"
            fi

            # Save screenshot if requested
            if [ "$SAVE_SCREENSHOTS" -eq 1 ] && [ -f "$SCREENSHOT_FILE" ]; then
                cp "$SCREENSHOT_FILE" "$SCREENSHOTS_DIR/${ROM_DIR}_${ROM_NAME}.png"
            fi

        elif echo "$RESULT" | grep -q "^FAIL"; then
            FAILED=$((FAILED + 1))
            FAILED_ROMS="$FAILED_ROMS\n  - $ROM_DIR/$ROM_NAME"
            if [ "$VERBOSE" -eq 1 ]; then
                echo -e "  ${RED}[FAIL]${NC} $RESULT"
            else
                echo -e "${RED}[FAIL]${NC} $ROM_DIR/$ROM_NAME - Black screen"
            fi

            # Always save failed screenshots
            if [ -f "$SCREENSHOT_FILE" ]; then
                mkdir -p "$SCRIPT_DIR/screenshots"
                cp "$SCREENSHOT_FILE" "$SCRIPT_DIR/screenshots/FAIL_${ROM_DIR}_${ROM_NAME}.png"
            fi

        else
            ERRORS=$((ERRORS + 1))
            echo -e "${YELLOW}[ERROR]${NC} $ROM_DIR/$ROM_NAME - Unknown result: $RESULT"
        fi
    else
        # Timeout or crash - no result found
        ERRORS=$((ERRORS + 1))
        echo -e "${YELLOW}[ERROR]${NC} $ROM_DIR/$ROM_NAME - Timeout or crash (no result)"
        # Dump first error output for debugging
        if [ "$FIRST_ERROR_DUMPED" -eq 0 ]; then
            FIRST_ERROR_DUMPED=1
            echo "  --- Debug output for first error ---"
            head -20 "$OUTPUT_FILE" 2>/dev/null || echo "  (no output file)"
            echo "  --- End debug output ---"
        fi
    fi
done

#------------------------------------------------------------------------------
# Summary
#------------------------------------------------------------------------------
echo ""
echo "========================================"
echo "Results: $PASSED/$TOTAL passed"
echo "========================================"

if [ "$FAILED" -gt 0 ]; then
    echo -e "${RED}$FAILED tests failed (black screen):${NC}"
    echo -e "$FAILED_ROMS"
fi

if [ "$ERRORS" -gt 0 ]; then
    echo -e "${YELLOW}$ERRORS tests had errors${NC}"
fi

# Exit with appropriate code
if [ "$FAILED" -gt 0 ] || [ "$ERRORS" -gt 0 ]; then
    exit 1
fi

echo -e "${GREEN}All tests passed!${NC}"
exit 0
