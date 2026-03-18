/**
 * @file main.c
 * @brief Semi-transparent darkened rectangle via HDMA windows and color math
 * @ingroup examples
 *
 * Combines two SNES hardware features to render a darkened rectangle over a
 * background image. HDMA drives the window position registers WH0 ($2126)
 * and WH1 ($2127) every scanline to define the rectangle boundaries. The
 * color math unit is configured to subtract a fixed color intensity inside
 * the window region only (CGWSEL bit 5-4 = 01), producing a tinted overlay.
 *
 * The HDMA tables use repeat mode (bit 7 set in the line count byte) because
 * WH0/WH1 are write-only registers that must be refreshed every scanline,
 * even when the window position is constant across the rectangle height.
 *
 * Ported from PVSnesLib "TransparentWindow" example by Digifox.
 * Uses bare-metal register writes to match PVSnesLib behavior exactly.
 *
 * @par SNES Concepts
 * - HDMA repeat mode for per-scanline window boundary updates
 * - Window registers WH0/WH1 ($2126/$2127) defining a rectangular region
 * - Color math subtraction inside the window region (CGWSEL/CGADSUB)
 * - Fixed color source (COLDATA, $2132) for darkening intensity
 *
 * @par What to Observe
 * - A background image with a darkened rectangle overlaid in the center
 * - The effect is static; no input is required
 * - The rectangle edges are pixel-sharp (HDMA-defined per scanline)
 *
 * @par Modules Used
 * console, sprite, dma, background, window, colormath, hdma, math
 *
 * @see hdma.h, colormath.h, video.h
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

/** @brief 4bpp background tile data (defined in data.asm, stored in ROM) */
extern u8 tiles[], tiles_end[];
/** @brief Background tilemap data for the 32x32 tile grid */
extern u8 tilemap[], tilemap_end[];
/** @brief 16-color palette for the background */
extern u8 palette[];

/*============================================================================
 * Rectangle parameters
 *============================================================================*/

/** @brief X coordinate of the darkened rectangle's left edge (pixels) */
#define RECT_X      40
/** @brief Y coordinate of the darkened rectangle's top edge (scanlines from top) */
#define RECT_Y      96
/** @brief Width of the darkened rectangle in pixels */
#define RECT_WIDTH  176
/** @brief Height of the darkened rectangle in scanlines */
#define RECT_HEIGHT 112

/*============================================================================
 * HDMA Tables (in RAM — built dynamically at startup)
 *
 * These tables drive WH0 ($2126) and WH1 ($2127) to define the left and
 * right window boundaries per scanline. The tables use HDMA repeat mode
 * (bit 7 set in the line count) because WH0/WH1 are write-only registers
 * that must be refreshed every scanline — even when the position is constant.
 *
 * Table structure:
 *   [y] [0xFF/0x00]              non-repeat: y lines, window disabled
 *   [0x80|height] [val]...[val]  repeat: height lines, per-scanline data
 *   [0x01] [0xFF/0x00]           non-repeat: 1 line, window disabled
 *   [0x00]                       end marker
 *
 * Max size: 2 + 1 + 112 + 2 + 1 = 118 bytes
 *============================================================================*/

/**
 * @brief HDMA table for the left window boundary (WH0, $2126).
 *
 * Built at startup to define where the darkened rectangle starts horizontally
 * on each scanline. Before and after the rectangle, WH0 is set to 0xFF
 * (disabling the window by placing the left edge beyond the right edge).
 */
u8 hdma_left[128];

/**
 * @brief HDMA table for the right window boundary (WH1, $2127).
 *
 * Built at startup to define where the darkened rectangle ends horizontally
 * on each scanline. Before the rectangle, WH1 is set to 0x00 (disabling the
 * window by placing the right edge before the left edge).
 */
u8 hdma_right[128];

/*============================================================================
 * Main
 *============================================================================*/

