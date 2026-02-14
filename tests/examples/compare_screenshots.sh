#!/usr/bin/env bash
#
# compare_screenshots.sh - Visual comparison of PVSnesLib vs OpenSNES ROMs
#
# Takes screenshots from both versions and compares them pixel-by-pixel.
#
# Usage:
#   ./compare_screenshots.sh              # Compare all ported examples
#   ./compare_screenshots.sh mode1        # Compare specific example
#   ./compare_screenshots.sh --verbose    # Show detailed output
#
# Requirements:
#   - Mesen2 emulator (MESEN_PATH environment variable)
#   - Python 3 with PIL/Pillow (for image comparison)
#
# Exit codes:
#   0 - All comparisons match or within tolerance
#   1 - Differences detected or error
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
PVSNESLIB_HOME="${PVSNESLIB_HOME:-/Users/k0b3/workspaces/github/pvsneslib}"

# Mesen path
MESEN_PATH="${MESEN_PATH:-}"
if [[ -z "$MESEN_PATH" ]]; then
    if [[ -x "/Users/k0b3/workspaces/github/Mesen2/bin/osx-arm64/Release/Mesen" ]]; then
        MESEN_PATH="/Users/k0b3/workspaces/github/Mesen2/bin/osx-arm64/Release/Mesen"
    fi
fi

# Lua script for screenshots
LUA_SCRIPT="$SCRIPT_DIR/compare_screenshots.lua"

# Output directory
OUTPUT_DIR="/tmp/opensnes_screenshot_compare"

# Frames to wait before screenshot (2 seconds at 60fps)
FRAMES_TO_WAIT=120

# Options
VERBOSE=0
SPECIFIC_EXAMPLE=""

# Example mappings (parallel arrays for bash 3.x compatibility)
EXAMPLE_NAMES=("mode1" "simple_sprite" "fading")
EXAMPLE_OPENSNES=("examples/graphics/mode1/mode1.sfc" "examples/graphics/simple_sprite/simple_sprite.sfc" "examples/graphics/fading/fading.sfc")
EXAMPLE_PVSNESLIB=("snes-examples/graphics/Backgrounds/Mode1/Mode1.sfc" "snes-examples/graphics/Sprites/SimpleSprite/SimpleSprite.sfc" "snes-examples/graphics/Effects/Fading/Fading.sfc")

# Get example index by name
get_example_index() {
    local name="$1"
    for i in "${!EXAMPLE_NAMES[@]}"; do
        if [[ "${EXAMPLE_NAMES[$i]}" == "$name" ]]; then
            echo "$i"
            return 0
        fi
    done
    return 1
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options] [example_name]"
            echo ""
            echo "Options:"
            echo "  --verbose, -v   Show detailed output"
            echo "  --help, -h      Show this help"
            echo ""
            echo "Examples:"
            echo "  mode1           Mode 1 background"
            echo "  simple_sprite   Simple sprite"
            echo "  fading          Fading effect"
            exit 0
            ;;
        *)
            SPECIFIC_EXAMPLE="$1"
            shift
            ;;
    esac
done

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
        echo -e "${CYAN}[DEBUG]${NC} $1"
    fi
}

# Check prerequisites
check_prerequisites() {
    if [[ -z "$MESEN_PATH" || ! -x "$MESEN_PATH" ]]; then
        log_error "Mesen2 not found. Set MESEN_PATH environment variable."
        exit 1
    fi
    log_verbose "Mesen: $MESEN_PATH"

    if ! command -v python3 &>/dev/null; then
        log_error "Python 3 not found"
        exit 1
    fi

    if [[ ! -f "$LUA_SCRIPT" ]]; then
        log_error "Lua script not found: $LUA_SCRIPT"
        exit 1
    fi

    mkdir -p "$OUTPUT_DIR"
}

# Capture screenshot from a ROM
capture_screenshot() {
    local rom_path="$1"
    local output_path="$2"
    local max_wait="${3:-30}"

    if [[ ! -f "$rom_path" ]]; then
        log_error "ROM not found: $rom_path"
        return 1
    fi

    log_verbose "Capturing: $rom_path -> $output_path"

    # Set environment for Lua script
    export SCREENSHOT_OUTPUT="$output_path"
    export FRAMES_TO_WAIT="$FRAMES_TO_WAIT"

    # Run Mesen in testrunner mode (correct argument order: --testrunner script.lua rom.sfc)
    "$MESEN_PATH" --testrunner "$LUA_SCRIPT" "$rom_path" >/dev/null 2>&1 &
    local mesen_pid=$!

    # Wait for completion with timeout
    local waited=0
    while kill -0 $mesen_pid 2>/dev/null; do
        sleep 1
        waited=$((waited + 1))
        if [[ $waited -ge $max_wait ]]; then
            log_error "Timeout after ${max_wait}s"
            kill -9 $mesen_pid 2>/dev/null || true
            return 1
        fi
    done

    # Check if screenshot was created
    if [[ -f "$output_path" ]]; then
        log_verbose "Screenshot saved: $output_path"
        return 0
    else
        log_error "Screenshot not created"
        return 1
    fi
}

