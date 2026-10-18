/**
 * @file main.c
 * @brief Mode 7 rotation and scaling, flown over: a glider, its shadow and the altitude
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
 * A glider flies over the plane at the rotation centre, and the zoom is its
 * altitude: zooming out widens the view as if it climbed, and its shadow —
 * the same sprite in a dark palette, behind it — slides further away. The
 * two together are how a flat Mode 7 plane reads as height. (The glider
 * and its shadow were the games/mode7_flying example.)
 *
 * @par SNES Concepts
 * - Mode 7 affine transformation (rotation + scaling via M7A-M7D)
 * - Interleaved VRAM format (tilemap in low bytes, pixels in high bytes)
 * - 8.8 fixed-point scale representation
 * - 8-bit angle for hardware sin/cos lookup
 * - The rotation centre on screen: M7X/M7Y is a point of the plane, the
 *   scroll says where it lands (here under the glider, the screen centre)
 * - Two depth cues on a flat plane: the scale, and a shadow offset that
 *   grows with it
 *
 * @par What to Observe
 * - Press A to rotate the background clockwise
 * - Press B to rotate counter-clockwise
 * - Press UP to zoom out (background shrinks, more visible)
 * - Press DOWN to zoom in (background magnifies)
 * - The plane turns under the glider; its shadow drifts away as you zoom
 *   out (climb) and closes in as you zoom in (descend)
 *
 * @par Modules Used
 * console, dma, background, sprite, input, mode7
 *
 * @see mode7.h, background.h, input.h
 */

#include <snes.h>

/** @brief Mode 7 tile pixel data (256 tiles x 64 bytes, 8bpp), converted by the build from res/ */
#include "res/mode7bg.inc"   /* generated: mode7bg_tiles[], mode7bg_map[], mode7bg_pal[] (Mode 7 .pc7 / .mp7), each with _end */
/** @brief Mode 7 tilemap (128x128 tile indices, one byte each), converted by the build from res/ */
/** @brief 256-colour palette for the Mode 7 plane (512 bytes), converted by the build from res/ */

/** @brief Where the glider is drawn: its 16x16 box centred on screen (128, 112). */
#define GLIDER_X 120
#define GLIDER_Y 104

/** @brief Probe oracle: the shadow's offset below the glider, in pixels. */
u16 shadow_dy;

/** @brief 4 8x8 4bpp tiles = one 16x16 shape (built at init). */
static u8 shape_tiles[128];

/** @brief Encode a 16x16 palette-index grid into 4bpp OBJ quadrants. */
static void encode_16x16(u8 pix[16][16]) {
    u8 row, plane_i, t, q;
    for (q = 0; q < 4; q++) {
        u8 qx = (u8)((q & 1) * 8);
        u8 qy = (u8)((q >> 1) * 8);
        for (row = 0; row < 8; row++) {
            for (plane_i = 0; plane_i < 4; plane_i += 2) {
                u8 b0 = 0, b1 = 0;
                for (t = 0; t < 8; t++) {
                    u8 p = pix[qy + row][qx + t];
                    b0 = (u8)((b0 << 1) | ((p >> plane_i) & 1));
                    b1 = (u8)((b1 << 1) | ((p >> (plane_i + 1)) & 1));
                }
                shape_tiles[(u16)(q * 32) + (plane_i ? 16 : 0) + row * 2] = b0;
                shape_tiles[(u16)(q * 32) + (plane_i ? 16 : 0) + row * 2 + 1] = b1;
            }
        }
    }
}

/** @brief Draw the glider seen from above (index 1 body, 2 canopy). */
static void build_glider(void) {
    static u8 pix[16][16];
    u8 x, y;
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            u8 dx = (u8)(x >= 8 ? x - 8 : 7 - x);
            u8 v = 0;
            if (y >= 2 && y <= 13 && dx <= 1)
                v = 1;                                   /* fuselage */
            if (y >= 6 && y <= 9 && dx <= (u8)(7 - (y - 6) * 2))
                v = 1;                                   /* wings */
            if (y >= 12 && y <= 13 && dx <= 3)
                v = 1;                                   /* tail */
            pix[y][x] = v;
        }
    }
    for (y = 4; y < 7; y++) {
        pix[y][7] = 2;
        pix[y][8] = 2;                                   /* canopy */
    }
    encode_16x16(pix);
}

