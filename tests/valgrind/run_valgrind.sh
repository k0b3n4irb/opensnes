#!/bin/bash
#==============================================================================
# OpenSNES Valgrind Memcheck Runner
#
# Runs Valgrind memcheck on all host-side binaries:
#   - Compiler: cproc-qbe and qbe (using compiler test files + library sources)
#   - Tools: gfx4snes, font2snes, smconv, img2snes
#
# Usage:
#   ./run_valgrind.sh                  # Full suite
#   ./run_valgrind.sh --compiler-only  # Compiler tests only
#   ./run_valgrind.sh --tools-only     # Tool tests only
#   ./run_valgrind.sh --no-leaks       # Errors only (faster)
#   ./run_valgrind.sh --verbose        # Show full Valgrind output
#
# Exit code:
#   0 = no memory errors (leaks are warnings, not failures)
#   1 = memory errors detected (use-after-free, uninitialized reads, etc.)
#==============================================================================

set -o pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPENSNES="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$OPENSNES/bin"
SUPP_FILE="$SCRIPT_DIR/cproc.supp"
WORK_DIR=$(mktemp -d -t opensnes_valgrind.XXXXXX)
trap 'rm -rf "$WORK_DIR"' EXIT

# Counters
TOTAL_CHECKS=0
TOTAL_ERRORS=0
TOTAL_LEAKS=0
TOTAL_SKIPPED=0

# Options
RUN_COMPILER=1
RUN_TOOLS=1
CHECK_LEAKS=1
VERBOSE=0

#------------------------------------------------------------------------------
# Argument parsing
#------------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case $1 in
        --compiler-only) RUN_TOOLS=0; shift ;;
        --tools-only)    RUN_COMPILER=0; shift ;;
        --no-leaks)      CHECK_LEAKS=0; shift ;;
        --verbose|-v)    VERBOSE=1; shift ;;
        -h|--help)
            echo "Usage: $(basename "$0") [--compiler-only] [--tools-only] [--no-leaks] [--verbose]"
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

# Build valgrind flags
# Leaks are never errors (exit code 42). Only real memory bugs
# (use-after-free, uninit reads, buffer overflows) cause failure.
# Leaks are reported as warnings by the script's own parsing.
# cproc/qbe use an "allocate everything, let OS reclaim" pattern —
# definite leaks from AST nodes/tokens are expected and harmless.
VALGRIND_FLAGS="--error-exitcode=42 --tool=memcheck --track-origins=yes -s"

if [[ $CHECK_LEAKS -eq 1 ]]; then
    VALGRIND_FLAGS="$VALGRIND_FLAGS --leak-check=full --errors-for-leak-kinds=none"
else
    VALGRIND_FLAGS="$VALGRIND_FLAGS --leak-check=no"
fi

# Add suppressions file for known upstream bugs
if [[ -f "$SUPP_FILE" ]]; then
    VALGRIND_FLAGS="$VALGRIND_FLAGS --suppressions=$SUPP_FILE"
fi

