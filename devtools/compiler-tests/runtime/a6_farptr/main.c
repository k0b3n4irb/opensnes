/*
 * A6 far-pointer runtime fixture — deref of a pointer to data OUTSIDE bank $00.
 *
 * a6_sentinel lives in BANK 2 (see a6_sentinel.asm). A *volatile* pointer forces
 * a register-indirect deref (the compiler can't fold it to a direct symbol
 * access), exercising the `lda.l $0000,x` path that ignores the bank byte.
 *
 * Expected once A6 lowers derefs as 24-bit: r_d0/3/7 = 0x11/0x44/0x88.
 * r_ptrhi isolates storage vs deref: it must be 0x0002 (the pointer VALUE keeps
 * the bank byte) even if the deref is still buggy.
 */
#include <snes.h>

extern const u8 a6_sentinel[];
volatile const u8 *g_p;   /* volatile → opaque far pointer, register-indirect deref */

u8  r_d0;     /* g_p[0] -> 0x11 */
u8  r_d3;     /* g_p[3] -> 0x44 */
u8  r_d7;     /* g_p[7] -> 0x88 */
u16 r_ptrhi;  /* high 16 of the pointer value -> 0x0002 (bank byte present) */

int main(void) {
    g_p = a6_sentinel;
    r_d0 = g_p[0];
    r_d3 = g_p[3];
    r_d7 = g_p[7];
    r_ptrhi = (u16)(((u32)g_p) >> 16);

    consoleInit();
    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
