/**
 * @file video.h
 * @brief SNES Video / PPU Functions
 *
 * Low-level video functions for PPU control, palette management,
 * and screen modes.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 *
 * @todo Implement video functions
 */

#ifndef OPENSNES_VIDEO_H
#define OPENSNES_VIDEO_H

#include <snes/types.h>

/*============================================================================
 * Background Modes
 *============================================================================*/

/** @defgroup bg_modes Background Mode Constants
 * @brief Constants for setMode() parameter
 * @{
 */
#define BG_MODE0    0   /**< Mode 0: 4 BG layers, 4 colors each */
#define BG_MODE1    1   /**< Mode 1: 2 BG 16-color, 1 BG 4-color (most common) */
#define BG_MODE2    2   /**< Mode 2: 2 BG 16-color, offset-per-tile */
#define BG_MODE3    3   /**< Mode 3: 1 BG 256-color, 1 BG 16-color */
#define BG_MODE4    4   /**< Mode 4: 1 BG 256-color, 1 BG 4-color, offset-per-tile */
#define BG_MODE5    5   /**< Mode 5: 1 BG 16-color, 1 BG 4-color, hi-res */
#define BG_MODE6    6   /**< Mode 6: 1 BG 16-color, offset-per-tile, hi-res */
#define BG_MODE7    7   /**< Mode 7: 1 BG 256-color, rotation/scaling */
/** @} */

/** @defgroup bg_priority Priority Flags
 * @brief Flags to combine with mode for setMode()
 * @{
 */
#define BG3_MODE1_PRIORITY_HIGH  0x08  /**< Give BG3 high priority in Mode 1 */
/** @} */

/**
 * @brief Set background mode
 *
 * @param mode Background mode (BG_MODE0-BG_MODE7), optionally OR'd with priority flags
 *
 * Mode overview:
 * - 0: 4 BG layers, 4 colors each
 * - 1: 2 BG 16-color, 1 BG 4-color (most common)
 * - 7: Mode 7 rotation/scaling
 *
 * @code
 * // Mode 1 with BG3 having high priority (for HUD overlay)
 * setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
 * @endcode
 */
void setMode(u8 mode, u8 flags);

/*============================================================================
 * Palette
 *============================================================================*/

/**
 * @brief Set a single color in palette
 *
 * @param index Color index (0-255 for BG, 128-255 for sprites)
 * @param color 15-bit BGR color (0bBBBBBGGGGGRRRRR)
 *
 * @todo Not yet implemented - use direct register writes:
 * @code
 * REG_CGADD = index;
 * REG_CGDATA = color & 0xFF;
 * REG_CGDATA = color >> 8;
 * @endcode
 */
void setPaletteColor(u8 index, u16 color);

/**
 * @brief Create RGB color value
 *
 * @param r Red (0-31)
 * @param g Green (0-31)
 * @param b Blue (0-31)
 * @return 15-bit BGR color
 */
#define RGB(r, g, b) (((b) << 10) | ((g) << 5) | (r))

/**
 * @brief Convert 24-bit RGB to SNES format
 *
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 * @return 15-bit BGR color
 */
#define RGB24(r, g, b) RGB((r) >> 3, (g) >> 3, (b) >> 3)

/* Placeholder - more functions to be implemented */

#endif /* OPENSNES_VIDEO_H */
