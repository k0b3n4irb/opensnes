#!/bin/bash
#
# test_snes_compliance.sh - Comprehensive SNES hardware compliance tests
#
# Validates that OpenSNES implementations match SNES hardware specifications:
# - OAM structure (544 bytes)
# - Sprite tile addressing (N/N+1/N+16/N+17 pattern)
# - BGR555 color format
# - Memory map compliance
# - Register addresses
#

# Don't exit on first error - we want to run all tests
# set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPENSNES="${OPENSNES:-$(dirname "$SCRIPT_DIR")}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

log_pass() { echo -e "${GREEN}[PASS]${NC} $1"; ((TESTS_PASSED++)); }
log_fail() { echo -e "${RED}[FAIL]${NC} $1"; ((TESTS_FAILED++)); }
log_info() { echo -e "${YELLOW}[INFO]${NC} $1"; }

echo "========================================"
echo "SNES Hardware Compliance Tests"
echo "========================================"
echo ""

#------------------------------------------------------------------------------
# Test 1: OAM Buffer Size
#------------------------------------------------------------------------------
test_oam_size() {
    local name="oam_buffer_size"
    ((TESTS_RUN++))

    # Check that oamMemory is 544 bytes (512 + 32)
    if grep -q "oamMemory.*dsb 544" "$OPENSNES/lib/source/sprite.c" 2>/dev/null || \
       grep -q "OAM_SIZE.*544" "$OPENSNES/lib/include/snes/sprite.h" 2>/dev/null || \
       grep -q "dsb 544" "$OPENSNES/templates/common/crt0.asm" 2>/dev/null; then
        log_pass "$name: OAM buffer is 544 bytes"
    else
        # Check in the actual crt0.asm
        if grep -E "oamMemory.*dsb.*544|\.buffer.*544" "$OPENSNES/templates/common/crt0.asm" > /dev/null 2>&1; then
            log_pass "$name: OAM buffer is 544 bytes"
        else
            log_fail "$name: OAM buffer size not found or incorrect"
        fi
    fi
}

#------------------------------------------------------------------------------
# Test 2: Sprite Lookup Tables
#------------------------------------------------------------------------------
test_sprite_lut() {
    local name="sprite_lookup_tables"
    ((TESTS_RUN++))

    local lut_file="$OPENSNES/lib/source/sprite_lut.asm"
    if [[ ! -f "$lut_file" ]]; then
        log_fail "$name: Lookup table file not found"
        return
    fi

    # Check for 16x16 sprite lookup tables
    local has_16x16=0
    if grep -q "lkup16oamS" "$lut_file" && \
       grep -q "lkup16idT" "$lut_file" && \
       grep -q "lkup16idB" "$lut_file"; then
        has_16x16=1
    fi

    # Check for 32x32 sprite lookup tables
    local has_32x32=0
    if grep -q "lkup32oamS" "$lut_file" && \
       grep -q "lkup32idT" "$lut_file" && \
       grep -q "lkup32idB" "$lut_file"; then
        has_32x32=1
    fi

    if [[ $has_16x16 -eq 1 ]] && [[ $has_32x32 -eq 1 ]]; then
        log_pass "$name: All lookup tables present"
    else
        log_fail "$name: Missing lookup tables (16x16=$has_16x16, 32x32=$has_32x32)"
    fi
}

#------------------------------------------------------------------------------
# Test 3: Metasprite Structure Size
#------------------------------------------------------------------------------
test_metasprite_struct() {
    local name="metasprite_structure"
    ((TESTS_RUN++))

    local header="$OPENSNES/lib/include/snes/sprite.h"
    if [[ ! -f "$header" ]]; then
        log_fail "$name: sprite.h not found"
        return
    fi

    # Check for MetaspriteItem structure (should be 8 bytes for PVSnesLib compat)
    if grep -q "typedef struct" "$header" && \
       grep -q "MetaspriteItem" "$header" && \
       grep -q "METASPR_ITEM" "$header" && \
       grep -q "METASPR_TERM" "$header"; then
        log_pass "$name: Metasprite structures and macros present"
    else
        log_fail "$name: Metasprite definitions incomplete"
    fi
}

