/**
 * @file main.c
 * @brief HDMA-driven triangle window masking on selectable BG layers
 * @ingroup examples
 *
 * Demonstrates the SNES window masking hardware combined with HDMA to create
 * a triangle-shaped cutout that clips background layers. Two backgrounds
 * are loaded (BG1 and BG2), and the user can apply the window mask to either
 * or both layers via button presses.
 *
 * HDMA channels 4 and 5 drive WH0 ($2126) and WH1 ($2127) in repeat mode,
 * updating the left and right window boundaries every scanline to form a
 * diamond/triangle shape. The W12SEL register ($2123) controls which BG
 * layers are affected by Window 1, with the invert bit set so pixels
 * outside the triangle are masked (clipped to black).
 *
 * Ported from PVSnesLib "Window" example.
 * Uses bare-metal register writes to match PVSnesLib behavior exactly.
 *
 * @par SNES Concepts
 * - Window masking via W12SEL ($2123) and TMW ($212E) registers
 * - HDMA repeat mode for per-scanline WH0/WH1 updates
 * - Window inversion (mask outside vs inside the region)
 * - Runtime window reconfiguration via register writes
 *
 * @par What to Observe
 * - A triangle/diamond shape masks portions of the background layers
 * - Press A to apply window to BG1 only
 * - Press X to apply window to BG2 only
 * - Press B to apply window to both BG1 and BG2
 *
 * @par Modules Used
 * console, sprite, dma, input, background, window, hdma
 *
 * @see hdma.h, video.h, background.h
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

/** @brief 4bpp tile data for background layer 1 (defined in data.asm) */
extern u8 tiles_bg1[], tiles_bg1_end[];
/** @brief Tilemap data for BG1 (32x32 grid) */
extern u8 tilemap_bg1[], tilemap_bg1_end[];
/** @brief 16-color palette for BG1 */
extern u8 palette_bg1[];

/** @brief 4bpp tile data for background layer 2 (defined in data.asm) */
extern u8 tiles_bg2[], tiles_bg2_end[];
/** @brief Tilemap data for BG2 (32x32 grid) */
extern u8 tilemap_bg2[], tilemap_bg2_end[];
/** @brief 16-color palette for BG2 (loaded to CGRAM palette slot 1) */
extern u8 palette_bg2[];

/*============================================================================
 * HDMA Triangle Tables (in RAM — NOT const)
 *
 * These tables must be mutable (not const) because hdmaSetup() assumes
 * bank $00 for ROM addresses, but the linker may place SUPERFREE const data
 * in bank $01+. Mutable data always resides in bank $00 WRAM ($7E:0000),
 * which hdmaSetup handles correctly.
 *
 * Format: [count | 0x80] [data per scanline...]
 *   - 0x80 bit = repeat mode (HDMA reads one new byte per scanline)
 *   - Without 0x80 = non-repeat (same data held for count scanlines)
 *============================================================================*/

/**
 * @brief HDMA table for the left window boundary (WH0, $2126).
 *
 * Defines a diamond/triangle shape: the left edge starts at x=0x7F (pixel 127,
 * center of screen), narrows toward x=0x60 (pixel 96) over 32 scanlines, then
 * widens back to 0x7F over 32 more scanlines. The 60-line header disables the
 * window above the triangle by setting WH0=0xFF (left > right = no window).
 */
u8 tablelefttriangle[] = {
    60, 0xFF,           /**< 60 lines: WH0=255 (left > right -> window disabled) */
    0x80 | 64,          /**< 64 lines of per-scanline entries (repeat mode) */
    0x7F, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A, 0x79, 0x78,
    0x77, 0x76, 0x75, 0x74, 0x73, 0x72, 0x71, 0x70,
    0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x69, 0x68,
    0x67, 0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0xFF, 0             /**< Tail: WH0=255 (disabled) + end-of-table marker */
};

/**
 * @brief HDMA table for the right window boundary (WH1, $2127).
 *
 * Mirrors the left table: starts at x=0x81 (pixel 129), expands outward to
 * x=0xA0 (pixel 160) then converges back. Together with tablelefttriangle,
 * this creates a diamond shape centered at pixel 128 that is 64 scanlines tall.
 */
u8 tablerighttriangle[] = {
    60, 0x00,           /**< 60 lines: WH1=0 (left > right -> window disabled) */
    0x80 | 64,          /**< 64 lines of per-scanline entries (repeat mode) */
    0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
    0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90,
    0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0,
    0x9F, 0x9E, 0x9D, 0x9C, 0x9B, 0x9A, 0x99, 0x98,
    0x97, 0x96, 0x95, 0x94, 0x93, 0x92, 0x91, 0x90,
    0x8F, 0x8E, 0x8D, 0x8C, 0x8B, 0x8A, 0x89, 0x88,
    0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81,
    0x00, 0             /**< Tail: WH1=0 (disabled) + end-of-table marker */
};

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

