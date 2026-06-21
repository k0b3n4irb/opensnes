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
#include <snes/registers.h>  /* REG_INIDISP for inline setScreenOn/Off bodies */

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
 * Inlined for zero-call-overhead access (saves ~28 cycles per call).
 * Shares the same `force_blanked` and `current_brightness` shadows as
 * setScreenOff(); both are declared extern below.
 *
 * @code
 * consoleInit();
 * // ... load graphics ...
 * setScreenOn();  // Display is now visible
 * @endcode
 */
inline void setScreenOn(void) {
    extern u8 force_blanked;
    extern u8 current_brightness;
    force_blanked = 0;
    REG_INIDISP = current_brightness & 0x0F;
}

/**
 * @brief Disable screen display (blank)
 *
 * Turns off the display. Use during major VRAM updates that
 * can't complete during VBlank.
 *
 * Inlined for zero-call-overhead access (saves ~28 cycles per call).
 * The standalone definition in console.c is still emitted (via the
 * `extern inline` declaration there) for ABI compatibility — direct
 * call sites collapse to two stores.
 *
 * @code
 * setScreenOff();
 * // ... massive VRAM update ...
 * setScreenOn();
 * @endcode
 */
extern u8 force_blanked;
inline void setScreenOff(void) {
    force_blanked = 1;
    REG_INIDISP = INIDISP_FORCE_BLANK;
}

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
 * @brief Fade the screen from full brightness (15) down to black (0).
 *
 * Steps INIDISP brightness from 15 → 0, waiting @p speed VBlank frames
 * between each step. Final state: `setBrightness(0)` (visually black,
 * screen still rendering). Combine with `setScreenOff()` afterwards if
 * you want to also enter force-blank for safe VRAM updates.
 *
 * At 60 fps (NTSC):
 * - `speed=1` → 16 frames (~0.27 s, fast/snappy)
 * - `speed=3` → 48 frames (~0.80 s, cinematic)
 * - `speed=6` → 96 frames (~1.60 s, dramatic)
 *
 * @param speed VBlank frames to wait between each brightness step.
 *              `speed=0` falls through with no inter-step delay (visible
 *              flash, ~16 frames if every step still hits VBlank via
 *              loop overhead — not typically useful).
 *
 * @code
 * fadeOut(3);              // ~0.8 s cinematic fade
 * setScreenOff();          // force-blank for VRAM swap
 * dmaCopyVram(new_tiles, 0, ...);
 * setScreenOn();
 * fadeIn(3);
 * @endcode
 */
void fadeOut(u8 speed);

/**
 * @brief Fade the screen from black (0) up to full brightness (15).
 *
 * Symmetric counterpart to @ref fadeOut. Steps INIDISP brightness from
 * 0 → 15, waiting @p speed VBlank frames between each step.
 *
 * @param speed VBlank frames to wait between steps (see @ref fadeOut for
 *              the wall-clock cost at common values).
 */
void fadeIn(u8 speed);

/**
 * @brief Get current brightness
 *
 * Inlined for zero-call-overhead access. The standalone definition is
 * still available (force-emitted in console.c) for fn-pointer callers.
 *
 * @return Current brightness level (0-15)
 */
extern u8 current_brightness;
inline u8 getBrightness(void) {
    return current_brightness;
}

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
 * Inlined for zero-call-overhead access (just a 16-bit load).
 *
 * @return Frame count
 *
 * @code
 * // Simple animation timing
 * u8 anim_frame = (getFrameCount() / 8) % 4;
 * @endcode
 */
inline u16 getFrameCount(void) {
    extern volatile u16 frame_count;
    return frame_count;
}

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
