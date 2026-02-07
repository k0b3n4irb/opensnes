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

/* Multiply by non-special constant (must use __mul16) */
u16 mul_by_13(u16 x) { return x * 13; }

int main(void) {
    u16 a = 7;
    u16 b = 3;
    return mul_by_3(a) + mul_var(a, b);
}
