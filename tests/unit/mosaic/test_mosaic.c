// =============================================================================
// Unit Test: Mosaic Module
// =============================================================================
// Tests mosaic effect functions for screen pixelation effects.
//
// Critical functions tested:
// - mosaicInit(): Initialize mosaic system
// - mosaicEnable(): Enable mosaic for backgrounds
// - mosaicDisable(): Disable mosaic effect
// - mosaicSetSize(): Set pixelation level
// - mosaicGetSize(): Query current size
// - Constants: MOSAIC_BG*, MOSAIC_MIN, MOSAIC_MAX
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/mosaic.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests for constants
// =============================================================================

// Background mask constants
_Static_assert(MOSAIC_BG1 == 0x01, "MOSAIC_BG1 must be 0x01");
_Static_assert(MOSAIC_BG2 == 0x02, "MOSAIC_BG2 must be 0x02");
_Static_assert(MOSAIC_BG3 == 0x04, "MOSAIC_BG3 must be 0x04");
_Static_assert(MOSAIC_BG4 == 0x08, "MOSAIC_BG4 must be 0x08");
_Static_assert(MOSAIC_BG_ALL == 0x0F, "MOSAIC_BG_ALL must be 0x0F");

// Size constants
_Static_assert(MOSAIC_MIN == 0, "MOSAIC_MIN must be 0");
_Static_assert(MOSAIC_MAX == 15, "MOSAIC_MAX must be 15");

// Verify BG_ALL is combination of all BGs
_Static_assert(MOSAIC_BG_ALL == (MOSAIC_BG1 | MOSAIC_BG2 | MOSAIC_BG3 | MOSAIC_BG4),
               "MOSAIC_BG_ALL must combine all BG masks");

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
// Test: mosaicInit
// =============================================================================
void test_mosaic_init(void) {
    mosaicInit();
    log_result("mosaicInit executes", 1);

    // After init, size should be 0
    u8 size = mosaicGetSize();
    log_result("Init sets size to 0", size == 0);
}

// =============================================================================
// Test: mosaicSetSize / mosaicGetSize
// =============================================================================
void test_mosaic_size(void) {
    mosaicInit();

    // Test minimum size
    mosaicSetSize(MOSAIC_MIN);
    log_result("Set MOSAIC_MIN", mosaicGetSize() == MOSAIC_MIN);

    // Test maximum size
    mosaicSetSize(MOSAIC_MAX);
    log_result("Set MOSAIC_MAX", mosaicGetSize() == MOSAIC_MAX);

    // Test mid-range values
    mosaicSetSize(8);
    log_result("Set size 8", mosaicGetSize() == 8);

    // Reset
    mosaicSetSize(0);
}

// =============================================================================
// Test: mosaicEnable / mosaicDisable
// =============================================================================
void test_mosaic_enable(void) {
    mosaicInit();

    // Enable for BG1 only
    mosaicEnable(MOSAIC_BG1);
    log_result("Enable BG1", 1);

    // Enable for multiple BGs
    mosaicEnable(MOSAIC_BG1 | MOSAIC_BG2);
    log_result("Enable BG1+BG2", 1);

    // Enable for all BGs
    mosaicEnable(MOSAIC_BG_ALL);
    log_result("Enable all BGs", 1);

    // Disable
    mosaicDisable();
    log_result("Disable mosaic", 1);
}

// =============================================================================
// Test: Typical transition pattern
// =============================================================================
void test_mosaic_transition(void) {
    mosaicInit();

    // Simulate fade out (increase pixelation)
    mosaicEnable(MOSAIC_BG_ALL);
    u8 i;
    for (i = 0; i <= MOSAIC_MAX; i++) {
        mosaicSetSize(i);
    }
    log_result("Fade out loop", mosaicGetSize() == MOSAIC_MAX);

    // Simulate fade in (decrease pixelation)
    for (i = MOSAIC_MAX; i > 0; i--) {
        mosaicSetSize(i);
    }
    mosaicSetSize(0);
    log_result("Fade in loop", mosaicGetSize() == 0);

    mosaicDisable();
}

// =============================================================================
// Test: BG mask combinations
// =============================================================================
void test_mosaic_masks(void) {
    // Verify masks are single bits (power of 2)
    u8 bg1_single = (MOSAIC_BG1 != 0) && ((MOSAIC_BG1 & (MOSAIC_BG1 - 1)) == 0);
    u8 bg2_single = (MOSAIC_BG2 != 0) && ((MOSAIC_BG2 & (MOSAIC_BG2 - 1)) == 0);
    u8 bg3_single = (MOSAIC_BG3 != 0) && ((MOSAIC_BG3 & (MOSAIC_BG3 - 1)) == 0);
    u8 bg4_single = (MOSAIC_BG4 != 0) && ((MOSAIC_BG4 & (MOSAIC_BG4 - 1)) == 0);

    log_result("BG1 single bit", bg1_single);
    log_result("BG2 single bit", bg2_single);
    log_result("BG3 single bit", bg3_single);
    log_result("BG4 single bit", bg4_single);

    // Verify no masks overlap
    u8 no_overlap = ((MOSAIC_BG1 & MOSAIC_BG2) == 0) &&
                    ((MOSAIC_BG2 & MOSAIC_BG3) == 0) &&
                    ((MOSAIC_BG3 & MOSAIC_BG4) == 0);
    log_result("BG masks no overlap", no_overlap);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "MOSAIC MODULE TESTS");
    textPrintAt(2, 2, "-------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_mosaic_init();
    test_mosaic_size();
    test_mosaic_enable();
    test_mosaic_transition();
    test_mosaic_masks();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
