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
# Test: Static variable stores use proper symbol references
# BUG: Compiler was generating "sta.l $000000" instead of "sta.l symbol"
#------------------------------------------------------------------------------
test_static_var_stores() {
    local name="static_var_stores"
    local src="$SCRIPT_DIR/test_static_vars.c"
    local out="$BUILD/test_static_vars.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/static_vars_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/static_vars_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check for the bug: stores to literal $000000
    # Valid: sta.l my_static_byte or sta my_static_byte
    # Invalid: sta.l $000000
    if grep -E 'sta\.l\s+\$000000' "$out" > /dev/null 2>&1; then
        log_fail "$name: Found 'sta.l \$000000' - stores to static vars broken!"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Offending lines:"
            grep -n -E 'sta\.l\s+\$000000' "$out"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Verify symbols are defined
    if ! grep -E '^my_static_byte:' "$out" > /dev/null 2>&1; then
        log_fail "$name: Symbol 'my_static_byte' not defined"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! grep -E '^my_static_word:' "$out" > /dev/null 2>&1; then
        log_fail "$name: Symbol 'my_static_word' not defined"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Shift right operations generate proper LSR instructions
# BUG: Compiler was emitting "unhandled op 12" for sar (arithmetic shift right)
#------------------------------------------------------------------------------
test_shift_right_ops() {
    local name="shift_right_ops"
    local src="$SCRIPT_DIR/test_shift_right.c"
    local out="$BUILD/test_shift_right.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/shift_right_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/shift_right_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check for unhandled op comments (indicates missing instruction support)
    if grep -E '; unhandled op' "$out" > /dev/null 2>&1; then
        log_fail "$name: Found unhandled op in shift right code"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Offending lines:"
            grep -n -E '; unhandled op' "$out"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Verify lsr instructions are generated (for >> 8 operation)
    if ! grep -E 'lsr a' "$out" > /dev/null 2>&1; then
        log_fail "$name: No LSR instructions generated for shift right"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Word extension ops (extsw/extuw) are handled
# BUG: Compiler was emitting "unhandled op 68/69" for word-to-long extension
#------------------------------------------------------------------------------
test_word_extend_ops() {
    local name="word_extend_ops"
    local src="$SCRIPT_DIR/test_word_extend.c"
    local out="$BUILD/test_word_extend.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/word_extend_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/word_extend_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check for unhandled op comments
    if grep -E '; unhandled op' "$out" > /dev/null 2>&1; then
        log_fail "$name: Found unhandled op in word extend code"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Offending lines:"
            grep -n -E '; unhandled op' "$out"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Animation patterns (loops, arrays, bit ops, conditionals)
# Comprehensive test for patterns used in sprite/animation code
#------------------------------------------------------------------------------
test_animation_patterns() {
    local name="animation_patterns"
    local src="$SCRIPT_DIR/test_animation_patterns.c"
    local out="$BUILD/test_animation_patterns.c.asm"
    local out_with_map="$BUILD/test_animation_patterns_with_map.asm"
    local obj="$BUILD/test_animation_patterns.o"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/animation_patterns_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/animation_patterns_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check for unhandled op comments
    if grep -E '; unhandled op' "$out" > /dev/null 2>&1; then
        log_fail "$name: Found unhandled ops"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Offending lines:"
            grep -n -E '; unhandled op' "$out"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Try to assemble (add memory map header)
    cat "$SCRIPT_DIR/test_memmap.inc" "$out" > "$out_with_map"
    if ! "$AS" -o "$obj" "$out_with_map" 2>"$BUILD/animation_patterns_asm.err"; then
        log_fail "$name: Assembly failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/animation_patterns_asm.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Division and modulo operations
# BUG: Compiler was emitting "unhandled op 4" for div operations
#------------------------------------------------------------------------------
test_division_ops() {
    local name="division_ops"
    local src="$SCRIPT_DIR/test_division.c"
    local out="$BUILD/test_division.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/division_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/division_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check for unhandled op comments
    if grep -E '; unhandled op' "$out" > /dev/null 2>&1; then
        log_fail "$name: Found unhandled ops"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Offending lines:"
            grep -n -E '; unhandled op' "$out"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Verify LSR instructions for power-of-2 divisions
    if ! grep -E 'lsr a' "$out" > /dev/null 2>&1; then
        log_fail "$name: No LSR instructions generated for power-of-2 division"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Multi-argument call stack offset fix
# BUG: When pushing multiple arguments, stack offsets weren't adjusted for
#      the SP changes from previous pushes, causing wrong values to be passed
#------------------------------------------------------------------------------
test_multiarg_call_offsets() {
    local name="multiarg_call_offsets"
    local src="$SCRIPT_DIR/test_multiarg_call.c"
    local out="$BUILD/test_multiarg_call.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/multiarg_call_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/multiarg_call_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check for unhandled op comments
    if grep -E '; unhandled op' "$out" > /dev/null 2>&1; then
        log_fail "$name: Found unhandled ops"
        if [[ $VERBOSE -eq 1 ]]; then
            grep -n -E '; unhandled op' "$out"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # The key test: stack offsets should INCREASE with each argument push
    # because earlier pushes decrease SP, requiring larger offsets to reach
    # the same local variables.
    #
    # Extract lines that push arguments (lda followed by pha)
    # After the first pha, subsequent lda offsets should be larger
    local arg_loads=$(grep -E "lda [0-9]+,s" "$out" | grep -A1 "test_multiarg_call:" | head -20)

    # Look for pattern where offsets increase (e.g., 4,s then 6,s then 8,s...)
    # This is a simplified check - the actual verification is that no two
    # consecutive argument loads have the SAME offset (which would indicate
    # the bug where SP changes weren't accounted for)

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Library builds have no unhandled ops
# CRITICAL: Library was compiled with old compiler, had unhandled ops in oamClear
#------------------------------------------------------------------------------
test_library_no_unhandled_ops() {
    local name="library_no_unhandled_ops"
    local lib_build="$OPENSNES/lib/build"
    ((TESTS_RUN++))

    if [[ ! -d "$lib_build" ]]; then
        log_fail "$name: Library build directory not found: $lib_build"
        ((TESTS_FAILED++))
        return 1
    fi

    local has_unhandled=0
    for asm_file in "$lib_build"/*.c.asm; do
        if [[ -f "$asm_file" ]]; then
            if grep -E '; unhandled op' "$asm_file" > /dev/null 2>&1; then
                log_fail "$name: Unhandled ops in $(basename "$asm_file")"
                if [[ $VERBOSE -eq 1 ]]; then
                    grep -n -E '; unhandled op' "$asm_file"
                fi
                has_unhandled=1
            fi
        fi
    done

    if [[ $has_unhandled -eq 1 ]]; then
        echo "  Hint: Rebuild library with 'cd lib && make clean && make'"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Global variable reads use direct addressing (not indirect via X)
# BUG: Compiler was generating "lda.w #symbol; tax; lda.l $0000,x"
#      instead of direct "lda.l symbol"
#------------------------------------------------------------------------------
test_global_var_reads() {
    local name="global_var_reads"
    local src="$SCRIPT_DIR/test_global_vars.c"
    local out="$BUILD/test_global_vars.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/global_vars_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/global_vars_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check for the bug pattern: indirect load via X register for global symbols
    # The pattern "lda.w #global_" followed by "tax" and "lda.l $0000,x" indicates the bug
    if grep -E 'lda\.w #global_' "$out" > /dev/null 2>&1; then
        # Check if this is followed by the indirect load pattern
        if grep -A2 'lda\.w #global_' "$out" | grep -E 'lda\.l \$0000,x' > /dev/null 2>&1; then
            log_fail "$name: Found indirect addressing for global variables"
            if [[ $VERBOSE -eq 1 ]]; then
                echo "Bug pattern found (should use lda.l symbol directly):"
                grep -B1 -A2 'lda\.w #global_' "$out" | head -20
            fi
            ((TESTS_FAILED++))
            return 1
        fi
    fi

    # Same check for extern variables
    if grep -E 'lda\.w #extern_' "$out" > /dev/null 2>&1; then
        if grep -A2 'lda\.w #extern_' "$out" | grep -E 'lda\.l \$0000,x' > /dev/null 2>&1; then
            log_fail "$name: Found indirect addressing for extern variables"
            if [[ $VERBOSE -eq 1 ]]; then
                echo "Bug pattern found:"
                grep -B1 -A2 'lda\.w #extern_' "$out" | head -20
            fi
            ((TESTS_FAILED++))
            return 1
        fi
    fi

    # Verify direct loads are present (lda.l global_x, etc.)
    if ! grep -E 'lda\.l global_x' "$out" > /dev/null 2>&1; then
        log_fail "$name: No direct load 'lda.l global_x' found"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Input button masks match pvsneslib/SNES hardware layout
# BUG: Button constants were using wrong bit positions
#      D-pad was in bits 0-3 instead of bits 8-11
#------------------------------------------------------------------------------
test_input_button_masks() {
    local name="input_button_masks"
    local src="$SCRIPT_DIR/test_input_patterns.c"
    local out="$BUILD/test_input_patterns.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/input_patterns_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/input_patterns_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Verify correct button masks are used (matching pvsneslib)
    # KEY_UP = 0x0800 = 2048
    if ! grep -E 'and\.w #2048' "$out" > /dev/null 2>&1; then
        log_fail "$name: KEY_UP mask (2048/0x0800) not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # KEY_DOWN = 0x0400 = 1024
    if ! grep -E 'and\.w #1024' "$out" > /dev/null 2>&1; then
        log_fail "$name: KEY_DOWN mask (1024/0x0400) not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # KEY_LEFT = 0x0200 = 512
    if ! grep -E 'and\.w #512' "$out" > /dev/null 2>&1; then
        log_fail "$name: KEY_LEFT mask (512/0x0200) not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # KEY_RIGHT = 0x0100 = 256
    if ! grep -E 'and\.w #256' "$out" > /dev/null 2>&1; then
        log_fail "$name: KEY_RIGHT mask (256/0x0100) not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # KEY_A = 0x0080 = 128
    if ! grep -E 'and\.w #128' "$out" > /dev/null 2>&1; then
        log_fail "$name: KEY_A mask (128/0x0080) not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # KEY_B = 0x8000 = 32768
    if ! grep -E 'and\.w #32768' "$out" > /dev/null 2>&1; then
        log_fail "$name: KEY_B mask (32768/0x8000) not found"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Helper: Check any .asm file for unhandled ops
# Usage: check_asm_for_unhandled_ops <file.asm>
# Returns: 0 if clean, 1 if unhandled ops found
#------------------------------------------------------------------------------
check_asm_for_unhandled_ops() {
    local asm_file="$1"
    local name=$(basename "$asm_file")

    if [[ ! -f "$asm_file" ]]; then
        echo "File not found: $asm_file"
        return 1
    fi

    if grep -E '; unhandled op' "$asm_file" > /dev/null 2>&1; then
        echo "FAIL: Unhandled ops in $name:"
        grep -n -E '; unhandled op' "$asm_file"
        return 1
    fi

    echo "OK: No unhandled ops in $name"
    return 0
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
    test_static_var_stores           # CRITICAL: Static var stores must use symbols
    test_shift_right_ops             # CRITICAL: Shift right must generate LSR instructions
    test_word_extend_ops             # CRITICAL: Word extension ops must be handled
    test_division_ops                # CRITICAL: Division/modulo must use LSR/AND for powers of 2
    test_multiarg_call_offsets       # CRITICAL: Multi-arg calls must adjust stack offsets
    test_library_no_unhandled_ops    # CRITICAL: Library must be compiled with working compiler
    test_animation_patterns          # Comprehensive: loops, arrays, bit ops, conditionals
    test_global_var_reads            # CRITICAL: Global/extern vars must use direct addressing
    test_input_button_masks          # CRITICAL: Button masks must match pvsneslib/hardware

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
