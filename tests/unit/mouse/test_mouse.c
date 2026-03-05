// =============================================================================
// Unit Test: Mouse Module
// =============================================================================
// Tests the SNES mouse API constants and functions.
//
// Critical items tested:
// - Mouse button constants (MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT)
// - Sensitivity constants (MOUSE_SENS_LOW/MEDIUM/HIGH)
// - mouseInit(), mouseIsConnected() behavior without hardware
// - mouseGetX(), mouseGetY() return 0 without mouse
// - mouseButtonsHeld(), mouseButtonsPressed() return 0 without mouse
// - mouseGetSensitivity() default value
// - Port 0 vs port 1 independence
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/input.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests using _Static_assert
// =============================================================================

// Button constants
_Static_assert(MOUSE_BUTTON_LEFT == 0x01, "MOUSE_BUTTON_LEFT must be 0x01");
_Static_assert(MOUSE_BUTTON_RIGHT == 0x02, "MOUSE_BUTTON_RIGHT must be 0x02");

// Buttons must not overlap
_Static_assert((MOUSE_BUTTON_LEFT & MOUSE_BUTTON_RIGHT) == 0,
               "Mouse button masks must not overlap");

// Sensitivity constants
_Static_assert(MOUSE_SENS_LOW == 0, "MOUSE_SENS_LOW must be 0");
_Static_assert(MOUSE_SENS_MEDIUM == 1, "MOUSE_SENS_MEDIUM must be 1");
_Static_assert(MOUSE_SENS_HIGH == 2, "MOUSE_SENS_HIGH must be 2");

// Sensitivity values must be distinct
_Static_assert(MOUSE_SENS_LOW != MOUSE_SENS_MEDIUM,
               "MOUSE_SENS_LOW and MEDIUM must differ");
_Static_assert(MOUSE_SENS_MEDIUM != MOUSE_SENS_HIGH,
               "MOUSE_SENS_MEDIUM and HIGH must differ");
_Static_assert(MOUSE_SENS_LOW != MOUSE_SENS_HIGH,
               "MOUSE_SENS_LOW and HIGH must differ");

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

// Test that mouseInit returns 0 when no mouse is connected
void test_mouse_init_no_hardware(void) {
    u8 result;

    result = mouseInit(0);
    log_result("mouseInit(0) returns 0", result == 0);

    result = mouseInit(1);
    log_result("mouseInit(1) returns 0", result == 0);
}

// Test mouseIsConnected returns 0 without mouse
void test_mouse_not_connected(void) {
    log_result("mouseIsConnected(0)==0", mouseIsConnected(0) == 0);
    log_result("mouseIsConnected(1)==0", mouseIsConnected(1) == 0);
}

// Test displacement getters return 0 without mouse
void test_mouse_displacement_zero(void) {
    s16 x, y;

    x = mouseGetX(0);
    y = mouseGetY(0);
    log_result("mouseGetX(0)==0", x == 0);
    log_result("mouseGetY(0)==0", y == 0);

    x = mouseGetX(1);
    y = mouseGetY(1);
    log_result("mouseGetX(1)==0", x == 0);
    log_result("mouseGetY(1)==0", y == 0);
}

// Test button getters return 0 without mouse
void test_mouse_buttons_zero(void) {
    log_result("buttonsHeld(0)==0", mouseButtonsHeld(0) == 0);
    log_result("buttonsHeld(1)==0", mouseButtonsHeld(1) == 0);
    log_result("buttonsPressed(0)==0", mouseButtonsPressed(0) == 0);
    log_result("buttonsPressed(1)==0", mouseButtonsPressed(1) == 0);
}

// Test sensitivity default is 0
void test_mouse_sensitivity_default(void) {
    log_result("getSens(0)==0", mouseGetSensitivity(0) == 0);
    log_result("getSens(1)==0", mouseGetSensitivity(1) == 0);
}

// Test invalid port doesn't crash
void test_mouse_invalid_port(void) {
    u8 result;
    s16 val;

    result = mouseInit(2);
    log_result("mouseInit(2) returns 0", result == 0);

    result = mouseIsConnected(2);
    log_result("mouseIsConnected(2)==0", result == 0);

    val = mouseGetX(2);
    log_result("mouseGetX(2)==0", val == 0);

    val = mouseGetY(2);
    log_result("mouseGetY(2)==0", val == 0);

    result = mouseButtonsHeld(2);
    log_result("buttonsHeld(2)==0", result == 0);

    result = mouseButtonsPressed(2);
    log_result("buttonsPressed(2)==0", result == 0);

    result = mouseGetSensitivity(2);
    log_result("getSens(2)==0", result == 0);
}

// Test init and isConnected coherence
void test_mouse_init_connected_coherence(void) {
    u8 init_result;
    u8 connected;

    init_result = mouseInit(0);
    connected = mouseIsConnected(0);

    // If init returned 0 (no mouse), isConnected should also be 0
    // (or if init returned 1, isConnected should be 1)
    log_result("init/connected coherent", init_result == connected);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "MOUSE MODULE TESTS");
    textPrintAt(2, 2, "------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_mouse_init_no_hardware();
    test_mouse_not_connected();
    test_mouse_displacement_zero();
    test_mouse_buttons_zero();
    test_mouse_sensitivity_default();
    test_mouse_invalid_port();
    test_mouse_init_connected_coherence();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
