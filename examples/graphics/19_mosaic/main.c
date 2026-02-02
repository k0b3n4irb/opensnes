/**
 * @file main.c
 * @brief Mosaic Effect Demo
 *
 * Demonstrates the SNES mosaic effect on a checkerboard pattern.
 *
 * Controls:
 *   Up/Down - Adjust mosaic size
 *   A       - Auto-cycle mode
 *
 * @author OpenSNES Team
 */

#include <snes.h>

/* Simple tiles: empty and solid */
static const u8 tiles[] = {
    /* Tile 0: Empty (blue) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 1: Solid (white) */
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
};

int main(void) {
    u16 i;
    u8 size;
    u8 auto_mode;
    u8 direction;
    u8 delay;
    u16 pad;
    u16 pad_prev;
    u16 pad_pressed;

    /* Initialize console */
    consoleInit();

    /* Set Mode 0 */
    setMode(BG_MODE0, 0);

    /* Configure BG1 */
    REG_BG1SC = 0x04;
    REG_BG12NBA = 0x00;

    /* Load tiles */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;
    for (i = 0; i < sizeof(tiles); i += 2) {
        REG_VMDATAL = tiles[i];
        REG_VMDATAH = tiles[i + 1];
    }

    /* Palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x50;  /* Blue */
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;  /* White */

    /* Checkerboard tilemap */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = ((i + (i / 32)) & 1);
        REG_VMDATAH = 0;
    }

    /* Enable BG1 */
    REG_TM = TM_BG1;

    /* Initialize mosaic */
    mosaicInit();
    mosaicEnable(MOSAIC_BG1);

    /* Turn on screen */
    setScreenOn();

    /* State */
    size = 0;
    auto_mode = 1;
    direction = 1;
    delay = 0;

    /* Initialize input - wait for auto-read and get initial state */
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}
    pad_prev = REG_JOY1L | (REG_JOY1H << 8);

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Read pad directly (like calculator example - more reliable) */
        while (REG_HVBJOY & 0x01) {}
        pad = REG_JOY1L | (REG_JOY1H << 8);
        pad_pressed = pad & ~pad_prev;
        pad_prev = pad;

        /* Skip if controller disconnected */
        if (pad == 0xFFFF) continue;

        /* A toggles auto mode */
        if (pad_pressed & KEY_A) {
            auto_mode = !auto_mode;
        }

        /* Manual control */
        if (!auto_mode) {
            if (pad_pressed & KEY_UP) {
                if (size < 15) size++;
                mosaicSetSize(size);
            }
            if (pad_pressed & KEY_DOWN) {
                if (size > 0) size--;
                mosaicSetSize(size);
            }
        }

        /* Auto cycle */
        if (auto_mode) {
            delay++;
            if (delay >= 4) {
                delay = 0;
                if (direction) {
                    size++;
                    if (size >= 15) direction = 0;
                } else {
                    size--;
                    if (size == 0) direction = 1;
                }
                mosaicSetSize(size);
            }
        }
    }

    return 0;
}
