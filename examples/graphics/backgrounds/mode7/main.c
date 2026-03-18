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

/* Assembly helper to load Mode 7 data with proper VRAM interleaving */
extern void asm_loadMode7Data(void);

int main(void) {
    u16 pad0;
    u8 angle = 0;
    u16 zscale = 0x0200; /* 2.0 in 8.8 fixed point (matches PVSnesLib default) */

    consoleInit();

    /* Force blank for VRAM loading */
    setScreenOff();

    /* Load Mode 7 tile data, tilemap, and palette via assembly helper */
    asm_loadMode7Data();

    /* Set Mode 7 and initialize transformation */
    setMode(BG_MODE7, 0);
    mode7Init();
    mode7SetScale(0x0200, 0x0200);
    mode7SetAngle(0);

    /* Turn on display with BG1 */
    REG_TM = TM_BG1;
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

        /* Zoom out with UP (increase scale = shows more of the plane) */
        if (pad0 & KEY_UP) {
            if (zscale < 0x0F00)
                zscale += 16;
            mode7SetScale(zscale, zscale);
            mode7SetAngle(angle);
        }

        /* Zoom in with DOWN (decrease scale = magnifies) */
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
