// =============================================================================
// Test: Bitwise operations
// =============================================================================
// Prevents: Incorrect bit manipulation code generation
//
// Bitwise operations are critical for SNES development:
// - Register manipulation
// - Sprite attributes
// - Tile flags
// - Input button masks
// =============================================================================

unsigned char byte_val;
unsigned short word_val;

void test_and(void) {
    byte_val = 0xFF;
    byte_val &= 0x0F;  // byte_val = 0x0F

    word_val = 0xFFFF;
    word_val &= 0x00FF;  // word_val = 0x00FF
}

void test_or(void) {
    byte_val = 0x0F;
    byte_val |= 0xF0;  // byte_val = 0xFF

    word_val = 0x00FF;
    word_val |= 0xFF00;  // word_val = 0xFFFF
}

void test_xor(void) {
    byte_val = 0xAA;
    byte_val ^= 0xFF;  // byte_val = 0x55

    word_val = 0xAAAA;
    word_val ^= 0xFFFF;  // word_val = 0x5555
}

void test_not(void) {
    byte_val = 0xAA;
    byte_val = ~byte_val;  // byte_val = 0x55

    word_val = 0xAAAA;
    word_val = ~word_val;  // word_val = 0x5555
}

void test_shift_left(void) {
    byte_val = 1;
    byte_val <<= 4;  // byte_val = 0x10

    word_val = 1;
    word_val <<= 8;  // word_val = 0x100

    // Shift by variable amount
    unsigned char shift = 3;
    byte_val = 1;
    byte_val <<= shift;  // byte_val = 8
}

void test_shift_right(void) {
    byte_val = 0x80;
    byte_val >>= 4;  // byte_val = 0x08

    word_val = 0x8000;
    word_val >>= 8;  // word_val = 0x80

    // Unsigned shift - no sign extension
    byte_val = 0xFF;
    byte_val >>= 4;  // byte_val = 0x0F (not 0xFF)
}

void test_bit_set_clear(void) {
    // Common SNES pattern: set/clear specific bits
    byte_val = 0;

    // Set bit 3
    byte_val |= (1 << 3);  // byte_val = 0x08

    // Set bit 7
    byte_val |= (1 << 7);  // byte_val = 0x88

    // Clear bit 3
    byte_val &= ~(1 << 3);  // byte_val = 0x80

    // Toggle bit 7
    byte_val ^= (1 << 7);  // byte_val = 0x00
}

void test_bit_test(void) {
    byte_val = 0x42;

    // Test if bit 1 is set
    if (byte_val & (1 << 1)) {
        word_val = 1;  // This should execute
    }

    // Test if bit 3 is set
    if (byte_val & (1 << 3)) {
        word_val = 2;  // This should NOT execute
    }
}

void test_mask_extract(void) {
    // Extract bits 4-7 (high nibble)
    byte_val = 0xAB;
    unsigned char high = (byte_val >> 4) & 0x0F;  // high = 0x0A

    // Extract bits 0-3 (low nibble)
    unsigned char low = byte_val & 0x0F;  // low = 0x0B

    // Combine nibbles (swap)
    byte_val = (low << 4) | high;  // byte_val = 0xBA
}

int main(void) {
    test_and();
    test_or();
    test_xor();
    test_not();
    test_shift_left();
    test_shift_right();
    test_bit_set_clear();
    test_bit_test();
    test_mask_extract();
    return (int)byte_val;
}
