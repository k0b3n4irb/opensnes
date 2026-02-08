/*
 * typeconsole - Ported from PVSnesLib to OpenSNES
 *
 * Tests isPAL() for region detection.
 * Displays whether the console is PAL (50Hz) or NTSC (60Hz).
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

    /* Clear tilemap */
    textClear();

    /* White text color */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    /* Enable BG1 */
    REG_TM = TM_BG1;

    /* Draw text */
    textPrintAt(10, 10, "Hello World !");

    if (isPAL()) {
        textPrintAt(5, 14, "YOU USE A PAL CONSOLE");
    } else {
        textPrintAt(5, 14, "YOU USE A NTSC CONSOLE");
    }
    textFlush();        /* Request DMA to VRAM */
    WaitForVBlank();    /* Wait for DMA to complete before screen on */

    /* Turn on screen */
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
