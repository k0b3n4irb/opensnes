/**
 * @file main.c
 * @brief Sprite Animation Example
 *
 * Demonstrates actual sprite animation on SNES.
 * A single sprite cycles through 4 walk animation frames.
 *
 * Note: Uses assembly helper for OAM updates due to 816-tcc
 * code generation issues with volatile pointer writes.
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

#include "spritesheet.h"

/*============================================================================
 * Assembly Helper (defined in crt0.asm)
 *============================================================================*/

/* Set sprite 0's tile number - works around 816-tcc volatile write bug */
extern void oam_set_tile(u8 tile);

/*============================================================================
 * Animation
 *============================================================================*/

#define ANIM_SPEED  8   /* Frames between animation updates */

/* Tile numbers for each animation frame */
#define FRAME_0_TILE  0
#define FRAME_1_TILE  2
#define FRAME_2_TILE  4
#define FRAME_3_TILE  6

/*============================================================================
 * Functions
 *============================================================================*/

static void wait_vblank(void) {
    while (REG_HVBJOY & 0x80) {}
    while (!(REG_HVBJOY & 0x80)) {}
}

static void load_sprite_tiles(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    for (i = 0; i < spritesheet_TILES_SIZE; i += 2) {
        REG_VMDATAL = spritesheet_tiles[i];
        REG_VMDATAH = spritesheet_tiles[i + 1];
    }
}

static void load_sprite_palette(void) {
    u8 i;
    REG_CGADD = 128;

    for (i = 0; i < spritesheet_PAL_COUNT; i++) {
        u16 color = spritesheet_pal[i];
        REG_CGDATA = color & 0xFF;
        REG_CGDATA = (color >> 8) & 0xFF;
    }
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 i;
    u8 counter;
    u8 frame;
    u8 tile;

    REG_OBJSEL = 0x00;

    load_sprite_tiles();
    load_sprite_palette();

    /* Initialize OAM (works fine before screen on) */
    REG_OAMADDL = 0;
    REG_OAMADDH = 0;

    /* Sprite 0: X=120, Y=104, Tile=0, Priority=3 */
    REG_OAMDATA = 120;
    REG_OAMDATA = 104;
    REG_OAMDATA = 0;
    REG_OAMDATA = 0x30;

    /* Hide sprites 1-127 */
    for (i = 1; i < 128; i++) {
        REG_OAMDATA = 0;
        REG_OAMDATA = 240;
        REG_OAMDATA = 0;
        REG_OAMDATA = 0;
    }

    /* High table: sprite 0 = large */
    REG_OAMDATA = 0x02;
    for (i = 1; i < 32; i++) {
        REG_OAMDATA = 0;
    }

    REG_NMITIMEN = 0x81;
    REG_TM = 0x10;
    REG_INIDISP = 0x0F;

    counter = 0;
    frame = 0;

    /* Main loop with animation */
    while (1) {
        wait_vblank();

        counter++;
        if (counter >= ANIM_SPEED) {
            counter = 0;
            frame++;
            if (frame >= 4) {
                frame = 0;
            }

            /* Calculate tile for current frame */
            if (frame == 0) {
                tile = FRAME_0_TILE;
            } else if (frame == 1) {
                tile = FRAME_1_TILE;
            } else if (frame == 2) {
                tile = FRAME_2_TILE;
            } else {
                tile = FRAME_3_TILE;
            }

            /* Update sprite tile using assembly helper */
            oam_set_tile(tile);
        }
    }

    return 0;
}
