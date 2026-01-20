/**
 * @file background.h
 * @brief SNES Background Layer Management
 *
 * Functions for configuring and scrolling background layers.
 *
 * ## Overview
 *
 * The SNES has up to 4 background layers (BG1-BG4), though not all
 * modes support all layers. Use these functions to configure tilemap
 * addresses, tile graphics addresses, and scroll positions.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_BACKGROUND_H
#define OPENSNES_BACKGROUND_H

#include <snes/types.h>

/*============================================================================
 * Constants
 *============================================================================*/

/** @defgroup bg_size Map Size Constants
 * @brief Constants for bgSetMapPtr() mapSize parameter
 * @{
 */
#define BG_MAP_32x32  0  /**< 32x32 tiles (1 screen) */
#define BG_MAP_64x32  1  /**< 64x32 tiles (2 screens wide) */
#define BG_MAP_32x64  2  /**< 32x64 tiles (2 screens tall) */
#define BG_MAP_64x64  3  /**< 64x64 tiles (4 screens) */

/* PVSnesLib compatibility aliases */
#define SC_32x32      BG_MAP_32x32
#define SC_64x32      BG_MAP_64x32
#define SC_32x64      BG_MAP_32x64
#define SC_64x64      BG_MAP_64x64
/** @} */

/** @defgroup bg_colors Color Mode Constants
 * @brief Constants for bgInitTileSet() colorMode parameter
 * @{
 */
#define BG_4COLORS    4   /**< 4 colors (2bpp) */
#define BG_16COLORS   16  /**< 16 colors (4bpp) */
#define BG_256COLORS  256 /**< 256 colors (8bpp) */

/* Mode 0 has separate subpalettes */
#define BG_4COLORS0   1   /**< 4 colors for Mode 0 (special handling) */
/** @} */

/*============================================================================
 * Scrolling
 *============================================================================*/

/**
 * @brief Set background scroll position
 *
 * @param bg Background number (0-3 for BG1-BG4)
 * @param x Horizontal scroll (0-1023)
 * @param y Vertical scroll (0-1023)
 *
 * @code
 * // Scroll BG1 right by 10 pixels
 * bgSetScroll(0, scrollX, 0);
 * @endcode
 */
void bgSetScroll(u8 bg, u16 x, u16 y);

/**
 * @brief Set horizontal scroll only
 *
 * @param bg Background number (0-3)
 * @param x Horizontal scroll
 */
void bgSetScrollX(u8 bg, u16 x);

/**
 * @brief Set vertical scroll only
 *
 * @param bg Background number (0-3)
 * @param y Vertical scroll
 */
void bgSetScrollY(u8 bg, u16 y);

/*============================================================================
 * Memory Configuration
 *============================================================================*/

/**
 * @brief Set background tilemap address and size
 *
 * Configures where the background's tilemap data is located in VRAM
 * and the map size (number of screens).
 *
 * @param bg Background number (0-3)
 * @param vramAddr VRAM word address (must be 1KB aligned, i.e., multiple of 0x400)
 * @param mapSize Map size (BG_MAP_32x32, BG_MAP_64x32, BG_MAP_32x64, or BG_MAP_64x64)
 *
 * @code
 * // Set BG1 tilemap at VRAM $0000, 32x32 size
 * bgSetMapPtr(0, 0x0000, BG_MAP_32x32);
 *
 * // Set BG2 tilemap at VRAM $0800, 64x32 size
 * bgSetMapPtr(1, 0x0800, BG_MAP_64x32);
 * @endcode
 */
void bgSetMapPtr(u8 bg, u16 vramAddr, u8 mapSize);

/**
 * @brief Set background tile graphics address
 *
 * Configures where the background's tile graphics (CHR) are located in VRAM.
 *
 * @param bg Background number (0-3)
 * @param vramAddr VRAM word address (must be 8KB aligned, i.e., multiple of 0x2000)
 *
 * @code
 * // Set BG1 tiles at VRAM $2000
 * bgSetGfxPtr(0, 0x2000);
 * @endcode
 *
 * @note BG1/BG2 share one register, BG3/BG4 share another.
 *       Setting one does not affect the other in the pair.
 */
void bgSetGfxPtr(u8 bg, u16 vramAddr);

/**
 * @brief Initialize background layer to defaults
 *
 * Resets scroll to (0,0) for the specified background.
 *
 * @param bg Background number (0-3)
 */
void bgInit(u8 bg);

/*============================================================================
 * Combined Initialization (PVSnesLib compatible)
 *============================================================================*/

/**
 * @brief Initialize tileset with tiles and palette
 *
 * Loads tile graphics to VRAM and palette to CGRAM, and configures
 * the background's tile graphics pointer.
 *
 * @param bgNumber Background number (0-3)
 * @param tileSource Address of tile graphics data
 * @param tilePalette Address of palette data
 * @param paletteEntry Palette slot (0-7 for 16-color, 0 for 256-color)
 * @param tileSize Size of tile data in bytes
 * @param paletteSize Size of palette data in bytes
 * @param colorMode Color mode (BG_4COLORS, BG_16COLORS, BG_256COLORS)
 * @param vramAddr VRAM address for tiles (must be 4KB aligned)
 *
 * @code
 * extern char tiles[], tiles_end[];
 * extern char palette[];
 * bgInitTileSet(0, tiles, palette, 0, tiles_end - tiles, 32, BG_16COLORS, 0x2000);
 * @endcode
 */
void bgInitTileSet(u8 bgNumber, u8 *tileSource, u8 *tilePalette,
                   u8 paletteEntry, u16 tileSize, u16 paletteSize,
                   u16 colorMode, u16 vramAddr);

/**
 * @brief Load tile graphics only (no palette)
 *
 * @param bgNumber Background number (0-3, or 0xFF to skip gfx pointer setup)
 * @param tileSource Address of tile graphics data
 * @param tileSize Size of tile data in bytes
 * @param vramAddr VRAM address for tiles
 */
void bgInitTileSetData(u8 bgNumber, u8 *tileSource, u16 tileSize, u16 vramAddr);

#endif /* OPENSNES_BACKGROUND_H */
