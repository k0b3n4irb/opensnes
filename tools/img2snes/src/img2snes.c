/*
 * img2snes - PNG RGB/RGBA to indexed PNG converter for OpenSNES
 *
 * Preprocesses PNG images for gfx4snes by converting RGB/RGBA PNGs
 * to palette-indexed PNGs using median-cut color quantization.
 *
 * Copyright (c) 2024-2026 OpenSNES contributors
 * Licensed under the MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cmdparser.h"
#include "lodepng.h"
#include "quantize.h"
#include "scale.h"

#define IMG2SNES_VERSION "1.0.0"

/* CLI options */
static char *opt_input    = NULL;
static char *opt_output   = NULL;
static int   opt_colors   = 16;
static double opt_scale   = 1.0;
static char *opt_palette  = NULL;
static int   opt_tile     = 0;
static bool  opt_round    = false;
static bool  opt_batch    = false;
static bool  opt_quiet    = false;
static bool  opt_verbose  = false;
static bool  opt_version  = false;

/* Round an RGB component to SNES BGR555 precision (multiples of 8) */
static unsigned char round_snes(unsigned char v) {
    int r = ((int)v + 4) / 8 * 8;
    if (r > 248) r = 248;
    return (unsigned char)r;
}

/* Generate default output filename: input_indexed.png */
static char *make_output_name(const char *input) {
    size_t len = strlen(input);
    /* Find the last dot */
    const char *dot = strrchr(input, '.');
    size_t base_len = dot ? (size_t)(dot - input) : len;

    /* _indexed.png = 12 chars + null */
    char *out = malloc(base_len + 13);
    if (!out) return NULL;
    memcpy(out, input, base_len);
    strcpy(out + base_len, "_indexed.png");
    return out;
}

/* Load a PNG as RGBA 8-bit. Returns 0 on success. */
static int load_png_rgba(const char *filename,
                         unsigned char **out_pixels,
                         unsigned *out_w, unsigned *out_h) {
    unsigned error = lodepng_decode32_file(out_pixels, out_w, out_h, filename);
    if (error) {
        fprintf(stderr, "Error loading %s: %s\n", filename, lodepng_error_text(error));
        return 1;
    }
    return 0;
}

/* Extract palette from an existing indexed or RGBA PNG.
 * Returns number of colors extracted, or -1 on error. */
static int load_palette_from_png(const char *filename,
                                 rgb_t *palette, int max_colors) {
    LodePNGState state;
    lodepng_state_init(&state);
    /* Request palette mode to get native palette if available */
    state.info_raw.colortype = LCT_RGBA;
    state.info_raw.bitdepth = 8;

    unsigned char *pngbuf = NULL;
    size_t pngsize = 0;
    unsigned error = lodepng_load_file(&pngbuf, &pngsize, filename);
    if (error) {
        fprintf(stderr, "Error loading palette file %s: %s\n",
                filename, lodepng_error_text(error));
        lodepng_state_cleanup(&state);
        return -1;
    }

    unsigned char *pixels = NULL;
    unsigned w, h;
    error = lodepng_decode(&pixels, &w, &h, &state, pngbuf, pngsize);
    free(pngbuf);
    if (error) {
        fprintf(stderr, "Error decoding palette file %s: %s\n",
                filename, lodepng_error_text(error));
        lodepng_state_cleanup(&state);
        return -1;
    }

    int ncolors = 0;

    /* If the source was a palette PNG, extract its palette directly */
    if (state.info_png.color.colortype == LCT_PALETTE &&
        state.info_png.color.palettesize > 0) {
        ncolors = (int)state.info_png.color.palettesize;
        if (ncolors > max_colors) ncolors = max_colors;
        for (int i = 0; i < ncolors; i++) {
            palette[i].r = state.info_png.color.palette[i * 4 + 0];
            palette[i].g = state.info_png.color.palette[i * 4 + 1];
            palette[i].b = state.info_png.color.palette[i * 4 + 2];
        }
    } else {
        /* Extract unique colors from the RGBA image */
        int npixels = (int)(w * h);
        for (int i = 0; i < npixels && ncolors < max_colors; i++) {
            unsigned char r = pixels[i * 4 + 0];
            unsigned char g = pixels[i * 4 + 1];
            unsigned char b = pixels[i * 4 + 2];
            unsigned char a = pixels[i * 4 + 3];
            if (a < 128) continue;

            /* Check if already in palette */
            int found = 0;
            for (int j = 0; j < ncolors; j++) {
                if (palette[j].r == r && palette[j].g == g && palette[j].b == b) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                palette[ncolors].r = r;
                palette[ncolors].g = g;
                palette[ncolors].b = b;
                ncolors++;
            }
        }
    }

    free(pixels);
    lodepng_state_cleanup(&state);
    return ncolors;
}

