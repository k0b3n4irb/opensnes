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
 * @brief (Deprecated) No-op, kept for API compatibility.
 *
 * Input is read automatically by the NMI handler in crt0.asm every VBlank.
 * There is no need to call this function. Use padHeld(), padPressed(), and
 * padReleased() directly after WaitForVBlank().
 *
 * @deprecated This function does nothing. Remove calls to it from your code.
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

/*============================================================================
 * Mouse Constants
 *============================================================================*/

/** @defgroup mouse_input Mouse Input
 * @brief SNES Mouse support (1-2 mice on ports 1/2)
 *
 * The SNES mouse provides 2 buttons and relative X/Y displacement.
 * Displacement is read via bit-bang in the NMI handler after mouseInit().
 *
 * ## Usage
 *
 * @code
 * // Detect and initialize mouse on port 1
 * if (mouseInit(0)) {
 *     // Mouse found!
 * }
 *
 * // Main loop
 * while (1) {
 *     WaitForVBlank();
 *     s16 dx = mouseGetX(0);
 *     s16 dy = mouseGetY(0);
 *     cursor_x += dx;
 *     cursor_y += dy;
 *     if (mouseButtonsPressed(0) & MOUSE_BUTTON_LEFT) {
 *         // Left click!
 *     }
 * }
 * @endcode
 * @{
 */

#define MOUSE_BUTTON_LEFT   0x01  /**< Left mouse button */
#define MOUSE_BUTTON_RIGHT  0x02  /**< Right mouse button */

#define MOUSE_SENS_LOW      0     /**< Low sensitivity */
#define MOUSE_SENS_MEDIUM   1     /**< Medium sensitivity */
#define MOUSE_SENS_HIGH     2     /**< High sensitivity */

/**
 * @brief Initialize mouse on given port.
 *
 * Detects if a mouse is connected by checking the auto-joypad device
 * signature. If found, cycles sensitivity to fix the Nintendo power-on bug
 * and enables mouse reading in the NMI handler.
 *
 * @param port Controller port (0 = port 1, 1 = port 2)
 * @return 1 if mouse detected, 0 if not
 */
u8 mouseInit(u8 port);

/**
 * @brief Check if mouse is connected on port.
 *
 * @param port Controller port (0 or 1)
 * @return 1 if connected, 0 if not
 */
u8 mouseIsConnected(u8 port);

/**
 * @brief Get X displacement since last frame.
 *
 * Returns signed displacement: positive = right, negative = left.
 * Raw hardware format is sign-magnitude; this function converts it.
 *
 * @param port Controller port (0 or 1)
 * @return X displacement (-127 to +127)
 */
s16 mouseGetX(u8 port);

/**
 * @brief Get Y displacement since last frame.
 *
 * Returns signed displacement: positive = down, negative = up.
 *
 * @param port Controller port (0 or 1)
 * @return Y displacement (-127 to +127)
 */
s16 mouseGetY(u8 port);

/**
 * @brief Get currently held mouse buttons.
 *
 * @param port Controller port (0 or 1)
 * @return Button mask (MOUSE_BUTTON_LEFT / MOUSE_BUTTON_RIGHT)
 */
u8 mouseButtonsHeld(u8 port);

/**
 * @brief Get newly pressed mouse buttons this frame.
 *
 * @param port Controller port (0 or 1)
 * @return Button mask of buttons pressed this frame (edge detection)
 */
u8 mouseButtonsPressed(u8 port);

/**
 * @brief Set mouse sensitivity.
 *
 * Cycles the mouse sensitivity by strobing the controller port.
 * The sensitivity cycles: LOW -> MEDIUM -> HIGH -> LOW -> ...
 *
 * @param port Controller port (0 or 1)
 * @param sensitivity Target sensitivity (MOUSE_SENS_LOW/MEDIUM/HIGH)
 */
void mouseSetSensitivity(u8 port, u8 sensitivity);

/**
 * @brief Get current mouse sensitivity.
 *
 * @param port Controller port (0 or 1)
 * @return Current sensitivity (MOUSE_SENS_LOW/MEDIUM/HIGH)
 */
u8 mouseGetSensitivity(u8 port);

/** @} */

