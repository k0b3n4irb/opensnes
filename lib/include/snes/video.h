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

/**
 * @brief Set background mode
 *
 * @param mode Background mode (0-7)
 *
 * Mode overview:
 * - 0: 4 BG layers, 4 colors each
 * - 1: 2 BG 16-color, 1 BG 4-color (most common)
 * - 7: Mode 7 rotation/scaling
 */
void setMode(u8 mode);

/*============================================================================
 * Palette
 *============================================================================*/

/**
 * @brief Set a single color in palette
 *
 * @param index Color index (0-255 for BG, 128-255 for sprites)
 * @param color 15-bit BGR color (0bBBBBBGGGGGRRRRR)
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
