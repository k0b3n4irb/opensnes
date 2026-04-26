/**
 * @file main.c
 * @brief Side-scrolling platformer with scroll streaming and SNESMOD audio
 * @ingroup examples
 *
 * A Mario-style platformer demonstrating the core techniques of a scrolling
 * SNES game: tile-based map streaming, subpixel physics, tile collision,
 * camera tracking, animated sprites, and SPC700 music/sound effects via
 * the SNESMOD audio driver.
 *
 * The map engine streams one tile column per frame using a prepare/flush
 * pattern: during active display, the next column is read from ROM into
 * a RAM buffer (map_prepare_column); during VBlank, a 64-byte DMA writes
 * it to VRAM (map_flush_column). This keeps VRAM writes within the VBlank
 * budget. The column at +32 tiles ahead is streamed to cover the 33rd
 * partial tile at the screen edge.
 *
 * Physics use 8.8 fixed-point velocities with arithmetic right shift for
 * signed values. Collision checks test two probe points per axis against
 * the tile property table (T_SOLID / T_EMPTY).
 *
 * Port of the PVSnesLib likemario example.
 *
 * @par SNES Concepts
 * - Scroll streaming: 1 column/frame prepare-during-display, DMA-in-VBlank
 * - SC_64x32 tilemap with 64-column wraparound for seamless scrolling
 * - Subpixel 8.8 fixed-point physics (acceleration, gravity, max velocity)
 * - Tile collision via property lookup table (tilesetatt)
 * - Dynamic sprite engine with frame-based animation
 * - SNESMOD: SPC700 module playback and sound effects
 * - Assembly DMA loader with bank bytes for SUPERFREE ROM data
 *
 * @par What to Observe
 * - Mario walks left/right with acceleration and deceleration
 * - Press A to jump (hold UP for a higher jump)
 * - Camera follows Mario; background scrolls smoothly
 * - Music plays continuously; a jump sound effect triggers on A press
 * - Falling off the bottom respawns Mario at the top
 *
 * @par Modules Used
 * console, sprite, sprite_dynamic, sprite_lut, dma, input, background
 *
 * @see background.h, sprite.h, input.h, dma.h, snesmod.h
 */

#include <snes.h>
#include <snes/snesmod.h>
#include "soundbank.h"

/*============================================================================
 * External Data (from data.asm)
 *============================================================================*/

/** @brief Mario sprite tile data (16x16, 4bpp) for the dynamic sprite engine */
extern u8 mario_sprite_til[];
/** @brief Map data (tile indices + header with width/height) */
extern u8 mapmario[];
/** @brief Tile attribute table (T_SOLID/T_EMPTY per tile index, b16 format) */
extern u8 tilesetatt[];

/**
 * @brief DMA all tile and palette data from ROM to VRAM/CGRAM.
 *
 * Implemented in assembly because SUPERFREE sections may land in ROM
 * banks $01+. The assembly uses `:label` to get the correct bank byte
 * for each DMA source, which C cannot express.
 */
extern void loadGraphics(void);

/**
 * @brief Get the ROM bank byte of mario_sprite_til.
 *
 * The dynamic sprite engine needs the bank byte to set up DMA source
 * addresses for sprite tile uploads. Since the tiles are in a SUPERFREE
 * section, their bank is determined at link time and only available
 * via the `:label` assembly operator.
 *
 * @return Bank byte (e.g., 0x00, 0x01) of the mario_sprite_til data
 */
extern u8 getSpriteTilBank(void);

/*============================================================================
 * Constants
 *============================================================================*/

/** @brief VRAM word address for large (16x16) sprite tiles */
#define VRAM_SPR_LARGE  0x0000
/** @brief VRAM word address for small (8x8) sprite tiles */
#define VRAM_SPR_SMALL  0x1000
/** @brief VRAM word address for BG1 tile character data */
#define VRAM_BG_TILES   0x2000
/** @brief VRAM word address for BG1 tilemap (SC_64x32) */
#define VRAM_BG_MAP     0x6800

/**
 * @brief Gravity acceleration in 8.8 fixed-point units per frame.
 *
 * Applied to mario_yvel every frame when airborne (jumping or falling).
 * 48/256 = 0.1875 pixels/frame^2 -- gives a natural parabolic arc.
 */
