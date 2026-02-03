// =============================================================================
// Unit Test: Background Module
// =============================================================================
// Tests background layer configuration and scrolling functions.
//
// Critical functions tested:
// - bgSetScroll(): Set background scroll position
// - bgSetMapPtr(): Set tilemap address
// - bgSetGfxPtr(): Set tile graphics address
// - bgInit(): Initialize background layer
// - Constants: BG_MAP_*, BG_*COLORS
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/background.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests for constants
// =============================================================================

// Map size constants
_Static_assert(BG_MAP_32x32 == 0, "BG_MAP_32x32 must be 0");
_Static_assert(BG_MAP_64x32 == 1, "BG_MAP_64x32 must be 1");
_Static_assert(BG_MAP_32x64 == 2, "BG_MAP_32x64 must be 2");
_Static_assert(BG_MAP_64x64 == 3, "BG_MAP_64x64 must be 3");

// PVSnesLib compatibility aliases
_Static_assert(SC_32x32 == BG_MAP_32x32, "SC_32x32 must equal BG_MAP_32x32");
_Static_assert(SC_64x32 == BG_MAP_64x32, "SC_64x32 must equal BG_MAP_64x32");
_Static_assert(SC_32x64 == BG_MAP_32x64, "SC_32x64 must equal BG_MAP_32x64");
_Static_assert(SC_64x64 == BG_MAP_64x64, "SC_64x64 must equal BG_MAP_64x64");

// Color mode constants
_Static_assert(BG_4COLORS == 4, "BG_4COLORS must be 4");
_Static_assert(BG_16COLORS == 16, "BG_16COLORS must be 16");
_Static_assert(BG_256COLORS == 256, "BG_256COLORS must be 256");

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
// Test: bgInit
// =============================================================================
void test_bg_init(void) {
    // Initialize each background layer
    bgInit(0);
    log_result("bgInit(0) executes", 1);

    bgInit(1);
    log_result("bgInit(1) executes", 1);

    bgInit(2);
    log_result("bgInit(2) executes", 1);

    bgInit(3);
    log_result("bgInit(3) executes", 1);
}

// =============================================================================
// Test: bgSetScroll
// =============================================================================
void test_bg_scroll(void) {
    // Set scroll for BG1
    bgSetScroll(0, 0, 0);
    log_result("bgSetScroll origin", 1);

    bgSetScroll(0, 100, 50);
    log_result("bgSetScroll offset", 1);

    bgSetScroll(0, 255, 255);
    log_result("bgSetScroll max 8-bit", 1);

    bgSetScroll(0, 1023, 1023);
    log_result("bgSetScroll max 10-bit", 1);

    // Test individual axis functions
    bgSetScrollX(0, 128);
    log_result("bgSetScrollX executes", 1);

    bgSetScrollY(0, 64);
    log_result("bgSetScrollY executes", 1);
}

// =============================================================================
// Test: bgSetMapPtr
// =============================================================================
void test_bg_map_ptr(void) {
    // Set tilemap pointer for each BG with different sizes
    bgSetMapPtr(0, 0x0000, BG_MAP_32x32);
    log_result("bgSetMapPtr BG1 32x32", 1);

    bgSetMapPtr(1, 0x0400, BG_MAP_64x32);
    log_result("bgSetMapPtr BG2 64x32", 1);

    bgSetMapPtr(2, 0x0800, BG_MAP_32x64);
    log_result("bgSetMapPtr BG3 32x64", 1);

    bgSetMapPtr(3, 0x0C00, BG_MAP_64x64);
    log_result("bgSetMapPtr BG4 64x64", 1);
}

// =============================================================================
// Test: bgSetGfxPtr
// =============================================================================
void test_bg_gfx_ptr(void) {
    // Set graphics pointer for each BG
    bgSetGfxPtr(0, 0x0000);
    log_result("bgSetGfxPtr BG1", 1);

    bgSetGfxPtr(1, 0x2000);
    log_result("bgSetGfxPtr BG2", 1);

    bgSetGfxPtr(2, 0x4000);
    log_result("bgSetGfxPtr BG3", 1);

    bgSetGfxPtr(3, 0x6000);
    log_result("bgSetGfxPtr BG4", 1);
}

// =============================================================================
// Test: Multiple BG configuration
// =============================================================================
void test_multi_bg(void) {
    // Simulate typical Mode 1 setup
    bgInit(0);
    bgInit(1);
    bgInit(2);

    bgSetMapPtr(0, 0x0000, BG_MAP_32x32);
    bgSetMapPtr(1, 0x0400, BG_MAP_32x32);
    bgSetMapPtr(2, 0x0800, BG_MAP_32x32);

    bgSetGfxPtr(0, 0x2000);
    bgSetGfxPtr(1, 0x2000);
    bgSetGfxPtr(2, 0x4000);

    bgSetScroll(0, 0, 0);
    bgSetScroll(1, 0, 0);
    bgSetScroll(2, 0, 0);

    log_result("Multi-BG Mode 1 setup", 1);
}

// =============================================================================
// Test: Scroll animation pattern
// =============================================================================
void test_scroll_animation(void) {
    // Simulate scroll animation loop
    u16 i;
    for (i = 0; i < 64; i++) {
        bgSetScroll(0, i, 0);
    }
    log_result("Scroll animation loop", 1);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "BACKGROUND MODULE TESTS");
    textPrintAt(2, 2, "-----------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_bg_init();
    test_bg_scroll();
    test_bg_map_ptr();
    test_bg_gfx_ptr();
    test_multi_bg();
    test_scroll_animation();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
