/**
 * @file main.c
 * @brief Text Module Test
 *
 * Tests the text module: textInit, textLoadFont, textPrintAt, textFlush.
 *
 * Expected: dark blue screen with "TEXT MODULE TEST" in white
 * If dark blue only (no text): textPrintAt or textFlush bug
 * If black: crash in textPrintAt or textFlush
 */

#include <snes.h>

int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);

    /* Palette: color 0 = dark blue, color 1 = white */
    setColor(0, 0x2800);
    setColor(1, RGB(31, 31, 31));
    setMainScreen(LAYER_BG1);

    /* STEP 2: textInit (confirmed working) */
    textInit();

    /* STEP 3: font + BG pointers */
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* STEP 4: textPrintAt + textFlush */
    textPrintAt(8, 14, "TEXT MODULE TEST");
    textFlush();

    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
