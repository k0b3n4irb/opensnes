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

/* External data from assembly */
extern void load_graphics(void);

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

    /* Load background graphics */
    load_graphics();

    /* Set Mode 1 (two 16-color BGs + one 4-color BG) */
    REG_BGMODE = 0x01;

    /* BG1 tilemap at $1800, 32x32 tiles */
    REG_BG1SC = 0x18;
    /* BG2 tilemap at $1400, 32x32 tiles */
    REG_BG2SC = 0x14;

    /* BG1 tiles at $4000, BG2 tiles at $5000 */
    /* Format: (BG2_addr >> 12) << 4 | (BG1_addr >> 12) */
    REG_BG12NBA = 0x54;

    /* Enable BG1 and BG2 on main screen */
    REG_TM = TM_BG1 | TM_BG2;

    /* Set initial scroll (0,0) */
    REG_BG1HOFS = 0; REG_BG1HOFS = 0;
    REG_BG1VOFS = 0; REG_BG1VOFS = 0;
    REG_BG2HOFS = 0; REG_BG2HOFS = 0;
    REG_BG2VOFS = 0; REG_BG2VOFS = 0;

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
