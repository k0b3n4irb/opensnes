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
 * console, dma, background, sprite, hdma, input
 *
 * @see hdma.h, background.h, input.h, video.h
 */

#include <snes.h>
#include <snes/hdma.h>

/** @brief 4bpp tile data for the background layer. */
extern u8 tiles[], tiles_end[];
/** @brief Tilemap data (32x32 tile grid) for the background layer. */
extern u8 tilemap[], tilemap_end[];
/** @brief 15-bit BGR palette for the background (up to 16 colors). */
extern u8 palette[], palette_end[];

/**
 * @brief Pre-built HDMA table for the backdrop color gradient (defined in data.asm).
 *
 * This table is consumed by HDMA channel 6 in mode 3 (2REG_2X), targeting
 * CGADD ($2121). Each entry is 5 bytes:
 * - Byte 0: scanline count (with bit 7 = repeat flag)
 * - Bytes 1-2: written to CGADD (always 0x0000 to select color index 0)
 * - Bytes 3-4: written to CGDATA (15-bit BGR color for this scanline group)
 *
 * The table ends with a 0x00 terminator byte. HDMA processes this table
 * automatically every frame without CPU intervention, rewriting the backdrop
 * color at each scanline group boundary to produce a smooth vertical gradient.
 */
extern u8 hdmaGradientList[];

u8 pada;    /**< Edge-detection flag for A button (prevents repeat triggers while held) */
u8 padb;    /**< Edge-detection flag for B button */
u16 pad0;   /**< Current joypad button state */

/**
 * @brief Enable the HDMA backdrop color gradient effect.
 *
 * Configures HDMA channel 6 in mode 3 (2REG_2X) targeting CGADD ($2121).
 * In this mode, HDMA writes 4 bytes per scanline to two consecutive register
 * pairs: the first two bytes go to $2121 (CGADD, selecting palette index 0),
 * and the next two bytes go to $2122 (CGDATA, writing the 15-bit BGR color).
 *
 * The net effect: on every scanline group boundary, CGRAM color 0 (the
 * backdrop/transparent color) is overwritten with a new gradient color.
 * Since CGRAM persists until the next write, each color covers its group
 * of scanlines, producing a vertical gradient behind all BG layers.
 *
 * This technique costs zero CPU time -- HDMA runs entirely via DMA hardware.
 */
void enableGradient(void) {
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_2REG_2X, HDMA_DEST_CGADD, hdmaGradientList);
    hdmaEnable(1 << HDMA_CHANNEL_6);
}

/**
 * @brief Disable the HDMA gradient and restore a flat backdrop color.
 *
 * Stops all HDMA channels. After stopping, CGRAM color 0 retains the last
 * value written by HDMA (typically the color at the bottom of the gradient).
 * The next VBlank's palette reload (if any) would restore the original color.
 */
void disableGradient(void) {
    hdmaDisableAll();
}

/**
 * @brief Entry point -- HDMA-driven per-scanline backdrop color gradient.
 *
 * Loads a background and enables an HDMA-driven vertical color gradient on
 * CGRAM color 0 (the backdrop). The A button disables the gradient (returning
 * to a flat backdrop), and the B button re-enables it. Edge-detection flags
 * (pada/padb) prevent repeated toggles while a button is held down.
 *
 * @return Does not return (infinite loop).
 */
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
    setMainScreen(TM_BG1);

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