#define GRAVITY         48
/** @brief Maximum horizontal velocity in 8.8 fixed-point (1.25 pixels/frame) */
#define MARIO_MAXACCEL  0x0140
/** @brief Horizontal acceleration per frame in 8.8 fixed-point (0.22 pixels/frame) */
#define MARIO_ACCEL     0x0038
/** @brief Normal jump initial upward velocity in 8.8 fixed-point */
#define MARIO_JUMPING   0x0394
/** @brief High jump initial upward velocity (when holding UP during jump) */
#define MARIO_HIJUMPING 0x0594

/** @brief Tile property: passable (air, background decoration) */
#define T_EMPTY  0x0000
/** @brief Tile property: solid (ground, walls, platforms) */
#define T_SOLID  0xFF00

/** @brief Mario is standing on solid ground, idle */
#define MARIO_ACT_STAND  0
/** @brief Mario is walking (horizontal velocity nonzero, on ground) */
#define MARIO_ACT_WALK   1
/** @brief Mario is jumping (upward velocity, ascending phase) */
#define MARIO_ACT_JUMP   2
/** @brief Mario is falling (downward velocity or walked off an edge) */
#define MARIO_ACT_FALL   3

/** @brief Dynamic sprite frame index: jump pose */
#define FRAME_JUMP   1
/** @brief Dynamic sprite frame index: walk animation frame 0 */
#define FRAME_WALK0  2
/** @brief Dynamic sprite frame index: walk animation frame 1 */
#define FRAME_WALK1  3
/** @brief Dynamic sprite frame index: standing idle pose */
#define FRAME_STAND  6

/*============================================================================
 * Global State
 *============================================================================*/

static u16 map_width;          /**< Map width in tiles (parsed from map header) */
static u16 map_height;         /**< Map height in tiles (parsed from map header) */
static u16 *map_data;          /**< Pointer to the tile index array within mapmario */
static u16 *tile_props;        /**< Pointer to the tile property table (T_SOLID/T_EMPTY) */

/**
 * @brief Precomputed row pointers into map_data for fast tile lookup.
 *
 * Avoids an expensive multiply (row * map_width) on every tile access.
 * map_row_ptrs[row] points directly to the start of that row's tile data.
 * Maximum 32 rows for SC_64x32 tilemap height.
 */
static u16 *map_row_ptrs[32];

static s16 camera_x;          /**< Current camera X scroll offset in pixels */
static s16 last_tile_x;       /**< Last tile column for which streaming was performed */

static s16 mario_x;           /**< Mario world X position in pixels */
static s16 mario_y;           /**< Mario world Y position in pixels */
static u8  mario_xfrac;       /**< Mario X sub-pixel fraction (8.8 fixed-point low byte) */
static u8  mario_yfrac;       /**< Mario Y sub-pixel fraction (8.8 fixed-point low byte) */
static s16 mario_xvel;        /**< Mario X velocity in 8.8 fixed-point */
static s16 mario_yvel;        /**< Mario Y velocity in 8.8 fixed-point (negative = upward) */
static u8  mario_action;      /**< Current action state (MARIO_ACT_STAND/WALK/JUMP/FALL) */
static u8  mario_anim_idx;    /**< Walk animation toggle (0 or 1, selects WALK0/WALK1) */
static u8  anim_tick;         /**< Frame counter for animation timing */

static s16 map_max_x;         /**< Rightmost valid Mario X position (map_width*8 - 16) */
static s16 cam_max_x;         /**< Maximum camera X offset (map_width*8 - 256) */

/** @brief SPC700 sound effect slot index for the jump sound */
static u8 sfx_jump_slot;

/**
 * @brief Column tile buffer for deferred VRAM streaming.
 *
 * During active display, map_prepare_column() fills this 32-entry
 * buffer from ROM map data (RAM-only writes). During VBlank,
 * map_flush_column() DMAs these 64 bytes to VRAM in one transfer.
 */
static u16 col_buffer[32];
static u16 col_vram_base;     /**< VRAM word address for the pending column DMA */
static u8  col_pending;       /**< Nonzero when col_buffer contains data awaiting flush */

/*============================================================================
 * Map Engine
 *============================================================================*/

