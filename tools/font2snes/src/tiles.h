/**
 * @file tiles.h
 * @brief SNES tile conversion functions
 *
 * Converts indexed pixel data to SNES bitplane format.
 *
 * License: MIT
 */

#ifndef TILES_H
#define TILES_H

#include <stdint.h>

/**
 * @brief Convert an 8x8 indexed tile to SNES 2bpp format
 *
 * SNES 2bpp format (16 bytes per tile):
 *   Byte 0: Row 0, bitplane 0
 *   Byte 1: Row 0, bitplane 1
 *   Byte 2: Row 1, bitplane 0
 *   ... and so on (interleaved)
 *
 * @param indexed Input: 64 bytes, one byte per pixel (palette index 0-3)
 * @param snes    Output: 16 bytes in SNES 2bpp format
 */
void convert_tile_2bpp(const uint8_t *indexed, uint8_t *snes);

/**
 * @brief Convert an 8x8 indexed tile to SNES 4bpp format
 *
 * SNES 4bpp format (32 bytes per tile):
 *   Bytes 0-15:  Low bitplanes (0 and 1, interleaved)
 *   Bytes 16-31: High bitplanes (2 and 3, interleaved)
 *
 * @param indexed Input: 64 bytes, one byte per pixel (palette index 0-15)
 * @param snes    Output: 32 bytes in SNES 4bpp format
 */
void convert_tile_4bpp(const uint8_t *indexed, uint8_t *snes);

/**
 * @brief Convert RGB palette to SNES BGR555 format
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return 16-bit SNES BGR555 color
 */
uint16_t rgb_to_bgr555(uint8_t r, uint8_t g, uint8_t b);

#endif /* TILES_H */
