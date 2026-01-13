/**
 * @file gfx2snes.c
 * @brief Convert PNG/BMP images to SNES tile format
 *
 * Follows pvsneslib's gfx4snes approach:
 * 1. Reorganize image to 128 pixels wide (VRAM layout)
 * 2. Extract tiles linearly from reorganized image
 * 3. Convert to SNES bitplane format
 *
 * License: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <libgen.h>
#include <ctype.h>

#include "loadbmp.h"

#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "stb_image.h"
#pragma GCC diagnostic pop

#define VERSION "1.1.0"

/* SNES VRAM is organized as 128 pixels (16 tiles) wide */
#define VRAM_WIDTH_PIXELS 128
#define TILE_SIZE 8

/* SNES color conversion: RGB to BGR555
 * SNES format: 0BBBBBGG GGGRRRRR (Blue high, Red low)
 */
#define RGB_TO_BGR555(r, g, b) \
    (((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10))

typedef struct {
    uint8_t r, g, b;
} Color;

typedef struct {
    Color colors[256];
    int count;
} Palette;

typedef struct {
    int bpp;            /* 2 or 4 */
    int block_size;     /* 8, 16, or 32 (sprite/block size) */
    int c_header;       /* Output as C header */
    int verbose;
    const char *input;
    const char *output;
    const char *name;
} Options;

static Options opts = {
    .bpp = 4,
    .block_size = 8,
    .c_header = 0,
    .verbose = 0
};

/*----------------------------------------------------------------------------*/
/* Palette functions                                                          */
/*----------------------------------------------------------------------------*/

static int color_distance(Color a, Color b)
{
    int dr = (int)a.r - (int)b.r;
    int dg = (int)a.g - (int)b.g;
    int db = (int)a.b - (int)b.b;
    return dr*dr + dg*dg + db*db;
}

static int find_color(Palette *pal, Color c)
{
    for (int i = 0; i < pal->count; i++) {
        if (color_distance(pal->colors[i], c) < 16) {
            return i;
        }
    }
    return -1;
}

static int add_color(Palette *pal, Color c, int max_colors)
{
    int idx = find_color(pal, c);
    if (idx >= 0) return idx;

    if (pal->count >= max_colors) {
        return -1;
    }

    pal->colors[pal->count] = c;
    return pal->count++;
}

/**
 * Check if filename has .bmp extension (case insensitive)
 */
static int is_bmp_file(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot) return 0;

    if ((dot[1] == 'b' || dot[1] == 'B') &&
        (dot[2] == 'm' || dot[2] == 'M') &&
        (dot[3] == 'p' || dot[3] == 'P') &&
        dot[4] == '\0') {
        return 1;
    }
    return 0;
}

