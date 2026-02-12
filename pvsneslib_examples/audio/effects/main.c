/**
 * @file main.c
 * @brief SNESMOD Sound Effects Demo
 *
 * Port of PVSnesLib "effects" example to OpenSNES.
 * Demonstrates loading and playing 5 sound effects from an
 * Impulse Tracker file using SNESMOD.
 *
 * Controls:
 *   A          - Play current effect
 *   Left/Right - Navigate effects (0-4)
 *
 * Effects:
 *   0: Tada
 *   1: Hall Strings
 *   2: Honky Tonk Piano
 *   3: Marimba 1
 *   4: Cowbell
 *
 * PVSnesLib API mapping:
 *   spcBoot()          -> snesmodInit()
 *   spcSetBank()       -> snesmodSetSoundbank(SOUNDBANK_BANK)
 *   spcStop()          -> snesmodStop()
 *   spcLoadEffect(i)   -> snesmodLoadEffect(i)
 *   spcEffect(p,i,vp)  -> snesmodPlayEffect(i, vol, pan, pitch)
 *   spcProcess()       -> snesmodProcess()
 *
 * License: CC0 (Public Domain)
 */

#include <snes.h>
#include <snes/snesmod.h>
#include "soundbank.h"

#define NUM_EFFECTS 5

static u8 sfx_index = 0;
static u8 key_a = 0;
static u8 key_l = 0;
static u8 key_r = 0;

static void show_effect_name(void) {
    WaitForVBlank();
    textSetPos(7, 14);
    switch (sfx_index) {
    case 0: textPrint("Effect: Tada            "); break;
    case 1: textPrint("Effect: Hall Strings    "); break;
    case 2: textPrint("Effect: Honky Tonk Piano"); break;
    case 3: textPrint("Effect: Marimba 1       "); break;
    case 4: textPrint("Effect: Cowbell         "); break;
    }
    textFlush();
}

int main(void) {
    u8 i;

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

    /* Initialize SNESMOD and set soundbank */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);

    /* Load all 5 effects */
    snesmodStop();
    for (i = 0; i < NUM_EFFECTS; i++) {
        snesmodLoadEffect(i);
    }

    /* Display instructions */
    textSetPos(5, 10);
    textPrint("Press A to play effect!");
    textFlush();
    WaitForVBlank();

    textSetPos(5, 11);
    textPrint("Press L and R to change!");
    textFlush();
    WaitForVBlank();

    /* Show initial effect name */
    show_effect_name();

    setScreenOn();

    while (1) {
        u16 pad0 = padHeld(0);

        /* A = play current effect at 32kHz, full volume, center pan */
        if (pad0 & KEY_A) {
            if (!key_a) {
                key_a = 1;
                snesmodPlayEffect(sfx_index, 127, 128, SNESMOD_PITCH_NORMAL);
            }
        } else {
            key_a = 0;
        }

        /* LEFT = previous effect */
        if (pad0 & KEY_LEFT) {
            if (!key_l) {
                key_l = 1;
                if (sfx_index > 0) {
                    sfx_index--;
                    show_effect_name();
                }
            }
        } else {
            key_l = 0;
        }

        /* RIGHT = next effect */
        if (pad0 & KEY_RIGHT) {
            if (!key_r) {
                key_r = 1;
                if (sfx_index < NUM_EFFECTS - 1) {
                    sfx_index++;
                    show_effect_name();
                }
            }
        } else {
            key_r = 0;
        }

        snesmodProcess();
        WaitForVBlank();
    }
    return 0;
}
