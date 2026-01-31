/**
 * @file main.c
 * @brief Animated Sprite Example
 *
 * Port of pvsneslib AnimatedSprite example.
 * Move sprite with D-pad, sprite animates while moving.
 *
 * Sprite from Stephen "Redshrike" Challener, http://opengameart.org
 */

#include <snes.h>
#include "assets/sprites.h"

/*============================================================================
 * Game State Variables (defined in helpers.asm)
 *============================================================================*/

extern u16 monster_x;
extern u16 monster_y;
extern u8 monster_state;
extern u8 monster_anim;
extern u8 monster_flipx;
extern u8 monster_tile;

/*============================================================================
 * Assembly Functions (in helpers.asm)
 *============================================================================*/

extern void game_init(void);
extern void calc_tile(void);

/*============================================================================
 * Sprite Tile Lookup Table (in helpers.asm)
 *============================================================================*/

extern const u8 sprite_tiles[];

/*============================================================================
 * Graphics Loading
 *============================================================================*/

static void load_sprite_tiles(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    for (i = 0; i < sprites_TILES_SIZE; i++) {
        u8 byte = sprites_tiles[i];
        if ((i & 1) == 0) {
            REG_VMDATAL = byte;
        } else {
            REG_VMDATAH = byte;
        }
    }
}

static void load_sprite_palette(void) {
    u8 i;
    const u8 *pal_bytes = (const u8 *)sprites_pal;

    REG_CGADD = 128;  /* Sprite palettes start at 128 */

    for (i = 0; i < sprites_PAL_COUNT * 2; i++) {
        REG_CGDATA = pal_bytes[i];
    }
}

static void set_background_color(void) {
    /* Set background color (CGRAM index 0) */
    /* Use a nice dark blue: RGB(4, 6, 14) in 5-bit = 0x38C4 */
    REG_CGADD = 0;
    REG_CGDATA = 0xC4;
    REG_CGDATA = 0x38;
}

/*============================================================================
 * Game Logic
 *============================================================================*/

static void handle_input(void) {
    /* Wait for auto-joypad read to complete */
    while (REG_HVBJOY & 0x01) { }

    /* Read directly from hardware */
    u16 pad = REG_JOY1L | (REG_JOY1H << 8);

    if (pad == 0) {
        return;  /* No input */
    }

    /* Handle UP */
    if (pad & KEY_UP) {
        monster_state = 1;
        monster_flipx = 0;
        if (monster_y > 0) {
            monster_y = monster_y - 1;
        }
    }

    /* Handle LEFT */
    if (pad & KEY_LEFT) {
        monster_state = 2;
        monster_flipx = 1;
        if (monster_x > 0) {
            monster_x = monster_x - 1;
        }
    }

    /* Handle RIGHT */
    if (pad & KEY_RIGHT) {
        monster_state = 2;
        monster_flipx = 0;
        if (monster_x < 255) {
            monster_x = monster_x + 1;
        }
    }

    /* Handle DOWN */
    if (pad & KEY_DOWN) {
        monster_state = 0;
        monster_flipx = 0;
        if (monster_y < 223) {
            monster_y = monster_y + 1;
        }
    }

    /* Advance animation */
    monster_anim = monster_anim + 1;
    if (monster_anim >= 3) {
        monster_anim = 0;
    }

    /* Calculate tile using assembly (avoids multiplication) */
    calc_tile();
}

static void update_sprite(void) {
    /* Set sprite 0 properties using library function */
    u8 flags = monster_flipx ? 0x40 : 0x00;  /* Horizontal flip */
    oamSet(0, monster_x, (u8)monster_y, monster_tile, 0, 3, flags);

    /* Hide sprite 1 */
    oamHide(1);
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    /* Initialize sprite system: 16x16 small / 32x32 large */
    oamInitEx(OBJ_SIZE16_L32, 0);

    /* Load graphics */
    load_sprite_tiles();
    load_sprite_palette();
    set_background_color();

    /* Initialize game state */
    game_init();

    /* Initial sprite update before screen on */
    update_sprite();
    oamUpdate();

    /* Enable sprites on main screen */
    REG_TM = 0x10;

    /* Screen on */
    REG_INIDISP = 0x0F;

    /* Main loop */
    while (1) {
        /* Handle input (reads joypad directly inside) */
        handle_input();

        /* Update sprite in OAM buffer */
        update_sprite();

        /* Wait for VBlank and transfer OAM */
        WaitForVBlank();
        oamUpdate();
    }

    return 0;
}
