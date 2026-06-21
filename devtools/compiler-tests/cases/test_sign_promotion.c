// =============================================================================
// Test: Signed/unsigned type promotion
// =============================================================================
// Prevents: Wrong sign extension, incorrect promotion in mixed expressions
//
// C integer promotion rules:
// - u8 in arithmetic -> promoted to int (s16 on 65816... but int=32bit here)
// - s8 in arithmetic -> sign-extended to wider type
// - Mixed signed/unsigned of same rank -> unsigned wins
//
// On 65816, this translates to:
// - Zero-extension: AND #$00FF (u8 -> u16)
// - Sign-extension: check bit 7, OR #$FF00 if set (s8 -> s16)
//
// Uses function parameters to prevent constant folding.
// =============================================================================

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;

u16 result_u16;
s16 result_s16;
u8  result_u8;

// Test: u8 promoted to u16 in addition (prevents overflow)
u16 add_u8_u8(u8 a, u8 b) {
    return (u16)a + (u16)b;
}

// Test: s8 sign-extended before addition with s16
s16 add_s8_s16(s8 a, s16 b) {
    return (s16)a + b;
}

// Test: negate s8 (sign extend then negate)
s16 negate_s8(s8 val) {
    return -val;
}

// Test: zero-extend u8 to u16
u16 zext_u8(u8 val) {
    return (u16)val;
}

// Test: sign-extend s8 to s16
s16 sext_s8(s8 val) {
    return (s16)val;
}

// Test: comparison between s8 values
u8 compare_s8(s8 a, s8 b) {
    if (a < b) {
        return 1;
    }
    return 0;
}

// Test: mixed unsigned/signed arithmetic
s16 mixed_arith(u8 unsigned_a, s8 signed_b) {
    return (s16)unsigned_a + (s16)signed_b;
}

// Test: narrowing from s16 to s8 then widening back
s16 narrow_widen(s16 val) {
    s8 narrow = (s8)val;
    return (s16)narrow;
}

// Test: arithmetic right shift (preserves sign)
s16 arith_shr(s16 val) {
    return val >> 1;
}

// Test: byte extraction from u16
u8 extract_low(u16 val) {
    return (u8)(val & 0xFF);
}

u8 extract_high(u16 val) {
    return (u8)(val >> 8);
}

int main(void) {
    result_u16 = add_u8_u8(200, 100);          // 300 (exceeds u8)
    result_s16 = add_s8_s16(-10, 100);          // 90
    result_s16 = negate_s8(42);                  // -42
    result_u16 = zext_u8(0xFF);                  // 255 (not -1)
    result_s16 = sext_s8(-1);                    // -1 (0xFFFF)
    result_u8  = compare_s8(-5, 5);              // 1 (true)
    result_s16 = mixed_arith(10, -3);            // 7
    result_s16 = narrow_widen(-10);              // -10
    result_s16 = arith_shr(-4);                  // -2
    result_u8  = extract_low(0xABCD);            // 0xCD
    result_u8  = extract_high(0xABCD);           // 0xAB
    return (int)result_u8;
}
