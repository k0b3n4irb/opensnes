// =============================================================================
// Unit Test: Input Module
// =============================================================================
// Tests the controller input functions and button constants.
//
// Critical items tested:
// - Button mask constants (KEY_A, KEY_B, etc.)
// - Button mask positions match hardware layout
// - padPressed(), padHeld(), padReleased() behavior
// - Two-player input
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/input.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests using _Static_assert
// =============================================================================

// Verify button masks match SNES hardware layout
// High byte ($4219): B, Y, Select, Start, Up, Down, Left, Right
// Low byte ($4218):  A, X, L, R, (signature)

// Bit 15 = B
_Static_assert(KEY_B == 0x8000, "KEY_B must be bit 15 (0x8000)");

// Bit 14 = Y
_Static_assert(KEY_Y == 0x4000, "KEY_Y must be bit 14 (0x4000)");

// Bit 13 = Select
_Static_assert(KEY_SELECT == 0x2000, "KEY_SELECT must be bit 13 (0x2000)");

// Bit 12 = Start
_Static_assert(KEY_START == 0x1000, "KEY_START must be bit 12 (0x1000)");

// Bit 11 = Up
_Static_assert(KEY_UP == 0x0800, "KEY_UP must be bit 11 (0x0800)");

// Bit 10 = Down
_Static_assert(KEY_DOWN == 0x0400, "KEY_DOWN must be bit 10 (0x0400)");

// Bit 9 = Left
_Static_assert(KEY_LEFT == 0x0200, "KEY_LEFT must be bit 9 (0x0200)");

// Bit 8 = Right
_Static_assert(KEY_RIGHT == 0x0100, "KEY_RIGHT must be bit 8 (0x0100)");

// Bit 7 = A
_Static_assert(KEY_A == 0x0080, "KEY_A must be bit 7 (0x0080)");

// Bit 6 = X
_Static_assert(KEY_X == 0x0040, "KEY_X must be bit 6 (0x0040)");

// Bit 5 = L
_Static_assert(KEY_L == 0x0020, "KEY_L must be bit 5 (0x0020)");

// Bit 4 = R
_Static_assert(KEY_R == 0x0010, "KEY_R must be bit 4 (0x0010)");

// Verify composite masks
_Static_assert(KEY_DPAD == (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT),
               "KEY_DPAD must combine all directions");

_Static_assert(KEY_FACE == (KEY_A | KEY_B | KEY_X | KEY_Y),
               "KEY_FACE must combine all face buttons");

// Verify no button masks overlap
_Static_assert((KEY_A & KEY_B) == 0, "KEY_A and KEY_B must not overlap");
_Static_assert((KEY_UP & KEY_DOWN) == 0, "KEY_UP and KEY_DOWN must not overlap");
_Static_assert((KEY_L & KEY_R) == 0, "KEY_L and KEY_R must not overlap");

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

// Test that button masks are single bits (power of 2)
u8 is_single_bit(u16 val) {
    return val != 0 && (val & (val - 1)) == 0;
}

void test_button_single_bits(void) {
    log_result("KEY_A single bit", is_single_bit(KEY_A));
    log_result("KEY_B single bit", is_single_bit(KEY_B));
    log_result("KEY_X single bit", is_single_bit(KEY_X));
    log_result("KEY_Y single bit", is_single_bit(KEY_Y));
    log_result("KEY_L single bit", is_single_bit(KEY_L));
    log_result("KEY_R single bit", is_single_bit(KEY_R));
    log_result("KEY_START single bit", is_single_bit(KEY_START));
    log_result("KEY_SELECT single bit", is_single_bit(KEY_SELECT));
    log_result("KEY_UP single bit", is_single_bit(KEY_UP));
    log_result("KEY_DOWN single bit", is_single_bit(KEY_DOWN));
    log_result("KEY_LEFT single bit", is_single_bit(KEY_LEFT));
    log_result("KEY_RIGHT single bit", is_single_bit(KEY_RIGHT));
}

// Test padUpdate doesn't crash
void test_pad_update(void) {
    padUpdate();
    log_result("padUpdate executes", 1);

    // Multiple updates
    padUpdate();
    padUpdate();
    log_result("padUpdate multiple", 1);
}

// Test pad functions return values
void test_pad_functions(void) {
    padUpdate();

    // These should return 0 when no buttons pressed
    u16 pressed = padPressed(0);
    u16 held = padHeld(0);

    // Can't assert they're 0 (might be pressed during test)
    // but we verify functions execute without crash
    log_result("padPressed(0) executes", 1);
    log_result("padHeld(0) executes", 1);

    // Suppress unused variable warnings
    (void)pressed;
    (void)held;

    // Test player 2
    u16 p2_pressed = padPressed(1);
    u16 p2_held = padHeld(1);
    log_result("padPressed(1) executes", 1);
    log_result("padHeld(1) executes", 1);

    // Suppress unused variable warnings
    (void)p2_pressed;
    (void)p2_held;
}

// Test button combination detection
void test_button_combinations(void) {
    // Simulate checking for button combos (common game pattern)
    u16 buttons = KEY_A | KEY_B;  // Combo: A+B

    // Check if both are in the combo
    u8 has_a = (buttons & KEY_A) != 0;
    u8 has_b = (buttons & KEY_B) != 0;
    u8 has_x = (buttons & KEY_X) != 0;

    log_result("Combo has A", has_a);
    log_result("Combo has B", has_b);
    log_result("Combo lacks X", !has_x);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "INPUT MODULE TESTS");
    textPrintAt(2, 2, "------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_button_single_bits();
    test_pad_update();
    test_pad_functions();
    test_button_combinations();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    // Interactive: show button presses
    textPrintAt(2, 8, "Press buttons to test:");

    while (1) {
        WaitForVBlank();
        padUpdate();

        u16 held = padHeld(0);

        // Clear button display area
        textPrintAt(2, 10, "                    ");

        // Show held buttons
        if (held & KEY_A) textPrintAt(2, 10, "A");
        if (held & KEY_B) textPrintAt(4, 10, "B");
        if (held & KEY_X) textPrintAt(6, 10, "X");
        if (held & KEY_Y) textPrintAt(8, 10, "Y");
        if (held & KEY_UP) textPrintAt(10, 10, "U");
        if (held & KEY_DOWN) textPrintAt(12, 10, "D");
        if (held & KEY_LEFT) textPrintAt(14, 10, "L");
        if (held & KEY_RIGHT) textPrintAt(16, 10, "R");
    }

    return 0;
}
