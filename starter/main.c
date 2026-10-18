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
 * test/ holds two luna tests of it: `make test` runs them (and `make
 * test-update` records the picture the first one checks).
 *
 * @see https://github.com/k0b3n4irb/opensnes — docs, tutorials, examples
 */

#include <snes.h>

/* Sprite graphics: res/player.png, converted at build time by opensnes-sprite
 * from the settings in res/player.png.toml; the .incbin is generated too. */
#include "res/player.inc"   /* generated: player_tiles[], player_tiles_end[], player_pal[] (and a GfxAsset `player`) */

/** Pixels the player moves per frame while a direction is held. */
#define SPEED 2

/* --- your game's state lives here (globals: the tests read them by name) --- */
static s16 player_x = 112;   /* sprite X */
static s16 player_y = 96;    /* sprite Y */

int main(void) {
    /* === 1. SETUP (force blank) ============================================ */
    consoleInit();
    setMode(BG_MODE1, 0);

    WaitForVBlank();
    /* Upload the sprite tiles to VRAM and its palette to sprite CGRAM. */
    dmaCopyVram(player_tiles, 0x2100, player_tiles_end - player_tiles);
    dmaCopyCGram(player_pal, OBJ_CGRAM_BASE, PALETTE_16_SIZE);

    oamInit(OBJ_SIZE8_L32, 1);
    oamSet(0, player_x, player_y, 0x0010, 0, 3, 0);   /* sprite 0: tile 0x10, priority 3 */
    oamSetSize(0, OBJ_LARGE);

    setMainScreen(LAYER_OBJ);

    /* === 2. SCREEN ON (always last in setup) ============================== */
    setScreenOn();

    /* === 3. GAME LOOP ===================================================== */
    while (1) {
        u16 pad = padHeld(0);   /* held-button bitmask; the NMI reads the pad */

        /* --- your game logic goes here --- */
        if (pad & KEY_LEFT)  player_x -= SPEED;
        if (pad & KEY_RIGHT) player_x += SPEED;
        if (pad & KEY_UP)    player_y -= SPEED;
        if (pad & KEY_DOWN)  player_y += SPEED;

        oamSetXY(0, player_x, player_y);   /* the new position; the NMI sends OAM at VBlank */

        WaitForVBlank();        /* one frame at 60 Hz (50 on PAL) */
    }

    return 0;
}
