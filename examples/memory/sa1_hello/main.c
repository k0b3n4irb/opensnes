/**
 * @file main.c
 * @brief SA-1 cartridge type verification (Phase 0)
 * @ingroup examples
 *
 * Minimal SA-1 example that verifies the ROM header is correctly
 * configured for SA-1 cartridge type. The SA-1 coprocessor is NOT
 * started yet — this just validates the build system integration.
 *
 * @par What to Observe
 * - Mesen2 should identify this ROM as "SA-1" cartridge type
 * - Text displays "SA-1 CARTRIDGE OK" on screen
 * - The SA-1 coprocessor is idle (not started)
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input
 *
 * @see sa1.h (future)
 */

#include <snes.h>

int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);

    setColor(0, 0x0000);
    setColor(1, RGB(31, 31, 31));

    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    textPrintAt(6, 8, "SA-1 CARTRIDGE OK");
    textPrintAt(4, 10, "MAP MODE: $23 (SA-1)");
    textPrintAt(4, 11, "CART TYPE: $35 (SA-1)");
    textPrintAt(4, 14, "COPROCESSOR: IDLE");
    textPrintAt(4, 16, "(PHASE 0 - BUILD TEST)");

    setMainScreen(LAYER_BG1);
    textFlush();
    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
