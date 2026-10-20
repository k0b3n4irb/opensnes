/**
 * @file main.c
 * @brief Side-scrolling platformer with scroll streaming and SNESMOD audio
 * @ingroup examples
 *
 * A platformer on the rooftops of the city at night: Kit, the fox-kit
 * courier, runs and jumps from roof to roof. It demonstrates the core
 * techniques of a scrolling SNES game: tile-based map streaming, subpixel
 * physics, tile collision, camera tracking, animated sprites, a parallax
 * layer, and SPC700 music/sound effects via the SNESMOD audio driver.
 *
 * The map engine streams one tile column per frame using a prepare/flush
 * pattern: during active display, the next column is read from ROM into
 * a RAM buffer (map_prepare_column) and noted in the VRAM queue; during
 * VBlank, vramQueueFlush() sends it with one 64-byte DMA. This keeps VRAM
 * writes within the VBlank budget. The column at +32 tiles ahead is streamed to cover the 33rd
 * partial tile at the screen edge.
 *
 * Physics use 8.8 fixed-point velocities with arithmetic right shift for
 * signed values. Collision checks test two probe points per axis against
 * the tile property table (T_SOLID / T_EMPTY).
 *
 * This began as the PVSnesLib likemario example; the level, the roofs, the
 * skyline and Kit are ours (devtools/art_assets/build.py writes the level
 * data from the pictures in assets/neo-lumen/).
 *
 * @par SNES Concepts
 * - Scroll streaming: 1 column/frame prepare-during-display, DMA-in-VBlank
 * - SC_64x32 tilemap with 64-column wraparound for seamless scrolling
 * - Subpixel 8.8 fixed-point physics (acceleration, gravity, max velocity)
 * - Tile collision via property lookup table (tilesetatt)
 * - Dynamic sprite engine with frame-based animation (32x32 frames)
 * - A second layer scrolled at a quarter of the camera's speed (parallax)
 * - SNESMOD: SPC700 module playback and sound effects
 * - Assembly DMA loader with bank bytes for SUPERFREE ROM data
 *
 * @par What to Observe
 * - Kit walks left/right with acceleration and deceleration
 * - Press A to jump (hold UP for a higher jump): the gaps between the roofs
 *   need a jump, a few of the higher roofs need the high one
 * - Camera follows Kit; the far towers behind drift by more slowly
 * - Music plays continuously; a jump sound effect triggers on A press
 * - Falling between two roofs respawns Kit on the first one
 *
 * @par Modules Used
 * console, sprite, sprite_dynamic, sprite_lut, dma, input, background, anim,
 * vramqueue
 *
 * @see background.h, sprite.h, input.h, dma.h, snesmod.h
 */

#include <snes.h>
#include <snes/vramqueue.h>
#include <snes/anim.h>
#include <snes/snesmod.h>
#include "soundbank.h"

/*============================================================================
 * External Data (from data.asm)
 *============================================================================*/

/** @brief The level's tiles, the skyline of BG2 and Kit's frames (32x32, 4bpp, for the dynamic sprite engine) */
#include "res/tiles.inc"          /* generated: tiles_tiles[], tiles_pal[], tiles_map[], each with _end */
#include "res/skyline.inc"        /* generated: skyline_tiles[], skyline_pal[], skyline_map[] (64x32 in two pages), each with _end */
#include "res/kit.inc"            /* generated: kit_tiles[], kit_pal[], each with _end */
/** @brief Map data (tile indices + header with width/height) */
extern const u8 maplevel[];   /* asset bank: every C read must be a far read, hence const */
/** @brief Tile attribute table (T_SOLID/T_EMPTY per tile index, b16 format) */
extern const u8 tilesetatt[];

/** @brief Asset symbols from data.asm (.incbin sections in the asset banks).
 * A C pointer carries its bank byte, and dmaCopyVram/dmaCopyCGram read it. */

/*============================================================================
 * Constants
 *============================================================================*/

/** @brief VRAM word address for large (32x32) sprite tiles */
#define VRAM_SPR_LARGE  0x0000
/** @brief VRAM word address for small sprite tiles: the engine's fixed
 *  second pool; this game draws no small sprite, so BG2 uses the room */
