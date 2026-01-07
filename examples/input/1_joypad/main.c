/**
 * @file main.c
 * @brief Joypad Input Example
 *
 * Move a sprite using the D-pad. Demonstrates controller reading.
 *
 * Controls:
 *   D-pad: Move sprite
 *   A/B: Speed up movement (hold)
 *
 * @author OpenSNES Team
 * @license CC0 (Public Domain)
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
#define REG_NMITIMEN (*(volatile u8*)0x4200)
#define REG_HVBJOY   (*(volatile u8*)0x4212)
#define REG_JOY1L    (*(volatile u8*)0x4218)
#define REG_JOY1H    (*(volatile u8*)0x4219)

/* Joypad button masks (active high after reading) */
#define JOY_B       0x8000
#define JOY_Y       0x4000
#define JOY_SELECT  0x2000
#define JOY_START   0x1000
#define JOY_UP      0x0800
#define JOY_DOWN    0x0400
#define JOY_LEFT    0x0200
#define JOY_RIGHT   0x0100
#define JOY_A       0x0080
#define JOY_X       0x0040
#define JOY_L       0x0020
#define JOY_R       0x0010

/* Include generated sprite data */
#include "player.h"

/*============================================================================
 * Assembly Helpers (defined in crt0.asm)
 *============================================================================*/

extern void oam_set_pos(u8 x, u8 y);

/*============================================================================
 * Functions
 *============================================================================*/

static void wait_vblank(void) {
    while (REG_HVBJOY & 0x80) {}
    while (!(REG_HVBJOY & 0x80)) {}
}

static u16 read_joypad(void) {
    /* Wait for auto-read to complete */
    while (REG_HVBJOY & 0x01) {}
    return REG_JOY1L | (REG_JOY1H << 8);
}

static void load_sprite_tiles(void) {
    u16 i;

    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    for (i = 0; i < player_TILES_SIZE; i += 2) {
        REG_VMDATAL = player_tiles[i];
        REG_VMDATAH = player_tiles[i + 1];
    }
}

static void load_sprite_palette(void) {
    u8 i;

    REG_CGADD = 128;

    for (i = 0; i < player_PAL_COUNT; i++) {
        u16 color = player_pal[i];
        REG_CGDATA = color & 0xFF;
        REG_CGDATA = (color >> 8) & 0xFF;
    }
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 i;
    u16 joy;
    u8 speed;
    u8 sprite_x;
    u8 sprite_y;

    /* Sprite size: small=8x8, large=16x16 */
    REG_OBJSEL = 0x00;

    /* Load graphics */
    load_sprite_tiles();
    load_sprite_palette();

    /* Initial sprite position (center of screen) */
    sprite_x = 120;
    sprite_y = 104;

    /* Initialize OAM */
    REG_OAMADDL = 0;
    REG_OAMADDH = 0;

    /* Sprite 0: initial position, tile 0, priority 3 */
    REG_OAMDATA = sprite_x;
    REG_OAMDATA = sprite_y;
    REG_OAMDATA = 0;
    REG_OAMDATA = 0x30;

    /* Hide sprites 1-127 */
    for (i = 1; i < 128; i++) {
        REG_OAMDATA = 0;
        REG_OAMDATA = 240;
        REG_OAMDATA = 0;
        REG_OAMDATA = 0;
    }

    /* High table: sprite 0 = large (16x16) */
    REG_OAMDATA = 0x02;
    for (i = 1; i < 32; i++) {
        REG_OAMDATA = 0;
    }

    /* Enable NMI and auto-joypad */
    REG_NMITIMEN = 0x81;

    /* Enable sprites on main screen */
    REG_TM = 0x10;

    /* Turn on screen */
    REG_INIDISP = 0x0F;

    /* Main loop */
    while (1) {
        wait_vblank();

        /* Read controller */
        joy = read_joypad();

        /* Base speed = 1, hold A or B for speed 3 */
        speed = 1;
        if (joy & (JOY_A | JOY_B)) {
            speed = 3;
        }

        /* Move based on D-pad */
        if (joy & JOY_UP) {
            if (sprite_y > speed) {
                sprite_y = sprite_y - speed;
            } else {
                sprite_y = 0;
            }
        }
        if (joy & JOY_DOWN) {
            if (sprite_y < 224 - 16 - speed) {
                sprite_y = sprite_y + speed;
            } else {
                sprite_y = 224 - 16;
            }
        }
        if (joy & JOY_LEFT) {
            if (sprite_x > speed) {
                sprite_x = sprite_x - speed;
            } else {
                sprite_x = 0;
            }
        }
        if (joy & JOY_RIGHT) {
            if (sprite_x < 240 - speed) {
                sprite_x = sprite_x + speed;
            } else {
                sprite_x = 240;
            }
        }

        /* Update sprite position */
        oam_set_pos(sprite_x, sprite_y);
    }

    return 0;
}
