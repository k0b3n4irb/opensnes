/**
 * @file mosaic.h
 * @brief SNES Mosaic Effects
 *
 * Mosaic is a PPU effect that pixelates backgrounds, commonly used for:
 * - Screen transitions (fade to mosaic, then change scene)
 * - Damage/death effects
 * - Retro visual styles
 *
 * Hardware: Register $2106 (MOSAIC)
 * - Bits 7-4: Mosaic size (0-15, block size = value + 1 pixels)
 * - Bit 3: Enable mosaic for BG4
 * - Bit 2: Enable mosaic for BG3
 * - Bit 1: Enable mosaic for BG2
 * - Bit 0: Enable mosaic for BG1
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef SNES_MOSAIC_H
#define SNES_MOSAIC_H

#include <snes/types.h>

/*============================================================================
 * Background Mask Constants
 *============================================================================*/

#define MOSAIC_BG1    0x01    /**< Enable mosaic for BG1 */
#define MOSAIC_BG2    0x02    /**< Enable mosaic for BG2 */
#define MOSAIC_BG3    0x04    /**< Enable mosaic for BG3 */
#define MOSAIC_BG4    0x08    /**< Enable mosaic for BG4 */
#define MOSAIC_BG_ALL 0x0F    /**< Enable mosaic for all backgrounds */

/*============================================================================
 * Mosaic Size Constants
 *============================================================================*/

#define MOSAIC_MIN    0       /**< Minimum mosaic (1x1, no visible effect) */
#define MOSAIC_MAX    15      /**< Maximum mosaic (16x16 pixel blocks) */

/*============================================================================
 * Function Declarations
 *============================================================================*/

/**
 * @brief Initialize mosaic system
 *
 * Disables all mosaic effects. Call this once during setup.
 */
void mosaicInit(void);

/**
 * @brief Enable mosaic effect for specified backgrounds
 *
 * @param bgMask Bitmask of backgrounds (MOSAIC_BG1 | MOSAIC_BG2 | etc.)
 *
 * Example:
 * @code
 * mosaicEnable(MOSAIC_BG1 | MOSAIC_BG2);  // Enable for BG1 and BG2
 * mosaicEnable(MOSAIC_BG_ALL);            // Enable for all backgrounds
 * @endcode
 */
void mosaicEnable(u8 bgMask);

/**
 * @brief Disable mosaic effect for all backgrounds
 */
void mosaicDisable(void);

/**
 * @brief Set mosaic pixel block size
 *
 * @param size Mosaic level (0-15)
 *             0 = 1x1 pixels (no visible effect)
 *             15 = 16x16 pixel blocks (maximum pixelation)
 *
 * Note: Mosaic must be enabled with mosaicEnable() to see the effect.
 */
void mosaicSetSize(u8 size);

/**
 * @brief Get current mosaic size
 *
 * @return Current mosaic size (0-15)
 */
u8 mosaicGetSize(void);

/**
 * @brief Animate mosaic fade in (pixelated -> clear)
 *
 * Smoothly decreases mosaic from current level to 0.
 * Blocks until animation completes.
 *
 * @param speed Frames to wait between steps (1 = fast, higher = slower)
 *
 * Example:
 * @code
 * mosaicSetSize(MOSAIC_MAX);
 * mosaicEnable(MOSAIC_BG_ALL);
 * mosaicFadeIn(2);  // Reveal screen over ~30 frames
 * @endcode
 */
void mosaicFadeIn(u8 speed);

/**
 * @brief Animate mosaic fade out (clear -> pixelated)
 *
 * Smoothly increases mosaic from current level to maximum.
 * Blocks until animation completes.
 *
 * @param speed Frames to wait between steps (1 = fast, higher = slower)
 *
 * Example:
 * @code
 * mosaicEnable(MOSAIC_BG_ALL);
 * mosaicFadeOut(2);  // Pixelate screen over ~30 frames
 * loadNewLevel();
 * mosaicFadeIn(2);   // Reveal new content
 * @endcode
 */
void mosaicFadeOut(u8 speed);

#endif /* SNES_MOSAIC_H */
