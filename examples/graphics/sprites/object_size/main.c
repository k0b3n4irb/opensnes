/*
 * Object Size Demo
 *
 * Interactive menu to switch between all 6 SNES sprite size modes.
 * UP/DOWN selects the mode, each mode shows a small and large sprite.
 *
 * VRAM layout:
 *   $0000  Font tiles (4bpp, loaded by textLoadFont4bpp)
 *   $3800  BG1 tilemap (text, 32x32)
 *   $4000  Sprite name base (OBJ_BASE = 2)
 *   $4100  Small sprite tiles (tile $10 from name base)
 *   $4500  Large sprite tiles (tile $50 from name base)
 *
 * Based on PVSnesLib example.
 */

#include <snes.h>

extern u8 sprite8[], sprite8_end[];
extern u8 sprite16[], sprite16_end[];
extern u8 sprite32[], sprite32_end[];
extern u8 sprite64[], sprite64_end[];
extern u8 palsprite8[], palsprite8_end[];
extern u8 palsprite16[], palsprite16_end[];
extern u8 palsprite32[], palsprite32_end[];
extern u8 palsprite64[], palsprite64_end[];

#define ADRSPRITE       0x4100
#define ADRSPRITLARGE   0x4500
#define PALETTESPRSIZE  (16 * 2)

/* Tile numbers relative to name base $4000 */
#define TILE_SMALL  0x0010   /* ($4100 - $4000) / 16 */
#define TILE_LARGE  0x0050   /* ($4500 - $4000) / 16 */

u16 selectedItem;

static void drawCursor(void) {
    u8 i;
    for (i = 0; i < 6; i++) {
        if (i == (u8)selectedItem) {
            textPrintAt(3, 3 + i, ">");
        } else {
            textPrintAt(3, 3 + i, " ");
        }
    }
    textFlush();
}

static void changeObjSize(void) {
    /* Force blank for unlimited VRAM write time.
     * Large sprites (32x32, 64x64) exceed the ~4KB VBlank DMA budget,
     * so we must disable rendering during the transfer. */
    WaitForVBlank();
    REG_INIDISP = 0x80;

    if (selectedItem == 0) {
        oamInitGfxSet(sprite8, sprite8_end - sprite8,
                      palsprite8, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE8_L16);
        dmaCopyVram(sprite16, ADRSPRITLARGE, sprite16_end - sprite16);
        dmaCopyCGram(palsprite16, 128 + 16, PALETTESPRSIZE);
    } else if (selectedItem == 1) {
        oamInitGfxSet(sprite8, sprite8_end - sprite8,
                      palsprite8, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE8_L32);
        dmaCopyVram(sprite32, ADRSPRITLARGE, sprite32_end - sprite32);
        dmaCopyCGram(palsprite32, 128 + 16, PALETTESPRSIZE);
    } else if (selectedItem == 2) {
        oamInitGfxSet(sprite8, sprite8_end - sprite8,
                      palsprite8, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE8_L64);
        dmaCopyVram(sprite64, ADRSPRITLARGE, sprite64_end - sprite64);
        dmaCopyCGram(palsprite64, 128 + 16, PALETTESPRSIZE);
    } else if (selectedItem == 3) {
        oamInitGfxSet(sprite16, sprite16_end - sprite16,
                      palsprite16, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE16_L32);
        dmaCopyVram(sprite32, ADRSPRITLARGE, sprite32_end - sprite32);
        dmaCopyCGram(palsprite32, 128 + 16, PALETTESPRSIZE);
    } else if (selectedItem == 4) {
        oamInitGfxSet(sprite16, sprite16_end - sprite16,
                      palsprite16, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE16_L64);
        dmaCopyVram(sprite64, ADRSPRITLARGE, sprite64_end - sprite64);
        dmaCopyCGram(palsprite64, 128 + 16, PALETTESPRSIZE);
    } else if (selectedItem == 5) {
        oamInitGfxSet(sprite32, sprite32_end - sprite32,
                      palsprite32, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE32_L64);
        dmaCopyVram(sprite64, ADRSPRITLARGE, sprite64_end - sprite64);
        dmaCopyCGram(palsprite64, 128 + 16, PALETTESPRSIZE);
    }

    /* Small sprite on the left */
    oamSet(0, 70, 120, TILE_SMALL, 0, 3, 0);
    oamSetEx(0, OBJ_SMALL, OBJ_SHOW);

    /* Large sprite on the right */
    oamSet(1, 170, 120, TILE_LARGE, 1, 3, 0);
    oamSetEx(1, OBJ_LARGE, OBJ_SHOW);

    /* Restore display */
    REG_INIDISP = 0x0F;
}

int main(void) {
    u16 pad;

    consoleInit();
    setMode(BG_MODE1, 0);

    /* Text system on BG1 (4bpp in Mode 1)
     * textInit() defaults: tilemap at $3800, font_tile=0, palette=0 */
    textInit();
    textLoadFont4bpp(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, SC_32x32);

    /* Font palette: color 0 = black, color 1 = white */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    REG_CGADD = 1;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;

    /* Draw static menu text */
    textPrintAt(3, 2, "Object size :");
    textPrintAt(5, 3, "Small:  8 - Large: 16");
    textPrintAt(5, 4, "Small:  8 - Large: 32");
    textPrintAt(5, 5, "Small:  8 - Large: 64");
    textPrintAt(5, 6, "Small: 16 - Large: 32");
    textPrintAt(5, 7, "Small: 16 - Large: 64");
    textPrintAt(5, 8, "Small: 32 - Large: 64");

    selectedItem = 0;
    drawCursor();
    changeObjSize();

    REG_TM = TM_BG1 | TM_OBJ;
    setScreenOn();

    while (1) {
        pad = padPressed(0);

        if (pad & KEY_UP) {
            if (selectedItem > 0) {
                selectedItem--;
                drawCursor();
                changeObjSize();
            }
        }
        if (pad & KEY_DOWN) {
            if (selectedItem < 5) {
                selectedItem++;
                drawCursor();
                changeObjSize();
            }
        }

        WaitForVBlank();
    }
    return 0;
}