#define VRAM_SPR_SMALL  0x1000
/** @brief VRAM word address for BG2 (the skyline) tiles, at most 256 */
#define VRAM_BG2_TILES  0x1000
/** @brief VRAM word address for BG1 tile character data (802 tiles, to $5220) */
#define VRAM_BG_TILES   0x2000
/** @brief VRAM word address for BG1 tilemap (SC_64x32) */
#define VRAM_BG_MAP     0x6800
/** @brief VRAM word address for BG2 tilemap (SC_64x32) */
#define VRAM_BG2_MAP    0x7000
/** @brief Where the 32x32 sprite is drawn relative to the 16x16 collision
 *  box: Kit's feet are on row 30 of his frame, the box's bottom is row 16 */
#define KIT_DRAW_DX     (-8)
/** @brief Vertical offset of the frame from the box (see KIT_DRAW_DX) */
#define KIT_DRAW_DY     (-14)

/**
 * @brief Gravity acceleration in 8.8 fixed-point units per frame.
 *
 * Applied to kit_yvel every frame when airborne (jumping or falling).
 * 48/256 = 0.1875 pixels/frame^2 -- gives a natural parabolic arc.
 */
#define GRAVITY         48
/** @brief Maximum horizontal velocity in 8.8 fixed-point (1.25 pixels/frame) */
#define KIT_MAXACCEL  0x0140
/** @brief Horizontal acceleration per frame in 8.8 fixed-point (0.22 pixels/frame) */
#define KIT_ACCEL     0x0038
/** @brief Normal jump initial upward velocity in 8.8 fixed-point */
#define KIT_JUMPING   0x0394
/** @brief High jump initial upward velocity (when holding UP during jump) */
#define KIT_HIJUMPING 0x0594

/** @brief Tile property: passable (air, background decoration) */
#define T_EMPTY  0x0000
/** @brief Tile property: solid (ground, walls, platforms) */
#define T_SOLID  0xFF00

/** @brief Kit is standing on solid ground, idle */
#define KIT_ACT_STAND  0
/** @brief Kit is walking (horizontal velocity nonzero, on ground) */
#define KIT_ACT_WALK   1
/** @brief Kit is jumping (upward velocity, ascending phase) */
#define KIT_ACT_JUMP   2
/** @brief Kit is falling (downward velocity or walked off an edge) */
#define KIT_ACT_FALL   3

/** @brief Kit's sheet (res/kit.png, four 32x32 frames a row): idle 0-3,
 *  walk 4-11, run 12-19, jump 20-27 (crouch, rise, top, fall, land) */
#define FRAME_IDLE   0
/** @brief First frame of the walk cycle */
#define FRAME_WALK   4
/** @brief Jump pose while going up */
#define FRAME_RISE   23
/** @brief Jump pose while coming down */
#define FRAME_FALL   25

/*============================================================================
 * Global State
 *============================================================================*/

static u16 map_width;          /**< Map width in tiles (parsed from map header) */
static u16 map_height;         /**< Map height in tiles (parsed from map header) */
static const u16 *map_data;          /**< Pointer to the tile index array within maplevel */
static const u16 *tile_props;        /**< Pointer to the tile property table (T_SOLID/T_EMPTY) */

/**
 * @brief Precomputed row pointers into map_data for fast tile lookup.
 *
 * Avoids an expensive multiply (row * map_width) on every tile access.
 * map_row_ptrs[row] points directly to the start of that row's tile data.
 * Maximum 32 rows for SC_64x32 tilemap height.
 */
static const u16 *map_row_ptrs[32];

static s16 camera_x;          /**< Current camera X scroll offset in pixels */
static s16 last_tile_x;       /**< Last tile column for which streaming was performed */

static s16 kit_x;           /**< Kit world X position in pixels */
static s16 kit_y;           /**< Kit world Y position in pixels */
static u8  kit_xfrac;       /**< Kit X sub-pixel fraction (8.8 fixed-point low byte) */
static u8  kit_yfrac;       /**< Kit Y sub-pixel fraction (8.8 fixed-point low byte) */
static s16 kit_xvel;        /**< Kit X velocity in 8.8 fixed-point */
static s16 kit_yvel;        /**< Kit Y velocity in 8.8 fixed-point (negative = upward) */
static u8  kit_action;      /**< Current action state (KIT_ACT_STAND/WALK/JUMP/FALL) */

