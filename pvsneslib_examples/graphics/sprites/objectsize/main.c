/*
 * objectsize - Ported from PVSnesLib to OpenSNES
 *
 * Object size demo
 * -- alekmaul (original PVSnesLib example)
 *
 * Demonstrates all 6 OBJ size combinations on the SNES PPU.
 * Use Up/Down to select, two sprites are shown: small (left) and large (right).
 *
 * VRAM Layout:
 *   $2000+ = Sprite tiles (OBSEL base, small at $2100, large at $2500)
 *   $3000  = Font tiles (2bpp, for BG3)
 *   $6800  = BG3 tilemap (text)
 *
 * Note: Text uses BG3 (2bpp in Mode 1) to match our 2bpp font.
 * PVSnesLib uses BG1 with internal 2bpp->4bpp conversion.
 */

#include <snes.h>

extern u8 sprite8[], sprite8_end[], palsprite8[];
extern u8 sprite16[], sprite16_end[], palsprite16[];
extern u8 sprite32[], sprite32_end[], palsprite32[];
extern u8 sprite64[], sprite64_end[], palsprite64[];

u16 selectedItem;
u8 keyPressed;
u16 pad0;

#define ADRSPRITE       0x2100
#define ADRSPRITLARGE   0x2500

#define PALETTESPRSIZE  (16 * 2)

void draw(void) {
    textPrintAt(3, 2, "OBJECT SIZE :");
    textPrintAt(3, 3, "  SMALL:  8 - LARGE: 16");
    textPrintAt(3, 4, "  SMALL:  8 - LARGE: 32");
    textPrintAt(3, 5, "  SMALL:  8 - LARGE: 64");
    textPrintAt(3, 6, "  SMALL: 16 - LARGE: 32");
    textPrintAt(3, 7, "  SMALL: 16 - LARGE: 64");
    textPrintAt(3, 8, "  SMALL: 32 - LARGE: 64");

    /* Draw selection indicator */
    textPrintAt(3, 3, " ");
    textPrintAt(3, 4, " ");
    textPrintAt(3, 5, " ");
    textPrintAt(3, 6, " ");
    textPrintAt(3, 7, " ");
    textPrintAt(3, 8, " ");
    textPrintAt(3, 3 + selectedItem, ">");

    textFlush();
    WaitForVBlank();
}

void changeObjSize(void) {
    /* Wait for VBlank to ensure we have maximum time for VRAM DMAs.
     * With the PVSnesLib-aligned NMI handler, OAM DMA happens first
     * (~4.5K cycles), then our callback runs, then this code executes
     * with the remaining VBlank time for sprite tile DMAs. */
    WaitForVBlank();

    /* --- DMA small sprite tiles to VRAM --- */
    if (selectedItem <= 2) {
        dmaCopyVram(sprite8, ADRSPRITE, sprite8_end - sprite8);
    } else if (selectedItem <= 4) {
        dmaCopyVram(sprite16, ADRSPRITE, sprite16_end - sprite16);
    } else {
        dmaCopyVram(sprite32, ADRSPRITE, sprite32_end - sprite32);
    }

    /* --- DMA large sprite tiles to VRAM --- */
    if (selectedItem == 0) {
        dmaCopyVram(sprite16, ADRSPRITLARGE, sprite16_end - sprite16);
    } else if (selectedItem == 1 || selectedItem == 3) {
        dmaCopyVram(sprite32, ADRSPRITLARGE, sprite32_end - sprite32);
    } else {
        dmaCopyVram(sprite64, ADRSPRITLARGE, sprite64_end - sprite64);
    }

    /* --- Palettes + OBJ size (CGRAM DMA also safe during VBlank) --- */
    if (selectedItem == 0) {
        dmaCopyCGram(palsprite8, 128, PALETTESPRSIZE);
        dmaCopyCGram(palsprite16, 128 + 1 * 16, PALETTESPRSIZE);
        oamInitEx(OBJ_SIZE8_L16, 1);
    } else if (selectedItem == 1) {
        dmaCopyCGram(palsprite8, 128, PALETTESPRSIZE);
        dmaCopyCGram(palsprite32, 128 + 1 * 16, PALETTESPRSIZE);
        oamInitEx(OBJ_SIZE8_L32, 1);
    } else if (selectedItem == 2) {
        dmaCopyCGram(palsprite8, 128, PALETTESPRSIZE);
        dmaCopyCGram(palsprite64, 128 + 1 * 16, PALETTESPRSIZE);
        oamInitEx(OBJ_SIZE8_L64, 1);
    } else if (selectedItem == 3) {
        dmaCopyCGram(palsprite16, 128, PALETTESPRSIZE);
        dmaCopyCGram(palsprite32, 128 + 1 * 16, PALETTESPRSIZE);
        oamInitEx(OBJ_SIZE16_L32, 1);
    } else if (selectedItem == 4) {
        dmaCopyCGram(palsprite16, 128, PALETTESPRSIZE);
        dmaCopyCGram(palsprite64, 128 + 1 * 16, PALETTESPRSIZE);
        oamInitEx(OBJ_SIZE16_L64, 1);
    } else {
        dmaCopyCGram(palsprite32, 128, PALETTESPRSIZE);
        dmaCopyCGram(palsprite64, 128 + 1 * 16, PALETTESPRSIZE);
        oamInitEx(OBJ_SIZE32_L64, 1);
    }

    oamSet(0, 70, 120, 0x0010, 0, 3, 0);
    oamSetEx(0, OBJ_SMALL, OBJ_SHOW);
    oamSet(1, 170, 120, 0x0050, 1, 3, 0);
    oamSetEx(1, OBJ_LARGE, OBJ_SHOW);
}

int main(void) {
    consoleInit();

    /* Text on BG3 (2bpp in Mode 1 -- matches our 2bpp font)
     * textInitEx params: byte_addr for tilemap, font_tile offset, palette
     * VRAM $6800 word = $D000 byte address -> textInitEx divides by 2
     */
    textInitEx(0xD000, 0, 0);
    textLoadFont(0x3000);
    bgSetGfxPtr(2, 0x3000);     /* BG3 char base = $3000 */
    bgSetMapPtr(2, 0x6800, BG_MAP_32x32);  /* BG3 tilemap at $6800 */

    /* Backdrop color = magenta (CGRAM 0), text = black (CGRAM 1 is default 0) */
    REG_CGADD = 0;
    REG_CGDATA = 0x1F;  /* Magenta: R=31, G=0, B=31 -> $7C1F */
    REG_CGDATA = 0x7C;

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG3 | TM_OBJ;  /* BG3 for text + sprites */

    selectedItem = 0;
    keyPressed = 0;
    draw();
    changeObjSize();

    setScreenOn();

    while (1) {
        pad0 = padHeld(0);

        if (pad0) {
            if (pad0 & KEY_UP) {
                if (selectedItem > 0 && !keyPressed) {
                    selectedItem--;
                    draw();
                    changeObjSize();
                    keyPressed = 1;
                }
            }
            if (pad0 & KEY_DOWN) {
                if (selectedItem < 5 && !keyPressed) {
                    selectedItem++;
                    draw();
                    changeObjSize();
                    keyPressed = 1;
                }
            }
        } else {
            keyPressed = 0;
        }

        WaitForVBlank();
    }

    return 0;
}
