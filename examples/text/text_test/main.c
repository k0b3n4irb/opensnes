/* Text Module Test — Crash isolation STEP 4
 *
 * Steps 2-3 confirmed working (textInit, textLoadFont, bgSet* OK)
 * Now testing: textPrintAt + textFlush
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
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x28;
    REG_CGADD = 1;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;
    REG_TM = TM_BG1;

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