/*============================================================================
 * Super Scope Constants
 *============================================================================*/

/** @defgroup scope_input Super Scope Input
 * @brief SNES Super Scope support (port 2 only)
 *
 * The Super Scope is a light gun that uses the PPU H/V counter latch
 * to determine aim position. It provides 4 buttons and 2 status flags.
 *
 * ## Usage
 *
 * @code
 * // Detect and initialize Super Scope
 * if (scopeInit()) {
 *     // Super Scope found on port 2!
 * }
 *
 * // Calibration: ask user to fire at screen center
 * // Wait for fire...
 * scopeCalibrate();
 *
 * // Main loop
 * while (1) {
 *     WaitForVBlank();
 *     u16 x = scopeGetX();
 *     u16 y = scopeGetY();
 *     if (scopeButtonsPressed() & SSC_FIRE) {
 *         // Shot fired!
 *     }
 * }
 * @endcode
 * @{
 */

#define SSC_FIRE        0x8000  /**< Fire button (trigger) */
#define SSC_CURSOR      0x4000  /**< Cursor button */
#define SSC_TURBO       0x2000  /**< Turbo switch */
#define SSC_PAUSE       0x1000  /**< Pause button */
#define SSC_OFFSCREEN   0x0200  /**< Off-screen flag */
#define SSC_NOISE       0x0100  /**< Noise flag (no signal) */

/**
 * @brief Detect Super Scope on port 2.
 *
 * Checks the auto-joypad device signature on port 2. If a Super Scope
 * is found, enables reading in the NMI handler and sets default
 * hold/repeat delays (60/20 frames).
 *
 * @return 1 if Super Scope detected, 0 if not
 */
u8 scopeInit(void);

/**
 * @brief Check if Super Scope is connected.
 *
 * @return 1 if connected, 0 if not
 */
u8 scopeIsConnected(void);

/**
 * @brief Get calibration-adjusted H position.
 *
 * @return Adjusted X coordinate (0-255 visible range)
 */
u16 scopeGetX(void);

/**
 * @brief Get calibration-adjusted V position.
 *
 * @return Adjusted Y coordinate (0-223 visible range)
 */
u16 scopeGetY(void);

/**
 * @brief Get raw (uncalibrated) H position from PPU.
 *
 * @return Raw X coordinate from PPU H counter
 */
u16 scopeGetRawX(void);

/**
 * @brief Get raw (uncalibrated) V position from PPU.
 *
 * @return Raw Y coordinate from PPU V counter
 */
u16 scopeGetRawY(void);

/**
 * @brief Get currently held buttons.
 *
 * @return Button mask (SSC_FIRE, SSC_CURSOR, SSC_TURBO, SSC_PAUSE,
 *         SSC_OFFSCREEN, SSC_NOISE)
 */
u16 scopeButtonsDown(void);

/**
 * @brief Get newly pressed buttons this frame.
 *
 * @return Button mask of buttons pressed this frame (edge detection)
 */
u16 scopeButtonsPressed(void);

/**
 * @brief Get buttons held past the hold delay threshold.
 *
 * After holding a button for holddelay frames, it triggers as "held".
 * Then it re-triggers every repdelay frames.
 *
 * @return Button mask of held buttons
 */
u16 scopeButtonsHeld(void);

/**
 * @brief Calibrate aim from a center-screen shot.
 *
 * Call this after the user fires at the center of the screen (128, 112).
 * Computes calibration offsets: centerh = 128 - rawX, centerv = 112 - rawY.
 * The NMI handler applies these offsets to all subsequent readings.
 */
void scopeCalibrate(void);

/**
 * @brief Set hold delay (frames before hold triggers).
 *
 * @param frames Number of frames (default: 60 = 1 second at 60Hz)
 */
void scopeSetHoldDelay(u16 frames);

/**
 * @brief Set repeat delay (frames between repeat fires after hold).
 *
 * @param frames Number of frames (default: 20)
 */
void scopeSetRepeatDelay(u16 frames);

/**
 * @brief Get frames since last detected shot.
 *
 * @return Number of frames since last PPU latch (shot detection)
 */
u16 scopeSinceShot(void);

/** @} */

#endif /* OPENSNES_INPUT_H */
