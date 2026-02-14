/**
 * @file main.c
 * @brief Background Scrolling Demo with Parallax
 *
 * Demonstrates SNES background scrolling with parallax effect.
 * Two backgrounds scroll at different speeds to create depth.
 * Uses real image assets for a clear visual demonstration.
 *
 * Controls:
 *   D-pad: Scroll the view
 *   A: Auto-scroll mode toggle
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

/* Foreground layer (shader) - scrolls at full speed */
extern u8 shader_tiles[], shader_tiles_end[];
extern u8 shader_map[], shader_map_end[];
extern u8 shader_pal[], shader_pal_end[];

/* Background layer (pvsneslib logo) - scrolls at half speed */
extern u8 bg_tiles[], bg_tiles_end[];
extern u8 bg_map[], bg_map_end[];
extern u8 bg_pal[], bg_pal_end[];

/* Scroll positions */
s16 scroll_x;
s16 scroll_y;
u8 auto_scroll;

int main(void) {
    u16 pad;
    u16 pad_prev;
    u16 pad_pressed;

    /* Initialize */
    scroll_x = 0;
    scroll_y = 0;
    auto_scroll = 1;  /* Start with auto-scroll on */

    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /*------------------------------------------------------------------------
     * Configure Background Tilemaps
     *------------------------------------------------------------------------*/

    /* BG1 (foreground) tilemap at VRAM $1800, 32x32 tiles */
    bgSetMapPtr(0, 0x1800, SC_32x32);

    /* BG2 (background) tilemap at VRAM $1400, 32x32 tiles */
    bgSetMapPtr(1, 0x1400, SC_32x32);

    /*------------------------------------------------------------------------
     * Load Background Tiles and Palettes
     *------------------------------------------------------------------------*/

    /* BG1: tiles at $4000, palette at slot 1 (offset 16) */
    bgInitTileSet(0, shader_tiles, shader_pal, 1,
                  shader_tiles_end - shader_tiles,
                  shader_pal_end - shader_pal,
                  BG_16COLORS, 0x4000);

    /* BG2: tiles at $5000, palette at slot 0 (offset 0) */
    bgInitTileSet(1, bg_tiles, bg_pal, 0,
                  bg_tiles_end - bg_tiles,
                  bg_pal_end - bg_pal,
                  BG_16COLORS, 0x5000);

    /*------------------------------------------------------------------------
     * Load Tilemap Data
     *------------------------------------------------------------------------*/

    /* Clear tilemap VRAM areas first (prevents garbage when scrolling
     * beyond the actual tilemap data - pvsneslib.bmp is only 224 pixels tall) */
    dmaFillVRAM(0, 0x1400, 2048);  /* BG2 tilemap: 32x32 tiles = 2KB */
    dmaFillVRAM(0, 0x1800, 2048);  /* BG1 tilemap: 32x32 tiles = 2KB */

    dmaCopyVram(shader_map, 0x1800, shader_map_end - shader_map);
    dmaCopyVram(bg_map, 0x1400, bg_map_end - bg_map);

    /*------------------------------------------------------------------------
     * Configure Video Mode
     *------------------------------------------------------------------------*/

    /* Set Mode 1 (two 16-color BGs + one 4-color BG) */
    setMode(BG_MODE1, 0);

    /* Enable BG1 and BG2 on main screen */
    REG_TM = TM_BG1 | TM_BG2;

    /* Set initial scroll (0,0) */
    bgSetScroll(0, 0, 0);
    bgSetScroll(1, 0, 0);

    /* Enable display at full brightness */
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Initialize pad reading */
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}
    pad_prev = REG_JOY1L | (REG_JOY1H << 8);

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Wait for auto-joypad */
        while (REG_HVBJOY & 0x01) {}

        /* Read controller */
        pad = REG_JOY1L | (REG_JOY1H << 8);
        pad_pressed = pad & ~pad_prev;
        pad_prev = pad;

        if (pad == 0xFFFF) pad_pressed = 0;

        /* Toggle auto-scroll with A */
        if (pad_pressed & KEY_A) {
            auto_scroll = !auto_scroll;
        }

        /* Auto-scroll or manual D-pad control */
        if (auto_scroll) {
            scroll_x++;
            scroll_y++;
        } else {
            if (pad & KEY_UP)    { scroll_y--; }
            if (pad & KEY_DOWN)  { scroll_y++; }
            if (pad & KEY_LEFT)  { scroll_x--; }
            if (pad & KEY_RIGHT) { scroll_x++; }
        }

        /* Update BG1 scroll (foreground - full speed) */
        REG_BG1HOFS = (u8)scroll_x;
        REG_BG1HOFS = (u8)(scroll_x >> 8);
        REG_BG1VOFS = (u8)scroll_y;
        REG_BG1VOFS = (u8)(scroll_y >> 8);

        /* Update BG2 scroll (background - half speed for parallax) */
        REG_BG2HOFS = (u8)(scroll_x >> 1);
        REG_BG2HOFS = (u8)((scroll_x >> 1) >> 8);
        REG_BG2VOFS = (u8)(scroll_y >> 1);
        REG_BG2VOFS = (u8)((scroll_y >> 1) >> 8);
    }

    return 0;
}
