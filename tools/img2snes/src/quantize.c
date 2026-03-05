/*
 * img2snes - PNG RGB to indexed PNG converter for OpenSNES
 * Median-cut color quantization
 */

#include "quantize.h"
#include <stdlib.h>
#include <string.h>

/* A "box" in RGB space containing a set of pixel colors */
typedef struct {
    int r_min, r_max;
    int g_min, g_max;
    int b_min, b_max;
    int count;       /* number of pixels in this box */
    int *indices;    /* indices into the unique-color array */
} color_box_t;

/* Unique color with frequency */
typedef struct {
    unsigned char r, g, b;
    int freq;
} ucolor_t;

/* Sort helpers — we use qsort with a wrapper since we need context.
 * To avoid non-portable qsort_r, we use a temporary global for the
 * comparison functions. */
static ucolor_t *g_sort_colors; /* temporary global for qsort */

static int cmp_by_r(const void *a, const void *b) {
    int ia = *(const int *)a, ib = *(const int *)b;
    return (int)g_sort_colors[ia].r - (int)g_sort_colors[ib].r;
}
static int cmp_by_g(const void *a, const void *b) {
    int ia = *(const int *)a, ib = *(const int *)b;
    return (int)g_sort_colors[ia].g - (int)g_sort_colors[ib].g;
}
static int cmp_by_b(const void *a, const void *b) {
    int ia = *(const int *)a, ib = *(const int *)b;
    return (int)g_sort_colors[ia].b - (int)g_sort_colors[ib].b;
}

static void box_compute_bounds(color_box_t *box, const ucolor_t *colors) {
    box->r_min = 255; box->r_max = 0;
    box->g_min = 255; box->g_max = 0;
    box->b_min = 255; box->b_max = 0;
    for (int i = 0; i < box->count; i++) {
        const ucolor_t *c = &colors[box->indices[i]];
        if (c->r < box->r_min) box->r_min = c->r;
        if (c->r > box->r_max) box->r_max = c->r;
        if (c->g < box->g_min) box->g_min = c->g;
        if (c->g > box->g_max) box->g_max = c->g;
        if (c->b < box->b_min) box->b_min = c->b;
        if (c->b > box->b_max) box->b_max = c->b;
    }
}

/* Find the longest axis of a box: 0=R, 1=G, 2=B */
static int box_longest_axis(const color_box_t *box) {
    int r_range = box->r_max - box->r_min;
    int g_range = box->g_max - box->g_min;
    int b_range = box->b_max - box->b_min;
    if (r_range >= g_range && r_range >= b_range) return 0;
    if (g_range >= r_range && g_range >= b_range) return 1;
    return 2;
}

/* Weighted pixel count of a box */
static long box_pixel_count(const color_box_t *box, const ucolor_t *colors) {
    long total = 0;
    for (int i = 0; i < box->count; i++)
        total += colors[box->indices[i]].freq;
    return total;
}

/* Compute the average color of a box, weighted by frequency */
static rgb_t box_average(const color_box_t *box, const ucolor_t *colors) {
    long r_sum = 0, g_sum = 0, b_sum = 0, total = 0;
    for (int i = 0; i < box->count; i++) {
        const ucolor_t *c = &colors[box->indices[i]];
        r_sum += (long)c->r * c->freq;
        g_sum += (long)c->g * c->freq;
        b_sum += (long)c->b * c->freq;
        total += c->freq;
    }
    rgb_t avg;
    if (total == 0) {
        avg.r = avg.g = avg.b = 0;
    } else {
        avg.r = (unsigned char)((r_sum + total / 2) / total);
        avg.g = (unsigned char)((g_sum + total / 2) / total);
        avg.b = (unsigned char)((b_sum + total / 2) / total);
    }
    return avg;
}

/* Split a box at the median of its longest axis.
 * Returns 1 on success, 0 if the box can't be split. */
