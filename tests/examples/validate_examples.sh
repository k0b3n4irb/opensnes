#!/bin/bash
#
# validate_examples.sh - Automated validation for OpenSNES examples
#
# Checks:
#   1. All examples build successfully
#   2. No WRAM memory overlaps (via symmap.py)
#   3. ROM files exist and have reasonable sizes
#   4. Required symbols are present
#
# Usage:
#   ./validate_examples.sh              # Validate all examples
#   ./validate_examples.sh --quick      # Skip rebuild, just validate
#   ./validate_examples.sh --verbose    # Show detailed output
#
# Exit codes:
#   0 - All validations passed
#   1 - One or more validations failed
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPENSNES_HOME="${OPENSNES_HOME:-$(dirname "$(dirname "$SCRIPT_DIR")")}"
EXAMPLES_DIR="$OPENSNES_HOME/examples"
SYMMAP="$OPENSNES_HOME/tools/symmap/symmap.py"
VRAMCHECK="$OPENSNES_HOME/tools/vramcheck/vramcheck.py"

# Counters
TOTAL=0
PASSED=0
FAILED=0
WARNINGS=0

# Options
QUICK=0
VERBOSE=0
REBUILD=1

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --quick|-q)
            QUICK=1
            REBUILD=0
            shift
            ;;
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --quick, -q     Skip rebuild, just validate existing ROMs"
            echo "  --verbose, -v   Show detailed output"
            echo "  --help, -h      Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    ((WARNINGS++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

log_verbose() {
    if [[ $VERBOSE -eq 1 ]]; then
        echo -e "${CYAN}[DEBUG]${NC} $1"
    fi
}

# Find all example directories with Makefiles
find_examples() {
    find "$EXAMPLES_DIR" -name "Makefile" -type f | while read makefile; do
        dirname "$makefile"
    done
}

# Build a single example
build_example() {
    local dir="$1"
    local name=$(basename "$dir")

    log_verbose "Building $name..."

    if ! make -C "$dir" clean >/dev/null 2>&1; then
        log_verbose "Clean failed for $name (continuing)"
    fi

    if make -C "$dir" 2>&1 | tee /tmp/build_$$.log | grep -q "error\|Error"; then
        log_error "Build failed: $name"
        if [[ $VERBOSE -eq 1 ]]; then
            cat /tmp/build_$$.log
        fi
        rm -f /tmp/build_$$.log
        return 1
    fi

    rm -f /tmp/build_$$.log
    return 0
}

# Check for memory overlaps
check_memory_overlaps() {
    local dir="$1"
    local name=$(basename "$dir")
    local sym_file=$(find "$dir" -name "*.sym" -type f | head -1)

    if [[ -z "$sym_file" || ! -f "$sym_file" ]]; then
        log_warn "$name: No .sym file found (skipping overlap check)"
        return 0
    fi

    log_verbose "Checking memory overlaps for $name..."

    local output
    output=$(python3 "$SYMMAP" --check-overlap "$sym_file" 2>&1)
    local exit_code=$?

    # symmap.py returns exit code 1 when overlap is detected
    # Check for ERROR message (not just "overlap detected" which appears in success msg too)
    if [[ $exit_code -ne 0 ]] || echo "$output" | grep -qi "^ERROR:\|COLLISION:"; then
        log_error "$name: Memory overlap detected!"
        # Show the collision details
        echo "$output" | grep -A2 "COLLISION\|Bank \$00 has\|Bank \$7E range" | head -10
        return 1
    else
        log_verbose "$name: No memory overlaps"
        return 0
    fi
}

# Check ROM file exists and has reasonable size
check_rom_file() {
    local dir="$1"
    local name=$(basename "$dir")
    local rom_file=$(find "$dir" -name "*.sfc" -type f | head -1)

    if [[ -z "$rom_file" || ! -f "$rom_file" ]]; then
        log_error "$name: No .sfc ROM file found"
        return 1
    fi

    local size=$(wc -c < "$rom_file" | tr -d ' ')

    # Check minimum size (should be at least 32KB for a valid SNES ROM)
    if [[ $size -lt 32768 ]]; then
        log_error "$name: ROM too small ($size bytes, expected >= 32KB)"
        return 1
    fi

    # Check for power of 2 size (valid SNES ROM sizes)
    # Valid sizes: 32KB, 64KB, 128KB, 256KB, 512KB, 1MB, 2MB, 4MB
    case $size in
        32768|65536|131072|262144|524288|1048576|2097152|4194304)
            log_verbose "$name: ROM size OK ($size bytes)"
            ;;
        *)
            log_warn "$name: Unusual ROM size ($size bytes)"
            ;;
    esac

    return 0
}

