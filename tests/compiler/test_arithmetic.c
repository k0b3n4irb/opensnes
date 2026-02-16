/* Comprehensive arithmetic tests for OpenSNES C compiler
 *
 * Tests runtime functions: __mul16, __div16, __mod16
 * These are called by generated code for operations the 65816 can't do natively.
 *
 * KNOWN ISSUE: The runtime functions may produce incorrect results.
 * The calculator example (examples/basics/calculator) uses a software
 * workaround with repeated addition/subtraction instead of * and /.
 *
 * This file documents the expected vs actual behavior to help trace and fix
 * the underlying compiler/runtime issues.
 */

/* ============================================================================
 * MULTIPLICATION TESTS (__mul16)
 * ============================================================================
 * Uses hardware multiplication via $4202-$4203, result at $4216-$4217
 * Implements 16x16 bit multiplication using 8x8 hardware:
 *   (a_hi*256 + a_lo) * (b_hi*256 + b_lo)
 * ============================================================================ */

/* Simple 8-bit * 8-bit (fits in hardware) */
unsigned short mul_8x8_simple(unsigned short a, unsigned short b) {
    return a * b;  /* Expected: 6 * 7 = 42 */
}

/* 8-bit * 8-bit at boundary */
unsigned short mul_8x8_boundary(unsigned short a, unsigned short b) {
    return a * b;  /* Expected: 255 * 2 = 510 */
}

/* 8-bit * 8-bit overflow to 16-bit */
unsigned short mul_8x8_overflow(unsigned short a, unsigned short b) {
    return a * b;  /* Expected: 255 * 255 = 65025 */
}

/* 16-bit * small number */
unsigned short mul_16x8(unsigned short a, unsigned short b) {
    return a * b;  /* Expected: 1000 * 5 = 5000 */
}

/* Large 16-bit multiplication */
unsigned short mul_16x16_small(unsigned short a, unsigned short b) {
    return a * b;  /* Expected: 100 * 100 = 10000 */
}

/* 16-bit with overflow (low 16 bits only) */
unsigned short mul_16x16_overflow(unsigned short a, unsigned short b) {
    return a * b;  /* Expected: 256 * 256 = 65536 -> 0 (overflow) */
}

/* Powers of 2 (should use shifts, not __mul16) */
unsigned short mul_by_2(unsigned short x) {
    return x * 2;
}

unsigned short mul_by_4(unsigned short x) {
    return x * 4;
}

unsigned short mul_by_8(unsigned short x) {
    return x * 8;
}

unsigned short mul_by_256(unsigned short x) {
    return x * 256;
}

/* ============================================================================
 * DIVISION TESTS (__div16)
 * ============================================================================
 * Uses hardware division for 8-bit divisors ($4204-$4206, result at $4214-$4217)
 * Falls back to software shift-and-subtract for 16-bit divisors.
 * ============================================================================ */

/* Simple division */
unsigned short div_simple(unsigned short a, unsigned short b) {
    return a / b;  /* Expected: 100 / 10 = 10 */
}

/* Division with remainder */
unsigned short div_with_remainder(unsigned short a, unsigned short b) {
    return a / b;  /* Expected: 100 / 7 = 14 */
}

/* Large dividend, small divisor (hardware path) */
unsigned short div_large_small(unsigned short a, unsigned short b) {
    return a / b;  /* Expected: 10000 / 100 = 100 */
}

/* 16-bit divisor (software path) */
unsigned short div_16bit_divisor(unsigned short a, unsigned short b) {
    return a / b;  /* Expected: 50000 / 1000 = 50 */
}

/* Powers of 2 (should use shifts, not __div16) */
unsigned short div_by_2(unsigned short x) {
    return x / 2;
}

unsigned short div_by_4(unsigned short x) {
    return x / 4;
}

unsigned short div_by_8(unsigned short x) {
    return x / 8;
}

unsigned short div_by_256(unsigned short x) {
    return x / 256;
}

/* Non-powers of 2 (must use __div16) */
unsigned short div_by_3(unsigned short x) {
    return x / 3;  /* Expected: 100 / 3 = 33 */
}

unsigned short div_by_7(unsigned short x) {
    return x / 7;  /* Expected: 100 / 7 = 14 */
}

unsigned short div_by_10(unsigned short x) {
    return x / 10; /* Expected: 100 / 10 = 10 */
}

/* ============================================================================
 * MODULO TESTS (__mod16)
 * ============================================================================
 * Calls __div16 and returns the remainder.
 * ============================================================================ */

