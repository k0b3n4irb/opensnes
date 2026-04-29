/**
 * @file main.c
 * @brief Dynamic metasprite engine demo — multi-tile characters with VRAM streaming
 * @ingroup examples
 *
 * Demonstrates `oamMetaDrawDyn(..., size_class)`: multi-tile sprite
 * characters where each sub-sprite's tiles are streamed to VRAM on demand
 * (dynamic sprite engine). The `size_class` parameter selects whether the
 * metasprite uses the small or large half of the configured size pair.
 * Three OBJSEL configurations are selectable via D-PAD:
 *
 * - Config 0: 8/16 — 16x16 metasprite (LARGE) + 8x8 metasprite (SMALL)
 * - Config 1: 8/32 — 32x32 metasprite (LARGE) + 8x8 metasprite (SMALL)
 * - Config 2: 16/32 — 32x32 metasprite (LARGE) + 16x16 metasprite (SMALL)
 *
 * Ported from PVSnesLib DynamicEngineMetaSprite example.
 *
 * @par SNES Concepts
 * - Dynamic VRAM tile streaming for multi-tile sprite characters
 * - MetaspriteItem arrays defining sub-sprite layout
 * - OBJSEL size configuration (8/16, 8/32, 16/32)
 * - oambuffer[] state management for metasprite sub-sprites
 *
 * @par What to Observe
 * - Two sprite characters visible on screen
 * - D-PAD UP/DOWN switches between 3 OBJSEL configurations
 * - Menu cursor ">" indicates current selection
 * - Sprites change size when switching configurations
 *
 * @par Modules Used
 * console, sprite, sprite_dynamic, sprite_dynamic_meta, sprite_lut,
 * dma, background, text, input
 *
 * @see sprite.h
 */

#include <snes.h>

/*========================================================================
 * Sprite graphics data (from data.asm)
 *========================================================================*/

extern u8 spritehero32_til[];
extern u8 spritehero32_pal[];
extern u8 spritehero16_til[];
extern u8 spritehero8_til[];


/*========================================================================
 * Metasprite Definitions
 *========================================================================*/

