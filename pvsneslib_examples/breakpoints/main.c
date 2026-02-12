/*
 * breakpoints - Ported from PVSnesLib to OpenSNES
 *
 * Demonstrates the WDM breakpoint instruction for Mesen debugging.
 * The 65816 CPU has an unused opcode called WDM which functions as a NOP.
 * Mesen can be told to break on it.
 *
 * In Mesen: Debug > Debugger (Ctrl+D), check "Break on WDM".
 *
 * -- jeffythedragonslayer (original PVSnesLib example)
 */

#include <snes.h>

/* Defined in wdm.asm — emits WDM $00 instruction */
extern void consoleMesenBreakpoint(void);

int main(void) {
    /* Initialize hardware */
    consoleMesenBreakpoint();
    consoleInit();

    /* Configure text system */
    consoleMesenBreakpoint();
    textInit();
    textLoadFont(0x0000);

    /* Configure BG1 */
    consoleMesenBreakpoint();
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* Set Mode 0 and enable BG1 */
    consoleMesenBreakpoint();
    setMode(BG_MODE0, 0);

    /* White text color */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    REG_TM = TM_BG1;

    /* Draw instructions */
    consoleMesenBreakpoint();
    textPrintAt(10, 10, "BREAKPOINTS!");
    textPrintAt(6, 14, "PRESS CTRL+D TO OPEN");
    textPrintAt(7, 15, "THE MESEN DEBUGGER.");
    textPrintAt(4, 18, "MAKE SURE 'BREAK ON...'");
    textPrintAt(8, 19, "WDM IS CHECKED");
    textFlush();
    WaitForVBlank();

    setScreenOn();

    while (1) {
        consoleMesenBreakpoint();
        WaitForVBlank();
    }

    return 0;
}
