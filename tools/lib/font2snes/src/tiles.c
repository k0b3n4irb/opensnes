/**
 * @file tiles.c
 * @brief SNES tile conversion implementation
 *
 * License: MIT
 */

#include "tiles.h"

void convert_tile_2bpp(const uint8_t *indexed, uint8_t *snes)
{
    int row, col;

    for (row = 0; row < 8; row++) {
        uint8_t bp0 = 0;
        uint8_t bp1 = 0;

        for (col = 0; col < 8; col++) {
            uint8_t pixel = indexed[row * 8 + col];

            /* Extract bit 0 and bit 1 of the palette index */
            bp0 = (bp0 << 1) | ((pixel >> 0) & 1);
            bp1 = (bp1 << 1) | ((pixel >> 1) & 1);
        }

        /* SNES 2bpp: interleaved bitplanes */
        snes[row * 2 + 0] = bp0;
        snes[row * 2 + 1] = bp1;
    }
}

void convert_tile_4bpp(const uint8_t *indexed, uint8_t *snes)
{
    int row, col;

    for (row = 0; row < 8; row++) {
        uint8_t bp0 = 0;
        uint8_t bp1 = 0;
        uint8_t bp2 = 0;
        uint8_t bp3 = 0;

        for (col = 0; col < 8; col++) {
            uint8_t pixel = indexed[row * 8 + col];

            /* Extract all 4 bits of the palette index */
            bp0 = (bp0 << 1) | ((pixel >> 0) & 1);
            bp1 = (bp1 << 1) | ((pixel >> 1) & 1);
            bp2 = (bp2 << 1) | ((pixel >> 2) & 1);
            bp3 = (bp3 << 1) | ((pixel >> 3) & 1);
        }

        /* SNES 4bpp: low bitplanes first, then high bitplanes */
        snes[row * 2 + 0] = bp0;       /* Low bitplane 0 */
        snes[row * 2 + 1] = bp1;       /* Low bitplane 1 */
        snes[16 + row * 2 + 0] = bp2;  /* High bitplane 2 */
        snes[16 + row * 2 + 1] = bp3;  /* High bitplane 3 */
    }
}

uint16_t rgb_to_bgr555(uint8_t r, uint8_t g, uint8_t b)
{
    /* SNES BGR555 format: 0BBBBBGG GGGRRRRR */
    uint16_t r5 = (r >> 3) & 0x1F;
    uint16_t g5 = (g >> 3) & 0x1F;
    uint16_t b5 = (b >> 3) & 0x1F;

    return (b5 << 10) | (g5 << 5) | r5;
}
