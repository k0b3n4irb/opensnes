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
 * // 50% transparent BG2 over BG1
 * REG_TM = TM_BG1 | TM_BG2;   // Both on main screen
 * REG_TS = TM_BG2;            // BG2 also on sub screen
 *
 * colorMathEnable(COLORMATH_BG2);  // Apply math to BG2
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
 * colorMathEnable(COLORMATH_ALL);   // Apply to all layers
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_COLORMATH_H
#define OPENSNES_COLORMATH_H

#include <snes/types.h>

/*============================================================================
 * Layer Masks (for colorMathEnable)
 *============================================================================*/

/** @brief Apply color math to BG1 */
#define COLORMATH_BG1       BIT(0)

/** @brief Apply color math to BG2 */
#define COLORMATH_BG2       BIT(1)

/** @brief Apply color math to BG3 */
#define COLORMATH_BG3       BIT(2)

/** @brief Apply color math to BG4 */
#define COLORMATH_BG4       BIT(3)

/** @brief Apply color math to sprites (OBJ) */
#define COLORMATH_OBJ       BIT(4)

/** @brief Apply color math to backdrop (color 0) */
#define COLORMATH_BACKDROP  BIT(5)

/** @brief Apply color math to all layers */
#define COLORMATH_ALL       0x3F

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
 * @brief Enable color math for specified layers
 *
 * Enables color math blending for the given layers.
 *
 * @param layers Layer mask (COLORMATH_BG1, COLORMATH_BG2, etc.)
 */
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
 * When enabled, the color math result is divided by 2.
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
