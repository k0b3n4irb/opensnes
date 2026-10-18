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
 * Tiles known at build time come from opensnes-tileset or opensnes-sprite; these are for tiles made
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
 * carried a C copy of this loop until 2026-09-29, at ~310 000 a tile.
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

/**
 * @brief OR rows of a 2bpp glyph into a tile at a pixel offset
 *
 * Puts `rows` rows of `glyph` (a 2bpp tile: 16 bytes, planes 0/1
 * interleaved per row) into `tile`, from the glyph's row `glyph_row` onto
 * the tile's row `tile_row`, each row shifted right by `shift` pixels
 * (0-7). The bits that leave the tile on the right go into `right`, the
 * tile next to it (NULL: they are dropped). Only ORs: the tile keeps what it
 * had, so glyphs may overlap, and a buffer is cleared with memset() before
 * a new layout.
 *
 * This is the primitive of text composed at the pixel in a 2bpp layer kept
 * in RAM (a proportional font at a 6-pixel pitch, a label at any x and y):
 * a glyph placed at (x, y) touches up to four tiles, and two calls cover it,
 * one per tile row, with `right` the tile to the right of each:
 *
 * @code
 * u8 top = y & 7;                      // the glyph's first row in the tile
 * tileBlitGlyph2bpp(t, tr, g, 0, top, 8 - top, x & 7);        // upper tiles
 * if (top)
 *     tileBlitGlyph2bpp(b, br, g, 8 - top, 0, top, x & 7);    // lower tiles
 * @endcode
 *
 * All pointers are far: the layer lives in FAR RAM and the font in an asset
 * bank. Cost: about 9 000 master cycles for the eight rows of a glyph with a
 * right tile (two calls), measured on luna.
 *
 * @param tile      the tile that receives the glyph's left part (never NULL)
 * @param right     the tile to its right, for the bits shifted out; NULL to
 *                  drop them (always safe when `shift` is 0)
 * @param glyph     16 bytes, planes 0/1 interleaved per row (a 2bpp tile)
 * @param glyph_row first row of the glyph to put (0-7)
 * @param tile_row  row of the tile that receives it (0-7)
 * @param rows      number of rows (1-8; `glyph_row + rows` and
 *                  `tile_row + rows` at most 8)
 * @param shift     pixels to shift right (0-7)
 */
void tileBlitGlyph2bpp(u8 FAR *tile, u8 FAR *right, const u8 FAR *glyph,
                       u8 glyph_row, u8 tile_row, u8 rows, u8 shift);

#endif /* SNES_TILE_H */
