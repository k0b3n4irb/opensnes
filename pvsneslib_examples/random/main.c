/*
 * random - Ported from PVSnesLib to OpenSNES
 *
 * Tests text + input + rand().
 * Press any button to generate a new random number.
 */

#include <snes.h>

int main(void) {
    u16 pad0;

    /* Initialize hardware */
    consoleInit();

    /* Set Mode 0 (2bpp BGs — matches built-in font) */
    setMode(BG_MODE0, 0);

    /* Initialize text system and load font */
    textInit();
    textLoadFont(0x0000);

    /* Configure BG1 to match text layout */
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* Clear tilemap */
    textClear();

    /* White text color */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    /* Enable BG1 */
    REG_TM = TM_BG1;

    /* Draw static text */
    textPrintAt(5, 8, "JUST DO RANDOM NUMBERS");
    textPrintAt(3, 10, "PRESS KEY FOR ANOTHER ONE");
    textPrintAt(6, 12, "RANDOM NUMBER=");
    textSetPos(20, 12);
    textPrintHex(rand(), 4);
    textFlush();        /* Request DMA to VRAM */
    WaitForVBlank();    /* Wait for DMA to complete before screen on */

    /* Turn on screen */
    setScreenOn();

    while (1) {
        /* Wait for a button press */
        do {
            WaitForVBlank();
            pad0 = padHeld(0);
        } while (pad0 == 0);

        /* Update random number — no forced blank needed! */
        textSetPos(20, 12);
        textPrintHex(rand(), 4);
        textFlush();    /* DMA buffer to VRAM next VBlank */
    }

    return 0;
}
