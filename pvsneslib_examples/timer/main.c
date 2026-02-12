/*
 * timer - Ported from PVSnesLib to OpenSNES
 *
 * Displays a VBlank frame counter that increments each frame.
 * PVSnesLib original used consoleDrawText("COUNTER=%u", snes_vblank_count).
 */

#include <snes.h>

int main(void) {
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

    /* White text color */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    /* Enable BG1 */
    REG_TM = TM_BG1;

    /* Static title */
    textPrintAt(9, 8, "JUST COUNT VBL");
    textFlush();
    WaitForVBlank();

    /* Turn on screen */
    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* Update counter display each frame.
         * Clear the number area first to avoid ghost characters
         * when the digit count changes (e.g. 99 -> 100). */
        textPrintAt(10, 10, "COUNTER=");
        textPrintU16(getFrameCount());
        textPrint("     ");
        textFlush();
    }

    return 0;
}
