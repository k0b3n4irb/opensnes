/**
 * @file main.c
 * @brief Mode 7 rotation and scaling demo
 * @ingroup examples
 *
 * Demonstrates SNES Mode 7, the hardware affine transformation mode that
 * applies a 2x2 matrix (rotation + scaling) to a single 128x128 tile
 * background layer. Mode 7 uses a unique interleaved VRAM format where
 * tilemap bytes occupy the low bytes and 8bpp tile pixel data occupies
 * the high bytes of each VRAM word. The transformation matrix is
 * controlled via registers M7A-M7D, with the library's mode7SetAngle()
 * and mode7SetScale() computing the sin/cos matrix entries from an
 * 8-bit angle and 8.8 fixed-point scale factors.
 *
 * @par SNES Concepts
 * - Mode 7 affine transformation (rotation + scaling via M7A-M7D)
 * - Interleaved VRAM format (tilemap in low bytes, pixels in high bytes)
 * - 8.8 fixed-point scale representation
 * - 8-bit angle for hardware sin/cos lookup
 *
 * @par What to Observe
 * - Press A to rotate the background clockwise
 * - Press B to rotate counter-clockwise
 * - Press UP to zoom out (background shrinks, more visible)
 * - Press DOWN to zoom in (background magnifies)
 *
 * @par Modules Used
 * console, dma, background, sprite, input, mode7
 *
 * @see mode7.h, background.h, input.h
 */

#include <snes.h>

/**
 * @brief Assembly helper to load Mode 7 tile data, tilemap, and palette.
 *
 * Mode 7 uses a unique interleaved VRAM format: each VRAM word has the tilemap
 * byte in the low byte and the 8bpp pixel data in the high byte. This requires
 * special DMA sequencing that cannot be done with the standard dmaCopyVram()
 * function. The assembly routine handles the interleaved write and also loads
 * the 256-color palette to CGRAM.
 *
 * Must be called during forced blank (screen off) since it writes to VRAM.
 */
extern void asm_loadMode7Data(void);

/**
 * @brief Entry point -- interactive Mode 7 rotation and scaling demo.
 *
 * Initializes the Mode 7 plane with an image, then allows the user to rotate
 * (A/B buttons) and zoom (D-pad UP/DOWN) in real time. The transformation
 * matrix is updated each frame via mode7SetAngle() and mode7SetScale(), which
 * compute sin/cos values and write to the M7A-M7D hardware registers.
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    u16 pad0;                          /**< Current joypad button state */
    u8 angle = 0;                      /**< Rotation angle (0-255 maps to 0-360 degrees) */
    u16 zscale = 0x0200;               /**< Zoom level in 8.8 fixed-point (2.0 = default, higher = more zoomed out) */

    consoleInit();

    /* Force blank for VRAM loading -- VRAM writes are silently ignored
     * during active display on the SNES PPU. */
    setScreenOff();

    /* Load Mode 7 tile data, tilemap, and palette via assembly helper.
     * This must happen during forced blank because it performs bulk VRAM DMA. */
    asm_loadMode7Data();

    /* Set Mode 7 and initialize the affine transformation matrix.
     * mode7Init() zeros the scroll center and offset registers (M7HOFS/M7VOFS,
     * M7X/M7Y). mode7SetScale() sets the initial zoom to 2.0x (showing the
     * 128x128 tile plane at half magnification), and mode7SetAngle() writes
     * the identity rotation (angle 0 = no rotation). */
    setMode(BG_MODE7, 0);
    mode7Init();
    mode7SetScale(0x0200, 0x0200);
    mode7SetAngle(0);

    /* Turn on display with BG1 -- Mode 7 only supports a single BG layer
     * (BG1), so we enable just that on the main screen via REG_TM. */
    setMainScreen(TM_BG1);
    setScreenOn();

    while (1) {
        pad0 = padHeld(0);

        /* Rotate clockwise with A */
        if (pad0 & KEY_A) {
            angle++;
            mode7SetAngle(angle);
        }

        /* Rotate counter-clockwise with B */
        if (pad0 & KEY_B) {
            angle--;
            mode7SetAngle(angle);
        }

        /* Zoom out with UP (increase scale = shows more of the plane).
         * Scale is 8.8 fixed-point: 0x0100 = 1.0x, 0x0200 = 2.0x.
         * Higher values shrink the image (show more area). Clamped
         * to 0x0F00 (15x) to prevent extreme distortion.
         * mode7SetAngle() must be called after mode7SetScale() because
         * the scale factors are incorporated into the rotation matrix. */
        if (pad0 & KEY_UP) {
            if (zscale < 0x0F00)
                zscale += 16;
            mode7SetScale(zscale, zscale);
            mode7SetAngle(angle);
        }

        /* Zoom in with DOWN (decrease scale = magnifies).
         * Lower scale values enlarge the image. Clamped to 0x0010 to
         * prevent division-by-zero-like artifacts in the matrix. */
        if (pad0 & KEY_DOWN) {
            if (zscale > 0x0010)
                zscale -= 16;
            mode7SetScale(zscale, zscale);
            mode7SetAngle(angle);
        }

        WaitForVBlank();
    }
    return 0;
}
