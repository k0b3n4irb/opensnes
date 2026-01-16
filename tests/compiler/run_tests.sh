#!/bin/bash
#==============================================================================
# OpenSNES Compiler Tests
#
# Tests the cc65816 compiler toolchain:
# 1. C -> Assembly (cproc-qbe + qbe)
# 2. Assembly syntax compatibility with wla-65816
# 3. Assembly -> Object file
#
# Usage:
#   ./run_tests.sh           # Run all tests
#   ./run_tests.sh -v        # Verbose output
#   ./run_tests.sh test_X    # Run specific test
#==============================================================================

# Don't exit on error - we want to run all tests
# set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPENSNES="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$OPENSNES/bin"
BUILD="$SCRIPT_DIR/build"

# Tools
CC="$BIN/cc65816"
AS="$BIN/wla-65816"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

VERBOSE=0

#------------------------------------------------------------------------------
# Helpers
#------------------------------------------------------------------------------

log_info() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_verbose() {
    if [[ $VERBOSE -eq 1 ]]; then
        echo "[DEBUG] $1"
    fi
}

#------------------------------------------------------------------------------
# Test: Compiler exists and runs
#------------------------------------------------------------------------------
test_compiler_exists() {
    local name="compiler_exists"
    ((TESTS_RUN++))

    if [[ ! -x "$CC" ]]; then
        log_fail "$name: cc65816 not found at $CC"
        ((TESTS_FAILED++))
        return 1
    fi

    if [[ ! -x "$BIN/cproc-qbe" ]]; then
        log_fail "$name: cproc-qbe not found"
        ((TESTS_FAILED++))
        return 1
    fi

    if [[ ! -x "$BIN/qbe" ]]; then
        log_fail "$name: qbe not found"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Assembler exists and runs
#------------------------------------------------------------------------------
test_assembler_exists() {
    local name="assembler_exists"
    ((TESTS_RUN++))

    if [[ ! -x "$AS" ]]; then
        log_fail "$name: wla-65816 not found at $AS"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Compile minimal C to assembly
#------------------------------------------------------------------------------
test_compile_minimal() {
    local name="compile_minimal"
    local src="$SCRIPT_DIR/test_minimal.c"
    local out="$BUILD/test_minimal.c.asm"
    ((TESTS_RUN++))

    mkdir -p "$BUILD"

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    if [[ ! -f "$out" ]]; then
        log_fail "$name: Output file not created"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Assembly output has no .dsb without data
#------------------------------------------------------------------------------
test_no_dsb_without_data() {
    local name="no_dsb_without_data"
    local asm="$BUILD/test_minimal.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$asm" ]]; then
        log_fail "$name: Assembly file not found (run compile test first)"
        ((TESTS_FAILED++))
        return 1
    fi

    # Check for .dsb without a value (the problematic pattern)
    # Valid: .dsb 4, 0
    # Invalid: .dsb 4 (no fill value)
    if grep -E '\.dsb\s+[0-9]+\s*$' "$asm" > /dev/null 2>&1; then
        log_fail "$name: Found .dsb without fill value"
        if [[ $VERBOSE -eq 1 ]]; then
            grep -n -E '\.dsb\s+[0-9]+\s*$' "$asm"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Assembly output uses correct directives
#------------------------------------------------------------------------------
test_correct_directives() {
    local name="correct_directives"
    local asm="$BUILD/test_minimal.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$asm" ]]; then
        log_fail "$name: Assembly file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    local errors=0

    # Check for GNU-style directives that wla-65816 doesn't understand
    if grep -E '^\.byte\s' "$asm" > /dev/null 2>&1; then
        log_verbose "Found .byte (should be .db)"
        ((errors++))
    fi

    if grep -E '^\.word\s' "$asm" > /dev/null 2>&1; then
        log_verbose "Found .word (should be .dw)"
        ((errors++))
    fi

    if grep -E '^\.long\s' "$asm" > /dev/null 2>&1; then
        log_verbose "Found .long (should be .dl)"
        ((errors++))
    fi

    if grep -E '^\.balign\s' "$asm" > /dev/null 2>&1; then
        log_verbose "Found .balign (not supported)"
        ((errors++))
    fi

    if [[ $errors -gt 0 ]]; then
        log_fail "$name: Found $errors incompatible directives"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Can assemble the output (with memory map)
#------------------------------------------------------------------------------
test_assemble_output() {
    local name="assemble_output"
    local asm="$BUILD/test_minimal.c.asm"
    local asm_with_map="$BUILD/test_minimal_with_map.asm"
    local obj="$BUILD/test_minimal.o"
    ((TESTS_RUN++))

    if [[ ! -f "$asm" ]]; then
        log_fail "$name: Assembly file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Add memory map header to assembly file
    cat "$SCRIPT_DIR/test_memmap.inc" "$asm" > "$asm_with_map"

    # Try to assemble
    if ! "$AS" -o "$obj" "$asm_with_map" 2>"$BUILD/assemble.err"; then
        log_fail "$name: Assembly failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/assemble.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    if [[ ! -f "$obj" ]]; then
        log_fail "$name: Object file not created"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Processed assembly can be assembled
#------------------------------------------------------------------------------
test_processed_assembly() {
    local name="processed_assembly"
    local src="$BUILD/test_minimal.c.asm"
    local processed="$BUILD/test_minimal_processed.asm"
    local processed_with_map="$BUILD/test_minimal_processed_with_map.asm"
    local obj="$BUILD/test_minimal_processed.o"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source assembly not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Apply the same transformations as lib/Makefile
    sed -e 's/\.byte/.db/g' \
        -e 's/\.word/.dw/g' \
        -e 's/\.long/.dl/g' \
        -e '/^\.data/d' \
        -e '/^\/\* end/d' \
        -e '/^\.balign/d' \
        -e '/^\.GLOBAL/d' \
        -e 's/\.dsb \([0-9]*\)$/.dsb \1, 0/g' \
        "$src" > "$processed"

    # Add memory map header
    cat "$SCRIPT_DIR/test_memmap.inc" "$processed" > "$processed_with_map"

    # Try to assemble processed file
    if ! "$AS" -o "$obj" "$processed_with_map" 2>"$BUILD/assemble_processed.err"; then
        log_fail "$name: Processed assembly failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/assemble_processed.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------

main() {
    # Parse args
    while [[ $# -gt 0 ]]; do
        case $1 in
            -v|--verbose)
                VERBOSE=1
                shift
                ;;
            *)
                # Specific test
                shift
                ;;
        esac
    done

    echo "========================================"
    echo "OpenSNES Compiler Tests"
    echo "========================================"
    echo ""

    # Clean build dir
    rm -rf "$BUILD"
    mkdir -p "$BUILD"

    # Run tests in order (some depend on previous)
    test_compiler_exists
    test_assembler_exists
    test_compile_minimal
    test_no_dsb_without_data || true  # May fail, that's what we're fixing
    test_correct_directives || true   # May fail, that's what we're fixing
    test_assemble_output || true      # May fail before fixes
    test_processed_assembly || true   # Test with post-processing

    echo ""
    echo "========================================"
    echo "Results: $TESTS_PASSED/$TESTS_RUN passed"
    if [[ $TESTS_FAILED -gt 0 ]]; then
        echo -e "${RED}$TESTS_FAILED tests failed${NC}"
        exit 1
    else
        echo -e "${GREEN}All tests passed${NC}"
        exit 0
    fi
}

main "$@"
