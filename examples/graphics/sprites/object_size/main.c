/**
 * @file main.c
 * @brief Interactive demo of all six SNES OBJ size modes
 * @ingroup examples
 *
 * The SNES PPU supports six sprite size combinations, configured globally
 * via the OBJSEL register ($2101, bits 7-5). Each mode defines a "small"
 * and "large" size that all 128 OAM entries share. Individual sprites
 * select small or large via bit 1 of their OAM high-table entry.
 *
 * This example provides a text menu (UP/DOWN to navigate) that cycles
 * through all six modes: 8/16, 8/32, 8/64, 16/32, 16/64, 32/64. Each
 * selection reloads the appropriate sprite tiles via DMA under force blank
 * (large sprites like 64x64 exceed the ~4KB VBlank DMA budget) and
 * displays one small and one large sprite side by side.
 *
 * Based on PVSnesLib ObjectSize example.
 *
 * @par SNES Concepts
 * - OBJSEL register ($2101): 6 size modes (bits 7-5) and name base (bits 1-0)
 * - OAM high table size bit: selects small or large per sprite
 * - Force blank for large VRAM transfers that exceed VBlank DMA budget
 * - Sprite tile numbering relative to the OBJSEL name base address
 *
 * @par What to Observe
 * - A text menu listing all 6 size combinations with a cursor
 * - Press UP/DOWN to select a mode
 * - One small sprite (left) and one large sprite (right) are shown
 * - Notice how the same tile data renders at different pixel sizes
 *
 * @par Modules Used
 * console, sprite, dma, text, text4bpp, input, background
 *
 * @see sprite.h, dma.h, input.h, video.h
 */

#include <snes.h>

/** @brief 8x8 sprite tile data (defined in data.asm) */
extern u8 sprite8[], sprite8_end[];
/** @brief 16x16 sprite tile data */
extern u8 sprite16[], sprite16_end[];
/** @brief 32x32 sprite tile data */
extern u8 sprite32[], sprite32_end[];
/** @brief 64x64 sprite tile data */
extern u8 sprite64[], sprite64_end[];
/** @brief Palette for the 8x8 sprite */
extern u8 palsprite8[], palsprite8_end[];
/** @brief Palette for the 16x16 sprite */
extern u8 palsprite16[], palsprite16_end[];
/** @brief Palette for the 32x32 sprite */
extern u8 palsprite32[], palsprite32_end[];
/** @brief Palette for the 64x64 sprite */
extern u8 palsprite64[], palsprite64_end[];

/**
 * @brief VRAM word address where the "small" sprite tiles are loaded.
 *
 * Placed at $4100 (offset $100 from name base $4000), giving tile number
 * $0010 = ($4100 - $4000) / 16. The offset from the name base determines
 * the OAM tile number used to reference these tiles.
 */
#define ADRSPRITE       0x4100

/**
 * @brief VRAM word address where the "large" sprite tiles are loaded.
 *
 * At $4500 (tile number $0050 from name base $4000). Placed after the small
 * sprite tiles to avoid VRAM overlap.
 */
#define ADRSPRITLARGE   0x4500

/** @brief Size of one 16-color sprite palette in bytes (16 colors x 2 bytes) */
#define PALETTESPRSIZE  PALETTE_16_SIZE

/**
 * @brief OAM tile number for the small sprite, relative to OBJSEL name base $4000.
 *
 * Tile number = (VRAM_address - name_base) / 16 = ($4100 - $4000) / 16 = 0x10.
 * The divisor of 16 is because each tile occupies 16 VRAM words (32 bytes for 4bpp).
 */
#define TILE_SMALL  0x0010

/**
 * @brief OAM tile number for the large sprite, relative to OBJSEL name base $4000.
 *
 * Tile number = ($4500 - $4000) / 16 = 0x50.
 */
#define TILE_LARGE  0x0050

/** @brief Currently selected OBJ size mode index (0-5), controlled by UP/DOWN */
u16 selectedItem;

/**
 * @brief Draw the selection cursor next to the currently selected menu item.
 *
 * Places a ">" character at the selected row and a space at all others.
 * textFlush() queues the tilemap update for DMA during the next VBlank.
 */
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

/**
 * @brief Load sprite tiles for the selected size mode under force blank.
 *
 * Force blank (INIDISP bit 7 = 1) disables rendering, giving unlimited
 * time for VRAM writes. This is necessary because large sprites (32x32,
 * 64x64) require more than the ~4KB VBlank DMA budget to transfer.
 *
 * For each mode, the small sprite tiles are loaded with oamInitGfxSet()
 * (which also sets OBJSEL) and the large sprite tiles are loaded separately
 * via dmaCopyVram(). Each size uses a different palette loaded to CGRAM
 * address 128+16 (sprite palette slot 1).
 *
 * After loading, two OAM entries are configured: one small sprite on the
 * left and one large sprite on the right.
 */
