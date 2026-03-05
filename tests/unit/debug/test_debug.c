// =============================================================================
// Unit Test: Debug Module
// =============================================================================
// Smoke tests for SNES_BREAK, SNES_NOCASH, and SNES_ASSERT macros.
// These functions write to emulator debug ports (write-only), so we can only
// verify they don't crash. The actual debug output must be verified in Mesen2.
// =============================================================================

#include <snes.h>
#include <snes/console.h>
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
// Test: SNES_NOCASH — smoke test (no crash)
// =============================================================================
void test_nocash_message(void) {
    // Send a debug message — should not crash
    SNES_NOCASH("test message");
    TEST("nocash: no crash", 1);

    // Empty string should also be safe
    SNES_NOCASH("");
    TEST("nocash: empty ok", 1);

    // Direct function call
    consoleNocashMessage("direct call");
    TEST("nocash: direct ok", 1);
}

// =============================================================================
// Test: SNES_ASSERT — true condition should not break
// =============================================================================
void test_assert_true(void) {
    // True conditions should pass without breaking
    SNES_ASSERT(1);
    TEST("assert: true ok", 1);

    SNES_ASSERT(42 == 42);
    TEST("assert: eq ok", 1);

    SNES_ASSERT(100 > 50);
    TEST("assert: gt ok", 1);
}

// =============================================================================
// Test: SNES_BREAK exists as a callable function
// Note: We don't actually call SNES_BREAK() here because it would halt
// execution in Mesen2. We just verify the function compiles and links.
// =============================================================================
void test_break_compiles(void) {
    // Verify the function pointer is valid (not null)
    void (*bp)(void) = consoleMesenBreakpoint;
    TEST("break: linked", bp != (void*)0);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(1, 1, "DEBUG MODULE TESTS");
    textPrintAt(1, 2, "------------------");

    tests_passed = 0;
    tests_failed = 0;
    test_line = 4;

    // Run all tests
    test_nocash_message();
    test_assert_true();
    test_break_compiles();

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
