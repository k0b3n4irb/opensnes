/**
 * @file main.c
 * @brief Continuous Scroll Example (Pure C Version)
 *
 * Port of PVSnesLib Mode1ContinuousScroll example.
 * Demonstrates:
 * - Two-layer parallax scrolling (BG1 + BG2)
 * - Player-controlled sprite movement
 * - D-pad controlled scrolling
 * - VBlank callback (nmiSet) for timing-critical scroll updates
 * - Pure C graphics loading using library functions
 *
 * Original by odelot
 * Sprite from Calciumtrice (CC-BY 3.0)
 * Backgrounds inspired by Streets of Rage 2
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

/* BG1 - Main scrolling background */
extern u8 bg1_tiles[], bg1_tiles_end[];
extern u8 bg1_pal[], bg1_pal_end[];
extern u8 bg1_map[], bg1_map_end[];

/* BG2 - Sub scrolling background (parallax) */
extern u8 bg2_tiles[], bg2_tiles_end[];
extern u8 bg2_pal[], bg2_pal_end[];
extern u8 bg2_map[], bg2_map_end[];

/* BG3 assets not used in this example */

/* Character sprite */
extern u8 char_tiles[], char_tiles_end[];
extern u8 char_pal[], char_pal_end[];

/*============================================================================
 * Game State
 *============================================================================*/

/* Scroll positions */
static u16 bg1_scroll_x;
static u16 bg1_scroll_y;
static u16 bg2_scroll_x;
static u16 bg2_scroll_y;

/* Player position (screen coordinates) */
static u16 player_x;
static u16 player_y;

/* Scroll limits - how far the background can scroll */
#define MAX_SCROLL_X 512   /* Maximum horizontal scroll (depends on tilemap size) */
#define SCROLL_THRESHOLD_RIGHT 140  /* When player X > this, scroll right */
#define SCROLL_THRESHOLD_LEFT  80   /* When player X < this, scroll left */

/* Flag to signal scroll update needed */
static u8 need_scroll_update;

/*============================================================================
 * VBlank Callback
 *============================================================================*/

/**
 * VBlank callback - updates scroll registers at the start of VBlank
 *
 * Scroll register updates should happen during VBlank to avoid
 * visual glitches. The VBlank callback is the perfect place for this.
 */
void myVBlankHandler(void) {
    if (need_scroll_update) {
        /* Update scroll registers during VBlank for glitch-free scrolling */
        bgSetScroll(0, bg1_scroll_x, bg1_scroll_y);
        bgSetScroll(1, bg2_scroll_x, bg2_scroll_y);
        need_scroll_update = 0;
    }
}

/*============================================================================
 * Main Program
 *============================================================================*/

