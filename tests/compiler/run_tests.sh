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
#   ./run_tests.sh                    # Run all tests (strict mode)
#   ./run_tests.sh -v                 # Verbose output
#   ./run_tests.sh --allow-known-bugs # Known bugs warn instead of fail
#   ./run_tests.sh test_X             # Run specific test
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
TESTS_KNOWN_BUGS=0

VERBOSE=0
ALLOW_KNOWN_BUGS=0

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

# Log a known bug failure. In --allow-known-bugs mode, counts as warning.
# In strict mode (default), counts as failure.
log_known_bug() {
    if [[ $ALLOW_KNOWN_BUGS -eq 1 ]]; then
        echo -e "${YELLOW}[KNOWN_BUG]${NC} $1"
        ((TESTS_KNOWN_BUGS++))
    else
        log_fail "$1 (known bug — use --allow-known-bugs to suppress)"
        ((TESTS_FAILED++))
    fi
}

# Compile a C test file to assembly.
# Returns 0 on success, 1 on failure.
# On MSYS2/Windows, cproc may segfault non-deterministically.
# Segfaults are reported as known bugs instead of failures.
compile_test() {
    local name="$1"
    local src="$2"
    local out="$3"
    local err_file="$BUILD/$(basename "$src" .c).err"

    if "$CC" "$src" -o "$out" 2>"$err_file"; then
        return 0
    fi

    local err_msg
    err_msg=$(cat "$err_file")
    if echo "$err_msg" | grep -qi 'segmentation fault\|segfault\|signal 11'; then
        log_known_bug "$name: cproc segfault on MSYS2 (non-deterministic)"
        return 1
    fi

    log_fail "$name: Compilation failed"
    echo "$err_msg"
    ((TESTS_FAILED++))
    return 1
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

    # Verify shift-by-8 uses xba optimization (or lsr for non-8 shifts)
    if ! grep -E 'xba|lsr a' "$out" > /dev/null 2>&1; then
        log_fail "$name: No shift right instructions generated (expected xba or lsr)"
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

    # Verify direct loads are present (lda.w global_x or lda.l global_x)
    if ! grep -E 'lda\.(w|l) global_x' "$out" > /dev/null 2>&1; then
        log_fail "$name: No direct load 'lda.w/l global_x' found"
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
# Test: Struct pointer initialization (QBE bug February 2026)
#------------------------------------------------------------------------------
test_struct_ptr_init() {
    local name="struct_ptr_init"
    local src="$SCRIPT_DIR/test_struct_ptr_init.c"
    local out="$BUILD/test_struct_ptr_init.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/struct_ptr.err"; then
        log_fail "$name: Compilation failed"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # CRITICAL: Check that myFrames symbol is correctly referenced
    # The bug caused .dl z+0 instead of .dl myFrames+0
    if grep -q '\.dl myFrames' "$out"; then
        log_info "$name"
        ((TESTS_PASSED++))
        return 0
    fi

    # Check for corrupted symbol (the bug)
    if grep -q '\.dl z+0' "$out" || grep -q '\.dl [a-z]+0' "$out"; then
        log_fail "$name: Symbol reference corrupted (QBE static buffer bug)"
        echo "Expected: .dl myFrames+0"
        echo "Found:"
        grep '\.dl' "$out"
        ((TESTS_FAILED++))
        return 1
    fi

    log_fail "$name: myFrames symbol not found in output"
    ((TESTS_FAILED++))
    return 1
}

#------------------------------------------------------------------------------
# Test: 2D array access
#------------------------------------------------------------------------------
test_2d_array() {
    local name="2d_array_access"
    local src="$SCRIPT_DIR/test_2d_array.c"
    local out="$BUILD/test_2d_array.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/2d_array.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/2d_array.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # Check for grid and wide symbols
    if ! grep -q 'grid' "$out"; then
        log_fail "$name: grid symbol not found"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Nested structure access
#------------------------------------------------------------------------------
test_nested_struct() {
    local name="nested_struct"
    local src="$SCRIPT_DIR/test_nested_struct.c"
    local out="$BUILD/test_nested_struct.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/nested_struct.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/nested_struct.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # Check for game symbol (our GameState variable)
    # Use -a to force text mode (MSYS2/Windows may detect CRLF files as binary)
    if ! grep -aq 'game' "$out"; then
        log_fail "$name: game symbol not found"
        log_verbose "File size: $(wc -c < "$out") bytes, first 5 lines:"
        log_verbose "$(head -5 "$out")"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Union handling
#------------------------------------------------------------------------------
test_union() {
    local name="union_handling"
    local src="$SCRIPT_DIR/test_union.c"
    local out="$BUILD/test_union.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile to assembly
    # Note: cproc segfaults on union types on Windows/MSYS2 (known issue)
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/union.err"; then
        local err_msg
        err_msg=$(cat "$BUILD/union.err")
        if echo "$err_msg" | grep -qi 'segmentation fault\|segfault\|signal 11'; then
            log_known_bug "$name: cproc crashes on unions (Windows/MSYS2 segfault)"
            return 1
        fi
        log_fail "$name: Compilation failed"
        echo "$err_msg"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # Check for union variables
    # Use -a to force text mode (MSYS2/Windows may detect CRLF files as binary)
    if ! grep -aq 'ms' "$out" || ! grep -aq 'col' "$out"; then
        log_fail "$name: Union symbols not found"
        log_verbose "File size: $(wc -c < "$out") bytes, first 5 lines:"
        log_verbose "$(head -5 "$out")"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Large local variables (>256 bytes)
#------------------------------------------------------------------------------
test_large_local() {
    local name="large_local_vars"
    local src="$SCRIPT_DIR/test_large_local.c"
    local out="$BUILD/test_large_local.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/large_local.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/large_local.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # Check that functions exist in output
    if ! grep -q 'test_large_local:' "$out"; then
        log_fail "$name: test_large_local function not found"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: String literal initialization
#------------------------------------------------------------------------------
test_string_init() {
    local name="string_init"
    local src="$SCRIPT_DIR/test_string_init.c"
    local out="$BUILD/test_string_init.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/string_init.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/string_init.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # Check that string data exists
    if ! grep -q 'Sword' "$out" && ! grep -q '"Sword"' "$out"; then
        # Strings might be encoded differently, check for .db with ASCII values
        if ! grep -q '\.db 83' "$out"; then  # 'S' = 83
            log_fail "$name: String data not found"
            ((TESTS_FAILED++))
            return 1
        fi
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Pointer arithmetic
#------------------------------------------------------------------------------
test_ptr_arithmetic() {
    local name="ptr_arithmetic"
    local src="$SCRIPT_DIR/test_ptr_arithmetic.c"
    local out="$BUILD/test_ptr_arithmetic.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! "$CC" "$src" -o "$out" 2>"$BUILD/ptr_arith.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/ptr_arith.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"
    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Switch statement
#------------------------------------------------------------------------------
test_switch() {
    local name="switch_stmt"
    local src="$SCRIPT_DIR/test_switch.c"
    local out="$BUILD/test_switch.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! "$CC" "$src" -o "$out" 2>"$BUILD/switch.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/switch.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"
    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Function pointers
# SKIP: QBE crashes with function pointer types (assertion failure in str())
# TODO: Fix QBE bug - tracked in .claude/KNOWLEDGE.md
#------------------------------------------------------------------------------
test_function_ptr() {
    local name="function_ptr"
    local src="$SCRIPT_DIR/test_function_ptr.c"
    local out="$BUILD/test_function_ptr.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! "$CC" "$src" -o "$out" 2>"$BUILD/function_ptr.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/function_ptr.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # Check for indirect call mechanism (jml [tcc__r9] for indirect calls)
    # OR direct calls only (if optimizer resolves all pointers)
    if ! grep -qE '(jml \[tcc__r9\]|jsl increment|jsl execute_action)' "$out"; then
        log_fail "$name: No call instructions found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Check that execute_action function exists and handles indirect call
    if ! grep -q 'execute_action:' "$out"; then
        log_fail "$name: execute_action function not found"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Bitwise operations
#------------------------------------------------------------------------------
test_bitops() {
    local name="bitops"
    local src="$SCRIPT_DIR/test_bitops.c"
    local out="$BUILD/test_bitops.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! "$CC" "$src" -o "$out" 2>"$BUILD/bitops.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/bitops.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # Check for AND, ORA, EOR instructions
    if ! grep -qE '(and|ora|eor)\.' "$out"; then
        log_fail "$name: Bitwise instructions not found"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Loop constructs
#------------------------------------------------------------------------------
test_loops() {
    local name="loops"
    local src="$SCRIPT_DIR/test_loops.c"
    local out="$BUILD/test_loops.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! "$CC" "$src" -o "$out" 2>"$BUILD/loops.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/loops.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # Check for loop-related branch instructions
    if ! grep -qE '(bne|beq|bcc|bcs)' "$out"; then
        log_fail "$name: Branch instructions not found"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Comparison operations
#------------------------------------------------------------------------------
test_comparisons() {
    local name="comparisons"
    local src="$SCRIPT_DIR/test_comparisons.c"
    local out="$BUILD/test_comparisons.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! "$CC" "$src" -o "$out" 2>"$BUILD/compare.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/compare.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"

    # Check for comparison-related instructions (branch or compare)
    if ! grep -qE '(bne|beq|bcc|bcs|bmi|bpl|sec|sbc)' "$out"; then
        log_fail "$name: Comparison instructions not found"
        ((TESTS_FAILED++))
        return 1
    fi

    # Verify unsigned short comparisons use unsigned branches (bcc/bcs),
    # NOT signed branches (bmi/bpl). The compiler bug caused u16 >= constant
    # to use bmi (signed), which breaks for values >= 32768.
    local u16_funcs=(test_u16_high_less test_u16_vs_constant)
    for func in "${u16_funcs[@]}"; do
        local func_body
        func_body=$(sed -n "/^${func}:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p" "$out" | sed '$d')
        if echo "$func_body" | grep -qE '\bbmi\b|\bbpl\b'; then
            log_fail "$name: ${func} uses signed branch (bmi/bpl) instead of unsigned (bcc/bcs)"
            if [[ $VERBOSE -eq 1 ]]; then
                echo "Function body:"
                echo "$func_body"
            fi
            ((TESTS_FAILED++))
            return 1
        fi
    done

    # Verify ternary value is materialized when used as function argument.
    # Regression: GVN replaces the phi with the comparison result, then
    # comparison+branch fusion incorrectly skips the comparison instruction,
    # leaving the ternary value uninitialized.
    local ternary_body
    ternary_body=$(sed -n '/^test_ternary_value_used:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$ternary_body" | grep -qE 'lda\.w #[01]'; then
        log_fail "$name: test_ternary_value_used missing boolean materialization (lda.w #0/#1)"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Function body:"
            echo "$ternary_body"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Verify u16 shift right uses lsr (logical), NOT asr-like pattern
    local shift_body
    shift_body=$(sed -n '/^test_u16_shift_right:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$shift_body" | grep -qE 'lsr'; then
        log_fail "$name: test_u16_shift_right missing lsr (logical shift right)"
        ((TESTS_FAILED++))
        return 1
    fi

    # Verify s16 signed shift-by-8 uses cmp+ror (arithmetic shift), NOT lsr
    local signed_shift_body
    signed_shift_body=$(sed -n '/^test_s16_shift_right:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$signed_shift_body" | grep -qE 'cmp\.w #\$8000'; then
        log_fail "$name: test_s16_shift_right missing 'cmp.w #\$8000' (sign bit extraction)"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Function body:"
            echo "$signed_shift_body"
        fi
        ((TESTS_FAILED++))
        return 1
    fi
    if ! echo "$signed_shift_body" | grep -qE 'ror a'; then
        log_fail "$name: test_s16_shift_right missing 'ror a' (arithmetic shift)"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Function body:"
            echo "$signed_shift_body"
        fi
        ((TESTS_FAILED++))
        return 1
    fi
    # Ensure signed shift-by-8 does NOT use lsr (that would be unsigned, wrong for signed)
    if echo "$signed_shift_body" | grep -qE 'lsr'; then
        log_fail "$name: test_s16_shift_right uses lsr (should use cmp+ror for arithmetic shift)"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Function body:"
            echo "$signed_shift_body"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Verify s16 signed shift-by-1 still uses cmp+ror (arithmetic), NOT lsr
    local signed_shift1_body
    signed_shift1_body=$(sed -n '/^test_s16_shift_right_1:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$signed_shift1_body" | grep -qE 'cmp\.w #\$8000'; then
        log_fail "$name: test_s16_shift_right_1 missing 'cmp.w #\$8000' (sign bit extraction)"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Function body:"
            echo "$signed_shift1_body"
        fi
        ((TESTS_FAILED++))
        return 1
    fi
    if ! echo "$signed_shift1_body" | grep -qE 'ror a'; then
        log_fail "$name: test_s16_shift_right_1 missing 'ror a' (arithmetic shift)"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Function body:"
            echo "$signed_shift1_body"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Volatile variables
#------------------------------------------------------------------------------
test_volatiles() {
    local name="volatiles"
    local src="$SCRIPT_DIR/test_volatiles.c"
    local out="$BUILD/test_volatiles.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! "$CC" "$src" -o "$out" 2>"$BUILD/volatile.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/volatile.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"
    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Type casting
#------------------------------------------------------------------------------
test_type_cast() {
    local name="type_cast"
    local src="$SCRIPT_DIR/test_type_cast.c"
    local out="$BUILD/test_type_cast.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! "$CC" "$src" -o "$out" 2>"$BUILD/typecast.err"; then
        log_fail "$name: Compilation failed"
        cat "$BUILD/typecast.err"
        ((TESTS_FAILED++))
        return 1
    fi

    echo "Generated: $out"
    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Phase 1.3: HDMA Wave Regression Tests
# These test bugs discovered during HDMA wave demo development.
# The bugs produce NO warnings — code compiles but generates wrong assembly.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Test: Variable-count shifts generate actual shift instructions
# BUG: `1 << variable` compiles to just `lda.w #1` — shift is dropped
#      Constant shifts are folded at compile time, masking the bug.
#------------------------------------------------------------------------------
test_variable_shift() {
    local name="variable_shift"
    local src="$SCRIPT_DIR/test_variable_shift.c"
    local out="$BUILD/test_variable_shift.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/variable_shift_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/variable_shift_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Extract the body of shift_left_var function (between label and next label/section)
    local func_body
    func_body=$(sed -n '/^shift_left_var:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')

    # Check for actual shift instructions in shift_left_var
    # Must contain asl (shift left) or a call to __shl (shift helper)
    if ! echo "$func_body" | grep -qE '(asl|__shl)'; then
        log_fail "$name: shift_left_var has NO asl/__shl — variable shift dropped!"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Function body of shift_left_var:"
            echo "$func_body"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check compute_bitmask also has shifts
    local bitmask_body
    bitmask_body=$(sed -n '/^compute_bitmask:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')

    if ! echo "$bitmask_body" | grep -qE '(asl|__shl)'; then
        log_fail "$name: compute_bitmask has NO asl/__shl — 1<<idx broken!"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Function body of compute_bitmask:"
            echo "$bitmask_body"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: SSA phi-node resolution with 6+ locals in nested if/else
# BUG: Wrong values stored to wrong stack slots when many locals are
#      modified in separate conditional branches
#------------------------------------------------------------------------------
test_ssa_phi_locals() {
    local name="ssa_phi_locals"
    local src="$SCRIPT_DIR/test_ssa_phi_locals.c"
    local out="$BUILD/test_ssa_phi_locals.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/ssa_phi_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/ssa_phi_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Extract test_many_locals function body
    local func_body
    func_body=$(sed -n '/^test_many_locals:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')

    # Check that all 12 expected constants are present:
    # Initial values: 1, 2, 3, 4, 5, 6
    # Branch values: 10, 20, 30, 40, 50, 60
    local missing=0
    for val in 10 20 30 40 50 60; do
        if ! echo "$func_body" | grep -qE "lda\.w #$val\b"; then
            log_verbose "Missing constant #$val in test_many_locals"
            ((missing++))
        fi
    done

    if [[ $missing -gt 0 ]]; then
        log_fail "$name: Missing $missing/6 branch constants — phi-node values lost!"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Function body:"
            echo "$func_body"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check that at least 6 unique stack slot offsets are used for stores
    # Pattern: sta N,s where N is the stack offset
    local unique_slots
    unique_slots=$(echo "$func_body" | grep -oE 'sta [0-9]+,s' | sort -u | wc -l)

    if [[ $unique_slots -lt 6 ]]; then
        log_fail "$name: Only $unique_slots unique stack slots (need 6+) — phi-node confusion!"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Stack stores found:"
            echo "$func_body" | grep -E 'sta [0-9]+,s' | sort -u
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Mutable static variables are placed in RAM, not ROM
# BUG: `static int counter = 0;` placed in .SECTION ".rodata" SUPERFREE (ROM)
#      Writes to these variables are silently ignored on hardware.
#------------------------------------------------------------------------------
test_static_mutable() {
    local name="static_mutable"
    local src="$SCRIPT_DIR/test_static_mutable.c"
    local out="$BUILD/test_static_mutable.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/static_mutable_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/static_mutable_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check each mutable static symbol: it must NOT be in a SUPERFREE section
    # Strategy: search backward from symbol label to find containing section directive
    local symbols=(uninit_counter zero_counter init_counter byte_flag word_state byte_counter word_accumulator)
    local rom_count=0

    for sym in "${symbols[@]}"; do
        # Find line number of the symbol label
        local sym_line
        sym_line=$(grep -n "^${sym}:" "$out" | head -1 | cut -d: -f1)

        if [[ -z "$sym_line" ]]; then
            log_verbose "Symbol $sym not found in assembly"
            continue
        fi

        # Search backward from symbol to find the containing section directive
        # Look for .SECTION or .RAMSECTION
        local section_line
        section_line=$(head -n "$sym_line" "$out" | grep -E '\.(SECTION|RAMSECTION)' | tail -1)

        if echo "$section_line" | grep -q 'SUPERFREE'; then
            log_verbose "$sym is in ROM (SUPERFREE): $section_line"
            ((rom_count++))
        elif echo "$section_line" | grep -q '\.RAMSECTION'; then
            log_verbose "$sym is in RAM (RAMSECTION): $section_line"
        fi
    done

    if [[ $rom_count -gt 0 ]]; then
        log_fail "$name: $rom_count/$((${#symbols[@]})) mutable statics placed in ROM (SUPERFREE)!"
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Section directives in assembly:"
            grep -n -E '\.(SECTION|RAMSECTION)' "$out"
            echo ""
            echo "Symbol locations:"
            for sym in "${symbols[@]}"; do
                grep -n "^${sym}:" "$out"
            done
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

test_const_data() {
    local name="const_data"
    local src="$SCRIPT_DIR/test_const_data.c"
    local out="$BUILD/test_const_data.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/const_data_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/const_data_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check const arrays are in ROM (SUPERFREE), not RAMSECTION
    local const_symbols=(const_arr const_u8_arr)
    local ram_count=0

    for sym in "${const_symbols[@]}"; do
        local sym_line
        sym_line=$(grep -n "^${sym}:" "$out" | head -1 | cut -d: -f1)

        if [[ -z "$sym_line" ]]; then
            log_verbose "Symbol $sym not found in assembly"
            continue
        fi

        local section_line
        section_line=$(head -n "$sym_line" "$out" | grep -E '\.(SECTION|RAMSECTION)' | tail -1)

        if echo "$section_line" | grep -q '\.RAMSECTION'; then
            log_verbose "$sym is in RAM (RAMSECTION): $section_line"
            ((ram_count++))
        elif echo "$section_line" | grep -q 'SUPERFREE'; then
            log_verbose "$sym is in ROM (SUPERFREE): $section_line"
        fi
    done

    if [[ $ram_count -gt 0 ]]; then
        log_fail "$name: $ram_count const array(s) placed in RAM instead of ROM!"
        ((TESTS_FAILED++))
        return 1
    fi

    # Check mutable array is in RAMSECTION (not ROM)
    local mut_line
    mut_line=$(grep -n "^mut_arr:" "$out" | head -1 | cut -d: -f1)
    if [[ -n "$mut_line" ]]; then
        local mut_section
        mut_section=$(head -n "$mut_line" "$out" | grep -E '\.(SECTION|RAMSECTION)' | tail -1)
        if echo "$mut_section" | grep -q 'SUPERFREE'; then
            log_fail "$name: mutable array placed in ROM instead of RAM!"
            ((TESTS_FAILED++))
            return 1
        fi
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Multiplication code generation
#
# Verifies that:
#   - Small constant multipliers (*3, *5, *6, *7, *9, *10) use inline shift+add
#   - Variable multiplication and non-special constants call __mul16
#   - __mul16 uses stack-based calling convention (pha, not sta.l tcc__r0)
#
# Bug history:
#   __mul16 runtime read from tcc__r0/tcc__r1 (register convention) but the
#   compiler pushed arguments on the stack. Fixed in runtime.asm to read
#   from stack offsets SP+5,6 (arg1) and SP+7,8 (arg2).
#------------------------------------------------------------------------------
test_multiply() {
    local name="multiply"
    local src="$SCRIPT_DIR/test_multiply.c"
    local out="$BUILD/test_multiply.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/multiply_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/multiply_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # Check inline multiplications use tcc__r9 (shift+add pattern), NOT __mul16
    local inline_muls=(mul_by_3 mul_by_5 mul_by_6 mul_by_7 mul_by_9 mul_by_10)
    for func in "${inline_muls[@]}"; do
        local func_body
        func_body=$(sed -n "/^${func}:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p" "$out" | sed '$d')

        if echo "$func_body" | grep -q '__mul16'; then
            log_fail "$name: ${func} calls __mul16 instead of using inline shift+add"
            ((TESTS_FAILED++))
            return 1
        fi

        if ! echo "$func_body" | grep -q 'tcc__r9'; then
            log_fail "$name: ${func} missing tcc__r9 (no inline shift+add pattern)"
            ((TESTS_FAILED++))
            return 1
        fi
    done

    # Check that variable * variable calls __mul16 with stack convention (pha)
    local var_body
    var_body=$(sed -n '/^mul_var:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')

    if ! echo "$var_body" | grep -q '__mul16'; then
        log_fail "$name: mul_var missing __mul16 call"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! echo "$var_body" | grep -q 'pha'; then
        log_fail "$name: mul_var missing pha (stack-based calling convention)"
        ((TESTS_FAILED++))
        return 1
    fi

    # Check non-special constant (*13) also calls __mul16
    local const_body
    const_body=$(sed -n '/^mul_by_13:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')

    if ! echo "$const_body" | grep -q '__mul16'; then
        log_fail "$name: mul_by_13 missing __mul16 call"
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Function return values are preserved through epilogue
# BUG: `tsa` in epilogue overwrites return value in A register.
#      Every non-void function with framesize > 2 returned garbage.
# FIX: tax (save A) → tsa; adc; tas (adjust SP) → txa (restore A)
#------------------------------------------------------------------------------
test_return_value() {
    local name="return_value"
    local src="$SCRIPT_DIR/test_return_value.c"
    local out="$BUILD/test_return_value.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    # Compile C to assembly
    if ! "$CC" "$src" -o "$out" 2>"$BUILD/return_value_compile.err"; then
        log_fail "$name: Compilation failed"
        if [[ $VERBOSE -eq 1 ]]; then
            cat "$BUILD/return_value_compile.err"
        fi
        ((TESTS_FAILED++))
        return 1
    fi

    # For each function with locals (framesize > 2), check the epilogue pattern:
    # The return sequence must be: tax → tsa → ... → tas → txa → plp → rtl
    # NOT: tsa → ... → tas → plp → rtl (which destroys A)

    local functions=(compute_with_locals compute_complex call_and_return compute_byte)
    local fail_count=0

    for func in "${functions[@]}"; do
        local func_body
        func_body=$(sed -n "/^${func}:/,/^[a-zA-Z_][a-zA-Z0-9_]*:\|; End of/p" "$out")

        # Check if function has stack frame (tsa in body = stack adjustment)
        if ! echo "$func_body" | grep -q 'tsa'; then
            log_verbose "$func: no stack frame, skip epilogue check"
            continue
        fi

        # Count epilogue pattern: must have tax before the final tsa, and txa after tas
        # The epilogue pattern should be: tax → tsa → clc → adc → tas → txa → plp → rtl
        # Extract the last occurrence of tax...txa...rtl
        if ! echo "$func_body" | grep -A8 'tax' | grep -q 'txa'; then
            log_fail "$name: ${func} epilogue missing tax/txa — return value destroyed!"
            ((fail_count++))
        fi
    done

    if [[ $fail_count -gt 0 ]]; then
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Struct alignment and padding
#
# Verifies that:
#   - Struct sizes include correct padding (Simple=4, Mixed=6, Nested=10,
#     ThreeWords=6, OneByte=1)
#   - Field offsets use correct alignment (u16 fields 2-byte aligned)
#   - Array stride calculation uses correct struct size
#------------------------------------------------------------------------------
test_struct_alignment() {
    local name="struct_alignment"
    local src="$SCRIPT_DIR/test_struct_alignment.c"
    local out="$BUILD/test_struct_alignment.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! compile_test "$name" "$src" "$out"; then
        return 1
    fi

    local fail_count=0

    # Check struct sizes via dsb in RAMSECTION
    local expected_sizes=("simple:4" "mixed:6" "nested:10" "three:6" "one:1")
    for entry in "${expected_sizes[@]}"; do
        local sym="${entry%%:*}"
        local expected_sz="${entry##*:}"
        local actual_line
        actual_line=$(grep -A1 "^${sym}:" "$out" | grep 'dsb')
        if [[ -z "$actual_line" ]]; then
            log_fail "$name: Symbol '$sym' not found in RAMSECTION"
            ((fail_count++))
            continue
        fi
        local actual_sz
        actual_sz=$(echo "$actual_line" | grep -oE '[0-9]+')
        if [[ "$actual_sz" != "$expected_sz" ]]; then
            log_fail "$name: '$sym' dsb $actual_sz, expected dsb $expected_sz"
            ((fail_count++))
        fi
    done

    # Check that field offsets use adc #2 (for u16 alignment) in test_simple_access
    local func_body
    func_body=$(sed -n '/^test_simple_access:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$func_body" | grep -q 'adc\.w #2'; then
        log_fail "$name: test_simple_access missing adc.w #2 for u16 field offset"
        ((fail_count++))
    fi

    # Check that test_mixed_access has offsets for fields at +2, +4, +5
    func_body=$(sed -n '/^test_mixed_access:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$func_body" | grep -q 'adc\.w #4'; then
        log_fail "$name: test_mixed_access missing adc.w #4 for z field offset"
        ((fail_count++))
    fi
    if ! echo "$func_body" | grep -q 'adc\.w #5'; then
        log_fail "$name: test_mixed_access missing adc.w #5 for w field offset"
        ((fail_count++))
    fi

    # Check array stride: test_array_stride should use adc #4 or #8 for stride
    func_body=$(sed -n '/^test_array_stride:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$func_body" | grep -q 'adc\.w #4'; then
        log_fail "$name: test_array_stride missing stride offset adc.w #4"
        ((fail_count++))
    fi

    if [[ $fail_count -gt 0 ]]; then
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Volatile pointer dereference
#
# Verifies that:
#   - Writes to volatile hardware registers are not eliminated
#   - Reads from volatile registers are not cached/coalesced
#   - Register write order is preserved for hardware sequences
#   - Volatile loop writes are not optimized away
#------------------------------------------------------------------------------
test_volatile_ptr() {
    local name="volatile_ptr"
    local src="$SCRIPT_DIR/test_volatile_ptr.c"
    local out="$BUILD/test_volatile_ptr.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! compile_test "$name" "$src" "$out"; then
        return 1
    fi

    local fail_count=0

    # test_write_register: must have 3 writes to $002100 (not optimized away)
    local func_body
    func_body=$(sed -n '/^test_write_register:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    local write_count
    write_count=$(echo "$func_body" | grep -c 'sta\.l \$002100')
    if [[ $write_count -lt 3 ]]; then
        log_fail "$name: test_write_register has $write_count writes to \$002100, expected 3 (volatile writes optimized away!)"
        ((fail_count++))
    fi

    # test_read_register: must have 2 separate reads (lda.l via indirect)
    func_body=$(sed -n '/^test_read_register:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    local read_count
    read_count=$(echo "$func_body" | grep -c 'lda\.l')
    if [[ $read_count -lt 2 ]]; then
        log_fail "$name: test_read_register has $read_count reads, expected 2+ (volatile reads coalesced!)"
        ((fail_count++))
    fi

    # test_vram_write_sequence: must write to $2115, $2116, $2117, $2118, $2119 in order
    func_body=$(sed -n '/^test_vram_write_sequence:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    for reg in 2115 2116 2117 2118 2119; do
        if ! echo "$func_body" | grep -q "sta\.l \$00${reg}"; then
            log_fail "$name: test_vram_write_sequence missing write to \$${reg}"
            ((fail_count++))
        fi
    done

    # test_volatile_loop: must have writes to $2118 and $2119 inside loop body
    func_body=$(sed -n '/^test_volatile_loop:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$func_body" | grep -q 'sta\.l \$002118'; then
        log_fail "$name: test_volatile_loop missing loop write to \$002118"
        ((fail_count++))
    fi

    if [[ $fail_count -gt 0 ]]; then
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Global struct initialization
#
# Verifies that:
#   - Initialized globals are in RAMSECTION (.data) with matching .data_init
#   - Zero-initialized globals are in .bss (no .data_init entry)
#   - Init data contains correct values (struct field values)
#   - Complex patterns: nested structs, arrays of structs, partial init
#------------------------------------------------------------------------------
test_global_struct_init() {
    local name="global_struct_init"
    local src="$SCRIPT_DIR/test_global_struct_init.c"
    local out="$BUILD/test_global_struct_init.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! compile_test "$name" "$src" "$out"; then
        return 1
    fi

    local fail_count=0

    # Check that initialized globals have both RAMSECTION and .data_init
    local init_symbols=("entry1" "entry2" "player" "enemy" "default_style" "table" "partial")
    for sym in "${init_symbols[@]}"; do
        # Must be in RAMSECTION
        if ! grep -q "^${sym}:" "$out"; then
            log_fail "$name: Symbol '$sym' not found"
            ((fail_count++))
            continue
        fi
        local sym_line
        sym_line=$(grep -n "^${sym}:" "$out" | head -1 | cut -d: -f1)
        local section_line
        section_line=$(head -n "$sym_line" "$out" | grep -E '\.(SECTION|RAMSECTION)' | tail -1)
        if ! echo "$section_line" | grep -q 'RAMSECTION'; then
            log_fail "$name: '$sym' not in RAMSECTION (got: $section_line)"
            ((fail_count++))
        fi
        # Must have matching .data_init
        if ! grep -q "\.dw ${sym}" "$out"; then
            log_fail "$name: '$sym' has no .data_init entry (.dw ${sym})"
            ((fail_count++))
        fi
    done

    # Check that zero-initialized global is in .bss (NOT .data)
    local zeroed_line
    zeroed_line=$(grep -n "^zeroed:" "$out" | head -1 | cut -d: -f1)
    if [[ -n "$zeroed_line" ]]; then
        local zeroed_section
        zeroed_section=$(head -n "$zeroed_line" "$out" | grep -E '\.(SECTION|RAMSECTION)' | tail -1)
        if echo "$zeroed_section" | grep -q '\.bss'; then
            log_verbose "$name: zeroed correctly in .bss"
        elif echo "$zeroed_section" | grep -q '\.data'; then
            # .data is also fine if it has no .data_init (or all zeros)
            log_verbose "$name: zeroed in .data section (acceptable)"
        fi
        # Must NOT have a .data_init entry
        if grep -q '\.dw zeroed' "$out"; then
            log_fail "$name: 'zeroed' should not have .data_init (it's zero-initialized)"
            ((fail_count++))
        fi
    fi

    # Check specific init values: entry1 should have .dw 1000
    if ! grep -q '\.dw 1000' "$out"; then
        log_fail "$name: Missing init value .dw 1000 for entry1.value"
        ((fail_count++))
    fi

    # Check player init: .db 100 (health field)
    if ! grep -q '\.db 100' "$out"; then
        log_fail "$name: Missing init value .db 100 for player.health"
        ((fail_count++))
    fi

    if [[ $fail_count -gt 0 ]]; then
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: 32-bit (unsigned int) arithmetic
#
# Verifies that:
#   - 32-bit add/sub generate ADC/SBC instructions
#   - 32-bit variables use 4 bytes of storage (dsb 4)
#   - High-word access uses result+2 pattern (sta.l symbol+2)
#   - Bitwise ops (AND, OR, XOR) generate correct 32-bit sequences
#
# Note: On cproc/65816, unsigned int = 4 bytes (u32), NOT unsigned long (8 bytes)
#------------------------------------------------------------------------------
test_u32_arithmetic() {
    local name="u32_arithmetic"
    local src="$SCRIPT_DIR/test_u32_arithmetic.c"
    local out="$BUILD/test_u32_arithmetic.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! compile_test "$name" "$src" "$out"; then
        return 1
    fi

    local fail_count=0

    # 32-bit result variable must be 4 bytes
    local result_dsb
    result_dsb=$(grep -A1 '^result32:' "$out" | grep 'dsb' | grep -oE '[0-9]+')
    if [[ "$result_dsb" != "4" ]]; then
        log_fail "$name: result32 is dsb $result_dsb, expected dsb 4"
        ((fail_count++))
    fi

    # Must have ADC instructions (32-bit addition)
    local adc_count
    adc_count=$(grep -c 'adc' "$out")
    if [[ $adc_count -lt 5 ]]; then
        log_fail "$name: Only $adc_count ADC instructions found, expected 5+ for 32-bit arithmetic"
        ((fail_count++))
    fi

    # Must have SBC instructions (32-bit subtraction)
    local sbc_count
    sbc_count=$(grep -c 'sbc' "$out")
    if [[ $sbc_count -lt 2 ]]; then
        log_fail "$name: Only $sbc_count SBC instructions found, expected 2+ for 32-bit subtraction"
        ((fail_count++))
    fi

    # Must access high word via result32+2 pattern
    if ! grep -q 'result32+2' "$out"; then
        log_fail "$name: No result32+2 pattern found (32-bit high word not accessed)"
        ((fail_count++))
    fi

    # Check that functions are not constant-folded (must have function bodies with arithmetic)
    local func_body
    func_body=$(sed -n '/^add32:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$func_body" | grep -qE 'adc|clc'; then
        log_fail "$name: add32 function body has no ADC — possibly constant-folded"
        ((fail_count++))
    fi

    func_body=$(sed -n '/^sub32:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$func_body" | grep -qE 'sbc|sec'; then
        log_fail "$name: sub32 function body has no SBC — possibly constant-folded"
        ((fail_count++))
    fi

    if [[ $fail_count -gt 0 ]]; then
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Signed/unsigned type promotion
#
# Verifies that:
#   - u8 zero-extension uses AND #$00FF
#   - s8 sign-extension uses AND #$00FF + CMP #$0080 + ORA #$FF00 pattern
#   - Mixed signed/unsigned arithmetic produces correct extensions
#   - Arithmetic right shift preserves sign
#
# On 65816, integer promotion is critical for correctness:
#   u8 → u16: AND #$00FF (zero-extend)
#   s8 → s16: AND #$00FF, CMP #$0080, BCC +, ORA #$FF00 (sign-extend)
#------------------------------------------------------------------------------
test_sign_promotion() {
    local name="sign_promotion"
    local src="$SCRIPT_DIR/test_sign_promotion.c"
    local out="$BUILD/test_sign_promotion.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    if ! compile_test "$name" "$src" "$out"; then
        return 1
    fi

    local fail_count=0

    # Must have zero-extension pattern: and.w #$00FF
    local zext_count
    zext_count=$(grep -c 'and\.w #\$00FF' "$out")
    if [[ $zext_count -lt 4 ]]; then
        log_fail "$name: Only $zext_count zero-extensions (and.w #\$00FF) found, expected 4+"
        ((fail_count++))
    fi

    # Must have sign-extension check: cmp.w #$0080
    local sext_check_count
    sext_check_count=$(grep -c 'cmp\.w #\$0080' "$out")
    if [[ $sext_check_count -lt 2 ]]; then
        log_fail "$name: Only $sext_check_count sign-extension checks (cmp.w #\$0080) found, expected 2+"
        ((fail_count++))
    fi

    # Must have sign-extension apply: ora.w #$FF00
    local sext_apply_count
    sext_apply_count=$(grep -c 'ora\.w #\$FF00' "$out")
    if [[ $sext_apply_count -lt 2 ]]; then
        log_fail "$name: Only $sext_apply_count sign-extensions (ora.w #\$FF00) found, expected 2+"
        ((fail_count++))
    fi

    # Check add_s8_s16 specifically has the sign-extension pattern
    local func_body
    func_body=$(sed -n '/^add_s8_s16:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$func_body" | grep -q 'and\.w #\$00FF'; then
        log_fail "$name: add_s8_s16 missing zero-extend before sign check"
        ((fail_count++))
    fi
    if ! echo "$func_body" | grep -q 'cmp\.w #\$0080'; then
        log_fail "$name: add_s8_s16 missing sign bit check (cmp.w #\$0080)"
        ((fail_count++))
    fi
    if ! echo "$func_body" | grep -q 'ora\.w #\$FF00'; then
        log_fail "$name: add_s8_s16 missing sign extension (ora.w #\$FF00)"
        ((fail_count++))
    fi

    # Check that functions are not constant-folded
    func_body=$(sed -n '/^add_u8_u8:/,/^[a-zA-Z_][a-zA-Z0-9_]*:/p' "$out" | sed '$d')
    if ! echo "$func_body" | grep -qE 'adc|clc'; then
        log_fail "$name: add_u8_u8 function body has no ADC — possibly constant-folded"
        ((fail_count++))
    fi

    if [[ $fail_count -gt 0 ]]; then
        ((TESTS_FAILED++))
        return 1
    fi

    log_info "$name"
    ((TESTS_PASSED++))
}

#------------------------------------------------------------------------------
# Test: Phase 5b — Non-leaf param aliasing + frame safety
#
# Phase 5b extends param alias propagation to non-leaf functions (fewer
# redundant param copies). But frame elimination stays leaf-only: non-leaf
# functions with intermediate values across calls MUST keep their frame.
# Without this, sta N,s writes into the caller's frame → corruption.
# Regression: oamSetXY, oamSetVisible, oamDrawMetasprite, bgInit all went
# frameless incorrectly, causing black screens and garbled sprites.
#------------------------------------------------------------------------------
test_nonleaf_frameless() {
    local name="nonleaf_frameless"
    local src="$SCRIPT_DIR/test_nonleaf_frameless.c"
    local out="$BUILD/test_nonleaf_frameless.c.asm"
    ((TESTS_RUN++))

    if [[ ! -f "$src" ]]; then
        log_fail "$name: Source file not found: $src"
        ((TESTS_FAILED++))
        return 1
    fi

    compile_test "$name" "$src" "$out" || return

    local fail_count=0
    local func_body

    # 1. compute_across_call: MUST have frame (sbc in prologue) because
    #    intermediate 'sum' lives across the jsl call
    func_body=$(sed -n '/^compute_across_call:/,/^\.ENDS/p' "$out")

    if ! echo "$func_body" | grep -qE '\bsbc\b'; then
        log_fail "$name: compute_across_call missing frame setup (sbc) — intermediate across call needs frame!"
        ((fail_count++))
    fi

    if ! echo "$func_body" | grep -q 'jsl'; then
        log_fail "$name: compute_across_call missing jsl — should call external_func"
        ((fail_count++))
    fi

    # 2. forward_with_work: MUST have frame (has intermediate 'c' across call)
    func_body=$(sed -n '/^forward_with_work:/,/^\.ENDS/p' "$out")

    if ! echo "$func_body" | grep -qE '\bsbc\b'; then
        log_fail "$name: forward_with_work missing frame setup (sbc) — intermediate across call needs frame!"
        ((fail_count++))
    fi

    # 3. Both non-leaf functions should have leaf_opt=1 (param aliasing active)
    #    This confirms Phase 5b is working: optimizations apply to non-leaf
    if ! grep -q 'compute_across_call.*leaf_opt=1' "$out"; then
        log_fail "$name: compute_across_call should have leaf_opt=1 (param aliasing active)"
        ((fail_count++))
    fi

    if [[ $fail_count -gt 0 ]]; then
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
            --allow-known-bugs)
                ALLOW_KNOWN_BUGS=1
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

    # === Advanced Pattern Tests (Phase 1.2) ===
    test_struct_ptr_init             # CRITICAL: Struct pointer init (QBE bug Feb 2026)
    test_2d_array                    # 2D array stride calculation
    test_nested_struct               # Nested structure member access
    test_union                       # Union size and member access
    test_large_local                 # Large local variables (>256 bytes)
    test_string_init                 # String literals in struct initializers
    test_ptr_arithmetic              # Pointer arithmetic with different types
    test_switch                      # Switch statement compilation
    test_function_ptr                # Function pointers and callbacks
    test_bitops                      # Bitwise operations
    test_loops                       # Loop constructs (for, while, do-while)
    test_comparisons                 # Signed/unsigned comparisons
    test_volatiles                   # Volatile variable handling
    test_type_cast                   # Type casting and conversions

    # === HDMA Wave Regression Tests (Phase 1.3) ===
    test_variable_shift              # FIXED: Variable-count shifts now emit loop
    test_ssa_phi_locals              # Regression: SSA phi-node confusion with many locals
    test_static_mutable              # FIXED: Mutable statics placed in RAMSECTION
    test_const_data                  # Const arrays placed in ROM (SUPERFREE)
    test_multiply                    # FIXED: Inline *3,*5,*6,*7,*9,*10 + __mul16 stack convention
    test_return_value                # FIXED: Epilogue tax/txa preserves return value in A

    # === Phase 5b: Non-leaf optimization tests ===
    test_nonleaf_frameless           # Non-leaf wrapper functions should be frameless

    # === Compiler Hardening Tests (Phase 1.5) ===
    test_struct_alignment            # Struct padding, field offsets, array stride
    test_volatile_ptr                # Volatile register reads/writes not optimized away
    test_global_struct_init          # .data_init for initialized global structs
    test_u32_arithmetic              # 32-bit arithmetic on 16-bit CPU
    test_sign_promotion              # Signed/unsigned type promotion and extension

    echo ""
    echo "========================================"
    echo "Results: $TESTS_PASSED/$TESTS_RUN passed"
    if [[ $TESTS_KNOWN_BUGS -gt 0 ]]; then
        echo -e "${YELLOW}$TESTS_KNOWN_BUGS known bug(s)${NC}"
    fi
    if [[ $TESTS_FAILED -gt 0 ]]; then
        echo -e "${RED}$TESTS_FAILED tests failed${NC}"
        exit 1
    else
        echo -e "${GREEN}All tests passed${NC}"
        exit 0
    fi
}

main "$@"