/**
 * @brief Look up the collision property of the tile at pixel coordinates.
 *
 * Converts pixel coordinates to tile coordinates, reads the tile index
 * from the map via precomputed row pointers, then looks up the tile's
 * property (T_SOLID or T_EMPTY) in the attribute table. Out-of-bounds
 * coordinates above or left return T_SOLID (wall); below or right return
 * T_EMPTY (open air, allowing fall-off-screen respawn).
 *
 * @param px World X position in pixels
 * @param py World Y position in pixels
 * @return T_SOLID (0xFF00) or T_EMPTY (0x0000)
 */
static u16 map_get_tile_prop(s16 px, s16 py) {
    u16 tx, ty;

    if (px < 0 || py < 0) return T_SOLID;

    tx = (u16)px >> 3;
    ty = (u16)py >> 3;

    if (tx >= map_width || ty >= map_height) return T_EMPTY;

    return tile_props[map_row_ptrs[ty][tx] & 0x03FF];
}

/**
 * @brief Write a full tile column to VRAM via direct register writes.
 *
 * Used during force blank (initial map load) when DMA is not needed
 * because there is no time constraint. Sets VMAIN to vertical increment
 * mode (bit 0 set = increment address by 32 after each write), which
 * writes one tile per row down a column.
 *
 * @param map_col  Source column index in the map data
 * @param vram_col Destination column in the SC_64x32 tilemap (0-63)
 */
static void write_vram_column(u16 map_col, u16 vram_col) {
    u16 base, row, tile;

    if (map_col >= map_width) return;

    if (vram_col < 32)
        base = VRAM_BG_MAP + vram_col;
    else
        base = VRAM_BG_MAP + 0x0400 + (vram_col - 32);

    /* Caller must ensure force blank or VBlank */
    REG_VMAIN = 0x81;
    REG_VMADDL = base & 0xFF;
    REG_VMADDH = base >> 8;

    for (row = 0; row < map_height; row++) {
        tile = map_row_ptrs[row][map_col];
        REG_VMDATAL = tile & 0xFF;
        REG_VMDATAH = tile >> 8;
    }
    for (; row < 32; row++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0;
    }

    REG_VMAIN = 0x80;
}

/**
 * @brief Load the map from ROM and populate the initial VRAM tilemap.
 *
 * Parses the map header (width, height in pixels), builds the row pointer
 * table for O(1) tile lookups, and writes all 64 columns to VRAM. Must
 * be called during force blank since it performs direct VRAM writes.
 */
static void map_load(void) {
    u16 col, row;
    u16 *hdr, *ptr;

    hdr = (u16 *)mapmario;
    map_width = hdr[0] >> 3;
    map_height = hdr[1] >> 3;
    map_data = &hdr[3];

    tile_props = (u16 *)tilesetatt;

    /* Build row pointer table for fast access */
    ptr = map_data;
    for (row = 0; row < map_height; row++) {
        map_row_ptrs[row] = ptr;
        ptr += map_width;
    }

    map_max_x = (s16)(map_width * 8 - 16);
    cam_max_x = (s16)(map_width * 8 - 256);

    col_pending = 0;

    /* Force blank is already set by consoleInit — safe for VRAM writes */
    for (col = 0; col < 64; col++) {
        if (col < map_width)
            write_vram_column(col, col);
    }

    camera_x = 0;
    last_tile_x = 0;
}

/**
 * @brief Buffer a map column for deferred VRAM write (prepare phase).
 *
 * Called during active display (not VBlank). Reads tile indices from
 * ROM map data into the col_buffer[] RAM array and records the target
 * VRAM address. No VRAM writes occur here -- those happen in
 * map_flush_column() during the next VBlank.
 *
 * @param map_col  Source column index in the map data
 * @param vram_col Target column in the SC_64x32 tilemap (0-63)
 */
static void map_prepare_column(u16 map_col, u16 vram_col) {
    u16 row;

    if (map_col >= map_width) return;

    /* Fill buffer during active display (RAM-only, no VRAM access) */
    for (row = 0; row < map_height; row++)
        col_buffer[row] = map_row_ptrs[row][map_col];
    for (; row < 32; row++)
        col_buffer[row] = 0;

    if (vram_col < 32)
        col_vram_base = VRAM_BG_MAP + vram_col;
    else
        col_vram_base = VRAM_BG_MAP + 0x0400 + (vram_col - 32);

    col_pending = 1;
}

