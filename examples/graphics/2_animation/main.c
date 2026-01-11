/**
 * @file main.c
 * @brief Animated Sprite Example
 *
 * Port of pvsneslib AnimatedSprite example.
 * Move sprite with D-pad, sprite animates while moving.
 *
 * Sprite from Stephen "Redshrike" Challener, http://opengameart.org
 */

typedef unsigned char u8;
typedef unsigned short u16;

/*============================================================================
 * Hardware Registers
 *============================================================================*/

#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_OBJSEL   (*(volatile u8*)0x2101)
#define REG_VMAIN    (*(volatile u8*)0x2115)
#define REG_VMADDL   (*(volatile u8*)0x2116)
#define REG_VMADDH   (*(volatile u8*)0x2117)
#define REG_VMDATAL  (*(volatile u8*)0x2118)
#define REG_VMDATAH  (*(volatile u8*)0x2119)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)
#define REG_TM       (*(volatile u8*)0x212C)

#include "sprites.h"

/*============================================================================
 * Assembly Functions (in helpers.asm)
 *============================================================================*/

extern void game_init(void);
extern void read_pad(void);
extern void update_monster(void);
extern void update_oam(void);
extern void wait_vblank(void);

/*============================================================================
 * Graphics Loading
 *============================================================================*/

static void load_sprite_tiles(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    for (i = 0; i < sprites_TILES_SIZE; i += 2) {
        REG_VMDATAL = sprites_tiles[i];
        REG_VMDATAH = sprites_tiles[i + 1];
    }
}

static void load_sprite_palette(void) {
    u8 i;
    REG_CGADD = 128;  /* Sprite palettes start at 128 */

    for (i = 0; i < sprites_PAL_COUNT; i++) {
        u16 color = sprites_pal[i];
        REG_CGDATA = color & 0xFF;
        REG_CGDATA = (color >> 8) & 0xFF;
    }
}

static void set_background_color(void) {
    /* Set background color (CGRAM index 0) */
    /* Use a nice dark blue: RGB(4, 6, 14) in 5-bit = 0x1CC4 */
    /* SNES format: 0bbbbbgg gggrrrrr */
    u16 color = (14 << 10) | (6 << 5) | 4;  /* Dark blue */
    REG_CGADD = 0;
    REG_CGDATA = color & 0xFF;
    REG_CGDATA = (color >> 8) & 0xFF;
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    /* Configure sprites: 16x16 small / 32x32 large */
    REG_OBJSEL = 0x60;

    /* Load graphics */
    load_sprite_tiles();
    load_sprite_palette();
    set_background_color();

    /* Initialize game state */
    game_init();

    /* Enable sprites on main screen */
    REG_TM = 0x10;

    /* Screen on */
    REG_INIDISP = 0x0F;

    /* Main loop - order matches pvsneslib */
    while (1) {
        /* Read joypad first */
        read_pad();

        /* Update monster position/animation/tile based on input */
        update_monster();

        /* Wait for VBlank */
        wait_vblank();

        /* Update OAM during VBlank with the freshly calculated tile */
        update_oam();
    }

    return 0;
}