static void changeObjSize(void) {
    /* Enter force blank — unlimited VRAM write time.
     * We wait for VBlank first to ensure any pending NMI DMA completes. */
    WaitForVBlank();
    setScreenOff();

    if (selectedItem == 0) {
        /* 8x8 small / 16x16 large */
        oamInitGfxSet(sprite8, sprite8_end - sprite8,
                      palsprite8, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE8_L16);
        dmaCopyVram(sprite16, ADRSPRITLARGE, sprite16_end - sprite16);
        dmaCopyCGram(palsprite16, OBJ_CGRAM_PAL(1), PALETTESPRSIZE);
    } else if (selectedItem == 1) {
        /* 8x8 small / 32x32 large */
        oamInitGfxSet(sprite8, sprite8_end - sprite8,
                      palsprite8, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE8_L32);
        dmaCopyVram(sprite32, ADRSPRITLARGE, sprite32_end - sprite32);
        dmaCopyCGram(palsprite32, OBJ_CGRAM_PAL(1), PALETTESPRSIZE);
    } else if (selectedItem == 2) {
        /* 8x8 small / 64x64 large */
        oamInitGfxSet(sprite8, sprite8_end - sprite8,
                      palsprite8, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE8_L64);
        dmaCopyVram(sprite64, ADRSPRITLARGE, sprite64_end - sprite64);
        dmaCopyCGram(palsprite64, OBJ_CGRAM_PAL(1), PALETTESPRSIZE);
    } else if (selectedItem == 3) {
        /* 16x16 small / 32x32 large */
        oamInitGfxSet(sprite16, sprite16_end - sprite16,
                      palsprite16, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE16_L32);
        dmaCopyVram(sprite32, ADRSPRITLARGE, sprite32_end - sprite32);
        dmaCopyCGram(palsprite32, OBJ_CGRAM_PAL(1), PALETTESPRSIZE);
    } else if (selectedItem == 4) {
        /* 16x16 small / 64x64 large */
        oamInitGfxSet(sprite16, sprite16_end - sprite16,
                      palsprite16, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE16_L64);
        dmaCopyVram(sprite64, ADRSPRITLARGE, sprite64_end - sprite64);
        dmaCopyCGram(palsprite64, OBJ_CGRAM_PAL(1), PALETTESPRSIZE);
    } else if (selectedItem == 5) {
        /* 32x32 small / 64x64 large */
        oamInitGfxSet(sprite32, sprite32_end - sprite32,
                      palsprite32, PALETTESPRSIZE, 0, ADRSPRITE, OBJ_SIZE32_L64);
        dmaCopyVram(sprite64, ADRSPRITLARGE, sprite64_end - sprite64);
        dmaCopyCGram(palsprite64, OBJ_CGRAM_PAL(1), PALETTESPRSIZE);
    }

    /* Display small sprite (left side): palette 0, priority 3, no flip.
     * OBJ_SMALL selects the smaller of the two sizes in the current OBJSEL mode. */
    oamSet(0, 70, 120, TILE_SMALL, 0, 3, 0);
    oamSetEx(0, OBJ_SMALL, OBJ_SHOW);

    /* Display large sprite (right side): palette 1, priority 3, no flip.
     * OBJ_LARGE selects the larger size. Using palette 1 (128+16 = CGRAM 144)
     * allows a different color scheme for the large sprite. */
    oamSet(1, 170, 120, TILE_LARGE, 1, 3, 0);
    oamSetEx(1, OBJ_LARGE, OBJ_SHOW);

    setScreenOn();
}

/**
 * @brief Entry point: interactive demo of all six SNES OBJ size modes.
 *
 * Displays a text menu listing the six OBJSEL size combinations (8/16, 8/32,
 * 8/64, 16/32, 16/64, 32/64). The user navigates with UP/DOWN to switch
 * modes, which triggers a force-blank reload of the appropriate sprite tiles.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    u16 pad;

    consoleInit();
    setMode(BG_MODE1, 0);

    /* Initialize the text system on BG1 (4bpp font in Mode 1).
     * textInit() defaults: tilemap at $3800, font_tile=0, palette=0.
     * Font tiles are loaded to VRAM $0000 and BG1 is configured to read
     * tiles from $0000 and the tilemap from $3800. */
    textInit();
    textLoadFont4bpp(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, SC_32x32);

    /* Set font palette: color 0 = black (background), color 1 = white (text).
     * CGRAM colors are written as 15-bit BGR: $7FFF = white, $0000 = black. */
    setColor(0, RGB(0, 0, 0));
    setColor(1, RGB(31, 31, 31));

    /* Draw the static menu labels (these never change, only the cursor moves) */
    textPrintAt(3, 2, "Object size :");
    textPrintAt(5, 3, "Small:  8 - Large: 16");
    textPrintAt(5, 4, "Small:  8 - Large: 32");
    textPrintAt(5, 5, "Small:  8 - Large: 64");
    textPrintAt(5, 6, "Small: 16 - Large: 32");
    textPrintAt(5, 7, "Small: 16 - Large: 64");
    textPrintAt(5, 8, "Small: 32 - Large: 64");

    /* Start with the first size mode selected */
    selectedItem = 0;
    drawCursor();
    changeObjSize();

    /* Enable BG1 (text menu) and OBJ (sprites) on the main screen */
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
