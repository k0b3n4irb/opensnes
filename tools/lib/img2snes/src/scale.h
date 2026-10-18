/*
 * img2snes - PNG RGB to indexed PNG converter for OpenSNES
 * Nearest-neighbor scaling and tile alignment
 */

#ifndef SCALE_H
#define SCALE_H

#include "quantize.h" /* for rgba_t */

/*
 * Scale an RGBA image using nearest-neighbor interpolation.
 * Returns a newly allocated buffer (caller must free).
 * Returns NULL on allocation failure.
 */
rgba_t *scale_nearest(
    const rgba_t *src,
    int src_w, int src_h,
    int dst_w, int dst_h
);

/*
 * Compute destination dimensions from source dimensions,
 * a scale factor, and optional tile alignment.
 *
 * scale: multiplier (1.0 = no change)
 * tile_align: if > 0, round up to the nearest multiple of this value
 */
void compute_dimensions(
    int src_w, int src_h,
    double scale, int tile_align,
    int *dst_w, int *dst_h
);

#endif /* SCALE_H */
