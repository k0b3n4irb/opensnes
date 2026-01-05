/**
 * @file main.c
 * @brief Sprite Display Example
 *
 * Displays a 16x16 sprite on screen.
 * Self-contained example - no library dependencies.
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

/* Include generated sprite data */
#include "sprite.h"

/*============================================================================
 * Functions
 *============================================================================*/

static void wait_vblank(void) {
    /* Wait until not in VBlank (in case we're already there) */
    while (REG_HVBJOY & 0x80) {}
    /* Wait until VBlank starts */
    while (!(REG_HVBJOY & 0x80)) {}
}

static void load_sprite_tiles(void) {
    u16 i;

    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    for (i = 0; i < sprite_TILES_SIZE; i += 2) {
        REG_VMDATAL = sprite_tiles[i];
        REG_VMDATAH = sprite_tiles[i + 1];
    }
}

static void load_sprite_palette(void) {
    u8 i;

    REG_CGADD = 128;

    for (i = 0; i < sprite_PAL_COUNT; i++) {
        u16 color = sprite_pal[i];
        REG_CGDATA = color & 0xFF;
        REG_CGDATA = (color >> 8) & 0xFF;
    }
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 i;

    /* Set sprite size mode: small=8x8, large=16x16 */
    REG_OBJSEL = 0x00;

    /* Load sprite graphics */
    load_sprite_tiles();
    load_sprite_palette();

    /* Initialize OAM before turning on screen */
    REG_OAMADDL = 0;
    REG_OAMADDH = 0;

    /* Sprite 0: X=120, Y=100, Tile=0, Attr=0x30 (priority 3) */
    REG_OAMDATA = 120;
    REG_OAMDATA = 100;
    REG_OAMDATA = 0;
    REG_OAMDATA = 0x30;

    /* Hide sprites 1-127 at Y=240 */
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

    /* Enable auto-joypad */
    REG_NMITIMEN = 0x81;

    /* Enable sprites on main screen */
    REG_TM = 0x10;

    /* Turn on screen */
    REG_INIDISP = 0x0F;

    /* Main loop - no movement for now, just display */
    while (1) {
        wait_vblank();
    }

    return 0;
}
