/**
 * @file colormath.h
 * @brief SNES Color Math (Blending/Transparency)
 *
 * Color math enables blending between the main screen and sub screen or
 * a fixed color. This creates effects like transparency, shadows, fading,
 * and underwater tints.
 *
 * ## How Color Math Works
 *
 * The SNES can add or subtract colors:
 * - **Add**: Main screen + Sub screen (or fixed color) = brighter
 * - **Subtract**: Main screen - Sub screen (or fixed color) = darker
 *
 * Half mode divides the result by 2, useful for 50% transparency.
 *
 * ## Main Screen vs Sub Screen
 *
 * - Main screen: Primary display (what you normally see)
 * - Sub screen: Secondary layer used for blending
 *
 * For transparency, put the background on main screen and the
 * transparent layer on sub screen.
 *
 * ## Usage Example
 *
 * @code
 * // 50% blend of BG2 over BG1: BG1 on the main screen, BG2 on the sub
 * // screen, math enabled on the MAIN-screen layer (CGADSUB bits 0-5 name
 * // main-screen layers — fullsnes; until 2026-10-04 this example put BG2
 * // on both screens and enabled math on BG2, which blends BG2 with itself)
 * REG_TM = TM_BG1;
 * REG_TS = TM_BG2;
 *
 * colorMathSetLayers(LAYER_BG1);  // Apply math where BG1 is drawn
 * colorMathSetOp(COLORMATH_ADD);   // Add mode
 * colorMathSetHalf(1);              // Divide by 2 = 50%
 * colorMathSetSource(COLORMATH_SRC_SUBSCREEN);  // Blend with sub screen
 * @endcode
 *
 * @code
 * // Fade to black
 * colorMathSetFixedColor(0, 0, 0);  // Black
 * colorMathSetSource(COLORMATH_SRC_FIXED);
 * colorMathSetOp(COLORMATH_SUB);    // Subtract = darken
 * colorMathSetLayers(COLORMATH_ALL);   // Apply to all layers
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_COLORMATH_H
#define OPENSNES_COLORMATH_H

#include <snes/types.h>
#include <snes/registers.h>  /* REG_CGWSEL / REG_CGADSUB / REG_COLDATA */

/*============================================================================
 * Layer Masks (for colorMathSetLayers)
 *============================================================================*/

/* The layers are named by LAYER_BG1..LAYER_BG4 and LAYER_OBJ (video.h), the
 * same bits everywhere a call takes a set of layers. The backdrop is the one
 * bit only color math has. */

/** @brief Apply color math to backdrop (color 0) — color math's own bit */
#define COLORMATH_BACKDROP  BIT(5)

/** @brief Apply color math to all layers and the backdrop */
#define COLORMATH_ALL       0x3F

/** @name Deprecated layer names (use LAYER_*)
 * @{ */
#define COLORMATH_BG1       BIT(0)  /**< @deprecated use LAYER_BG1 */
#define COLORMATH_BG2       BIT(1)  /**< @deprecated use LAYER_BG2 */
#define COLORMATH_BG3       BIT(2)  /**< @deprecated use LAYER_BG3 */
#define COLORMATH_BG4       BIT(3)  /**< @deprecated use LAYER_BG4 */
#define COLORMATH_OBJ       BIT(4)  /**< @deprecated use LAYER_OBJ */
/** @} */
#ifdef __clang__
#pragma clang deprecated(COLORMATH_BG1, "use LAYER_BG1")
#pragma clang deprecated(COLORMATH_BG2, "use LAYER_BG2")
#pragma clang deprecated(COLORMATH_BG3, "use LAYER_BG3")
#pragma clang deprecated(COLORMATH_BG4, "use LAYER_BG4")
#pragma clang deprecated(COLORMATH_OBJ, "use LAYER_OBJ")
#endif

/*============================================================================
 * Color Math Operations
 *============================================================================*/

/** @brief Add colors (brightens) */
#define COLORMATH_ADD       0

/** @brief Subtract colors (darkens) */
#define COLORMATH_SUB       1

/*============================================================================
 * Color Math Source
 *============================================================================*/

/** @brief Use sub screen as color source */
#define COLORMATH_SRC_SUBSCREEN 0

/** @brief Use fixed color as color source */
#define COLORMATH_SRC_FIXED     1

/*============================================================================
 * Color Math Enable Conditions
 *============================================================================*/

/** @brief Always enable color math */
#define COLORMATH_ALWAYS    0

/** @brief Enable inside window only */
#define COLORMATH_INSIDE    1

/** @brief Enable outside window only */
#define COLORMATH_OUTSIDE   2

/** @brief Never enable color math */
#define COLORMATH_NEVER     3

/*============================================================================
 * Fixed Color Channel Masks
 *============================================================================*/

/** @brief Apply fixed color to red channel */
#define COLDATA_RED     0x20

/** @brief Apply fixed color to green channel */
#define COLDATA_GREEN   0x40

/** @brief Apply fixed color to blue channel */
#define COLDATA_BLUE    0x80

/** @brief Apply fixed color to all channels */
#define COLDATA_ALL     (COLDATA_RED | COLDATA_GREEN | COLDATA_BLUE)

/*============================================================================
 * Core Color Math Functions
 *============================================================================*/

/**
 * @brief Initialize color math to defaults
 *
 * Disables all color math effects.
 */
void colorMathInit(void);