/**
 * @brief Entry point: darkened rectangle overlay using HDMA windows and color math.
 *
 * Builds HDMA tables to define a rectangle region, loads a background image,
 * then uses the PPU's color math unit to subtract a fixed color intensity
 * inside the window — creating a semi-transparent darkened overlay. This
 * technique is used in RPGs for dialogue boxes, inventory screens, etc.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    u8 i;

    /* Force blank during setup */
    setScreenOff();

    /*--------------------------------------------------------------------
     * Build HDMA tables with repeat mode (matching PVSnesLib exactly)
     *--------------------------------------------------------------------*/

    /* Left table (WH0): disabled → x → disabled */
    hdma_left[0] = RECT_Y;
    hdma_left[1] = 0xFF;                       /* WH0=255: window disabled */
    hdma_left[2] = (u8)(0x80 | RECT_HEIGHT);   /* Repeat mode, 112 lines */
    for (i = 0; i < RECT_HEIGHT; i++) {
        hdma_left[3 + i] = RECT_X;             /* WH0=40 each scanline */
    }
    hdma_left[3 + RECT_HEIGHT] = 0x01;         /* 1 line non-repeat */
    hdma_left[4 + RECT_HEIGHT] = 0xFF;         /* WH0=255: disabled */
    hdma_left[5 + RECT_HEIGHT] = 0x00;         /* End marker */

    /* Right table (WH1): disabled → x+width → end */
    hdma_right[0] = RECT_Y;
    hdma_right[1] = 0x00;                      /* WH1=0: window disabled */
    hdma_right[2] = (u8)(0x80 | RECT_HEIGHT);  /* Repeat mode, 112 lines */
    for (i = 0; i < RECT_HEIGHT; i++) {
        hdma_right[3 + i] = (u8)(RECT_X + RECT_WIDTH); /* WH1=216 */
    }
    hdma_right[3 + RECT_HEIGHT] = 0x00;        /* End marker */

    /*--------------------------------------------------------------------
     * Load background on BG2 (index 1):
     *   tiles at VRAM $4000, tilemap at $0000, palette slot 0
     *--------------------------------------------------------------------*/
    bgSetMapPtr(1, 0x0000, SC_32x32);
    bgInitTileSet(1, tiles, palette, 0,
                  tiles_end - tiles,
                  16 * 2, BG_16COLORS, 0x4000);
    dmaCopyVram(tilemap, 0x0000, tilemap_end - tilemap);

    /*--------------------------------------------------------------------
     * Video mode: Mode 1, display BG2 only
     *--------------------------------------------------------------------*/
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG2;

    /*--------------------------------------------------------------------
     * Color math: direct register writes matching PVSnesLib exactly
     *
     * CGWSEL ($2130) = 0x10:
     *   bits 7-6 = 00: never force main screen black
     *   bits 5-4 = 01: color math inside color window only
     *   bit 1 = 0: use fixed color (not sub screen)
     *   bit 0 = 0: no direct color
     *
     * CGADSUB ($2131) = 0x82:
     *   bit 7 = 1: subtract mode
     *   bit 6 = 0: no half
     *   bit 1 = 1: BG2 participates in color math
     *
     * COLDATA ($2132) = 0xEC:
     *   bits 7-5 = 111: apply to R+G+B
     *   bits 4-0 = 01100: intensity 12
     *--------------------------------------------------------------------*/
    REG_CGWSEL = 0x10;
    REG_CGADSUB = 0x82;
    REG_COLDATA = 0xEC;

    /*--------------------------------------------------------------------
     * Window: direct register writes matching PVSnesLib exactly
     *
     * W12SEL ($2123) = 0x20: BG2 Window 1 Enable
     * WOBJSEL ($2125) = 0x20: Color Math Window 1 Enable
     * TMW ($212E) = 0x00: no main screen masking
     *--------------------------------------------------------------------*/
    REG_W12SEL = 0x20;
    REG_WOBJSEL = 0x20;
    REG_TMW = 0;

    /*--------------------------------------------------------------------
     * HDMA: drive WH0 and WH1 to define the rectangle
     *--------------------------------------------------------------------*/
    /* NMI uses DMA channel 7 for OAM — use channels 4+5 for HDMA */
    hdmaSetup(HDMA_CHANNEL_4, HDMA_MODE_1REG, HDMA_DEST_WH0, hdma_left);
    hdmaSetup(HDMA_CHANNEL_5, HDMA_MODE_1REG, HDMA_DEST_WH1, hdma_right);
    hdmaEnable((1 << HDMA_CHANNEL_4) | (1 << HDMA_CHANNEL_5));

    /* Turn on screen */
    setScreenOn();

    /*--------------------------------------------------------------------
     * Main loop — static effect, no input
     *--------------------------------------------------------------------*/
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