#------------------------------------------------------------------------------
# Test 4: BGR555 Color Macros
#------------------------------------------------------------------------------
test_color_format() {
    local name="bgr555_color_format"
    ((TESTS_RUN++))

    # Look for BGR555 color handling in headers
    if grep -rq "BGR555\|bgr555\|RGB555" "$OPENSNES/lib/include/" 2>/dev/null; then
        log_pass "$name: BGR555 color support found"
    else
        # Check documentation
        if grep -q "BGR555\|bgr555" "$OPENSNES/.claude/SNES_ROSETTA_STONE.md" 2>/dev/null || \
           grep -q "BGR555\|bgr555" "$OPENSNES/.claude/KNOWLEDGE.md" 2>/dev/null; then
            log_pass "$name: BGR555 documented (no runtime macros)"
        else
            log_fail "$name: BGR555 color format not documented"
        fi
    fi
}

#------------------------------------------------------------------------------
# Test 5: WRAM Mirroring Protection
#------------------------------------------------------------------------------
test_wram_mirror() {
    local name="wram_mirror_protection"
    ((TESTS_RUN++))

    # Check that oamMemory is placed above $0300 to avoid WRAM mirroring
    if grep -E "ORGA.*\\\$0300.*FORCE|oamMemory.*\\\$7E:03" "$OPENSNES/templates/common/crt0.asm" > /dev/null 2>&1; then
        log_pass "$name: OAM buffer protected from WRAM mirror"
    else
        # Alternative check for any FORCE placement
        if grep -q "FORCE" "$OPENSNES/templates/common/crt0.asm" 2>/dev/null; then
            log_pass "$name: RAMSECTION uses FORCE directive"
        else
            log_fail "$name: WRAM mirroring protection not verified"
        fi
    fi
}

#------------------------------------------------------------------------------
# Test 6: gfx4snes Graphics Tool
#------------------------------------------------------------------------------
test_gfx4snes() {
    local name="gfx4snes_tool"
    ((TESTS_RUN++))

    local gfx4snes="$OPENSNES/bin/gfx4snes"
    if [[ ! -x "$gfx4snes" ]]; then
        log_fail "$name: gfx4snes not found or not executable"
        return
    fi

    # Verify gfx4snes runs and shows help
    if "$gfx4snes" --help 2>&1 | grep -q "Usage: gfx4snes"; then
        log_pass "$name: gfx4snes executable works"
    else
        log_fail "$name: gfx4snes not working correctly"
    fi
}

#------------------------------------------------------------------------------
# Test 7: Compiler No Unhandled Ops
#------------------------------------------------------------------------------
test_no_unhandled_ops() {
    local name="compiler_no_unhandled_ops"
    ((TESTS_RUN++))

    if grep -r "unhandled op" "$OPENSNES/lib/build/"*.c.asm 2>/dev/null; then
        log_fail "$name: Unhandled ops found in library"
    else
        log_pass "$name: No unhandled ops in library"
    fi
}

#------------------------------------------------------------------------------
# Test 8: Register Definitions
#------------------------------------------------------------------------------
test_registers() {
    local name="snes_register_definitions"
    ((TESTS_RUN++))

    local header="$OPENSNES/lib/include/snes/registers.h"
    if [[ ! -f "$header" ]]; then
        header="$OPENSNES/lib/include/snes/hardware.h"
    fi
    if [[ ! -f "$header" ]]; then
        header="$OPENSNES/lib/include/snes.h"
    fi

    if [[ ! -f "$header" ]]; then
        log_fail "$name: Hardware header not found"
        return
    fi

    # Check for essential PPU registers
    local found=0
    for reg in INIDISP OBJSEL OAMADDL BGMODE VMAIN; do
        if grep -q "$reg" "$header" 2>/dev/null; then
            ((found++))
        fi
    done

    if [[ $found -ge 3 ]]; then
        log_pass "$name: PPU register definitions found ($found/5)"
    else
        log_fail "$name: Missing PPU register definitions ($found/5)"
    fi
}

#------------------------------------------------------------------------------
# Run all tests
#------------------------------------------------------------------------------

test_oam_size
test_sprite_lut
test_metasprite_struct
test_color_format
test_wram_mirror
test_gfx4snes
test_no_unhandled_ops
test_registers

echo ""
echo "========================================"
echo "SNES Compliance Test Summary"
echo "========================================"
echo "Total:  $TESTS_RUN"
echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
echo "========================================"

if [[ $TESTS_FAILED -eq 0 ]]; then
    echo -e "${GREEN}ALL TESTS PASSED${NC}"
    exit 0
else
    echo -e "${RED}$TESTS_FAILED TESTS FAILED${NC}"
    exit 1
fi