/**
 * @brief Set the layers colour math applies to — REPLACES the previous set
 *
 * `colorMathSetLayers(LAYER_BG1)` after `colorMathSetLayers(LAYER_BG2)`
 * leaves only BG1 blended. This is the function colorMathEnable() was until
 * 2026-09-22; it is renamed because "Enable" reads as additive — windowEnable()
 * IS additive — and the old name silently undid the previous call.
 *
 * @param layers Layer mask (LAYER_BG1, LAYER_BG2, ...; 0 disables)
 */
void colorMathSetLayers(u8 layers);

/** @brief The pre-2026-09-22 name of colorMathSetLayers(). Same behaviour:
 *         it REPLACES the layer set. */
OPENSNES_DEPRECATED("use colorMathSetLayers() — this call replaces the layer set, it does not add to it")
void colorMathEnable(u8 layers);

/**
 * @brief Disable all color math
 */
void colorMathDisable(void);

/**
 * @brief Set color math operation (add or subtract)
 *
 * @param op Operation (COLORMATH_ADD or COLORMATH_SUB)
 */
void colorMathSetOp(u8 op);

/**
 * @brief Enable or disable half mode
 *
 * When enabled, the color math result is divided by 2 — except where the
 * sub-screen pixel is transparent (the fixed colour stands in as the sub
 * backdrop, without division) and when the main screen is forced black
 * (CGWSEL): fullsnes, CGADSUB bit 6. A 50 % blend therefore shows full
 * brightness wherever the sub screen has nothing to blend with.
 * This creates 50% transparency/blending.
 *
 * @param enable 1 = divide by 2, 0 = full result
 */
void colorMathSetHalf(u8 enable);

/**
 * @brief Set color math source
 *
 * Chooses whether to blend with the sub screen or a fixed color.
 *
 * @param source COLORMATH_SRC_SUBSCREEN or COLORMATH_SRC_FIXED
 */
void colorMathSetSource(u8 source);

/**
 * @brief Set when color math is enabled (window control)
 *
 * @param condition COLORMATH_ALWAYS, COLORMATH_INSIDE, COLORMATH_OUTSIDE, or COLORMATH_NEVER
 */
void colorMathSetCondition(u8 condition);

/**
 * @brief Enable or disable direct color mode (CGWSEL bit 0)
 *
 * In direct color mode the PPU stops looking 256-color (8bpp) BG
 * pixels up in CGRAM: the tile's pixel byte IS the color, interpreted
 * as BBGGGRRR (2 bits blue, 3 green, 3 red) and expanded to 15-bit
 * BGR by shifting each field left — B: bits 7-6 -> color bits 14-13,
 * G: bits 5-3 -> 9-7, R: bits 2-0 -> 4-2. The tilemap entry's palette
 * bits stop selecting a palette and instead supply one extra LOW bit
 * per channel (pal bit 0 -> red bit 1, pal bit 1 -> green bit 6, pal
 * bit 2 -> blue bit 12), for 2048 distinct colors with no CGRAM cost.
 *
 * Only affects 8bpp layers: BG1 in modes 3/4 and the Mode 7 layer
 * (EXTBG included). 2/4bpp layers and sprites keep using CGRAM, so a
 * HUD or sprite palette coexists untouched. Worked example:
 * `examples/graphics/effects/direct_color` (the same VRAM bytes drawn
 * both ways).
 *
 * @param enable 1 = pixel bytes are colors, 0 = CGRAM lookup (default)
 */
void colorMathSetDirectColor(u8 enable);

/**
 * @brief Set fixed color for blending
 *
 * Sets the fixed color used when source is COLORMATH_SRC_FIXED.
 *
 * @param r Red intensity (0-31)
 * @param g Green intensity (0-31)
 * @param b Blue intensity (0-31)
 */
void colorMathSetFixedColor(u8 r, u8 g, u8 b);

/**
 * @brief Set a single fixed color channel
 *
 * @param channel Channel mask (COLDATA_RED, COLDATA_GREEN, COLDATA_BLUE)
 * @param intensity Intensity (0-31)
 */
void colorMathSetChannel(u8 channel, u8 intensity);

/*============================================================================
 * Color Math Effect Helpers
 *============================================================================*/

/**
 * @brief Set up 50% transparency for layers
 *
 * Quick setup for semi-transparent layers. The transparent layers
 * should be on both main and sub screen.
 *
 * @param layers Layers to make 50% transparent
 *
 * @note You must also set REG_TS to include the transparent layers
 */
void colorMathTransparency50(u8 layers);

/**
 * @brief Set up shadow/darkening effect
 *
 * Subtracts fixed color from layers to darken them.
 *
 * @param layers Layers to darken
 * @param intensity Darkness level (0-31, higher = darker)
 */
void colorMathShadow(u8 layers, u8 intensity);

/**
 * @brief Set up color tint effect
 *
 * Adds a fixed color tint to layers.
 *
 * @param layers Layers to tint
 * @param r Red tint (0-31)
 * @param g Green tint (0-31)
 * @param b Blue tint (0-31)
 */
void colorMathTint(u8 layers, u8 r, u8 g, u8 b);

/**
 * @brief Set brightness for fade effects
 *
 * Sets the fixed color for use in fading. Use with colorMathSetOp()
 * to fade to white (add) or black (subtract).
 *
 * @param brightness Fade level (0 = no effect, 31 = full white/black)
 */
void colorMathSetBrightness(u8 brightness);

#endif /* OPENSNES_COLORMATH_H */
