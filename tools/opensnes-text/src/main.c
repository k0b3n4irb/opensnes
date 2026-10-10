/*
 * opensnes-text — bitmap fonts for the text module (the 1.x successor of
 * font2snes; its tile packer is linked here, so the tiles are its own
 * bytes).
 *
 *   opensnes-text font    [--bpp 2|4] [--out DIR] [--save] [--json] <font.png>...
 *   opensnes-text inspect [--json] <font.png>...
 *
 * A font is a picture of the 96 glyphs of ASCII 32..127 (space first), in
 * 8x8 cells laid out on any grid: 16 x 6 (128x48 pixels), 96 x 1, 32 x 3.
 * `font` writes <stem>.pic (the tiles, in glyph order), <stem>.pal (a grey
 * ramp, so the font shows before the game sets its colours) and the
 * <stem>.inc / <stem>_data.as glue the build assembles; the text module
 * draws from them once dmaCopyVram() has put the tiles where textInit()
 * points.
 *
 * How a pixel becomes a colour index: an indexed PNG gives its index as
 * is (the artist's choice); a grey or RGB picture is ranked by brightness
 * into the 4 (2 bpp) or 16 (4 bpp) levels — black is colour 0, the
 * transparent background, white the brightest colour.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "incfile.h"
#include "lodepng.h"
#include "tiles.h"   /* tools/font2snes/src: convert_tile_2bpp / 4bpp, rgb_to_bgr555 */

#define CELL       8
#define GLYPHS     96
#define FIRST_CHAR 32

typedef struct {
    unsigned w, h;
    unsigned char *index;    /* one colour index per pixel */
    const char *source;      /* "indexed", "grey" or "rgb" */
    int levels_used, highest;
} font_image;

/* Decode a PNG into colour indices for `colors` levels (4 or 16). */
static int load_font(cli_ctx *ctx, const char *path, int colors, font_image *img)
{
    unsigned char *file = NULL, *raw = NULL;
    size_t fsize = 0;
    unsigned err = lodepng_load_file(&file, &fsize, path);
    if (err) { cli_error(ctx, path, "cannot read: %s", lodepng_error_text(err)); return CLI_IO; }

    LodePNGState st;
    lodepng_state_init(&st);
    st.decoder.color_convert = 0;   /* the PNG's own colour type: indices stay indices */
    err = lodepng_decode(&raw, &img->w, &img->h, &st, file, fsize);
    if (err) {
        cli_error(ctx, path, "not a PNG this tool reads: %s", lodepng_error_text(err));
        free(file); lodepng_state_cleanup(&st);
        return CLI_REFUSED;
    }
    size_t npix = (size_t)img->w * img->h;
    img->index = calloc(npix, 1);
    if (!img->index) { free(file); free(raw); lodepng_state_cleanup(&st); cli_error(ctx, path, "out of memory"); return CLI_IO; }

    LodePNGColorType ct = st.info_png.color.colortype;
    unsigned bd = st.info_png.color.bitdepth;
    int rc = CLI_OK;
    if (ct == LCT_PALETTE) {
        img->source = "indexed";
        size_t rowbytes = ((size_t)img->w * bd + 7) / 8;
        for (unsigned y = 0; y < img->h; y++)
            for (unsigned x = 0; x < img->w; x++) {
                size_t bit = (size_t)x * bd;
                unsigned v = (raw[y * rowbytes + bit / 8] >> (8 - bd - bit % 8)) & ((1u << bd) - 1);
                if (bd == 8) v = raw[y * rowbytes + x];
                img->index[y * img->w + x] = (unsigned char)v;
                if ((int)v >= colors) {
                    cli_error(ctx, path, "colour index %u at (%u,%u) — %d bpp holds %d colours (0..%d); see --bpp",
                              v, x, y, colors == 4 ? 2 : 4, colors, colors - 1);
                    rc = CLI_REFUSED;
                    goto done;
                }
            }
    } else {
        /* grey or colour: rank the brightness into the levels */
        img->source = (ct == LCT_GREY || ct == LCT_GREY_ALPHA) ? "grey" : "rgb";
        unsigned char *rgba = NULL;
        unsigned w2, h2;
        err = lodepng_decode32(&rgba, &w2, &h2, file, fsize);
        if (err) { cli_error(ctx, path, "cannot decode: %s", lodepng_error_text(err)); rc = CLI_REFUSED; goto done; }
        int step = 256 / colors;
        for (size_t i = 0; i < npix; i++) {
            int bright = (rgba[i * 4] + rgba[i * 4 + 1] + rgba[i * 4 + 2]) / 3;
            img->index[i] = (unsigned char)(bright / step);
        }
        free(rgba);
    }
    int used[256] = { 0 };
    img->levels_used = 0; img->highest = 0;
    for (size_t i = 0; i < npix; i++) {
        if (!used[img->index[i]]) { used[img->index[i]] = 1; img->levels_used++; }
        if (img->index[i] > img->highest) img->highest = img->index[i];
    }
done:
    free(file); free(raw); lodepng_state_cleanup(&st);
    if (rc != CLI_OK) { free(img->index); img->index = NULL; }
    return rc;
}

