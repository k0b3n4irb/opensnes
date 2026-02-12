/**
 * @file main.c
 * @brief SNESMOD Music Playback Demo
 *
 * Port of PVSnesLib "music" example to OpenSNES.
 * Demonstrates simple music playback with auto-loop using SNESMOD.
 * The background color cycles continuously while music plays.
 *
 * PVSnesLib API mapping:
 *   spcBoot()       -> snesmodInit()
 *   spcSetBank()    -> snesmodSetSoundbank(SOUNDBANK_BANK)
 *   spcLoad(MOD_X)  -> snesmodLoadModule(MOD_X)
 *   spcPlay(0)      -> snesmodPlay(0)
 *   spcProcess()    -> snesmodProcess()
 *
 * License: CC0 (Public Domain)
 */

#include <snes.h>
#include <snes/snesmod.h>
#include "soundbank.h"

static u16 bgcolor = 0;

int main(void) {
    consoleInit();

    /* Setup text display */
    textInit();
    textLoadFont(0x3000);
    bgSetGfxPtr(0, 0x3000);
    bgSetMapPtr(0, 0x6800, BG_MAP_32x32);
    /* Set palette color 1 to white for text */
    REG_CGADD = 0x01;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;

    /* Initialize SNESMOD and load music */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodLoadModule(MOD_POLLEN8);

    /* Display text */
    textSetPos(5, 10);
    textPrint("Lets the music play!");
    textFlush();

    setScreenOn();

    /* Start music playback from the beginning */
    snesmodPlay(0);

    while (1) {
        snesmodProcess();
        WaitForVBlank();

        /* Cycle background color each frame */
        bgcolor++;
        /* Cycle backdrop color */
        REG_CGADD = 0x00;
        REG_CGDATA = bgcolor & 0xFF;
        REG_CGDATA = (bgcolor >> 8) & 0x7F;
    }
    return 0;
}
