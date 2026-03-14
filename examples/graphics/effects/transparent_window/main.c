/**
 * @file main.c
 * @brief Transparent Window Example
 *
 * Demonstrates a semi-transparent rectangle using SNES color math combined
 * with HDMA-driven window boundaries. A darkened rectangle is rendered over
 * a background image — no input, static effect.
 *
 * Ported from PVSnesLib "TransparentWindow" example by Digifox.
 *
 * Technique:
 *   - HDMA drives WH0/WH1 to define a rectangle per scanline
 *   - Window is enabled for color math only (not for BG masking)
 *   - Color math subtracts a fixed color inside the window region
 *   - Result: the rectangle area appears darkened/tinted
 *
 * BARE-METAL register writes to match PVSnesLib exactly.
 * HDMA tables use REPEAT MODE (per-scanline data) as required by WH0/WH1.
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[];

/*============================================================================
 * Rectangle parameters
 *============================================================================*/

#define RECT_X      40
#define RECT_Y      96
#define RECT_WIDTH  176
#define RECT_HEIGHT 112

/*============================================================================
 * HDMA Tables (in RAM — built dynamically at startup)
 *
 * Format (repeat mode for the rectangle segment):
 *   [y] [0xFF/0x00]              non-repeat: y lines, window disabled
 *   [0x80|height] [val]...[val]  repeat: height lines, per-scanline data
 *   [0x01] [0xFF/0x00]           non-repeat: 1 line, window disabled
 *   [0x00]                       end marker
 *
 * Max size: 2 + 1 + 112 + 2 + 1 = 118 bytes
 *============================================================================*/

u8 hdma_left[128];
u8 hdma_right[128];

/*============================================================================
 * Main
 *============================================================================*/

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
