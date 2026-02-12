/*---------------------------------------------------------------------------------

    Transparent window effect in mode 1
    Ported from PVSnesLib (digifox) to OpenSNES

    Displays a background on BG2, with a transparent rectangle window
    created using HDMA-driven window boundaries (WH0/WH1).
    Inside the window, fixed-color subtraction darkens the image.

    Window rectangle: x=40, y=96, w=176, h=112

---------------------------------------------------------------------------------*/
#include <snes.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

/* HDMA tables for window left/right boundaries (in RAM, updated at init) */
static u8 hdma_left[256];
static u8 hdma_right[256];

static void create_transparent_window(u8 x, u8 y, u8 w, u8 h) {
    u16 i;
    u16 idx;

    /*
     * Build HDMA table for WH0 (window 1 left position):
     *
     * Before window: y scanlines with left=255
     *   (left > right disables window on those lines)
     * Window region: h scanlines in repeat mode with left=x
     * After window:  disable
     */
    hdma_left[0] = y;      /* y scanlines before window */
    hdma_left[1] = 0xFF;   /* left = 255 (window disabled) */

    hdma_left[2] = 0x80 | h;  /* repeat mode for h scanlines */
    idx = 3;
    for (i = 0; i < h; i++) {
        hdma_left[idx + i] = x;
    }
    idx += h;

    hdma_left[idx]     = 0x01;  /* 1 remaining scanline */
    hdma_left[idx + 1] = 0xFF;  /* left = 255 (disabled) */
    hdma_left[idx + 2] = 0x00;  /* end of HDMA table */

    /*
     * Build HDMA table for WH1 (window 1 right position):
     *
     * Before window: y scanlines with right=0
     * Window region: h scanlines in repeat mode with right=x+w
     * After window:  end
     */
    hdma_right[0] = y;     /* y scanlines before window */
    hdma_right[1] = 0x00;  /* right = 0 (window disabled) */

    hdma_right[2] = 0x80 | h;  /* repeat mode for h scanlines */
    idx = 3;
    for (i = 0; i < h; i++) {
        hdma_right[idx + i] = x + w;
    }
    idx += h;

    hdma_right[idx]     = 0x00;  /* end of HDMA table */

    /* Setup HDMA channels for window boundaries */
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, HDMA_DEST_WH0, hdma_left);
    hdmaSetup(HDMA_CHANNEL_7, HDMA_MODE_1REG, HDMA_DEST_WH1, hdma_right);
    hdmaEnable((1 << HDMA_CHANNEL_6) | (1 << HDMA_CHANNEL_7));

    /*
     * Color math: subtract fixed color INSIDE window area
     *
     * REG_CGWSEL ($2130):
     *   bits 5:4 = 01: enable color math inside color window only
     *   bit 1 = 0: use fixed color (not subscreen)
     *   = 0x10
     *
     * REG_CGADSUB ($2131):
     *   bit 7 = 1: subtract mode
     *   bit 1 = 1: apply to BG2
     *   = 0x82
     */
    REG_CGWSEL = 0x10;
    REG_CGADSUB = 0x82;

    /* Fixed color: subtract RGB intensity 12 (darker inside window) */
    REG_COLDATA = 0x20 | 12;   /* Red, intensity 12 */
    REG_COLDATA = 0x40 | 12;   /* Green, intensity 12 */
    REG_COLDATA = 0x80 | 12;   /* Blue, intensity 12 */

    /* Enable color math window 1 (REG_WOBJSEL bit 5 = color W1 enable) */
    REG_WOBJSEL = 0x20;

    /* Don't mask any BG layers with window (no BG clipping) */
    REG_TMW = 0;
}

int main(void) {
    consoleInit();

    /* Load BG2 tiles at VRAM $4000, palette slot 0 */
    bgInitTileSet(1, tiles, palette, 0,
                  tiles_end - tiles, 16 * 2,
                  BG_16COLORS, 0x4000);

    /* Load tilemap at VRAM $0000 */
    bgSetMapPtr(1, 0x0000, BG_MAP_32x32);
    dmaCopyVram(tilemap, 0x0000, tilemap_end - tilemap);

    /* Create transparent rectangle window */
    create_transparent_window(40, 96, 176, 112);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG2;
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
