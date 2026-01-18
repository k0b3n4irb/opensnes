/**
 * @file test_types.c
 * @brief Unit tests for OpenSNES type definitions
 *
 * Validates that types have correct sizes and behavior on 65816.
 */

#include <snes.h>
#include "../../harness/test_harness.h"
#include "../../harness/test_harness.c"

/*============================================================================
 * Type Size Tests
 *============================================================================*/

void test_u8_range(void) {
    u8 val = 0;
    TEST_ASSERT_EQUAL(0, val);

    val = 255;
    TEST_ASSERT_EQUAL(255, val);

    /* Overflow wraps */
    val = 255;
    val++;
    TEST_ASSERT_EQUAL(0, val);
}

void test_u16_range(void) {
    u16 val = 0;
    TEST_ASSERT_EQUAL(0, val);

    val = 65535;
    TEST_ASSERT_EQUAL(65535, val);

    /* Overflow wraps */
    val++;
    TEST_ASSERT_EQUAL(0, val);
}

void test_s16_range(void) {
    s16 val = 0;
    TEST_ASSERT_EQUAL(0, val);

    val = 32767;
    TEST_ASSERT_EQUAL(32767, val);

    val = -32768;
    TEST_ASSERT_EQUAL(-32768, val);

    /* Negative numbers */
    val = -1;
    TEST_ASSERT_EQUAL(-1, val);
}

void test_s32_range(void) {
    s32 val = 0;
    TEST_ASSERT_EQUAL(0, val);

    /* Large positive */
    val = 100000;
    TEST_ASSERT_EQUAL(100000, val);

    /* Large negative */
    val = -100000;
    TEST_ASSERT_EQUAL(-100000, val);
}

/*============================================================================
 * Arithmetic Tests
 *============================================================================*/

void test_u16_arithmetic(void) {
    u16 a = 100;
    u16 b = 200;

    TEST_ASSERT_EQUAL(300, a + b);
    TEST_ASSERT_EQUAL(100, b - a);
    TEST_ASSERT_EQUAL(20000, a * b);
    TEST_ASSERT_EQUAL(2, b / a);
}

void test_s16_arithmetic(void) {
    s16 a = 100;
    s16 b = -50;

    TEST_ASSERT_EQUAL(50, a + b);
    TEST_ASSERT_EQUAL(150, a - b);
    TEST_ASSERT_EQUAL(-5000, a * b);
    TEST_ASSERT_EQUAL(-2, a / b);
}

void test_s32_arithmetic(void) {
    s32 a = 50000;
    s32 b = 50000;

    /* This would overflow in s16 */
    TEST_ASSERT_EQUAL(100000, a + b);
    TEST_ASSERT_EQUAL(2500000000LL, a * b);
}

/*============================================================================
 * Boolean Tests
 *============================================================================*/

void test_boolean_values(void) {
    bool t = TRUE;
    bool f = FALSE;

    TEST_ASSERT(t);
    TEST_ASSERT(!f);
    TEST_ASSERT_EQUAL(0xFF, TRUE);
    TEST_ASSERT_EQUAL(0x00, FALSE);
}

/*============================================================================
 * Macro Tests
 *============================================================================*/

void test_bit_macro(void) {
    TEST_ASSERT_EQUAL(0x0001, BIT(0));
    TEST_ASSERT_EQUAL(0x0002, BIT(1));
    TEST_ASSERT_EQUAL(0x0080, BIT(7));
    TEST_ASSERT_EQUAL(0x8000, BIT(15));
}

void test_byte_macros(void) {
    u16 val = 0x1234;

    TEST_ASSERT_EQUAL(0x34, LO_BYTE(val));
    TEST_ASSERT_EQUAL(0x12, HI_BYTE(val));
    TEST_ASSERT_EQUAL(0x1234, MAKE_WORD(0x34, 0x12));
}

void test_minmax_macros(void) {
    TEST_ASSERT_EQUAL(5, MIN(5, 10));
    TEST_ASSERT_EQUAL(5, MIN(10, 5));
    TEST_ASSERT_EQUAL(10, MAX(5, 10));
    TEST_ASSERT_EQUAL(10, MAX(10, 5));
    TEST_ASSERT_EQUAL(5, CLAMP(3, 5, 10));
    TEST_ASSERT_EQUAL(7, CLAMP(7, 5, 10));
    TEST_ASSERT_EQUAL(10, CLAMP(15, 5, 10));
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    test_init();

    /* Type size tests */
    RUN_TEST(test_u8_range);
    RUN_TEST(test_u16_range);
    RUN_TEST(test_s16_range);
    RUN_TEST(test_s32_range);

    /* Arithmetic tests */
    RUN_TEST(test_u16_arithmetic);
    RUN_TEST(test_s16_arithmetic);
    RUN_TEST(test_s32_arithmetic);

    /* Boolean tests */
    RUN_TEST(test_boolean_values);

    /* Macro tests */
    RUN_TEST(test_bit_macro);
    RUN_TEST(test_byte_macros);
    RUN_TEST(test_minmax_macros);

    test_report();
    return 0;
}