/** @brief 32x32 metasprite: 64x64 character (2x2 grid of 32x32) */
static const MetaspriteItem hero32_frame0[] = {
    METASPR_ITEM(0,  0,  0, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(32, 0,  1, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(0,  32, 6, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(32, 32, 7, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_TERM
};

/** @brief 16x16 metasprite: 32x48 character (2x3 grid of 16x16) */
static const MetaspriteItem hero16_frame0[] = {
    METASPR_ITEM(0,  0,  0, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(16, 0,  1, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(0,  16, 2, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(16, 16, 3, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(0,  32, 4, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(16, 32, 5, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_TERM
};

/** @brief 8x8 metasprite: 16x16 character (2x2 grid of 8x8) */
static const MetaspriteItem hero8_frame0[] = {
    METASPR_ITEM(0, 0, 0, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(8, 0, 1, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(0, 8, 8, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_ITEM(8, 8, 9, OBJ_PAL(0) | OBJ_PRIO(3)),
    METASPR_TERM
};

/*========================================================================
 * State
 *========================================================================*/

u16 selectedItem;
u16 pad0;

/** @brief Draw metasprites for the current OBJSEL configuration */
void drawSprites(void) {
    if (selectedItem == 0) {
        /* mode 0 (8/16): 16 large + 8 small */
        oamMetaDrawDyn(1, 64, 140, hero16_frame0, spritehero16_til, OBJ_LARGE);
        oamMetaDrawDyn(10, 128, 140, hero8_frame0, spritehero8_til, OBJ_SMALL);
    } else if (selectedItem == 1) {
        /* mode 1 (8/32): 32 large + 8 small */
        oamMetaDrawDyn(1, 64, 140, hero32_frame0, spritehero32_til, OBJ_LARGE);
        oamMetaDrawDyn(10, 192, 140, hero8_frame0, spritehero8_til, OBJ_SMALL);
    } else {
        /* mode 3 (16/32): 32 large + 16 small */
        oamMetaDrawDyn(1, 64, 140, hero32_frame0, spritehero32_til, OBJ_LARGE);
        oamMetaDrawDyn(10, 192, 140, hero16_frame0, spritehero16_til, OBJ_SMALL);
    }
}

/** @brief Draw the configuration menu text */
void drawText(void) {
    textPrintAt(3, 2, "Object size :");

    if (selectedItem == 0)
        textPrintAt(3, 3, "> Small:  8 - Large: 16");
    else
        textPrintAt(3, 3, "  Small:  8 - Large: 16");

    if (selectedItem == 1)
        textPrintAt(3, 4, "> Small:  8 - Large: 32");
    else
        textPrintAt(3, 4, "  Small:  8 - Large: 32");

    if (selectedItem == 2)
        textPrintAt(3, 5, "> Small: 16 - Large: 32");
    else
        textPrintAt(3, 5, "  Small: 16 - Large: 32");

    textFlush();
}

/**
 * @brief Reinitialize sprite engine and upload all tiles during force blank.
 *
 * Uses force blank for a glitch-free OBJSEL switch. The current
 * configuration can enqueue >7 entries (config 2 hits 10), and the NMI
 * auto-flush hook drains 7 per VBlank, so `oamDynamicDrainQueue` loops
 * `WaitForVBlank()` until the queue is empty before we release force
 * blank. NMI suppresses the end-frame "hide stale sprites" path during
 * the drain so the just-queued sprites are not pushed off-screen.
 */
void changeObjSize(void) {
    WaitForVBlank();
    setScreenOff();

    {
        static const u8 obj_sizes[] = {
            OBJ_SIZE8_L16, OBJ_SIZE8_L32, OBJ_SIZE16_L32
        };
        OamDynamicConfig dyn_cfg = {
            .vramLarge     = 0x0000,
            .vramSmall     = 0x1000,
            .slotLargeInit = 0,
            .slotSmallInit = 0,
            .sizeMode      = obj_sizes[selectedItem],
        };
        oamDynamicInit(&dyn_cfg);
    }

    oambuffer[1].oamrefresh = 1;
    oambuffer[10].oamrefresh = 1;
    drawSprites();
    oamDynamicDrainQueue();

    setScreenOn();
}

/** @brief Entry point */
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);

    setColor(0, RGB(0, 0, 0));
    setColor(1, RGB(31, 31, 31));

    /* VRAM layout (Mode 0, all word addresses):
     * $0000-$0FFF: Large sprite tiles (dynamic engine, name table 0)
     * $1000-$1FFF: Small sprite tiles (dynamic engine, name table 1)
     * $3000-$32FF: Font tiles (2bpp, 96 chars × 8 words)
     * $3800-$3BFF: BG1 tilemap (32×32 entries) */
    textInit(TEXT_DEFAULT_TILEMAP_ADDR, TEXT_DEFAULT_FONT_TILE, TEXT_DEFAULT_PALETTE);
    textLoadFont(0x3000);
    bgSetGfxPtr(0, 0x3000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    dmaCopyCGram(spritehero32_pal, OBJ_CGRAM_BASE, 32);
    setMainScreen(LAYER_BG1 | LAYER_OBJ);

    selectedItem = 0;
    drawText();
    changeObjSize();
    setScreenOn();

    while (1) {
        pad0 = padPressed(0);

        if (pad0 & KEY_UP) {
            if (selectedItem > 0) {
                selectedItem--;
                drawText();
                changeObjSize();
            }
        }
        if (pad0 & KEY_DOWN) {
            if (selectedItem < 2) {
                selectedItem++;
                drawText();
                changeObjSize();
            }
        }

        drawSprites();
        WaitForVBlank();
        /* NMI auto-flushes end-frame + VRAM tile queue. */
    }
    return 0;
}
