/**
 * @file main.c
 * @brief SNESMOD Music with Pause/Resume Demo
 *
 * Port of PVSnesLib "music2" example to OpenSNES.
 * Demonstrates music playback with pause and resume controls.
 *
 * Controls:
 *   A - Pause music
 *   B - Resume music
 *
 * PVSnesLib API mapping:
 *   spcBoot()          -> snesmodInit()
 *   spcSetBank()       -> snesmodSetSoundbank(SOUNDBANK_BANK)
 *   spcLoad(MOD_X)     -> snesmodLoadModule(MOD_X)
 *   spcPlay(0)         -> snesmodPlay(0)
 *   spcPauseMusic()    -> snesmodPause()
 *   spcResumeMusic()   -> snesmodResume()
 *   spcProcess()       -> snesmodProcess()
 *
 * License: CC0 (Public Domain)
 */

#include <snes.h>
#include <snes/snesmod.h>
#include "soundbank.h"

static u16 bgcolor = 0;
static u8 key_a = 0;
static u8 key_b = 0;

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

    /* Display instructions */
    textSetPos(5, 10);
    textPrint("Lets the music play!");
    textFlush();
    WaitForVBlank();

    textSetPos(5, 12);
    textPrint("     A to PAUSE");
    textFlush();
    WaitForVBlank();

    textSetPos(5, 13);
    textPrint("    B to RESUME");
    textFlush();

    setScreenOn();

    /* Start music playback from the beginning */
    snesmodPlay(0);

    while (1) {
        u16 pad0 = padHeld(0);

        /* A = pause music */
        if (pad0 & KEY_A) {
            if (!key_a) {
                key_a = 1;
                snesmodPause();
            }
        } else {
            key_a = 0;
        }

        /* B = resume music */
        if (pad0 & KEY_B) {
            if (!key_b) {
                key_b = 1;
                snesmodResume();
            }
        } else {
            key_b = 0;
        }

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