static int box_split(color_box_t *box, color_box_t *new_box,
                     ucolor_t *colors) {
    if (box->count < 2) return 0;

    int axis = box_longest_axis(box);

    g_sort_colors = colors;
    switch (axis) {
    case 0: qsort(box->indices, box->count, sizeof(int), cmp_by_r); break;
    case 1: qsort(box->indices, box->count, sizeof(int), cmp_by_g); break;
    case 2: qsort(box->indices, box->count, sizeof(int), cmp_by_b); break;
    }

    /* Find the median by pixel frequency */
    long total = box_pixel_count(box, colors);
    long half = total / 2;
    long accum = 0;
    int split = 1; /* at least 1 in each half */
    for (int i = 0; i < box->count - 1; i++) {
        accum += colors[box->indices[i]].freq;
        if (accum >= half) {
            split = i + 1;
            break;
        }
    }
    /* Ensure at least 1 color in each box */
    if (split == 0) split = 1;
    if (split >= box->count) split = box->count - 1;

    new_box->indices = box->indices + split;
    new_box->count = box->count - split;
    box->count = split;

    box_compute_bounds(box, colors);
    box_compute_bounds(new_box, colors);

    return 1;
}

/* Find the box with the largest range on its longest axis (best to split) */
static int find_best_box(color_box_t *boxes, int nboxes, const ucolor_t *colors) {
    int best = -1;
    int best_range = -1;
    for (int i = 0; i < nboxes; i++) {
        if (boxes[i].count < 2) continue;
        int axis = box_longest_axis(&boxes[i]);
        int range;
        switch (axis) {
        case 0: range = boxes[i].r_max - boxes[i].r_min; break;
        case 1: range = boxes[i].g_max - boxes[i].g_min; break;
        case 2: range = boxes[i].b_max - boxes[i].b_min; break;
        default: range = 0;
        }
        /* Weight by pixel count to prefer splitting large populations */
        long pop = box_pixel_count(&boxes[i], colors);
        long score = (long)range * pop;
        if (score > best_range) {
            best_range = score;
            best = i;
        }
    }
    return best;
}

static int color_distance_sq(int r1, int g1, int b1, int r2, int g2, int b2) {
    int dr = r1 - r2, dg = g1 - g2, db = b1 - b2;
    return dr * dr + dg * dg + db * db;
}

static int find_nearest_color(unsigned char r, unsigned char g, unsigned char b,
                              const rgb_t *palette, int start, int num_colors) {
    int best = start;
    int best_dist = color_distance_sq(r, g, b,
                                      palette[start].r, palette[start].g, palette[start].b);
    for (int i = start + 1; i < num_colors; i++) {
        int d = color_distance_sq(r, g, b,
                                  palette[i].r, palette[i].g, palette[i].b);
        if (d < best_dist) {
            best_dist = d;
            best = i;
        }
    }
    return best;
}

