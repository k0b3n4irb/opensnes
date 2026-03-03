/**
 * @file main.c
 * @brief Mosaic — Pixelation and Fade Transitions
 *
 * Demonstrates fade and mosaic screen transition effects.
 * Press any button to advance to the next effect step:
 *   1. Fade out (screen goes black)
 *   2. Fade in (screen returns)
 *   3. Mosaic out (pixels get bigger)
 *   4. Mosaic in (pixels return to normal)
 *   (loops)
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
