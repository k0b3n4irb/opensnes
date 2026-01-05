/**
 * @file font2snes.c
 * @brief Convert PNG font images to SNES tile format
 *
 * font2snes - A simple font converter for SNES development
 *
 * Input: 128x48 PNG image (16 columns x 6 rows of 8x8 characters)
 * Output: C header file or binary .pic/.pal files
 *
 * License: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>

/* Configure stb_image for PNG only */
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION

/* Suppress warnings for unused functions in stb_image.h */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "stb_image.h"
#pragma GCC diagnostic pop

#include "tiles.h"
#include "output.h"

#define VERSION "1.0.0"

/* Character dimensions */
#define CHAR_SIZE    8     /* 8x8 pixels per character */
#define TOTAL_CHARS  96    /* ASCII 32-127 */

static void print_usage(const char *program)
{
    printf("font2snes v%s - SNES font converter\n\n", VERSION);
    printf("Usage: %s [options] input.png output\n\n", program);
    printf("Converts a PNG font image to SNES tile format.\n\n");
    printf("Options:\n");
    printf("  -b, --bpp <2|4>    Bits per pixel (default: 2)\n");
    printf("  -c, --c-header     Output as C header instead of binary\n");
    printf("  -v, --verbose      Verbose output\n");
    printf("  -h, --help         Show this help message\n");
    printf("  -V, --version      Show version\n\n");
    printf("Font Image Layouts (auto-detected):\n");
    printf("  - 128x48 pixels: 16 cols x 6 rows of 8x8 chars (recommended)\n");
    printf("  - 768x8 pixels:  96 chars in a single row\n");
    printf("  - Any NxM where N*M/64 = 96 characters\n\n");
    printf("Requirements:\n");
    printf("  - Dimensions must be multiples of 8\n");
    printf("  - Must contain exactly 96 characters (ASCII 32-127)\n\n");
    printf("Examples:\n");
    printf("  %s -c myfont.png myfont.h      # C header output\n", program);
    printf("  %s myfont.png myfont.pic       # Binary output\n", program);
    printf("  %s -b 4 -c font.png font.h     # 4bpp C header\n", program);
}

static void print_version(void)
{
    printf("font2snes v%s\n", VERSION);
    printf("SNES font converter for OpenSNES\n");
    printf("License: MIT\n");
}

/* Extract base name without extension */
static void get_basename(const char *path, char *name, size_t size)
{
    char *tmp = strdup(path);
    char *base = basename(tmp);
    char *dot;

    strncpy(name, base, size - 1);
    name[size - 1] = '\0';

    /* Remove extension */
    dot = strrchr(name, '.');
    if (dot) {
        *dot = '\0';
    }

    free(tmp);
}

/* Replace extension in path */
static void replace_extension(const char *path, const char *new_ext,
                             char *output, size_t size)
{
    const char *dot = strrchr(path, '.');

    if (dot) {
        size_t base_len = (size_t)(dot - path);
        if (base_len >= size - 1) base_len = size - 1;
        strncpy(output, path, base_len);
        output[base_len] = '\0';
    } else {
        strncpy(output, path, size - 1);
        output[size - 1] = '\0';
    }

    strncat(output, new_ext, size - strlen(output) - 1);
}

