/**
 * @file main.c
 * @brief Sound Effects Demo - Press A to play beep
 *
 * Uses OpenSNES library for initialization and input handling.
 * Screen flashes blue when sound plays.
 */

#include <snes.h>

/* Audio functions from spc.asm */
extern void spc_init(void);
extern void spc_play(void);

int main(void) {
    u16 pad;
    u16 pad_prev;
    u16 pad_pressed;

    /* Initialize console (sets up hardware) */
    consoleInit();

    /* Force blank */
    REG_INIDISP = 0x80;

    /* Disable all layers */
    REG_TM = 0x00;

    /* Set backdrop to GREEN (audio ready) */
    REG_CGADD = 0;
    REG_CGDATA = 0xE0;
    REG_CGDATA = 0x03;

    /* Initialize audio - uploads driver + sample to SPC */
    /* Disable NMI during upload - timing-critical on accurate emulators */
    REG_NMITIMEN = 0x00;
    spc_init();
    REG_NMITIMEN = 0x81;  /* Re-enable NMI + auto-joypad */

    /* Screen on */
    setScreenOn();

    /* Initialize pad_prev */
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}
    pad_prev = REG_JOY1L | (REG_JOY1H << 8);

    /* Main loop - press A to play sound */
    while (1) {
        WaitForVBlank();

        /* Wait for auto-read complete, then read joypad */
        while (REG_HVBJOY & 0x01) {}
        pad = REG_JOY1L | (REG_JOY1H << 8);
        pad_pressed = pad & ~pad_prev;
        pad_prev = pad;

        /* Skip if controller disconnected */
        if (pad == 0xFFFF) pad_pressed = 0;

        /* A button pressed? */
        if (pad_pressed & KEY_A) {
            /* Flash blue */
            REG_CGADD = 0;
            REG_CGDATA = 0x00;
            REG_CGDATA = 0x7C;

            /* Play sound */
            spc_play();
        }
    }

    return 0;
}
