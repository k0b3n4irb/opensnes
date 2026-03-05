// =============================================================================
// Unit Test: Console Module
// =============================================================================
// Tests core console initialization and system functions.
//
// Critical functions tested:
// - consoleInit(): Hardware initialization
// - setScreenOn/Off(): Display control
// - setBrightness/getBrightness(): Brightness control
// - WaitForVBlank(): Frame synchronization
// - getFrameCount/resetFrameCount(): Frame counter
// - isPAL/getRegion(): System detection
// - rand/srand(): Random number generation
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/text.h>

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
// Test: Screen control
// =============================================================================
void test_screen_control(void) {
    // Screen should be off after consoleInit
    // Turn it on
    setScreenOn();
    log_result("setScreenOn executes", 1);

    // Turn it off
    setScreenOff();
    log_result("setScreenOff executes", 1);

    // Back on for the rest of tests
    setScreenOn();
}

// =============================================================================
// Test: Brightness control
// =============================================================================
void test_brightness(void) {
    // Full brightness
    setBrightness(15);
    u8 bright = getBrightness();
    log_result("Brightness 15", bright == 15);

    // Minimum brightness
    setBrightness(0);
    bright = getBrightness();
    log_result("Brightness 0", bright == 0);

    // Mid brightness
    setBrightness(8);
    bright = getBrightness();
    log_result("Brightness 8", bright == 8);

    // Restore full brightness
    setBrightness(15);
}

// =============================================================================
// Test: VBlank synchronization
// =============================================================================
void test_vblank(void) {
    // WaitForVBlank should return (not hang)
    WaitForVBlank();
    log_result("WaitForVBlank returns", 1);

    // Multiple waits
    WaitForVBlank();
    WaitForVBlank();
    log_result("Multiple VBlank waits", 1);
}

// =============================================================================
// Test: Frame counter
// =============================================================================
void test_frame_counter(void) {
    // Reset frame counter
    resetFrameCount();
    u16 count1 = getFrameCount();
    log_result("Reset frame count", count1 == 0 || count1 == 1);  // May be 1 if VBlank occurred

    // Wait a frame and check it increments
    WaitForVBlank();
    u16 count2 = getFrameCount();
    log_result("Frame count increments", count2 > count1);

    // Wait another frame
    WaitForVBlank();
    u16 count3 = getFrameCount();
    log_result("Frame count continues", count3 > count2);
}

// =============================================================================
// Test: System region
// =============================================================================
void test_region(void) {
    // isPAL returns 0 or 1
    u8 pal = isPAL();
    log_result("isPAL returns valid", pal == 0 || pal == 1);

    // getRegion returns 0 or 1
    u8 region = getRegion();
    log_result("getRegion returns valid", region == 0 || region == 1);

    // Both should match
    log_result("Region functions match", (pal == 0 && region == 0) || (pal == 1 && region == 1));
}

// =============================================================================
// Test: Random number generation
// =============================================================================
void test_random(void) {
    // Seed the RNG
    srand(12345);
    log_result("srand executes", 1);

    // Get some random numbers
    u16 r1 = rand();
    u16 r2 = rand();
    u16 r3 = rand();

    // Numbers should be different (very high probability)
    log_result("rand returns varied", (r1 != r2) || (r2 != r3));

    // Re-seed with same value should produce same sequence
    srand(12345);
    u16 r1_again = rand();
    log_result("rand is reproducible", r1 == r1_again);

    // Different seed should produce different sequence
    srand(54321);
    u16 r_different = rand();
    log_result("Different seed differs", r_different != r1);
}

// =============================================================================
// Test: Brightness fade pattern
// =============================================================================
void test_brightness_fade(void) {
    // Simulate fade out
    u8 i;
    for (i = 15; i > 0; i--) {
        setBrightness(i);
        // Don't WaitForVBlank here to keep test fast
    }
    setBrightness(0);
    log_result("Fade out loop", getBrightness() == 0);

    // Simulate fade in
    for (i = 0; i <= 15; i++) {
        setBrightness(i);
    }
    log_result("Fade in loop", getBrightness() == 15);
}

// =============================================================================
// Test: Multiple random sequences
// =============================================================================
void test_random_distribution(void) {
    srand(getFrameCount());  // Seed from frame count

    // Generate many numbers and check they vary
    u8 i;
    u16 values[8];
    for (i = 0; i < 8; i++) {
        values[i] = rand();
    }

    // Check at least some variation exists
    u8 all_same = 1;
    for (i = 1; i < 8; i++) {
        if (values[i] != values[0]) {
            all_same = 0;
            break;
        }
    }
    log_result("Random has variation", !all_same);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    // consoleInit is already called implicitly - it's required for everything
    consoleInit();
    log_result("consoleInit executes", 1);

    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "CONSOLE MODULE TESTS");
    textPrintAt(2, 2, "--------------------");

    test_passed = 0;
    test_failed = 0;

    // Count consoleInit as first test
    test_passed = 1;

    // Run tests
    test_screen_control();
    test_brightness();
    test_vblank();
    test_frame_counter();
    test_region();
    test_random();
    test_brightness_fade();
    test_random_distribution();

    // Display results
    textPrintAt(2, 4, "Tests completed");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
