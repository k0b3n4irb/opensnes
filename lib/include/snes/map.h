/**
 * @file map.h
 * @brief Scrolling tilemap engine for Mode 1 backgrounds
 *
 * Provides hardware-efficient tilemap streaming using metatiles (8x8 pixel blocks).
 * Supports horizontal and vertical scrolling with incremental VRAM updates
 * during VBlank.
 *
 * ## Usage
 *
 * @code
 * #include <snes.h>
 * #include <snes/map.h>
 *
 * extern u8 mapmario, tilesetdef, tilesetatt;
 *
 * // Background tileset must be loaded at VRAM $2000 (tiles) and $6800 (tilemap)
 * bgInitTileSet(0, &tileset, &tilesetpal, 0, size, 16*2, BG_16COLORS, 0x2000);
 * bgSetMapPtr(0, 0x6800, SC_64x32);
 *
 * // Load map data
 * mapLoad(&mapmario, &tilesetdef, &tilesetatt);
 *
 * // In main loop:
 * while (1) {
 *     mapUpdate();                   // Update scroll buffers
 *     mapUpdateCamera(playerx, playery);
 *     WaitForVBlank();
 *     mapVblank();                   // DMA updates to VRAM
 * }
 * @endcode
 *
 * @note Map engine uses VRAM address $6800 for the tilemap by default.
 *
 * ## Attribution
 *
 * Based on: PVSnesLib map engine by Alekmaul
 * Original: undisbeliever's castle_platformer
 * License: zlib (compatible with MIT)
 */

#ifndef OPENSNES_MAP_H
#define OPENSNES_MAP_H

#include <snes/types.h>

/*============================================================================
 * Tile Property Constants
 *============================================================================*/

/** @brief Empty tile (object will fall through) */
#define T_EMPTY     0x0000

/** @brief Solid tile (blocks movement) */
#define T_SOLID     0xFF00

/** @brief Ladder tile (object can climb) */
#define T_LADDER    0x0001

/** @brief Fire tile (object burns) */
#define T_FIRES     0x0002

/** @brief Spike tile (object dies) */
#define T_SPIKE     0x0004

/** @brief Platform tile (can jump through from below, land on top) */
#define T_PLATE     0x0008

/** @brief Slope 1x1 ascending right (floor rises left-to-right) */
#define T_SLOPEU1   0x0020

/** @brief Slope 1x1 descending right (floor drops left-to-right) */
#define T_SLOPED1   0x0021

/** @brief Slope 2x1 lower half ascending right */
#define T_SLOPELU2  0x0022

/** @brief Slope 2x1 lower half descending right */
#define T_SLOPELD2  0x0023

/** @brief Slope 2x1 upper half ascending right */
#define T_SLOPEUU2  0x0024

/** @brief Slope 2x1 upper half descending right */
#define T_SLOPEUD2  0x0025

/*============================================================================
 * Action Constants (used by both map and object engines)
 *============================================================================*/

#define ACT_STAND   0x0000  /**< Standing still */
#define ACT_WALK    0x0001  /**< Walking */
#define ACT_JUMP    0x0002  /**< Jumping */
#define ACT_FALL    0x0004  /**< Falling */
#define ACT_CLIMB   0x0008  /**< Climbing ladder */
#define ACT_DIE     0x0010  /**< Dying */
#define ACT_BURN    0x0020  /**< Burning */

/*============================================================================
 * Map Options
 *============================================================================*/

/** @brief Only allow scrolling to the right (no left scroll) */
#define MAP_OPT_1WAY   0x01

/** @brief Scroll BG2 instead of BG1 (VRAM address is still $6800) */
#define MAP_OPT_BG2    0x02

/*============================================================================
 * Exported Variables
 *============================================================================*/

/* --- Bank $00 SLOT 1 (C-accessible, < $2000) --- */

/** @brief Current camera X position in pixels */
extern u16 x_pos;

/** @brief Current camera Y position in pixels */
extern u16 y_pos;

/* --- Bank $7E SLOT 2 (NOT C-accessible, ASM-only) --- */
/* The map engine's bulk data lives in Bank $7E:
 *   metatiles[4096]     — metatile definitions (4 tile entries per metatile)
 *   metatilesprop[2048] — tile collision properties
 *   bg_L1[2048]         — BG1 tilemap buffer
 *   mapadrrowlut[1024]  — row address lookup table
 * These are managed internally by mapLoad/mapUpdate/mapVblank.
 * C code accesses map data through mapGetMetaTile/mapGetMetaTilesProp.
 */

/*============================================================================
 * Functions
 *============================================================================*/

/**
 * @brief Load map data into the engine
 *
 * Initializes metatile definitions, tile properties, and the row lookup table.
 * Also performs initial full-screen refresh.
 *
 * @param layer1map  Address of map data (format: u16 width, u16 height, u16 pad, then tile indices)
 * @param layertiles Address of metatile definitions (4 tiles per metatile, max 512 metatiles)
 * @param tilesprop  Address of tile property data (collision types)
 *
 * @note Must be called during forced blank or before screen is enabled.
 */
void mapLoad(u8 *layer1map, u8 *layertiles, u8 *tilesprop);

/**
 * @brief Update map scroll buffers based on camera position
 *
 * Call once per frame in the main loop. Builds horizontal/vertical
 * update buffers when the camera crosses tile boundaries.
 */
void mapUpdate(void);

/**
 * @brief Transfer map updates to VRAM
 *
 * Performs DMA transfers of scroll update buffers during VBlank.
 * Also updates BG scroll registers.
 *
 * @note Must be called during VBlank (after WaitForVBlank).
 */
void mapVblank(void);

/**
 * @brief Update camera position to follow an object
 *
 * Centers the camera on the given coordinates with boundary clamping.
 * Scroll trigger distances: X=128px from edges, Y=80px from edges.
 *
 * @param xpos X coordinate to focus on (in map pixels)
 * @param ypos Y coordinate to focus on (in map pixels)
 */
void mapUpdateCamera(u16 xpos, u16 ypos);

/**
 * @brief Get metatile index at map coordinates
 *
 * @param xpos X coordinate in map pixels
 * @param ypos Y coordinate in map pixels
 * @return Metatile index (0-511)
 */
u16 mapGetMetaTile(u16 xpos, u16 ypos);

/**
 * @brief Get tile collision property at map coordinates
 *
 * @param xpos X coordinate in map pixels
 * @param ypos Y coordinate in map pixels
 * @return Tile property value (T_EMPTY, T_SOLID, T_LADDER, etc.)
 */
u16 mapGetMetaTilesProp(u16 xpos, u16 ypos);

/**
 * @brief Set map engine options
 *
 * @param optmap Options bitmask (MAP_OPT_1WAY, MAP_OPT_BG2)
 */
void mapSetMapOptions(u8 optmap);

#endif /* OPENSNES_MAP_H */
