/**
 * @file main.c
 * @brief Dynamic sprite engine with per-frame VRAM tile streaming
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
 * @par SNES Concepts
 * - Dynamic VRAM tile streaming to conserve OBJ VRAM space
 * - Batched VBlank DMA for sprite tile uploads
 * - Multiple independently-animated sprites sharing a tile pool
 * - oambuffer[] structure for sprite state management
 *
 * @par What to Observe
 * - Four 16x16 sprites displayed in a row, each animating through a
 *   24-frame cycle with an 8-frame delay between advances
 * - No input required; the animation runs continuously
 *
 * @par Modules Used
 * console, sprite, sprite_dynamic, sprite_lut, dma, background, input
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
extern u8 spr16_tiles[];

/** @brief 16-color palette for the 16x16 sprites */
extern u8 spr16_properpal[];

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

/**
 * @brief Entry point: dynamic sprite engine demo with four animated sprites.
 *
 * Initializes the dynamic sprite system, which streams sprite tiles from ROM
 * to VRAM on demand each frame. Four 16x16 sprites are placed in a row,
 * each cycling through 24 animation frames with an 8-frame delay.
 *
 * The dynamic sprite pipeline per frame:
 * 1. oamDynamicDraw() — main thread marks which tiles each sprite needs
 * 2. NMI VBlank handler auto-flushes: DMAs queued tiles to VRAM and
 *    resets the per-frame tile allocator for next frame
 *
 * This approach trades VBlank DMA time for VRAM conservation: instead of
 * pre-loading all 24 frames (24 x 4 tiles x 32 bytes = 3KB per sprite),
 * only the 4 active tiles per sprite are in VRAM at any time.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    setScreenOff();

    /* Initialize the dynamic sprite engine:
     * - Large-tile pool at VRAM word address $0000
     * - Small-tile pool at $1000
     * - Allocator starts at OAM slot 0 for both pools
     * - OBJSEL size mode: small=8x8, large=16x16 */
    static const OamDynamicConfig dyn_cfg = {
        .vramLarge     = 0x0000,
        .vramSmall     = 0x1000,
        .slotLargeInit = 0,
        .slotSmallInit = 0,
        .sizeMode      = OBJ_SIZE8_L16,
    };
    oamDynamicInit(&dyn_cfg);

    /* Load sprite palette to CGRAM 128 (first sprite palette slot).
     * 32 bytes = 16 colors x 2 bytes per color (15-bit BGR). */
    dmaCopyCGram(spr16_properpal, OBJ_CGRAM_BASE, PALETTE_16_SIZE);

    /* Initialize all frame counters to 0 (same starting frame) */
    frame0 = 0;
    frame1 = 0;
    frame2 = 0;
    frame3 = 0;

    /* Initialize each sprite's state in the oambuffer[] array.
     * The oambuffer is the dynamic sprite engine's internal state, separate
     * from the hardware OAM. Each entry tracks position, current frame,
     * attributes, and a refresh flag that triggers VRAM re-upload.
     *
     * Sprites are initialized explicitly (not in a loop) to avoid potential
     * compiler issues with for-loop index variables on the 65816. */
    oambuffer[0].oamx = xpos[0];
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = frame0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);   /**< Priority 3 = in front of all BGs */
    oambuffer[0].oamrefresh = 1;                /**< Force initial tile upload */
    OAM_SET_GFX(0, spr16_tiles);                /**< Point to ROM tile source */

    oambuffer[1].oamx = xpos[1];
    oambuffer[1].oamy = 100;
    oambuffer[1].oamframeid = frame1;
    oambuffer[1].oamattribute = OBJ_PRIO(3);
    oambuffer[1].oamrefresh = 1;
    OAM_SET_GFX(1, spr16_tiles);

    oambuffer[2].oamx = xpos[2];
    oambuffer[2].oamy = 100;
    oambuffer[2].oamframeid = frame2;
    oambuffer[2].oamattribute = OBJ_PRIO(3);
    oambuffer[2].oamrefresh = 1;
    OAM_SET_GFX(2, spr16_tiles);

    oambuffer[3].oamx = xpos[3];
    oambuffer[3].oamy = 100;
    oambuffer[3].oamframeid = frame3;
    oambuffer[3].oamattribute = OBJ_PRIO(3);
    oambuffer[3].oamrefresh = 1;
    OAM_SET_GFX(3, spr16_tiles);

    /* Initial draw pass: queue starting tiles, then let NMI auto-flush
     * during force blank so all 4 tile uploads land in VRAM before
     * setScreenOn — otherwise the first frame shows uninitialized VRAM
     * in the sprite area. The 4 queue entries fit in one VBlank. */
    oamDynamicDraw(0);
    oamDynamicDraw(1);
    oamDynamicDraw(2);
    oamDynamicDraw(3);
    WaitForVBlank();

    /* Mode 1 with only OBJ layer visible (no backgrounds) */
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_OBJ);
    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* Advance animation every 8 frames (8/60 Hz = ~133ms per frame) */
        frame_counter++;
        if (frame_counter >= 8) {
            frame_counter = 0;

            /* Advance each sprite's animation frame (0-23 cycle).
             * Setting oamrefresh = 1 tells oamDynamicDraw() that this
             * sprite's tile data has changed and needs re-uploading to VRAM. */

            frame0++;
            if (frame0 >= 24) frame0 = 0;
            oambuffer[0].oamframeid = frame0;
            oambuffer[0].oamrefresh = 1;

            frame1++;
            if (frame1 >= 24) frame1 = 0;
            oambuffer[1].oamframeid = frame1;
            oambuffer[1].oamrefresh = 1;

            frame2++;
            if (frame2 >= 24) frame2 = 0;
            oambuffer[2].oamframeid = frame2;
            oambuffer[2].oamrefresh = 1;

            frame3++;
            if (frame3 >= 24) frame3 = 0;
            oambuffer[3].oamframeid = frame3;
            oambuffer[3].oamrefresh = 1;
        }

        /* Per-frame draw: oamDynamicDraw() updates oambuffer + OAM and
         * queues a tile DMA when oamrefresh is set. The NMI handler
         * auto-flushes the queue and resets the tile allocator each
         * VBlank — no manual orchestration needed in user code. */
        oamDynamicDraw(0);
        oamDynamicDraw(1);
        oamDynamicDraw(2);
        oamDynamicDraw(3);
    }

    return 0;
}
