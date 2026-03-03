/**
 * @file main.c
 * @brief Metasprite Demo — Multi-Size Sprite Composition
 *
 * Demonstrates metasprites: large characters composed of multiple
 * hardware sprites. Three OBJ size modes selectable via D-PAD.
 *
 * Mode 0: Small=8x8,  Large=16x16 — hero16 + hero8
 * Mode 1: Small=8x8,  Large=32x32 — hero32 + hero8
 * Mode 2: Small=16x16, Large=32x32 — hero32 + hero16
 */

#include <snes.h>

/* Sprite tile data (from data.asm) */
extern u8 spritehero32_til[];
extern u8 spritehero32_pal[];
extern u8 spritehero16_til[];
extern u8 spritehero8_til[];

/* Metasprite frame definitions */
#include "hero8_meta.inc"
#include "hero16_meta.inc"
#include "hero32_meta.inc"

/*
 * VRAM Layout:
 *   0x0000: hero32 tiles (192 tiles = 3072 words)
 *   0x0C00: hero16 tiles (96 tiles = 1536 words)
 *   0x1200: hero8 tiles  (16 tiles = 256 words)
 *   0x4000: BG1 font tiles (4bpp, 8KB-aligned for bgSetGfxPtr)
 *   0x6800: BG1 text tilemap
 *
 * OBJSEL: base=0x0000, name_select=0 (second page at 0x1000)
 * All sprite tiles addressable as OAM tiles 0-303.
 */
#define VRAM_HERO32   0x0000
#define VRAM_HERO16   0x0C00
#define VRAM_HERO8    0x1200
#define VRAM_FONT     0x4000
#define VRAM_TEXT_MAP 0x6800

#define HERO32_TILES  192
#define HERO16_TILES  96
#define HERO8_TILES   16
#define TILE_BYTES    32

/* Base tile numbers for oamDrawMeta baseTile parameter */
#define BASE_TILE_32  0
#define BASE_TILE_16  (HERO32_TILES)       /* 192 */
#define BASE_TILE_8   (HERO32_TILES + HERO16_TILES)  /* 288 */

u16 selectedItem;

static void drawMenu(void) {
    textClearRect(3, 2, 28, 4);
    textPrintAt(3, 2, "Object size :");
    if (selectedItem == 0) {
        textPrintAt(3, 3, "> Small:  8 - Large: 16");
    } else {
        textPrintAt(3, 3, "  Small:  8 - Large: 16");
    }
    if (selectedItem == 1) {
        textPrintAt(3, 4, "> Small:  8 - Large: 32");
    } else {
        textPrintAt(3, 4, "  Small:  8 - Large: 32");
    }
    if (selectedItem == 2) {
        textPrintAt(3, 5, "> Small: 16 - Large: 32");
    } else {
        textPrintAt(3, 5, "  Small: 16 - Large: 32");
    }
}

static void changeObjSize(void) {
    WaitForVBlank();
    if (selectedItem == 0)
        REG_OBJSEL = OBJ_SIZE_TO_REG(OBJ_SIZE8_L16);
    else if (selectedItem == 1)
        REG_OBJSEL = OBJ_SIZE_TO_REG(OBJ_SIZE8_L32);
    else
        REG_OBJSEL = OBJ_SIZE_TO_REG(OBJ_SIZE16_L32);
    oamClear();
}

static void drawSprites(void) {
    u8 nextId;

    if (selectedItem == 0) {
        /* Mode 0: hero16 as LARGE (16x16) + hero8 as SMALL (8x8) */
        nextId = oamDrawMeta(0, 64, 140, hero16_frame0,
                             BASE_TILE_16, 0, OBJ_LARGE);
        oamDrawMeta(nextId, 160, 148, hero8_frame0,
                    BASE_TILE_8, 0, OBJ_SMALL);
    } else if (selectedItem == 1) {
        /* Mode 1: hero32 as LARGE (32x32) + hero8 as SMALL (8x8) */
        nextId = oamDrawMeta(0, 48, 108, hero32_frame0,
                             BASE_TILE_32, 0, OBJ_LARGE);
        oamDrawMeta(nextId, 160, 148, hero8_frame0,
                    BASE_TILE_8, 0, OBJ_SMALL);
    } else {
        /* Mode 2: hero32 as LARGE (32x32) + hero16 as SMALL (16x16) */
        nextId = oamDrawMeta(0, 48, 108, hero32_frame0,
                             BASE_TILE_32, 0, OBJ_LARGE);
        oamDrawMeta(nextId, 160, 124, hero16_frame0,
                    BASE_TILE_16, 0, OBJ_SMALL);
    }
}

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);

    /* Move BG1 tile/map pointers away from OBJ VRAM area */
    bgSetGfxPtr(0, VRAM_FONT);
    bgSetMapPtr(0, VRAM_TEXT_MAP, SC_32x32);

    /* Load font for text on BG1 (4bpp in Mode 1) */
    textLoadFont4bpp(VRAM_FONT);
    /* textInitEx takes byte address (word addr * 2), font_tile=0 (font at BG base) */
    textInitEx(VRAM_TEXT_MAP * 2, 0, 0);

    /* Font palette: BG palette 0, color 0=black, color 1=white */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    REG_CGADD = 1;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;

    /* Load all sprite tile data to OBJ VRAM (screen still blanked) */
    dmaCopyVram(spritehero32_til, VRAM_HERO32, HERO32_TILES * TILE_BYTES);
    dmaCopyVram(spritehero16_til, VRAM_HERO16, HERO16_TILES * TILE_BYTES);
    dmaCopyVram(spritehero8_til,  VRAM_HERO8,  HERO8_TILES * TILE_BYTES);

    /* Load sprite palette (sprite palette 0 = CGRAM $80) */
    dmaCopyCGram(spritehero32_pal, 128, 32);

    /* Initialize OBJ settings */
    selectedItem = 0;
    REG_OBJSEL = OBJ_SIZE_TO_REG(OBJ_SIZE8_L16);
    oamClear();

    /* Enable BG1 (text) and OBJ (sprites) */
    REG_TM = TM_BG1 | TM_OBJ;

    drawMenu();
    drawSprites();
    textFlush();
    WaitForVBlank();
    setScreenOn();

    while (1) {
        u16 pressed;
        pressed = padPressed(0);

        if (pressed & KEY_UP) {
            if (selectedItem > 0) {
                selectedItem--;
                drawMenu();
                changeObjSize();
                drawSprites();
            }
        }
        if (pressed & KEY_DOWN) {
            if (selectedItem < 2) {
                selectedItem++;
                drawMenu();
                changeObjSize();
                drawSprites();
            }
        }

        drawSprites();
        WaitForVBlank();
        textFlush();
    }

    return 0;
}
