// =============================================================================
// Test: Comparison operations
// =============================================================================
// Prevents: Incorrect comparison and branching code
//
// Tests signed and unsigned comparisons which generate different
// branch instructions on 65816:
// - Unsigned: BCC (less), BCS (greater or equal)
// - Signed: BMI/BPL or CMP with V flag handling
// =============================================================================

unsigned char u_result;
signed char s_result;

// Unsigned comparisons
void test_unsigned_less(void) {
    unsigned char a = 5;
    unsigned char b = 10;

    if (a < b) {
        u_result = 1;
    } else {
        u_result = 0;
    }
    // u_result = 1
}

void test_unsigned_greater(void) {
    unsigned char a = 10;
    unsigned char b = 5;

    if (a > b) {
        u_result = 1;
    } else {
        u_result = 0;
    }
    // u_result = 1
}

void test_unsigned_equal(void) {
    unsigned char a = 42;
    unsigned char b = 42;

    if (a == b) {
        u_result = 1;
    } else {
        u_result = 0;
    }
    // u_result = 1
}

void test_unsigned_not_equal(void) {
    unsigned char a = 10;
    unsigned char b = 20;

    if (a != b) {
        u_result = 1;
    } else {
        u_result = 0;
    }
    // u_result = 1
}

// Signed comparisons (tricky on 65816!)
void test_signed_less(void) {
    signed char a = -5;
    signed char b = 5;

    if (a < b) {
        s_result = 1;
    } else {
        s_result = 0;
    }
    // s_result = 1 (because -5 < 5 in signed)
}

void test_signed_negative(void) {
    signed char a = -10;
    signed char b = -5;

    if (a < b) {
        s_result = 1;
    } else {
        s_result = 0;
    }
    // s_result = 1 (because -10 < -5)
}

void test_signed_vs_unsigned(void) {
    // This demonstrates why signed/unsigned matters
    unsigned char u = 200;  // 200 unsigned
    signed char s = -56;    // -56 signed (same bit pattern: 0xC8)

    // Unsigned comparison: 200 > 100
    if (u > 100) {
        u_result = 1;
    }

    // Signed comparison: -56 < 100
    if (s < 100) {
        s_result = 1;
    }
}

// Word (16-bit) comparisons
void test_word_compare(void) {
    unsigned short a = 0x1234;
    unsigned short b = 0x5678;

    if (a < b) {
        u_result = 1;
    }

    if (a > b) {
        u_result = 2;
    }
    // u_result = 1
}

// Comparison with zero (optimizable)
void test_zero_compare(void) {
    unsigned char val = 0;

    if (val == 0) {
        u_result = 1;  // BEQ
    }

    val = 5;
    if (val != 0) {
        u_result = 2;  // BNE
    }
}

// Compound comparisons
void test_compound(void) {
    unsigned char x = 50;

    // Range check
    if (x >= 10 && x <= 100) {
        u_result = 1;
    }

    // Or condition
    if (x == 50 || x == 100) {
        u_result = 2;
    }
}

// u16 high-value comparisons (regression: bcc not bmi)
// Use parameters to prevent constant folding — the compiler
// must emit actual comparison instructions at runtime.
void test_u16_high_less(unsigned short a, unsigned short b) {
    // Call with a=50000, b=60000
    if (a < b) u_result = 1;
    else u_result = 0;
    // u_result = 1 (50000 < 60000, unsigned)
}

void test_u16_vs_constant(unsigned short val) {
    // Call with val=50000
    if (val >= 10000) u_result = 1;
    else u_result = 0;
    // u_result = 1
}

void test_u16_shift_right(unsigned short val) {
    // Call with val=0xC000
    unsigned short result = val >> 1;
    // result should be 0x6000 (logical), NOT 0xE000 (arithmetic)
    if (result == 0x6000) u_result = 1;
    else u_result = 0;
}

void test_u16_div(unsigned short a, unsigned short b) {
    // Call with a=50000, b=10
    unsigned short result = a / b;
    // result should be 5000 (unsigned div), NOT some negative-signed result
    if (result == 5000) u_result = 1;
    else u_result = 0;
}

void test_u16_mod(unsigned short a, unsigned short b) {
    // Call with a=50003, b=10
    unsigned short result = a % b;
    // result should be 3 (unsigned mod)
    if (result == 3) u_result = 1;
    else u_result = 0;
}

// Ternary value used as function argument (regression: GVN+fusion bug)
// GVN may replace the ternary's phi with the comparison result directly.
// The comparison+branch fusion must NOT skip the comparison instruction
// when its result is also used as a value (not just by jnz).
extern void use_val(unsigned short v);
unsigned char cflags;

void test_ternary_value_used(void) {
    unsigned short palette;
    palette = (cflags != 0) ? 1 : 0;
    use_val(palette);
}

short test_s16_shift_right(short val) {
    /* Signed right shift by 8: -320 >> 8 should give -2, not 254 */
    return val >> 8;
}

short test_s16_shift_right_1(short val) {
    /* Signed right shift by 1: -4 >> 1 should give -2, not 32766 */
    return val >> 1;
}

int main(void) {
    test_unsigned_less();
    test_unsigned_greater();
    test_unsigned_equal();
    test_unsigned_not_equal();
    test_signed_less();
    test_signed_negative();
    test_signed_vs_unsigned();
    test_word_compare();
    test_zero_compare();
    test_compound();
    test_u16_high_less(50000, 60000);
    test_u16_vs_constant(50000);
    test_u16_shift_right(0xC000);
    test_u16_div(50000, 10);
    test_u16_mod(50003, 10);
    test_s16_shift_right(-320);
    test_s16_shift_right_1(-4);
    return (int)u_result;
}
