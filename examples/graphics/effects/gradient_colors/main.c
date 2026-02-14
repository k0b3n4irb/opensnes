/*
 * gradientcolors - Ported from PVSnesLib to OpenSNES
 *
 * Simple gradient color effect in mode 1
 * -- alekmaul (original PVSnesLib example)
 *
 * Demonstrates HDMA color gradient effect on a background.
 * Press A to remove the gradient, B to re-enable it.
 *
 * API differences from PVSnesLib:
 *  - setModeHdmaColor(table) -> hdmaSetup(ch, MODE_2REG_2X, CGADD, table)
 *  - setModeHdmaReset(0)     -> hdmaDisableAll()
 *  - padsCurrent(0)          -> padHeld(0)
 *  - bgInitMapSet()          -> dmaCopyVram()
 *  - bgSetDisable()          -> REG_TM
 *
 * The gradient works by changing CGRAM color 0 (backdrop) via HDMA.
 * HDMA mode 3 writes 4 bytes/entry: CGADD(x2) + CGDATA(x2) = set
 * a 15-bit SNES color per scanline group. This is the same technique
 * PVSnesLib uses in setModeHdmaColor (HDMA Channel 6).
 */

#include <snes.h>
#include <snes/hdma.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];
extern u8 hdmaGradientList[];

u8 pada, padb;
u16 pad0;

void enableGradient(void) {
    /* Set up HDMA to write CGRAM color 0 (backdrop) directly per scanline.
     * Uses mode 3 (2REG_2X): 4 bytes → $2121, $2121, $2122, $2122
     * Each entry: [count] [0x00] [0x00] [color_lo] [color_hi]
     * This sets CGADD=0 then writes a 15-bit SNES color to CGDATA. */
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_2REG_2X, HDMA_DEST_CGADD, hdmaGradientList);
    hdmaEnable(1 << HDMA_CHANNEL_6);
}

void disableGradient(void) {
    hdmaDisableAll();
}

int main(void) {
    consoleInit();

    /* Load BG1 tiles and palette */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x4000);

    /* Load tilemap */
    bgSetMapPtr(0, 0x0000, BG_MAP_32x32);
    dmaCopyVram(tilemap, 0x0000, tilemap_end - tilemap);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;

    setScreenOn();

    /* Initialize gradient effect */
    enableGradient();
    pada = 0;
    padb = 0;

    while (1) {
        pad0 = padHeld(0);

        /* Press A to remove gradient */
        if (pad0 & KEY_A) {
            if (!pada) {
                pada = 1;
                disableGradient();
            }
        } else {
            pada = 0;
        }

        /* Press B to restore gradient */
        if (pad0 & KEY_B) {
            if (!padb) {
                padb = 1;
                enableGradient();
            }
        } else {
            padb = 0;
        }

        WaitForVBlank();
    }

    return 0;
}
