/**
 * @file main.c
 * @brief Collision Detection Demo
 *
 * Demonstrates the collision detection library:
 * - Rectangle vs rectangle (AABB) collision
 * - Point vs rectangle collision
 * - Tile-based collision
 * - Visual feedback on collision
 *
 * Controls:
 * - D-pad: Move player sprite
 * - A button: Toggle tile collision display
 */

#include <snes.h>
#include <snes/collision.h>

/*============================================================================
 * Game Objects
 *============================================================================*/

#define PLAYER_SIZE 8
#define ENEMY_SIZE  8
#define NUM_ENEMIES 4

/* Player position */
static s16 player_x, player_y;
static Rect player_box;

/* Enemy positions */
static s16 enemy_x[NUM_ENEMIES];
static s16 enemy_y[NUM_ENEMIES];
static Rect enemy_box[NUM_ENEMIES];

/* Collision state */
static u8 collision_flags;

/*============================================================================
 * Tile Collision Map (16x14 tiles, 8x8 pixels each = 128x112 area)
 *============================================================================*/

#define MAP_WIDTH  16
#define MAP_HEIGHT 14

/* Simple collision map: 1 = solid wall, 0 = empty */
static u8 collision_map[MAP_WIDTH * MAP_HEIGHT] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  /* Top wall */
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,  /* Platforms */
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,1,1,1,1,0,0,0,0,0,1,  /* Center platform */
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,  /* Platforms */
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  /* Bottom wall */
};

/* Map offset on screen */
#define MAP_OFFSET_X 64
#define MAP_OFFSET_Y 56

/*============================================================================
 * Sprite Graphics
 *============================================================================*/

/* Player sprite (8x8, 4bpp) - blue square */
static const u8 player_tile[] = {
    0xFF,0xFF, 0x81,0x81, 0x81,0x81, 0x81,0x81,
    0x81,0x81, 0x81,0x81, 0x81,0x81, 0xFF,0xFF,
    0x00,0x00, 0x7E,0x00, 0x7E,0x00, 0x7E,0x00,
    0x7E,0x00, 0x7E,0x00, 0x7E,0x00, 0x00,0x00,
};

/* Enemy sprite (8x8, 4bpp) - red square */
static const u8 enemy_tile[] = {
    0xFF,0x00, 0x81,0x00, 0x81,0x00, 0x81,0x00,
    0x81,0x00, 0x81,0x00, 0x81,0x00, 0xFF,0x00,
    0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
    0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
};

/* Wall tile for BG (8x8, 2bpp) */
static const u8 wall_tile[] = {
    0xFF,0xFF, 0x81,0x81, 0x81,0x81, 0x81,0x81,
    0x81,0x81, 0x81,0x81, 0x81,0x81, 0xFF,0xFF,
};

