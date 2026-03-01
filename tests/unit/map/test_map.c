// =============================================================================
// Unit Test: Map Engine
// =============================================================================
// Tests the map engine's constants, camera variables, and basic initialization.
//
// Note: Most map engine functionality writes to VRAM (not readable from C),
// so these are primarily smoke tests and constant verification.
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/map.h>
#include <snes/debug.h>
#include <snes/text.h>

// Test counters
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
// Test: Map constants
// =============================================================================
void test_map_constants(void) {
    // Tile properties
    TEST("T_EMPTY=0x0000", T_EMPTY == 0x0000);
    TEST("T_SOLID=0xFF00", T_SOLID == 0xFF00);
    TEST("T_LADDER=0x0001", T_LADDER == 0x0001);
    TEST("T_FIRES=0x0002", T_FIRES == 0x0002);
    TEST("T_SPIKE=0x0004", T_SPIKE == 0x0004);
    TEST("T_PLATE=0x0008", T_PLATE == 0x0008);

    // Slope constants
    TEST("T_SLOPEU1=0x0020", T_SLOPEU1 == 0x0020);
    TEST("T_SLOPED1=0x0021", T_SLOPED1 == 0x0021);

    // Action constants
    TEST("ACT_STAND=0", ACT_STAND == 0x0000);
    TEST("ACT_WALK=1", ACT_WALK == 0x0001);
    TEST("ACT_JUMP=2", ACT_JUMP == 0x0002);
    TEST("ACT_FALL=4", ACT_FALL == 0x0004);

    // Map options
    TEST("MAP_OPT_1WAY=1", MAP_OPT_1WAY == 0x01);
    TEST("MAP_OPT_BG2=2", MAP_OPT_BG2 == 0x02);
}

// =============================================================================
// Test: Camera variables accessible and initially 0
// =============================================================================
void test_camera_vars(void) {
    // Camera positions should be accessible (Bank $00)
    // After consoleInit but before mapLoad, they should be 0
    TEST("x_pos init=0", x_pos == 0);
    TEST("y_pos init=0", y_pos == 0);

    // Verify they're writable (C-accessible Bank $00)
    x_pos = 100;
    y_pos = 200;
    TEST("x_pos write", x_pos == 100);
    TEST("y_pos write", y_pos == 200);

    // Reset for other tests
    x_pos = 0;
    y_pos = 0;
}

// =============================================================================
// Test: Tile property bit patterns are unique
// =============================================================================
void test_tile_props_unique(void) {
    // Each tile property should be distinguishable
    TEST("empty!=solid", T_EMPTY != T_SOLID);
    TEST("ladder!=fires", T_LADDER != T_FIRES);
    TEST("spike!=plate", T_SPIKE != T_PLATE);

    // Solid should have high byte set
    TEST("solid highbyte", (T_SOLID & 0xFF00) != 0);
    TEST("empty no high", (T_EMPTY & 0xFF00) == 0);
}

// =============================================================================
// Test: mapSetMapOptions — smoke test (no crash)
// =============================================================================
void test_set_options(void) {
    // These should not crash
    mapSetMapOptions(0);
    TEST("opts: 0 ok", 1);

    mapSetMapOptions(MAP_OPT_1WAY);
    TEST("opts: 1way ok", 1);

    mapSetMapOptions(MAP_OPT_BG2);
    TEST("opts: bg2 ok", 1);

    mapSetMapOptions(MAP_OPT_1WAY | MAP_OPT_BG2);
    TEST("opts: both ok", 1);

    // Reset
    mapSetMapOptions(0);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(1, 1, "MAP ENGINE TESTS");
    textPrintAt(1, 2, "----------------");

    tests_passed = 0;
    tests_failed = 0;
    test_line = 4;

    // Run all tests
    test_map_constants();
    test_camera_vars();
    test_tile_props_unique();
    test_set_options();

    // Show summary
    test_line += 2;
    textPrintAt(1, test_line, "Passed: ");
    textPrintU16(tests_passed);
    test_line++;
    textPrintAt(1, test_line, "Failed: ");
    textPrintU16(tests_failed);
    test_line++;

    if (tests_failed == 0) {
        textPrintAt(1, test_line, "ALL TESTS PASSED");
    }

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