/* Simple modulo */
unsigned short mod_simple(unsigned short a, unsigned short b) {
    return a % b;  /* Expected: 17 % 5 = 2 */
}

/* Modulo no remainder */
unsigned short mod_no_remainder(unsigned short a, unsigned short b) {
    return a % b;  /* Expected: 100 % 10 = 0 */
}

/* Powers of 2 (should use AND masks) */
unsigned short mod_by_2(unsigned short x) {
    return x % 2;  /* Same as x & 1 */
}

unsigned short mod_by_8(unsigned short x) {
    return x % 8;  /* Same as x & 7 */
}

/* Non-powers of 2 */
unsigned short mod_by_3(unsigned short x) {
    return x % 3;
}

unsigned short mod_by_10(unsigned short x) {
    return x % 10;
}

/* ============================================================================
 * COMBINED/COMPLEX TESTS
 * ============================================================================ */

/* Expression with both multiply and divide */
unsigned short combined_mul_div(unsigned short a, unsigned short b, unsigned short c) {
    return (a * b) / c;  /* Expected: (10 * 20) / 5 = 40 */
}

/* Expression with multiply, divide, modulo */
unsigned short combined_all(unsigned short a, unsigned short b) {
    return (a * b) % 100;  /* Expected: (15 * 7) % 100 = 5 */
}

/* Chained multiplications */
unsigned short chained_mul(unsigned short a, unsigned short b, unsigned short c) {
    return a * b * c;  /* Expected: 5 * 6 * 7 = 210 */
}

/* ============================================================================
 * EDGE CASES
 * ============================================================================ */

/* Multiply by 0 */
unsigned short mul_by_zero(unsigned short x) {
    return x * 0;  /* Expected: 0 */
}

/* Multiply by 1 */
unsigned short mul_by_one(unsigned short x) {
    return x * 1;  /* Expected: x */
}

/* Divide by 1 */
unsigned short div_by_one(unsigned short x) {
    return x / 1;  /* Expected: x */
}

/* Max values */
unsigned short mul_max(void) {
    return 255 * 257;  /* Expected: 65535 */
}

/* ============================================================================
 * TEST DRIVER
 * ============================================================================ */

/* Global to hold test results - prevents optimizer from removing tests */
volatile unsigned short test_result;

/* Counters for pass/fail tracking */
unsigned short tests_passed = 0;
unsigned short tests_failed = 0;

/* Simple test helper */
void check_result(unsigned short expected, unsigned short actual) {
    if (expected == actual) {
        tests_passed++;
    } else {
        tests_failed++;
    }
    test_result = actual;  /* Prevent dead code elimination */
}

int main(void) {
    /* Multiplication tests */
    check_result(42, mul_8x8_simple(6, 7));
    check_result(510, mul_8x8_boundary(255, 2));
    check_result(65025, mul_8x8_overflow(255, 255));
    check_result(5000, mul_16x8(1000, 5));
    check_result(10000, mul_16x16_small(100, 100));
    check_result(0, mul_16x16_overflow(256, 256));

    /* Power-of-2 multiplications */
    check_result(200, mul_by_2(100));
    check_result(400, mul_by_4(100));
    check_result(800, mul_by_8(100));
    check_result(25600, mul_by_256(100));

    /* Division tests */
    check_result(10, div_simple(100, 10));
    check_result(14, div_with_remainder(100, 7));
    check_result(100, div_large_small(10000, 100));
    check_result(50, div_16bit_divisor(50000, 1000));

    /* Power-of-2 divisions */
    check_result(50, div_by_2(100));
    check_result(25, div_by_4(100));
    check_result(12, div_by_8(100));
    check_result(0, div_by_256(100));

    /* Non-power-of-2 divisions */
    check_result(33, div_by_3(100));
    check_result(14, div_by_7(100));
    check_result(10, div_by_10(100));

    /* Modulo tests */
    check_result(2, mod_simple(17, 5));
    check_result(0, mod_no_remainder(100, 10));
    check_result(0, mod_by_2(100));
    check_result(4, mod_by_8(100));
    check_result(1, mod_by_3(100));
    check_result(0, mod_by_10(100));

    /* Combined tests */
    check_result(40, combined_mul_div(10, 20, 5));
    check_result(5, combined_all(15, 7));
    check_result(210, chained_mul(5, 6, 7));

    /* Edge cases */
    check_result(0, mul_by_zero(12345));
    check_result(100, mul_by_one(100));
    check_result(100, div_by_one(100));
    check_result(65535, mul_max());

    /* Return total (for verification) */
    return tests_passed;
}