/** @brief Draw the shadow below the glider, further away the higher it flies. */
static void place_shadow(u16 zscale) {
    shadow_dy = (u16)(zscale >> 5);           /* 8 px at 1:1, 60 at the widest view */
    oamSet(0, GLIDER_X + 2, (u16)(GLIDER_Y + shadow_dy), 0, 1, 2, 0);
    oamSetSize(0, OBJ_LARGE);
}

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
    u16 zscale = 0x0100;               /**< Zoom level in 8.8 fixed-point (1.0 = 1:1, higher = more zoomed out) */

    consoleInit();

    /* Force blank for VRAM loading -- VRAM writes are silently ignored
     * during active display on the SNES PPU. */
    setScreenOff();

    /* Load the Mode 7 plane. VRAM is interleaved in this mode (tilemap in
     * the low bytes, 8bpp pixels in the high bytes), so dmaCopyVramMode7()
     * runs the two DMAs with the right VMAIN each time; the palette is a
     * plain CGRAM copy. Both need forced blank. */
    dmaCopyVramMode7(mode7bg_map, (u16)(mode7bg_map_end - mode7bg_map),
                     mode7bg_tiles, (u16)(mode7bg_tiles_end - mode7bg_tiles));
    dmaCopyCGram(mode7bg_pal, 0, (u16)(mode7bg_pal_end - mode7bg_pal));

    /* Set Mode 7 and initialize the affine transformation matrix.
     * mode7Init() sets the center (M7X/M7Y) and the scroll (M7HOFS/M7VOFS).
     * mode7SetScale(0x0100) is 1:1,
     * and mode7SetAngle() writes the unrotated matrix (angle 0). */
    setMode(BG_MODE7, 0);
    mode7Init();
    mode7SetScale(0x0100, 0x0100);
    mode7SetAngle(0);
    /* mode7Init leaves the rotation centre (M7X/M7Y = 128, 128) above the
     * screen; this scroll puts it at screen (128, 112), under the glider */
    mode7SetScroll(0, 16);

    /* The glider and its shadow: one 16x16 shape at OBJ base 3 ($6000),
     * palette 0 orange and dark blue (it must show over a pale plane),
     * palette 1 all dark for the shadow (the plane uses CGRAM 0-13, so 128
     * and up are free) */
    build_glider();
    dmaCopyVram(shape_tiles, 0x6000, 64);
    dmaCopyVram(shape_tiles + 64, 0x6100, 64);
    setColor(129, RGB(31, 14, 0));
    setColor(130, RGB(4, 6, 20));
    setColor(145, RGB(3, 5, 3));
    setColor(146, RGB(3, 5, 3));
    oamInit(OBJ_SIZE8_L16, 3);
    oamSet(1, GLIDER_X, GLIDER_Y, 0, 0, 3, 0);        /* sprite 1 = glider, in front */
    oamSetSize(1, OBJ_LARGE);
    place_shadow(zscale);                            /* sprite 0 = shadow, behind it */

    /* Turn on display with BG1 -- Mode 7 only supports a single BG layer
     * (BG1), so we enable just that on the main screen via REG_TM. */
    setMainScreen(TM_BG1 | TM_OBJ);
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
         * Scale is 8.8 fixed-point texels per pixel: 0x0100 = 1:1,
         * 0x0200 = shrunk twice. Higher values shrink the image (show
         * more area). Clamped to 0x0780 (7.5x) to prevent extreme
         * distortion.
         * mode7SetAngle() must be called after mode7SetScale() because
         * the scale factors are incorporated into the rotation matrix. */
        if (pad0 & KEY_UP) {
            if (zscale < 0x0780)
                zscale += 8;
            mode7SetScale(zscale, zscale);
            mode7SetAngle(angle);
            place_shadow(zscale);
        }

        /* Zoom in with DOWN (decrease scale = magnifies).
         * Lower scale values enlarge the image. Clamped to 0x0008 to
         * prevent division-by-zero-like artifacts in the matrix. */
        if (pad0 & KEY_DOWN) {
            if (zscale > 0x0008)
                zscale -= 8;
            mode7SetScale(zscale, zscale);
            mode7SetAngle(angle);
            place_shadow(zscale);
        }

        WaitForVBlank();
    }
    return 0;
}
