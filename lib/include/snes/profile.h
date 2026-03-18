/**
 * @file profile.h
 * @brief Performance profiling tools for SNES development
 *
 * Provides scanline-based timing, VBlank usage measurement, lag frame
 * counting, and visual color-bar profiling for the SNES.
 *
 * ## Quick Start
 *
 * @code
 * #include <snes.h>
 * #include <snes/profile.h>
 *
 * while (1) {
 *     WaitForVBlank();
 *
 *     profileColorStart(PROFILE_RED);
 *     // ... game logic ...
 *     profileColorEnd();
 *
 *     profileColorStart(PROFILE_GREEN);
 *     // ... rendering ...
 *     profileColorEnd();
 * }
 * @endcode
 *
 * The colored bars visible at the bottom of the screen show how much
 * CPU time each section uses. If the bars reach VBlank, you're lagging.
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
 * COLDATA format: gggrrrrr (green high 3 bits, red 5 bits)
 * These write to $2132 with intensity + channel select bits.
 *============================================================================*/

/** Red color bar */
#define PROFILE_RED     0x21
/** Green color bar */
#define PROFILE_GREEN   0x42
/** Blue color bar */
#define PROFILE_BLUE    0x80
/** Yellow color bar */
#define PROFILE_YELLOW  0x63
/** Cyan color bar */
#define PROFILE_CYAN    0xC2
/** Magenta color bar */
#define PROFILE_MAGENTA 0xA1
/** White color bar */
#define PROFILE_WHITE   0xE3

/*============================================================================
 * Visual Color-Bar Profiling
 *
 * Sets the fixed color (backdrop) to a bright color before a code section,
 * then clears it after. The resulting color band on screen shows execution
 * time visually — the classic SNES developer technique.
 *
 * Multiple sections can use different colors to identify bottlenecks:
 *   profileColorStart(PROFILE_RED);    // AI update
 *   profileColorEnd();
 *   profileColorStart(PROFILE_GREEN);  // physics
 *   profileColorEnd();
 *============================================================================*/

/**
 * @brief Start a color-bar profiling section
 *
 * Sets the SNES fixed color (COLDATA $2132) to the given color.
 * The colored band is visible in the overscan/border area and during
 * active display if color math is enabled with fixed color source.
 *
 * @param color One of the PROFILE_* color constants
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
 *
 * Reads the PPU vertical counter (OPVCT $213D) to measure elapsed
 * scanlines. Each scanline ≈ 1364 master cycles (NTSC).
 *
 * Accuracy: ±1 scanline. For sub-scanline precision, use Mesen2's
 * built-in profiler instead.
 *============================================================================*/

/**
 * @brief Get the current scanline number
 *
 * Latches and reads the PPU vertical position counter (OPVCT).
 * Returns 0-261 (NTSC) or 0-311 (PAL).
 *
 * @return Current scanline (0 = top of frame)
 */
u16 profileGetScanline(void);

/**
 * @brief Start a scanline-based timing measurement
 *
 * Records the current scanline. Call profileScanlineEnd() after the
 * code section to get the elapsed scanline count.
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
 *
 * These read system variables maintained by the NMI handler in crt0.asm.
 *============================================================================*/

/**
 * @brief Get the total frame count
 *
 * Incremented by the NMI handler every VBlank. Wraps at 65535.
 *
 * @return Frame count since boot
 */
u16 profileGetFrameCount(void);

/**
 * @brief Get the lag frame count
 *
 * A lag frame occurs when main-thread code doesn't call WaitForVBlank()
 * before the next NMI fires. The NMI handler skips DMA work on lag
 * frames and increments this counter.
 *
 * @return Number of lag frames since boot
 */
u16 profileGetLagFrames(void);

#endif /* OPENSNES_PROFILE_H */
