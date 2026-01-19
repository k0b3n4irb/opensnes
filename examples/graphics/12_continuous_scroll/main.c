/**
 * @file main.c
 * @brief Continuous Scroll Example
 *
 * Port of PVSnesLib Mode1ContinuousScroll example.
 * Demonstrates:
 * - Multi-layer parallax scrolling
 * - Player-controlled sprite movement
 * - D-pad controlled scrolling
 * - VBlank callback (nmiSet) for timing-critical scroll updates
 *
 * Original by odelot
 * Sprite from Calciumtrice (CC-BY 3.0)
 * Backgrounds inspired by Streets of Rage 2
 */

#include <snes.h>

/* External assembly functions */
extern void load_scroll_graphics(void);
extern void setup_scroll_regs(void);

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

int main(void) {
    u16 pad;

    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Load all graphics first */
    load_scroll_graphics();

    /* Configure PPU registers */
    setup_scroll_regs();

    /* Initialize sprite system with correct settings */
    /* OBJ_SIZE16_L32 = 16x16 small, 32x32 large */
    /* tileBase = 3 for sprites at VRAM $6000 ($6000 / $2000 = 3) */
    oamInitEx(OBJ_SIZE16_L32, 3);

    /* Initialize positions */
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

    /* Enable display */
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop */
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