#------------------------------------------------------------------------------
# run_check: Run a command under Valgrind and report results
#
# Args: $1=label (e.g. "[cproc] test_minimal")
#       $2=log_tag (unique filename tag)
#       $3...=command and arguments
#
# stdout of the checked command goes to /dev/null unless --verbose.
# Valgrind output goes to a log file.
#
# Prints one result line. Updates global counters.
#------------------------------------------------------------------------------
run_check() {
    local label="$1"
    local log_tag="$2"
    shift 2

    local log_file="$WORK_DIR/${log_tag}.valgrind"
    local stdout_file="$WORK_DIR/${log_tag}.stdout"

    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))

    # Run command under Valgrind, capturing exit code
    local rc=0
    valgrind $VALGRIND_FLAGS --log-file="$log_file" "$@" > "$stdout_file" 2>/dev/null || rc=$?

    # Parse Valgrind log
    local has_errors=0
    local has_leaks=0
    local leak_info=""

    if [[ $rc -eq 42 ]]; then
        has_errors=1
    fi

    # Double-check error summary line
    if [[ -f "$log_file" ]]; then
        local err_line
        err_line=$(grep "ERROR SUMMARY:" "$log_file" 2>/dev/null | tail -1)
        if [[ -n "$err_line" ]] && ! echo "$err_line" | grep -q "ERROR SUMMARY: 0 errors"; then
            has_errors=1
        fi

        # Check for leaks
        if [[ $CHECK_LEAKS -eq 1 ]]; then
            local def_lost
            def_lost=$(grep "definitely lost:" "$log_file" 2>/dev/null | tail -1 || echo "")
            if [[ -n "$def_lost" ]] && ! echo "$def_lost" | grep -q "0 bytes in 0 blocks"; then
                has_leaks=1
                leak_info=$(echo "$def_lost" | sed 's/.*definitely lost: //' | sed 's/ *$//')
            fi

            local ind_lost
            ind_lost=$(grep "indirectly lost:" "$log_file" 2>/dev/null | tail -1 || echo "")
            if [[ -n "$ind_lost" ]] && ! echo "$ind_lost" | grep -q "0 bytes in 0 blocks"; then
                has_leaks=1
            fi
        fi
    fi

    # Format and print result
    local padding
    padding=$(printf '%-55s' "$label")

    if [[ $has_errors -gt 0 ]]; then
        echo -e "${padding} ${RED}ERROR${NC}"
        TOTAL_ERRORS=$((TOTAL_ERRORS + 1))
        if [[ $VERBOSE -eq 1 ]] && [[ -f "$log_file" ]]; then
            cat "$log_file"
        fi
    elif [[ $has_leaks -gt 0 ]]; then
        echo -e "${padding} ${YELLOW}LEAK${NC} ($leak_info)"
        TOTAL_LEAKS=$((TOTAL_LEAKS + 1))
        if [[ $VERBOSE -eq 1 ]] && [[ -f "$log_file" ]]; then
            cat "$log_file"
        fi
    else
        echo -e "${padding} ${GREEN}OK${NC}"
    fi
}

#------------------------------------------------------------------------------
# Pre-flight checks
#------------------------------------------------------------------------------

if ! command -v valgrind &> /dev/null; then
    echo -e "${RED}Error: valgrind not found. Install it first.${NC}" >&2
    exit 1
fi

echo "========================================"
echo "  OpenSNES Valgrind Memcheck"
echo "========================================"
echo "Valgrind: $(valgrind --version)"
echo "SDK root: $OPENSNES"
if [[ $CHECK_LEAKS -eq 0 ]]; then
    echo "Mode:     errors only (--no-leaks)"
fi
echo ""

#------------------------------------------------------------------------------
# Section A: Compiler (cproc-qbe + qbe)
#------------------------------------------------------------------------------

