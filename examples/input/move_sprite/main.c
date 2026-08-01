/**
 * @file main.c
 * @brief Family 4 (Input) · rung 4.2 — drive a sprite with the pad
 * @ingroup examples
 *
 * The rung that turns a demo into a game: read the D-pad every frame and move
 * a sprite with it. It reuses the whole simple_sprite setup — tiles to VRAM,
 * palette to CGRAM 128, OBJSEL — and adds one thing, the input loop. The
 * lesson: padHeld(0) hands you the held-button bitmask; act on it, update the
 * sprite's position with oamSetXY, and the player is in control.
 *
 * @par SNES Concepts
 * - padHeld(0) returns a 16-bit held-button bitmask (the NMI auto-reads the pad)
 * - KEY_UP / KEY_DOWN / KEY_LEFT / KEY_RIGHT are the D-pad bits
 * - oamSetXY(id, x, y) updates only a sprite's position (tile and size stay put)
 * - Position is 9-bit X / 8-bit Y, so walking off an edge wraps — the OAM
 *   coordinate quirk, visible here for free
 *
 * @par What to Observe
 * - A 32x32 sprite you steer with the D-pad; hold a direction to glide, and
 *   walk off an edge to see it wrap around
 *
 * @par Modules Used
 * console, dma, sprite, input
 *
 * @see input.h, sprite.h, tutorial_input
 */

#include <snes.h>

/** @brief 4bpp 32x32 sprite tile data (data.asm, in ROM) */
extern u8 sprite32[], sprite32_end[];
/** @brief 16-colour palette for the sprite */
extern u8 palsprite32[];

/** @brief Pixels moved per frame while a direction is held. */
#define SPEED 2

/**
 * @brief Entry point — set up one sprite, then steer it with the D-pad.
 *
 * The setup is identical to simple_sprite. The only new part is the loop:
 * read the pad, nudge (x, y), push the new position to OAM, wait a frame.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    u16 x = 112;   /* 9-bit sprite X */
    u8  y = 96;    /* 8-bit sprite Y */

    consoleInit();
    setMode(BG_MODE1, 0);

    WaitForVBlank();
    dmaCopyVram(sprite32, 0x2100, sprite32_end - sprite32);
    dmaCopyCGram(palsprite32, OBJ_CGRAM_BASE, PALETTE_16_SIZE);

    oamInit(OBJ_SIZE8_L32, 1);
    oamSet(0, x, y, 0x0010, 0, 3, 0);
    oamSetSize(0, OBJ_LARGE);

    setMainScreen(LAYER_OBJ);
    setScreenOn();

    while (1) {
        u16 pad = padHeld(0);

        if (pad & KEY_LEFT)  x -= SPEED;
        if (pad & KEY_RIGHT) x += SPEED;
        if (pad & KEY_UP)    y -= SPEED;
        if (pad & KEY_DOWN)  y += SPEED;

        /* Update position only — the NMI DMAs the OAM buffer this VBlank. */
        oamSetXY(0, x, y);
        WaitForVBlank();
    }

    return 0;
}