/**
 * @brief Flush the buffered column to VRAM via DMA (flush phase).
 *
 * Must be called during VBlank. Uses DMA channel 1 in word mode to
 * transfer 64 bytes (32 tilemap entries) to VRAM with vertical
 * increment addressing. DMA takes ~512 master cycles vs ~57,600 for
 * a compiled C register-write loop -- a 100x speedup that fits easily
 * within the VBlank budget.
 */
static void map_flush_column(void) {
    if (!col_pending) return;

    /* Set VRAM address and vertical column mode (increment by 32 words) */
    REG_VMAIN = 0x81;
    REG_VMADDL = col_vram_base & 0xFF;
    REG_VMADDH = col_vram_base >> 8;

    /* DMA channel 1: transfer col_buffer (64 bytes) to VRAM
     * Using DMA instead of C loop — the compiled register-write loop
     * takes ~57,600 master cycles (exceeds VBlank), while DMA takes ~512. */
    REG_DMAP(1) = 0x01;            /* 2-register word mode ($2118/$2119) */
    REG_BBAD(1) = 0x18;            /* Destination: VMDATAL */
    REG_A1TL(1) = (u16)col_buffer & 0xFF;
    REG_A1TH(1) = ((u16)col_buffer >> 8) & 0xFF;
    REG_A1B(1)  = 0x7E;            /* Source bank: WRAM */
    REG_DASL(1) = 64;              /* 32 words = 64 bytes */
    REG_DASH(1) = 0;
    REG_MDMAEN  = 0x02;            /* Fire DMA channel 1 */

    REG_VMAIN = 0x80;
    col_pending = 0;
}

/**
 * @brief Check if the camera has scrolled to a new tile column and prepare it.
 *
 * Compares the current camera tile position to last_tile_x. If the camera
 * moved right, prepares the column at +32 (one tile ahead of the visible
 * right edge, covering the 33rd partial tile). If moved left, prepares
 * the newly revealed left column. At most one column is prepared per frame.
 */
static void map_update(void) {
    s16 tile_x;
    u16 new_col, vram_col;

    tile_x = camera_x >> 3;

    /* Prepare at most 1 column per frame (buffered for VBlank flush).
     * Stream column at +32 (one ahead of visible edge) so partial
     * tiles at the right edge are always ready. */
    if (last_tile_x < tile_x) {
        last_tile_x++;
        new_col = last_tile_x + 32;
        if (new_col < map_width) {
            vram_col = new_col & 63;
            map_prepare_column(new_col, vram_col);
        }
    } else if (last_tile_x > tile_x) {
        last_tile_x--;
        new_col = last_tile_x;
        vram_col = new_col & 63;
        map_prepare_column(new_col, vram_col);
    }
}

/*============================================================================
 * Mario - Split into small functions to keep stack frames manageable
 *============================================================================*/

/**
 * @brief Initialize Mario's position, velocity, animation, and sprite configuration.
 *
 * Places Mario at a safe spawn point, sets the dynamic sprite engine's
 * frame to the standing pose, and configures the sprite attribute byte
 * (priority 3, H-flip for facing right). The bank byte for sprite tile
 * data is retrieved via getSpriteTilBank() because the tile data may
 * reside in any ROM bank due to SUPERFREE section placement.
 */
static void mario_init(void) {
    mario_x = 48;
    mario_y = 96;       /* Ground level (row 14 = tile 40 SOLID at y=112, so feet at 112 → y=96) */
    mario_xfrac = 0;
    mario_yfrac = 0;
    mario_xvel = 0;
    mario_yvel = 0;
    mario_action = MARIO_ACT_STAND;
    mario_anim_idx = 0;
    anim_tick = 0;

    oambuffer[0].oamframeid = FRAME_STAND;
    oambuffer[0].oamrefresh = 1;
    oambuffer[0].oamattribute = OBJ_PRIO(3) | 0x40;
    OAM_SET_GFX_BANK(0, mario_sprite_til, getSpriteTilBank());
}

