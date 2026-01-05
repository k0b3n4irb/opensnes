/**
 * @file output.h
 * @brief Output file generation for font2snes
 *
 * License: MIT
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>

/**
 * @brief Font data structure
 */
typedef struct {
    uint8_t *tiles;         /* SNES format tile data */
    int tile_count;         /* Number of characters (96) */
    int bytes_per_tile;     /* 16 for 2bpp, 32 for 4bpp */
    int bpp;                /* 2 or 4 */
    uint16_t palette[16];   /* BGR555 palette */
    int palette_count;      /* Number of colors */
    const char *name;       /* Base name for output */
} font_data_t;

/**
 * @brief Write font data as C header file
 *
 * @param font Font data structure
 * @param output_path Output file path
 * @return 0 on success, -1 on error
 */
int output_c_header(const font_data_t *font, const char *output_path);

/**
 * @brief Write font tile data as binary file (.pic)
 *
 * @param font Font data structure
 * @param output_path Output file path
 * @return 0 on success, -1 on error
 */
int output_binary_tiles(const font_data_t *font, const char *output_path);

/**
 * @brief Write font palette as binary file (.pal)
 *
 * @param font Font data structure
 * @param output_path Output file path
 * @return 0 on success, -1 on error
 */
int output_binary_palette(const font_data_t *font, const char *output_path);

#endif /* OUTPUT_H */