/* The grid: every dimension a multiple of 8, exactly 96 cells. */
static int check_grid(cli_ctx *ctx, const char *path, const font_image *img, int *cols)
{
    if (img->w % CELL || img->h % CELL) {
        cli_error(ctx, path, "%ux%u pixels — the glyphs are 8x8 cells, so both sizes must be multiples of 8", img->w, img->h);
        return 0;
    }
    *cols = (int)(img->w / CELL);
    int cells = *cols * (int)(img->h / CELL);
    if (cells != GLYPHS) {
        cli_error(ctx, path, "%d cells of 8x8 — a font holds exactly %d glyphs (ASCII %d..%d, the space first): 128x48, 768x8 or any grid of %d",
                  cells, GLYPHS, FIRST_CHAR, FIRST_CHAR + GLYPHS - 1, GLYPHS);
        return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ font */

static const cli_opt font_opts[] = {
    { "bpp", 0, CLI_INT, "N", "bits per pixel: 2 (4 colours, the text module's BG3 / Mode 0 font) or 4 (16, a Mode 1 BG1 or BG2 layer)", "2" },
    CLI_OPT_OUT,
    CLI_OPT_SAVE,
};

static int font_one(cli_ctx *ctx, const char *in)
{
    int rc = cli_load_settings(ctx, in);
    if (rc != CLI_OK) return rc;
    int bpp = cli_int(ctx, "bpp", 2);
    if (bpp != 2 && bpp != 4) { cli_error(ctx, in, "--bpp %d: 2 or 4", bpp); return CLI_REFUSED; }
    int colors = bpp == 2 ? 4 : 16, bytes_per_glyph = bpp * 8;

    font_image img = { 0 };
    rc = load_font(ctx, in, colors, &img);
    if (rc != CLI_OK) return rc;
    int cols;
    if (!check_grid(ctx, in, &img, &cols)) { free(img.index); return CLI_REFUSED; }

    unsigned char *tiles = malloc((size_t)GLYPHS * bytes_per_glyph);
    if (!tiles) { free(img.index); cli_error(ctx, in, "out of memory"); return CLI_IO; }
    for (int g = 0; g < GLYPHS; g++) {
        uint8_t cell[CELL * CELL];
        int bx = (g % cols) * CELL, by = (g / cols) * CELL;
        for (int y = 0; y < CELL; y++)
            for (int x = 0; x < CELL; x++)
                cell[y * CELL + x] = img.index[(size_t)(by + y) * img.w + bx + x];
        if (bpp == 2) convert_tile_2bpp(cell, tiles + g * bytes_per_glyph);
        else          convert_tile_4bpp(cell, tiles + g * bytes_per_glyph);
    }
    /* a grey ramp: black at 0, white at the top, the font visible before setColor() */
    unsigned char pal[16 * 2];
    for (int i = 0; i < colors; i++) {
        uint8_t level = (uint8_t)((i * 255) / (colors - 1));
        uint16_t c = rgb_to_bgr555(level, level, level);
        pal[i * 2] = (unsigned char)(c & 0xFF);
        pal[i * 2 + 1] = (unsigned char)(c >> 8);
    }

    char pic_path[1024], pal_path[1024], inc_path[1024], as_path[1024], ident[256], upper[256];
    if (cli_output_path(ctx, in, ".pic", pic_path, sizeof pic_path) != CLI_OK ||
        cli_output_path(ctx, in, ".pal", pal_path, sizeof pal_path) != CLI_OK ||
        cli_output_path(ctx, in, ".inc", inc_path, sizeof inc_path) != CLI_OK ||
        cli_output_path(ctx, in, "_data.as", as_path, sizeof as_path) != CLI_OK) { free(tiles); free(img.index); return CLI_IO; }
    cli_ident(in, ident, sizeof ident);
    for (size_t i = 0; i <= strlen(ident); i++) upper[i] = (char)((ident[i] >= 'a' && ident[i] <= 'z') ? ident[i] - 32 : ident[i]);

    rc = cli_write_file(ctx, pic_path, tiles, (size_t)GLYPHS * bytes_per_glyph);
    if (rc == CLI_OK) rc = cli_write_file(ctx, pal_path, pal, (size_t)colors * 2);
    free(tiles);
    if (rc != CLI_OK) { free(img.index); return rc; }

    FILE *f = fopen(inc_path, "w");
    if (!f) { cli_error(ctx, inc_path, "cannot write"); free(img.index); return CLI_IO; }
    fprintf(f, "/* %s.inc — generated by %s %s; do not edit. The data labels live in %s_data.as,\n"
               " * which the build gathers into assets_gen.asm (docs/tools/CONVENTIONS.md). */\n"
               "#ifndef ASSET_%s_INC\n#define ASSET_%s_INC\n\n#include <snes.h>\n\n",
            ident, ctx->tool->name, ctx->tool->version, ident, ident, ident);
    fprintf(f, "/* %d glyphs (ASCII %d..%d) of 8x8 at %d bpp, %d bytes. Put them where the text module reads:\n"
               " *   dmaCopyVram(%s_tiles, vram_addr, %s_tiles_end - %s_tiles);\n"
               " *   textInit(TEXT_DEFAULT_TILEMAP_ADDR, first_tile, palette);   // first_tile = vram_addr / %d\n"
               " * Glyph pixels are colours 1..%d of the palette slot; colour 0 is the transparent background. */\n",
            GLYPHS, FIRST_CHAR, FIRST_CHAR + GLYPHS - 1, bpp, GLYPHS * bytes_per_glyph, ident, ident, ident, bytes_per_glyph / 2, colors - 1);
    fprintf(f, "extern const u8 %s_tiles[], %s_tiles_end[];\n", ident, ident);
    fprintf(f, "extern const u8 %s_pal[], %s_pal_end[];   /* a grey ramp of %d colours (%d bytes); setColor() writes the game's own */\n\n",
            ident, ident, colors, colors * 2);
    fprintf(f, "#define %s_GLYPHS          %d\n#define %s_FIRST_CHAR      %d\n#define %s_BPP             %d\n"
               "#define %s_BYTES_PER_GLYPH %d\n#define %s_TILES_SIZE      %d\n\n#endif\n",
            upper, GLYPHS, upper, FIRST_CHAR, upper, bpp, upper, bytes_per_glyph, upper, GLYPHS * bytes_per_glyph);
    fclose(f);

    f = fopen(as_path, "w");
    if (!f) { cli_error(ctx, as_path, "cannot write"); free(img.index); return CLI_IO; }
    fprintf(f, "; %s_data.as — generated by %s %s; one ASSET_SECTION per blob, .include it from assets_gen.asm (the build does)\n\n",
            ident, ctx->tool->name, ctx->tool->version);
    incfile_write_blob(f, ident, "tiles", pic_path);
    incfile_write_blob(f, ident, "pal", pal_path);
    fclose(f);

    if (cli_has(ctx, "save") && (rc = cli_save_settings(ctx, in)) != CLI_OK) { free(img.index); return rc; }

    if (ctx->json) {
        cli_json_object(ctx, NULL);
        cli_json_str(ctx, "input", in);
        cli_json_int(ctx, "width", img.w); cli_json_int(ctx, "height", img.h);
        cli_json_str(ctx, "source", img.source);
        cli_json_int(ctx, "glyphs", GLYPHS); cli_json_int(ctx, "first_char", FIRST_CHAR);
        cli_json_int(ctx, "bpp", bpp); cli_json_int(ctx, "colors_used", img.levels_used);
        cli_json_array(ctx, "outputs");
        const char *paths[] = { pic_path, pal_path, inc_path, as_path };
        const char *kinds[] = { "tiles", "palette", "header", "asm" };
        for (int i = 0; i < 4; i++) { cli_json_object(ctx, NULL); cli_json_str(ctx, "path", paths[i]); cli_json_str(ctx, "kind", kinds[i]); cli_json_close(ctx); }
        cli_json_close(ctx);
        cli_json_int(ctx, "vram_bytes", GLYPHS * bytes_per_glyph);
        cli_json_close(ctx);
    } else if (!ctx->quiet) {
        printf("%s: %s -> %s + .pal (%d glyphs at %d bpp, %d bytes of VRAM, %d colour%s used)\n", ctx->tool->name, in, pic_path,
               GLYPHS, bpp, GLYPHS * bytes_per_glyph, img.levels_used, img.levels_used == 1 ? "" : "s");
    }
    free(img.index);
    return CLI_OK;
}

static int run_font(cli_ctx *ctx)
{
    int worst = CLI_OK;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "fonts"); }
    for (int i = 0; i < ctx->nargs; i++) {
        int rc = font_one(ctx, ctx->args[i]);
        if (rc != CLI_OK && ctx->json) { cli_json_object(ctx, NULL); cli_json_str(ctx, "input", ctx->args[i]); cli_json_int(ctx, "error", rc); cli_json_close(ctx); }
        if (rc > worst) worst = rc;
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

/* --------------------------------------------------------------- inspect */

static int run_inspect(cli_ctx *ctx)
{
    int worst = CLI_OK;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "fonts"); }
    for (int i = 0; i < ctx->nargs; i++) {
        const char *in = ctx->args[i];
        font_image img = { 0 };
        int rc = load_font(ctx, in, 256, &img);   /* every index accepted: this only looks */
        if (rc != CLI_OK) { if (rc > worst) worst = rc; continue; }
        int cells = (int)((img.w / CELL) * (img.h / CELL)), aligned = !(img.w % CELL) && !(img.h % CELL);
        int fits = aligned && cells == GLYPHS;
        if (ctx->json) {
            cli_json_object(ctx, NULL); cli_json_str(ctx, "input", in);
            cli_json_int(ctx, "width", img.w); cli_json_int(ctx, "height", img.h); cli_json_str(ctx, "source", img.source);
            cli_json_int(ctx, "cells", cells); cli_json_bool(ctx, "font", fits);
            cli_json_int(ctx, "colors_used", img.levels_used);
            if (!strcmp(img.source, "indexed")) cli_json_int(ctx, "highest_index", img.highest);
            cli_json_int(ctx, "vram_bytes_2bpp", GLYPHS * 16); cli_json_int(ctx, "vram_bytes_4bpp", GLYPHS * 32);
            cli_json_close(ctx);
        } else {
            char hi[48] = "";
            if (!strcmp(img.source, "indexed")) snprintf(hi, sizeof hi, " (highest index %d)", img.highest);
            printf("%s: %ux%u px (%s), %d cells of 8x8%s, %d colour%s used%s; %d bytes of VRAM at 2 bpp, %d at 4 bpp\n",
                   in, img.w, img.h, img.source, cells, fits ? ", a font of 96 glyphs" : aligned ? " — a font has 96" : " — sizes not multiples of 8",
                   img.levels_used, img.levels_used == 1 ? "" : "s", hi, GLYPHS * 16, GLYPHS * 32);
        }
        free(img.index);
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

static const cli_cmd cmds[] = {
    { "font", "a picture of the 96 glyphs (ASCII 32..127, 8x8 cells) -> .pic tiles in glyph order, a grey .pal, .inc",
      font_opts, CLI_N(font_opts), "font res/font.png --save", 1, run_font },
    { "inspect", "what a font picture holds (grid, cells, colours) and costs in VRAM", NULL, 0, "inspect res/*.png", 1, run_inspect },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes-text", TOOL_VERSION, "bitmap fonts -> tiles for the text module", cmds, CLI_N(cmds),
    };
    return cli_main(&tool, argc, argv);
}
