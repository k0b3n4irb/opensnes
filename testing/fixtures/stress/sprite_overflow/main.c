/* luna stress probe — PPU sprite-per-scanline overflow (STAT77 range/time-over).
 * 40 small (8x8) sprites all on scanline 100: >32 sprites AND >34 slivers on the
 * line, so STAT77 ($213E) bit6 (range over) and bit7 (time over) must set.
 * Captures STAT77 for several rendered frames for a luna/Mesen2 diff.
 * Éprouve luna's OBJ evaluation AND the SDK sprite/oamSet + NMI-OAM-DMA path. */
#include <snes.h>

#define STAT77 (*(volatile u8 *)0x213E)
u8 stat77_res[8];

int main(void)
{
    u16 i;
    consoleInit();
    setMode(BG_MODE1, 0);
    oamInit(OBJ_SIZE8_L16, 1);           /* small = 8x8 */
    for (i = 0; i < 40; i++) {           /* 40 sprites, all on line 100 */
        oamSet(i, (u16)(i * 4), 100, 0x0000, 0, 3, 0);
        oamSetSize((u8)i, OBJ_SMALL);
    }
    setScreenOn();

    for (i = 0; i < 8; i++) {
        WaitForVBlank();                 /* flags from the just-rendered frame */
        stat77_res[i] = STAT77;
    }
    while (1) { }
    return 0;
}
