/*
 * opensnes-tileset — background images for the SNES BG layers (the 1.x
 * successor of `gfx4snes -m`; its converter is linked here as a library,
 * so the bytes are its own).
 *
 *   opensnes-tileset convert [--size N] [--bpp N] [--colors N] [--mode N] [--palette FILE] [--palette-entry N]
 *                            [--rearrange] [--offset N] [--priority] [--pages] [--no-reduce] [--flip] [--blank]
 *                            [--lz] [--pack] [--round] [--no-palette] [--out DIR] [--save] [--json] <image.png>...
 *   opensnes-tileset inspect [--size N] [--bpp N] [--json] <image.png>...
 *
 * `convert` cuts a picture into --size blocks, deduplicates them (and
 * their mirrors with --flip) and writes <stem>.pic (the tileset), <stem>.map
 * (the tilemap: tile number, palette bank, priority, flips), <stem>.pal
 * and the <stem>.inc / <stem>_data.as the build assembles. --mode 7 writes
 * the packed .pc7 / .mp7 pair.
 */
#include <stdbool.h>   /* gfx4snes's headers take bool; C before C23 needs this first */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "common.h"
#include "errors.h"
#include "images.h"
#include "palettes.h"
#include "tiles.h"
#include "maps.h"
#include "incgener.h"

extern cli_ctx *tileset_ctx;      /* diag.c */
extern const char *tileset_file;

static int palette_snes[256];

static int split_input(cli_ctx *ctx, const char *in, char *base, size_t n, const char **type)
{
    const char *slash = strrchr(in, '/');
    const char *dot = strrchr(slash ? slash + 1 : in, '.');
    if (!dot) { cli_error(ctx, in, "no extension — a .png or .bmp picture is expected"); return 0; }
    if (!strcmp(dot, ".png") || !strcmp(dot, ".PNG")) *type = "png";
    else if (!strcmp(dot, ".bmp") || !strcmp(dot, ".BMP")) *type = "bmp";
    else { cli_error(ctx, in, "%s is not a .png or .bmp picture", dot); return 0; }
    snprintf(base, n, "%.*s", (int)(dot - in), in);
    return 1;
}

static int bpp_to_colors(int bpp) { return bpp == 2 ? 4 : bpp == 8 ? 256 : bpp == 4 ? 16 : 0; }

/* --------------------------------------------------------------- convert */

static const cli_opt convert_opts[] = {
    { "size", 0, CLI_INT, "N", "the block in pixels: 8 (a BG tile) or 16", "8" },
    { "bpp", 0, CLI_INT, "N", "bits per pixel: 2 (4 colours, Mode 0), 4 (16), 8 (256, Modes 3/4/7)", "4" },
    { "colors", 0, CLI_INT, "N", "colours written to the .pal (0..256)", "256" },
    { "mode", 0, CLI_INT, "N", "the BG mode the map is for: 1 (also 0, 2, 3, 4), 5, 6 or 7", "1" },
    { "palette", 0, CLI_STR, "FILE", "impose this raw .pal; a colour the picture uses that is not in it is refused", NULL },
    { "palette-entry", 0, CLI_INT, "N", "first palette bank (0..7) the map entries carry", "0" },
    { "rearrange", 0, CLI_FLAG, NULL, "regroup the colours into the 8 banks of --bpp and renumber the tiles (4 bpp or 2 bpp)", NULL },
    { "offset", 0, CLI_INT, "N", "add N to every tile number in the map (0..2047): the tileset sits after N others", "0" },
    { "priority", 0, CLI_FLAG, NULL, "set the priority bit on every map entry", NULL },
    { "pages", 0, CLI_FLAG, NULL, "write the map in 32x32 pages (a scrolling 64x64 background)", NULL },
    { "no-reduce", 0, CLI_FLAG, NULL, "keep every block, duplicates included", NULL },
    { "flip", 0, CLI_FLAG, NULL, "deduplicate mirrored blocks; the map carries the flip bits", NULL },
    { "blank", 0, CLI_FLAG, NULL, "prepend a blank tile", NULL },
    { "lz", 0, CLI_FLAG, NULL, "LZ77-compress the .pic", NULL },
    { "pack", 0, CLI_FLAG, NULL, "packed-pixel tiles (what --mode 7 implies)", NULL },
    { "round", 0, CLI_FLAG, NULL, "round the palette (to a maximum of 63 per channel)", NULL },
    { "no-palette", 0, CLI_FLAG, NULL, "do not write the .pal", NULL },
    CLI_OPT_OUT,
    CLI_OPT_SAVE,
};