int quantize_median_cut(
    const rgba_t *pixels,
    int width, int height,
    int max_colors,
    rgb_t *out_palette,
    unsigned char *out_indices)
{
    int npixels = width * height;
    if (max_colors < 2) max_colors = 2;
    if (max_colors > 256) max_colors = 256;

    /* Detect transparency */
    int has_alpha = 0;
    for (int i = 0; i < npixels; i++) {
        if (pixels[i].a < 128) {
            has_alpha = 1;
            break;
        }
    }

    int opaque_colors = has_alpha ? max_colors - 1 : max_colors;
    int first_opaque = has_alpha ? 1 : 0;

    if (opaque_colors < 1) opaque_colors = 1;

    /* Build unique-color histogram (only opaque pixels) */
    /* Use a simple hash table for dedup */
    #define HASH_SIZE 65536
    typedef struct hnode { unsigned char r, g, b; int freq; int idx; struct hnode *next; } hnode;
    hnode **htable = calloc(HASH_SIZE, sizeof(hnode *));
    ucolor_t *ucolors = NULL;
    int nucolors = 0;
    int ucap = 4096;
    ucolors = malloc(ucap * sizeof(ucolor_t));

    for (int i = 0; i < npixels; i++) {
        if (pixels[i].a < 128) continue;
        unsigned char r = pixels[i].r, g = pixels[i].g, b = pixels[i].b;
        unsigned int h = ((unsigned int)r * 73856093u ^ (unsigned int)g * 19349669u
                          ^ (unsigned int)b * 83492791u) % HASH_SIZE;
        hnode *n = htable[h];
        while (n) {
            if (n->r == r && n->g == g && n->b == b) {
                n->freq++;
                ucolors[n->idx].freq++;
                break;
            }
            n = n->next;
        }
        if (!n) {
            if (nucolors >= ucap) {
                ucap *= 2;
                ucolors = realloc(ucolors, ucap * sizeof(ucolor_t));
            }
            ucolors[nucolors].r = r;
            ucolors[nucolors].g = g;
            ucolors[nucolors].b = b;
            ucolors[nucolors].freq = 1;

            hnode *nn = malloc(sizeof(hnode));
            nn->r = r; nn->g = g; nn->b = b;
            nn->freq = 1; nn->idx = nucolors;
            nn->next = htable[h];
            htable[h] = nn;

            nucolors++;
        }
    }

    /* Free hash table */
    for (int i = 0; i < HASH_SIZE; i++) {
        hnode *n = htable[i];
        while (n) { hnode *tmp = n->next; free(n); n = tmp; }
    }
    free(htable);

    /* If fewer unique colors than target, just use them all */
    int final_colors;
    if (nucolors <= opaque_colors) {
        final_colors = nucolors + first_opaque;
        if (has_alpha) {
            out_palette[0].r = out_palette[0].g = out_palette[0].b = 0;
        }
        for (int i = 0; i < nucolors; i++) {
            out_palette[first_opaque + i].r = ucolors[i].r;
            out_palette[first_opaque + i].g = ucolors[i].g;
            out_palette[first_opaque + i].b = ucolors[i].b;
        }
    } else {
        /* Median-cut: allocate index array for boxes */
        int *idx_pool = malloc(nucolors * sizeof(int));
        for (int i = 0; i < nucolors; i++) idx_pool[i] = i;

        /* Allocate boxes (max opaque_colors) */
        color_box_t *boxes = calloc(opaque_colors, sizeof(color_box_t));
        boxes[0].indices = idx_pool;
        boxes[0].count = nucolors;
        box_compute_bounds(&boxes[0], ucolors);
        int nboxes = 1;

        /* Split until we have enough boxes */
        while (nboxes < opaque_colors) {
            int best = find_best_box(boxes, nboxes, ucolors);
            if (best < 0) break; /* no splittable box */

            color_box_t new_box;
            if (!box_split(&boxes[best], &new_box, ucolors))
                break;

            boxes[nboxes] = new_box;
            nboxes++;
        }

        /* Extract palette from boxes */
        if (has_alpha) {
            out_palette[0].r = out_palette[0].g = out_palette[0].b = 0;
        }
        for (int i = 0; i < nboxes; i++) {
            out_palette[first_opaque + i] = box_average(&boxes[i], ucolors);
        }
        final_colors = first_opaque + nboxes;

        free(boxes);
        free(idx_pool);
    }

    free(ucolors);

    /* Map each pixel to the nearest palette color */
    for (int i = 0; i < npixels; i++) {
        if (has_alpha && pixels[i].a < 128) {
            out_indices[i] = 0;
        } else {
            out_indices[i] = (unsigned char)find_nearest_color(
                pixels[i].r, pixels[i].g, pixels[i].b,
                out_palette, first_opaque, final_colors);
        }
    }

    return final_colors;
}

int quantize_map_to_palette(
    const rgba_t *pixels,
    int width, int height,
    const rgb_t *palette,
    int num_colors,
    unsigned char *out_indices)
{
    int npixels = width * height;
    int has_alpha = 0;

    for (int i = 0; i < npixels; i++) {
        if (pixels[i].a < 128) {
            has_alpha = 1;
            break;
        }
    }

    int start = has_alpha ? 1 : 0;

    for (int i = 0; i < npixels; i++) {
        if (has_alpha && pixels[i].a < 128) {
            out_indices[i] = 0;
        } else {
            out_indices[i] = (unsigned char)find_nearest_color(
                pixels[i].r, pixels[i].g, pixels[i].b,
                palette, start, num_colors);
        }
    }

    return 0;
}
