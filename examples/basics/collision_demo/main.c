/**
 * @file main.c
 * @brief AABB and tile-based collision detection with visual feedback
 * @ingroup examples
 *
 * Demonstrates the OpenSNES collision module with two complementary
 * techniques: axis-aligned bounding box (AABB) tests between sprites, and
 * tile-based collision against a map. Kit walks through a warehouse of the
 * Rue basse, between wall plates and stacks of crates, while four security
 * drones hover at fixed places. Collisions are detected every frame: a
 * drone Kit touches turns its lights green. Wall collisions use corner-point
 * checks against a 16x14 collision map of 16x16 cells, with per-axis
 * rejection so that Kit slides along walls.
 *
 * The collision map is also the picture: the example builds BG1's tilemap
 * from it at boot (a wall plate, a crate or the floor for each cell), so
 * what Kit bumps into is exactly what is drawn.
 *
 * A hitbox is not a sprite. Kit is a 32x32 sprite; what collides is a
 * 16x16 box at his feet, and the sprite is drawn around it. The drones'
 * boxes are their bodies, smaller than their frames too.
 *
 * @par SNES Concepts
 * - AABB collision via collideRect() for sprite-vs-sprite overlap testing
 * - Tile collision via collideTileEx() (16x16 cells) for sprite-vs-map
 *   wall rejection
 * - A tilemap built from game data in force blank: one DMA a row
 * - A hitbox smaller than its sprite, and the sprite drawn from the box
 * - A palette per state: the same tiles, OBJ palette 1 or 2
 *
 * @par What to Observe
 * - Walk Kit with the D-pad: he faces the way he walks (left is his right
 *   frames mirrored) and takes steps while he moves
 * - He cannot pass through the crates or the walls; he slides along them
 * - When his box overlaps a drone's, the drone's lights turn green
 * - Two drones can be touched at once; each is tracked on its own
 *
 * @par Modules Used
 * console, input, sprite, dma, collision, background
 *
 * @see collision.h, sprite.h, dma.h, input.h
 */

#include <snes.h>
#include <snes/collision.h>
#include "vram_map.h"  /* generated VRAM bases (devtools/vram_layout) */

#include "res/cells.inc"
#include "res/cast.inc"
/* generated: cells_tiles/_pal, cast_tiles/_pal (each with _end) */

/*============================================================================
 * Game Objects
 *============================================================================*/

#define BOX_SIZE     16  /**< Kit's and the drones' hitboxes, 16x16 pixels */
#define NUM_ENEMIES  4   /**< Number of drones in the warehouse */
#define PLAYER_SPEED 2   /**< Kit's speed in pixels per frame */

/** @name Sprite frames in res/cast.png (32x32, four a row)
 *  @{ */
#define FRAME_DOWN   0   /**< Kit walking down: frames 0-4 */
#define FRAME_UP     5   /**< walking up: 5-9 */
#define FRAME_RIGHT  10  /**< walking right: 10-14; left is the same, mirrored */
#define FRAME_DRONE  15  /**< the drone's hover: 15-18 */
/** @} */

/** @name OBJ palettes
 *  @{ */
#define PAL_KIT        0
#define PAL_DRONE      1  /**< pink lights */
#define PAL_DRONE_SEEN 2  /**< the same drone, its lights green: Kit is touching it */
/** @} */

/** @brief Kit's hitbox, top-left corner, in screen coordinates */
static s16 player_x;
/** @brief Kit's hitbox, top-left corner, Y */
static s16 player_y;
/** @brief Kit's bounding box for AABB collision tests via collideRect() */
static Rect player_box;

/** @brief Drones' hitboxes, top-left corner X */
static s16 enemy_x[NUM_ENEMIES];
/** @brief Drones' hitboxes, top-left corner Y */
static s16 enemy_y[NUM_ENEMIES];
/** @brief Bounding boxes for each drone, rebuilt every frame for collideRect() */
static Rect enemy_box[NUM_ENEMIES];

