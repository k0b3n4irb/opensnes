/*
 * controller - Ported from PVSnesLib to OpenSNES
 *
 * Tests padHeld() with text display for all 12 buttons.
 * Uses switch/case to display which button is currently held.
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
    textPrintAt(12, 1, "PAD TEST");
    textPrintAt(6, 5, "USE PAD TO SEE VALUE");
    textFlush();        /* Request DMA to VRAM */
    WaitForVBlank();    /* Wait for DMA to complete before screen on */

    /* Turn on screen */
    setScreenOn();

    while (1) {
        /* Get current pad state */
        pad0 = padHeld(0);

        /* Update display with current pad */
        switch (pad0) {
        case KEY_A:
            textPrintAt(9, 10, "A PRESSED     ");
            break;
        case KEY_B:
            textPrintAt(9, 10, "B PRESSED     ");
            break;
        case KEY_SELECT:
            textPrintAt(9, 10, "SELECT PRESSED");
            break;
        case KEY_START:
            textPrintAt(9, 10, "START PRESSED ");
            break;
        case KEY_RIGHT:
            textPrintAt(9, 10, "RIGHT PRESSED ");
            break;
        case KEY_LEFT:
            textPrintAt(9, 10, "LEFT PRESSED  ");
            break;
        case KEY_DOWN:
            textPrintAt(9, 10, "DOWN PRESSED  ");
            break;
        case KEY_UP:
            textPrintAt(9, 10, "UP PRESSED    ");
            break;
        case KEY_R:
            textPrintAt(9, 10, "R PRESSED     ");
            break;
        case KEY_L:
            textPrintAt(9, 10, "L PRESSED     ");
            break;
        case KEY_X:
            textPrintAt(9, 10, "X PRESSED     ");
            break;
        case KEY_Y:
            textPrintAt(9, 10, "Y PRESSED     ");
            break;
        default:
            textPrintAt(9, 10, "              ");
            break;
        }
        textFlush();    /* DMA buffer to VRAM next VBlank */

        WaitForVBlank();
    }

    return 0;
}
