// =============================================================================
// Unit Test: Super Scope Module
// =============================================================================
// Tests the SNES Super Scope API constants and functions.
//
// Critical items tested:
// - Super Scope button/flag constants (SSC_FIRE, SSC_CURSOR, etc.)
// - scopeInit() behavior without hardware (returns 0)
// - scopeIsConnected() returns 0 without Super Scope
// - Position getters return 0 without Super Scope
// - Button getters return 0 without Super Scope
// - scopeCalibrate() does not crash
// - scopeSetHoldDelay() / scopeSetRepeatDelay() do not crash
// - scopeSinceShot() returns 0 without Super Scope
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/input.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests using _Static_assert
// =============================================================================

// Button constants
_Static_assert(SSC_FIRE == 0x8000, "SSC_FIRE must be 0x8000");
_Static_assert(SSC_CURSOR == 0x4000, "SSC_CURSOR must be 0x4000");
_Static_assert(SSC_TURBO == 0x2000, "SSC_TURBO must be 0x2000");
_Static_assert(SSC_PAUSE == 0x1000, "SSC_PAUSE must be 0x1000");
_Static_assert(SSC_OFFSCREEN == 0x0200, "SSC_OFFSCREEN must be 0x0200");
_Static_assert(SSC_NOISE == 0x0100, "SSC_NOISE must be 0x0100");

// Button masks must not overlap
_Static_assert((SSC_FIRE & SSC_CURSOR) == 0, "FIRE and CURSOR must not overlap");
_Static_assert((SSC_FIRE & SSC_TURBO) == 0, "FIRE and TURBO must not overlap");
_Static_assert((SSC_FIRE & SSC_PAUSE) == 0, "FIRE and PAUSE must not overlap");
_Static_assert((SSC_CURSOR & SSC_TURBO) == 0, "CURSOR and TURBO must not overlap");
_Static_assert((SSC_CURSOR & SSC_PAUSE) == 0, "CURSOR and PAUSE must not overlap");
_Static_assert((SSC_TURBO & SSC_PAUSE) == 0, "TURBO and PAUSE must not overlap");
_Static_assert((SSC_OFFSCREEN & SSC_NOISE) == 0, "OFFSCREEN and NOISE must not overlap");

// Flags must not overlap with buttons
_Static_assert((SSC_OFFSCREEN & (SSC_FIRE | SSC_CURSOR | SSC_TURBO | SSC_PAUSE)) == 0,
               "OFFSCREEN must not overlap buttons");
_Static_assert((SSC_NOISE & (SSC_FIRE | SSC_CURSOR | SSC_TURBO | SSC_PAUSE)) == 0,
               "NOISE must not overlap buttons");

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

// Test that scopeInit returns 0 when no Super Scope is connected
void test_scope_init_no_hardware(void) {
    u8 result;

    result = scopeInit();
    log_result("scopeInit() returns 0", result == 0);
}

// Test scopeIsConnected returns 0 without Super Scope
void test_scope_not_connected(void) {
    log_result("scopeIsConnected()==0", scopeIsConnected() == 0);
}

// Test position getters return 0 without Super Scope
void test_scope_position_zero(void) {
    u16 x, y;

    x = scopeGetX();
    y = scopeGetY();
    log_result("scopeGetX()==0", x == 0);
    log_result("scopeGetY()==0", y == 0);

    x = scopeGetRawX();
    y = scopeGetRawY();
    log_result("scopeGetRawX()==0", x == 0);
    log_result("scopeGetRawY()==0", y == 0);
}

// Test button getters return 0 without Super Scope
void test_scope_buttons_zero(void) {
    log_result("buttonsDown()==0", scopeButtonsDown() == 0);
    log_result("buttonsPressed()==0", scopeButtonsPressed() == 0);
    log_result("buttonsHeld()==0", scopeButtonsHeld() == 0);
}

// Test scopeSinceShot returns 0 without Super Scope
void test_scope_since_shot_zero(void) {
    log_result("sinceShot()==0", scopeSinceShot() == 0);
}

// Test scopeCalibrate does not crash
void test_scope_calibrate_safe(void) {
    scopeCalibrate();
    log_result("calibrate no crash", 1);
}

// Test scopeSetHoldDelay / scopeSetRepeatDelay do not crash
void test_scope_set_delays_safe(void) {
    scopeSetHoldDelay(30);
    log_result("setHoldDelay no crash", 1);

    scopeSetRepeatDelay(10);
    log_result("setRepeatDelay no crash", 1);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "SUPER SCOPE TESTS");
    textPrintAt(2, 2, "-----------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_scope_init_no_hardware();
    test_scope_not_connected();
    test_scope_position_zero();
    test_scope_buttons_zero();
    test_scope_since_shot_zero();
    test_scope_calibrate_safe();
    test_scope_set_delays_safe();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
