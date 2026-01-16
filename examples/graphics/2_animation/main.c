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
#define REG_OAMADDL  (*(volatile u8*)0x2102)
#define REG_OAMADDH  (*(volatile u8*)0x2103)
#define REG_OAMDATA  (*(volatile u8*)0x2104)
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

    /* Write tiles byte by byte to avoid compiler bug with i+1 indexing */
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
    /* Cast to byte pointer - avoids compiler bug with >> 8 shift */
    const u8 *pal_bytes = (const u8 *)sprites_pal;

    REG_CGADD = 128;  /* Sprite palettes start at 128 */

    /* Write palette as bytes (little endian: low byte first) */
    for (i = 0; i < sprites_PAL_COUNT * 2; i++) {
        REG_CGDATA = pal_bytes[i];
    }
}

static void set_background_color(void) {
    /* Set background color (CGRAM index 0) */
    /* Use a nice dark blue: RGB(4, 6, 14) in 5-bit = 0x38C4 */
    /* SNES format: 0bbbbbgg gggrrrrr */
    /* Pre-computed: (14 << 10) | (6 << 5) | 4 = 0x38C4 */
    REG_CGADD = 0;
    REG_CGDATA = 0xC4;  /* Low byte */
    REG_CGDATA = 0x38;  /* High byte */
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

    /* Initial OAM update before screen on */
    update_oam();

    /* Enable sprites on main screen */
    REG_TM = 0x10;

    /* Screen on */
    REG_INIDISP = 0x0F;

    /* Main loop */
    while (1) {
        /* Read joypad */
        read_pad();

        /* Update monster position/animation/tile based on input */
        update_monster();

        /* Update OAM buffer - will be DMA'd during next VBlank */
        update_oam();

        /* Wait for VBlank (NMI handler transfers OAM) */
        wait_vblank();
    }

    return 0;
}
