/*
 * debug - Ported from PVSnesLib to OpenSNES
 *
 * PVSnesLib original used consoleNocashMessage() to send debug strings
 * to the no$sns emulator's debug console. Since no$sns is discontinued
 * and modern emulators (Mesen, bsnes) don't support that debug port,
 * this port uses on-screen text output instead.
 *
 * For debugging in Mesen, use:
 *   - The built-in debugger (CPU/PPU/APU state)
 *   - Memory viewer / trace logger
 *   - WDM breakpoints (see breakpoints example)
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
    textPrintAt(6, 6, "DEBUG OUTPUT DEMO");
    textPrintAt(2, 9, "ORIGINAL USED NO$SNS PORT");
    textPrintAt(2, 11, "NOW USING ON-SCREEN TEXT");
    textFlush();
    WaitForVBlank();

    /* Turn on screen */
    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* Update counter display each frame */
        textPrintAt(8, 16, "VBL COUNT ");
        textPrintU16(getFrameCount());
        textFlush();
    }

    return 0;
}
