/**
 * @file main.c
 * @brief HDMA Triangle Window Example
 *
 * Demonstrates HDMA-driven window masking to create a triangle shape
 * that reveals portions of the background. Two backgrounds are loaded;
 * press A/X/B to apply the window to BG1, BG2, or both.
 *
 * Ported from PVSnesLib "Window" example.
 *
 * BARE-METAL register writes to match PVSnesLib exactly.
 *
 * Controls:
 *   A: Window on BG1 only
 *   X: Window on BG2 only
 *   B: Window on both BG1 and BG2
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 tiles_bg1[], tiles_bg1_end[];
extern u8 tilemap_bg1[], tilemap_bg1_end[];
extern u8 palette_bg1[];

extern u8 tiles_bg2[], tiles_bg2_end[];
extern u8 tilemap_bg2[], tilemap_bg2_end[];
extern u8 palette_bg2[];

/*============================================================================
 * HDMA Triangle Tables (in RAM — NOT const)
 *
 * NOT const: hdmaSetup uses bank $00 for all ROM addresses, but the linker
 * may place SUPERFREE const data in bank $01+. RAM tables always stay in
 * bank $00 WRAM, which hdmaSetup handles correctly via bank $7E mirror.
 *
 * Format: [count | 0x80] [data per scanline...]
 *   - 0x80 bit = repeat mode (new data each scanline)
 *   - Without 0x80 = non-repeat (same data held for count scanlines)
 *============================================================================*/

/* Left boundary: converges from 0x7F to 0x60 then back to 0x7F */
u8 tablelefttriangle[] = {
    60, 0xFF,           /* 60 lines: WH0=255 (left > right -> window disabled) */
    0x80 | 64,          /* 64 lines of per-scanline entries (repeat mode) */
    0x7F, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A, 0x79, 0x78,
    0x77, 0x76, 0x75, 0x74, 0x73, 0x72, 0x71, 0x70,
    0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x69, 0x68,
    0x67, 0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0xFF, 0             /* tail: WH0=255 (disabled) + end */
};

/* Right boundary: diverges from 0x81 to 0xA0 then back to 0x81 */
u8 tablerighttriangle[] = {
    60, 0x00,           /* 60 lines: WH1=0 (left > right -> window disabled) */
    0x80 | 64,          /* 64 lines of per-scanline entries (repeat mode) */
    0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
    0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90,
    0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0,
    0x9F, 0x9E, 0x9D, 0x9C, 0x9B, 0x9A, 0x99, 0x98,
    0x97, 0x96, 0x95, 0x94, 0x93, 0x92, 0x91, 0x90,
    0x8F, 0x8E, 0x8D, 0x8C, 0x8B, 0x8A, 0x89, 0x88,
    0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81,
    0x00, 0             /* tail: WH1=0 (disabled) + end */
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

#define W12SEL_BG1  0x03   /* BG1: enable(0x02) + invert(0x01) */
#define W12SEL_BG2  0x30   /* BG2: enable(0x20) + invert(0x10) */
#define W12SEL_BOTH 0x33   /* Both BGs */

/*============================================================================
 * Window setup helper — bare-metal register writes
 *
 * Matches PVSnesLib setModeHdmaWindow() exactly:
 *   TMW = layers | 0x10    (OBJ also included)
 *   W12SEL = w12sel_val
 *   WOBJSEL = w12sel_val   (PVSnesLib sets this too)
 *   HDMA ch6/7 → WH0/WH1
 *============================================================================*/

static void setup_window(u8 layers, u8 w12sel_val) {
    /* Disable HDMA first */
    hdmaDisable((1 << HDMA_CHANNEL_6) | (1 << HDMA_CHANNEL_7));

    /* Set window registers — direct writes matching PVSnesLib */
    REG_TMW = layers | 0x10;      /* Main screen mask + OBJ */
    REG_W12SEL = w12sel_val;       /* Window 1 enable + invert */
    REG_WOBJSEL = w12sel_val;      /* PVSnesLib sets WOBJSEL = bgrndmask */

    /* Set up HDMA: channel 6 → WH0, channel 7 → WH1 */
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, HDMA_DEST_WH0,
              tablelefttriangle);
    hdmaSetup(HDMA_CHANNEL_7, HDMA_MODE_1REG, HDMA_DEST_WH1,
              tablerighttriangle);
    hdmaEnable((1 << HDMA_CHANNEL_6) | (1 << HDMA_CHANNEL_7));
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    /* Force blank during setup */
    setScreenOff();

    /*--------------------------------------------------------------------
     * Load BG1: tiles at VRAM $4000, tilemap at $0000, palette slot 0
     *--------------------------------------------------------------------*/
    bgSetMapPtr(0, 0x0000, SC_32x32);
    bgInitTileSet(0, tiles_bg1, palette_bg1, 0,
                  tiles_bg1_end - tiles_bg1,
                  16 * 2, BG_16COLORS, 0x4000);
    dmaCopyVram(tilemap_bg1, 0x0000, tilemap_bg1_end - tilemap_bg1);

    /*--------------------------------------------------------------------
     * Load BG2: tiles at VRAM $6000, tilemap at $1000, palette slot 1
     *--------------------------------------------------------------------*/
    bgSetMapPtr(1, 0x1000, SC_32x32);
    bgInitTileSet(1, tiles_bg2, palette_bg2, 1,
                  tiles_bg2_end - tiles_bg2,
                  16 * 2, BG_16COLORS, 0x6000);
    dmaCopyVram(tilemap_bg2, 0x1000, tilemap_bg2_end - tilemap_bg2);

    /*--------------------------------------------------------------------
     * Video mode: Mode 1, enable BG1 + BG2
     *--------------------------------------------------------------------*/
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1 | TM_BG2;

    /*--------------------------------------------------------------------
     * Initialize triangle window on both BGs
     *--------------------------------------------------------------------*/
    setup_window(TM_BG1 | TM_BG2, W12SEL_BOTH);

    /* Turn on screen */
    setScreenOn();

    /*--------------------------------------------------------------------
     * Main loop
     *--------------------------------------------------------------------*/
    while (1) {
        WaitForVBlank();
        padUpdate();

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