/**
 * @brief Bitmask tracking which drones Kit is currently overlapping
 *
 * Bit N is set when Kit's box overlaps drone N's. It picks each drone's
 * palette (PAL_DRONE or PAL_DRONE_SEEN).
 */
static u8 collision_flags;

/** @brief The way Kit faces: FRAME_DOWN, FRAME_UP or FRAME_RIGHT (with facing_left) */
static u8 facing;
/** @brief Kit faces left: his right frames, mirrored */
static u8 facing_left;
/** @brief Frames since boot, for the walk and the hover */
static u16 tick;

/*============================================================================
 * Tile Collision Map (16x14 cells, 16x16 pixels each = the whole screen)
 *============================================================================*/

#define MAP_WIDTH  16  /**< Collision map width in 16x16 cells */
#define MAP_HEIGHT 14  /**< Collision map height in 16x16 cells */
#define CELL       16  /**< A cell's size in pixels, as collideTileEx() is told */

/** @name Cell values: 0 passable, anything else solid
 *  @{ */
#define FLOOR 0
#define CRATE 1
#define WALL  2
/** @} */

/**
 * @brief Tile-based collision map (16x14 = 224 bytes, 256x224 pixels)
 *
 * Row-major, one byte a cell: FLOOR is passable, CRATE and WALL are solid
 * (collideTileEx() returns the value; the example only asks "non-zero").
 * The two solid kinds differ only in how they are drawn.
 *
 * @note `const`, so it lives in ROM and costs zero WRAM. collideTileEx()
 *       takes a `const u8 *`, and const-qualified loads compile to
 *       bank-honouring far reads — the map may sit in any bank.
 */
static const u8 collision_map[MAP_WIDTH * MAP_HEIGHT] = {
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  /* The north wall */
    2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,
    2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,
    2,0,0,1,1,0,0,0,0,0,0,1,1,0,0,2,  /* Crate stacks */
    2,0,0,1,1,0,0,0,0,0,0,1,1,0,0,2,
    2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,
    2,0,0,0,0,0,1,1,1,1,0,0,0,0,0,2,  /* The middle stack */
    2,0,0,0,0,0,1,1,1,1,0,0,0,0,0,2,
    2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,
    2,0,0,1,1,0,0,0,0,0,0,1,1,0,0,2,  /* Crate stacks */
    2,0,0,1,1,0,0,0,0,0,0,1,1,0,0,2,
    2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,
    2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  /* The south wall */
};

/*============================================================================
 * Helper Functions
 *============================================================================*/

/**
 * @brief The OAM tile of frame @p f of res/cast.png
 *
 * A 32x32 frame is 4x4 tiles and a row of OBJ tiles is 16 wide, so frame f
 * (four a row of the sheet) starts at column (f % 4) * 4 of tile row
 * (f / 4) * 4. Frames 16 and up are past tile 255: the second OBJ name
 * table, which oamSet() reaches through the tile's ninth bit.
 */
static u16 frame_tile(u16 f) {
    return (u16)(((f & 3) << 2) + ((f >> 2) << 6));
}

/**
 * @brief Build BG1's tilemap from the collision map, in force blank
 *
 * Each cell is 2x2 tiles of res/cells.png: cell k of the picture (0 floor,
 * 1 crate, 2 wall, 3 floor with a drain) is tiles 2k, 2k + 1 above
 * 8 + 2k, 9 + 2k. A floor cell gets the drain now and then, chosen from its
 * position so the floor is not one repeated slab. One row of 32 map entries
 * is built in RAM and sent with one DMA, 28 times.
 */