/* Empty tile (8x8, 2bpp) */
static const u8 empty_tile[] = {
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

/*============================================================================
 * Sprite Palette
 *============================================================================*/

static const u8 sprite_palette[] = {
    0x00, 0x00,  /* Transparent */
    0xE0, 0x03,  /* Blue */
    0x00, 0x7C,  /* Red */
    0x1F, 0x00,  /* Green (collision indicator) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void load_graphics(void) {
    u16 i;

    REG_INIDISP = 0x80;  /* Force blank */

    /* Load BG tiles (tile 0 = empty, tile 1 = wall) */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    /* Tile 0: empty */
    for (i = 0; i < 16; i += 2) {
        REG_VMDATAL = empty_tile[i];
        REG_VMDATAH = empty_tile[i + 1];
    }
    /* Tile 1: wall */
    for (i = 0; i < 16; i += 2) {
        REG_VMDATAL = wall_tile[i];
        REG_VMDATAH = wall_tile[i + 1];
    }

    /* Load sprite tiles at $4000 */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x40;

    /* Tile 0: player (blue) */
    for (i = 0; i < 32; i += 2) {
        REG_VMDATAL = player_tile[i];
        REG_VMDATAH = player_tile[i + 1];
    }
    /* Tile 1: enemy (red) */
    for (i = 0; i < 32; i += 2) {
        REG_VMDATAL = enemy_tile[i];
        REG_VMDATAH = enemy_tile[i + 1];
    }

    /* Load sprite palette */
    REG_CGADD = 128;
    for (i = 0; i < 32; i++) {
        REG_CGDATA = sprite_palette[i];
    }

    /* Load BG palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Black BG */
    REG_CGDATA = 0x94; REG_CGDATA = 0x52;  /* Gray walls */
}

static void draw_tilemap(void) {
    u8 tx, ty;
    u16 addr;
    u8 tile;

    /* Set tilemap at $0400 */
    REG_VMAIN = 0x80;

    for (ty = 0; ty < MAP_HEIGHT; ty++) {
        for (tx = 0; tx < MAP_WIDTH; tx++) {
            /* Calculate screen position */
            addr = 0x0400 + ((ty + 7) * 32) + (tx + 8);

            REG_VMADDL = addr & 0xFF;
            REG_VMADDH = addr >> 8;

            /* Get collision tile value */
            tile = collision_map[ty * MAP_WIDTH + tx];

            REG_VMDATAL = tile;  /* Tile 0 = empty, 1 = wall */
            REG_VMDATAH = 0;
        }
    }
}

static void update_sprites(void) {
    u8 i;
    u8 palette;

    /* Update player sprite - green if colliding, blue if not */
    palette = (collision_flags != 0) ? 1 : 0;  /* Palette 1 = red/green zone */
    oamSet(0, player_x, player_y, 0, palette, 3, 0);

    /* Update enemy sprites */
    for (i = 0; i < NUM_ENEMIES; i++) {
        palette = (collision_flags & (1 << i)) ? 1 : 0;
        oamSet(i + 1, enemy_x[i], enemy_y[i], 1, palette, 2, 0);
    }

    /* Hide remaining sprites */
    oamHide(5);
}

static void check_collisions(void) {
    u8 i;

    collision_flags = 0;

    /* Update player bounding box */
    player_box.x = player_x;
    player_box.y = player_y;
    player_box.width = PLAYER_SIZE;
    player_box.height = PLAYER_SIZE;

    /* Check player vs each enemy */
    for (i = 0; i < NUM_ENEMIES; i++) {
        enemy_box[i].x = enemy_x[i];
        enemy_box[i].y = enemy_y[i];
        enemy_box[i].width = ENEMY_SIZE;
        enemy_box[i].height = ENEMY_SIZE;

        if (collideRect(&player_box, &enemy_box[i])) {
            collision_flags = collision_flags | (1 << i);
        }
    }
}

static u8 check_wall_collision(s16 new_x, s16 new_y) {
    s16 map_x, map_y;

    /* Convert screen coords to map coords */
    map_x = new_x - MAP_OFFSET_X;
    map_y = new_y - MAP_OFFSET_Y;

    /* Check if new position would hit a wall */
    /* Check all four corners of player box */
    if (collideTile(map_x, map_y, collision_map, MAP_WIDTH)) return 1;
    if (collideTile(map_x + PLAYER_SIZE - 1, map_y, collision_map, MAP_WIDTH)) return 1;
    if (collideTile(map_x, map_y + PLAYER_SIZE - 1, collision_map, MAP_WIDTH)) return 1;
    if (collideTile(map_x + PLAYER_SIZE - 1, map_y + PLAYER_SIZE - 1, collision_map, MAP_WIDTH)) return 1;

    return 0;
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 pad, pad_prev;
    s16 new_x, new_y;

    /* Initialize */
    consoleInit();
    setMode(BG_MODE0, 0);
    oamInit();

    /* Load graphics */
    load_graphics();

    /* Draw tilemap */
    draw_tilemap();

    /* Configure BG1 */
    REG_BG1SC = 0x04;  /* Tilemap at $0800 */
    REG_BG12NBA = 0x00;
    REG_TM = TM_BG1 | TM_OBJ;

    /* Object settings */
    REG_OBJSEL = 0x02;  /* 8x8/16x16, tiles at $4000 */

    /* Initialize player position (center of map) */
    player_x = MAP_OFFSET_X + 56;
    player_y = MAP_OFFSET_Y + 48;

    /* Initialize enemy positions */
    enemy_x[0] = MAP_OFFSET_X + 24;
    enemy_y[0] = MAP_OFFSET_Y + 24;

    enemy_x[1] = MAP_OFFSET_X + 88;
    enemy_y[1] = MAP_OFFSET_Y + 24;

    enemy_x[2] = MAP_OFFSET_X + 24;
    enemy_y[2] = MAP_OFFSET_Y + 80;

    enemy_x[3] = MAP_OFFSET_X + 88;
    enemy_y[3] = MAP_OFFSET_Y + 80;

    /* Initial update */
    check_collisions();
    update_sprites();
    oamUpdate();

    /* Enable screen */
    setScreenOn();

    /* Input state */
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}
    pad_prev = REG_JOY1L | (REG_JOY1H << 8);

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Read input */
        while (REG_HVBJOY & 0x01) {}
        pad = REG_JOY1L | (REG_JOY1H << 8);
        pad_prev = pad;

        if (pad == 0xFFFF) continue;

        /* Calculate new position */
        new_x = player_x;
        new_y = player_y;

        if (pad & KEY_LEFT) {
            new_x = player_x - 2;
        }
        if (pad & KEY_RIGHT) {
            new_x = player_x + 2;
        }
        if (pad & KEY_UP) {
            new_y = player_y - 2;
        }
        if (pad & KEY_DOWN) {
            new_y = player_y + 2;
        }

        /* Check wall collision before moving */
        if (!check_wall_collision(new_x, new_y)) {
            player_x = new_x;
            player_y = new_y;
        } else {
            /* Try moving just X */
            if (!check_wall_collision(new_x, player_y)) {
                player_x = new_x;
            }
            /* Try moving just Y */
            if (!check_wall_collision(player_x, new_y)) {
                player_y = new_y;
            }
        }

        /* Check sprite collisions */
        check_collisions();

        /* Update display */
        update_sprites();
        oamUpdate();
    }

    return 0;
}