/**
 * @brief Process D-pad and button input for Mario's movement.
 *
 * LEFT/RIGHT apply acceleration (with max velocity clamping and
 * deceleration when released). A triggers a jump if grounded; holding
 * UP during jump gives a higher arc. The sprite's H-flip attribute is
 * set to match the facing direction.
 */
static void mario_handle_input(void) {
    u16 pad;

    pad = padHeld(0);

    if (pad & KEY_LEFT) {
        oambuffer[0].oamattribute = OBJ_PRIO(3);
        if (mario_action == MARIO_ACT_STAND)
            mario_action = MARIO_ACT_WALK;
        mario_xvel -= MARIO_ACCEL;
        if (mario_xvel < -MARIO_MAXACCEL)
            mario_xvel = -MARIO_MAXACCEL;
    } else if (pad & KEY_RIGHT) {
        oambuffer[0].oamattribute = OBJ_PRIO(3) | 0x40;
        if (mario_action == MARIO_ACT_STAND)
            mario_action = MARIO_ACT_WALK;
        mario_xvel += MARIO_ACCEL;
        if (mario_xvel > MARIO_MAXACCEL)
            mario_xvel = MARIO_MAXACCEL;
    } else {
        if (mario_xvel > 0) {
            mario_xvel -= MARIO_ACCEL;
            if (mario_xvel < 0) mario_xvel = 0;
        } else if (mario_xvel < 0) {
            mario_xvel += MARIO_ACCEL;
            if (mario_xvel > 0) mario_xvel = 0;
        }
    }

    if (padPressed(0) & KEY_A) {
        if (mario_action == MARIO_ACT_STAND || mario_action == MARIO_ACT_WALK) {
            mario_action = MARIO_ACT_JUMP;
            if (pad & KEY_UP)
                mario_yvel = -MARIO_HIJUMPING;
            else
                mario_yvel = -MARIO_JUMPING;
            snesmodPlayEffect(sfx_jump_slot, 127, 128, 3);
        }
    }

}

/**
 * @brief Arithmetic right shift by 8 bits (extract integer part of 8.8 fixed-point).
 *
 * The 65816 has no native arithmetic shift right instruction, and the
 * compiler's LSR (logical shift) would zero-fill the sign bit, turning
 * negative values positive. This function works around it:
 * - Positive values: simple unsigned shift (LSR is correct)
 * - Negative values: bitwise NOT, unsigned shift, NOT back -- preserves
 *   the sign bit through the inversion trick.
 *
 * @param val 8.8 fixed-point signed value
 * @return Integer part (effectively val / 256 with sign preservation)
 */
static s16 asr8(s16 val) {
    if (val >= 0)
        return (u16)val >> 8;
    return ~(((u16)~val) >> 8);
}

/**
 * @brief Apply gravity and integrate velocity into position.
 *
 * Adds GRAVITY to Y velocity when airborne (capped at terminal velocity).
 * Then integrates both axes: adds velocity to the fractional accumulator,
 * extracts the integer pixel displacement via asr8(), and stores the
 * remaining fraction for sub-pixel precision next frame.
 */
static void mario_apply_physics(void) {
    s16 new_frac;

    if (mario_action == MARIO_ACT_JUMP || mario_action == MARIO_ACT_FALL) {
        mario_yvel += GRAVITY;
        if (mario_yvel > 0x0400)
            mario_yvel = 0x0400;
    }

    new_frac = (s16)mario_xfrac + mario_xvel;
    mario_x += asr8(new_frac);
    mario_xfrac = new_frac & 0xFF;

    new_frac = (s16)mario_yfrac + mario_yvel;
    mario_y += asr8(new_frac);
    mario_yfrac = new_frac & 0xFF;
}

/**
 * @brief Check vertical tile collisions (ground landing and ceiling bonk).
 *
 * Ground check: probes two points at Mario's feet (left+2, right+13).
 * If either hits a solid tile, snaps Y to the tile boundary and resets
 * vertical velocity. Ceiling check: probes one point above Mario's head.
 * Collision detection uses the tile property table, not per-pixel checks.
 */
