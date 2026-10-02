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
 * // Set scale (0x0200 = 1:1, 0x0100 = magnified twice; see mode7SetScale)
 * mode7SetScale(0x0200, 0x0200);
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
 * Use dmaCopyVramMode7() (two DMAs: tilemap to the low bytes, tiles to the
 * high bytes, from any bank) or set up DMA manually with interleaved data.
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
 * Writes the identity matrix (A = D = $0100, 1:1), the center point
 * (128,128), M7HOFS = 0 and M7VOFS = $17F (texel row $180 on the first
 * line: the view starts in the middle of the 1024-texel plane, not at its
 * top — call mode7SetScroll(0, 0) to see row 0 first). It also sets the
 * scale mode7SetAngle() uses to 0x0100, which that function turns into a
 * matrix of $7F (magnified twice, see mode7SetScale): call
 * mode7SetScale(0x0200, 0x0200) before the first mode7SetAngle() to keep
 * 1:1. Call this before using other Mode 7 functions.
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
 * Sets the X and Y scale for Mode 7 transformation. The matrix
 * mode7SetAngle() writes is **scale / 2** (it multiplies by a cosine of
 * amplitude 127 and keeps the high byte), so:
 * - 0x0200 = 1:1 (one texel per pixel; matrix A = D = $00FE)
 * - 0x0100 = magnified twice (the plane looks larger)
 * - 0x0400 = shrunk twice (the plane looks smaller)
 *
 * (Until 2026-10-02 this said 0x0100 = 1.0, which is not what the code
 * does; whether to change the code instead is an open API decision.)
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
 * @param y Vertical scroll (13-bit signed); written as y - 1 to M7VOFS, the
 *          same scanline-0 convention as bgSetScroll()
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
 * @param scalePercent Scale as a percentage of mode7SetScale's units
 *        (percent x 2.5), so 200 is about 1:1, 100 magnifies twice, 400
 *        shrinks twice (see mode7SetScale)
 *
 * @code
 * mode7Transform(45, 200);   // 45 degree rotation, about 1:1
 * mode7Transform(0, 100);    // no rotation, magnified twice
 * mode7Transform(90, 400);   // 90 degrees, shrunk twice
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
 *   Bit 1: Flip vertically
 *   Bit 0: Flip horizontally
 *   Bits 7-6: Out of bounds behavior (0=wrap, 0x80=transparent, 0xC0=tile 0)
 */
void mode7SetSettings(u8 settings);

/**
 * @brief Enable/disable Mode 7 EXTBG (SETINI bit 6): a second, split layer
 * @param on 1 to enable, 0 to disable
 *
 * With EXTBG on, BG2 shows the same Mode 7 plane as BG1 (same tilemap, same
 * pixels, same transform), but reads each pixel's bit 7 as a priority bit
 * and bits 0-6 as its colour (128 colours). Front to back, Mode 7 EXTBG
 * draws: sprites of priority 3, sprites 2, BG2 pixels with bit 7 set,
 * sprites 1, BG1, sprites 0, BG2 pixels with bit 7 clear (anomie's register
 * doc, Mode 7; snesdev-wiki, Backgrounds). So a sprite of priority 1 passes
 * in front of the bit-7-clear pixels and behind the bit-7-set ones: put
 * BG2, not BG1, on the screen (BG1 would cover the low pixels), e.g.
 * `setMainScreen(LAYER_BG2 | LAYER_OBJ)`. BG1 still reads all 8 bits, so a
 * pixel 0x83 is colour 3 on BG2 and colour 131 on BG1. Direct colour does
 * not apply to BG2.
 *
 * SETINI is write-only: this goes through the same software copy as
 * videoSetInterlace(), videoSetOverscan() and videoSetPseudoHires(), so it
 * does not clear their bits.
 *
 * @code
 * setMode(BG_MODE7, 0);
 * mode7SetExtBg(1);
 * setMainScreen(LAYER_BG2 | LAYER_OBJ);
 * @endcode
 */
void mode7SetExtBg(u8 on);

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
