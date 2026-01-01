/**
 * @file main.c
 * @brief Platformer Template - Main Entry Point
 *
 * A simple platformer demonstrating OpenSNES capabilities.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

/*============================================================================
 * Constants
 *============================================================================*/

#define SCREEN_WIDTH  256
#define SCREEN_HEIGHT 224

/*============================================================================
 * Game State
 *============================================================================*/

/** Player position */
u16 player_x;
u8  player_y;

/** Player velocity */
s16 player_vx;
s16 player_vy;

/** Player flags */
u8 player_on_ground;
u8 player_facing_right;

/** Gravity constant */
#define GRAVITY 1

/** Jump velocity */
#define JUMP_VELOCITY -8

/** Max fall speed */
#define MAX_FALL_SPEED 8

/*============================================================================
 * Initialization
 *============================================================================*/

/**
 * @brief Initialize game
 */
void gameInit(void) {
    /* Initialize player */
    player_x = 100;
    player_y = 160;
    player_vx = 0;
    player_vy = 0;
    player_on_ground = TRUE;
    player_facing_right = TRUE;

    /* Set up graphics mode */
    setMode(BGMODE_MODE1);

    /* TODO: Load tiles and palettes */
    /* dmaCopyToVRAM(tiles, 0x0000, tiles_size); */
    /* dmaCopyToCGRAM(palette, 0, 16); */

    /* Enable BG1 and sprites */
    REG_TM = TM_BG1 | TM_OBJ;

    /* Initialize sprites */
    oamInit();
}

/*============================================================================
 * Input Handling
 *============================================================================*/

/**
 * @brief Process player input
 */
void handleInput(void) {
    u16 keys = padHeld(0);
    u16 pressed = padPressed(0);

    /* Horizontal movement */
    if (keys & KEY_LEFT) {
        player_vx = -2;
        player_facing_right = FALSE;
    } else if (keys & KEY_RIGHT) {
        player_vx = 2;
        player_facing_right = TRUE;
    } else {
        player_vx = 0;
    }

    /* Jumping */
    if ((pressed & KEY_B) && player_on_ground) {
        player_vy = JUMP_VELOCITY;
        player_on_ground = FALSE;
    }
}

/*============================================================================
 * Physics
 *============================================================================*/

/**
 * @brief Update player physics
 */
void updatePhysics(void) {
    /* Apply gravity */
    if (!player_on_ground) {
        player_vy += GRAVITY;
        if (player_vy > MAX_FALL_SPEED) {
            player_vy = MAX_FALL_SPEED;
        }
    }

    /* Update position */
    player_x += player_vx;
    player_y += player_vy;

    /* Simple ground collision (floor at y=192) */
    if (player_y >= 192) {
        player_y = 192;
        player_vy = 0;
        player_on_ground = TRUE;
    }

    /* Screen boundaries */
    if (player_x < 8) player_x = 8;
    if (player_x > SCREEN_WIDTH - 24) player_x = SCREEN_WIDTH - 24;
}

/*============================================================================
 * Rendering
 *============================================================================*/

/**
 * @brief Update sprite display
 */
void updateSprites(void) {
    u8 flip = player_facing_right ? 0 : 0x40;

    /* Set player sprite */
    oamSet(0, player_x, player_y, 0, 0, 3, flip);
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    /* Initialize SNES hardware */
    consoleInit();

    /* Initialize game */
    gameInit();

    /* Turn on screen */
    setScreenOn();

    /* Main game loop */
    while (1) {
        /* Wait for VBlank */
        WaitForVBlank();

        /* Update OAM */
        oamUpdate();

        /* Handle input */
        handleInput();

        /* Update physics */
        updatePhysics();

        /* Update sprites */
        updateSprites();
    }

    return 0;
}
