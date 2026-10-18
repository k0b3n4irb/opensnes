/**
 * @file main.c
 * @brief Dynamic sprite engine: per-frame VRAM tile streaming, single sprites and metasprites
 * @ingroup examples
 *
 * Demonstrates the OpenSNES dynamic sprite engine, which uploads only the
 * currently-needed sprite tiles to VRAM each frame instead of pre-loading
 * the entire sprite sheet. This trades VBlank DMA bandwidth for VRAM space,
 * allowing many unique animation frames without exhausting the 32KB OBJ
 * VRAM region.
 *
 * Four 16x16 sprites are animated simultaneously. Each sprite's tile data
 * is streamed from ROM to VRAM via oamDynamicDraw() — the engine queues
 * dirty tiles and the NMI handler auto-flushes the queue every VBlank
 * within the DMA budget.
 *
 * SELECT switches to the next step of the same engine: a metasprite — a
 * character made of several OBJ — drawn with oamMetaDrawDyn(), its
 * sub-sprites' tiles streamed the same way, under the three OBJSEL size
 * pairs (8/16, 8/32, 16/32) chosen with UP/DOWN. SELECT again comes back.
 * (The metasprite scene was the dynamic_metasprite example, ported from
 * PVSnesLib's DynamicEngineMetaSprite.)
 *
 * @par SNES Concepts
 * - Dynamic VRAM tile streaming to conserve OBJ VRAM space
 * - Batched VBlank DMA for sprite tile uploads
 * - Multiple independently-animated sprites sharing a tile pool
 * - oambuffer[] structure for sprite state management
 * - MetaspriteItem arrays defining a character's sub-sprite layout, and the
 *   size class (OBJ_SMALL / OBJ_LARGE) of the OBJSEL pair it is drawn with
 * - OBJSEL size pairs, switched at run time with oamDynamicInit()
 *
 * @par What to Observe
 * - Four 16x16 sprites displayed in a row, each animating through a
 *   24-frame cycle with an 8-frame delay between advances
 * - No input required; the animation runs continuously
 * - SELECT: two hero characters; UP/DOWN switch between the 3 OBJSEL size
 *   pairs (the menu cursor ">" shows which); the characters change size
 *
 * @par Modules Used
 * console, sprite, sprite_dynamic, sprite_dynamic_meta, sprite_lut, dma,
 * background, text, input
 *
 * @see sprite.h, dma.h, video.h
 */

#include <snes.h>

/**
 * @brief ROM source for the 16x16 sprite sheet tile data.
 *
 * Contains all 24 animation frames laid out sequentially. The dynamic sprite
 * engine reads individual frames from this ROM data and DMAs only the
 * currently-needed tiles to VRAM each frame, conserving OBJ VRAM space.
 */
#include "res/sprite16_grid.inc"   /* generated: sprite16_grid_tiles[], sprite16_grid_pal[] */
#include "res/spritehero32.inc"
#include "res/spritehero16.inc"
#include "res/spritehero8.inc"
/* generated: spriteheroN_tiles[], spriteheroN_pal[] (the metasprite scene) */

/** @brief 16-color palette for the 16x16 sprites */

/**
 * @brief X positions for the four sprites displayed in a horizontal row.
 *
 * Static const arrays are placed in ROM (SUPERFREE section) by the compiler.
 * The sprites are evenly spaced across the 256-pixel screen width.
 */
static const s16 xpos[4] = {64, 112, 160, 208};

/**
 * @brief Global frame counter for animation timing.
 *
 * Counts from 0 to 7, advancing sprite animation every 8th frame (8/60 = 133ms).
 * Using a shared counter keeps all four sprites synchronized.
 */
static u8 frame_counter;

/** @brief Animation frame index for sprite 0 (0-23, wraps at 24) */
static u16 frame0;
/** @brief Animation frame index for sprite 1 (0-23, wraps at 24) */
static u16 frame1;
/** @brief Animation frame index for sprite 2 (0-23, wraps at 24) */
static u16 frame2;
/** @brief Animation frame index for sprite 3 (0-23, wraps at 24) */
static u16 frame3;

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
static void drawSprites(void) {
    if (selectedItem == 0) {
        /* mode 0 (8/16): 16 large + 8 small */
        oamMetaDrawDyn(1, 64, 140, hero16_frame0, spritehero16_tiles, OBJ_LARGE);
        oamMetaDrawDyn(10, 128, 140, hero8_frame0, spritehero8_tiles, OBJ_SMALL);
    } else if (selectedItem == 1) {
        /* mode 1 (8/32): 32 large + 8 small */
        oamMetaDrawDyn(1, 64, 140, hero32_frame0, spritehero32_tiles, OBJ_LARGE);
        oamMetaDrawDyn(10, 192, 140, hero8_frame0, spritehero8_tiles, OBJ_SMALL);
    } else {
        /* mode 3 (16/32): 32 large + 16 small */
        oamMetaDrawDyn(1, 64, 140, hero32_frame0, spritehero32_tiles, OBJ_LARGE);
        oamMetaDrawDyn(10, 192, 140, hero16_frame0, spritehero16_tiles, OBJ_SMALL);
    }
}

