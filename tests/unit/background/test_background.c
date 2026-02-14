// =============================================================================
// Unit Test: Background Module
// =============================================================================
// Tests background layer configuration and scrolling functions.
//
// Critical functions tested:
// - bgSetScroll() / bgSetScrollX() / bgSetScrollY(): Set scroll position
// - bgGetScrollX() / bgGetScrollY(): Read scroll shadow
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
// Test framework
// =============================================================================

static u8 tests_passed;
static u8 tests_failed;
static u8 test_line;

#define TEST(name, condition) do { \
    if (condition) { \
        tests_passed++; \
    } else { \
        tests_failed++; \
        textPrintAt(1, test_line, "FAIL:"); \
        textPrintAt(7, test_line, name); \
        test_line++; \
    } \
} while(0)

// =============================================================================
// Test: bgSetScroll + bgGetScrollX/Y
// =============================================================================
void test_bg_scroll(void) {
    // Origin
    bgSetScroll(0, 0, 0);
    TEST("setScroll X=0", bgGetScrollX(0) == 0);
    TEST("setScroll Y=0", bgGetScrollY(0) == 0);

    // Typical offset
    bgSetScroll(0, 100, 50);
    TEST("setScroll X=100", bgGetScrollX(0) == 100);
    TEST("setScroll Y=50", bgGetScrollY(0) == 50);

    // Max 8-bit values
    bgSetScroll(0, 255, 255);
    TEST("setScroll X=255", bgGetScrollX(0) == 255);
    TEST("setScroll Y=255", bgGetScrollY(0) == 255);

    // Max 10-bit values
    bgSetScroll(0, 1023, 1023);
    TEST("setScroll X=1023", bgGetScrollX(0) == 1023);
    TEST("setScroll Y=1023", bgGetScrollY(0) == 1023);
}

// =============================================================================
// Test: bgSetScrollX / bgSetScrollY (individual axis)
// =============================================================================
void test_bg_scroll_axis(void) {
    // Set both first
    bgSetScroll(0, 100, 200);

    // Change X only — Y must be preserved
    bgSetScrollX(0, 300);
    TEST("scrollX sets X", bgGetScrollX(0) == 300);
    TEST("scrollX keeps Y", bgGetScrollY(0) == 200);

    // Change Y only — X must be preserved
    bgSetScrollY(0, 400);
    TEST("scrollY sets Y", bgGetScrollY(0) == 400);
    TEST("scrollY keeps X", bgGetScrollX(0) == 300);
}

// =============================================================================
// Test: Multiple BGs are independent
// =============================================================================
void test_bg_scroll_independence(void) {
    bgSetScroll(0, 10, 20);
    bgSetScroll(1, 30, 40);
    bgSetScroll(2, 50, 60);
    bgSetScroll(3, 70, 80);

    TEST("BG0 X indep", bgGetScrollX(0) == 10);
    TEST("BG0 Y indep", bgGetScrollY(0) == 20);
    TEST("BG1 X indep", bgGetScrollX(1) == 30);
    TEST("BG1 Y indep", bgGetScrollY(1) == 40);
    TEST("BG2 X indep", bgGetScrollX(2) == 50);
    TEST("BG2 Y indep", bgGetScrollY(2) == 60);
    TEST("BG3 X indep", bgGetScrollX(3) == 70);
    TEST("BG3 Y indep", bgGetScrollY(3) == 80);
}

// =============================================================================
// Test: bgInit resets scroll shadows
// =============================================================================
void test_bg_init(void) {
    // Set non-zero scroll
    bgSetScroll(0, 999, 888);
    TEST("pre-init X", bgGetScrollX(0) == 999);
    TEST("pre-init Y", bgGetScrollY(0) == 888);

    // bgInit should reset to (0, 0)
    bgInit(0);
    TEST("init resets X", bgGetScrollX(0) == 0);
    TEST("init resets Y", bgGetScrollY(0) == 0);

    // Other BGs should be unaffected
    bgSetScroll(1, 123, 456);
    bgInit(0);
    TEST("init BG0 no BG1 X", bgGetScrollX(1) == 123);
    TEST("init BG0 no BG1 Y", bgGetScrollY(1) == 456);
}

// =============================================================================
// Test: Scroll animation pattern (shadow tracks increments)
// =============================================================================
void test_scroll_animation(void) {
    u16 i;
    for (i = 0; i < 64; i++) {
        bgSetScroll(0, i, 0);
    }
    TEST("anim final X=63", bgGetScrollX(0) == 63);
    TEST("anim final Y=0", bgGetScrollY(0) == 0);
}

// =============================================================================
// Test: bgSetMapPtr (smoke — registers are write-only)
// =============================================================================
void test_bg_map_ptr(void) {
    bgSetMapPtr(0, 0x0000, BG_MAP_32x32);
    bgSetMapPtr(1, 0x0400, BG_MAP_64x32);
    bgSetMapPtr(2, 0x0800, BG_MAP_32x64);
    bgSetMapPtr(3, 0x0C00, BG_MAP_64x64);
    TEST("mapPtr no crash", 1);
}

// =============================================================================
// Test: bgSetGfxPtr (smoke — registers are write-only)
// =============================================================================
void test_bg_gfx_ptr(void) {
    bgSetGfxPtr(0, 0x0000);
    bgSetGfxPtr(1, 0x2000);
    bgSetGfxPtr(2, 0x4000);
    bgSetGfxPtr(3, 0x6000);
    TEST("gfxPtr no crash", 1);
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

    tests_passed = 0;
    tests_failed = 0;
    test_line = 4;

    // Run tests
    test_bg_scroll();
    test_bg_scroll_axis();
    test_bg_scroll_independence();
    test_bg_init();
    test_scroll_animation();
    test_bg_map_ptr();
    test_bg_gfx_ptr();

    // Display results
    textPrintAt(2, 26, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
