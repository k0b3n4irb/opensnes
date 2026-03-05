#!/bin/bash
#==============================================================================
# Test: Module dependency auto-resolution in common.mk
#
# Verifies that _resolve_deps correctly adds transitive dependencies.
#==============================================================================

# Don't use set -e — arithmetic expressions ((x++)) return 1 when x starts at 0

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

# Write the resolver Makefile once
cat > "$TMPDIR/resolver.mk" << 'ENDMK'
_DEP_sprite    := dma
_DEP_text      := dma
_DEP_text4bpp  := dma
_DEP_object    := map
_DEP_map       := dma
_DEP_snesmod   := console

_resolve_one = $(1) $(foreach m,$(1),$(_DEP_$(m)))
_resolve_deps = $(sort $(call _resolve_one,$(call _resolve_one,$(call _resolve_one,$(1)))))

RESOLVED := $(call _resolve_deps,$(LIB_INPUT))

.PHONY: test
test:
	@echo $(RESOLVED)
ENDMK

check() {
    local desc="$1"
    local input="$2"
    local expected="$3"
    ((TESTS_RUN++))

    local result
    result=$(make -f "$TMPDIR/resolver.mk" --no-print-directory "LIB_INPUT=$input" test 2>/dev/null)

    local norm_expected norm_result
    norm_expected=$(echo "$expected" | xargs)
    norm_result=$(echo "$result" | xargs)

    if [[ "$norm_result" == "$norm_expected" ]]; then
        echo -e "${GREEN}[PASS]${NC} $desc"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}[FAIL]${NC} $desc"
        echo "  Input:    $input"
        echo "  Expected: $norm_expected"
        echo "  Got:      $norm_result"
        ((TESTS_FAILED++))
    fi
}

echo "Module Dependency Resolution Tests"
echo "==================================="

check "sprite adds dma"       "sprite"              "dma sprite"
check "object adds map+dma"   "object"              "dma map object"
check "no duplicates"         "console sprite dma"  "console dma sprite"
check "text adds dma"         "text"                "dma text"
check "snesmod adds console"  "snesmod"             "console snesmod"
check "multi overlap"         "sprite text"         "dma sprite text"
check "console alone"         "console"             "console"
check "lzss no deps"          "lzss"                "lzss"

echo ""
echo "========================================"
echo "Results: $TESTS_PASSED/$TESTS_RUN passed"
if [[ $TESTS_FAILED -gt 0 ]]; then
    echo -e "${RED}$TESTS_FAILED tests failed${NC}"
    exit 1
fi
echo -e "${GREEN}All tests passed${NC}"