static int convert_one(cli_ctx *ctx, const char *in)
{
    int rc = cli_load_settings(ctx, in);
    if (rc != CLI_OK) return rc;
    tileset_file = in;

    char inbase[1024], outbase[1024];
    const char *type;
    if (!split_input(ctx, in, inbase, sizeof inbase, &type)) return CLI_REFUSED;
    if (cli_output_path(ctx, in, "", outbase, sizeof outbase) != CLI_OK) return CLI_IO;

    int size = cli_int(ctx, "size", 8), bpp = cli_int(ctx, "bpp", 4), colors = cli_int(ctx, "colors", 256);
    int mode = cli_int(ctx, "mode", 1), entry = cli_int(ctx, "palette-entry", 0), offset = cli_int(ctx, "offset", 0);
    int rearrange = cli_has(ctx, "rearrange"), prio = cli_has(ctx, "priority"), pages = cli_has(ctx, "pages");
    int noreduce = cli_has(ctx, "no-reduce"), flip = cli_has(ctx, "flip"), blank = cli_has(ctx, "blank");
    int lz = cli_has(ctx, "lz"), pack = cli_has(ctx, "pack") || mode == 7;
    int ncolors = bpp_to_colors(bpp);
    if (size != 8 && size != 16) { cli_error(ctx, in, "--size %d: 8 or 16", size); return CLI_REFUSED; }
    if (!ncolors) { cli_error(ctx, in, "--bpp %d: 2, 4 or 8", bpp); return CLI_REFUSED; }
    if (colors < 0 || colors > 256) { cli_error(ctx, in, "--colors %d: 0 to 256", colors); return CLI_REFUSED; }
    if (mode != 1 && mode != 5 && mode != 6 && mode != 7) { cli_error(ctx, in, "--mode %d: 1 (Modes 0 to 4), 5, 6 or 7", mode); return CLI_REFUSED; }
    if (entry < 0 || entry > 7) { cli_error(ctx, in, "--palette-entry %d: 0 to 7", entry); return CLI_REFUSED; }
    if (offset < 0 || offset > 2047) { cli_error(ctx, in, "--offset %d: 0 to 2047", offset); return CLI_REFUSED; }
    if (rearrange && (ncolors == 128 || ncolors == 256)) {
        cli_warn(ctx, in, "--rearrange means nothing at %d bpp (one palette); ignored", bpp);
        rearrange = 0;
    }
    if (rearrange && !cli_has(ctx, "colors")) {
        colors = ncolors * 8 > 256 ? 256 : ncolors * 8;   /* the 8 banks the rearrangement fills (gfx4snes -a) */
    }

    image_load(inbase, type, &snesimage, true);
    palette_convert_snes((t_RGB_color *)&snesimage.palette, palette_snes, cli_has(ctx, "round"), true);
    if (cli_has(ctx, "palette"))
        palette_impose(cli_str(ctx, "palette", NULL), &snesimage, palette_snes, ncolors, true);

    int w = (int)snesimage.header.width, h = (int)snesimage.header.height;
    int blksx = w / size + (w % size ? 1 : 0), blksy = h / size + (h % size ? 1 : 0);
    int nbtilesx = blksx, nbtiles = blksy;

    /* gfx4snes's map path */
    unsigned char *tiles = tiles_convertsnes(snesimage.buffer, w, h, size, size, &nbtilesx, &nbtiles, 8, true);
    if (rearrange) palette_rearrange_snes(tiles, palette_snes, nbtiles, ncolors, true);
    unsigned short *map = map_convertsnes(tiles, &nbtiles, size, size, blksx, blksy, ncolors, entry, mode, noreduce, blank, pages, flip, true);
    int map_blksx = (mode == 5 || mode == 6) ? blksx >> 1 : blksx;
    map_save(outbase, map, mode, map_blksx, blksy, offset, prio, true);
    if (pack) tiles_savepacked(outbase, tiles, nbtiles, blank, true);
    else tiles_save(outbase, tiles, nbtiles, ncolors, blank, lz, true);
    int savepal = !cli_has(ctx, "no-palette");
    if (savepal) palette_save(outbase, palette_snes, colors, true);
    inc_save(outbase, true, true, savepal, false, mode == 7, true);
    free(map); free(tiles); free(snesimage.buffer); snesimage.buffer = NULL;

    if (cli_has(ctx, "save") && (rc = cli_save_settings(ctx, in)) != CLI_OK) return rc;

    long vram_tiles = (long)(nbtiles + (blank ? 1 : 0)) * 8 * (mode == 7 ? 8 : bpp);
    long vram_map = (long)map_blksx * blksy * (mode == 7 ? 1 : 2);
    if (ctx->json) {
        cli_json_object(ctx, NULL);
        cli_json_str(ctx, "input", in);
        cli_json_object(ctx, "picture"); cli_json_int(ctx, "width", w); cli_json_int(ctx, "height", h);
        cli_json_int(ctx, "block", size); cli_json_int(ctx, "blocks_x", blksx); cli_json_int(ctx, "blocks_y", blksy); cli_json_close(ctx);
        cli_json_array(ctx, "outputs");
        const char *exts[] = { pack ? ".pc7" : ".pic", pack ? ".mp7" : ".map", ".inc", "_data.as", savepal ? ".pal" : NULL };
        const char *kinds[] = { "tiles", "map", "header", "asm", "palette" };
        for (int i = 0; i < 5; i++) {
            if (!exts[i]) continue;
            char path[1100]; snprintf(path, sizeof path, "%s%s", outbase, exts[i]);
            cli_json_object(ctx, NULL); cli_json_str(ctx, "path", path); cli_json_str(ctx, "kind", kinds[i]); cli_json_close(ctx);
        }
        cli_json_close(ctx);
        cli_json_int(ctx, "tiles", nbtiles); cli_json_int(ctx, "unique_of", (long)blksx * blksy * (size / 8) * (size / 8));
        cli_json_int(ctx, "bpp", bpp); cli_json_int(ctx, "mode", mode);
        cli_json_int(ctx, "vram_tiles_bytes", vram_tiles); cli_json_int(ctx, "vram_map_bytes", vram_map);
        cli_json_int(ctx, "palette_colors", savepal ? colors : 0);
        cli_json_close(ctx);
    } else if (!ctx->quiet) {
        printf("%s: %s -> %s.%s + .%s (%d tiles, %ld + %ld bytes of VRAM at %d bpp, mode %d%s)\n", ctx->tool->name, in, outbase,
               pack ? "pc7" : "pic", pack ? "mp7" : "map", nbtiles, vram_tiles, vram_map, bpp, mode, savepal ? ", .pal" : "");
    }
    return CLI_OK;
}

