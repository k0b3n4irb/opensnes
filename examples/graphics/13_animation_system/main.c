/**
 * @file main.c
 * @brief Animation System Example
 *
 * Demonstrates the sprite animation framework:
 * - Creating animation definitions
 * - Playing and controlling animations
 * - Integrating with OAM sprites
 * - Multiple animation states
 *
 * Controls:
 * - D-pad: Move sprite
 * - A button: Play jump animation
 * - B button: Toggle walk/idle
 * - Start: Pause/resume animation
 */

#include <snes.h>
#include <snes/animation.h>

/*============================================================================
 * Sprite Graphics (8x8, 4bpp = 32 bytes per tile)
 * Simple bouncing ball animation (4 frames)
 *============================================================================*/

static const u8 sprite_tiles[] = {
    /* Frame 0: Ball - small */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x3C,0x3C, 0x7E,0x42, 0x7E,0x42,
    0x7E,0x42, 0x3C,0x3C, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,

    /* Frame 1: Ball - medium */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x18,0x18,
    0x3C,0x24, 0x7E,0x42, 0x7E,0x42, 0x7E,0x42,
    0x7E,0x42, 0x3C,0x24, 0x18,0x18, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,

    /* Frame 2: Ball - large */
    0x00,0x00, 0x18,0x18, 0x3C,0x24, 0x7E,0x42,
    0xFF,0x81, 0xFF,0x81, 0xFF,0x81, 0xFF,0x81,
    0xFF,0x81, 0xFF,0x81, 0x7E,0x42, 0x3C,0x24,
    0x18,0x18, 0x00,0x00, 0x00,0x00, 0x00,0x00,

    /* Frame 3: Ball - squashed (hitting ground) */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x18,0x18, 0x7E,0x42,
    0xFF,0x81, 0xFF,0x81, 0xFF,0x81, 0x7E,0x42,
    0x3C,0x24, 0x00,0x00, 0x00,0x00, 0x00,0x00,

    /* Frame 4: Idle pose (simple standing) */
    0x3C,0x3C, 0x7E,0x42, 0x7E,0x5A, 0x7E,0x42,
    0x3C,0x3C, 0x18,0x18, 0x3C,0x24, 0x3C,0x24,
    0x3C,0x24, 0x18,0x18, 0x24,0x24, 0x24,0x24,
    0x66,0x66, 0x00,0x00, 0x00,0x00, 0x00,0x00,

    /* Frame 5: Walk frame 1 */
    0x3C,0x3C, 0x7E,0x42, 0x7E,0x5A, 0x7E,0x42,
    0x3C,0x3C, 0x18,0x18, 0x3C,0x24, 0x3C,0x24,
    0x1C,0x1C, 0x38,0x20, 0x20,0x20, 0x70,0x70,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,

    /* Frame 6: Walk frame 2 */
    0x3C,0x3C, 0x7E,0x42, 0x7E,0x5A, 0x7E,0x42,
    0x3C,0x3C, 0x18,0x18, 0x3C,0x24, 0x3C,0x24,
    0x38,0x38, 0x1C,0x04, 0x04,0x04, 0x0E,0x0E,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,

    /* Frame 7: Jump frame */
    0x3C,0x3C, 0x7E,0x42, 0x7E,0x5A, 0x7E,0x42,
    0x3C,0x3C, 0x5A,0x5A, 0x5A,0x42, 0x18,0x18,
    0x18,0x18, 0x3C,0x24, 0x66,0x66, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

#define SPRITE_TILE_SIZE (8 * 32)

/*============================================================================
 * Animation Definitions
 *============================================================================*/

/* Frame indices for animations */
static u8 bounce_frames[] = { 0, 1, 2, 1, 0, 3 };
static u8 idle_frames[]   = { 4 };
static u8 walk_frames[]   = { 5, 4, 6, 4 };
static u8 jump_frames[]   = { 7, 7, 7, 4 };

/* Animation definitions */
static Animation anim_bounce = {
    bounce_frames,
    6,      /* frameCount */
    8,      /* frameDelay (8 VBlanks = ~7.5 FPS) */
    1       /* loop */
};

static Animation anim_idle = {
    idle_frames,
    1,
    60,     /* slow - just repeat same frame */
    1
};

static Animation anim_walk = {
    walk_frames,
    4,
    6,      /* 10 FPS */
    1
};

static Animation anim_jump = {
    jump_frames,
    4,
    8,
    0       /* one-shot */
};

/*============================================================================
 * Game State
 *============================================================================*/

#define ANIM_SLOT_BALL   0
#define ANIM_SLOT_PLAYER 1

static s16 ball_x, ball_y;
static s16 player_x, player_y;
static u8 player_walking;

/*============================================================================
 * Sprite Palette
 *============================================================================*/

static const u8 sprite_palette[] = {
    0x00, 0x00,  /* Color 0: Transparent */
    0xFF, 0x7F,  /* Color 1: White */
    0x00, 0x7C,  /* Color 2: Red */
    0xE0, 0x03,  /* Color 3: Blue */
    0x1F, 0x00,  /* Color 4: Green (unused) */
    0x00, 0x00,  /* Colors 5-15: unused */
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
};

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void load_sprite_graphics(void) {
    u16 i;

    /* Set force blank for VRAM access */
    REG_INIDISP = 0x80;

    /* Load sprite tiles to VRAM at $4000 (word address $2000) */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x40;

    for (i = 0; i < SPRITE_TILE_SIZE; i += 2) {
        REG_VMDATAL = sprite_tiles[i];
        REG_VMDATAH = sprite_tiles[i + 1];
    }

    /* Load sprite palette to CGRAM (palette 8 = sprites start at color 128) */
    REG_CGADD = 128;
    for (i = 0; i < 32; i++) {
        REG_CGDATA = sprite_palette[i];
    }
}

static void update_sprites(void) {
    u8 ball_tile, player_tile;

    /* Get current animation frames */
    ball_tile = animGetFrame(ANIM_SLOT_BALL);
    player_tile = animGetFrame(ANIM_SLOT_PLAYER);

    /* Set ball sprite (id, x, y, tile, palette, priority, flags) */
    oamSet(0, ball_x, ball_y, ball_tile, 0, 3, 0);

    /* Set player sprite */
    oamSet(1, player_x, player_y, player_tile, 0, 2, 0);

    /* Hide unused sprites */
    oamHide(2);
}

/*============================================================================
 * Text Display (for status)
 *============================================================================*/

/* Simple embedded font for status text */
static const u8 font_tiles[] = {
    /* Space (tile 0) */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    /* A (tile 1) */
    0x18,0x00, 0x3C,0x00, 0x66,0x00, 0x7E,0x00,
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* B (tile 2) */
    0x7C,0x00, 0x66,0x00, 0x66,0x00, 0x7C,0x00,
    0x66,0x00, 0x66,0x00, 0x7C,0x00, 0x00,0x00,
    /* D (tile 3) */
    0x78,0x00, 0x6C,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x6C,0x00, 0x78,0x00, 0x00,0x00,
    /* E (tile 4) */
    0x7E,0x00, 0x60,0x00, 0x60,0x00, 0x78,0x00,
    0x60,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
    /* I (tile 5) */
    0x3C,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x3C,0x00, 0x00,0x00,
    /* J (tile 6) */
    0x1E,0x00, 0x0C,0x00, 0x0C,0x00, 0x0C,0x00,
    0x0C,0x00, 0x6C,0x00, 0x38,0x00, 0x00,0x00,
    /* K (tile 7) */
    0x66,0x00, 0x6C,0x00, 0x78,0x00, 0x70,0x00,
    0x78,0x00, 0x6C,0x00, 0x66,0x00, 0x00,0x00,
    /* L (tile 8) */
    0x60,0x00, 0x60,0x00, 0x60,0x00, 0x60,0x00,
    0x60,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
    /* M (tile 9) */
    0x63,0x00, 0x77,0x00, 0x7F,0x00, 0x6B,0x00,
    0x63,0x00, 0x63,0x00, 0x63,0x00, 0x00,0x00,
    /* N (tile 10) */
    0x66,0x00, 0x76,0x00, 0x7E,0x00, 0x7E,0x00,
    0x6E,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* O (tile 11) */
    0x3C,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* P (tile 12) */
    0x7C,0x00, 0x66,0x00, 0x66,0x00, 0x7C,0x00,
    0x60,0x00, 0x60,0x00, 0x60,0x00, 0x00,0x00,
    /* S (tile 13) */
    0x3E,0x00, 0x60,0x00, 0x60,0x00, 0x3C,0x00,
    0x06,0x00, 0x06,0x00, 0x7C,0x00, 0x00,0x00,
    /* T (tile 14) */
    0x7E,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x18,0x00, 0x00,0x00,
    /* U (tile 15) */
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* W (tile 16) */
    0x63,0x00, 0x63,0x00, 0x63,0x00, 0x6B,0x00,
    0x7F,0x00, 0x77,0x00, 0x63,0x00, 0x00,0x00,
    /* : (tile 17) */
    0x00,0x00, 0x18,0x00, 0x18,0x00, 0x00,0x00,
    0x18,0x00, 0x18,0x00, 0x00,0x00, 0x00,0x00,
};

#define FONT_SIZE (18 * 16)

#define T_SPACE 0
#define T_A 1
#define T_B 2
#define T_D 3
#define T_E 4
#define T_I 5
#define T_J 6
#define T_K 7
#define T_L 8
#define T_M 9
#define T_N 10
#define T_O 11
#define T_P 12
#define T_S 13
#define T_T 14
#define T_U 15
#define T_W 16
#define T_COLON 17

#define TILEMAP_ADDR 0x0400

static void write_tile(u8 x, u8 y, u8 tile) {
    u16 addr = TILEMAP_ADDR + y * 32 + x;
    REG_VMAIN = 0x80;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;
    REG_VMDATAL = tile;
    REG_VMDATAH = 0;
}

static void draw_text(u8 x, u8 y, const u8 *tiles, u8 len) {
    u8 i;
    for (i = 0; i < len; i++) {
        write_tile(x + i, y, tiles[i]);
    }
}

static void load_font(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;
    for (i = 0; i < FONT_SIZE; i += 2) {
        REG_VMDATAL = font_tiles[i];
        REG_VMDATAH = font_tiles[i + 1];
    }
}

static void clear_tilemap(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = TILEMAP_ADDR & 0xFF;
    REG_VMADDH = TILEMAP_ADDR >> 8;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = T_SPACE;
        REG_VMDATAH = 0;
    }
}

