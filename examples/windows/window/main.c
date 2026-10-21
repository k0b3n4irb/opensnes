/**
 * @file main.c
 * @brief An HDMA-shaped window: a torch beam in a dark alley, masking layers or lighting them
 * @ingroup examples
 *
 * Kit stands in a dark back alley with a torch. The beam is a window: its
 * left edge (WH0) changes on every scanline, driven by HDMA, so the window
 * is a cone that leaves the torch and opens to the right. Two backgrounds
 * are loaded — the alley (BG1) and the rain falling in front of it (BG2) —
 * and the buttons choose what the beam does to them.
 *
 * HDMA channels 4 and 5 drive WH0 ($2126) and WH1 ($2127) in repeat mode,
 * updating the left and right window boundaries every scanline. The W12SEL
 * register ($2123) controls which BG layers are affected by Window 1, with
 * the invert bit set so pixels outside the beam are masked (clipped to
 * black). Kit is a sprite, and the window is not enabled on sprites: he
 * stays visible in the dark.
 *
 * The same window has a second use: instead of hiding layers, it can mark
 * where colour math happens. Press Y and the beam stops clipping and lights
 * instead — colour math adds a warm fixed colour inside it only (CGWSEL
 * "inside the colour window"), the way a game draws a light, a shadow or a
 * dialogue box over the scene.
 *
 * Ported from PVSnesLib's "Window" example; the colour-math mode is its
 * "TransparentWindow" example (by Digifox), adding instead of subtracting.
 *
 * @par SNES Concepts
 * - Window masking via W12SEL ($2123) and TMW ($212E) registers
 * - HDMA repeat mode for per-scanline WH0/WH1 updates, the table computed
 *   at boot from the shape it draws
 * - Window inversion (mask outside vs inside the region)
 * - Runtime window reconfiguration via register writes
 * - The colour window (WOBJSEL) as the region of colour math (CGWSEL/CGADSUB)
 * - Fixed colour source (COLDATA, $2132) added to brighten
 *
 * @par What to Observe
 * - At boot the alley and the rain are black but for the torch's beam
 * - Press A: the beam on the alley only — the rain falls everywhere
 * - Press X: the beam on the rain only — the alley is lit, the rain shows
 *   only where the torch catches it
 * - Press B: the beam on both again
 * - Press Y: nothing is clipped, and the beam brightens what it falls on
 *
 * @par Modules Used
 * console, sprite, dma, input, background, window, hdma, colormath
 *
 * @see hdma.h, window.h, colormath.h, video.h, background.h
 */

#include <snes.h>

/*============================================================================
 * Graphics Data (converted by the build from res/, one settings file beside each picture)
 *============================================================================*/

#include "res/alley.inc"
#include "res/rain.inc"
#include "res/kit.inc"
/* generated: alley_tiles/_map/_pal, rain_tiles/_map/_pal, kit_tiles/_pal (each with _end) */

/** @name VRAM layout (word addresses; a BG's tiles start on a $1000 boundary)
 *  @{ */
#define VRAM_BG1_MAP   0x0000   /**< the alley's map, 32x32 */
#define VRAM_BG2_MAP   0x0400   /**< the rain's map, 32x32 */
#define VRAM_BG1_TILES 0x1000   /**< the alley's 889 tiles, up to $478F */
#define VRAM_BG2_TILES 0x5000   /**< the rain's tiles */
#define VRAM_OBJ       0x6000   /**< Kit */
/** @} */

/** @name Kit and his torch
 *  @{ */
#define KIT_X   16   /**< Kit's sprite x */
#define KIT_Y   168  /**< Kit's sprite y: his feet on the alley's ground */
#define BEAM_X  44   /**< the beam's tip: the torch, at (27, 19) in his frame */
#define BEAM_Y  187
/** @} */

/*============================================================================
 * HDMA Beam Tables (in RAM, computed by buildBeam())
 *
 * Format: [count | 0x80] [data per scanline...]
 *   - 0x80 bit = repeat mode (HDMA reads one new byte per scanline)
 *   - Without 0x80 = non-repeat (same data held for count scanlines)
 *============================================================================*/

/** @brief First line of the beam: it widens by one line every 3 pixels, so
 *  at x 255 it spans BEAM_Y - 70 to the bottom of the screen */
#define BEAM_TOP   (BEAM_Y - 70)
/** @brief Lines of the beam, BEAM_TOP to the last visible line (223) */
#define BEAM_LINES (224 - BEAM_TOP)

/**
 * @brief HDMA table for the left window boundary (WH0, $2126).
 *
 * BEAM_TOP lines with WH0 = 255 (left > right: no window), then one value a
 * line for the beam — BEAM_X + 3 x |line - BEAM_Y|, the cone's edge, or 255
 * where that is past the screen — then the end of the table.
 */