/* Animation as data (anim module): one clip per action, one player.
 * animPlay() is called unconditionally each frame — continue-if-same
 * keeps the walk cycle running instead of restarting it.
 *
 * The clips are initialized RAM statics rather than DECLARE_ANIM_CLIP
 * (ROM const). That was a workaround when this ROM's bank $00 was within
 * a few bytes of full and a const read past it was wrong; since v0.41.0
 * const data lives in the asset banks and every C read of it is far, so
 * DECLARE_ANIM_CLIP would work here too. The 44 bytes of WRAM stay for
 * now; the layout and the API are identical. (Comment corrected
 * 2026-10-04; it described the pre-#127.3 world.) */
static u16 clip_walk_frames[]  = { FRAME_WALK, FRAME_WALK + 1, FRAME_WALK + 2, FRAME_WALK + 3,
                                   FRAME_WALK + 4, FRAME_WALK + 5, FRAME_WALK + 6, FRAME_WALK + 7 };
static u16 clip_jump_frames[]  = { FRAME_RISE };
static u16 clip_fall_frames[]  = { FRAME_FALL };
static u16 clip_stand_frames[] = { FRAME_IDLE, FRAME_IDLE + 1, FRAME_IDLE + 2, FRAME_IDLE + 3 };
static AnimClip clip_walk  = { clip_walk_frames,  0, 8, 5, ANIM_LOOP, 0 };
static AnimClip clip_jump  = { clip_jump_frames,  0, 1, 1, ANIM_LOOP, 0 };
static AnimClip clip_fall  = { clip_fall_frames,  0, 1, 1, ANIM_LOOP, 0 };
static AnimClip clip_stand = { clip_stand_frames, 0, 4, 12, ANIM_LOOP, 0 };
static AnimPlayer kit_anim = ANIM_PLAYER_INIT;  /**< Kit's animation state */

static s16 map_max_x;         /**< Rightmost valid Kit X position (map_width*8 - 16) */
static s16 cam_max_x;         /**< Maximum camera X offset (map_width*8 - 256) */

/** @brief SPC700 sound effect slot index for the jump sound */
static u8 sfx_jump_slot;

/**
 * @brief Column tile buffer for deferred VRAM streaming.
 *
 * During active display, map_prepare_column() fills this 32-entry
 * buffer from ROM map data (RAM-only writes) and notes it in the VRAM
 * queue; vramQueueFlush() sends the 64 bytes in VBlank.
 */
static u16 col_buffer[32];

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
    REG_VMAIN = VMAIN_INC32;
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

    REG_VMAIN = VMAIN_INC1;
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
    const u16 *hdr, *ptr;

    hdr = (const u16 *)maplevel;
    map_width = hdr[0] >> 3;
    map_height = hdr[1] >> 3;
    map_data = &hdr[3];

    tile_props = (const u16 *)tilesetatt;

    /* Build row pointer table for fast access */
    ptr = map_data;
    for (row = 0; row < map_height; row++) {
        map_row_ptrs[row] = ptr;
        ptr += map_width;
    }

    map_max_x = (s16)(map_width * 8 - 16);
    cam_max_x = (s16)(map_width * 8 - 256);

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
 * ROM map data into the col_buffer[] RAM array and notes the transfer in
 * the VRAM queue (a column: the address steps by 32 words). No VRAM write
 * occurs here -- vramQueueFlush() sends it during the next VBlank.
 *
 * @param map_col  Source column index in the map data
 * @param vram_col Target column in the SC_64x32 tilemap (0-63)
 */