/* Encode an indexed image as PNG using lodepng state API.
 * Returns 0 on success. */
static int save_indexed_png(const char *filename,
                            const unsigned char *indices,
                            int width, int height,
                            const rgb_t *palette, int num_colors,
                            int has_alpha) {
    LodePNGState state;
    lodepng_state_init(&state);

    /* Disable auto_convert — we provide exact palette data */
    state.encoder.auto_convert = 0;

    /* Set output format: palette, 8-bit */
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = 8;

    /* Set raw input format: palette, 8-bit (indices are our pixel data) */
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;

    /* Add palette entries to both info_png and info_raw */
    for (int i = 0; i < num_colors; i++) {
        unsigned char a = (has_alpha && i == 0) ? 0 : 255;
        lodepng_palette_add(&state.info_png.color,
                            palette[i].r, palette[i].g, palette[i].b, a);
        lodepng_palette_add(&state.info_raw,
                            palette[i].r, palette[i].g, palette[i].b, a);
    }

    unsigned char *outbuf = NULL;
    size_t outsize = 0;
    unsigned error = lodepng_encode(&outbuf, &outsize,
                                    indices, (unsigned)width, (unsigned)height,
                                    &state);
    if (error) {
        fprintf(stderr, "Error encoding %s: %s\n",
                filename, lodepng_error_text(error));
        lodepng_state_cleanup(&state);
        return 1;
    }

    error = lodepng_save_file(outbuf, outsize, filename);
    free(outbuf);
    lodepng_state_cleanup(&state);

    if (error) {
        fprintf(stderr, "Error saving %s: %s\n",
                filename, lodepng_error_text(error));
        return 1;
    }

    return 0;
}

/* Process a single image through the full pipeline */
static int process_image(const char *input_path, const char *output_path) {
    unsigned char *raw_pixels = NULL;
    unsigned w, h;

    if (!opt_quiet) {
        printf("Processing: %s\n", input_path);
    }

    /* Step 1: Load PNG as RGBA */
    if (load_png_rgba(input_path, &raw_pixels, &w, &h) != 0)
        return 1;

    if (opt_verbose) {
        printf("  Input: %ux%u RGBA\n", w, h);
    }

    rgba_t *pixels = (rgba_t *)raw_pixels;
    int cur_w = (int)w, cur_h = (int)h;

    /* Step 2: Scale if requested */
    rgba_t *scaled = NULL;
    if (opt_scale != 1.0 || opt_tile > 0) {
        int dst_w, dst_h;
        compute_dimensions(cur_w, cur_h, opt_scale, opt_tile, &dst_w, &dst_h);

        if (dst_w != cur_w || dst_h != cur_h) {
            scaled = scale_nearest(pixels, cur_w, cur_h, dst_w, dst_h);
            if (!scaled) {
                fprintf(stderr, "Error: scaling failed (out of memory)\n");
                free(raw_pixels);
                return 1;
            }
            if (opt_verbose) {
                printf("  Scaled: %dx%d -> %dx%d", cur_w, cur_h, dst_w, dst_h);
                if (opt_tile > 0)
                    printf(" (tile-aligned to %d)", opt_tile);
                printf("\n");
            }
            pixels = scaled;
            cur_w = dst_w;
            cur_h = dst_h;
        }
    }

    /* Step 3: Quantize or map to reference palette */
    int num_colors = opt_colors;
    rgb_t palette[256];
    unsigned char *indices = malloc((size_t)cur_w * cur_h);
    if (!indices) {
        fprintf(stderr, "Error: out of memory\n");
        free(scaled);
        free(raw_pixels);
        return 1;
    }

    int has_alpha = 0;
    for (int i = 0; i < cur_w * cur_h; i++) {
        if (pixels[i].a < 128) { has_alpha = 1; break; }
    }

    if (opt_palette) {
        /* Load external palette and map pixels to it */
        num_colors = load_palette_from_png(opt_palette, palette, opt_colors);
        if (num_colors < 0) {
            free(indices);
            free(scaled);
            free(raw_pixels);
            return 1;
        }
        if (opt_verbose) {
            printf("  Reference palette: %d colors from %s\n", num_colors, opt_palette);
        }
        quantize_map_to_palette(pixels, cur_w, cur_h,
                                palette, num_colors, indices);
    } else {
        /* Median-cut quantization */
        num_colors = quantize_median_cut(pixels, cur_w, cur_h,
                                         opt_colors, palette, indices);
        if (opt_verbose) {
            printf("  Quantized: %d colors (target: %d)\n", num_colors, opt_colors);
        }
    }

    /* Step 4: Round palette to SNES BGR555 */
    if (opt_round) {
        int start = has_alpha ? 1 : 0;
        for (int i = start; i < num_colors; i++) {
            palette[i].r = round_snes(palette[i].r);
            palette[i].g = round_snes(palette[i].g);
            palette[i].b = round_snes(palette[i].b);
        }
        if (opt_verbose) {
            printf("  Palette rounded to SNES BGR555 precision\n");
        }
    }

    /* Step 5: Encode indexed PNG */
    if (save_indexed_png(output_path, indices, cur_w, cur_h,
                         palette, num_colors, has_alpha) != 0) {
        free(indices);
        free(scaled);
        free(raw_pixels);
        return 1;
    }

    if (!opt_quiet) {
        printf("  Output: %s (%dx%d, %d colors%s)\n",
               output_path, cur_w, cur_h, num_colors,
               has_alpha ? ", transparent" : "");
    }

    free(indices);
    free(scaled);
    free(raw_pixels);
    return 0;
}

