/**
 * @file input.h
 * @brief SNES Controller Input
 *
 * Functions for reading controller input.
 *
 * ## Button Constants
 *
 * Use these constants with padPressed(), padHeld(), etc:
 *
 * | Constant | Button |
 * |----------|--------|
 * | KEY_A | A button |
 * | KEY_B | B button |
 * | KEY_X | X button |
 * | KEY_Y | Y button |
 * | KEY_L | L shoulder |
 * | KEY_R | R shoulder |
 * | KEY_START | Start |
 * | KEY_SELECT | Select |
 * | KEY_UP | D-pad up |
 * | KEY_DOWN | D-pad down |
 * | KEY_LEFT | D-pad left |
 * | KEY_RIGHT | D-pad right |
 *
 * ## Usage
 *
 * @code
 * // Check for button press (just pressed this frame)
 * if (padPressed(0) & KEY_A) {
 *     playerJump();
 * }
 *
 * // Check for button held (continuously pressed)
 * if (padHeld(0) & KEY_RIGHT) {
 *     playerMoveRight();
 * }
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_INPUT_H
#define OPENSNES_INPUT_H

#include <snes/types.h>

/*============================================================================
 * Button Constants
 *============================================================================*/

/** @defgroup input_buttons Button Constants
 * @{
 */

/*
 * SNES Joypad Register Layout (16-bit read as JOY1L | JOY1H << 8):
 *
 *   $4219 (JOY1H) → bits 15-8:  B, Y, Select, Start, Up, Down, Left, Right
 *   $4218 (JOY1L) → bits 7-0:   A, X, L, R, (signature bits)
 *
 * Bit layout (accent accent = accent accent accent accent accent):
 *   15   14   13   12   11   10   9    8    7    6    5    4    3-0
 *   B    Y    Sel  Sta  Up   Dn   Lt   Rt   A    X    L    R    ID
 *
 * Verified against fullsnes documentation and real hardware.
 */

/* High byte ($4219 → result bits 15-8) */
#define KEY_B       BIT(15)  /**< B button */
#define KEY_Y       BIT(14)  /**< Y button */
#define KEY_SELECT  BIT(13)  /**< Select button */
#define KEY_START   BIT(12)  /**< Start button */
#define KEY_UP      BIT(11)  /**< D-pad up */
#define KEY_DOWN    BIT(10)  /**< D-pad down */
#define KEY_LEFT    BIT(9)   /**< D-pad left */
#define KEY_RIGHT   BIT(8)   /**< D-pad right */

/* Low byte ($4218 → result bits 7-0) */
#define KEY_A       BIT(7)   /**< A button */
#define KEY_X       BIT(6)   /**< X button */
#define KEY_L       BIT(5)   /**< L shoulder */
#define KEY_R       BIT(4)   /**< R shoulder */

/** @brief All D-pad directions */
#define KEY_DPAD    (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)

/** @brief All face buttons */
#define KEY_FACE    (KEY_A | KEY_B | KEY_X | KEY_Y)

/** @} */

/*============================================================================
 * Functions
 *============================================================================*/

/**
 * @brief Update input state
 *
 * Call once per frame, typically after WaitForVBlank().
 * This reads the hardware registers and updates internal state.
 *
 * @note If using consoleInit() with auto-joypad read enabled,
 *       this is called automatically during WaitForVBlank().
 */
void padUpdate(void);

/**
 * @brief Get buttons pressed this frame
 *
 * Returns buttons that were just pressed (not held from previous frame).
 *
 * @param pad Controller number (0-3)
 * @return Button mask of newly pressed buttons
 *
 * @code
 * if (padPressed(0) & KEY_A) {
 *     // A was just pressed
 * }
 * @endcode
 */
u16 padPressed(u8 pad);

/**
 * @brief Get buttons currently held
 *
 * Returns all buttons currently being pressed.
 *
 * @param pad Controller number (0-3)
 * @return Button mask of held buttons
 *
 * @code
 * if (padHeld(0) & KEY_RIGHT) {
 *     player_x++;
 * }
 * @endcode
 */
u16 padHeld(u8 pad);

/**
 * @brief Get buttons released this frame
 *
 * Returns buttons that were just released (held last frame, not now).
 *
 * @param pad Controller number (0-3)
 * @return Button mask of released buttons
 */
u16 padReleased(u8 pad);

/**
 * @brief Get raw button state
 *
 * Returns the raw hardware state without edge detection.
 *
 * @param pad Controller number (0-3)
 * @return Raw button state
 */
u16 padRaw(u8 pad);

/**
 * @brief Check if controller is connected
 *
 * @param pad Controller number (0-3)
 * @return TRUE if connected, FALSE otherwise
 */
u8 padIsConnected(u8 pad);

#endif /* OPENSNES_INPUT_H */
