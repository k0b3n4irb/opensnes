/**
 * @file main.c
 * @brief TADA Sound Example - Press A to play sound
 *
 * Uses OpenSNES library for initialization and input handling.
 */

#include <snes.h>

/* Assembly functions for SPC audio */
extern void spc_init(void);
extern void spc_play(void);
extern void vram_init_text(void);

int main(void) {
    u16 pad;
    u16 pad_prev;
    u16 pad_pressed;

    /* Initialize console (sets up hardware) */
    consoleInit();

    /* Force blank during setup */
    REG_INIDISP = 0x80;

    /* Mode 0: 4 BG layers, all 2bpp */
    setMode(BG_MODE0, 0);

    /* BG1 tilemap at $0400 (word address $0200), 32x32 */
    REG_BG1SC = 0x04;

    /* BG1 chr at $0000 */
    REG_BG12NBA = 0x00;

    /* Set up palette: color 0 = dark blue (bg), color 3 = white (text) */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;  /* Color 0: dark blue */
    REG_CGDATA = 0x40;
    REG_CGADD = 3;
    REG_CGDATA = 0xFF;  /* Color 3: white */
    REG_CGDATA = 0x7F;

    /* Initialize SPC and upload driver + sample */
    /* Disable NMI during upload - timing-critical on accurate emulators */
    REG_NMITIMEN = 0x00;
    spc_init();
    REG_NMITIMEN = 0x81;  /* Re-enable NMI + auto-joypad */

    /* Upload font and "TADA" text to VRAM */
    vram_init_text();

    /* Enable BG1 */
    REG_TM = 0x01;

    /* Screen on, full brightness */
    setScreenOn();

    /* Initialize pad_prev */
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}
    pad_prev = REG_JOY1L | (REG_JOY1H << 8);

    /* Main loop */
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
            spc_play();
        }
    }

    return 0;
}
