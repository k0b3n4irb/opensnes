/**
 * @file main.c
 * @brief AABB and tile-based collision detection with visual feedback
 * @ingroup examples
 *
 * Demonstrates the OpenSNES collision module with two complementary
 * techniques: axis-aligned bounding box (AABB) tests between sprites, and
 * tile-based collision against a static map. A white player sprite moves
 * through a walled arena while four red enemy sprites sit at fixed
 * positions. Collisions are detected every frame and indicated by color
 * changes (white -> green). Wall collisions use corner-point checks against
 * a 16x14 collision map, with per-axis rejection so the player can slide
 * along walls.
 *
 * The example also illustrates the direct oamMemory[] write pattern instead
 * of calling oamSet() per sprite, which avoids the 158-byte stack frame
 * overhead of oamSet() and keeps the main loop fast enough for 5 sprites
 * at 60 fps.
 *
 * @par SNES Concepts
 * - AABB collision via collideRect() for sprite-vs-sprite overlap testing
 * - Tile collision via collideTile() for sprite-vs-map wall rejection
 * - Direct OAM buffer writes (oamMemory[]) with oam_update_flag for NMI DMA
 * - OAM high table management (bytes 512+) for X high bit and sprite size
 * - Separate sprite palette banks for normal and collision-highlight colors
 * - Mode 0 BG with hand-coded wall/empty tiles for the collision arena
 *
 * @par What to Observe
 * - Move the white square with D-pad through the gray-walled arena
 * - The player cannot pass through walls; it slides along them
 * - When the player overlaps a red enemy, both turn green
 * - Multiple simultaneous collisions are tracked independently
 *
 * @par Modules Used
 * console, input, sprite, dma, collision, background
 *
 * @see collision.h, sprite.h, dma.h, input.h
 */

#include <snes.h>
#include <snes/collision.h>

/* oamMemory[] and oam_update_flag declared in <snes/system.h> (via <snes.h>) */

/*============================================================================
 * Game Objects
 *============================================================================*/

#define PLAYER_SIZE  8  /**< Player sprite width/height in pixels (8x8) */
#define ENEMY_SIZE   8  /**< Enemy sprite width/height in pixels (8x8) */
#define NUM_ENEMIES  4  /**< Number of static enemy sprites in the arena */
#define PLAYER_SPEED 2  /**< Player movement speed in pixels per frame */

/** @brief Player X position in screen coordinates */
static s16 player_x;
/** @brief Player Y position in screen coordinates */
static s16 player_y;
/** @brief Player bounding box for AABB collision tests via collideRect() */
static Rect player_box;

/** @brief X positions of the four static enemy sprites */
static s16 enemy_x[NUM_ENEMIES];
/** @brief Y positions of the four static enemy sprites */
static s16 enemy_y[NUM_ENEMIES];
/** @brief Bounding boxes for each enemy, rebuilt every frame for collideRect() */
static Rect enemy_box[NUM_ENEMIES];

/**
 * @brief Bitmask tracking which enemies the player is currently overlapping
 *
 * Bit N is set when the player's AABB overlaps enemy N. Used to switch
 * both the player and the colliding enemy to the green "collision" palette.
 */
static u8 collision_flags;

/*============================================================================
 * Tile Collision Map (16x14 tiles, 8x8 pixels each = 128x112 area)
 *============================================================================*/

#define MAP_WIDTH  16  /**< Collision map width in 8x8 tiles */
#define MAP_HEIGHT 14  /**< Collision map height in 8x8 tiles */

/**
 * @brief Tile-based collision map (16x14 = 224 bytes, 128x112 pixel area)
 *
 * A flat 2D array where 1 = solid wall and 0 = passable. The map is stored
 * in row-major order. collideTile() converts a pixel coordinate to a tile
 * index (px/8) and looks up this array to determine if the tile is solid.
 * The border is all walls with internal platforms at symmetric positions.
 *
 * @note This is mutable (not const) because collideTile() takes a non-const
 *       pointer. It lives in WRAM, not ROM.
 */
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

