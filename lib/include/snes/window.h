/**
 * @file window.h
 * @brief SNES Window/Masking System
 *
 * Windows are rectangular regions that can mask (hide or show) portions
 * of background layers and sprites. The SNES has two windows that can be
 * combined using logic operations.
 *
 * ## How Windows Work
 *
 * Each window has left and right boundaries (0-255 pixel positions).
 * Pixels inside the window are treated differently than pixels outside.
 * Windows can be enabled independently for each layer (BG1-4, OBJ).
 *
 * ## Window Logic
 *
 * When both windows affect a layer, they can be combined:
 * - OR: Either window masks the pixel
 * - AND: Both windows must overlap to mask
 * - XOR: Exactly one window masks the pixel
 * - XNOR: Both or neither window masks the pixel
 *
 * ## Usage Example
 *
 * @code
 * // Create a spotlight effect - show only inside window
 * windowSetPos(WINDOW_1, 80, 176);     // Window from x=80 to x=176
 * windowEnable(WINDOW_1, WINDOW_BG1);  // Apply to BG1
 * windowSetMask(WINDOW_BG1, WINDOW_MASK_INSIDE);  // Show inside only
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_WINDOW_H
#define OPENSNES_WINDOW_H

#include <snes/types.h>

/*============================================================================
 * Window Identifiers
 *============================================================================*/

/** @brief Window 1 */
#define WINDOW_1    0

/** @brief Window 2 */
#define WINDOW_2    1

/*============================================================================
 * Layer Masks (for windowEnable/windowDisable)
 *============================================================================*/

/** @brief BG1 layer */
#define WINDOW_BG1      BIT(0)

/** @brief BG2 layer */
#define WINDOW_BG2      BIT(1)

/** @brief BG3 layer */
#define WINDOW_BG3      BIT(2)

/** @brief BG4 layer */
#define WINDOW_BG4      BIT(3)

/** @brief Sprites (OBJ) layer */
#define WINDOW_OBJ      BIT(4)

/** @brief Color math (affects color blending) */
#define WINDOW_MATH     BIT(5)

/** @brief All background layers */
#define WINDOW_ALL_BG   (WINDOW_BG1 | WINDOW_BG2 | WINDOW_BG3 | WINDOW_BG4)

/** @brief All layers including sprites */
#define WINDOW_ALL      (WINDOW_ALL_BG | WINDOW_OBJ)

/*============================================================================
 * Window Logic Operations
 *============================================================================*/

/** @brief OR: mask if inside window 1 OR window 2 */
#define WINDOW_LOGIC_OR     0

/** @brief AND: mask if inside window 1 AND window 2 */
#define WINDOW_LOGIC_AND    1

/** @brief XOR: mask if inside exactly one window */
#define WINDOW_LOGIC_XOR    2

/** @brief XNOR: mask if inside both or neither window */
#define WINDOW_LOGIC_XNOR   3

/*============================================================================
 * Window Masking Modes
 *============================================================================*/

/** @brief Show layer inside window, hide outside */
#define WINDOW_MASK_INSIDE  0

/** @brief Show layer outside window, hide inside */
#define WINDOW_MASK_OUTSIDE 1

/*============================================================================
 * Main Screen / Sub Screen Window Selection
 *============================================================================*/

/** @brief Apply window to main screen */
#define WINDOW_MAIN_SCREEN  0

/** @brief Apply window to sub screen */
#define WINDOW_SUB_SCREEN   1

/*============================================================================
 * Core Window Functions
 *============================================================================*/

/**
 * @brief Initialize window system
 *
 * Resets all window settings to defaults (windows disabled, full screen).
 */
void windowInit(void);

/**
 * @brief Set window boundaries
 *
 * Sets the left and right pixel positions for a window.
 * Valid range is 0-255. If left > right, window is empty.
 *
 * @param window Window number (WINDOW_1 or WINDOW_2)
 * @param left Left boundary (0-255)
 * @param right Right boundary (0-255)
 *
 * @code
 * windowSetPos(WINDOW_1, 64, 192);  // Window from x=64 to x=192
 * @endcode
 */
void windowSetPos(u8 window, u8 left, u8 right);

/**
 * @brief Enable window for specified layers
 *
 * Enables a window to affect the specified layers. The window's masking
 * behavior depends on the invert setting (see windowSetInvert).
 *
 * @param window Window number (WINDOW_1 or WINDOW_2)
 * @param layers Layer mask (WINDOW_BG1, WINDOW_BG2, etc.)
 *
 * @code
 * windowEnable(WINDOW_1, WINDOW_BG1 | WINDOW_OBJ);  // Affect BG1 and sprites
 * @endcode
 */
void windowEnable(u8 window, u8 layers);

/**
 * @brief Disable window for specified layers
 *
 * @param window Window number (WINDOW_1 or WINDOW_2)
 * @param layers Layer mask
 */
void windowDisable(u8 window, u8 layers);

/**
 * @brief Disable all windows
 *
 * Convenience function to turn off all window effects.
 */
void windowDisableAll(void);

/**
 * @brief Set window inversion for layers
 *
 * Controls whether pixels inside the window are shown or hidden.
 *
 * @param window Window number (WINDOW_1 or WINDOW_2)
 * @param layers Layer mask
 * @param invert 0 = mask inside (show inside), 1 = mask outside (show outside)
 */
void windowSetInvert(u8 window, u8 layers, u8 invert);

/**
 * @brief Set logic operation for combining windows
 *
 * When both windows affect a layer, this sets how they combine.
 *
 * @param layer Single layer (WINDOW_BG1, WINDOW_BG2, etc.)
 * @param logic Logic operation (WINDOW_LOGIC_*)
 */
void windowSetLogic(u8 layer, u8 logic);

/**
 * @brief Set main screen window masking
 *
 * Controls which layers use window masking on the main screen.
 *
 * @param layers Layer mask for main screen windowing
 */
void windowSetMainMask(u8 layers);

/**
 * @brief Set sub screen window masking
 *
 * Controls which layers use window masking on the sub screen.
 *
 * @param layers Layer mask for sub screen windowing
 */
void windowSetSubMask(u8 layers);

/*============================================================================
 * Window Effect Helpers
 *============================================================================*/

/**
 * @brief Create a centered rectangular window
 *
 * Convenience function to set up a window centered on screen.
 *
 * @param window Window number
 * @param width Width in pixels (1-256)
 */
void windowCentered(u8 window, u8 width);

/**
 * @brief Create a vertical split at the specified X position
 *
 * Sets up Window 1 for left side, Window 2 for right side.
 * Useful for split-screen or scene transitions.
 *
 * @param splitX X position of the split (0-255)
 */
void windowSplit(u8 splitX);

#endif /* OPENSNES_WINDOW_H */
