/*
 * img2snes - PNG RGB to indexed PNG converter for OpenSNES
 * Median-cut color quantization
 */

#ifndef QUANTIZE_H
#define QUANTIZE_H

typedef struct {
    unsigned char r, g, b, a;
} rgba_t;

typedef struct {
    unsigned char r, g, b;
} rgb_t;

/*
 * Quantize an RGBA image to a palette of at most max_colors colors
 * using the median-cut algorithm.
 *
 * If transparent pixels (alpha < 128) are present, index 0 is reserved
 * for transparency and the effective palette is max_colors - 1 opaque colors.
 *
 * out_palette: must hold at least max_colors entries
 * out_indices: must hold at least width * height entries
 *
 * Returns the number of colors actually used (<= max_colors).
 */
int quantize_median_cut(
    const rgba_t *pixels,
    int width, int height,
    int max_colors,
    rgb_t *out_palette,
    unsigned char *out_indices
);

/*
 * Map each pixel to the nearest color in the given palette.
 * Transparent pixels (alpha < 128) are mapped to index 0.
 *
 * Returns 0 on success.
 */
int quantize_map_to_palette(
    const rgba_t *pixels,
    int width, int height,
    const rgb_t *palette,
    int num_colors,
    unsigned char *out_indices
);

#endif /* QUANTIZE_H */
