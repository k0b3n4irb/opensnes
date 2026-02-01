/**
 * @file mode7.h
 * @brief SNES Mode 7 Support
 *
 * Functions for Mode 7 rotation and scaling effects.
 * Mode 7 is the SNES's hardware rotation/scaling mode, famously used
 * in F-Zero, Super Mario Kart, and Pilotwings.
 *
 * ## Usage
 *
 * @code
 * #include <snes.h>
 *
 * // Initialize Mode 7
 * mode7Init();
 *
 * // Set scale (0x0100 = 1.0, 0x0080 = 0.5, 0x0200 = 2.0)
 * mode7SetScale(0x0100, 0x0100);
 *
 * // Set rotation angle (0-255, where 256 = 360 degrees)
 * mode7SetAngle(angle);
 *
 * // In main loop
 * while (1) {
 *     WaitForVBlank();
 *     angle++;
 *     mode7SetAngle(angle);
 * }
 * @endcode
 *
 * ## Mode 7 VRAM Layout
 *
 * Mode 7 uses a special interleaved VRAM format:
 * - Tilemap in low bytes of VRAM words 0x0000-0x3FFF
 * - Tile data in high bytes of VRAM words 0x0000-0x3FFF
 *
 * Use mode7LoadGraphics() or set up DMA manually with interleaved data.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_MODE7_H
#define OPENSNES_MODE7_H

#include <snes/types.h>

/**
 * @brief Initialize Mode 7
 *
 * Sets up default scale (1.0), identity matrix, center point (128,128),
 * and scroll position. Call this before using other Mode 7 functions.
 *
 * @note This does NOT set BGMODE to Mode 7. You must do that separately:
 * @code
 * REG_BGMODE = BGMODE_MODE7;
 * REG_M7SEL = 0x00;  // No flip, wrap around
 * REG_TM = TM_BG1;   // Enable BG1
 * @endcode
 */
void mode7Init(void);

/**
 * @brief Set Mode 7 scale factors
 *
 * Sets the X and Y scale for Mode 7 transformation.
 * Scale is in 8.8 fixed point format:
 * - 0x0100 = 1.0 (normal size)
 * - 0x0080 = 0.5 (zoomed in / larger)
 * - 0x0200 = 2.0 (zoomed out / smaller)
 *
 * @param scale_x Horizontal scale (8.8 fixed point)
 * @param scale_y Vertical scale (8.8 fixed point)
 *
 * @note Call mode7SetAngle() after changing scale to update the matrix.
 */
void mode7SetScale(u16 scale_x, u16 scale_y);

/**
 * @brief Set Mode 7 rotation angle
 *
 * Sets the rotation angle and recalculates the transformation matrix.
 * The angle is 0-255 where 256 would equal 360 degrees (wraps at 256).
 *
 * @param angle Rotation angle (0-255)
 *
 * This function:
 * 1. Looks up sin/cos from table
 * 2. Multiplies by current scale using hardware multiplier
 * 3. Writes the 4 matrix values (M7A, M7B, M7C, M7D) to PPU
 */
void mode7SetAngle(u8 angle);

/**
 * @brief Set Mode 7 center point
 *
 * Sets the center of rotation/scaling. Default is (128, 128).
 *
 * @param x Center X coordinate (13-bit signed, -4096 to 4095)
 * @param y Center Y coordinate (13-bit signed, -4096 to 4095)
 */
void mode7SetCenter(s16 x, s16 y);

/**
 * @brief Set Mode 7 scroll position
 *
 * Sets the scroll offset for the Mode 7 plane.
 *
 * @param x Horizontal scroll (13-bit signed)
 * @param y Vertical scroll (13-bit signed)
 */
void mode7SetScroll(s16 x, s16 y);

/*============================================================================
 * Higher-Level Mode 7 Functions
 *============================================================================*/

/**
 * @brief Set rotation in degrees (0-359)
 *
 * Convenience function that converts degrees to the internal 0-255 format.
 *
 * @param degrees Rotation angle in degrees (0-359)
 *
 * @code
 * mode7Rotate(45);   // Rotate 45 degrees
 * mode7Rotate(180);  // Rotate 180 degrees
 * @endcode
 */
void mode7Rotate(u16 degrees);

/**
 * @brief Set rotation and scale together
 *
 * Combined transformation with rotation in degrees and percentage-based scaling.
 *
 * @param degrees Rotation angle in degrees (0-359)
 * @param scalePercent Scale as percentage (100 = normal, 50 = half, 200 = double)
 *
 * @code
 * mode7Transform(45, 100);   // 45 degree rotation, normal scale
 * mode7Transform(0, 50);     // No rotation, 2x zoom in
 * mode7Transform(90, 200);   // 90 degrees, 0.5x zoom out
 * @endcode
 */
void mode7Transform(u16 degrees, u16 scalePercent);

/**
 * @brief Set pivot point (screen coordinates)
 *
 * Sets the rotation center using screen coordinates (0-255).
 * The pivot point is where the Mode 7 plane appears to rotate around.
 *
 * @param x Screen X coordinate (0-255)
 * @param y Screen Y coordinate (0-223)
 *
 * @code
 * mode7SetPivot(128, 112);  // Center of screen
 * mode7SetPivot(0, 0);      // Top-left corner
 * @endcode
 */
void mode7SetPivot(u8 x, u8 y);

/**
 * @brief Set Mode 7 matrix directly
 *
 * For advanced users who need direct control over the transformation matrix.
 * Bypasses the angle/scale system.
 *
 * The matrix maps screen (x,y) to Mode 7 plane (X,Y):
 * X = A*(x-cx) + B*(y-cy) + sx + cx
 * Y = C*(x-cx) + D*(y-cy) + sy + cy
 *
 * @param a Matrix A (1.7.8 fixed point)
 * @param b Matrix B (1.7.8 fixed point)
 * @param c Matrix C (1.7.8 fixed point)
 * @param d Matrix D (1.7.8 fixed point)
 */
void mode7SetMatrix(s16 a, s16 b, s16 c, s16 d);

/**
 * @brief Set Mode 7 settings register
 *
 * Controls flipping and out-of-bounds behavior.
 *
 * @param settings M7SEL value:
 *   Bit 7: Flip vertically
 *   Bit 6: Flip horizontally
 *   Bits 7-6: Out of bounds behavior (0=wrap, 0x80=transparent, 0xC0=tile 0)
 */
void mode7SetSettings(u8 settings);

/*============================================================================
 * Mode 7 Settings Constants
 *============================================================================*/

/** @brief Wrap around when out of bounds (default) */
#define MODE7_WRAP          0x00

/** @brief Show transparent when out of bounds */
#define MODE7_TRANSPARENT   0x80

/** @brief Show tile 0 when out of bounds */
#define MODE7_TILE0         0xC0

/** @brief Flip Mode 7 plane horizontally */
#define MODE7_FLIP_H        0x01

/** @brief Flip Mode 7 plane vertically */
#define MODE7_FLIP_V        0x02

#endif /* OPENSNES_MODE7_H */
