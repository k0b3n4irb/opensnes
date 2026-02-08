/*
 * hello_world - Ported from PVSnesLib to OpenSNES
 *
 * First real test of the OpenSNES text module.
 * Displays text strings using textInit/textLoadFont/textPrintAt.
 */

#include <snes.h>

int main(void) {
    /* Initialize hardware */
    consoleInit();

    /* Set Mode 0 (4 BG layers, all 2bpp — matches built-in font) */
    setMode(BG_MODE0, 0);

    /* Initialize text system (default: tilemap at word $3800, font tile 0) */
    textInit();

    /* Load built-in font to VRAM word address $0000 */
    textLoadFont(0x0000);

    /* Configure BG1 to match text module layout */
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* Set palette: color 1 = white on color 0 = black (default after consoleInit) */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    /* Enable BG1 on main screen */
    REG_TM = TM_BG1;

    /* Draw text */
    textPrintAt(10, 10, "Hello World !");
    textPrintAt(6, 14, "WELCOME TO OPENSNES");
    textPrintAt(8, 18, "OPENSNES PROJECT");
    textFlush();        /* Request DMA to VRAM */
    WaitForVBlank();    /* Wait for DMA to complete before screen on */

    /* Turn on screen */
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
