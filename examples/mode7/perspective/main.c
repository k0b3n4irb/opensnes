/**
 * @file main.c
 * @brief Mode 7 perspective with an HDMA split screen (F-Zero style)
 * @ingroup examples
 *
 * Creates an F-Zero-style pseudo-3D ground plane by combining two BG modes
 * on one screen. HDMA changes registers between scanlines: the top 96 lines
 * show a sky in Mode 1 (BG2), the lines below a Mode 7 ground whose scale
 * changes on every line, so that distant rows are compressed. Four HDMA
 * channels share the work: one writes BGMODE at the split line, one the
 * main screen layers (BG2 for the sky, BG1 for the ground), and two write
 * M7A and M7D on every ground line.
 *
 * This is the first step: the two diagonal terms of the matrix give depth
 * without rotation. mode7/perspective_rotate drives all four terms per line
 * and turns the world.
 *
 * Based on PVSnesLib's Mode7Perspective (mills32 / alekmaul); the city and
 * its sky are ours (devtools/art_assets/mode7.py writes them).
 *
 * @par SNES Concepts
 * - Mid-frame BG mode switching via HDMA (Mode 1 sky + Mode 7 ground)
 * - Per-scanline M7A/M7D writes (HDMA repeat mode) for the perspective
 * - HDMA-driven main screen layer switching
 * - HDMA tables set up once: the hardware restarts each one every frame
 * - Mode 7 scroll and center written in VBlank
 *
 * @par What to Observe
 * - The screen is split: a flat sky image on top and a perspective ground below
 * - Press D-pad to scroll the ground plane in any direction
 * - Distant ground rows appear more compressed (perspective foreshortening)
 *
 * @par Modules Used
 * console, dma, background, sprite, input, mode7, hdma
 *
 * @see mode7.h, hdma.h, background.h, input.h
 */

#include <snes.h>
#include <snes/mode7.h>
#include <snes/hdma.h>

/* The ground (Mode 7: interleaved map and tiles) and the sky (a 4bpp BG),
 * converted by the build from res/ground.png and res/sky.png */
#include "res/ground.inc"   /* generated: ground_tiles[], ground_map[], ground_pal[] (Mode 7 .pc7 / .mp7), each with _end */
#include "res/sky.inc"      /* generated: sky_tiles[], sky_map[], sky_pal[], each with _end */

/** @brief The scanline where the sky ends and the ground begins */
#define SKYLINE 96

/** @brief BGMODE per line: Mode 1 for the sky ($09: Mode 1 with the BG3
 * priority bit), Mode 7 for the ground. An entry writes once and holds. */
static const u8 mode_table[] = {
    SKYLINE, 0x09,
    1,       0x07,
    0x00
};

/** @brief TM (main screen layers) per line: OBJ + BG2 for the sky,
 * OBJ + BG1 for the ground. */
static const u8 layer_table[] = {
    SKYLINE, 0x12,
    1,       0x11,
    0x00
};

/** @brief M7A per ground line: 512 / zoom, the zoom growing by 0.025 a line (8.8 fixed point, low byte first; the first line is the horizon). */
static const u8 perspective_x[] = {
    SKYLINE, 0x00, 0x01,    /* the sky lines: 1.0 (unused in Mode 1) */
    0x80 | 127,             /* then 127 lines, a new value on each line */
    1, 0, 0, 2, 243, 1, 231, 1, 220, 1, 209, 1, 199, 1, 189, 1,
    179, 1, 170, 1, 161, 1, 153, 1, 145, 1, 137, 1, 130, 1, 123, 1,
    116, 1, 109, 1, 103, 1, 97, 1, 91, 1, 85, 1, 79, 1, 74, 1,
    69, 1, 64, 1, 59, 1, 54, 1, 49, 1, 45, 1, 40, 1, 36, 1,
    32, 1, 28, 1, 24, 1, 20, 1, 17, 1, 13, 1, 9, 1, 6, 1,
    3, 1, 0, 1, 252, 0, 249, 0, 246, 0, 243, 0, 240, 0, 238, 0,
    235, 0, 232, 0, 230, 0, 227, 0, 225, 0, 222, 0, 220, 0, 217, 0,
    215, 0, 213, 0, 211, 0, 208, 0, 206, 0, 204, 0, 202, 0, 200, 0,
    198, 0, 196, 0, 195, 0, 193, 0, 191, 0, 189, 0, 187, 0, 186, 0,
    184, 0, 182, 0, 181, 0, 179, 0, 178, 0, 176, 0, 175, 0, 173, 0,
    172, 0, 170, 0, 169, 0, 167, 0, 166, 0, 165, 0, 163, 0, 162, 0,
    161, 0, 159, 0, 158, 0, 157, 0, 156, 0, 155, 0, 153, 0, 152, 0,
    151, 0, 150, 0, 149, 0, 148, 0, 147, 0, 146, 0, 145, 0, 144, 0,
    143, 0, 142, 0, 141, 0, 140, 0, 139, 0, 138, 0, 137, 0, 136, 0,
    135, 0, 134, 0, 133, 0, 132, 0, 132, 0, 131, 0, 130, 0, 129, 0,
    128, 0, 127, 0, 127, 0, 126, 0, 125, 0, 124, 0, 124, 0,
    0x00                    /* end: the last line keeps the last value */
};