static void mario_collide_vertical(void) {
    u16 prop;

    /* Ground check */
    if (mario_yvel >= 0) {
        prop = map_get_tile_prop(mario_x + 2, mario_y + 16);
        if (prop == T_EMPTY)
            prop = map_get_tile_prop(mario_x + 13, mario_y + 16);

        if (prop != T_EMPTY) {
            mario_y = ((mario_y + 16) & 0xFFF8) - 16;
            mario_yfrac = 0;
            mario_yvel = 0;
            if (mario_action == MARIO_ACT_FALL || mario_action == MARIO_ACT_JUMP)
                mario_action = MARIO_ACT_STAND;
        } else {
            if (mario_action == MARIO_ACT_STAND || mario_action == MARIO_ACT_WALK)
                mario_action = MARIO_ACT_FALL;
        }
    }

    /* Ceiling check */
    if (mario_yvel < 0) {
        prop = map_get_tile_prop(mario_x + 8, mario_y - 1);
        if (prop != T_EMPTY) {
            mario_y = (mario_y & 0xFFF8) + 8;
            mario_yfrac = 0;
            mario_yvel = 0;
            mario_action = MARIO_ACT_FALL;
        }
    }
}

/**
 * @brief Check horizontal tile collisions (wall sliding).
 *
 * Probes one point at Mario's vertical midpoint in the direction of
 * movement. If a solid tile is hit, snaps X to the tile boundary and
 * zeroes horizontal velocity. Only checks the direction Mario is
 * actually moving to avoid false positives.
 */
static void mario_collide_horizontal(void) {
    u16 prop;
    s16 mid_y;

    mid_y = mario_y + 8;

    if (mario_xvel > 0) {
        prop = map_get_tile_prop(mario_x + 15, mid_y);
        if (prop != T_EMPTY) {
            mario_x = ((mario_x + 15) & 0xFFF8) - 16;
            mario_xfrac = 0;
            mario_xvel = 0;
        }
    }

    if (mario_xvel < 0) {
        prop = map_get_tile_prop(mario_x, mid_y);
        if (prop != T_EMPTY) {
            mario_x = (mario_x & 0xFFF8) + 8;
            mario_xfrac = 0;
            mario_xvel = 0;
        }
    }
}

/**
 * @brief Clamp Mario to world boundaries and handle action state transitions.
 *
 * Prevents Mario from leaving the map horizontally or going above the
 * top. If Mario falls below Y=240 (off the bottom of the screen),
 * respawns at the top-left. Also handles state machine transitions:
 * WALK->STAND when velocity reaches zero, JUMP->FALL at the apex.
 */
static void mario_clamp_and_transition(void) {
    if (mario_x < 0) {
        mario_x = 0;
        mario_xvel = 0;
    }
    if (mario_x > map_max_x) {
        mario_x = map_max_x;
        mario_xvel = 0;
    }
    if (mario_y < 0) {
        mario_y = 0;
        mario_yvel = 0;
    }
    if (mario_y > 240) {
        mario_x = 48;
        mario_y = 32;
        mario_xvel = 0;
        mario_yvel = 0;
        mario_action = MARIO_ACT_FALL;
    }

    if (mario_action == MARIO_ACT_WALK && mario_xvel == 0)
        mario_action = MARIO_ACT_STAND;
    if (mario_action == MARIO_ACT_JUMP && mario_yvel >= 0)
        mario_action = MARIO_ACT_FALL;
}

/**
 * @brief Update Mario's sprite animation frame based on current action.
 *
 * Walking alternates between WALK0 and WALK1 every 4 frames. Jumping
 * and falling use the JUMP frame. Standing uses the STAND frame.
 * The oamrefresh flag tells the dynamic sprite engine to re-upload
 * the new frame's tile data to VRAM on the next flush.
 */
static void mario_animate(void) {
    anim_tick++;

    if (mario_action == MARIO_ACT_WALK) {
        if ((anim_tick & 3) == 3) {
            mario_anim_idx ^= 1;
            oambuffer[0].oamframeid = FRAME_WALK0 + mario_anim_idx;
            oambuffer[0].oamrefresh = 1;
        }
    } else if (mario_action == MARIO_ACT_JUMP || mario_action == MARIO_ACT_FALL) {
        if (oambuffer[0].oamframeid != FRAME_JUMP) {
            oambuffer[0].oamframeid = FRAME_JUMP;
            oambuffer[0].oamrefresh = 1;
        }
    } else {
        if (oambuffer[0].oamframeid != FRAME_STAND) {
            oambuffer[0].oamframeid = FRAME_STAND;
            oambuffer[0].oamrefresh = 1;
        }
    }
}

