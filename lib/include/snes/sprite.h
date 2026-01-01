/**
 * @file sprite.h
 * @brief SNES Sprite (Object) Management
 *
 * Functions for managing hardware sprites (OBJ).
 *
 * ## SNES Sprite Limits
 *
 * - 128 sprites total
 * - 32 sprites per scanline
 * - Sizes: 8x8, 16x16, 32x32, 64x64
 * - 8 palettes (16 colors each, sharing color 0)
 *
 * ## Usage
 *
 * @code
 * // Initialize sprite system
 * oamInit();
 *
 * // Set up a sprite
 * oamSet(0, 100, 80, 0, 0, 0, 0);  // Sprite 0 at (100, 80)
 *
 * // In main loop
 * WaitForVBlank();
 * oamUpdate();  // Copy OAM buffer to hardware
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 *
 * ## Attribution
 *
 * Based on: PVSnesLib sprite system by Alekmaul
 */

#ifndef OPENSNES_SPRITE_H
#define OPENSNES_SPRITE_H

#include <snes/types.h>

/*============================================================================
 * Constants
 *============================================================================*/

/** @brief Maximum number of hardware sprites */
#define MAX_SPRITES 128

/** @brief Sprite sizes (for OBJSEL register) */
#define OBJ_SIZE8_L16   0   /**< Small=8x8, Large=16x16 */
#define OBJ_SIZE8_L32   1   /**< Small=8x8, Large=32x32 */
#define OBJ_SIZE8_L64   2   /**< Small=8x8, Large=64x64 */
#define OBJ_SIZE16_L32  3   /**< Small=16x16, Large=32x32 */
#define OBJ_SIZE16_L64  4   /**< Small=16x16, Large=64x64 */
#define OBJ_SIZE32_L64  5   /**< Small=32x32, Large=64x64 */

/** @brief Y position to hide sprite */
#define OBJ_HIDE_Y  240

/*============================================================================
 * Initialization
 *============================================================================*/

/**
 * @brief Initialize sprite system
 *
 * Clears OAM buffer and sets up default configuration.
 * Must be called before using any sprite functions.
 */
void oamInit(void);

/**
 * @brief Initialize sprite system with configuration
 *
 * @param size Sprite size configuration (OBJ_SIZE_*)
 * @param tileBase Base address for sprite tiles in VRAM (word address >> 13)
 */
void oamInitEx(u8 size, u8 tileBase);

/*============================================================================
 * Sprite Properties
 *============================================================================*/

/**
 * @brief Set sprite properties
 *
 * @param id Sprite ID (0-127)
 * @param x X position (0-511, negative wraps)
 * @param y Y position (0-255, use OBJ_HIDE_Y to hide)
 * @param tile Tile number (0-511)
 * @param palette Palette (0-7)
 * @param priority Priority (0-3, 3=highest)
 * @param flags Flip flags (bit 6 = H flip, bit 7 = V flip)
 *
 * @code
 * oamSet(0, 100, 80, 0, 0, 3, 0);  // Sprite 0, priority 3, no flip
 * oamSet(1, 50, 50, 4, 1, 2, 0x40);  // Sprite 1, palette 1, H-flip
 * @endcode
 */
void oamSet(u8 id, u16 x, u8 y, u16 tile, u8 palette, u8 priority, u8 flags);

/**
 * @brief Set sprite X position
 *
 * @param id Sprite ID (0-127)
 * @param x X position
 */
void oamSetX(u8 id, u16 x);

/**
 * @brief Set sprite Y position
 *
 * @param id Sprite ID (0-127)
 * @param y Y position
 */
void oamSetY(u8 id, u8 y);

/**
 * @brief Set sprite position
 *
 * @param id Sprite ID (0-127)
 * @param x X position
 * @param y Y position
 */
void oamSetXY(u8 id, u16 x, u8 y);

/**
 * @brief Set sprite tile
 *
 * @param id Sprite ID (0-127)
 * @param tile Tile number (0-511)
 */
void oamSetTile(u8 id, u16 tile);

/**
 * @brief Set sprite visibility
 *
 * @param id Sprite ID (0-127)
 * @param visible TRUE to show, FALSE to hide
 */
void oamSetVisible(u8 id, u8 visible);

/**
 * @brief Hide sprite
 *
 * @param id Sprite ID (0-127)
 */
void oamHide(u8 id);

/**
 * @brief Set sprite size (large/small)
 *
 * @param id Sprite ID (0-127)
 * @param large TRUE for large size, FALSE for small
 */
void oamSetSize(u8 id, u8 large);

/*============================================================================
 * OAM Update
 *============================================================================*/

/**
 * @brief Copy OAM buffer to hardware
 *
 * Call during VBlank to update sprite display.
 *
 * @code
 * WaitForVBlank();
 * oamUpdate();
 * @endcode
 */
void oamUpdate(void);

/**
 * @brief Clear all sprites
 *
 * Hides all sprites by moving them off-screen.
 */
void oamClear(void);

/*============================================================================
 * Metasprites
 *============================================================================*/

/**
 * @brief Draw a metasprite (multi-tile sprite)
 *
 * @param startId First sprite ID to use
 * @param x X position
 * @param y Y position
 * @param data Metasprite data (from gfx4snes)
 * @param palette Palette to use
 *
 * @return Number of hardware sprites used
 */
u8 oamDrawMetasprite(u8 startId, u16 x, u8 y, const u8* data, u8 palette);

#endif /* OPENSNES_SPRITE_H */
