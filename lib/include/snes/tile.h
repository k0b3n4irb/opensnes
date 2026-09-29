/**
 * @file tile.h
 * @brief Build SNES tiles from pixels at run time
 *
 * A SNES tile is planar: its bits are spread over bitplanes, two planes
 * interleaved per 16-byte block (row 0 plane 0, row 0 plane 1, row 1
 * plane 0, ...), then the next pair. These functions turn an 8x8 block of
 * colour indices — one byte per pixel, row by row, the form a generator or
 * an effect writes naturally — into that layout, ready for dmaCopyVram().
 *
 * Tiles known at build time come from gfx4snes; these are for tiles made
 * while the game runs (procedural art, a generated font, a plotted
 * sprite).
 *
 * @code
 * static u8 pixels[64];            // one colour index per pixel
 * static u8 tile[32];
 * pixels[3 * 8 + 5] = 7;           // x = 5, y = 3 in colour 7
 * tileEncode4bpp(pixels, tile);
 * dmaCopyVram(tile, 0x1000, 32);   // during VBlank or force blank
 * @endcode
 *
 * Link with `LIB_MODULES += tile`. Colour bits above the depth are ignored.
 * Cost: about 47 000 master cycles a tile (an eighth of a frame), measured
 * on luna — build tiles during loading, or a few per frame. Four examples
 * carried a C copy of this loop until 2026-09-30, at ~310 000 a tile.
 */

#ifndef SNES_TILE_H
#define SNES_TILE_H

#include <snes/types.h>

/**
 * @brief 64 colour indices (0-3) to one 2bpp tile
 * @param pixels 64 bytes, row by row
 * @param tile   16 bytes out: planes 0/1 interleaved per row
 */
void tileEncode2bpp(const u8 *pixels, u8 *tile);

/**
 * @brief 64 colour indices (0-15) to one 4bpp tile
 * @param pixels 64 bytes, row by row
 * @param tile   32 bytes out: planes 0/1 interleaved per row, then 2/3
 */
void tileEncode4bpp(const u8 *pixels, u8 *tile);

/**
 * @brief 64 colour indices (0-255) to one 8bpp tile
 * @param pixels 64 bytes, row by row
 * @param tile   64 bytes out: plane pairs 0/1, 2/3, 4/5, 6/7, each
 *               interleaved per row
 */
void tileEncode8bpp(const u8 *pixels, u8 *tile);

#endif /* SNES_TILE_H */
