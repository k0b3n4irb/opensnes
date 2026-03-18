/**
 * @file main.c
 * @brief Mosaic and fade screen transition effects
 * @ingroup examples
 *
 * Demonstrates two SNES screen transition techniques: brightness fading and
 * the hardware mosaic filter. Brightness fading uses register $2100 (INIDISP)
 * to ramp the master brightness from 0 (black) to 15 (full) over multiple
 * frames. The mosaic effect uses register $2106 (MOSAIC) to progressively
 * enlarge the pixel blocks on selected BG layers, creating a pixelation
 * effect commonly used in RPG battle transitions.
 *
 * @par SNES Concepts
 * - Master brightness fading via setBrightness() (register $2100)
 * - Hardware mosaic filter via mosaicEnable() / mosaicFadeIn/Out() (register $2106)
 * - Frame-paced transitions using WaitForVBlank() timing
 *
 * @par What to Observe
 * - Press any button to cycle through 4 effects: fade out, fade in,
 *   mosaic out (pixels enlarge), mosaic in (pixels shrink back)
 * - The cycle repeats indefinitely
 *
 * @par Modules Used
 * console, dma, background, sprite, input, mosaic
 *
 * @see video.h, mosaic.h, background.h
 */

#include <snes.h>

/** @brief 4bpp background tile data (defined in data.asm, stored in ROM) */
extern u8 tiles[], tiles_end[];
/** @brief Background tilemap data mapping tiles to the 32x32 grid */
extern u8 tilemap[], tilemap_end[];
/** @brief 16-color palette for the background tiles */
extern u8 palette[], palette_end[];

/**
 * @brief Block until a button is pressed, debouncing any currently held buttons.
 *
 * First waits for all buttons to be released (debounce), then waits for any
 * new button press. Each iteration calls WaitForVBlank() to yield to the NMI
 * handler so the system stays synchronized to the 60Hz refresh rate.
 */
void WaitForKey(void) {
    while (padHeld(0) != 0) WaitForVBlank();
    while (padHeld(0) == 0) WaitForVBlank();
}

/**
 * @brief Fade the screen from full brightness to black over ~30 frames.
 *
 * Decrements the SNES master brightness register (INIDISP, $2100) from 15
 * (full) to 0 (black). Each brightness step lasts 2 frames (~33ms), producing
 * a smooth ~500ms fade-out. The PPU multiplies all pixel colors by
 * (brightness / 15), so brightness 0 = completely black.
 */
void doFadeOut(void) {
    u8 i;
    for (i = 15; i > 0; i--) {
        setBrightness(i);
        WaitForVBlank();
        WaitForVBlank();
    }
    setBrightness(0);
}

/**
 * @brief Fade the screen from black to full brightness over ~32 frames.
 *
 * Increments INIDISP brightness from 0 to 15. Two WaitForVBlank() calls per
 * step create a 2-frame hold at each level, matching doFadeOut() timing for
 * a symmetrical fade cycle.
 */
void doFadeIn(void) {
    u8 i;
    for (i = 0; i <= 15; i++) {
        setBrightness(i);
        WaitForVBlank();
        WaitForVBlank();
    }
}

/**
 * @brief Activate the hardware mosaic filter and animate pixel blocks growing.
 *
 * The SNES mosaic register ($2106) groups NxN pixel blocks and fills each
 * block with the color of its top-left pixel. mosaicFadeOut() increases N
 * from 1 to 16 over time, making the image progressively more pixelated.
 * The speed parameter (3) controls how many frames each mosaic level is held.
 *
 * This effect is commonly used in RPG battle transitions (e.g., Final Fantasy).
 */
void doMosaicOut(void) {
    mosaicEnable(MOSAIC_BG1);
    mosaicFadeOut(3);
}

/**
 * @brief Animate the mosaic filter shrinking back to normal, then disable it.
 *
 * mosaicFadeIn() decreases the mosaic block size from 16 back to 1, restoring
 * the original image. mosaicDisable() then clears the mosaic register so the
 * PPU no longer applies the filter, saving a tiny bit of processing.
 */
void doMosaicIn(void) {
    mosaicFadeIn(3);
    mosaicDisable();
}

/**
 * @brief Entry point: set up a background and cycle through transition effects.
 *
 * Initializes Mode 1 with a single 4bpp background layer, then enters an
 * infinite loop cycling through four effects on each button press:
 * fade out -> fade in -> mosaic out -> mosaic in.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    consoleInit();

    /* Load 4bpp tiles to VRAM $4000 and palette to CGRAM slot 0 */
    bgInitTileSet(0, tiles, palette, 0, tiles_end - tiles, palette_end - palette, BG_16COLORS, 0x4000);

    /* Place the tilemap at VRAM $1000 with a 32x32 tile arrangement */
    bgSetMapPtr(0, 0x1000, BG_MAP_32x32);
    dmaCopyVram(tilemap, 0x1000, tilemap_end - tilemap);

    /* Mode 1 with only BG1 visible on the main screen */
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    setScreenOn();

    /* Wait for initial button press before starting the demo cycle */
    WaitForKey();

    while (1) {
        doFadeOut();
        WaitForVBlank();
        WaitForKey();

        doFadeIn();
        WaitForVBlank();
        WaitForKey();

        doMosaicOut();
        WaitForVBlank();
        WaitForKey();

        doMosaicIn();
        WaitForVBlank();
        WaitForKey();
    }

    return 0;
}
