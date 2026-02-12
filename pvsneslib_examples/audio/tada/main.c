/**
 * @file main.c
 * @brief BRR Sample Playback Demo
 *
 * Port of PVSnesLib "tada" example to OpenSNES.
 * Demonstrates loading and playing a BRR sound sample using the
 * OpenSNES audio system (not SNESMOD).
 *
 * Press A to play the tada sound effect and change background color.
 *
 * PVSnesLib API mapping:
 *   spcBoot()            -> audioInit()
 *   spcAllocateSoundRegion() -> (handled internally)
 *   spcSetSoundEntry()   -> audioLoadSample()
 *   spcPlaySound()       -> audioPlaySample()
 *   spcProcess()         -> audioUpdate()
 *
 * License: CC0 (Public Domain)
 */

#include <snes.h>
#include <snes/audio.h>

extern u8 tada_brr[];
extern u8 tada_brr_end[];

static u16 bgcolor = 128;
static u8 key_debounce = 0;

int main(void) {
    consoleInit();

    /* Initialize audio system (uploads SPC700 driver) */
    audioInit();

    /* Setup text display using library text module */
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

    /* Load BRR sample into slot 0, no loop */
    audioLoadSample(0, tada_brr, tada_brr_end - tada_brr, 0);

    /* Display instructions */
    textSetPos(5, 10);
    textPrint("Press A to play effect!");
    textFlush();

    setScreenOn();

    while (1) {
        u16 pad0 = padHeld(0);

        /* Play effect on A press (with debounce) */
        if (pad0 & KEY_A) {
            if (!key_debounce) {
                key_debounce = 1;

                /* Play the tada sample */
                audioPlaySample(0);

                /* Change background color */
                bgcolor += 16;
                /* Set backdrop color */
                REG_CGADD = 0x00;
                REG_CGDATA = bgcolor & 0xFF;
                REG_CGDATA = (bgcolor >> 8) & 0x7F;
            }
        } else {
            key_debounce = 0;
        }

        audioUpdate();
        WaitForVBlank();
    }
    return 0;
}