static void map_prepare_column(u16 map_col, u16 vram_col) {
    u16 row, vram_addr;

    if (map_col >= map_width) return;

    /* Fill buffer during active display (RAM-only, no VRAM access) */
    for (row = 0; row < map_height; row++)
        col_buffer[row] = map_row_ptrs[row][map_col];
    for (; row < 32; row++)
        col_buffer[row] = 0;

    if (vram_col < 32)
        vram_addr = VRAM_BG_MAP + vram_col;
    else
        vram_addr = VRAM_BG_MAP + 0x0400 + (vram_col - 32);

    vramQueuePush((const u8 *)col_buffer, vram_addr, 64, VRAM_QUEUE_COLUMN);
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
 * Kit - Split into small functions to keep stack frames manageable
 *============================================================================*/

/**
 * @brief Initialize Kit's position, velocity, animation, and sprite configuration.
 *
 * Places Kit at a safe spawn point, sets the dynamic sprite engine's
 * frame to the standing pose, and configures the sprite attribute byte
 * (priority 3, H-flip for facing right). The tile data may sit in any ROM
 * bank (SUPERFREE placement); OAM_SET_GFX() records the bank byte of the
 * pointer it is given.
 */
static void kit_init(void) {
    kit_x = 48;
    kit_y = 128;      /* on the first roof: its surface is row 18 (y 144), the box is 16 tall */       /* Ground level (row 14 = tile 40 SOLID at y=112, so feet at 112 → y=96) */
    kit_xfrac = 0;
    kit_yfrac = 0;
    kit_xvel = 0;
    kit_yvel = 0;
    kit_action = KIT_ACT_STAND;
    animPlay(&kit_anim, &clip_stand);

    oambuffer[0].oamframeid = FRAME_IDLE;
    oambuffer[0].oamrefresh = 1;
    oambuffer[0].oamattribute = OBJ_PRIO(3);       /* the sheet faces right */
    OAM_SET_GFX(0, kit_tiles);
}

/**
 * @brief Process D-pad and button input for Kit's movement.
 *
 * LEFT/RIGHT apply acceleration (with max velocity clamping and
 * deceleration when released). A triggers a jump if grounded; holding
 * UP during jump gives a higher arc. The sprite's H-flip attribute is
 * set to match the facing direction.
 */
static void kit_handle_input(void) {
    u16 pad;

    pad = padHeld(0);

    if (pad & KEY_LEFT) {
        oambuffer[0].oamattribute = OBJ_PRIO(3) | OBJ_FLIPX;   /* mirror the right-facing sheet */
        if (kit_action == KIT_ACT_STAND)
            kit_action = KIT_ACT_WALK;
        kit_xvel -= KIT_ACCEL;
        if (kit_xvel < -KIT_MAXACCEL)
            kit_xvel = -KIT_MAXACCEL;
    } else if (pad & KEY_RIGHT) {
        oambuffer[0].oamattribute = OBJ_PRIO(3);
        if (kit_action == KIT_ACT_STAND)
            kit_action = KIT_ACT_WALK;
        kit_xvel += KIT_ACCEL;
        if (kit_xvel > KIT_MAXACCEL)
            kit_xvel = KIT_MAXACCEL;
    } else {
        if (kit_xvel > 0) {
            kit_xvel -= KIT_ACCEL;
            if (kit_xvel < 0) kit_xvel = 0;
        } else if (kit_xvel < 0) {
            kit_xvel += KIT_ACCEL;
            if (kit_xvel > 0) kit_xvel = 0;
        }
    }

    if (padPressed(0) & KEY_A) {
        if (kit_action == KIT_ACT_STAND || kit_action == KIT_ACT_WALK) {
            kit_action = KIT_ACT_JUMP;
            if (pad & KEY_UP)
                kit_yvel = -KIT_HIJUMPING;
            else
                kit_yvel = -KIT_JUMPING;
            snesmodPlayEffect(sfx_jump_slot, 127, 128, 3);
        }
    }

}

/**
 * @brief Arithmetic right shift by 8 bits (extract integer part of 8.8 fixed-point).
 *
 * The 65816 has no arithmetic shift right instruction, but cc65816's `>>`
 * on a signed value IS arithmetic (compiler check test_signed_ops.c), so
 * `val >> 8` would do; this helper spells the sign handling out:
 * - Positive values: simple unsigned shift
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
static void kit_apply_physics(void) {
    s16 new_frac;

    if (kit_action == KIT_ACT_JUMP || kit_action == KIT_ACT_FALL) {
        kit_yvel += GRAVITY;
        if (kit_yvel > 0x0400)
            kit_yvel = 0x0400;
    }

    new_frac = (s16)kit_xfrac + kit_xvel;
    kit_x += asr8(new_frac);
    kit_xfrac = new_frac & 0xFF;

    new_frac = (s16)kit_yfrac + kit_yvel;
    kit_y += asr8(new_frac);
    kit_yfrac = new_frac & 0xFF;
}

/**
 * @brief Check vertical tile collisions (ground landing and ceiling bonk).
 *
 * Ground check: probes two points at Kit's feet (left+2, right+13).
 * If either hits a solid tile, snaps Y to the tile boundary and resets
 * vertical velocity. Ceiling check: probes one point above Kit's head.
 * Collision detection uses the tile property table, not per-pixel checks.
 */
static void kit_collide_vertical(void) {
    u16 prop;

    /* Ground check */
    if (kit_yvel >= 0) {
        prop = map_get_tile_prop(kit_x + 2, kit_y + 16);
        if (prop == T_EMPTY)
            prop = map_get_tile_prop(kit_x + 13, kit_y + 16);

        if (prop != T_EMPTY) {
            kit_y = ((kit_y + 16) & 0xFFF8) - 16;
            kit_yfrac = 0;
            kit_yvel = 0;
            if (kit_action == KIT_ACT_FALL || kit_action == KIT_ACT_JUMP)
                kit_action = KIT_ACT_STAND;
        } else {
            if (kit_action == KIT_ACT_STAND || kit_action == KIT_ACT_WALK)
                kit_action = KIT_ACT_FALL;
        }
    }

    /* Ceiling check */
    if (kit_yvel < 0) {
        prop = map_get_tile_prop(kit_x + 8, kit_y - 1);
        if (prop != T_EMPTY) {
            kit_y = (kit_y & 0xFFF8) + 8;
            kit_yfrac = 0;
            kit_yvel = 0;
            kit_action = KIT_ACT_FALL;
        }
    }
}

/**
 * @brief Check horizontal tile collisions (wall sliding).
 *
 * Probes one point at Kit's vertical midpoint in the direction of
 * movement. If a solid tile is hit, snaps X to the tile boundary and
 * zeroes horizontal velocity. Only checks the direction Kit is
 * actually moving to avoid false positives.
 */
static void kit_collide_horizontal(void) {
    u16 prop;
    s16 mid_y;

    mid_y = kit_y + 8;

    if (kit_xvel > 0) {
        prop = map_get_tile_prop(kit_x + 15, mid_y);
        if (prop != T_EMPTY) {
            kit_x = ((kit_x + 15) & 0xFFF8) - 16;
            kit_xfrac = 0;
            kit_xvel = 0;
        }
    }

    if (kit_xvel < 0) {
        prop = map_get_tile_prop(kit_x, mid_y);
        if (prop != T_EMPTY) {
            kit_x = (kit_x & 0xFFF8) + 8;
            kit_xfrac = 0;
            kit_xvel = 0;
        }
    }
}

/**
 * @brief Clamp Kit to world boundaries and handle action state transitions.
 *
 * Prevents Kit from leaving the map horizontally or going above the
 * top. If Kit falls below Y=240 (off the bottom of the screen),
 * respawns at the top-left. Also handles state machine transitions:
 * WALK->STAND when velocity reaches zero, JUMP->FALL at the apex.
 */
static void kit_clamp_and_transition(void) {
    if (kit_x < 0) {
        kit_x = 0;
        kit_xvel = 0;
    }
    if (kit_x > map_max_x) {
        kit_x = map_max_x;
        kit_xvel = 0;
    }
    if (kit_y < 0) {
        kit_y = 0;
        kit_yvel = 0;
    }
    if (kit_y > 240) {
        kit_x = 48;
        kit_y = 64;
        kit_xvel = 0;
        kit_yvel = 0;
        kit_action = KIT_ACT_FALL;
    }

    if (kit_action == KIT_ACT_WALK && kit_xvel == 0)
        kit_action = KIT_ACT_STAND;
    if (kit_action == KIT_ACT_JUMP && kit_yvel >= 0)
        kit_action = KIT_ACT_FALL;
}

/**
 * @brief Update Kit's sprite animation frame based on current action.
 *
 * Pure state->clip mapping: select the clip for the current action and
 * tick. The anim module handles cadence (walk alternates every 4 ticks),
 * continue-if-same (walking keeps its phase), and the oamrefresh flag
 * (tile re-upload only when the frame actually changes).
 */
static void kit_animate(void) {
    if (kit_action == KIT_ACT_WALK)
        animPlay(&kit_anim, &clip_walk);
    else if (kit_action == KIT_ACT_JUMP)
        animPlay(&kit_anim, &clip_jump);
    else if (kit_action == KIT_ACT_FALL)
        animPlay(&kit_anim, &clip_fall);
    else
        animPlay(&kit_anim, &clip_stand);
    animTickOam(&kit_anim, 0);
}

/**
 * @brief Update camera to follow Kit and position the sprite on screen.
 *
 * Centers the camera on Kit (offset by half-screen width = 128 pixels),
 * clamped to the map boundaries. Then converts Kit's world position
 * to screen-relative coordinates for the sprite engine and triggers
 * the dynamic sprite draw for OAM buffer update.
 */
static void kit_update_camera(void) {
    camera_x = kit_x - 128;
    if (camera_x < 0) camera_x = 0;
    if (camera_x > cam_max_x)
        camera_x = cam_max_x;

    oambuffer[0].oamx = kit_x - camera_x + KIT_DRAW_DX;
    oambuffer[0].oamy = kit_y + KIT_DRAW_DY;
    oamDynamicDraw(0);
    bgSetScroll(1, camera_x >> 2, 0);   /* the far towers: a quarter of the speed */
}

/*============================================================================
 * Main
 *============================================================================*/

/**
 * @brief Main entry point -- platformer initialization and game loop.
 *
 * Initializes all subsystems during force blank: Mode 1 video, tile/palette
 * DMA via assembly loader, dynamic sprite engine, map loading, Kit state,
 * and SNESMOD audio (music + jump sound effect). Then enters the main loop
 * with a strict pipeline:
 *
 * Active display (CPU-only, no VRAM):
 *   1. kit_handle_input() - read pad, apply acceleration
 *   2. kit_apply_physics() - gravity, velocity integration
 *   3. kit_collide_vertical/horizontal() - tile collision response
 *   4. kit_clamp_and_transition() - boundary checks, state transitions
 *   5. kit_animate() - frame selection
 *   6. kit_update_camera() - camera + sprite positioning
 *   7. map_update() - prepare next column in RAM buffer
 *
 * VBlank (VRAM writes allowed):
 *   1. vramQueueFlush() - DMA the column noted (64 bytes)
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
    bgSetGfxPtr(1, VRAM_BG2_TILES);
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_64x32);

    /* DMA tile + palette assets: the pointers carry their bank. */
    dmaCopyVram(tiles_tiles,        VRAM_BG_TILES, tiles_tiles_end - tiles_tiles);
    dmaCopyCGram(tiles_pal,        0,             tiles_pal_end - tiles_pal);
    dmaCopyVram(skyline_tiles,      VRAM_BG2_TILES, skyline_tiles_end - skyline_tiles);
    dmaCopyVram(skyline_map,        VRAM_BG2_MAP,   skyline_map_end - skyline_map);
    dmaCopyCGram(skyline_pal,      16,            skyline_pal_end - skyline_pal);
    dmaCopyCGram(kit_pal,          128,           kit_pal_end - kit_pal);

    {
        static const OamDynamicConfig dyn_cfg = {
            .vramLarge     = VRAM_SPR_LARGE,
            .vramSmall     = VRAM_SPR_SMALL,
            .slotLargeInit = 0,
            .slotSmallInit = 0,
            .sizeMode      = OBJ_SIZE16_L32,
        };
        oamDynamicInit(&dyn_cfg);
    }

    map_load();     /* All VRAM writes safe — force blank from consoleInit */
    kit_init();

    /* Initialize SNESMOD audio */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodLoadModule(MOD_ROOFTOPS);
    sfx_jump_slot = snesmodLoadEffect(SFX_JUMP);

    setMainScreen(TM_BG1 | TM_BG2 | TM_OBJ);

    /* Initial sprite draw — queue Kit's tile upload. The NMI handler
     * runs even under force blank, so the WaitForVBlank below lets the
     * auto-flush hook drain the VRAM queue + sync OAM before we release
     * force blank. Kit is in VRAM by the time the first frame renders. */
    oambuffer[0].oamx = kit_x + KIT_DRAW_DX;
    oambuffer[0].oamy = kit_y + KIT_DRAW_DY;
    oamDynamicDraw(0);
    WaitForVBlank();

    setScreenOn();      /* Release force blank — display begins */

    snesmodPlay(0);
    snesmodSetModuleVolume(60);

    WaitForVBlank();

    while (1) {
        kit_handle_input();
        kit_apply_physics();
        kit_collide_vertical();
        kit_collide_horizontal();
        kit_clamp_and_transition();
        kit_animate();
        kit_update_camera();
        map_update();

        WaitForVBlank();
        /* VRAM operations first — must complete within VBlank.
         * NMI auto-flushes the dynamic sprite engine (end-frame +
         * VRAM tile queue), so no manual oamInit*EndFrame /
         * oamVramQueueUpdate calls are needed here. */
        vramQueueFlush();
        bgSetScroll(0, camera_x, 0);
        /* SPC700 communication last — no VBlank restriction */
        snesmodProcess();
    }

    return 0;
}
