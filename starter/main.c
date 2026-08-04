/**
 * @file main.c
 * @brief OpenSNES starter project — a movable sprite you can build on.
 *
 * This is a complete, running SNES game in ~50 lines: it loads one sprite,
 * then steers it with the D-pad every frame. Use it as the seed for your own
 * game — the three parts you will edit are marked below.
 *
 * The three parts of every SNES game:
 *   1. SETUP   — runs once, during force blank (screen off): load graphics
 *                into VRAM and palettes into CGRAM.
 *   2. SCREEN ON — the last setup step; never write VRAM after this except
 *                during VBlank.
 *   3. GAME LOOP — runs forever: read input, update state, wait for VBlank.
 *
 * @see https://github.com/k0b3n4irb/opensnes — docs, tutorials, 82 examples
 */

#include <snes.h>

/* Sprite graphics, converted from res/player.png by gfx4snes at build time
 * and .incbin'd via data.asm. */
extern u8 player[], player_end[];
extern u8 player_pal[];

/** Pixels the player moves per frame while a direction is held. */
#define SPEED 2

int main(void) {
    /* --- your game's state lives here --- */
    u16 x = 112;   /* sprite X (9-bit: 0..511) */
    u8  y = 96;    /* sprite Y (8-bit: 0..255) */

    /* === 1. SETUP (force blank) ============================================ */
    consoleInit();
    setMode(BG_MODE1, 0);

    WaitForVBlank();
    /* Upload the sprite tiles to VRAM and its palette to sprite CGRAM. */
    dmaCopyVram(player, 0x2100, player_end - player);
    dmaCopyCGram(player_pal, OBJ_CGRAM_BASE, PALETTE_16_SIZE);

    oamInit(OBJ_SIZE8_L32, 1);
    oamSet(0, x, y, 0x0010, 0, 3, 0);   /* sprite 0: tile 0x10, priority 3 */
    oamSetSize(0, OBJ_LARGE);

    setMainScreen(LAYER_OBJ);

    /* === 2. SCREEN ON (always last in setup) ============================== */
    setScreenOn();

    /* === 3. GAME LOOP ===================================================== */
    while (1) {
        u16 pad = padHeld(0);   /* held-button bitmask; the NMI reads the pad */

        /* --- your game logic goes here --- */
        if (pad & KEY_LEFT)  x -= SPEED;
        if (pad & KEY_RIGHT) x += SPEED;
        if (pad & KEY_UP)    y -= SPEED;
        if (pad & KEY_DOWN)  y += SPEED;

        oamSetXY(0, x, y);      /* push the new position; NMI DMAs OAM at VBlank */

        WaitForVBlank();        /* one frame at 60 Hz (50 on PAL) */
    }

    return 0;
}
