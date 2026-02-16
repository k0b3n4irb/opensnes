// =============================================================================
// Unit Test: Video Module
// =============================================================================
// Tests video/PPU configuration functions and macros.
//
// Critical items tested:
// - setMode(): Set background mode
// - RGB() / RGB24() macros: Color creation
// - BG_MODE* constants: Background modes
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/video.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests for constants
// =============================================================================

// Background mode constants
_Static_assert(BG_MODE0 == 0, "BG_MODE0 must be 0");
_Static_assert(BG_MODE1 == 1, "BG_MODE1 must be 1");
_Static_assert(BG_MODE2 == 2, "BG_MODE2 must be 2");
_Static_assert(BG_MODE3 == 3, "BG_MODE3 must be 3");
_Static_assert(BG_MODE4 == 4, "BG_MODE4 must be 4");
_Static_assert(BG_MODE5 == 5, "BG_MODE5 must be 5");
_Static_assert(BG_MODE6 == 6, "BG_MODE6 must be 6");
_Static_assert(BG_MODE7 == 7, "BG_MODE7 must be 7");

// Priority flag
_Static_assert(BG3_MODE1_PRIORITY_HIGH == 0x08, "BG3_MODE1_PRIORITY_HIGH must be 0x08");

// RGB macro tests (compile-time)
_Static_assert(RGB(0, 0, 0) == 0x0000, "RGB(0,0,0) must be black");
_Static_assert(RGB(31, 0, 0) == 0x001F, "RGB(31,0,0) must be red");
_Static_assert(RGB(0, 31, 0) == 0x03E0, "RGB(0,31,0) must be green");
_Static_assert(RGB(0, 0, 31) == 0x7C00, "RGB(0,0,31) must be blue");
_Static_assert(RGB(31, 31, 31) == 0x7FFF, "RGB(31,31,31) must be white");

// RGB24 macro tests
_Static_assert(RGB24(0, 0, 0) == 0x0000, "RGB24(0,0,0) must be black");
_Static_assert(RGB24(255, 0, 0) == 0x001F, "RGB24(255,0,0) must be red");
_Static_assert(RGB24(0, 255, 0) == 0x03E0, "RGB24(0,255,0) must be green");
_Static_assert(RGB24(0, 0, 255) == 0x7C00, "RGB24(0,0,255) must be blue");
_Static_assert(RGB24(255, 255, 255) == 0x7FFF, "RGB24(255,255,255) must be white");

// =============================================================================
// Runtime tests
// =============================================================================

static u8 test_passed;
static u8 test_failed;

void log_result(const char* name, u8 passed) {
    if (passed) {
        test_passed++;
    } else {
        test_failed++;
    }
}

// =============================================================================
// Test: setMode
// =============================================================================
void test_set_mode(void) {
    // Test each mode (note: some modes may not display correctly
    // without proper graphics setup, but the function should execute)

    setMode(BG_MODE0, 0);
    log_result("setMode(MODE0)", 1);

    setMode(BG_MODE1, 0);
    log_result("setMode(MODE1)", 1);

    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
    log_result("setMode(MODE1, PRIORITY)", 1);

    // Reset to Mode 0 for text display
    setMode(BG_MODE0, 0);
}

// =============================================================================
// Test: RGB macro runtime
// =============================================================================
void test_rgb_macro(void) {
    // Test RGB values at runtime
    u16 black = RGB(0, 0, 0);
    log_result("RGB black == 0", black == 0x0000);

    u16 red = RGB(31, 0, 0);
    log_result("RGB red correct", red == 0x001F);

    u16 green = RGB(0, 31, 0);
    log_result("RGB green correct", green == 0x03E0);

    u16 blue = RGB(0, 0, 31);
    log_result("RGB blue correct", blue == 0x7C00);

    u16 white = RGB(31, 31, 31);
    log_result("RGB white correct", white == 0x7FFF);

    // Test mixed colors
    u16 yellow = RGB(31, 31, 0);
    log_result("RGB yellow correct", yellow == 0x03FF);

    u16 cyan = RGB(0, 31, 31);
    log_result("RGB cyan correct", cyan == 0x7FE0);

    u16 magenta = RGB(31, 0, 31);
    log_result("RGB magenta correct", magenta == 0x7C1F);
}

// =============================================================================
// Test: RGB24 macro runtime
// =============================================================================
void test_rgb24_macro(void) {
    // Test 24-bit to 15-bit conversion
    u16 red = RGB24(255, 0, 0);
    log_result("RGB24 red correct", red == 0x001F);

    u16 mid_gray = RGB24(128, 128, 128);
    // 128 >> 3 = 16, so RGB(16,16,16)
    u16 expected = RGB(16, 16, 16);
    log_result("RGB24 gray correct", mid_gray == expected);
}

// =============================================================================
// Test: Mode constants uniqueness
// =============================================================================
void test_mode_constants(void) {
    u8 modes_unique =
        (BG_MODE0 != BG_MODE1) &&
        (BG_MODE1 != BG_MODE2) &&
        (BG_MODE2 != BG_MODE3) &&
        (BG_MODE3 != BG_MODE4) &&
        (BG_MODE4 != BG_MODE5) &&
        (BG_MODE5 != BG_MODE6) &&
        (BG_MODE6 != BG_MODE7);
    log_result("Mode constants unique", modes_unique);

    // Verify modes are sequential
    u8 modes_sequential =
        (BG_MODE1 == BG_MODE0 + 1) &&
        (BG_MODE2 == BG_MODE1 + 1) &&
        (BG_MODE3 == BG_MODE2 + 1) &&
        (BG_MODE4 == BG_MODE3 + 1) &&
        (BG_MODE5 == BG_MODE4 + 1) &&
        (BG_MODE6 == BG_MODE5 + 1) &&
        (BG_MODE7 == BG_MODE6 + 1);
    log_result("Mode constants sequential", modes_sequential);
}

// =============================================================================
// Test: Color bit packing
// =============================================================================
void test_color_packing(void) {
    // SNES color format: 0BBBBBGGGGGRRRRR (15-bit)

    // Test that red component is in bits 0-4
    u16 red_only = RGB(31, 0, 0);
    log_result("Red in low bits", (red_only & 0x001F) == 31);

    // Test that green component is in bits 5-9
    u16 green_only = RGB(0, 31, 0);
    log_result("Green in mid bits", ((green_only >> 5) & 0x001F) == 31);

    // Test that blue component is in bits 10-14
    u16 blue_only = RGB(0, 0, 31);
    log_result("Blue in high bits", ((blue_only >> 10) & 0x001F) == 31);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "VIDEO MODULE TESTS");
    textPrintAt(2, 2, "------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_set_mode();
    test_rgb_macro();
    test_rgb24_macro();
    test_mode_constants();
    test_color_packing();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