int main(void) {
    u16 pad;

    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /*------------------------------------------------------------------------
     * Configure Background Tilemaps (where tilemap data goes in VRAM)
     *------------------------------------------------------------------------*/

    /* BG1 tilemap at VRAM $0000, 32x32 tiles */
    bgSetMapPtr(0, 0x0000, SC_32x32);

    /* BG2 tilemap at VRAM $0800, 32x32 tiles */
    bgSetMapPtr(1, 0x0800, SC_32x32);

    /* BG3 disabled - this example focuses on parallax scrolling */

    /*------------------------------------------------------------------------
     * Load Background Tiles and Palettes
     *------------------------------------------------------------------------*/

    /* BG1: tiles at $2000, palette at slot 2 (offset 32)
     * BG1 tiles = 7552 bytes, occupies $2000-$3D7F */
    bgInitTileSet(0, bg1_tiles, bg1_pal, 2,
                  bg1_tiles_end - bg1_tiles,
                  bg1_pal_end - bg1_pal,
                  BG_16COLORS, 0x2000);

    /* BG2: tiles at $4000, palette at slot 4 (offset 64)
     * Must not overlap with BG1! */
    bgInitTileSet(1, bg2_tiles, bg2_pal, 4,
                  bg2_tiles_end - bg2_tiles,
                  bg2_pal_end - bg2_pal,
                  BG_16COLORS, 0x4000);

    /* BG3 disabled - not loading tiles */

    /*------------------------------------------------------------------------
     * Load Tilemap Data
     *------------------------------------------------------------------------*/

    /* Load initial tilemaps (first 2KB of each) */
    dmaCopyVram(bg1_map, 0x0000, 2048);
    dmaCopyVram(bg2_map, 0x0800, 2048);

    /*------------------------------------------------------------------------
     * Load Sprite Graphics
     *------------------------------------------------------------------------*/

    /* Sprite tiles at $6000, palette 0 (colors 128-143) */
    oamInitGfxSet(char_tiles, char_tiles_end - char_tiles,
                  char_pal, char_pal_end - char_pal,
                  0, 0x6000, OBJ_SIZE16_L32);

    /*------------------------------------------------------------------------
     * Configure Video Mode
     *------------------------------------------------------------------------*/

    /* Mode 1 for parallax scrolling */
    setMode(BG_MODE1, 0);

    /* Enable BG1, BG2, and sprites on main screen */
    REG_TM = 0x13;  /* TM = 00010011 = OBJ + BG2 + BG1 */

    /*------------------------------------------------------------------------
     * Initialize Game State
     *------------------------------------------------------------------------*/

    player_x = 20;
    player_y = 100;
    bg1_scroll_x = 0;
    bg1_scroll_y = 32;
    bg2_scroll_x = 0;
    bg2_scroll_y = 32;
    need_scroll_update = 1;

    /* Register VBlank callback for scroll updates */
    /* Note: Use nmiSetBank with bank 1 because myVBlankHandler is placed in bank 1 */
    /* by the linker. In LoROM, bank 1 = ROM $8000-$FFFF (second 32KB). */
    nmiSetBank(myVBlankHandler, 1);

    /* Set initial sprite - tile 0, palette 0, priority 2 */
    oamSet(0, player_x, (u8)player_y, 0, 0, 2, 0);

    /* Transfer OAM buffer to hardware before turning screen on */
    oamUpdate();

    /* Enable display at full brightness */
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /*------------------------------------------------------------------------
     * Main Loop
     *------------------------------------------------------------------------*/

    while (1) {
        /* Wait for auto-joypad read to complete */
        while (REG_HVBJOY & 0x01) { }

        /* Read joypad directly from hardware */
        pad = REG_JOY1L | (REG_JOY1H << 8);

        /* Handle vertical movement - player moves freely */
        if (pad & KEY_UP) {
            if (player_y > 32) player_y -= 2;
        }
        if (pad & KEY_DOWN) {
            if (player_y < 200) player_y += 2;
        }

        /* Handle horizontal movement - player always moves when pressing D-pad */
        if (pad & KEY_LEFT) {
            if (player_x > 8) player_x -= 2;
        }
        if (pad & KEY_RIGHT) {
            if (player_x < 230) player_x += 2;
        }

        /* PVSnesLib-style auto-scroll: when player passes threshold, scroll background */
        /* and push player back to keep them visually centered */
        if (player_x > SCROLL_THRESHOLD_RIGHT && bg1_scroll_x < MAX_SCROLL_X) {
            bg1_scroll_x += 1;
            bg2_scroll_x += 1;  /* Parallax: could use slower increment */
            player_x -= 1;      /* Push player back to keep them in place */
        }
        if (player_x < SCROLL_THRESHOLD_LEFT && bg1_scroll_x > 0) {
            bg1_scroll_x -= 1;
            bg2_scroll_x -= 1;
            player_x += 1;      /* Push player forward to keep them in place */
        }

        /* Update sprite position */
        oamSetXY(0, player_x, (u8)player_y);

        /* Signal scroll update - VBlank callback will apply it */
        need_scroll_update = 1;

        /* Wait for VBlank and update OAM */
        WaitForVBlank();
        oamUpdate();
    }

    return 0;
}