# Check for required symbols in the ROM
check_required_symbols() {
    local dir="$1"
    local name=$(basename "$dir")
    local sym_file=$(find "$dir" -name "*.sym" -type f | head -1)

    if [[ -z "$sym_file" || ! -f "$sym_file" ]]; then
        return 0  # Already warned in overlap check
    fi

    # Required symbols for a valid OpenSNES ROM
    local required_symbols=("RESET" "NMI" "IRQ")
    local missing=0

    for sym in "${required_symbols[@]}"; do
        if ! grep -q "^[0-9a-fA-F]*:$sym$\|:$sym " "$sym_file" 2>/dev/null; then
            # Try case-insensitive search
            if ! grep -qi ":$sym" "$sym_file" 2>/dev/null; then
                log_verbose "$name: Missing required symbol: $sym"
                # This is just informational, not a failure
            fi
        fi
    done

    return 0
}

# Check for VRAM layout issues (overlapping regions, split DMA patterns)
check_vram_layout() {
    local dir="$1"
    local name=$(basename "$dir")

    # Only check if vramcheck tool exists
    if [[ ! -f "$VRAMCHECK" ]]; then
        return 0
    fi

    # Only check directories with C files
    local c_files=$(find "$dir" -maxdepth 1 -name "*.c" -type f | head -1)
    if [[ -z "$c_files" ]]; then
        return 0
    fi

    log_verbose "Checking VRAM layout for $name..."

    local output
    output=$(python3 "$VRAMCHECK" --dir "$dir" --no-color 2>&1)
    local exit_code=$?

    # vramcheck returns 1 on errors (dangerous split DMA with overlapping regions)
    if [[ $exit_code -ne 0 ]]; then
        log_error "$name: VRAM layout issues detected!"
        echo "$output" | grep -E "^DANGEROUS:|^ERRORS" | head -5
        return 1
    else
        # Check for warnings (informational, don't fail)
        if echo "$output" | grep -q "WARNINGS"; then
            log_verbose "$name: VRAM layout has warnings (informational)"
        fi
        return 0
    fi
}

# Validate a single example
validate_example() {
    local dir="$1"
    local name=$(basename "$dir")
    local parent=$(basename "$(dirname "$dir")")
    local full_name="$parent/$name"

    ((TOTAL++))

    echo ""
    log_info "Validating: $full_name"

    local failed=0

    # Build if requested
    if [[ $REBUILD -eq 1 ]]; then
        if ! build_example "$dir"; then
            ((FAILED++))
            return 1
        fi
    fi

    # Check ROM file
    if ! check_rom_file "$dir"; then
        failed=1
    fi

    # Check memory overlaps
    if ! check_memory_overlaps "$dir"; then
        failed=1
    fi

    # Check VRAM layout (only fails on dangerous patterns)
    if ! check_vram_layout "$dir"; then
        failed=1
    fi

    # Check required symbols
    check_required_symbols "$dir"

    if [[ $failed -eq 0 ]]; then
        log_pass "$full_name"
        ((PASSED++))
    else
        ((FAILED++))
    fi

    return $failed
}

# Print summary
print_summary() {
    echo ""
    echo "========================================"
    echo "Example Validation Summary"
    echo "========================================"
    echo -e "Total:    $TOTAL"
    echo -e "Passed:   ${GREEN}$PASSED${NC}"
    echo -e "Failed:   ${RED}$FAILED${NC}"
    echo -e "Warnings: ${YELLOW}$WARNINGS${NC}"
    echo "========================================"

    if [[ $FAILED -gt 0 ]]; then
        echo -e "${RED}VALIDATION FAILED${NC}"
        return 1
    else
        echo -e "${GREEN}ALL VALIDATIONS PASSED${NC}"
        return 0
    fi
}

# Main
main() {
    echo "========================================"
    echo "OpenSNES Example Validator"
    echo "========================================"
    echo "OPENSNES_HOME: $OPENSNES_HOME"
    echo "Examples dir:  $EXAMPLES_DIR"
    echo ""

    # Check prerequisites
    if [[ ! -d "$EXAMPLES_DIR" ]]; then
        log_error "Examples directory not found: $EXAMPLES_DIR"
        exit 1
    fi

    if [[ ! -f "$SYMMAP" ]]; then
        log_error "symmap.py not found: $SYMMAP"
        exit 1
    fi

    # Find and validate all examples
    while IFS= read -r example_dir; do
        validate_example "$example_dir" || true
    done < <(find_examples | sort)

    print_summary
}

main "$@"
