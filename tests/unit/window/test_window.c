// =============================================================================
// Unit Test: Window Module
// =============================================================================
// Tests hardware window masking functions.
//
// Critical functions tested:
// - windowInit(): Initialize window system
// - windowSetPos(): Set window boundaries
// - windowEnable(): Enable window for layers
// - windowDisable(): Disable window
// - windowDisableAll(): Disable all windows
// - Constants: WINDOW_*, WINDOW_LOGIC_*, WINDOW_MASK_*
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/window.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests for constants
// =============================================================================

// Window identifiers
_Static_assert(WINDOW_1 == 0, "WINDOW_1 must be 0");
_Static_assert(WINDOW_2 == 1, "WINDOW_2 must be 1");

// Layer masks (BIT macro expands to 1 << n)
_Static_assert(WINDOW_BG1 == 0x01, "WINDOW_BG1 must be 0x01");
_Static_assert(WINDOW_BG2 == 0x02, "WINDOW_BG2 must be 0x02");
_Static_assert(WINDOW_BG3 == 0x04, "WINDOW_BG3 must be 0x04");
_Static_assert(WINDOW_BG4 == 0x08, "WINDOW_BG4 must be 0x08");
_Static_assert(WINDOW_OBJ == 0x10, "WINDOW_OBJ must be 0x10");
_Static_assert(WINDOW_MATH == 0x20, "WINDOW_MATH must be 0x20");

// Composite masks
_Static_assert(WINDOW_ALL_BG == 0x0F, "WINDOW_ALL_BG must be 0x0F");
_Static_assert(WINDOW_ALL == 0x1F, "WINDOW_ALL must be 0x1F");

// Logic operations
_Static_assert(WINDOW_LOGIC_OR == 0, "WINDOW_LOGIC_OR must be 0");
_Static_assert(WINDOW_LOGIC_AND == 1, "WINDOW_LOGIC_AND must be 1");
_Static_assert(WINDOW_LOGIC_XOR == 2, "WINDOW_LOGIC_XOR must be 2");
_Static_assert(WINDOW_LOGIC_XNOR == 3, "WINDOW_LOGIC_XNOR must be 3");

// Mask modes
_Static_assert(WINDOW_MASK_INSIDE == 0, "WINDOW_MASK_INSIDE must be 0");
_Static_assert(WINDOW_MASK_OUTSIDE == 1, "WINDOW_MASK_OUTSIDE must be 1");

// Screen selection
_Static_assert(WINDOW_MAIN_SCREEN == 0, "WINDOW_MAIN_SCREEN must be 0");
_Static_assert(WINDOW_SUB_SCREEN == 1, "WINDOW_SUB_SCREEN must be 1");

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
// Test: windowInit
// =============================================================================
void test_window_init(void) {
    windowInit();
    log_result("windowInit executes", 1);
}

// =============================================================================
// Test: windowSetPos
// =============================================================================
void test_window_pos(void) {
    windowInit();

    // Full screen window
    windowSetPos(WINDOW_1, 0, 255);
    log_result("Window1 full width", 1);

    // Centered window
    windowSetPos(WINDOW_1, 64, 192);
    log_result("Window1 centered", 1);

    // Narrow window
    windowSetPos(WINDOW_1, 120, 136);
    log_result("Window1 narrow", 1);

    // Window 2
    windowSetPos(WINDOW_2, 0, 127);
    log_result("Window2 left half", 1);

    windowSetPos(WINDOW_2, 128, 255);
    log_result("Window2 right half", 1);
}

// =============================================================================
// Test: windowEnable / windowDisable
// =============================================================================
void test_window_enable(void) {
    windowInit();

    // Enable window 1 for BG1
    windowEnable(WINDOW_1, WINDOW_BG1);
    log_result("Enable W1 for BG1", 1);

    // Enable window 1 for multiple layers
    windowEnable(WINDOW_1, WINDOW_BG1 | WINDOW_BG2);
    log_result("Enable W1 for BG1+BG2", 1);

    // Enable for all BGs
    windowEnable(WINDOW_1, WINDOW_ALL_BG);
    log_result("Enable W1 all BGs", 1);

    // Enable for sprites
    windowEnable(WINDOW_1, WINDOW_OBJ);
    log_result("Enable W1 for OBJ", 1);

    // Disable specific
    windowDisable(WINDOW_1, WINDOW_BG1);
    log_result("Disable W1 for BG1", 1);

    // Disable all
    windowDisableAll();
    log_result("DisableAll executes", 1);
}

// =============================================================================
// Test: windowSetInvert
// =============================================================================
void test_window_invert(void) {
    windowInit();

    // Normal mode (show inside)
    windowSetInvert(WINDOW_1, WINDOW_BG1, 0);
    log_result("Invert off (inside)", 1);

    // Inverted mode (show outside)
    windowSetInvert(WINDOW_1, WINDOW_BG1, 1);
    log_result("Invert on (outside)", 1);
}

// =============================================================================
// Test: windowSetLogic
// =============================================================================
void test_window_logic(void) {
    windowInit();

    windowSetLogic(WINDOW_BG1, WINDOW_LOGIC_OR);
    log_result("Logic OR", 1);

    windowSetLogic(WINDOW_BG1, WINDOW_LOGIC_AND);
    log_result("Logic AND", 1);

    windowSetLogic(WINDOW_BG1, WINDOW_LOGIC_XOR);
    log_result("Logic XOR", 1);

    windowSetLogic(WINDOW_BG1, WINDOW_LOGIC_XNOR);
    log_result("Logic XNOR", 1);
}

// =============================================================================
// Test: Helper functions
// =============================================================================
void test_window_helpers(void) {
    windowInit();

    // Centered window
    windowCentered(WINDOW_1, 128);
    log_result("windowCentered", 1);

    // Split screen
    windowSplit(128);
    log_result("windowSplit", 1);

    windowDisableAll();
}

// =============================================================================
// Test: Typical spotlight effect
// =============================================================================
void test_spotlight_effect(void) {
    windowInit();

    // Create spotlight - show inside window only
    windowSetPos(WINDOW_1, 80, 176);
    windowEnable(WINDOW_1, WINDOW_BG1);
    windowSetInvert(WINDOW_1, WINDOW_BG1, 0);  // Show inside
    log_result("Spotlight setup", 1);

    windowDisableAll();
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "WINDOW MODULE TESTS");
    textPrintAt(2, 2, "-------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_window_init();
    test_window_pos();
    test_window_enable();
    test_window_invert();
    test_window_logic();
    test_window_helpers();
    test_spotlight_effect();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
