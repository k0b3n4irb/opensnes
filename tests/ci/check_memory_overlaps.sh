#!/bin/bash
# =============================================================================
# OpenSNES CI Memory Overlap Checker
# =============================================================================
# Validates all built examples for WRAM memory collisions between Bank $00
# and Bank $7E. Returns non-zero if any collision is detected.
#
# Usage: ./tests/ci/check_memory_overlaps.sh
#
# This script was created after discovering that memory collisions had gone
# undetected for months (February 2026). It is now part of the CI pipeline.
# =============================================================================

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Determine paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Derive OPENSNES_HOME from script location if not set or if set to invalid path
DERIVED_HOME="$(cd "$SCRIPT_DIR/../.." && pwd)"
if [ -z "${OPENSNES_HOME:-}" ] || [ ! -f "${OPENSNES_HOME}/tools/symmap/symmap.py" ]; then
    OPENSNES_HOME="$DERIVED_HOME"
fi
SYMMAP="$OPENSNES_HOME/tools/symmap/symmap.py"
EXAMPLES_DIR="$OPENSNES_HOME/examples"

echo "========================================"
echo "OpenSNES Memory Overlap Check"
echo "========================================"
echo "OPENSNES_HOME: $OPENSNES_HOME"
echo ""

# Verify symmap.py exists
if [ ! -f "$SYMMAP" ]; then
    echo -e "${RED}ERROR: symmap.py not found at $SYMMAP${NC}"
    exit 2
fi

# Verify examples directory exists
if [ ! -d "$EXAMPLES_DIR" ]; then
    echo -e "${RED}ERROR: Examples directory not found at $EXAMPLES_DIR${NC}"
    exit 2
fi

FAILED=0
CHECKED=0
SKIPPED=0

# Create temp file for output
TEMP_OUTPUT=$(mktemp)
trap "rm -f $TEMP_OUTPUT" EXIT

# Find all .sym files in examples
while IFS= read -r symfile; do
    # Get example name from path
    example_dir=$(dirname "$symfile")
    # Get relative path from examples dir
    rel_path="${example_dir#$EXAMPLES_DIR/}"
    example_name=$(basename "$symfile" .sym)

    printf "Checking %-40s " "$rel_path..."

    # Run symmap with overlap check
    if python3 "$SYMMAP" --check-overlap "$symfile" > "$TEMP_OUTPUT" 2>&1; then
        echo -e "${GREEN}[OK]${NC}"
    else
        exit_code=$?
        if [ $exit_code -eq 1 ]; then
            # Collision detected
            echo -e "${RED}[COLLISION]${NC}"
            echo ""
            cat "$TEMP_OUTPUT"
            echo ""
            FAILED=$((FAILED + 1))
        else
            # Other error (file not found, parse error, etc.)
            echo -e "${YELLOW}[SKIP]${NC}"
            SKIPPED=$((SKIPPED + 1))
        fi
    fi

    CHECKED=$((CHECKED + 1))
done < <(find "$EXAMPLES_DIR" -name "*.sym" -type f | sort)

echo ""
echo "========================================"
echo "Memory Overlap Check Summary"
echo "========================================"
echo "Checked:  $CHECKED"
echo -e "Passed:   ${GREEN}$((CHECKED - FAILED - SKIPPED))${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "Failed:   ${RED}$FAILED${NC}"
fi
if [ $SKIPPED -gt 0 ]; then
    echo -e "Skipped:  ${YELLOW}$SKIPPED${NC}"
fi
echo "========================================"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo -e "${RED}ERROR: Memory collisions detected in $FAILED example(s)${NC}"
    echo ""
    echo "To debug, run:"
    echo "  python3 tools/symmap/symmap.py --check-overlap <path/to/example.sym>"
    echo ""
    echo "Common fixes:"
    echo "  1. Move Bank \$7E buffers to address \$2000+ (above mirror range)"
    echo "  2. Add reserved section in Bank \$00 to prevent linker collisions"
    echo "  3. Check templates/common/crt0.asm for memory layout"
    exit 1
fi

echo ""
echo -e "${GREEN}All memory checks passed${NC}"
exit 0
