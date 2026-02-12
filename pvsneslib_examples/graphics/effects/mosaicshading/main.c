#include <snes.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

void WaitForKey(void) {
    // Wait for any button release first (avoid immediate trigger)
    while (padHeld(0) != 0) WaitForVBlank();
    // Wait for any button press
    while (padHeld(0) == 0) WaitForVBlank();
}

void fadeOut(void) {
    u8 i;
    for (i = 15; i > 0; i--) {
        setBrightness(i);
        WaitForVBlank();
        WaitForVBlank();
    }
    setBrightness(0);
}

void fadeIn(void) {
    u8 i;
    for (i = 0; i <= 15; i++) {
        setBrightness(i);
        WaitForVBlank();
        WaitForVBlank();
    }
}

int main(void) {
    consoleInit();

    // BG1 tiles at VRAM $4000, palette
    bgInitTileSet(0, tiles, palette, 0, tiles_end - tiles, palette_end - palette, BG_16COLORS, 0x4000);

    // BG1 tilemap at VRAM $1000
    bgSetMapPtr(0, 0x1000, BG_MAP_32x32);
    dmaCopyVram(tilemap, 0x1000, tilemap_end - tilemap);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    setScreenOn();

    WaitForKey();

    while (1) {
        fadeOut();
        WaitForVBlank();
        WaitForKey();

        fadeIn();
        WaitForVBlank();
        WaitForKey();

        mosaicEnable(MOSAIC_BG1);
        mosaicFadeOut(2);
        WaitForVBlank();
        WaitForKey();

        mosaicFadeIn(2);
        mosaicDisable();
        WaitForVBlank();
        WaitForKey();
    }

    return 0;
}
