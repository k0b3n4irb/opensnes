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
    return (int)u_result;
}