/* CLI callback */
static cmdp_action_t cmd_process(cmdp_process_param_st *params) {
    if (opt_version) {
        printf("img2snes %s (%s)\n", IMG2SNES_VERSION, __BUILD_DATE);
        return CMDP_ACT_OK;
    }

    if (!opt_input) {
        fprintf(stderr, "Error: input file required (-i)\n");
        return CMDP_ACT_ERROR | CMDP_ACT_SHOW_HELP;
    }

    /* Apply defaults for values that cmdparser zeroed */
    if (opt_colors == 0) opt_colors = 16;
    if (opt_scale == 0.0) opt_scale = 1.0;

    if (opt_colors < 2 || opt_colors > 256) {
        fprintf(stderr, "Error: --colors must be between 2 and 256\n");
        return CMDP_ACT_ERROR;
    }

    if (opt_scale <= 0.0) {
        fprintf(stderr, "Error: --scale must be positive\n");
        return CMDP_ACT_ERROR;
    }

    if (opt_batch) {
        fprintf(stderr, "Error: --batch mode not yet implemented\n");
        return CMDP_ACT_ERROR;
    }

    /* Determine output path */
    char *auto_output = NULL;
    const char *output_path = opt_output;
    if (!output_path) {
        auto_output = make_output_name(opt_input);
        if (!auto_output) {
            fprintf(stderr, "Error: out of memory\n");
            return CMDP_ACT_ERROR;
        }
        output_path = auto_output;
    }

    int result = process_image(opt_input, output_path);
    free(auto_output);

    return result == 0 ? CMDP_ACT_OK : CMDP_ACT_ERROR;
}

static cmdp_option_st options[] = {
    { 'i', "input",      "Input PNG file (RGB, RGBA, or indexed)",
      CMDP_TYPE_STRING_PTR, &opt_input, "<file>" },
    { 'o', "output",     "Output PNG file (default: <input>_indexed.png)",
      CMDP_TYPE_STRING_PTR, &opt_output, "<file>" },
    { 'c', "colors",     "Number of colors (2-256, default: 16)",
      CMDP_TYPE_INT4, &opt_colors, "<n>" },
    { 's', "scale",      "Scale factor (default: 1.0)",
      CMDP_TYPE_DOUBLE, &opt_scale, "<factor>" },
    { 'p', "palette",    "Reference PNG to extract palette from",
      CMDP_TYPE_STRING_PTR, &opt_palette, "<file>" },
    { 't', "tile",       "Align dimensions to multiple of N (e.g. 8)",
      CMDP_TYPE_INT4, &opt_tile, "<n>" },
    { 0,   "round-snes", "Round palette to SNES BGR555 (multiples of 8)",
      CMDP_TYPE_BOOL, &opt_round },
    { 0,   "batch",      "Treat input as glob pattern (batch mode)",
      CMDP_TYPE_BOOL, &opt_batch },
    { 'q', "quiet",      "Suppress output messages",
      CMDP_TYPE_BOOL, &opt_quiet },
    { 'v', "verbose",    "Show detailed processing info",
      CMDP_TYPE_BOOL, &opt_verbose },
    { 'V', "version",    "Show version and exit",
      CMDP_TYPE_BOOL, &opt_version },
    {0},
};

static cmdp_command_st root = {
    .name = "img2snes",
    .desc = "Convert RGB/RGBA PNG to indexed PNG for gfx4snes",
    .doc = "Usage: img2snes -i <input.png> [options]\n\n"
           "Converts RGB/RGBA PNG images to palette-indexed PNGs\n"
           "suitable for gfx4snes (OpenSNES graphics converter).\n\n"
           "Options:\n",
    .options = options,
    .fn_process = cmd_process,
};

int main(int argc, char **argv) {
    if (argc < 2) {
        cmdp_help(&root);
        return 1;
    }
    /* Pass explicit context to avoid dangling-pointer issue at -O2 */
    cmdp_ctx ctx = {0};
    cmdp_set_default_context(&ctx);
    return cmdp_run(argc - 1, argv + 1, &root, &ctx);
}