static int run_convert(cli_ctx *ctx)
{
    int worst = CLI_OK;
    tileset_ctx = ctx;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "pictures"); }
    for (int i = 0; i < ctx->nargs; i++) {
        int rc = convert_one(ctx, ctx->args[i]);
        if (rc != CLI_OK && ctx->json) { cli_json_object(ctx, NULL); cli_json_str(ctx, "input", ctx->args[i]); cli_json_int(ctx, "error", rc); cli_json_close(ctx); }
        if (rc > worst) worst = rc;
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

/* --------------------------------------------------------------- inspect */

static const cli_opt inspect_opts[] = {
    { "size", 0, CLI_INT, "N", "the block in pixels", "8" },
    { "bpp", 0, CLI_INT, "N", "bits per pixel", "4" },
};

static int run_inspect(cli_ctx *ctx)
{
    int worst = CLI_OK;
    tileset_ctx = ctx;
    int size = cli_int(ctx, "size", 8), bpp = cli_int(ctx, "bpp", 4);
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "pictures"); }
    for (int i = 0; i < ctx->nargs; i++) {
        const char *in = ctx->args[i];
        tileset_file = in;
        char inbase[1024];
        const char *type;
        if (!split_input(ctx, in, inbase, sizeof inbase, &type)) { worst = CLI_REFUSED; continue; }
        image_load(inbase, type, &snesimage, true);
        int w = (int)snesimage.header.width, h = (int)snesimage.header.height;
        int blksx = w / size + (w % size ? 1 : 0), blksy = h / size + (h % size ? 1 : 0);
        int used[256] = { 0 }, ncol = 0, maxidx = 0;
        for (long k = 0; k < (long)w * h; k++) if (!used[snesimage.buffer[k]]) { used[snesimage.buffer[k]] = 1; ncol++; }
        for (int k = 255; k >= 0; k--) if (used[k]) { maxidx = k; break; }
        long blocks = (long)blksx * blksy, worst_tiles = blocks * (size / 8) * (size / 8);
        free(snesimage.buffer); snesimage.buffer = NULL;
        if (ctx->json) {
            cli_json_object(ctx, NULL); cli_json_str(ctx, "input", in);
            cli_json_int(ctx, "width", w); cli_json_int(ctx, "height", h); cli_json_int(ctx, "block", size);
            cli_json_int(ctx, "blocks", blocks); cli_json_int(ctx, "tiles_before_dedup", worst_tiles);
            cli_json_int(ctx, "vram_tiles_bytes_max", worst_tiles * 8 * bpp); cli_json_int(ctx, "vram_map_bytes", blocks * 2);
            cli_json_int(ctx, "colors_used", ncol); cli_json_int(ctx, "highest_index", maxidx);
            cli_json_int(ctx, "palette_banks_touched", maxidx / bpp_to_colors(bpp) + 1);
            cli_json_close(ctx);
        } else {
            printf("%s: %dx%d px, %ld blocks of %d (up to %ld tiles before deduplication, %ld bytes of VRAM at %d bpp, map %ld bytes), %d colours used, highest index %d (bank %d)\n",
                   in, w, h, blocks, size, worst_tiles, worst_tiles * 8 * bpp, bpp, blocks * 2, ncol, maxidx, maxidx / bpp_to_colors(bpp));
        }
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

static const cli_cmd cmds[] = {
    { "convert", "picture (indexed PNG or BMP) -> .pic tileset, .map tilemap, .pal, .inc; deduplicated, per-tile palette banks",
      convert_opts, CLI_N(convert_opts), "convert res/town.png --colors 16 --save", 1, run_convert },
    { "inspect", "what a picture would cost (blocks, tiles before deduplication, VRAM, colours, palette banks)", inspect_opts, CLI_N(inspect_opts),
      "inspect res/*.png", 1, run_inspect },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes-tileset", TOOL_VERSION, "background pictures -> tileset, tilemap and palette for the BG layers", cmds, CLI_N(cmds),
    };
    return cli_main(&tool, argc, argv);
}
