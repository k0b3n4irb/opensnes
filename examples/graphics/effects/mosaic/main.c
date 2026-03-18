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

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

void WaitForKey(void) {
    while (padHeld(0) != 0) WaitForVBlank();
    while (padHeld(0) == 0) WaitForVBlank();
}

void doFadeOut(void) {
    u8 i;
    for (i = 15; i > 0; i--) {
        setBrightness(i);
        WaitForVBlank();
        WaitForVBlank();
    }
    setBrightness(0);
}

void doFadeIn(void) {
    u8 i;
    for (i = 0; i <= 15; i++) {
        setBrightness(i);
        WaitForVBlank();
        WaitForVBlank();
    }
}

void doMosaicOut(void) {
    mosaicEnable(MOSAIC_BG1);
    mosaicFadeOut(3);
}

void doMosaicIn(void) {
    mosaicFadeIn(3);
    mosaicDisable();
}

int main(void) {
    consoleInit();

    bgInitTileSet(0, tiles, palette, 0, tiles_end - tiles, palette_end - palette, BG_16COLORS, 0x4000);

    bgSetMapPtr(0, 0x1000, BG_MAP_32x32);
    dmaCopyVram(tilemap, 0x1000, tilemap_end - tilemap);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    setScreenOn();

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