/**
 * @brief Update camera to follow Mario and position the sprite on screen.
 *
 * Centers the camera on Mario (offset by half-screen width = 128 pixels),
 * clamped to the map boundaries. Then converts Mario's world position
 * to screen-relative coordinates for the sprite engine and triggers
 * the dynamic sprite draw for OAM buffer update.
 */
static void mario_update_camera(void) {
    camera_x = mario_x - 128;
    if (camera_x < 0) camera_x = 0;
    if (camera_x > cam_max_x)
        camera_x = cam_max_x;

    oambuffer[0].oamx = mario_x - camera_x;
    oambuffer[0].oamy = mario_y;
    oamDynamic16Draw(0);
}

/*============================================================================
 * Main
 *============================================================================*/

/**
 * @brief Main entry point -- platformer initialization and game loop.
 *
 * Initializes all subsystems during force blank: Mode 1 video, tile/palette
 * DMA via assembly loader, dynamic sprite engine, map loading, Mario state,
 * and SNESMOD audio (music + jump sound effect). Then enters the main loop
 * with a strict pipeline:
 *
 * Active display (CPU-only, no VRAM):
 *   1. mario_handle_input() - read pad, apply acceleration
 *   2. mario_apply_physics() - gravity, velocity integration
 *   3. mario_collide_vertical/horizontal() - tile collision response
 *   4. mario_clamp_and_transition() - boundary checks, state transitions
 *   5. mario_animate() - frame selection
 *   6. mario_update_camera() - camera + sprite positioning
 *   7. map_update() - prepare next column in RAM buffer
 *
 * VBlank (VRAM writes allowed):
 *   1. map_flush_column() - DMA one column (64 bytes)
 *   2. bgSetScroll() - update hardware scroll registers
 *   3. oamVramQueueUpdate() - upload sprite tiles
 *   4. snesmodProcess() - SPC700 communication (no VBlank restriction)
 *
 * @return 0 (never reached -- infinite game loop)
 */
int main(void) {
    consoleInit();      /* Sets force blank ON — stays until setScreenOn */

    setMode(BG_MODE1, 0);

    bgSetGfxPtr(0, VRAM_BG_TILES);
    bgSetMapPtr(0, VRAM_BG_MAP, SC_64x32);

    /* DMA all tile/palette data with correct bank bytes (SUPERFREE may be bank $01+) */
    loadGraphics();

    oamInitDynamicSprite(VRAM_SPR_LARGE, VRAM_SPR_SMALL, 0, 0, OBJ_SIZE8_L16);

    map_load();     /* All VRAM writes safe — force blank from consoleInit */
    mario_init();

    /* Initialize SNESMOD audio */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodLoadModule(MOD_OVERWORLD);
    sfx_jump_slot = snesmodLoadEffect(SFX_JUMP);

    setMainScreen(TM_BG1 | TM_OBJ);

    /* Initial sprite draw + upload (still in force blank from consoleInit) */
    oambuffer[0].oamx = mario_x;
    oambuffer[0].oamy = mario_y;
    oamDynamic16Draw(0);
    oamVramQueueUpdate();
    oamInitDynamicSpriteEndFrame();

    setScreenOn();      /* Release force blank — display begins */

    snesmodPlay(0);
    snesmodSetModuleVolume(60);

    WaitForVBlank();

    while (1) {
        mario_handle_input();
        mario_apply_physics();
        mario_collide_vertical();
        mario_collide_horizontal();
        mario_clamp_and_transition();
        mario_animate();
        mario_update_camera();
        map_update();

        oamInitDynamicSpriteEndFrame();

        WaitForVBlank();
        /* VRAM operations first — must complete within VBlank */
        map_flush_column();
        bgSetScroll(0, camera_x, 0);
        oamVramQueueUpdate();
        /* SPC700 communication last — no VBlank restriction */
        snesmodProcess();
    }

    return 0;
}
