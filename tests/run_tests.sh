#!/bin/bash
#
# OpenSNES Test Runner
#
# Runs automated tests using Mesen2 emulator.
#
# Usage:
#   ./run_tests.sh              # Run all tests
#   ./run_tests.sh unit         # Run unit tests only
#   ./run_tests.sh hardware     # Run hardware tests only
#   ./run_tests.sh -v           # Verbose output
#
# Environment:
#   MESEN_PATH  - Path to Mesen2 executable
#   OPENSNES_HOME - Path to OpenSNES SDK
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPENSNES_HOME="${OPENSNES_HOME:-$(dirname "$SCRIPT_DIR")}"
MESEN_PATH="${MESEN_PATH:-}"
VERBOSE=0
CATEGORY=""

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options] [category]"
            echo ""
            echo "Options:"
            echo "  -v, --verbose    Show detailed output"
            echo "  -h, --help       Show this help"
            echo ""
            echo "Categories:"
            echo "  unit             Run unit tests"
            echo "  hardware         Run hardware tests"
            echo "  integration      Run integration tests"
            echo "  templates        Run template smoke tests"
            echo ""
            echo "Environment:"
            echo "  MESEN_PATH       Path to Mesen2 executable"
            echo "  OPENSNES_HOME    Path to OpenSNES SDK"
            exit 0
            ;;
        *)
            CATEGORY="$1"
            shift
            ;;
    esac
done

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_verbose() {
    if [[ $VERBOSE -eq 1 ]]; then
        echo "[DEBUG] $1"
    fi
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    # Check OPENSNES_HOME
    if [[ ! -d "$OPENSNES_HOME" ]]; then
        log_error "OPENSNES_HOME not set or directory not found: $OPENSNES_HOME"
        exit 1
    fi
    log_verbose "OPENSNES_HOME: $OPENSNES_HOME"

    # Check Mesen
    if [[ -z "$MESEN_PATH" ]]; then
        # Try common locations
        if [[ -x "/Users/k0b3/workspaces/github/Mesen2/bin/osx-arm64/Release/Mesen" ]]; then
            MESEN_PATH="/Users/k0b3/workspaces/github/Mesen2/bin/osx-arm64/Release/Mesen"
        elif command -v mesen &> /dev/null; then
            MESEN_PATH="mesen"
        else
            log_error "MESEN_PATH not set and Mesen not found in PATH"
            log_error "Set MESEN_PATH environment variable to Mesen2 executable"
            exit 1
        fi
    fi

    if [[ ! -x "$MESEN_PATH" ]]; then
        log_error "Mesen not executable: $MESEN_PATH"
        exit 1
    fi
    log_verbose "MESEN_PATH: $MESEN_PATH"

    log_info "Prerequisites OK"
}

# Build a test ROM
build_test() {
    local test_dir="$1"
    local test_name="$(basename "$test_dir")"

    log_verbose "Building test: $test_name"

    if [[ ! -f "$test_dir/Makefile" ]]; then
        log_warn "No Makefile in $test_dir, skipping"
        return 1
    fi

    # Build the test
    if ! make -C "$test_dir" -s; then
        log_error "Build failed for $test_name"
        return 1
    fi

    # Find the ROM
    local rom=$(find "$test_dir" -name "*.sfc" -type f | head -1)
    if [[ -z "$rom" ]]; then
        log_error "No ROM found in $test_dir"
        return 1
    fi

    echo "$rom"
}

# Run a single test
run_test() {
    local test_dir="$1"
    local test_name="$(basename "$test_dir")"

    log_info "Running test: $test_name"
    ((TOTAL_TESTS++))

    # Check for test script
    local lua_script="$test_dir/test.lua"
    if [[ ! -f "$lua_script" ]]; then
        lua_script="$SCRIPT_DIR/harness/test_harness.lua"
    fi

    # Build the test
    local rom
    rom=$(build_test "$test_dir") || {
        log_error "SKIP: Build failed for $test_name"
        ((SKIPPED_TESTS++))
        return
    }

    log_verbose "ROM: $rom"
    log_verbose "Script: $lua_script"

    # Run with Mesen2
    local timeout=30
    local output
    output=$(timeout "$timeout" "$MESEN_PATH" "$rom" \
        --testrunner \
        --LoadScript="$lua_script" \
        2>&1) || {
        local exit_code=$?
        if [[ $exit_code -eq 124 ]]; then
            log_error "TIMEOUT: $test_name (${timeout}s)"
            ((FAILED_TESTS++))
            return
        elif [[ $exit_code -ne 0 ]]; then
            log_error "FAIL: $test_name (exit code: $exit_code)"
            if [[ $VERBOSE -eq 1 ]]; then
                echo "$output"
            fi
            ((FAILED_TESTS++))
            return
        fi
    }

    # Check output for pass/fail
    if echo "$output" | grep -q "Status: PASS"; then
        log_info "PASS: $test_name"
        ((PASSED_TESTS++))
    else
        log_error "FAIL: $test_name"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "$output"
        fi
        ((FAILED_TESTS++))
    fi
}

# Run all tests in a category
run_category() {
    local category="$1"
    local category_dir="$SCRIPT_DIR/$category"

    if [[ ! -d "$category_dir" ]]; then
        log_warn "Category not found: $category"
        return
    fi

    log_info "Running category: $category"

    for test_dir in "$category_dir"/*/; do
        if [[ -d "$test_dir" ]]; then
            run_test "$test_dir"
        fi
    done
}

# Print summary
print_summary() {
    echo ""
    echo "========================================"
    echo "Test Summary"
    echo "========================================"
    echo "Total:   $TOTAL_TESTS"
    echo -e "Passed:  ${GREEN}$PASSED_TESTS${NC}"
    echo -e "Failed:  ${RED}$FAILED_TESTS${NC}"
    echo -e "Skipped: ${YELLOW}$SKIPPED_TESTS${NC}"
    echo "========================================"

    if [[ $FAILED_TESTS -gt 0 ]]; then
        return 1
    fi
    return 0
}

# Main
main() {
    log_info "OpenSNES Test Runner"
    log_info "===================="

    check_prerequisites

    if [[ -n "$CATEGORY" ]]; then
        # Run specific category
        run_category "$CATEGORY"
    else
        # Run all categories
        for category in unit hardware integration templates; do
            if [[ -d "$SCRIPT_DIR/$category" ]]; then
                run_category "$category"
            fi
        done
    fi

    print_summary
}

main "$@"
