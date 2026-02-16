/**
 * @file main.c
 * @brief Animated Sprite Example
 *
 * Port of PVSnesLib AnimatedSprite demo.
 * Demonstrates sprite animation with direction states and horizontal flip.
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 sprite_tiles[], sprite_tiles_end[];
extern u8 sprite_pal[], sprite_pal_end[];

/*============================================================================
 * Constants
 *============================================================================*/

#define FRAMES_PER_ANIMATION 3

/* Animation states */
enum SpriteState {
    W_DOWN = 0,
    W_UP = 1,
    W_RIGHT = 2,
    W_LEFT = 2  /* Reuses W_RIGHT with flipx */
};

/* Screen boundaries */
enum {
    SCREEN_TOP = -16,
    SCREEN_BOTTOM = 224,
    SCREEN_LEFT = -16,
    SCREEN_RIGHT = 256
};

/*============================================================================
 * Game Data
 *============================================================================*/

/* Monster sprite structure */
typedef struct {
    s16 x, y;
    u16 gfx_frame;
    u16 anim_frame;
    u16 anim_delay;
    u8 state;
    u8 flipx;
} Monster;

/* Animation delay (frames to wait before advancing animation) */
#define ANIM_DELAY 6

Monster monster = {100, 100, 0, 0, 0, W_DOWN, 0};

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 pad0;

    /* Force blank during setup */
    setScreenOff();

    /* Initialize sprite graphics:
     * - Load tiles to VRAM $0000
     * - Load palette to CGRAM 128 (sprite palette 0)
     * - Configure 16x16 small / 32x32 large sprites
     */
    oamInitGfxSet(sprite_tiles, sprite_tiles_end - sprite_tiles,
                  sprite_pal, sprite_pal_end - sprite_pal,
                  0, 0x0000, OBJ_SIZE16_L32);

    /* Initial sprite setup */
    oamSet(0, monster.x, monster.y, 0, 0, 3, 0);
    oamSetSize(0, 0);       /* 0 = small size (16x16) */
    oamSetVisible(0, 1);

    /* Hide unused sprites */
    for (u8 i = 1; i < 128; i++) {
        oamHide(i);
    }

    /* Update OAM buffer to hardware */
    oamUpdate();

    /* Set Mode 1, enable sprites only */
    setMode(BG_MODE1, 0);
    REG_TM = TM_OBJ;

    /* Enable display at full brightness */
    setScreenOn();

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Wait for auto-joypad read to complete */
        while (REG_HVBJOY & 0x01) {}

        /* Read controller directly from hardware registers */
        pad0 = REG_JOY1L | (REG_JOY1H << 8);

        /* Disconnected controller reads as $FFFF */
        if (pad0 != 0xFFFF && pad0 != 0) {
            /* Handle movement */
            if (pad0 & KEY_UP) {
                if (monster.y >= SCREEN_TOP) monster.y--;
                monster.state = W_UP;
                monster.flipx = 0;
            }
            if (pad0 & KEY_LEFT) {
                if (monster.x >= SCREEN_LEFT) monster.x--;
                monster.state = W_LEFT;
                monster.flipx = 1;
            }
            if (pad0 & KEY_RIGHT) {
                if (monster.x <= SCREEN_RIGHT) monster.x++;
                monster.state = W_RIGHT;
                monster.flipx = 0;
            }
            if (pad0 & KEY_DOWN) {
                if (monster.y <= SCREEN_BOTTOM) monster.y++;
                monster.state = W_DOWN;
                monster.flipx = 0;
            }

            /* Advance animation with delay */
            monster.anim_delay++;
            if (monster.anim_delay >= ANIM_DELAY) {
                monster.anim_delay = 0;
                monster.anim_frame++;
                if (monster.anim_frame >= FRAMES_PER_ANIMATION)
                    monster.anim_frame = 0;
            }
        }

        /* Calculate tile based on state and animation frame
         * Sprite sheet layout (16x16 sprites, tile numbers):
         *   DOWN:  0, 2, 4
         *   UP:    6, 8, 10
         *   RIGHT: 12, 14, 32
         *   LEFT:  same as RIGHT with H-flip
         */
        if (monster.state == W_DOWN) {
            monster.gfx_frame = monster.anim_frame * 2;        /* 0, 2, 4 */
        } else if (monster.state == W_UP) {
            monster.gfx_frame = 6 + monster.anim_frame * 2;    /* 6, 8, 10 */
        } else {
            /* W_RIGHT / W_LEFT */
            if (monster.anim_frame < 2) {
                monster.gfx_frame = 12 + monster.anim_frame * 2; /* 12, 14 */
            } else {
                monster.gfx_frame = 32;                         /* 32 */
            }
        }
        u16 flags = monster.flipx ? OBJ_FLIPX : 0;
        oamSet(0, monster.x, monster.y, monster.gfx_frame, 0, 3, flags);
    }

    return 0;
}