if [[ $RUN_COMPILER -eq 1 ]]; then
    echo -e "${CYAN}=== Compiler ===${NC}"
    echo ""

    if [[ ! -x "$BIN/cproc-qbe" ]]; then
        echo -e "${RED}cproc-qbe not found at $BIN/cproc-qbe — skipping${NC}"
    elif [[ ! -x "$BIN/qbe" ]]; then
        echo -e "${RED}qbe not found at $BIN/qbe — skipping${NC}"
    else
        # Collect test inputs
        COMPILER_INPUTS=()

        for f in "$OPENSNES"/tests/compiler/test_*.c; do
            [[ -f "$f" ]] && COMPILER_INPUTS+=("$f")
        done

        for f in "$OPENSNES"/lib/source/*.c; do
            [[ -f "$f" ]] && COMPILER_INPUTS+=("$f")
        done

        for src in "${COMPILER_INPUTS[@]}"; do
            base=$(basename "$src" .c)
            pp_file="$WORK_DIR/${base}.i"
            ir_file="$WORK_DIR/${base}.qbe"
            asm_file="$WORK_DIR/${base}.asm"

            # Preprocess (host cc, no Valgrind needed)
            if ! cc -E -I "$OPENSNES/lib/include" "$src" > "$pp_file" 2>/dev/null; then
                printf '%-55s %s\n' "[cproc] $base" "SKIP (preprocess failed)"
                TOTAL_SKIPPED=$((TOTAL_SKIPPED + 1))
                continue
            fi

            # cproc-qbe under Valgrind
            # stdout = QBE IR (captured for qbe stage)
            cproc_rc=0
            valgrind $VALGRIND_FLAGS --log-file="$WORK_DIR/cproc_${base}.valgrind" \
                "$BIN/cproc-qbe" "$pp_file" > "$ir_file" 2>/dev/null || cproc_rc=$?

            # Parse cproc results
            cproc_errors=0
            cproc_leaks=0
            cproc_leak_info=""

            if [[ $cproc_rc -eq 42 ]]; then
                cproc_errors=1
            fi

            if [[ -f "$WORK_DIR/cproc_${base}.valgrind" ]]; then
                err_line=$(grep "ERROR SUMMARY:" "$WORK_DIR/cproc_${base}.valgrind" 2>/dev/null | tail -1)
                if [[ -n "$err_line" ]] && ! echo "$err_line" | grep -q "ERROR SUMMARY: 0 errors"; then
                    cproc_errors=1
                fi

                if [[ $CHECK_LEAKS -eq 1 ]]; then
                    def_lost=$(grep "definitely lost:" "$WORK_DIR/cproc_${base}.valgrind" 2>/dev/null | tail -1 || echo "")
                    if [[ -n "$def_lost" ]] && ! echo "$def_lost" | grep -q "0 bytes in 0 blocks"; then
                        cproc_leaks=1
                        cproc_leak_info=$(echo "$def_lost" | sed 's/.*definitely lost: //' | sed 's/ *$//')
                    fi
                fi
            fi

            TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
            padding=$(printf '%-55s' "[cproc] $base")

            if [[ $cproc_errors -gt 0 ]]; then
                echo -e "${padding} ${RED}ERROR${NC}"
                TOTAL_ERRORS=$((TOTAL_ERRORS + 1))
                if [[ $VERBOSE -eq 1 ]]; then
                    cat "$WORK_DIR/cproc_${base}.valgrind"
                fi
            elif [[ $cproc_leaks -gt 0 ]]; then
                echo -e "${padding} ${YELLOW}LEAK${NC} ($cproc_leak_info)"
                TOTAL_LEAKS=$((TOTAL_LEAKS + 1))
                if [[ $VERBOSE -eq 1 ]]; then
                    cat "$WORK_DIR/cproc_${base}.valgrind"
                fi
            else
                echo -e "${padding} ${GREEN}OK${NC}"
            fi

            # qbe under Valgrind (only if cproc produced IR)
            if [[ ! -s "$ir_file" ]]; then
                printf '%-55s %s\n' "[qbe]   $base" "SKIP (no IR)"
                TOTAL_SKIPPED=$((TOTAL_SKIPPED + 1))
                continue
            fi

            run_check "[qbe]   $base" "qbe_${base}" "$BIN/qbe" -t w65816 "$ir_file"
        done
    fi

    echo ""
fi

#------------------------------------------------------------------------------
# Section B: Tools (gfx4snes, font2snes, smconv, img2snes)
#------------------------------------------------------------------------------

if [[ $RUN_TOOLS -eq 1 ]]; then
    echo -e "${CYAN}=== Tools ===${NC}"
    echo ""

    #--- gfx4snes ---
    if [[ -x "$BIN/gfx4snes" ]]; then
        # 4bpp BG with tilemap (Mode 1)
        mode1_png="$OPENSNES/examples/graphics/backgrounds/mode1/res/opensnes.png"
        if [[ -f "$mode1_png" ]]; then
            cp "$mode1_png" "$WORK_DIR/"
            run_check "[gfx4snes] 4bpp_bg_mode1" "gfx4snes_4bpp" \
                "$BIN/gfx4snes" -s 8 -o 16 -u 16 -p -m -i "$WORK_DIR/opensnes.png"
        fi

        # 32x32 sprite
        sprite_png="$OPENSNES/examples/graphics/sprites/simple_sprite/res/sprite32.png"
        if [[ -f "$sprite_png" ]]; then
            cp "$sprite_png" "$WORK_DIR/"
            run_check "[gfx4snes] sprite_32x32" "gfx4snes_sprite32" \
                "$BIN/gfx4snes" -s 32 -o 16 -u 16 -p -i "$WORK_DIR/sprite32.png"
        fi

        # Mode 7
        mode7_png="$OPENSNES/examples/graphics/backgrounds/mode7/res/mode7bg.png"
        if [[ -f "$mode7_png" ]]; then
            cp "$mode7_png" "$WORK_DIR/"
            run_check "[gfx4snes] mode7_bg" "gfx4snes_mode7" \
                "$BIN/gfx4snes" -p -m -M 7 -i "$WORK_DIR/mode7bg.png"
        fi
    else
        printf '%-55s %s\n' "[gfx4snes]" "SKIP (not found)"
        TOTAL_SKIPPED=$((TOTAL_SKIPPED + 1))
    fi

    #--- font2snes ---
    if [[ -x "$BIN/font2snes" ]]; then
        font_tested=0
        for font_candidate in \
            "$OPENSNES"/examples/text/*/res/*.png \
            "$OPENSNES"/projects/*/res/font*.png; do
            if [[ -f "$font_candidate" ]]; then
                cp "$font_candidate" "$WORK_DIR/"
                local_name=$(basename "$font_candidate")
                run_check "[font2snes] $local_name" "font2snes_${local_name%.png}" \
                    "$BIN/font2snes" "$WORK_DIR/$local_name" "$WORK_DIR/font_out"
                font_tested=1
                break
            fi
        done
        if [[ $font_tested -eq 0 ]]; then
            printf '%-55s %s\n' "[font2snes]" "SKIP (no font PNG found)"
            TOTAL_SKIPPED=$((TOTAL_SKIPPED + 1))
        fi
    else
        printf '%-55s %s\n' "[font2snes]" "SKIP (not found)"
        TOTAL_SKIPPED=$((TOTAL_SKIPPED + 1))
    fi

    #--- smconv ---
    if [[ -x "$BIN/smconv" ]]; then
        smconv_tested=0
        for it_file in "$OPENSNES"/examples/audio/snesmod_music/music/*.it; do
            if [[ -f "$it_file" ]]; then
                it_name=$(basename "$it_file" .it)
                run_check "[smconv]   $it_name" "smconv_${it_name}" \
                    "$BIN/smconv" -s -o "$WORK_DIR/soundbank" -b 5 -n -p soundbank "$it_file"
                smconv_tested=1
            fi
        done
        if [[ $smconv_tested -eq 0 ]]; then
            printf '%-55s %s\n' "[smconv]" "SKIP (no .it files found)"
            TOTAL_SKIPPED=$((TOTAL_SKIPPED + 1))
        fi
    else
        printf '%-55s %s\n' "[smconv]" "SKIP (not found)"
        TOTAL_SKIPPED=$((TOTAL_SKIPPED + 1))
    fi

    #--- img2snes ---
    if [[ -x "$BIN/img2snes" ]]; then
        img2snes_tested=0
        for test_png in \
            "$OPENSNES/examples/graphics/backgrounds/mode1/res/opensnes.png" \
            "$OPENSNES/examples/graphics/sprites/simple_sprite/res/sprite32.png"; do
            if [[ -f "$test_png" ]]; then
                png_name=$(basename "$test_png" .png)
                run_check "[img2snes] $png_name" "img2snes_${png_name}" \
                    "$BIN/img2snes" -i "$test_png" -o "$WORK_DIR/img2snes_out.png" -c 16
                img2snes_tested=1
                break
            fi
        done
        if [[ $img2snes_tested -eq 0 ]]; then
            printf '%-55s %s\n' "[img2snes]" "SKIP (no test PNG found)"
            TOTAL_SKIPPED=$((TOTAL_SKIPPED + 1))
        fi
    else
        printf '%-55s %s\n' "[img2snes]" "SKIP (not found)"
        TOTAL_SKIPPED=$((TOTAL_SKIPPED + 1))
    fi

    echo ""
fi

#------------------------------------------------------------------------------
# Summary
#------------------------------------------------------------------------------

echo "========================================"
echo "  Summary"
echo "========================================"
echo -e "Total checks: ${CYAN}$TOTAL_CHECKS${NC}"

if [[ $TOTAL_ERRORS -gt 0 ]]; then
    echo -e "Errors:       ${RED}$TOTAL_ERRORS${NC}"
else
    echo -e "Errors:       ${GREEN}0${NC}"
fi

if [[ $TOTAL_LEAKS -gt 0 ]]; then
    echo -e "Leaks:        ${YELLOW}$TOTAL_LEAKS${NC} (warnings)"
else
    echo -e "Leaks:        ${GREEN}0${NC}"
fi

if [[ $TOTAL_SKIPPED -gt 0 ]]; then
    echo -e "Skipped:      $TOTAL_SKIPPED"
fi

echo "========================================"

# Exit non-zero only for real memory errors, not leaks
if [[ $TOTAL_ERRORS -gt 0 ]]; then
    echo -e "${RED}FAIL: Memory errors detected${NC}"
    exit 1
fi

if [[ $TOTAL_LEAKS -gt 0 ]]; then
    echo -e "${YELLOW}WARN: Memory leaks detected (no hard errors)${NC}"
else
    echo -e "${GREEN}PASS: No memory issues detected${NC}"
fi
exit 0