u8 tablelefttriangle[2 + 1 + BEAM_LINES + 1];
/**
 * @brief HDMA table for the right window boundary (WH1, $2127).
 *
 * The cone reaches the right edge of the screen on every line it covers:
 * WH1 = 255 there, and 0 (no window) above it.
 */
u8 tablerighttriangle[2 + 1 + BEAM_LINES + 1];

/** @brief Compute the two tables of the beam */
static void buildBeam(void) {
    u16 i, y, d, left;

    tablelefttriangle[0] = BEAM_TOP;  tablelefttriangle[1] = 0xFF;   /* no window above */
    tablerighttriangle[0] = BEAM_TOP; tablerighttriangle[1] = 0x00;
    tablelefttriangle[2] = 0x80 | BEAM_LINES;                       /* one byte a line */
    tablerighttriangle[2] = 0x80 | BEAM_LINES;
    for (i = 0; i < BEAM_LINES; i++) {
        y = BEAM_TOP + i;
        d = (y > BEAM_Y) ? (u16)(y - BEAM_Y) : (u16)(BEAM_Y - y);
        left = BEAM_X + 3 * d;
        tablelefttriangle[3 + i] = (u8)(left > 255 ? 255 : left);
        tablerighttriangle[3 + i] = 255;
    }
    tablelefttriangle[3 + BEAM_LINES] = 0;                           /* end of table */
    tablerighttriangle[3 + BEAM_LINES] = 0;
}

/*============================================================================
 * W12SEL bit layout for Window 1:
 *   bit 0: BG1 Window 1 Invert
 *   bit 1: BG1 Window 1 Enable
 *   bit 4: BG2 Window 1 Invert
 *   bit 5: BG2 Window 1 Enable
 *
 * Precomputed masks: Enable + Invert (MSKOUT) for each BG combo
 *============================================================================*/
/** @brief W12SEL value to enable Window 1 on BG1 with inversion (mask outside) */
#define W12SEL_BG1  0x03   /* BG1: enable(0x02) + invert(0x01) */
/** @brief W12SEL value to enable Window 1 on BG2 with inversion */
#define W12SEL_BG2  0x30   /* BG2: enable(0x20) + invert(0x10) */
/** @brief W12SEL value to enable Window 1 on both BG1 and BG2 with inversion */
#define W12SEL_BOTH 0x33   /* Both BGs */

/** @brief The rain's scroll: it falls a pixel left for four down */
static u16 rain_x, rain_y;

/**
 * @brief Configure HDMA-driven window masking on the specified BG layers.
 *
 * The window "inverts" masking (via the invert bits in W12SEL), so pixels
 * OUTSIDE the beam are clipped to black, while pixels inside remain
 * visible. This is the opposite of the default behavior (which masks inside).
 *
 * HDMA channels 4 and 5 are used (not 7, which carries every DMA of the NMI
 * handler). HDMA is disabled before reconfiguration and re-enabled after to
 * avoid partial-table reads during the transition.
 *
 * @param layers  Bitmask of BG layers for TMW (e.g., TM_BG1 | TM_BG2)
 * @param w12sel_val  W12SEL register value specifying which BGs use Window 1
 */
static void setup_window(u8 layers, u8 w12sel_val) {
    u8 w1 = 0;

    /* Disable HDMA first to prevent partial table reads during reconfiguration */
    hdmaDisableMask((1 << HDMA_CHANNEL_4) | (1 << HDMA_CHANNEL_5));

    /* Masking mode: no colour math (the lighting mode below turns it on) */
    colorMathDisable();

    /* Window 1, inverted (pixels OUTSIDE the beam are clipped), on the BGs
     * W12SEL names. Not on sprites: Kit stays visible in the dark. */
    if (w12sel_val & 0x02) w1 |= LAYER_BG1;
    if (w12sel_val & 0x20) w1 |= LAYER_BG2;
    windowDisableAll();
    windowEnable(WINDOW_1, w1);
    windowSetInvert(WINDOW_1, w1, 1);
    windowSetMainMask(layers);

    /* Configure HDMA: channel 4 drives WH0 (left boundary),
     * channel 5 drives WH1 (right boundary). Mode 1REG = one byte per
     * scanline written to the target register. */
    hdmaSetup(HDMA_CHANNEL_4, HDMA_MODE_1REG, HDMA_DEST_WH0, tablelefttriangle);
    hdmaSetup(HDMA_CHANNEL_5, HDMA_MODE_1REG, HDMA_DEST_WH1, tablerighttriangle);
    hdmaEnableMask((1 << HDMA_CHANNEL_4) | (1 << HDMA_CHANNEL_5));
}