/**
 * @brief Configure HDMA-driven window masking on the specified BG layers.
 *
 * Sets up the SNES window hardware and HDMA channels to clip the specified
 * background layers using the triangle-shaped boundary tables. Uses bare-metal
 * PPU register writes to match PVSnesLib's setModeHdmaWindow() behavior exactly.
 *
 * The window "inverts" masking (via the invert bits in W12SEL), so pixels
 * OUTSIDE the triangle shape are clipped to black, while pixels inside remain
 * visible. This is the opposite of the default behavior (which masks inside).
 *
 * HDMA channels 4 and 5 are used (not 7, which the NMI handler reserves for
 * OAM DMA). HDMA is disabled before reconfiguration and re-enabled after to
 * avoid partial-table reads during the transition.
 *
 * @param layers  Bitmask of BG layers for TMW (e.g., TM_BG1 | TM_BG2)
 * @param w12sel_val  W12SEL register value specifying which BGs use Window 1
 */
static void setup_window(u8 layers, u8 w12sel_val) {
    /* Disable HDMA first to prevent partial table reads during reconfiguration */
    hdmaDisable((1 << HDMA_CHANNEL_4) | (1 << HDMA_CHANNEL_5));

    /* TMW ($212E): main screen window mask. Bits enable window clipping per
     * layer. 0x10 includes OBJ in the mask (PVSnesLib convention). */
    REG_TMW = layers | 0x10;
    /* W12SEL ($2123): enable Window 1 for the target BGs with inversion. */
    REG_W12SEL = w12sel_val;
    /* WOBJSEL ($2125): PVSnesLib also writes the same value here to control
     * color math window and OBJ window behavior. */
    REG_WOBJSEL = w12sel_val;

    /* Configure HDMA: channel 4 drives WH0 (left boundary),
     * channel 5 drives WH1 (right boundary). Mode 1REG = one byte per
     * scanline written to the target register. */
    hdmaSetup(HDMA_CHANNEL_4, HDMA_MODE_1REG, HDMA_DEST_WH0,
              tablelefttriangle);
    hdmaSetup(HDMA_CHANNEL_5, HDMA_MODE_1REG, HDMA_DEST_WH1,
              tablerighttriangle);
    hdmaEnable((1 << HDMA_CHANNEL_4) | (1 << HDMA_CHANNEL_5));
}

/*============================================================================
 * Main
 *============================================================================*/

/**
 * @brief Entry point: HDMA triangle window masking with selectable BG layers.
 *
 * Loads two BG layers, applies a diamond-shaped window mask via HDMA, and
 * enters a loop where button presses reconfigure which layers are clipped.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    /* Force blank during setup — screen off prevents visible VRAM corruption */
    setScreenOff();

    /*--------------------------------------------------------------------
     * Load BG1: tiles at VRAM $4000, tilemap at $0000, palette slot 0
     *--------------------------------------------------------------------*/
    bgSetMapPtr(0, 0x0000, SC_32x32);
    bgInitTileSet(0, tiles_bg1, palette_bg1, 0,
                  tiles_bg1_end - tiles_bg1,
                  PALETTE_16_SIZE, BG_16COLORS, 0x4000);
    dmaCopyVram(tilemap_bg1, 0x0000, tilemap_bg1_end - tilemap_bg1);

    /*--------------------------------------------------------------------
     * Load BG2: tiles at VRAM $6000, tilemap at $1000, palette slot 1
     *--------------------------------------------------------------------*/
    bgSetMapPtr(1, 0x1000, SC_32x32);
    bgInitTileSet(1, tiles_bg2, palette_bg2, 1,
                  tiles_bg2_end - tiles_bg2,
                  PALETTE_16_SIZE, BG_16COLORS, 0x6000);
    dmaCopyVram(tilemap_bg2, 0x1000, tilemap_bg2_end - tilemap_bg2);

    /*--------------------------------------------------------------------
     * Video mode: Mode 1, enable BG1 + BG2
     *--------------------------------------------------------------------*/
    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1 | TM_BG2);

    /*--------------------------------------------------------------------
     * Initialize triangle window on both BGs
     *--------------------------------------------------------------------*/
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

        /* A: window on BG1 only */
        if (padPressed(0) & KEY_A) {
            setup_window(TM_BG1, W12SEL_BG1);
        }

        /* X: window on BG2 only */
        if (padPressed(0) & KEY_X) {
            setup_window(TM_BG2, W12SEL_BG2);
        }

        /* B: window on both BGs */
        if (padPressed(0) & KEY_B) {
            setup_window(TM_BG1 | TM_BG2, W12SEL_BOTH);
        }
    }

    return 0;
}
