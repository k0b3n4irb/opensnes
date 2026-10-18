/*
 * img2snes - PNG RGB to indexed PNG converter for OpenSNES
 * Nearest-neighbor scaling and tile alignment
 */

#include "scale.h"
#include <stdlib.h>
#include <math.h>

rgba_t *scale_nearest(
    const rgba_t *src,
    int src_w, int src_h,
    int dst_w, int dst_h)
{
    if (dst_w <= 0 || dst_h <= 0) return NULL;

    rgba_t *dst = malloc((size_t)dst_w * dst_h * sizeof(rgba_t));
    if (!dst) return NULL;

    for (int y = 0; y < dst_h; y++) {
        int sy = y * src_h / dst_h;
        if (sy >= src_h) sy = src_h - 1;
        for (int x = 0; x < dst_w; x++) {
            int sx = x * src_w / dst_w;
            if (sx >= src_w) sx = src_w - 1;
            dst[y * dst_w + x] = src[sy * src_w + sx];
        }
    }

    return dst;
}

void compute_dimensions(
    int src_w, int src_h,
    double scale, int tile_align,
    int *dst_w, int *dst_h)
{
    int w = (int)round((double)src_w * scale);
    int h = (int)round((double)src_h * scale);

    if (w < 1) w = 1;
    if (h < 1) h = 1;

    if (tile_align > 0) {
        w = ((w + tile_align - 1) / tile_align) * tile_align;
        h = ((h + tile_align - 1) / tile_align) * tile_align;
    }

    *dst_w = w;
    *dst_h = h;
}
