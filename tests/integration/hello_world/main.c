/**
 * @file main.c
 * @brief Hello World Integration Test
 *
 * Displays "HELLO WORLD" using the text module.
 * Verifies that the compiler, library, and text rendering work together.
 */

#include <snes.h>

int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);

    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    REG_CGADD = 1;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;
    REG_TM = TM_BG1;

    textPrintAt(10, 14, "HELLO WORLD");
    textFlush();

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
