/**
 * @file main.c
 * @brief Fixture: the region the ROM declares and the region it runs on.
 *
 * Two different things: the country byte of the header ($FFD9, set at build
 * time by `ROM_REGION=ntsc|pal|jp`) and the video standard of the console
 * the ROM runs on (PPU STAT78 bit 4, read once by consoleInit() and returned
 * by isPAL()). The fixture records both so a manifest can assert each, and
 * assert that they are independent: a ROM marked NTSC started on a PAL
 * console answers isPAL() == 1.
 */
#include <snes.h>

/** @brief the header's country byte, read back from the ROM image ($00:FFD9) */
u16 r_header_country;
/** @brief isPAL() once consoleInit() has run: 1 PAL (50 Hz), 0 NTSC */
u16 r_ispal;
/** @brief marker: the run reached its end */
u16 r_done;

int main(void) {
    consoleInit();
    r_header_country = *(volatile u8 *)0xFFD9;
    r_ispal = isPAL();
    r_done = 0xBEEF;
    setScreenOn();
    for (;;) {
        WaitForVBlank();
    }
    return 0;
}