int main(int argc, char *argv[])
{
    const char *input_path = NULL;
    const char *output_path = NULL;
    int bpp = 2;
    int c_header = 0;
    int verbose = 0;

    unsigned char *img_data = NULL;
    int img_width, img_height, img_channels;

    font_data_t font;
    int bytes_per_tile;
    int i, row, col;
    int result = 0;

    /* Command line options */
    static struct option long_options[] = {
        {"bpp",      required_argument, 0, 'b'},
        {"c-header", no_argument,       0, 'c'},
        {"verbose",  no_argument,       0, 'v'},
        {"help",     no_argument,       0, 'h'},
        {"version",  no_argument,       0, 'V'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "b:cvhV", long_options, NULL)) != -1) {
        switch (opt) {
            case 'b':
                bpp = atoi(optarg);
                if (bpp != 2 && bpp != 4) {
                    fprintf(stderr, "Error: BPP must be 2 or 4\n");
                    return 1;
                }
                break;
            case 'c':
                c_header = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'V':
                print_version();
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* Check positional arguments */
    if (optind + 2 > argc) {
        fprintf(stderr, "Error: Missing input or output file\n\n");
        print_usage(argv[0]);
        return 1;
    }

    input_path = argv[optind];
    output_path = argv[optind + 1];

    if (verbose) {
        printf("Input:  %s\n", input_path);
        printf("Output: %s\n", output_path);
        printf("BPP:    %d\n", bpp);
        printf("Format: %s\n", c_header ? "C header" : "binary");
    }

    /* Load PNG image */
    if (verbose) {
        printf("Loading %s...\n", input_path);
    }

    img_data = stbi_load(input_path, &img_width, &img_height, &img_channels, 0);
    if (!img_data) {
        fprintf(stderr, "Error: Failed to load %s: %s\n",
                input_path, stbi_failure_reason());
        return 1;
    }

    if (verbose) {
        printf("Image: %dx%d, %d channels\n", img_width, img_height, img_channels);
    }

    /* Validate dimensions */
    int chars_per_row = img_width / CHAR_SIZE;
    int char_rows = img_height / CHAR_SIZE;
    int total_chars = chars_per_row * char_rows;

    if (img_width % CHAR_SIZE != 0 || img_height % CHAR_SIZE != 0) {
        fprintf(stderr, "Error: Image dimensions must be multiples of %d (got %dx%d)\n",
                CHAR_SIZE, img_width, img_height);
        stbi_image_free(img_data);
        return 1;
    }

    if (total_chars != TOTAL_CHARS) {
        fprintf(stderr, "Error: Image must contain exactly %d characters\n", TOTAL_CHARS);
        fprintf(stderr, "       Got %dx%d = %d chars per row x %d rows = %d chars\n",
                img_width, img_height, chars_per_row, char_rows, total_chars);
        fprintf(stderr, "       Supported layouts: 128x48, 768x8, or any %d-char grid\n",
                TOTAL_CHARS);
        stbi_image_free(img_data);
        return 1;
    }

    if (verbose) {
        printf("Layout: %d cols x %d rows of 8x8 characters\n", chars_per_row, char_rows);
    }

    /* Allocate font data */
    bytes_per_tile = (bpp == 2) ? 16 : 32;
    font.tiles = (uint8_t *)malloc(TOTAL_CHARS * bytes_per_tile);
    if (!font.tiles) {
        fprintf(stderr, "Error: Out of memory\n");
        stbi_image_free(img_data);
        return 1;
    }

    font.tile_count = TOTAL_CHARS;
    font.bytes_per_tile = bytes_per_tile;
    font.bpp = bpp;

    /* Get base name for output */
    {
        static char name_buf[64];
        get_basename(output_path, name_buf, sizeof(name_buf));
        font.name = name_buf;
    }

    if (verbose) {
        printf("Converting %d characters to %dbpp format...\n", TOTAL_CHARS, bpp);
    }

    /* Convert each character */
    for (i = 0; i < TOTAL_CHARS; i++) {
        uint8_t indexed[64];  /* 8x8 indexed pixels */
        int char_col = i % chars_per_row;
        int char_row_idx = i / chars_per_row;
        int base_x = char_col * CHAR_SIZE;
        int base_y = char_row_idx * CHAR_SIZE;

        /* Extract 8x8 tile from image */
        for (row = 0; row < CHAR_SIZE; row++) {
            for (col = 0; col < CHAR_SIZE; col++) {
                int px = base_x + col;
                int py = base_y + row;
                int pixel_idx = (py * img_width + px) * img_channels;

                /* Convert to palette index based on brightness */
                /* For indexed PNG: use first channel as index */
                /* For RGB: quantize to palette index */
                if (img_channels == 1) {
                    /* Grayscale - use as palette index */
                    indexed[row * 8 + col] = img_data[pixel_idx];
                } else if (img_channels >= 3) {
                    /* RGB - compute brightness and quantize */
                    int r = img_data[pixel_idx];
                    int g = img_data[pixel_idx + 1];
                    int b = img_data[pixel_idx + 2];
                    int brightness = (r + g + b) / 3;

                    if (bpp == 2) {
                        /* 4 colors: 0, 1, 2, 3 */
                        indexed[row * 8 + col] = (uint8_t)(brightness / 64);
                    } else {
                        /* 16 colors: 0-15 */
                        indexed[row * 8 + col] = (uint8_t)(brightness / 16);
                    }
                } else {
                    indexed[row * 8 + col] = 0;
                }
            }
        }

        /* Convert to SNES format */
        if (bpp == 2) {
            convert_tile_2bpp(indexed, &font.tiles[i * bytes_per_tile]);
        } else {
            convert_tile_4bpp(indexed, &font.tiles[i * bytes_per_tile]);
        }
    }

    /* Set up default palette (grayscale) */
    font.palette_count = (bpp == 2) ? 4 : 16;
    for (i = 0; i < font.palette_count; i++) {
        int level;
        if (bpp == 2) {
            level = (i * 255) / 3;  /* 0, 85, 170, 255 */
        } else {
            level = (i * 255) / 15; /* 0, 17, 34, ... 255 */
        }
        font.palette[i] = rgb_to_bgr555((uint8_t)level, (uint8_t)level, (uint8_t)level);
    }

    /* Write output */
    if (c_header) {
        result = output_c_header(&font, output_path);
    } else {
        char pal_path[256];

        result = output_binary_tiles(&font, output_path);
        if (result == 0) {
            replace_extension(output_path, ".pal", pal_path, sizeof(pal_path));
            result = output_binary_palette(&font, pal_path);
            if (result == 0 && verbose) {
                printf("Palette written to %s\n", pal_path);
            }
        }
    }

    if (result == 0) {
        printf("Converted %d characters to %dbpp format\n", TOTAL_CHARS, bpp);
        printf("Output: %s (%d bytes)\n", output_path,
               TOTAL_CHARS * bytes_per_tile);
    }

    /* Cleanup */
    free(font.tiles);
    stbi_image_free(img_data);

    return (result == 0) ? 0 : 1;
}
