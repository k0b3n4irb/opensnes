/**
 * Test: multiplication code generation
 *
 * Verifies that the compiler generates correct code for multiply operations.
 * Special cases (*1, *2, *4, *8, *16, *32) should be inline shifts.
 * Common small constants (*3, *5, *6, *7, *9, *10) should be inline shift+add.
 * Other constants fall through to __mul16 runtime (stack-based calling convention).
 */

typedef unsigned short u16;
typedef unsigned char u8;

/* Multiply by special-case constants (inline shifts) */
u16 mul_by_1(u16 x) { return x * 1; }
u16 mul_by_2(u16 x) { return x * 2; }
u16 mul_by_4(u16 x) { return x * 4; }
u16 mul_by_8(u16 x) { return x * 8; }
u16 mul_by_16(u16 x) { return x * 16; }
u16 mul_by_32(u16 x) { return x * 32; }

/* Multiply by common small constants (inline shift+add) */
u16 mul_by_3(u16 x) { return x * 3; }
u16 mul_by_5(u16 x) { return x * 5; }
u16 mul_by_6(u16 x) { return x * 6; }
u16 mul_by_7(u16 x) { return x * 7; }
u16 mul_by_9(u16 x) { return x * 9; }
u16 mul_by_10(u16 x) { return x * 10; }

/* Multiply by variable (must use __mul16) */
u16 mul_var(u16 x, u16 y) { return x * y; }

/* Multiply by non-special constant (inline shift+add) */
u16 mul_by_13(u16 x) { return x * 13; }

/* Power-of-2 multiply (should be inline shifts, NOT __mul16) */
u16 mul_by_64(u16 x) { return x * 64; }
u16 mul_by_128(u16 x) { return x * 128; }
u16 mul_by_256(u16 x) { return x * 256; }
u16 mul_by_1024(u16 x) { return x * 1024; }
u16 mul_by_2048(u16 x) { return x * 2048; }

/* Composite constant multiply: base * 2^k, base in [3..15] */
u16 mul_by_24(u16 x) { return x * 24; }    /* 3 * 8 */
u16 mul_by_48(u16 x) { return x * 48; }    /* 3 * 16 */
u16 mul_by_20(u16 x) { return x * 20; }    /* 5 * 4 */
u16 mul_by_40(u16 x) { return x * 40; }    /* 5 * 8 */
u16 mul_by_36(u16 x) { return x * 36; }    /* 9 * 4 */
u16 mul_by_60(u16 x) { return x * 60; }    /* 15 * 4 */
u16 mul_by_96(u16 x) { return x * 96; }    /* 3 * 32 */

/* Power-of-2 divide (should be inline shifts, NOT __div16) */
u16 div_by_32(u16 x) { return x / 32; }
u16 div_by_64(u16 x) { return x / 64; }
u16 div_by_1024(u16 x) { return x / 1024; }

/* Power-of-2 modulo (should be inline AND, NOT __mod16) */
u16 mod_by_32(u16 x) { return x % 32; }
u16 mod_by_64(u16 x) { return x % 64; }
u16 mod_by_1024(u16 x) { return x % 1024; }

int main(void) {
    u16 a = 7;
    u16 b = 3;
    return mul_by_3(a) + mul_var(a, b);
}
