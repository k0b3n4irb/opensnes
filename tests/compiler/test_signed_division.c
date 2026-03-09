/**
 * @file test_signed_division.c
 * @brief Test signed division and modulo codegen
 *
 * Verifies that signed division calls __sdiv16 (not unsigned __div16).
 */

typedef signed short s16;

volatile s16 a_val;
volatile s16 b_val;

s16 signed_div(s16 a, s16 b) {
    return a / b;
}

s16 signed_mod(s16 a, s16 b) {
    return a % b;
}

unsigned short unsigned_div(unsigned short a, unsigned short b) {
    return a / b;
}

unsigned short unsigned_mod(unsigned short a, unsigned short b) {
    return a % b;
}
