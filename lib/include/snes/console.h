/**
 * @file console.h
 * @brief SNES Console Initialization and Core Functions
 *
 * Provides functions for initializing SNES hardware and core functionality
 * like VBlank synchronization.
 *
 * ## Basic Usage
 *
 * @code
 * #include <snes.h>
 *
 * int main(void) {
 *     // Initialize hardware
 *     consoleInit();
 *
 *     // Set up your game...
 *
 *     // Main game loop
 *     while (1) {
 *         // Wait for VBlank (required for stable graphics)
 *         WaitForVBlank();
 *
 *         // Update game logic
 *         // Handle input
 *         // Update sprites/backgrounds
 *     }
 * }
 * @endcode
 *
 * ## Attribution
 *
 * Originally from: PVSnesLib (https://github.com/alekmaul/pvsneslib)
 * Author: Alekmaul
 * License: zlib (compatible with MIT)
 * Modifications:
 *   - Renamed functions for clarity
 *   - Added comprehensive documentation
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_CONSOLE_H
#define OPENSNES_CONSOLE_H

#include <snes/types.h>

/*============================================================================
 * Initialization
 *============================================================================*/

/**
 * @brief Initialize SNES hardware
 *
 * Must be called at the start of your program. Performs:
 * - PPU initialization (screen blank, registers cleared)
 * - CPU register setup
 * - Work RAM clearing
 * - Default palette loading
 * - VBlank interrupt setup
 *
 * After calling, screen is blanked (black). Call setScreenOn() to enable
 * display after you've set up your graphics.
 *
 * @code
 * int main(void) {
 *     consoleInit();
 *
 *     // Load graphics...
 *     // Set up sprites...
 *
 *     setScreenOn();  // Now show the screen
 *
 *     while (1) {
 *         WaitForVBlank();
 *     }
 * }
 * @endcode
 */
void consoleInit(void);

/**
 * @brief Initialize console with options
 *
 * Advanced initialization with configuration options.
 *
 * @param options Initialization flags (reserved for future use)
 *
 * @note For most games, use consoleInit() instead.
 */
void consoleInitEx(u16 options);

/*============================================================================
 * Screen Control
 *============================================================================*/

/**
 * @brief Enable screen display
 *
 * Turns on the display after initialization or a screen blank.
 * Sets full brightness (15).
 *
 * @code
 * consoleInit();
 * // ... load graphics ...
 * setScreenOn();  // Display is now visible
 * @endcode
 */
void setScreenOn(void);

/**
 * @brief Disable screen display (blank)
 *
 * Turns off the display. Use during major VRAM updates that
 * can't complete during VBlank.
 *
 * @code
 * setScreenOff();
 * // ... massive VRAM update ...
 * setScreenOn();
 * @endcode
 */
void setScreenOff(void);

/**
 * @brief Set screen brightness
 *
 * @param brightness Brightness level (0-15, 0=black, 15=full)
 *
 * @code
 * // Fade in effect
 * for (u8 i = 0; i <= 15; i++) {
 *     setBrightness(i);
 *     WaitForVBlank();
 * }
 * @endcode
 */
void setBrightness(u8 brightness);

/**
 * @brief Get current brightness
 *
 * @return Current brightness level (0-15)
 */
u8 getBrightness(void);

/*============================================================================
 * VBlank Synchronization
 *============================================================================*/

/**
 * @brief Wait for next VBlank period
 *
 * Blocks until the PPU enters Vertical Blank. This is essential for:
 * - Stable graphics (prevents tearing)
 * - Safe VRAM/OAM/CGRAM updates
 * - Consistent game timing
 *
 * Call once per frame in your main loop.
 *
 * @code
 * while (1) {
 *     // Update game logic (can happen during active display)
 *     updatePlayer();
 *     checkCollisions();
 *
 *     // Wait for VBlank
 *     WaitForVBlank();
 *
 *     // Now safe to update graphics
 *     updateOAM();
 *     scrollBackground();
 * }
 * @endcode
 *
 * @note VBlank occurs ~60 times/second (NTSC) or ~50 times/second (PAL)
 */
void WaitForVBlank(void);

/**
 * @brief Check if currently in VBlank
 *
 * @return TRUE if in VBlank, FALSE if in active display
 */
u8 isInVBlank(void);

/*============================================================================
 * Frame Counter
 *============================================================================*/

/**
 * @brief Get frame counter
 *
 * Returns the number of VBlanks since initialization.
 * Wraps at 65535.
 *
 * @return Frame count
 *
 * @code
 * // Simple animation timing
 * u8 anim_frame = (getFrameCount() / 8) % 4;
 * @endcode
 */
u16 getFrameCount(void);

/**
 * @brief Reset frame counter
 *
 * Sets frame counter to 0. Useful for timing game events.
 */
void resetFrameCount(void);

/*============================================================================
 * System Information
 *============================================================================*/

/**
 * @brief Check if PAL system
 *
 * @return TRUE if PAL (50Hz), FALSE if NTSC (60Hz)
 *
 * @code
 * if (isPAL()) {
 *     // Adjust game speed for 50Hz
 * }
 * @endcode
 */
u8 isPAL(void);

/**
 * @brief Get system region
 *
 * @return 0 = NTSC, 1 = PAL
 */
u8 getRegion(void);

/*============================================================================
 * Random Number Generation
 *============================================================================*/

/**
 * @brief Get random 16-bit number
 *
 * Returns a pseudo-random number using a linear feedback shift register.
 *
 * @return Random value 0-65535
 *
 * @code
 * u16 enemy_x = rand() % 256;
 * @endcode
 */
u16 rand(void);

/**
 * @brief Seed random number generator
 *
 * @param seed Initial seed value
 *
 * @code
 * // Seed from player input timing for variety
 * srand(getFrameCount());
 * @endcode
 */
void srand(u16 seed);

#endif /* OPENSNES_CONSOLE_H */