static void build_tilemap(void) {
    static u16 row[32];
    u8 tx, ty, half;
    u8 cell;

    for (ty = 0; ty < MAP_HEIGHT; ty++) {
        for (half = 0; half < 2; half++) {
            for (tx = 0; tx < MAP_WIDTH; tx++) {
                cell = collision_map[ty * MAP_WIDTH + tx];
                if (cell == FLOOR && (u8)(tx * 7 + ty * 3) % 11 == 0)
                    cell = 3;
                row[tx * 2]     = (u16)(cell * 2 + half * 8);
                row[tx * 2 + 1] = (u16)(cell * 2 + half * 8 + 1);
            }
            dmaCopyVram((u8 *)row, (u16)(VRAM_BG_TILEMAP + (ty * 2 + half) * 32), 64);
        }
    }
}

/**
 * @brief Place the five sprites from their hitboxes
 *
 * The drones take OAM slots 0-3 and Kit slot 4: the lower slot is drawn in
 * front, and the drones fly, so a drone Kit walks under stays visible —
 * its green lights are the feedback.
 * Kit's 32x32 frame is drawn so that his 16x16 box sits at its bottom
 * centre (8 pixels in, 16 down): his feet. The drones' boxes sit 8 pixels
 * into their frames each way, round their bodies. Kit walks while he
 * moves (five frames, six vblanks each) and stands on the first frame of
 * his direction otherwise; the drones hover through four frames together.
 */
static void update_sprites(u8 moving) {
    u8 i;
    u16 f;
    u16 hover;

    f = facing;
    if (moving)
        f += (tick / 6) % 5;
    oamSet(NUM_ENEMIES, player_x - 8, player_y - 16, frame_tile(f), PAL_KIT, 3,
           facing_left ? OBJ_FLIPX : 0);

    hover = frame_tile(FRAME_DRONE + (tick / 8) % 4);
    for (i = 0; i < NUM_ENEMIES; i++) {
        oamSet(i, enemy_x[i] - 8, enemy_y[i] - 8, hover,
               (collision_flags & (1 << i)) ? PAL_DRONE_SEEN : PAL_DRONE, 3, 0);
    }
}

/**
 * @brief Test Kit-vs-drone AABB overlap and update collision_flags
 *
 * Rebuilds Kit's and every drone's box from their current positions, then
 * calls collideRect() for each Kit-drone pair. The result is stored in
 * collision_flags as a bitmask (bit N = drone N is overlapped).
 */
static void check_collisions(void) {
    u8 i;

    collision_flags = 0;

    player_box.x = player_x;
    player_box.y = player_y;
    player_box.width = BOX_SIZE;
    player_box.height = BOX_SIZE;

    for (i = 0; i < NUM_ENEMIES; i++) {
        enemy_box[i].x = enemy_x[i];
        enemy_box[i].y = enemy_y[i];
        enemy_box[i].width = BOX_SIZE;
        enemy_box[i].height = BOX_SIZE;

        if (collideRect(&player_box, &enemy_box[i])) {
            collision_flags |= (1 << i);
        }
    }
}

/**
 * @brief Test whether Kit's box at a position would overlap a solid cell
 *
 * Probes the four corners of the 16x16 box against collision_map[] via
 * collideTileEx() with 16-pixel cells. If any corner lands on a solid cell,
 * the function returns 1 to reject the movement.
 *
 * The caller uses per-axis rejection: if the combined (new_x, new_y)
 * collides, it tries each axis independently so that Kit slides along
 * walls instead of stopping dead.
 *
 * @param new_x Proposed box X in screen coordinates (the map covers the screen)
 * @param new_y Proposed box Y
 * @return 1 if the position overlaps a solid cell, 0 if clear
 */
static u8 check_wall_collision(s16 new_x, s16 new_y) {
    if (collideTileEx(new_x, new_y, collision_map, MAP_WIDTH, CELL)) return 1;
    if (collideTileEx(new_x + BOX_SIZE - 1, new_y, collision_map, MAP_WIDTH, CELL)) return 1;
    if (collideTileEx(new_x, new_y + BOX_SIZE - 1, collision_map, MAP_WIDTH, CELL)) return 1;
    if (collideTileEx(new_x + BOX_SIZE - 1, new_y + BOX_SIZE - 1, collision_map, MAP_WIDTH, CELL)) return 1;

    return 0;
}