# Compare two images using Python/PIL
compare_images() {
    local img1="$1"
    local img2="$2"
    local diff_output="$3"

    python3 << EOF
import sys
try:
    from PIL import Image, ImageChops
except ImportError:
    print("PIL/Pillow not installed. Install with: pip3 install Pillow")
    sys.exit(2)

try:
    im1 = Image.open("$img1").convert("RGB")
    im2 = Image.open("$img2").convert("RGB")
except Exception as e:
    print(f"Error loading images: {e}")
    sys.exit(1)

# Check dimensions
if im1.size != im2.size:
    print(f"Size mismatch: {im1.size} vs {im2.size}")
    sys.exit(1)

# Calculate difference
diff = ImageChops.difference(im1, im2)

# Get bounding box of differences (None if identical)
bbox = diff.getbbox()

if bbox is None:
    print("IDENTICAL: Images are pixel-perfect match")
    sys.exit(0)
else:
    # Calculate number of different pixels
    diff_pixels = list(diff.getdata())
    num_diff = sum(1 for p in diff_pixels if p != (0, 0, 0))
    total_pixels = im1.size[0] * im1.size[1]
    diff_pct = (num_diff / total_pixels) * 100

    print(f"DIFFERENT: {num_diff}/{total_pixels} pixels ({diff_pct:.2f}%)")
    print(f"  Difference region: {bbox}")

    # Save diff image if path provided
    if "$diff_output":
        # Amplify differences for visibility
        diff_amplified = diff.point(lambda x: min(255, x * 10))
        diff_amplified.save("$diff_output")
        print(f"  Diff image saved: $diff_output")

    # Allow small tolerance (< 1% different pixels)
    if diff_pct < 1.0:
        print("  Within tolerance (< 1%)")
        sys.exit(0)
    else:
        sys.exit(1)
EOF
}

# Compare a single example
compare_example() {
    local name="$1"
    local idx
    idx=$(get_example_index "$name")

    if [[ -z "$idx" ]]; then
        log_error "Unknown example: $name"
        return 1
    fi

    local opensnes_rel="${EXAMPLE_OPENSNES[$idx]}"
    local pvsneslib_rel="${EXAMPLE_PVSNESLIB[$idx]}"

    local opensnes_rom="$OPENSNES_HOME/$opensnes_rel"
    local pvsneslib_rom="$PVSNESLIB_HOME/$pvsneslib_rel"

    local opensnes_screenshot="$OUTPUT_DIR/${name}_opensnes.png"
    local pvsneslib_screenshot="$OUTPUT_DIR/${name}_pvsneslib.png"
    local diff_image="$OUTPUT_DIR/${name}_diff.png"

    echo ""
    log_info "Comparing: $name"
    echo "  OpenSNES: $opensnes_rom"
    echo "  PVSnesLib: $pvsneslib_rom"

    # Check ROMs exist
    if [[ ! -f "$opensnes_rom" ]]; then
        log_error "OpenSNES ROM not found: $opensnes_rom"
        return 1
    fi
    if [[ ! -f "$pvsneslib_rom" ]]; then
        log_error "PVSnesLib ROM not found: $pvsneslib_rom"
        return 1
    fi

    # Capture screenshots
    log_info "Capturing OpenSNES screenshot..."
    if ! capture_screenshot "$opensnes_rom" "$opensnes_screenshot"; then
        return 1
    fi

    log_info "Capturing PVSnesLib screenshot..."
    if ! capture_screenshot "$pvsneslib_rom" "$pvsneslib_screenshot"; then
        return 1
    fi

    # Compare
    log_info "Comparing images..."
    local result
    result=$(compare_images "$opensnes_screenshot" "$pvsneslib_screenshot" "$diff_image" 2>&1)
    local exit_code=$?

    echo "  $result"

    if [[ $exit_code -eq 0 ]]; then
        echo -e "  ${GREEN}PASS${NC}"
        return 0
    elif [[ $exit_code -eq 2 ]]; then
        log_warn "PIL not installed, skipping comparison"
        return 0
    else
        echo -e "  ${RED}FAIL${NC}"
        echo "  Screenshots saved to: $OUTPUT_DIR"
        return 1
    fi
}

# Print summary
print_summary() {
    local passed=$1
    local failed=$2
    local total=$((passed + failed))

    echo ""
    echo "========================================"
    echo "Screenshot Comparison Summary"
    echo "========================================"
    echo "Total:  $total"
    echo -e "Passed: ${GREEN}$passed${NC}"
    echo -e "Failed: ${RED}$failed${NC}"
    echo "Output: $OUTPUT_DIR"
    echo "========================================"

    if [[ $failed -gt 0 ]]; then
        return 1
    fi
    return 0
}

# Main
main() {
    echo "========================================"
    echo "OpenSNES vs PVSnesLib Screenshot Compare"
    echo "========================================"

    check_prerequisites

    local passed=0
    local failed=0

    if [[ -n "$SPECIFIC_EXAMPLE" ]]; then
        # Compare specific example
        if compare_example "$SPECIFIC_EXAMPLE"; then
            passed=$((passed + 1))
        else
            failed=$((failed + 1))
        fi
    else
        # Compare all examples
        for name in "${EXAMPLE_NAMES[@]}"; do
            if compare_example "$name"; then
                passed=$((passed + 1))
            else
                failed=$((failed + 1))
            fi
        done
    fi

    print_summary $passed $failed
}

main "$@"
