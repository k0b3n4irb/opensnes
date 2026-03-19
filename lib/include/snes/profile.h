/**
 * @file profile.h
 * @brief Performance profiling tools for SNES development
 *
 * Provides scanline-based timing, visual color-bar profiling,
 * and frame/lag counters.
 *
 * ## Quick Start
 *
 * @code
 * #include <snes.h>
 * #include <snes/profile.h>
 *
 * int main(void) {
 *     consoleInit();
 *     profileInit();  // enable color math for visual bars
 *     // ... setup ...
 *     setScreenOn();
 *
 *     while (1) {
 *         WaitForVBlank();
 *
 *         profileColorStart(PROFILE_RED);
 *         // ... game logic ...
 *         profileColorEnd();
 *     }
 * }
 * @endcode
 *
 * ## Build
 *
 * Add `profile` to LIB_MODULES:
 * @code
 * LIB_MODULES := console sprite dma profile
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_PROFILE_H
#define OPENSNES_PROFILE_H

#include <snes/types.h>

/*============================================================================
 * Color Constants for Visual Profiling
 *
 * Each constant is an index into the internal color table.
 *============================================================================*/

#define PROFILE_RED       0
#define PROFILE_GREEN     1
#define PROFILE_BLUE      2
#define PROFILE_YELLOW    3
#define PROFILE_CYAN      4
#define PROFILE_MAGENTA   5
#define PROFILE_WHITE     6

/*============================================================================
 * Initialization
 *============================================================================*/

/**
 * @brief Initialize the profiler
 *
 * Enables color math so that profileColorStart/End produce visible
 * color bars on the backdrop. Call once after consoleInit(), before
 * setScreenOn().
 *
 * Sets CGADSUB ($2131) to add fixed color to backdrop.
 */
void profileInit(void);

/*============================================================================
 * Visual Color-Bar Profiling
 *
 * Sets the fixed color (COLDATA $2132) to show colored bands on screen.
 * The band width shows how much CPU time the section uses.
 * Multiple sections can use different colors to identify bottlenecks.
 *============================================================================*/

/**
 * @brief Start a color-bar profiling section
 *
 * @param color One of the PROFILE_* color constants (0-6)
 */
void profileColorStart(u16 color);

/**
 * @brief End a color-bar profiling section
 *
 * Clears the fixed color back to black.
 */
void profileColorEnd(void);

/*============================================================================
 * Scanline-Based Timing
 *============================================================================*/

/**
 * @brief Get the current scanline number (0-261 NTSC, 0-311 PAL)
 */
u16 profileGetScanline(void);

/**
 * @brief Start a scanline-based timing measurement
 */
void profileScanlineStart(void);

/**
 * @brief End a scanline-based timing measurement
 *
 * @return Number of scanlines elapsed since profileScanlineStart().
 *         Multiply by 1364 for approximate master cycles (NTSC).
 */
u16 profileScanlineEnd(void);

/*============================================================================
 * Frame Counters
 *============================================================================*/

/**
 * @brief Get total frame count since boot (wraps at 65535)
 */
u16 profileGetFrameCount(void);

/**
 * @brief Get lag frame count since boot
 *
 * A lag frame occurs when main code doesn't finish before the next NMI.
 */
u16 profileGetLagFrames(void);

#endif /* OPENSNES_PROFILE_H */