static int build_palette(const uint8_t *img, int w, int h, int channels,
                         Palette *pal, int max_colors)
{
    pal->count = 0;

    /* First color is transparent (color 0) */
    Color first = {img[0], img[1], img[2]};
    add_color(pal, first, max_colors);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * channels;
            Color c = {img[idx], img[idx+1], img[idx+2]};

            if (add_color(pal, c, max_colors) < 0) {
                fprintf(stderr, "Error: Image has more than %d colors\n", max_colors);
                return -1;
            }
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/* Image reorganization (like pvsneslib's tiles_convertsnes)                  */
/*----------------------------------------------------------------------------*/

/**
 * Reorganize image to VRAM-compatible layout (128 pixels wide).
 * Copies blocks of block_size x block_size pixels, wrapping to next row
 * when the output row is full.
 *
 * @param img       Source indexed image
 * @param w         Source width
 * @param h         Source height
 * @param block_size Block size in pixels (8, 16, or 32)
 * @param out_w     Output: new width
 * @param out_h     Output: new height
 * @return          Newly allocated reorganized buffer (caller must free)
 */
static uint8_t *reorganize_for_vram(const uint8_t *img, int w, int h,
                                     int block_size, int *out_w, int *out_h)
{
    int blocks_x = w / block_size;
    int blocks_y = h / block_size;
    int total_blocks = blocks_x * blocks_y;

    /* Calculate output dimensions */
    int new_width = VRAM_WIDTH_PIXELS;
    int blocks_per_row = new_width / block_size;
    int new_rows = (total_blocks + blocks_per_row - 1) / blocks_per_row;
    int new_height = new_rows * block_size;

    if (opts.verbose) {
        printf("Reorganizing: %dx%d -> %dx%d (%d blocks)\n",
               w, h, new_width, new_height, total_blocks);
    }

    uint8_t *buffer = calloc(new_width * new_height, 1);
    if (!buffer) return NULL;

    /* Copy blocks to new positions */
    int dst_x = 0, dst_y = 0;

    for (int by = 0; by < blocks_y; by++) {
        for (int bx = 0; bx < blocks_x; bx++) {
            /* Copy each line of the block */
            for (int line = 0; line < block_size; line++) {
                int src_offset = (by * block_size + line) * w + (bx * block_size);
                int dst_offset = (dst_y + line) * new_width + dst_x;

                memcpy(&buffer[dst_offset], &img[src_offset], block_size);
            }

            /* Move to next block position */
            dst_x += block_size;
            if (dst_x >= new_width) {
                dst_x = 0;
                dst_y += block_size;
            }
        }
    }

    *out_w = new_width;
    *out_h = new_height;
    return buffer;
}

/*----------------------------------------------------------------------------*/
/* Tile conversion to SNES bitplane format                                    */
/*----------------------------------------------------------------------------*/

/**
 * Convert 8x8 tile from indexed to SNES 2bpp format (16 bytes).
 * Layout: rows 0-7, each row has bitplane 0 then bitplane 1.
 */
static void convert_tile_2bpp(const uint8_t *indexed, uint8_t *snes)
{
    for (int row = 0; row < 8; row++) {
        uint8_t bp0 = 0, bp1 = 0;
        for (int col = 0; col < 8; col++) {
            uint8_t pixel = indexed[row * 8 + col] & 0x03;
            bp0 = (bp0 << 1) | ((pixel >> 0) & 1);
            bp1 = (bp1 << 1) | ((pixel >> 1) & 1);
        }
        snes[row * 2 + 0] = bp0;
        snes[row * 2 + 1] = bp1;
    }
}

/**
 * Convert 8x8 tile from indexed to SNES 4bpp format (32 bytes).
 * Layout: rows 0-7 with bp0,bp1 (16 bytes), then rows 0-7 with bp2,bp3 (16 bytes).
 */
static void convert_tile_4bpp(const uint8_t *indexed, uint8_t *snes)
{
    for (int row = 0; row < 8; row++) {
        uint8_t bp0 = 0, bp1 = 0, bp2 = 0, bp3 = 0;
        for (int col = 0; col < 8; col++) {
            uint8_t pixel = indexed[row * 8 + col] & 0x0F;
            bp0 = (bp0 << 1) | ((pixel >> 0) & 1);
            bp1 = (bp1 << 1) | ((pixel >> 1) & 1);
            bp2 = (bp2 << 1) | ((pixel >> 2) & 1);
            bp3 = (bp3 << 1) | ((pixel >> 3) & 1);
        }
        snes[row * 2 + 0] = bp0;
        snes[row * 2 + 1] = bp1;
        snes[16 + row * 2 + 0] = bp2;
        snes[16 + row * 2 + 1] = bp3;
    }
}

/*----------------------------------------------------------------------------*/
/* Output functions                                                           */
/*----------------------------------------------------------------------------*/

static void write_c_header(FILE *f, const char *name,
                           const uint8_t *tiles, int tile_count, int tile_size,
                           const Palette *pal)
{
    fprintf(f, "/* Generated by gfx2snes */\n\n");
    fprintf(f, "#ifndef %s_H\n", name);
    fprintf(f, "#define %s_H\n\n", name);

    /* Palette */
    fprintf(f, "/* Palette: %d colors */\n", pal->count);
    fprintf(f, "const unsigned short %s_pal[%d] = {\n    ", name, pal->count);
    for (int i = 0; i < pal->count; i++) {
        uint16_t c = RGB_TO_BGR555(pal->colors[i].r, pal->colors[i].g, pal->colors[i].b);
        fprintf(f, "0x%04X", c);
        if (i < pal->count - 1) fprintf(f, ", ");
        if ((i + 1) % 8 == 0 && i < pal->count - 1) fprintf(f, "\n    ");
    }
    fprintf(f, "\n};\n\n");

    /* Tiles */
    int total_bytes = tile_count * tile_size;
    fprintf(f, "/* Tiles: %d tiles, %d bytes each */\n", tile_count, tile_size);
    fprintf(f, "const unsigned char %s_tiles[%d] = {\n", name, total_bytes);

    for (int t = 0; t < tile_count; t++) {
        fprintf(f, "    /* Tile %d */\n    ", t);
        for (int b = 0; b < tile_size; b++) {
            fprintf(f, "0x%02X", tiles[t * tile_size + b]);
            if (t < tile_count - 1 || b < tile_size - 1) fprintf(f, ",");
            if ((b + 1) % 16 == 0 && b < tile_size - 1) fprintf(f, "\n    ");
        }
        fprintf(f, "\n");
    }
    fprintf(f, "};\n\n");

    fprintf(f, "#define %s_TILES_COUNT %d\n", name, tile_count);
    fprintf(f, "#define %s_TILES_SIZE %d\n", name, total_bytes);
    fprintf(f, "#define %s_PAL_COUNT %d\n\n", name, pal->count);

    fprintf(f, "#endif /* %s_H */\n", name);
}

static void write_binary(const char *basename,
                         const uint8_t *tiles, int tile_count, int tile_size,
                         const Palette *pal)
{
    char filename[256];
    FILE *f;

    /* Write tiles */
    snprintf(filename, sizeof(filename), "%s.pic", basename);
    f = fopen(filename, "wb");
    if (f) {
        fwrite(tiles, 1, tile_count * tile_size, f);
        fclose(f);
        printf("Tiles: %s (%d bytes)\n", filename, tile_count * tile_size);
    }

    /* Write palette */
    snprintf(filename, sizeof(filename), "%s.pal", basename);
    f = fopen(filename, "wb");
    if (f) {
        for (int i = 0; i < pal->count; i++) {
            uint16_t c = RGB_TO_BGR555(pal->colors[i].r, pal->colors[i].g, pal->colors[i].b);
            fputc(c & 0xFF, f);
            fputc((c >> 8) & 0xFF, f);
        }
        fclose(f);
        printf("Palette: %s (%d bytes)\n", filename, pal->count * 2);
    }
}

/*----------------------------------------------------------------------------*/
/* Main conversion                                                            */
/*----------------------------------------------------------------------------*/

static int convert_image(void)
{
    int w, h;
    uint8_t *indexed = NULL;
    Palette pal = {0};
    int max_colors = (opts.bpp == 2) ? 4 : 16;
    int tile_size = (opts.bpp == 2) ? 16 : 32;

    if (is_bmp_file(opts.input)) {
        /* Load BMP with indexed palette preserved */
        unsigned char *bmp_data = NULL;
        size_t bmp_size = 0;
        unsigned err = bmp_load_file(&bmp_data, &bmp_size, opts.input);
        if (err) {
            fprintf(stderr, "Error: Cannot load '%s': %s\n", opts.input, bmp_error_text(err));
            return 1;
        }

        BMPState state;
        memset(&state, 0, sizeof(state));
        unsigned int uw, uh;
        err = bmp_decode(&indexed, &state, &uw, &uh, bmp_data, bmp_size);
        free(bmp_data);

        if (err) {
            fprintf(stderr, "Error: Cannot decode BMP '%s': %s\n", opts.input, bmp_error_text(err));
            return 1;
        }

        w = (int)uw;
        h = (int)uh;

        /* BMP palette has fixed size based on bit depth (e.g., 256 for 8-bit).
         * We need to extract only the actually-used colors and remap indices. */
        int bmp_palette_size = (int)state.info_bmp.palettesize;
        uint8_t color_used[256] = {0};
        uint8_t color_remap[256] = {0};

        /* Find which palette entries are actually used */
        for (int i = 0; i < w * h; i++) {
            color_used[indexed[i]] = 1;
        }

        /* Build our palette from only used colors, preserve order starting with index 0 */
        pal.count = 0;
        for (int i = 0; i < bmp_palette_size && i < 256; i++) {
            if (color_used[i]) {
                if (pal.count >= max_colors) {
                    fprintf(stderr, "Error: BMP uses more than %d colors (limit for %dbpp)\n",
                            max_colors, opts.bpp);
                    free(indexed);
                    return 1;
                }
                pal.colors[pal.count].r = state.info_bmp.palette[i].red;
                pal.colors[pal.count].g = state.info_bmp.palette[i].green;
                pal.colors[pal.count].b = state.info_bmp.palette[i].blue;
                color_remap[i] = pal.count;
                pal.count++;
            }
        }

        /* Remap pixel indices to our compact palette */
        for (int i = 0; i < w * h; i++) {
            indexed[i] = color_remap[indexed[i]];
        }

        if (opts.verbose) {
            printf("BMP Input: %dx%d, %d colors used (from %d palette entries), block size %d, %dbpp\n",
                   w, h, pal.count, bmp_palette_size, opts.block_size, opts.bpp);
        }
    } else {
        /* Load PNG with stb_image (converts to RGB) */
        int channels;
        uint8_t *img = stbi_load(opts.input, &w, &h, &channels, 3);

        if (!img) {
            fprintf(stderr, "Error: Cannot load '%s'\n", opts.input);
            return 1;
        }

        if (opts.verbose) {
            printf("PNG Input: %dx%d, block size %d, %dbpp\n", w, h, opts.block_size, opts.bpp);
        }

        /* Build palette from RGB image */
        if (build_palette(img, w, h, 3, &pal, max_colors) < 0) {
            stbi_image_free(img);
            return 1;
        }

        /* Convert RGB image to indexed */
        indexed = malloc(w * h);
        if (!indexed) {
            fprintf(stderr, "Error: Out of memory\n");
            stbi_image_free(img);
            return 1;
        }

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int idx = (y * w + x) * 3;
                Color c = {img[idx], img[idx+1], img[idx+2]};
                indexed[y * w + x] = find_color(&pal, c);
            }
        }
        stbi_image_free(img);
    }

    /* Validate dimensions */
    if (w % opts.block_size != 0 || h % opts.block_size != 0) {
        fprintf(stderr, "Error: Image dimensions (%dx%d) must be multiple of block size (%d)\n",
                w, h, opts.block_size);
        free(indexed);
        return 1;
    }

    /* Reorganize image to 128 pixels wide (VRAM layout) */
    int new_w, new_h;
    uint8_t *reorganized = reorganize_for_vram(indexed, w, h, opts.block_size, &new_w, &new_h);
    free(indexed);

    if (!reorganized) {
        fprintf(stderr, "Error: Out of memory during reorganization\n");
        return 1;
    }

    /* Calculate tile count from reorganized image */
    int tiles_x = new_w / TILE_SIZE;
    int tiles_y = new_h / TILE_SIZE;
    int tile_count = tiles_x * tiles_y;

    if (opts.verbose) {
        printf("Output: %d tiles (%dx%d grid)\n", tile_count, tiles_x, tiles_y);
    }

    /* Allocate tile output buffer */
    uint8_t *tiles = malloc(tile_count * tile_size);
    if (!tiles) {
        fprintf(stderr, "Error: Out of memory\n");
        free(reorganized);
        return 1;
    }

    /* Extract tiles LINEARLY from reorganized image */
    uint8_t tile_indexed[64];

    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            /* Extract 8x8 tile pixels */
            for (int py = 0; py < TILE_SIZE; py++) {
                for (int px = 0; px < TILE_SIZE; px++) {
                    int src_x = tx * TILE_SIZE + px;
                    int src_y = ty * TILE_SIZE + py;
                    tile_indexed[py * TILE_SIZE + px] = reorganized[src_y * new_w + src_x];
                }
            }

            /* Convert to SNES bitplane format */
            int tile_idx = ty * tiles_x + tx;  /* LINEAR index */
            if (opts.bpp == 2) {
                convert_tile_2bpp(tile_indexed, &tiles[tile_idx * tile_size]);
            } else {
                convert_tile_4bpp(tile_indexed, &tiles[tile_idx * tile_size]);
            }
        }
    }

    free(reorganized);

    /* Generate output name */
    char name_buf[64];
    const char *name = opts.name;
    if (!name) {
        char *base = basename((char *)opts.input);
        char *dot = strrchr(base, '.');
        if (dot) *dot = '\0';
        snprintf(name_buf, sizeof(name_buf), "%s", base);
        for (char *p = name_buf; *p; p++) {
            if ((*p < 'A' || *p > 'Z') && (*p < 'a' || *p > 'z') &&
                (*p < '0' || *p > '9')) {
                *p = '_';
            }
        }
        name = name_buf;
    }

    /* Write output */
    if (opts.c_header) {
        FILE *f = fopen(opts.output, "w");
        if (!f) {
            fprintf(stderr, "Error: Cannot create '%s'\n", opts.output);
            free(tiles);
            return 1;
        }
        write_c_header(f, name, tiles, tile_count, tile_size, &pal);
        fclose(f);
        printf("Output: %s (%d tiles, %d colors)\n", opts.output, tile_count, pal.count);
    } else {
        char base[256];
        strncpy(base, opts.output, sizeof(base) - 1);
        base[sizeof(base) - 1] = '\0';
        char *dot = strrchr(base, '.');
        if (dot && (strcmp(dot, ".pic") == 0 || strcmp(dot, ".pal") == 0)) {
            *dot = '\0';
        }
        write_binary(base, tiles, tile_count, tile_size, &pal);
    }

    free(tiles);
    return 0;
}

