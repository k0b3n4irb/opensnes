/**
 * HDMA Wave Demo
 *
 * Uses HDMA to create animated wave distortion on vertical stripes.
 * Demonstrates water reflection / heat shimmer effects.
 *
 * Controls:
 *   A: Toggle HDMA effect on/off
 *   LEFT/RIGHT: Change wave amplitude (4 levels)
 *   UP: Start animation
 *   DOWN: Stop animation (freeze)
 */
#include <snes.h>
#include <snes/console.h>
#include <snes/hdma.h>

/* Simple tiles for visual pattern */
static const u8 tiles[] = {
    /* Tile 0: empty */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 1: solid */
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
};

/* Amplitude levels */
static const u8 ampLevels[] = { 2, 4, 8, 16 };

/* State */
static u16 pad;
static u16 pad_old;
static u8 amplitude;
static u8 hdma_enabled;
static u8 animating;

int main(void) {
    u16 i;

    /* Initialize state */
    pad = 0;
    pad_old = 0;
    amplitude = 1;      /* Start at level 1 (4 pixels) */
    hdma_enabled = 1;
    animating = 1;

    consoleInit();
    setMode(BG_MODE0, 0);

    /* Setup BG1 tilemap at VRAM $0400 (word address) */
    REG_BG1SC = 0x04;
    REG_BG12NBA = 0x00;

    /* Upload tiles to VRAM $0000 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;
    for (i = 0; i < sizeof(tiles); i += 2) {
        REG_VMDATAL = tiles[i];
        REG_VMDATAH = tiles[i + 1];
    }

    /* Setup palette: color 0 = dark blue, color 1 = cyan */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x50;  /* Dark blue */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* Cyan */

    /* Fill tilemap with vertical stripes */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = ((i % 32) & 1);
        REG_VMDATAH = 0;
    }

    /* Enable BG1 on main screen */
    REG_TM = TM_BG1;

    /* Initialize HDMA wave */
    hdmaWaveInit();
    hdmaWaveH(HDMA_CHANNEL_6, 1, ampLevels[amplitude], 2);
    hdmaWaveSetSpeed(2);
    hdmaEnable(1 << HDMA_CHANNEL_6);

    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* Read joypad */
        while (REG_HVBJOY & 0x01) {}
        pad = *(volatile u16*)0x4218;

        /* A: Toggle HDMA on/off */
        if ((pad & KEY_A) && !(pad_old & KEY_A)) {
            hdma_enabled = !hdma_enabled;
            if (hdma_enabled) {
                hdmaWaveH(HDMA_CHANNEL_6, 1, ampLevels[amplitude], 2);
                hdmaWaveSetSpeed(2);
                hdmaEnable(1 << HDMA_CHANNEL_6);
            } else {
                hdmaWaveStop();
                REG_BG1HOFS = 0;
                REG_BG1HOFS = 0;
            }
        }

        /* LEFT: Decrease amplitude */
        if ((pad & KEY_LEFT) && !(pad_old & KEY_LEFT)) {
            if (amplitude > 0) {
                amplitude--;
                if (hdma_enabled) {
                    hdmaWaveH(HDMA_CHANNEL_6, 1, ampLevels[amplitude], 2);
                    hdmaEnable(1 << HDMA_CHANNEL_6);
                }
            }
        }

        /* RIGHT: Increase amplitude */
        if ((pad & KEY_RIGHT) && !(pad_old & KEY_RIGHT)) {
            if (amplitude < 3) {
                amplitude++;
                if (hdma_enabled) {
                    hdmaWaveH(HDMA_CHANNEL_6, 1, ampLevels[amplitude], 2);
                    hdmaEnable(1 << HDMA_CHANNEL_6);
                }
            }
        }

        /* UP: Start animation */
        if ((pad & KEY_UP) && !(pad_old & KEY_UP)) {
            animating = 1;
        }

        /* DOWN: Stop animation */
        if ((pad & KEY_DOWN) && !(pad_old & KEY_DOWN)) {
            animating = 0;
        }

        pad_old = pad;

        /* Update wave animation */
        if (hdma_enabled && animating) {
            hdmaWaveUpdate();
        }
    }

    return 0;
}
