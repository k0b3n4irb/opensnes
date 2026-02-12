#include <snes.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

int main(void) {
    u16 pad0;
    u16 scrX = 0;
    u16 scrY = 0;
    u8 moved = 1;

    consoleInit();

    // BG2 tiles at VRAM $4000, palette for BG2
    bgInitTileSet(1, tiles, palette, 0, tiles_end - tiles, palette_end - palette, BG_16COLORS, 0x4000);

    // BG2 tilemap at VRAM $1000, 64x64 (512x512 pixels)
    bgSetMapPtr(1, 0x1000, BG_MAP_64x64);
    dmaCopyVram(tilemap, 0x1000, tilemap_end - tilemap);

    // BG1 text setup: font tiles at $3000, tilemap at $6800
    textInit();
    textLoadFont(0x3000);
    bgSetGfxPtr(0, 0x3000);
    bgSetMapPtr(0, 0x6800, BG_MAP_32x32);

    // Set palette color 1 to white for text visibility
    // Set palette color 1 to white for text visibility
    REG_CGADD = 0x01;          // CGRAM address 1
    REG_CGDATA = 0xFF;         // low byte (0x7FFF)
    REG_CGDATA = 0x7F;         // high byte

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1 | TM_BG2;
    setScreenOn();

    while (1) {
        WaitForVBlank();

        pad0 = padHeld(0);
        moved = 0;

        if (pad0 & KEY_RIGHT) {
            scrX += 2;
            moved = 1;
        }
        if (pad0 & KEY_LEFT) {
            scrX -= 2;
            moved = 1;
        }
        if (pad0 & KEY_DOWN) {
            scrY += 2;
            moved = 1;
        }
        if (pad0 & KEY_UP) {
            scrY -= 2;
            moved = 1;
        }

        if (moved) {
            bgSetScroll(1, scrX, scrY);
        }

        textSetPos(0, 0);
        textPrint("SCR X=");
        textPrintU16(scrX);
        textPrint(" Y=");
        textPrintU16(scrY);
        textFlush();
    }

    return 0;
}
