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

#ifndef OPENSNES_MOSAIC_H
#define OPENSNES_MOSAIC_H

#include <snes/types.h>
#include <snes/registers.h>  /* REG_MOSAIC for the inline mosaicInit body */

/*============================================================================
 * Background Mask Constants
 *============================================================================*/

/* The backgrounds are named by LAYER_BG1..LAYER_BG4 (video.h), the same
 * bits everywhere a call takes a set of layers. */
#define MOSAIC_BG_ALL 0x0F    /**< Enable mosaic for all backgrounds */

/* MOSAIC_BG1..MOSAIC_BG4 were removed on 2026-10-05: use LAYER_BG1..LAYER_BG4
 * (video.h), the same values (docs/UPGRADING.md). */

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
 * @brief Set the backgrounds the mosaic applies to — REPLACES the previous set
 *
 * `mosaicSetLayers(LAYER_BG1)` after `mosaicSetLayers(LAYER_BG2)` leaves
 * only BG1 pixelated. This is the function mosaicEnable() was until
 * 2026-09-22, renamed because "Enable" reads as additive — windowEnable() IS
 * additive — and the old name silently undid the previous call.
 *
 * @param bgMask Bitmask of backgrounds (LAYER_BG1 | LAYER_BG2 | ...; 0
 *               disables, like mosaicDisable())
 *
 * Example:
 * @code
 * mosaicSetLayers(LAYER_BG1 | LAYER_BG2);  // BG1 and BG2, nothing else
 * mosaicSetLayers(MOSAIC_BG_ALL);            // every background
 * @endcode
 */
void mosaicSetLayers(u8 bgMask);

/** @brief The pre-2026-09-22 name of mosaicSetLayers(). Same behaviour: it
 *         REPLACES the background set. */
OPENSNES_DEPRECATED("use mosaicSetLayers() — this call replaces the background set, it does not add to it")
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
 * Note: a background must be selected with mosaicSetLayers() to see the effect.
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
 * mosaicSetLayers(LAYER_BG1 | LAYER_BG2 | LAYER_BG3 | LAYER_BG4);
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
 * mosaicSetLayers(LAYER_BG1 | LAYER_BG2 | LAYER_BG3 | LAYER_BG4);
 * mosaicFadeOut(2);  // Pixelate screen over ~30 frames
 * loadNewLevel();
 * mosaicFadeIn(2);   // Reveal new content
 * @endcode
 */
void mosaicFadeOut(u8 speed);

#endif /* OPENSNES_MOSAIC_H */
