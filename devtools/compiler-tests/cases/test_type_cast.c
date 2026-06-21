// =============================================================================
// Test: Type casting
// =============================================================================
// Prevents: Incorrect type conversion code
//
// Tests various type conversions that the 65816 must handle:
// - Widening (u8 -> u16, u16 -> u32)
// - Narrowing (u16 -> u8, u32 -> u16)
// - Signed/unsigned conversions
// - Pointer casts
// =============================================================================

unsigned char u8_val;
unsigned short u16_val;
unsigned long u32_val;
signed char s8_val;
signed short s16_val;

// Widening conversions (no data loss)
void test_widen_unsigned(void) {
    u8_val = 0xAB;

    // u8 -> u16: zero-extend
    u16_val = (unsigned short)u8_val;  // u16_val = 0x00AB

    // u16 -> u32: zero-extend
    u16_val = 0xCDEF;
    u32_val = (unsigned long)u16_val;  // u32_val = 0x0000CDEF
}

void test_widen_signed(void) {
    s8_val = -10;  // 0xF6

    // s8 -> s16: sign-extend
    s16_val = (signed short)s8_val;  // s16_val = 0xFFF6 = -10

    // Positive value sign-extends with zeros
    s8_val = 10;
    s16_val = (signed short)s8_val;  // s16_val = 0x000A = 10
}

// Narrowing conversions (may lose data)
void test_narrow(void) {
    u16_val = 0x1234;

    // u16 -> u8: keep low byte
    u8_val = (unsigned char)u16_val;  // u8_val = 0x34

    u32_val = 0x12345678;

    // u32 -> u16: keep low word
    u16_val = (unsigned short)u32_val;  // u16_val = 0x5678

    // u32 -> u8: keep low byte
    u8_val = (unsigned char)u32_val;  // u8_val = 0x78
}

// Signed <-> unsigned (same size, different interpretation)
void test_sign_cast(void) {
    u8_val = 200;  // 0xC8

    // Reinterpret as signed
    s8_val = (signed char)u8_val;  // s8_val = -56

    s8_val = -10;

    // Reinterpret as unsigned
    u8_val = (unsigned char)s8_val;  // u8_val = 246
}

// Common SNES patterns
void test_snes_patterns(void) {
    // Extract high/low bytes from word
    u16_val = 0xABCD;
    unsigned char lo = (unsigned char)u16_val;         // lo = 0xCD
    unsigned char hi = (unsigned char)(u16_val >> 8);  // hi = 0xAB

    // Combine bytes into word
    u16_val = ((unsigned short)hi << 8) | lo;  // u16_val = 0xABCD

    // Color conversion: 8-bit RGB to 5-bit
    unsigned char r8 = 200;
    unsigned char r5 = (unsigned char)(r8 >> 3);  // r5 = 25
}

// Pointer casts
void test_pointer_cast(void) {
    unsigned char bytes[4] = {0x12, 0x34, 0x56, 0x78};

    // Cast byte pointer to word pointer
    unsigned short *words = (unsigned short*)bytes;
    u16_val = words[0];  // u16_val = 0x3412 (little endian)

    // Cast back
    unsigned char *bp = (unsigned char*)words;
    u8_val = bp[1];  // u8_val = 0x34
}

// Implicit conversions in expressions
void test_implicit(void) {
    u8_val = 100;
    u16_val = 200;

    // u8 is promoted to u16 for comparison
    if (u8_val < u16_val) {
        u32_val = 1;
    }

    // u8 promoted for arithmetic
    u16_val = u8_val + u8_val;  // No overflow, result is u16
}

int main(void) {
    test_widen_unsigned();
    test_widen_signed();
    test_narrow();
    test_sign_cast();
    test_snes_patterns();
    test_pointer_cast();
    test_implicit();
    return (int)u8_val;
}