/**
 * @brief Use the beam as the region of colour math instead of a mask.
 *
 * The HDMA tables are the same; what changes is what the window is for.
 * No layer is clipped (TMW = 0): the colour window, not inverted, is the
 * beam, and colour math adds a warm fixed colour (6, 5, 2) to BG1 and BG2
 * inside it only (CGWSEL bits 5-4 = 01). PVSnesLib's TransparentWindow
 * subtracted, to darken; adding lights.
 */
static void setup_light(void) {
    hdmaDisableMask((1 << HDMA_CHANNEL_4) | (1 << HDMA_CHANNEL_5));
    windowDisableAll();
    windowEnable(WINDOW_1, WINDOW_MATH);           /* WOBJSEL $20: the colour window */
    windowSetMainMask(0);                          /* TMW $00: nothing is clipped */
    colorMathInit();
    colorMathSetCondition(COLORMATH_INSIDE);       /* CGWSEL bits 5-4 = 01 */
    colorMathSetSource(COLORMATH_SRC_FIXED);       /* CGWSEL bit 1 = 0 */
    colorMathSetOp(COLORMATH_ADD);                 /* CGADSUB bit 7 clear */
    colorMathSetLayers(LAYER_BG1 | LAYER_BG2);     /* CGADSUB bits 0-1 */
    colorMathSetFixedColor(6, 5, 2);               /* COLDATA: a warm light */
    hdmaSetup(HDMA_CHANNEL_4, HDMA_MODE_1REG, HDMA_DEST_WH0, tablelefttriangle);
    hdmaSetup(HDMA_CHANNEL_5, HDMA_MODE_1REG, HDMA_DEST_WH1, tablerighttriangle);
    hdmaEnableMask((1 << HDMA_CHANNEL_4) | (1 << HDMA_CHANNEL_5));
}

/*============================================================================
 * Main
 *============================================================================*/

/**
 * @brief Entry point: an HDMA beam window with selectable BG layers.
 *
 * Loads the alley, the rain and Kit, computes the beam, and enters a loop
 * where button presses reconfigure what the beam does.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    consoleInit();

    /* Force blank during setup — screen off prevents visible VRAM corruption */
    setScreenOff();

    /* BG1: the alley, palette slot 0 */
    bgSetMapPtr(0, VRAM_BG1_MAP, SC_32x32);
    bgInitTileSet(0, alley_tiles, alley_pal, 0,
                  alley_tiles_end - alley_tiles,
                  PALETTE_16_SIZE, BG_16COLORS, VRAM_BG1_TILES);
    dmaCopyVram(alley_map, VRAM_BG1_MAP, alley_map_end - alley_map);

    /* BG2: the rain, palette slot 1 */
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
    bgInitTileSet(1, rain_tiles, rain_pal, 1,
                  rain_tiles_end - rain_tiles,
                  PALETTE_16_SIZE, BG_16COLORS, VRAM_BG2_TILES);
    dmaCopyVram(rain_map, VRAM_BG2_MAP, rain_map_end - rain_map);

    /* Kit, OBJ palette 0, a 32x32 sprite */
    oamInitGfxSet(kit_tiles, kit_tiles_end - kit_tiles,
                  kit_pal, kit_pal_end - kit_pal,
                  0, VRAM_OBJ, OBJ_SIZE16_L32);
    oamSet(0, KIT_X, KIT_Y, 0, 0, 3, 0);
    oamSetSize(0, OBJ_LARGE);

    /* Video mode: Mode 1, BG1 + BG2 + sprites */
    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1 | TM_BG2 | TM_OBJ);

    /* The beam on both BGs */
    buildBeam();
    setup_window(TM_BG1 | TM_BG2, W12SEL_BOTH);

    /* Turn on screen */
    setScreenOn();

    /*--------------------------------------------------------------------
     * Main loop
     *
     * PPU register writes in setup_window() happen immediately after
     * WaitForVBlank returns, which is still during VBlank (~11K cycles
     * remain after NMI handler). This satisfies Pattern 4.
     *--------------------------------------------------------------------*/
    while (1) {
        WaitForVBlank();

        /* A: the beam on the alley only */
        if (padPressed(0) & KEY_A) {
            setup_window(TM_BG1, W12SEL_BG1);
        }

        /* X: the beam on the rain only */
        if (padPressed(0) & KEY_X) {
            setup_window(TM_BG2, W12SEL_BG2);
        }

        /* B: the beam on both BGs */
        if (padPressed(0) & KEY_B) {
            setup_window(TM_BG1 | TM_BG2, W12SEL_BOTH);
        }

        /* Y: the beam lights both BGs through colour math */
        if (padPressed(0) & KEY_Y) {
            setup_light();
        }

        /* The rain falls: a pixel left every four frames, four down a frame */
        rain_y -= 4;
        if ((rain_y & 15) == 0) rain_x++;
        bgSetScroll(1, rain_x, rain_y);
    }

    return 0;
}
