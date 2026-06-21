/**
 * @file test_signed_ops.c
 * @brief Regression test for signed arithmetic
 *
 * Tests that signed operations generate correct code.
 * The 65816 doesn't have separate signed instructions,
 * so the compiler must handle sign extension correctly.
 */

typedef signed char s8;
typedef signed short s16;
typedef unsigned char u8;
typedef unsigned short u16;

s8 signed_byte = -50;
s16 signed_word = -1000;

// Test sign extension from s8 to s16
s16 extend_signed_byte(s8 val) {
    return val;  // Must sign-extend, not zero-extend
}

// Test signed comparison
u8 signed_compare(s16 a, s16 b) {
    if (a < b) return 1;  // Signed comparison
    return 0;
}

// Test signed division
s16 signed_divide(s16 a, s16 b) {
    return a / b;  // Must handle negative numbers
}

// Test signed right shift (arithmetic shift)
s16 signed_shift_right(s16 val, u8 count) {
    return val >> count;  // Must preserve sign bit
}

// Test negative literal
s16 use_negative_literal(void) {
    s16 result = -100;
    result = result - 50;  // Should be -150
    return result;
}

// Test mixed signed/unsigned
s16 mixed_ops(s8 signed_val, u8 unsigned_val) {
    return signed_val + unsigned_val;  // Promotion rules
}
