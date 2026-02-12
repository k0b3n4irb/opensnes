/*
 * memory_mapping - Ported from PVSnesLib to OpenSNES
 *
 * Memory Mapping and Speed setting
 * -- alekmaul (original PVSnesLib example)
 * Feature added by DigiDwrf
 *
 * Demonstrates HiROM-FastROM ROM configuration.
 *
 * API differences from PVSnesLib:
 *  - consoleInitText()    -> textInit() + textLoadFont()
 *  - consoleDrawText()    -> textPrintAt()
 *  - bgSetDisable()       -> REG_TM
 */

#include <snes.h>

int main(void) {
    consoleInit();

    /* Mode 0 (2bpp) matches our built-in font */
    setMode(BG_MODE0, 0);

    /* Initialize text system and load built-in font */
    textInit();
    textLoadFont(0x0000);

    /* Configure BG1 */
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* White text on black background */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    REG_TM = TM_BG1;

    /* Draw text — faithful to PVSnesLib original */
    textPrintAt(4, 13, "This is a HiROM-FastROM");
    textPrintAt(10, 15, "mapped ROM!");

    textFlush();
    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