/** @brief Draw the configuration menu text */
static void drawText(void) {
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
static void changeObjSize(void) {
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
/** @brief 1 while the metasprite scene is shown */
u8 in_meta;

/**
 * @brief The four animated 16x16 sprites: the engine, the palette, the
 * sprites' state and their first draw; the screen comes on at the end.
 */
static void enterAnimScene(void) {
    u8 i;
    setScreenOff();

    /* Initialize the dynamic sprite engine: large sprites stream to VRAM
     * $0000, small ones to $1000, the 8/16 size pair. */
    {
        static const OamDynamicConfig dyn_cfg = {
            .vramLarge     = 0x0000,
            .vramSmall     = 0x1000,
            .slotLargeInit = 0,
            .slotSmallInit = 0,
            .sizeMode      = OBJ_SIZE8_L16,
        };
        oamDynamicInit(&dyn_cfg);
    }

    /* Load sprite palette to CGRAM 128 (first sprite palette slot). */
    dmaCopyCGram(sprite16_grid_pal, OBJ_CGRAM_BASE, PALETTE_16_SIZE);

    /* Each sprite starts on the same frame, at its own x */
    frame0 = 0;
    frame1 = 0;
    frame2 = 0;
    frame3 = 0;
    for (i = 0; i < 4; i++) {
        oambuffer[i].oamx = xpos[i];
        oambuffer[i].oamy = 100;
        oambuffer[i].oamframeid = 0;
        oambuffer[i].oamattribute = OBJ_PRIO(3);   /* in front of all BGs */
        oambuffer[i].oamrefresh = 1;               /* force the first upload */
        OAM_SET_GFX(i, sprite16_grid_tiles);       /* the ROM tile source */
    }

    /* First draw pass: queue the starting tiles, the NMI flushes them */
    for (i = 0; i < 4; i++)
        oamDynamicDraw(i);
    WaitForVBlank();

    /* Mode 1 with only OBJ layer visible (no backgrounds) */
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_OBJ);
    setScreenOn();
    in_meta = 0;
}

/** @brief One frame of the animated scene: a step every 8 frames, then the draws. */
static void animStep(void) {
    /* Advance animation every 8 frames (8/60 Hz = ~133ms per frame) */
    frame_counter++;
    if (frame_counter >= 8) {
        frame_counter = 0;
        /* Advance each sprite's frame (0-23). oamrefresh = 1 tells
         * oamDynamicDraw() that its tiles changed and must be uploaded. */
        frame0++; if (frame0 >= 24) frame0 = 0;
        oambuffer[0].oamframeid = frame0; oambuffer[0].oamrefresh = 1;
        frame1++; if (frame1 >= 24) frame1 = 0;
        oambuffer[1].oamframeid = frame1; oambuffer[1].oamrefresh = 1;
        frame2++; if (frame2 >= 24) frame2 = 0;
        oambuffer[2].oamframeid = frame2; oambuffer[2].oamrefresh = 1;
        frame3++; if (frame3 >= 24) frame3 = 0;
        oambuffer[3].oamframeid = frame3; oambuffer[3].oamrefresh = 1;
    }
    /* Per-frame draw: oamDynamicDraw() updates oambuffer + OAM and queues a
     * tile DMA when oamrefresh is set; the NMI handler flushes the queue
     * and resets the tile allocator each VBlank. */
    oamDynamicDraw(0);
    oamDynamicDraw(1);
    oamDynamicDraw(2);
    oamDynamicDraw(3);
}

/**
 * @brief The metasprite scene: Mode 0, the text menu on BG1, the heroes'
 * palette; changeObjSize() configures the engine and draws them.
 *
 * VRAM (words): $0000 large tiles, $1000 small tiles (the engine), $3000
 * the 2bpp font, $3800 BG1's tilemap.
 */
static void enterMetaScene(void) {
    setScreenOff();
    oamClear();
    setMode(BG_MODE0, 0);
    setColor(0, RGB(0, 0, 0));
    setColor(1, RGB(31, 31, 31));
    textInit(TEXT_DEFAULT_TILEMAP_ADDR, TEXT_DEFAULT_FONT_TILE, TEXT_DEFAULT_PALETTE);
    textLoadFont(0x3000);
    bgSetGfxPtr(0, 0x3000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);
    dmaCopyCGram(spritehero32_pal, OBJ_CGRAM_BASE, 32);
    setMainScreen(LAYER_BG1 | LAYER_OBJ);
    selectedItem = 0;
    drawText();
    changeObjSize();          /* ends with the screen on */
    in_meta = 1;
}

/** @brief One frame of the metasprite scene: UP/DOWN pick the size pair. */
static void metaStep(void) {
    pad0 = padPressed(0);
    if ((pad0 & KEY_UP) && selectedItem > 0) {
        selectedItem--;
        drawText();
        changeObjSize();
    }
    if ((pad0 & KEY_DOWN) && selectedItem < 2) {
        selectedItem++;
        drawText();
        changeObjSize();
    }
    drawSprites();
}

/**
 * @brief Entry point: the animated sprites at boot, SELECT to the
 * metasprites and back.
 *
 * @return Never returns
 */
int main(void) {
    consoleInit();
    enterAnimScene();

    while (1) {
        WaitForVBlank();
        if (padPressed(0) & KEY_SELECT) {
            if (in_meta) {
                oamClear();
                textClear();
                enterAnimScene();
            } else {
                enterMetaScene();
            }
            continue;
        }
        if (in_meta)
            metaStep();
        else
            animStep();
    }

    return 0;
}