/*----------------------------------------------------------------------------*/
/* CLI                                                                        */
/*----------------------------------------------------------------------------*/

static void print_usage(const char *prog)
{
    printf("gfx2snes v%s - SNES graphics converter\n\n", VERSION);
    printf("Usage: %s [options] input.png output\n\n", prog);
    printf("Converts PNG images to SNES tile format.\n");
    printf("Reorganizes image to 128px wide (VRAM layout) before conversion.\n\n");
    printf("Options:\n");
    printf("  -b, --bpp <2|4>       Bits per pixel (default: 4)\n");
    printf("  -s, --size <8|16|32>  Block/sprite size in pixels (default: 8)\n");
    printf("  -c, --c-header        Output as C header file\n");
    printf("  -n, --name <name>     Variable name for C output\n");
    printf("  -v, --verbose         Verbose output\n");
    printf("  -h, --help            Show this help\n");
    printf("  -V, --version         Show version\n\n");
    printf("Examples:\n");
    printf("  %s -s 16 -c sprite.png sprite.h  # 16x16 sprites\n", prog);
    printf("  %s -s 8 -b 2 -c font.png font.h  # 8x8 tiles, 2bpp\n", prog);
}

int main(int argc, char *argv[])
{
    static struct option long_opts[] = {
        {"bpp",      required_argument, 0, 'b'},
        {"size",     required_argument, 0, 's'},
        {"c-header", no_argument,       0, 'c'},
        {"name",     required_argument, 0, 'n'},
        {"verbose",  no_argument,       0, 'v'},
        {"help",     no_argument,       0, 'h'},
        {"version",  no_argument,       0, 'V'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "b:s:cn:vhV", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'b':
                opts.bpp = atoi(optarg);
                if (opts.bpp != 2 && opts.bpp != 4) {
                    fprintf(stderr, "Error: BPP must be 2 or 4\n");
                    return 1;
                }
                break;
            case 's':
                opts.block_size = atoi(optarg);
                if (opts.block_size != 8 && opts.block_size != 16 && opts.block_size != 32) {
                    fprintf(stderr, "Error: Block size must be 8, 16, or 32\n");
                    return 1;
                }
                break;
            case 'c':
                opts.c_header = 1;
                break;
            case 'n':
                opts.name = optarg;
                break;
            case 'v':
                opts.verbose = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'V':
                printf("gfx2snes v%s\n", VERSION);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (optind + 2 != argc) {
        fprintf(stderr, "Error: Expected input and output files\n");
        print_usage(argv[0]);
        return 1;
    }

    opts.input = argv[optind];
    opts.output = argv[optind + 1];

    return convert_image();
}
