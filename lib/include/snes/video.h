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
 * @note consoleInit() sets BG1 tilemap at VRAM $0400 and tile data at $0000.
 *       Use bgSetMapPtr() and bgSetGfxPtr() to customize after setMode().
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

/*============================================================================
 * Layer Enable Constants (for REG_TM / REG_TS / setMainScreen / setSubScreen)
 *============================================================================*/

/** @defgroup layer_enable Layer Enable Bit Masks
 * @brief OR these together for setMainScreen() / setSubScreen()
 * @{
 */
#define LAYER_BG1   0x01  /**< Background layer 1 */
#define LAYER_BG2   0x02  /**< Background layer 2 */
#define LAYER_BG3   0x04  /**< Background layer 3 */
#define LAYER_BG4   0x08  /**< Background layer 4 */
#define LAYER_OBJ   0x10  /**< Sprite (OBJ) layer */
/** @} */

/*============================================================================
 * Screen Layer Control
 *============================================================================*/

/**
 * @brief Enable layers on the main screen
 *
 * Sets which layers are visible on the main screen (REG_TM $212C).
 * Forgetting to call this (or set REG_TM directly) is the #1 cause
 * of blank screens after consoleInit().
 *
 * @param layers OR'd combination of LAYER_BG1..LAYER_OBJ
 *
 * @code
 * setMainScreen(LAYER_BG1 | LAYER_OBJ);   // Show BG1 + sprites
 * setMainScreen(LAYER_BG1 | LAYER_BG2);   // Show BG1 + BG2
 * @endcode
 */
#define setMainScreen(layers) (REG_TM = (u8)(layers))

/**
 * @brief Enable layers on the sub screen
 *
 * Sets which layers are visible on the sub screen (REG_TS $212D).
 * The sub screen is used as the second operand for color math.
 *
 * @param layers OR'd combination of LAYER_BG1..LAYER_OBJ
 *
 * @code
 * setSubScreen(LAYER_BG2);  // BG2 as color math source
 * @endcode
 */
#define setSubScreen(layers) (REG_TS = (u8)(layers))

/*============================================================================
 * Palette Helpers
 *============================================================================*/

/**
 * @brief Set a single CGRAM color
 *
 * Writes a 15-bit BGR color to the specified palette index.
 * Works during VBlank or force blank only.
 *
 * @param index Color index (0-255)
 * @param color 15-bit BGR color (use RGB() or RGB24() macros)
 *
 * @code
 * setColor(0, RGB(0, 0, 0));        // Color 0 = black
 * setColor(1, RGB(31, 31, 31));     // Color 1 = white
 * setColor(128, RGB(31, 0, 0));     // Sprite palette 0, color 0 = red
 * @endcode
 */
#define setColor(index, color) do { \
    REG_CGADD = (u8)(index); \
    REG_CGDATA = (u8)((color) & 0xFF); \
    REG_CGDATA = (u8)(((color) >> 8) & 0xFF); \
} while(0)

#endif /* OPENSNES_VIDEO_H */
