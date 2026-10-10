/* A constant shift count is a 16-bit word: `(s16)(3 + 0x20000000UL)` is 3.
 * The front end does not narrow the count and the emitter read the whole
 * constant, 536870915, as "16 or more": the result was 0 (difftest seeds
 * 102026, 102200, 103718; 2026-10-10; w65816/isel.c). */
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;
typedef signed char s8;
extern u16 x;

u16 right3(void) { return x >> ((s16)(((s8)3) + 536870912UL)); }
u16 left0(void) { return x << ((s16)(128U * 65536L)); }
