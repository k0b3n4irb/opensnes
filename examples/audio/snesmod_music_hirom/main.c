/**
 * @file main.c
 * @brief HiROM music playback via SNESMOD
 * @ingroup examples
 *
 * Plays "What Is Love" (extended version, 210KB IT module) using HiROM
 * mode. HiROM uses 64KB banks instead of LoROM's 32KB, allowing larger
 * soundbanks with fewer bank crossings.
 *
 * Ported from PVSnesLib "musicHiROM" example.
 *
 * @par SNES Concepts
 * - HiROM: 64KB banks ($0000-$FFFF per bank) vs LoROM 32KB ($8000-$FFFF)
 * - smconv -i flag generates HiROM-compatible addresses
 * - SNESMOD bank crossing works the same — incptr handles it
 *
 * @par What to Observe
 * - Music plays immediately on boot
 * - A=play, B=stop, X=pause/resume
 * - Larger .it file (210KB) than LoROM example (108KB)
 *
 * @par Modules Used
 * console, sprite, dma, input, background, text
 *
 * @see snesmod.h
 */

#include <snes.h>
#include <snes/snesmod.h>
#include <snes/text.h>
#include "soundbank.h"

int main(void) {
    u16 pad;
    u8 paused;

    textModeInit();
    setColor(0, RGB(0, 0, 10));

    textPrintAt(4, 2, "SNESMOD HIROM MUSIC TEST");
    textPrintAt(5, 4, "64KB BANKS (HIROM MODE)");
    textPrintAt(5, 7, "A - PLAY");
    textPrintAt(5, 8, "B - STOP");
    textPrintAt(5, 9, "X - PAUSE/RESUME");
    textPrintAt(5, 12, "NOW PLAYING:");
    textPrintAt(5, 13, "WHAT IS LOVE (210KB IT)");
    WaitForVBlank();

    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodLoadModule(MOD_WHATISLOVE);
    snesmodPlay(0);

    setScreenOn();

    paused = 0;

    while (1) {
        WaitForVBlank();
        snesmodProcess();

        pad = padPressed(0);

        if (pad & KEY_A) {
            snesmodPlay(0);
            paused = 0;
        }
        if (pad & KEY_B) {
            snesmodStop();
        }
        if (pad & KEY_X) {
            if (paused) {
                snesmodResume();
                paused = 0;
            } else {
                snesmodPause();
                paused = 1;
            }
        }
    }

    return 0;
}