/** @brief Screen X offset where the collision map starts (centers the 128px map on 256px screen) */
#define MAP_OFFSET_X 64
/** @brief Screen Y offset where the collision map starts (centers the 112px map on 224px screen) */
#define MAP_OFFSET_Y 56

/*============================================================================
 * Sprite Graphics
 *============================================================================*/

/**
 * @brief Player sprite tile (8x8, 4bpp = 32 bytes)
 *
 * A solid filled square using palette color 1 (white). In 4bpp format,
 * each tile is 32 bytes: bitplanes 0-1 interleaved (16 bytes), then
 * bitplanes 2-3 interleaved (16 bytes). Only bitplane 0 is set (0xFF),
 * so every pixel maps to color index 1.
 */
static const u8 player_tile[] = {
    /* Bitplanes 0,1: bp0=1,bp1=0 → color 1 */
    0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
    0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
    /* Bitplanes 2,3: all zero */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

/**
 * @brief Enemy sprite tile (8x8, 4bpp = 32 bytes)
 *
 * A solid filled square using palette color 2 (red). Only bitplane 1 is
 * set (0xFF per row), so every pixel maps to color index 2.
 */
static const u8 enemy_tile[] = {
    /* Bitplanes 0,1: bp0=0,bp1=1 → color 2 */
    0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF,
    0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF,
    /* Bitplanes 2,3: all zero */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

/**
 * @brief Wall background tile (8x8, 2bpp = 16 bytes)
 *
 * An outlined square: border pixels use color 1 (gray), interior is
 * color 0 (black/transparent). This provides visual feedback for the
 * collision map walls while the collision logic uses the separate
 * collision_map[] array.
 */
static const u8 wall_tile[] = {
    /* bp0=1,bp1=0 → color 1 for border, color 0 for interior */
    0xFF,0x00, 0x81,0x00, 0x81,0x00, 0x81,0x00,
    0x81,0x00, 0x81,0x00, 0x81,0x00, 0xFF,0x00,
};

/** @brief Empty background tile (8x8, 2bpp = 16 bytes, all zeros = transparent) */
static const u8 empty_tile[] = {
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

/** @brief BG palette (2 BGR555 colors): black background + gray walls */
static const u8 bg_palette[] = {
    0x00, 0x00,  /* Color 0: Black (background) */
    0x94, 0x52,  /* Color 1: Gray (walls) */
};

/*============================================================================
 * Sprite Palette
 *============================================================================*/

/**
 * @brief Sprite palette 0 -- normal state (CGRAM offset 128, 16 colors)
 *
 * Loaded at CGRAM byte offset 128 (sprite palette bank 0). Color 0 is
 * transparent (ignored by PPU for sprites), color 1 = white (player),
 * color 2 = red (enemy), color 3 = green (unused in normal state).
 */
static const u8 sprite_palette[] = {
    0x00, 0x00,  /* Color 0: Transparent */
    0xFF, 0x7F,  /* Color 1: White (player) - BGR555: max all channels */
    0x1F, 0x00,  /* Color 2: Red (enemy) - BGR555: B=0,G=0,R=31 */
    0xE0, 0x03,  /* Color 3: Green - BGR555: B=0,G=31,R=0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/**
 * @brief Sprite palette 1 -- collision highlight (CGRAM offset 144)
 *
 * When a collision is detected, the sprite's OAM attribute byte is
 * switched to palette bank 1, turning both the player and enemy green
 * as visual feedback. Colors 1 and 2 are both green so both the player
 * tile (color 1) and enemy tile (color 2) appear highlighted.
 */
static const u8 collision_palette[] = {
    0x00, 0x00,  /* Color 0: Transparent */
    0xE0, 0x03,  /* Color 1: Green (player colliding) */
    0xE0, 0x03,  /* Color 2: Green (enemy colliding) */
    0xFF, 0x7F,  /* Color 3: White */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/*============================================================================
 * Helper Functions
 *============================================================================*/

/**
 * @brief Load all tile and palette data into VRAM/CGRAM via DMA
 *
 * Called once during initialization with screen off (force blank).
 * Loads BG tiles (empty + wall) at VRAM $0000, sprite tiles (player +
 * enemy) at VRAM $4000, and sets up both BG and sprite color palettes.
 * Sprite palettes are loaded at CGRAM offsets 128 and 144 (banks 0 and 1).
 */
static void load_graphics(void) {
    setScreenOff();

    /* Load BG tiles via DMA (tile 0 = empty, tile 1 = wall) */
    dmaCopyVram((u8 *)empty_tile, 0x0000, 16);
    dmaCopyVram((u8 *)wall_tile, 0x0008, 16);

    /* Load sprite tiles via DMA at $4000 */
    dmaCopyVram((u8 *)player_tile, 0x4000, 32);
    dmaCopyVram((u8 *)enemy_tile, 0x4010, 32);

    /* Load palettes via DMA */
    dmaCopyCGram((u8 *)sprite_palette, 128, 32);
    dmaCopyCGram((u8 *)collision_palette, 144, 32);
    dmaCopyCGram((u8 *)bg_palette, 0, 4);
}

/**
 * @brief Render the collision map as BG tiles in VRAM
 *
 * First clears the entire 32x32 BG1 tilemap at VRAM $0400 with tile 0
 * (empty), then writes the 16x14 collision map into the tilemap at an
 * offset that centers it on screen. Each collision_map[] entry of 1
 * becomes BG tile 1 (wall outline), and 0 stays as tile 0 (empty).
 *
 * VRAM writes are done via direct PPU register access (REG_VMDATAL/H),
 * which only works during forced blank or VBlank.
 */
static void draw_tilemap(void) {
    u8 tx, ty;
    u16 addr;
    u8 tile;
    u16 i;

    REG_VMAIN = 0x80;

    /* Clear entire 32x32 tilemap at $0400 with tile 0 (empty) */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0;
    }

    /* Draw collision map tiles */
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

/**
 * @brief Write all sprite positions and attributes into the OAM buffer
 *
 * Uses direct oamMemory[] writes instead of oamSet() to avoid the
 * 158-byte stack frame overhead per call. Each sprite occupies 4 bytes:
 * [X_low, Y, tile_number, attribute]. The attribute byte encodes
 * priority (bits 5-4) and palette bank (bits 3-1). When a collision is
 * active, the palette bank switches from 0 (normal colors) to 1 (green
 * highlight). Setting oam_update_flag = 1 tells the NMI handler to DMA
 * the buffer to OAM during the next VBlank.
 */
static void update_sprites(void) {
    u8 i;
    u8 palette;
    u16 offset;

    /* Player sprite (ID 0) - green if colliding, white if not */
    palette = (collision_flags != 0) ? 1 : 0;
    oamMemory[0] = (u8)player_x;
    oamMemory[1] = (u8)player_y;
    oamMemory[2] = 0;  /* tile 0 */
    oamMemory[3] = (u8)((3 << 4) | (palette << 1));  /* priority 3 */

    /* Enemy sprites (IDs 1-4) */
    for (i = 0; i < NUM_ENEMIES; i++) {
        offset = (i + 1) << 2;
        palette = (collision_flags & (1 << i)) ? 1 : 0;
        oamMemory[offset] = (u8)enemy_x[i];
        oamMemory[offset + 1] = (u8)enemy_y[i];
        oamMemory[offset + 2] = 1;  /* tile 1 */
        oamMemory[offset + 3] = (u8)((2 << 4) | (palette << 1));  /* priority 2 */
    }

    oam_update_flag = 1;
}

/**
 * @brief Test player-vs-enemy AABB overlap and update collision_flags
 *
 * Rebuilds the player and all enemy bounding boxes from their current
 * positions, then calls collideRect() for each player-enemy pair. The
 * result is stored in collision_flags as a bitmask (bit N = enemy N is
 * overlapping). This bitmask drives the palette swap in update_sprites().
 */
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
            collision_flags |= (1 << i);
        }
    }
}

/**
 * @brief Test whether a position would overlap a solid wall tile
 *
 * Converts screen coordinates to collision map coordinates (subtracting
 * the map offset), then probes all four corners of the player's 8x8
 * bounding box against the collision_map[] via collideTile(). If any
 * corner lands on a solid tile (value 1), the function returns 1 to
 * reject the movement.
 *
 * The caller uses per-axis rejection: if the combined (new_x, new_y)
 * collides, it tries each axis independently so the player can slide
 * along walls instead of stopping dead.
 *
 * @param new_x Proposed player X position in screen coordinates
 * @param new_y Proposed player Y position in screen coordinates
 * @return 1 if the position overlaps a wall, 0 if clear
 */
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

/**
 * @brief Entry point -- initialize arena, run game loop with collision tests
 *
 * Sets up Mode 0 with BG1 for the wall tilemap and sprites for the player
 * and enemies. The main loop reads the D-pad every frame, proposes a new
 * position, validates it against the wall collision map (with per-axis
 * sliding), checks AABB overlaps with enemies, and updates all sprite
 * positions in the OAM buffer. The NMI handler handles the actual OAM
 * DMA transfer during VBlank.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    u16 pad;            /**< Current joypad button state (bitmask) */
    s16 new_x, new_y;  /**< Proposed player position before wall check */

    /* Initialize */
    consoleInit();
    setMode(BG_MODE0, 0);
    oamInit();

    /* Load graphics */
    load_graphics();

    /* Draw tilemap */
    draw_tilemap();

    /* Configure BG1 */
    bgSetMapPtr(0, 0x0400, BG_MAP_32x32);
    bgSetGfxPtr(0, 0x0000);
    REG_TM = TM_BG1 | TM_OBJ;

    /* Object settings */
    REG_OBJSEL = OBJSEL(OBJ_SIZE8_L16, 0x4000);

    /* Initialize player position - tile (7,7) = open area below center platform */
    player_x = MAP_OFFSET_X + 56;
    player_y = MAP_OFFSET_Y + 56;

    /* Initialize enemy positions - in the 4 open corners */
    enemy_x[0] = MAP_OFFSET_X + 16;   /* tile (2,2) - top-left open area */
    enemy_y[0] = MAP_OFFSET_Y + 16;

    enemy_x[1] = MAP_OFFSET_X + 104;  /* tile (13,2) - top-right open area */
    enemy_y[1] = MAP_OFFSET_Y + 16;

    enemy_x[2] = MAP_OFFSET_X + 16;   /* tile (2,11) - bottom-left open area */
    enemy_y[2] = MAP_OFFSET_Y + 88;

    enemy_x[3] = MAP_OFFSET_X + 104;  /* tile (13,11) - bottom-right open area */
    enemy_y[3] = MAP_OFFSET_Y + 88;

    /* Show sprites 0-4 (clear X high bits set by oamClear) */
    oamMemory[512] = 0x00;  /* Sprites 0-3: X high=0, size=small */
    oamMemory[513] = oamMemory[513] & 0xFC;  /* Sprite 4: clear X high bit */

    /* Initial update */
    check_collisions();
    update_sprites();

    /* Enable screen */
    setScreenOn();

    /* Main loop */
    while (1) {
        WaitForVBlank();

        pad = padHeld(0);
        new_x = player_x;
        new_y = player_y;

        if (pad & KEY_LEFT)  new_x = player_x - PLAYER_SPEED;
        if (pad & KEY_RIGHT) new_x = player_x + PLAYER_SPEED;
        if (pad & KEY_UP)    new_y = player_y - PLAYER_SPEED;
        if (pad & KEY_DOWN)  new_y = player_y + PLAYER_SPEED;

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

        /* Update all sprites via direct buffer writes */
        update_sprites();
    }

    return 0;
}
