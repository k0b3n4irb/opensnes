/*
 * scoring - Ported from PVSnesLib to OpenSNES
 *
 * Simple scoring demo
 * -- alekmaul (original PVSnesLib example)
 *
 * Demonstrates the scoring module: scoMemory struct with two u16 fields
 * (scohi/scolo). scolo carries into scohi at 10000 (0x2710).
 * Total score = scohi * 10000 + scolo.
 */

#include <snes.h>

scoMemory scoretst;
scoMemory scoretst1;

int main(void) {
    /* Initialize hardware */
    consoleInit();
    setMode(BG_MODE0, 0);

    /* Initialize text system and load font */
    textInit();
    textLoadFont(0x0000);

    /* Configure BG1 */
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* White text color */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    REG_TM = TM_BG1;

    /* --- Test 1: Clear the score --- */
    scoretst.scohi = 18;
    scoretst.scolo = 900;
    scoreClear(&scoretst);

    /* Display: should be 0000 / 0000 */
    textPrintAt(2, 8, "SCORE HI ");
    textPrintHex(scoretst.scohi, 4);
    textPrint(" SCORE LO ");
    textPrintHex(scoretst.scolo, 4);

    /* --- Test 2: Add 0x4DB (=1243) --- */
    scoreAdd(&scoretst, 0x4DB);

    textPrintAt(2, 9, "SCORE HI ");
    textPrintHex(scoretst.scohi, 4);
    textPrint(" SCORE LO ");
    textPrintHex(scoretst.scolo, 4);

    /* --- Test 3: Add 0x2710 (=10000), causes carry --- */
    scoreAdd(&scoretst, 0x2710);

    textPrintAt(2, 10, "SCORE HI ");
    textPrintHex(scoretst.scohi, 4);
    textPrint(" SCORE LO ");
    textPrintHex(scoretst.scolo, 4);

    /* --- Test 4: Compare scores --- */

    /* 4a: scoretst(hi=18) > scoretst1(hi=17) -> higher */
    scoretst.scohi = 18;
    scoretst.scolo = 900;
    scoretst1.scohi = 17;
    scoretst1.scolo = 900;
    if (scoreCmp(&scoretst, &scoretst1) == 0)
        textPrintAt(2, 12, "1 SCORES EQUALS");
    else if (scoreCmp(&scoretst, &scoretst1) == 0xFF)
        textPrintAt(2, 12, "1 SCORETST HIGHER");
    else
        textPrintAt(2, 12, "1 SCORETST LOWER");

    /* 4b: scoretst(lo=900) < scoretst1(lo=901) -> lower */
    scoretst1.scohi = 18;
    scoretst1.scolo = 901;
    if (scoreCmp(&scoretst, &scoretst1) == 0)
        textPrintAt(2, 13, "2 SCORES EQUALS");
    else if (scoreCmp(&scoretst, &scoretst1) == 0xFF)
        textPrintAt(2, 13, "2 SCORETST HIGHER");
    else
        textPrintAt(2, 13, "2 SCORETST LOWER");

    /* 4c: compare with itself -> equal */
    if (scoreCmp(&scoretst, &scoretst) == 0)
        textPrintAt(2, 14, "3 SCORES EQUALS");
    else if (scoreCmp(&scoretst, &scoretst) == 0xFF)
        textPrintAt(2, 14, "3 SCORETST HIGHER");
    else
        textPrintAt(2, 14, "3 SCORETST LOWER");

    textFlush();
    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
