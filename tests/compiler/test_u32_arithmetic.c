// =============================================================================
// Test: 32-bit (unsigned int) arithmetic
// =============================================================================
// Prevents: Incorrect multi-word arithmetic on 16-bit CPU
//
// The 65816 is a 16-bit CPU. 32-bit operations require multi-word
// sequences (two 16-bit operations with carry propagation).
//
// NOTE: On cproc/65816, type sizes are:
//   unsigned char  = 1 byte (u8)
//   unsigned short = 2 bytes (u16)
//   unsigned int   = 4 bytes (u32)  <-- THIS is 32-bit
//   unsigned long  = 8 bytes (u64)  <-- NOT 32-bit!
//
// Uses function parameters to prevent constant folding, ensuring
// the compiler generates actual runtime arithmetic instructions.
// =============================================================================

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

u32 result32;
u16 result16;
u8  result8;

// Test: 32-bit addition (must use ADC for carry propagation)
u32 add32(u32 a, u32 b) {
    return a + b;
}

// Test: 32-bit subtraction (must use SBC for borrow)
u32 sub32(u32 a, u32 b) {
    return a - b;
}

// Test: 32-bit bitwise AND
u32 and32(u32 a, u32 b) {
    return a & b;
}

// Test: 32-bit bitwise OR
u32 or32(u32 a, u32 b) {
    return a | b;
}

// Test: 32-bit bitwise XOR
u32 xor32(u32 a, u32 b) {
    return a ^ b;
}

// Test: 32-bit comparison (must check both words)
u8 compare_gt(u32 a, u32 b) {
    if (a > b) {
        return 1;
    }
    return 0;
}

// Test: 32-bit comparison equal
u8 compare_eq(u32 a, u32 b) {
    if (a == b) {
        return 1;
    }
    return 0;
}

// Test: truncate u32 to u16 (keep low word)
u16 truncate32(u32 val) {
    return (u16)val;
}

// Test: zero-extend u16 to u32
u32 extend16(u16 val) {
    return (u32)val;
}

// Test: left shift by constant
u32 shl_const(u32 val) {
    return val << 8;
}

// Test: right shift by constant
u32 shr_const(u32 val) {
    return val >> 8;
}

// Test: 32-bit increment
u32 inc32(u32 val) {
    return val + 1;
}

int main(void) {
    result32 = add32(0x0000FFFF, 0x00000001);  // carry test
    result32 = sub32(0x00010000, 0x00000001);   // borrow test
    result32 = and32(0x12345678, 0x00FF00FF);
    result32 = or32(0x12000056, 0x00340078);
    result32 = xor32(0xAAAA5555, 0xFFFF0000);
    result8  = compare_gt(0x00020000, 0x00010000);
    result8  = compare_eq(0x12345678, 0x12345678);
    result16 = truncate32(0x12345678);
    result32 = extend16(0xABCD);
    result32 = shl_const(0x00001234);
    result32 = shr_const(0x12340000);
    result32 = inc32(0x0000FFFF);               // carry test
    return (int)result8;
}