/*============================================================================
 * Main
 *============================================================================*/

/**
 * @brief Entry point -- build the warehouse, run the loop with collision tests
 *
 * Sets up Mode 1 with BG1 for the warehouse and sprites for Kit and the
 * drones. The main loop reads the D-pad every frame, proposes a new
 * position, validates it against the wall collision map (with per-axis
 * sliding), checks AABB overlaps with the drones, and places the sprites.
 * The NMI handler sends the OAM buffer during VBlank.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    u16 pad;            /**< Current joypad button state (bitmask) */
    s16 new_x, new_y;   /**< Proposed box position before the wall check */
    u8 moving;

    consoleInit();
    setMode(BG_MODE1, 0);

    /* BG1: the four cells' tiles and palette, then the map built from the
     * collision array (everything here runs in force blank) */
    bgInitTileSet(0, cells_tiles, cells_pal, 0,
                  cells_tiles_end - cells_tiles,
                  cells_pal_end - cells_pal,
                  BG_16COLORS, VRAM_BG_TILES);
    bgSetMapPtr(0, VRAM_BG_TILEMAP, BG_MAP_32x32);
    build_tilemap();

    /* The sprites: Kit's walk and the drone's hover, 320 tiles, 32x32;
     * OBJ palettes 0-2 (Kit, the drone, the drone seen) */
    oamInitGfxSet(cast_tiles, cast_tiles_end - cast_tiles,
                  cast_pal, cast_pal_end - cast_pal,
                  0, VRAM_OBJ_TILES, OBJ_SIZE16_L32);

    setMainScreen(TM_BG1 | TM_OBJ);

    /* Kit's box: cell (7, 8), the open row below the middle stack */
    player_x = 7 * CELL;
    player_y = 8 * CELL;
    facing = FRAME_DOWN;

    /* The drones: the four corners of the floor */
    enemy_x[0] = 2 * CELL;   enemy_y[0] = 1 * CELL;
    enemy_x[1] = 13 * CELL;  enemy_y[1] = 1 * CELL;
    enemy_x[2] = 2 * CELL;   enemy_y[2] = 12 * CELL;
    enemy_x[3] = 13 * CELL;  enemy_y[3] = 12 * CELL;

    /* All five are 32x32: the LARGE size, set once */
    {
        u8 i;
        for (i = 0; i < 1 + NUM_ENEMIES; i++)
            oamSetSize(i, OBJ_LARGE);
    }

    check_collisions();
    update_sprites(0);

    setScreenOn();

    while (1) {
        WaitForVBlank();
        tick++;

        pad = padHeld(0);
        new_x = player_x;
        new_y = player_y;
        moving = 0;

        if (pad & KEY_LEFT)  { new_x = player_x - PLAYER_SPEED; facing = FRAME_RIGHT; facing_left = 1; moving = 1; }
        if (pad & KEY_RIGHT) { new_x = player_x + PLAYER_SPEED; facing = FRAME_RIGHT; facing_left = 0; moving = 1; }
        if (pad & KEY_UP)    { new_y = player_y - PLAYER_SPEED; facing = FRAME_UP;    facing_left = 0; moving = 1; }
        if (pad & KEY_DOWN)  { new_y = player_y + PLAYER_SPEED; facing = FRAME_DOWN;  facing_left = 0; moving = 1; }

        /* Wall collision */
        if (!check_wall_collision(new_x, new_y)) {
            player_x = new_x;
            player_y = new_y;
        } else {
            if (!check_wall_collision(new_x, player_y)) {
                player_x = new_x;
            }
            if (!check_wall_collision(player_x, new_y)) {
                player_y = new_y;
            }
        }

        /* Sprite collisions */
        check_collisions();

        update_sprites(moving);
    }

    return 0;
}
