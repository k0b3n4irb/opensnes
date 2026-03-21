/**
 * @file main.c
 * @brief Large soundbank music playback (>32KB, multi-bank LoROM)
 * @ingroup examples
 *
 * Plays "What Is Love" — a 108KB IT module that produces a 56KB soundbank,
 * split across 2 ROM banks by smconv. The SNESMOD library handles bank
 * crossing transparently.
 *
 * Ported from PVSnesLib "musicGreaterThan32k" example.
 *
 * @par SNES Concepts
 * - Large soundbanks (>32KB) automatically split across ROM banks
 * - SNESMOD incptr macro handles bank boundary crossing during playback
 * - Single snesmodSetSoundbank() call — multi-bank is transparent
 *
 * @par What to Observe
 * - Music plays immediately on boot
 * - A=play, B=stop, X=pause/resume
 * - Verify no audio glitches at bank crossings
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

    consoleInit();
    setMode(BG_MODE0, 0);

    setColor(0, 0x2800);
    setColor(1, RGB(31, 31, 31));

    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    textPrintAt(3, 2, "SNESMOD LARGE MUSIC TEST");
    textPrintAt(3, 4, "SOUNDBANK > 32KB (2 BANKS)");
    textPrintAt(5, 7, "A - PLAY");
    textPrintAt(5, 8, "B - STOP");
    textPrintAt(5, 9, "X - PAUSE/RESUME");
    textPrintAt(5, 12, "NOW PLAYING:");
    textPrintAt(5, 13, "WHAT IS LOVE (108KB IT)");

    setMainScreen(LAYER_BG1);
    textFlush();
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