/* Pre-encoded labels */
static const u8 lbl_title[] = { T_A, T_N, T_I, T_M, T_A, T_T, T_I, T_O, T_N, T_SPACE, T_D, T_E, T_M, T_O };
static const u8 lbl_state[] = { T_S, T_T, T_A, T_T, T_E, T_COLON };
static const u8 lbl_idle[] = { T_I, T_D, T_L, T_E };
static const u8 lbl_walk[] = { T_W, T_A, T_L, T_K };
static const u8 lbl_jump[] = { T_J, T_U, T_M, T_P };
static const u8 lbl_pause[] = { T_P, T_A, T_U, T_S, T_E };

static void draw_status(void) {
    u8 state = animGetState(ANIM_SLOT_PLAYER);

    /* Clear status area */
    u8 i;
    for (i = 0; i < 10; i++) {
        write_tile(13 + i, 3, T_SPACE);
    }

    /* Draw current state */
    if (state == ANIM_STATE_PAUSED) {
        draw_text(13, 3, lbl_pause, 5);
    } else if (state == ANIM_STATE_FINISHED) {
        draw_text(13, 3, lbl_idle, 4);
        /* Restart idle after jump finishes */
        animInit(ANIM_SLOT_PLAYER, &anim_idle);
        animPlay(ANIM_SLOT_PLAYER);
        player_walking = 0;
    } else if (player_walking) {
        draw_text(13, 3, lbl_walk, 4);
    } else {
        draw_text(13, 3, lbl_idle, 4);
    }
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 pad, pad_prev, pad_pressed;

    /* Initialize */
    consoleInit();
    setMode(BG_MODE0, 0);
    oamInit();

    /* Load graphics */
    load_font();
    clear_tilemap();
    load_sprite_graphics();

    /* Configure BG1 for text */
    REG_BG1SC = 0x04;
    REG_BG12NBA = 0x00;
    REG_TM = TM_BG1 | TM_OBJ;

    /* Set BG palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x28;  /* Dark blue BG */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* White text */

    /* Object settings: 8x8 sprites, tiles at $4000 */
    REG_OBJSEL = 0x02;  /* 8x8/16x16, name base = $4000 */

    /* Draw title and labels */
    draw_text(9, 1, lbl_title, 14);
    draw_text(6, 3, lbl_state, 6);

    /* Initialize positions */
    ball_x = 50;
    ball_y = 100;
    player_x = 150;
    player_y = 150;
    player_walking = 0;

    /* Initialize animations */
    animInit(ANIM_SLOT_BALL, &anim_bounce);
    animInit(ANIM_SLOT_PLAYER, &anim_idle);

    /* Start animations */
    animPlay(ANIM_SLOT_BALL);
    animPlay(ANIM_SLOT_PLAYER);

    /* Update display */
    update_sprites();
    oamUpdate();
    draw_status();

    /* Enable screen */
    setScreenOn();

    /* Input state */
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}
    pad_prev = REG_JOY1L | (REG_JOY1H << 8);

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Update all animations */
        animUpdate();

        /* Update sprite display */
        update_sprites();
        oamUpdate();

        /* Read input */
        while (REG_HVBJOY & 0x01) {}
        pad = REG_JOY1L | (REG_JOY1H << 8);
        pad_pressed = pad & ~pad_prev;
        pad_prev = pad;

        if (pad == 0xFFFF) continue;

        /* Handle input */

        /* D-pad: Move player */
        if (pad & KEY_LEFT) {
            if (player_x > 0) player_x = player_x - 2;
            if (!player_walking && animGetState(ANIM_SLOT_PLAYER) != ANIM_STATE_PAUSED) {
                animSetAnim(ANIM_SLOT_PLAYER, &anim_walk);
                animPlay(ANIM_SLOT_PLAYER);
                player_walking = 1;
            }
        }
        if (pad & KEY_RIGHT) {
            if (player_x < 240) player_x = player_x + 2;
            if (!player_walking && animGetState(ANIM_SLOT_PLAYER) != ANIM_STATE_PAUSED) {
                animSetAnim(ANIM_SLOT_PLAYER, &anim_walk);
                animPlay(ANIM_SLOT_PLAYER);
                player_walking = 1;
            }
        }
        if (pad & KEY_UP) {
            if (player_y > 0) player_y = player_y - 2;
        }
        if (pad & KEY_DOWN) {
            if (player_y < 216) player_y = player_y + 2;
        }

        /* Stop walking if not moving horizontally */
        if (!(pad & (KEY_LEFT | KEY_RIGHT)) && player_walking) {
            animSetAnim(ANIM_SLOT_PLAYER, &anim_idle);
            animPlay(ANIM_SLOT_PLAYER);
            player_walking = 0;
        }

        /* A button: Jump animation (one-shot) */
        if (pad_pressed & KEY_A) {
            animSetAnim(ANIM_SLOT_PLAYER, &anim_jump);
            animPlay(ANIM_SLOT_PLAYER);
            player_walking = 0;
        }

        /* B button: Toggle walk/idle */
        if (pad_pressed & KEY_B) {
            if (player_walking) {
                animSetAnim(ANIM_SLOT_PLAYER, &anim_idle);
                animPlay(ANIM_SLOT_PLAYER);
                player_walking = 0;
            } else {
                animSetAnim(ANIM_SLOT_PLAYER, &anim_walk);
                animPlay(ANIM_SLOT_PLAYER);
                player_walking = 1;
            }
        }

        /* Start: Pause/resume */
        if (pad_pressed & KEY_START) {
            if (animGetState(ANIM_SLOT_PLAYER) == ANIM_STATE_PAUSED) {
                animResume(ANIM_SLOT_PLAYER);
            } else {
                animPause(ANIM_SLOT_PLAYER);
            }
        }

        /* Update status display */
        draw_status();
    }

    return 0;
}
