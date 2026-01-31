/**
 * @file test_math.c
 * @brief Unit tests for math functions
 *
 * Tests fixed-point math, trigonometry, etc.
 */

#include <snes.h>
#include <snes/math.h>
#include <snes/console.h>
#include <snes/text.h>

static u8 tests_passed;
static u8 tests_failed;

#define TEST(name, condition) do { \
    if (condition) { \
        tests_passed++; \
        textPrintAt(0, tests_passed + tests_failed, "PASS: " name); \
    } else { \
        tests_failed++; \
        textPrintAt(0, tests_passed + tests_failed, "FAIL: " name); \
    } \
} while(0)

void test_fixed_point(void) {
    fixed a, b;
    s16 result;

    // Test FIX macro (8.8 fixed point)
    a = FIX(10);
    TEST("FIX_10", a == 2560);  // 10 * 256 = 2560

    // Test UNFIX macro
    result = UNFIX(FIX(25));
    TEST("UNFIX_25", result == 25);

    // Test fixed-point addition
    a = FIX(5);
    b = FIX(3);
    result = UNFIX(a + b);
    TEST("fix_add", result == 8);

    // Test fixed-point subtraction
    result = UNFIX(a - b);
    TEST("fix_sub", result == 2);

    // Test fractional values
    a = FIX(1) / 2;  // 0.5
    b = FIX(1) / 2;  // 0.5
    result = UNFIX(a + b);
    TEST("fix_half", result == 1);

    // Test negative values
    a = FIX(-5);
    result = UNFIX(a);
    TEST("fix_neg", result == -5);

    // Test FIX_FRAC
    a = FIX(3) + 128;  // 3.5
    TEST("fix_frac", FIX_FRAC(a) == 128);

    // Test FIX_MAKE
    a = FIX_MAKE(5, 128);  // 5.5
    result = UNFIX(a);
    TEST("fix_make_int", result == 5);
    TEST("fix_make_frac", FIX_FRAC(a) == 128);
}

void test_fix_mul(void) {
    fixed a, b, result;

    // 2.0 * 3.0 = 6.0
    a = FIX(2);
    b = FIX(3);
    result = fixMul(a, b);
    TEST("fixMul_int", UNFIX(result) == 6);

    // 0.5 * 4.0 = 2.0
    a = FIX(1) / 2;
    b = FIX(4);
    result = fixMul(a, b);
    TEST("fixMul_frac", UNFIX(result) == 2);

    // -2.0 * 3.0 = -6.0
    a = FIX(-2);
    b = FIX(3);
    result = fixMul(a, b);
    TEST("fixMul_neg", UNFIX(result) == -6);
}

void test_trig(void) {
    fixed result;

    // sin(0) = 0
    result = fixSin(0);
    TEST("sin_0", UNFIX(result) == 0);

    // sin(64) = sin(90deg) = 1.0 (256 in fixed)
    result = fixSin(64);
    TEST("sin_90", result == 256 || result == 255);  // Allow small error

    // sin(128) = sin(180deg) = 0
    result = fixSin(128);
    TEST("sin_180", UNFIX(result) == 0);

    // cos(0) = 1.0
    result = fixCos(0);
    TEST("cos_0", result == 256 || result == 255);

    // cos(64) = cos(90deg) = 0
    result = fixCos(64);
    TEST("cos_90", UNFIX(result) == 0);
}

void test_fixAbs(void) {
    fixed result;

    result = fixAbs(FIX(10));
    TEST("fixAbs_pos", UNFIX(result) == 10);

    result = fixAbs(FIX(-10));
    TEST("fixAbs_neg", UNFIX(result) == 10);

    result = fixAbs(0);
    TEST("fixAbs_zero", result == 0);
}

void test_random(void) {
    u16 r1, r2, r3;

    // Seed random
    srand(12345);

    // Get random numbers
    r1 = rand();
    r2 = rand();
    r3 = rand();

    // Test: Numbers should be different
    TEST("rand_diff_1_2", r1 != r2);
    TEST("rand_diff_2_3", r2 != r3);

    // Test: Reproducible with same seed
    srand(12345);
    TEST("rand_repro", rand() == r1);
}

void main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();
    setScreenOn();

    textPrintAt(0, 0, "=== Math Tests ===");

    tests_passed = 0;
    tests_failed = 0;

    test_fixed_point();
    test_fix_mul();
    test_trig();
    test_fixAbs();
    test_random();

    textPrintAt(0, 26, "--------------------");
    if (tests_failed == 0) {
        textPrintAt(0, 27, "ALL TESTS PASSED!");
    } else {
        textPrintAt(0, 27, "SOME TESTS FAILED");
    }

    while (1) {
        WaitForVBlank();
    }
}
