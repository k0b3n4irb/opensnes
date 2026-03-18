/**
 * @file main.c
 * @brief HDMA-driven per-scanline backdrop color gradient
 * @ingroup examples
 *
 * Uses HDMA to rewrite CGRAM color 0 (the backdrop color) on every scanline
 * group, creating a smooth vertical color gradient behind the background
 * layer. HDMA channel 6 is configured in mode 3 (2REG_2X), which writes
 * 4 bytes per entry to consecutive register pairs: two bytes to CGADD
 * ($2121) to select color index 0, then two bytes to CGDATA ($2122) to
 * write the 15-bit BGR color value. A pre-built HDMA table in ROM defines
 * the gradient colors for each scanline group. This technique was widely
 * used in SNES games for sky gradients, underwater tinting, and lava glow
 * effects without consuming any CPU time.
 *
 * @par SNES Concepts
 * - HDMA mode 3 (2REG_2X) targeting CGADD + CGDATA
 * - Per-scanline CGRAM color modification (color 0 = backdrop)
 * - HDMA table format: [count] [CGADD_lo] [CGADD_hi] [color_lo] [color_hi]
 * - Runtime HDMA enable/disable toggling
 *
 * @par What to Observe
 * - A vertical color gradient covers the screen behind the background tiles
 * - Press A to disable the gradient (flat backdrop color returns)
 * - Press B to re-enable the gradient effect
 *
 * @par Modules Used
 * console, dma, background, sprite, hdma, input, math
 *
 * @see hdma.h, background.h, input.h, video.h
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