/** @brief M7D per ground line: 4096 / zoom, the zoom growing by 0.05 a line (8.8 fixed point, low byte first; the first line is the horizon). */
static const u8 perspective_y[] = {
    SKYLINE, 0x00, 0x01,    /* the sky lines: 1.0 (unused in Mode 1) */
    0x80 | 127,             /* then 127 lines, a new value on each line */
    122, 0, 0, 16, 60, 15, 139, 14, 233, 13, 85, 13, 204, 12, 78, 12,
    218, 11, 109, 11, 8, 11, 170, 10, 82, 10, 0, 10, 178, 9, 105, 9,
    36, 9, 227, 8, 166, 8, 107, 8, 52, 8, 0, 8, 206, 7, 158, 7,
    113, 7, 69, 7, 28, 7, 244, 6, 206, 6, 170, 6, 135, 6, 102, 6,
    70, 6, 39, 6, 9, 6, 237, 5, 209, 5, 182, 5, 157, 5, 132, 5,
    108, 5, 85, 5, 62, 5, 41, 5, 20, 5, 0, 5, 236, 4, 217, 4,
    198, 4, 180, 4, 163, 4, 146, 4, 129, 4, 113, 4, 98, 4, 83, 4,
    68, 4, 53, 4, 39, 4, 26, 4, 12, 4, 0, 4, 243, 3, 231, 3,
    218, 3, 207, 3, 195, 3, 184, 3, 173, 3, 162, 3, 152, 3, 142, 3,
    132, 3, 122, 3, 112, 3, 103, 3, 94, 3, 85, 3, 76, 3, 67, 3,
    59, 3, 51, 3, 43, 3, 35, 3, 27, 3, 19, 3, 12, 3, 4, 3,
    253, 2, 246, 2, 239, 2, 232, 2, 226, 2, 219, 2, 212, 2, 206, 2,
    200, 2, 194, 2, 188, 2, 182, 2, 176, 2, 170, 2, 165, 2, 159, 2,
    154, 2, 148, 2, 143, 2, 138, 2, 133, 2, 127, 2, 123, 2, 118, 2,
    113, 2, 108, 2, 103, 2, 99, 2, 94, 2, 90, 2, 85, 2, 81, 2,
    77, 2, 73, 2, 68, 2, 64, 2, 60, 2, 56, 2, 52, 2,
    0x00                    /* end: the last line keeps the last value */
};

u16 pad0;           /**< Current joypad button state */
u16 sx;             /**< Mode 7 ground horizontal scroll offset (camera X) */
u16 sy;             /**< Mode 7 ground vertical scroll offset (camera Y) */

/**
 * @brief Place the camera: Mode 7 scroll and center, and the sky's scroll.
 *
 * mode7SetScroll() and mode7SetCenter() write the registers at once, so
 * this runs in VBlank or force blank. The sky follows the ground
 * horizontally (bgSetScroll, written by the NMI handler).
 */
static void setCamera(void) {
    mode7SetScroll((s16)sx, (s16)sy);
    mode7SetCenter((s16)(sx + 128), (s16)(sy + 112));
    bgSetScroll(1, sx, 0);
}

/**
 * @brief Entry point -- F-Zero-style Mode 7 perspective with an HDMA split screen.
 *
 * Loads the ground and the sky, sets up the four HDMA channels once, then
 * scrolls the ground with the D-pad.
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    consoleInit();

    /* Force blank for VRAM loading */
    setScreenOff();

    /* Mode 7 ground at VRAM $0000 (low byte of each word = map, high byte
     * = pixels), 128x128 tiles at 256 colors */
    dmaCopyVramMode7(ground_map, ground_map_end - ground_map,
                     ground_tiles, ground_tiles_end - ground_tiles);
    dmaCopyCGram(ground_pal, 0, ground_pal_end - ground_pal);

    /* The sky, a 4bpp BG2: map at $4000 (64x32), tiles at $5000 */
    dmaCopyVram(sky_map, 0x4000, (u16)(sky_map_end - sky_map));
    dmaCopyVram(sky_tiles, 0x5000, (u16)(sky_tiles_end - sky_tiles));
    bgSetMapPtr(1, 0x4000, SC_64x32);
    bgSetGfxPtr(1, 0x5000);

    /* Mode 7 as the base mode, identity matrix; HDMA switches the sky lines
     * to Mode 1 */
    setMode(BG_MODE7, 0);
    mode7Init();

    sx = 0;
    sy = 0;
    setCamera();

    /* The four channels. Each table is read from its start again at the
     * top of every frame, so this is done once. (Every DMA of the NMI
     * handler runs on channel 7; channel 0 serves the DMA helpers.) */
    hdmaSetup(HDMA_CHANNEL_1, HDMA_MODE_1REG,    HDMA_DEST_BGMODE, mode_table);
    hdmaSetup(HDMA_CHANNEL_2, HDMA_MODE_1REG,    HDMA_DEST_TM,     layer_table);
    hdmaSetup(HDMA_CHANNEL_3, HDMA_MODE_1REG_2X, HDMA_DEST_M7A,    perspective_x);
    hdmaSetup(HDMA_CHANNEL_4, HDMA_MODE_1REG_2X, HDMA_DEST_M7D,    perspective_y);
    hdmaEnableMask((1 << HDMA_CHANNEL_1) | (1 << HDMA_CHANNEL_2)
                   | (1 << HDMA_CHANNEL_3) | (1 << HDMA_CHANNEL_4));

    setScreenOn();

    while (1) {
        pad0 = padHeld(0);

        if (pad0 & KEY_LEFT)  sx--;
        if (pad0 & KEY_RIGHT) sx++;
        if (pad0 & KEY_UP)    sy--;
        if (pad0 & KEY_DOWN)  sy++;

        WaitForVBlank();
        setCamera();
    }

    return 0;
}
