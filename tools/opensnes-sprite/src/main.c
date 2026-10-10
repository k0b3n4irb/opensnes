/*
 * opensnes-sprite — sprite sheets and Aseprite exports for the SNES OBJ
 * layer (the 1.x successor of `gfx4snes -s/-T` and `aseprite2snes`; their
 * converters are linked here as libraries, so the bytes are theirs).
 *
 *   opensnes-sprite sheet   [--size N] [--bpp N] [--colors N] [--palette FILE] [--palette-entry N]
 *                           [--metasprite W H] [--priority N] [--flip] [--pack] [--lz] [--blank]
 *                           [--no-palette] [--out DIR] [--save] [--json] <sheet.png | sheet.bmp>...
 *   opensnes-sprite anim    [--prefix NAME] [--stride N] [--fps N] [--out DIR] [--json] <export.json>...
 *   opensnes-sprite inspect [--size N] [--bpp N] [--json] <sheet.png>...
 *
 * `sheet` cuts a sheet into --size blocks and writes <stem>.pic (tiles in
 * OBJ VRAM order), <stem>.pal, <stem>.inc / <stem>_data.as (the externs
 * and the .incbin), and with --metasprite the <stem>_meta.inc table of
 * METASPR_ITEM entries, one metasprite per W x H cell. `anim` turns the
 * JSON Aseprite writes with --data --list-tags into <stem>_anim.h, one
 * AnimClip per tag. `inspect` says what a sheet will cost.
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
#include "metasprites.h"
#include "incfile.h"
#include "anim.h"

extern cli_ctx *sprite_ctx;      /* diag.c */
extern const char *sprite_file;

static int palette_snes[256];

/* ----------------------------------------------------------------- paths */

/* "res/hero.png" → base "res/hero" and type "png"|"bmp"; 0 if neither. */
static int split_input(cli_ctx *ctx, const char *in, char *base, size_t n, const char **type)
{
    const char *slash = strrchr(in, '/');
    const char *dot = strrchr(slash ? slash + 1 : in, '.');
    if (!dot) { cli_error(ctx, in, "no extension — a .png or .bmp sheet is expected"); return 0; }
    if (!strcmp(dot, ".png") || !strcmp(dot, ".PNG")) *type = "png";
    else if (!strcmp(dot, ".bmp") || !strcmp(dot, ".BMP")) *type = "bmp";
    else { cli_error(ctx, in, "%s is not a .png or .bmp sheet", dot); return 0; }
    snprintf(base, n, "%.*s", (int)(dot - in), in);
    return 1;
}

/* ----------------------------------------------------------------- sheet */

static const cli_opt sheet_opts[] = {
    { "size", 0, CLI_INT, "N", "the sprite block in pixels: 8, 16, 32 or 64", "16" },
    { "bpp", 0, CLI_INT, "N", "bits per pixel: 2 (4 colours), 4 (16), 8 (256)", "4" },
    { "colors", 0, CLI_INT, "N", "colours written to the .pal (0..256)", "256" },
    { "palette", 0, CLI_STR, "FILE", "impose this raw .pal; a colour the sheet uses that is not in it is refused", NULL },
    { "palette-entry", 0, CLI_INT, "N", "OBJ palette number (0..7) the metasprite entries carry", "0" },
    { "metasprite", 0, CLI_INT2, "W H", "write <stem>_meta.inc: one metasprite per W x H pixels of the sheet", NULL },
    { "priority", 0, CLI_INT, "N", "OBJ priority (0..3) of the metasprite entries", "0" },
    { "flip", 0, CLI_FLAG, NULL, "a block that is the mirror of another is that block, flipped (metasprite entries carry OBJ_FLIPX / OBJ_FLIPY)", NULL },
    { "compact", 0, CLI_FLAG, NULL, "write each distinct block once (with --flip, mirrors count as one) and <stem>_blocks.inc: where each block of the sheet went", NULL },
    { "pack", 0, CLI_FLAG, NULL, "packed-pixel tile format (Mode 7 style)", NULL },
    { "lz", 0, CLI_FLAG, NULL, "LZ77-compress the .pic (LzssDecode at runtime)", NULL },
    { "blank", 0, CLI_FLAG, NULL, "prepend a blank tile", NULL },
    { "round", 0, CLI_FLAG, NULL, "round the palette (to a maximum of 63 per channel)", NULL },
    { "no-palette", 0, CLI_FLAG, NULL, "do not write the .pal", NULL },
    CLI_OPT_OUT,
    CLI_OPT_SAVE,
};

static int bpp_to_colors(int bpp) { return bpp == 2 ? 4 : bpp == 8 ? 256 : bpp == 4 ? 16 : 0; }

/* --compact. A sheet often holds the same block several times, and its
 * mirror as often (a character facing left and right). Until 2026-10-10
 * --flip only said so in the metasprite table: the .pic still held every
 * block, mirrors included, so nothing was saved in ROM or in VRAM, and
 * without --metasprite nothing told the game which block was whose mirror
 * (issue #165).
 *
 * One pixel of block `b` of the sheet, 0 outside the picture. */
static int block_px(const unsigned char *img, int w, int h, int size, int blksx, int b, int x, int y)
{
    int px = (b % blksx) * size + x, py = (b / blksx) * size + y;
    return (px < w && py < h) ? img[py * w + px] : 0;
}

/* Is block `b` block `u` seen through `flips` (bit 0: mirrored in x, bit 1: in y)? */
static int block_is(const unsigned char *img, int w, int h, int size, int blksx, int b, int u, int flips)
{
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++)
            if (block_px(img, w, h, size, blksx, b, x, y)
                != block_px(img, w, h, size, blksx, u, (flips & 1) ? size - 1 - x : x, (flips & 2) ? size - 1 - y : y))
                return 0;
    return 1;
}

#define BLOCK_FLIPX 0x4000      /* the bits of an OAM tile word, and of the block map */
#define BLOCK_FLIPY 0x8000

/* Fills where[b] = stored block | flip bits for every block of the sheet and
 * kept[k] = the sheet block stored as block k; returns how many are kept. */
static int compact_blocks(const unsigned char *img, int w, int h, int size, int blksx, int blksy,
                          int flip, unsigned short *where, int *kept)
{
    int n = 0;
    for (int b = 0; b < blksx * blksy; b++) {
        int found = -1, how = 0;
        for (int k = 0; k < n && found < 0; k++)
            for (int f = 0; f < (flip ? 4 : 1); f++)
                if (block_is(img, w, h, size, blksx, b, kept[k], f)) { found = k; how = f; break; }
        if (found < 0) { kept[n] = b; found = n++; }
        where[b] = (unsigned short)(found | ((how & 1) ? BLOCK_FLIPX : 0) | ((how & 2) ? BLOCK_FLIPY : 0));
    }
    return n;
}

static int sheet_one(cli_ctx *ctx, const char *in)
{
    int rc = cli_load_settings(ctx, in);
    if (rc != CLI_OK) return rc;
    sprite_file = in;

    char inbase[1024], outbase[1024];
    const char *type;
    if (!split_input(ctx, in, inbase, sizeof inbase, &type)) return CLI_REFUSED;
    if (cli_output_path(ctx, in, "", outbase, sizeof outbase) != CLI_OK) return CLI_IO;

    int size = cli_int(ctx, "size", 16), bpp = cli_int(ctx, "bpp", 4), colors = cli_int(ctx, "colors", 256);
    int entry = cli_int(ctx, "palette-entry", 0), prio = cli_int(ctx, "priority", 0);
    int metaw = 0, metah = 0, meta = cli_int2(ctx, "metasprite", &metaw, &metah);
    int flip = cli_has(ctx, "flip"), pack = cli_has(ctx, "pack"), lz = cli_has(ctx, "lz"), blank = cli_has(ctx, "blank");
    int compact = cli_has(ctx, "compact");
    int ncolors = bpp_to_colors(bpp);
    if (size != 8 && size != 16 && size != 32 && size != 64) { cli_error(ctx, in, "--size %d: the OBJ sizes are 8, 16, 32 and 64", size); return CLI_REFUSED; }
    if (!ncolors) { cli_error(ctx, in, "--bpp %d: 2, 4 or 8", bpp); return CLI_REFUSED; }
    if (colors < 0 || colors > 256) { cli_error(ctx, in, "--colors %d: 0 to 256", colors); return CLI_REFUSED; }
    if (entry < 0 || entry > 7) { cli_error(ctx, in, "--palette-entry %d: 0 to 7", entry); return CLI_REFUSED; }
    if (prio < 0 || prio > 3) { cli_error(ctx, in, "--priority %d: 0 to 3", prio); return CLI_REFUSED; }
    if (meta && (metaw <= 0 || metaw > 128 || metah <= 0 || metah > 128 || metaw % size || metah % size)) {
        cli_error(ctx, in, "--metasprite %d %d: 1 to 128 pixels each, multiples of the block size %d", metaw, metah, size);
        return CLI_REFUSED;
    }

    /* the converter (gfx4snes's block path: no map) */
    image_load(inbase, type, &snesimage, true);
    palette_convert_snes((t_RGB_color *)&snesimage.palette, palette_snes, cli_has(ctx, "round"), true);
    char palpath[1100];
    if (cli_has(ctx, "palette"))
        palette_impose(cli_path(ctx, "palette", palpath, sizeof palpath), &snesimage, palette_snes, ncolors, true);

    int w = (int)snesimage.header.width, h = (int)snesimage.header.height;
    int blksx = w / size + (w % size ? 1 : 0), blksy = h / size + (h % size ? 1 : 0);
    if (w % size || h % size)
        cli_warn(ctx, in, "%dx%d px is not a multiple of the %d px block: the last row or column of blocks is padded with pixels that are not in the sheet", w, h, size);

    /* --compact: the picture the rest of this function converts becomes the
     * distinct blocks, 128 pixels to a row as OBJ VRAM is, and `where` says
     * which of them each block of the sheet is */
    unsigned short *where = NULL;
    int nkept = 0, sheet_blocks = blksx * blksy, sheet_blksx = blksx;
    unsigned char *sheet_img = NULL;
    int sheet_w = w, sheet_h = h;
    if (compact) {
        int *kept = malloc(sizeof *kept * (size_t)sheet_blocks);
        where = malloc(sizeof *where * (size_t)sheet_blocks);
        if (!kept || !where) { cli_error(ctx, in, "out of memory"); return CLI_IO; }
        nkept = compact_blocks(snesimage.buffer, w, h, size, blksx, blksy, flip, where, kept);
        int perrow = 128 / size, cw = perrow * size, ch = ((nkept + perrow - 1) / perrow) * size;
        unsigned char *packed = calloc((size_t)cw * (size_t)ch, 1);
        if (!packed) { cli_error(ctx, in, "out of memory"); return CLI_IO; }
        for (int k = 0; k < nkept; k++)
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                    packed[((k / perrow) * size + y) * cw + (k % perrow) * size + x]
                        = (unsigned char)block_px(snesimage.buffer, w, h, size, blksx, kept[k], x, y);
        free(kept);
        sheet_img = snesimage.buffer;       /* the sheet itself, for the metasprite map below */
        snesimage.buffer = packed;
        w = cw; h = ch;
        blksx = perrow; blksy = ch / size;
    }

    unsigned short *map = NULL;
    int nmeta = 0;
    if (meta && compact) {
        /* the map of the SHEET, with each block's index and flips replaced by
         * where it went: the metasprite names then follow the compacted .pic */
        int bx = sheet_blksx, sby = sheet_blocks / sheet_blksx, nbt = sby;
        unsigned char *mt = tiles_convertsnes(sheet_img, sheet_w, sheet_h, size, size, &bx, &nbt, size, true);
        map = map_convertsnes(mt, &nbt, size, size, sheet_blksx, sby, ncolors, entry, 0, 1, 0, 0, 0, true);
        for (int b = 0; b < sheet_blocks; b++)
            map[b] = (unsigned short)((map[b] & ~(TILEIDX_MASK | BLOCK_FLIPX | BLOCK_FLIPY)) | where[b]);
        metasprite_save(outbase, map, sheet_blksx, sby, size, metaw, metah, prio, sheet_w, sheet_h, true);
        nmeta = sheet_blocks / ((metaw / size) * (metah / size));
        free(mt);
    } else if (meta) {
        int bx = blksx, by = blksy, nbtiles = by;
        unsigned char *mt = tiles_convertsnes(snesimage.buffer, w, h, size, size, &bx, &nbtiles, size, true);
        map = map_convertsnes(mt, &nbtiles, size, size, blksx, blksy, ncolors, entry, 0, 1, 0, 0, flip, true);
        metasprite_save(outbase, map, blksx, blksy, size, metaw, metah, prio, w, h, true);
        nmeta = (blksx * blksy) / ((metaw / size) * (metah / size));
        free(mt);
    }
    int bx = blksx, by = blksy;
    unsigned char *nomap = tiles_convertsnes(snesimage.buffer, w, h, size, size, &bx, &by, 16 * 8, true);
    bx *= size / 8; by *= size / 8;
    unsigned char *tiles = tiles_convertsnes(nomap, bx * 8, by * 8, 8, 8, &bx, &by, 8, true);
    free(nomap);
    int nbtiles = bx * by;

    if (pack) tiles_savepacked(outbase, tiles, nbtiles, blank, true);
    else { tiles_checkbanks(tiles, nbtiles, ncolors); tiles_save(outbase, tiles, nbtiles, ncolors, blank, lz, true); }
    int savepal = !cli_has(ctx, "no-palette");
    if (savepal) palette_save(outbase, palette_snes, colors, true);
    if (compact) {
        /* <stem>_blocks.inc: one word per block of the sheet, in reading order */
        char path[1100], ident2[256];
        snprintf(path, sizeof path, "%s_blocks.inc", outbase);
        cli_ident(in, ident2, sizeof ident2);
        FILE *fp = fopen(path, "wb");
        if (!fp) { cli_error(ctx, in, "can't write %s", path); return CLI_IO; }
        fprintf(fp, "/* Generated by %s %s --compact%s. One word per %dx%d block of the\n"
                    " * sheet, in reading order: the low bits are the block of the .pic that holds\n"
                    " * its pixels, bit 14 says it is drawn mirrored in x, bit 15 in y — the bits\n"
                    " * of OBJ_FLIPX / OBJ_FLIPY in an OAM attribute word. %d blocks in the sheet,\n"
                    " * %d in the .pic. */\n", ctx->tool->name, ctx->tool->version, flip ? " --flip" : "", size, size, sheet_blocks, nkept);
        fprintf(fp, "#define %s_BLOCKS %d\n#define %s_STORED_BLOCKS %d\n", ident2, sheet_blocks, ident2, nkept);
        fprintf(fp, "const unsigned short %s_blocks[%d] = {", ident2, sheet_blocks);
        for (int b = 0; b < sheet_blocks; b++)
            fprintf(fp, "%s0x%04X%s", b % 8 ? " " : "\n\t", where[b], b + 1 < sheet_blocks ? "," : "");
        fprintf(fp, "\n};\n");
        fclose(fp);
    }
    free(where); free(sheet_img);
    free(map); free(tiles); free(snesimage.buffer); snesimage.buffer = NULL;
    /* the glue: <stem>.inc (DECLARE_GFX_ASSET) and <stem>_data.as, in asset.h's naming */
    char ident[256], generator[64], err[160];
    cli_ident(in, ident, sizeof ident);
    snprintf(generator, sizeof generator, "%s %s", ctx->tool->name, ctx->tool->version);
    incfile_spec spec = { generator, bpp, savepal, 0, meta, pack, 0, 0, cli_has(ctx, "lz") };
    if (incfile_write(outbase, ident, &spec, err, sizeof err) != 0) { cli_error(ctx, in, "%s", err); return CLI_IO; }

    if (cli_has(ctx, "save") && (rc = cli_save_settings(ctx, in)) != CLI_OK) return rc;

    long vram = (long)nbtiles * 8 * bpp;   /* 8 rows x 8 px x bpp bits = 8*bpp bytes per tile */
    if (ctx->json) {
        cli_json_object(ctx, NULL);
        cli_json_str(ctx, "input", in);
        cli_json_object(ctx, "sheet"); cli_json_int(ctx, "width", w); cli_json_int(ctx, "height", h);
        cli_json_int(ctx, "block", size); cli_json_int(ctx, "blocks_x", blksx); cli_json_int(ctx, "blocks_y", blksy); cli_json_close(ctx);
        cli_json_array(ctx, "outputs");
        const char *exts[] = { pack ? ".pc7" : ".pic", ".inc", "_data.as", savepal ? ".pal" : NULL, meta ? "_meta.inc" : NULL,
                               compact ? "_blocks.inc" : NULL };
        const char *kinds[] = { "tiles", "header", "asm", "palette", "metasprites", "blocks" };
        for (int i = 0; i < 6; i++) {
            if (!exts[i]) continue;
            char path[1100]; snprintf(path, sizeof path, "%s%s", outbase, exts[i]);
            cli_json_object(ctx, NULL); cli_json_str(ctx, "path", path); cli_json_str(ctx, "kind", kinds[i]); cli_json_close(ctx);
        }
        cli_json_close(ctx);
        cli_json_int(ctx, "tiles", nbtiles); cli_json_int(ctx, "bpp", bpp); cli_json_int(ctx, "vram_bytes", vram);
        cli_json_int(ctx, "metasprites", nmeta); cli_json_int(ctx, "palette_colors", savepal ? colors : 0);
        cli_json_close(ctx);
    } else if (!ctx->quiet) {
        printf("%s: %s -> %s.%s (%d tiles, %ld bytes of VRAM at %d bpp%s%s)\n", ctx->tool->name, in, outbase, pack ? "pc7" : "pic",
               nbtiles, vram, bpp, savepal ? ", .pal" : "", meta ? ", _meta.inc" : "");
        if (nmeta) printf("  %d metasprite%s of %dx%d\n", nmeta, nmeta == 1 ? "" : "s", metaw, metah);
        if (compact) printf("  %d blocks of %dx%d in the sheet, %d kept (_blocks.inc)\n", sheet_blocks, size, size, nkept);
    }
    return CLI_OK;
}

static int run_sheet(cli_ctx *ctx)
{
    int worst = CLI_OK;
    sprite_ctx = ctx;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "sheets"); }
    for (int i = 0; i < ctx->nargs; i++) {
        int rc = sheet_one(ctx, ctx->args[i]);
        if (rc != CLI_OK && ctx->json) { cli_json_object(ctx, NULL); cli_json_str(ctx, "input", ctx->args[i]); cli_json_int(ctx, "error", rc); cli_json_close(ctx); }
        if (rc > worst) worst = rc;
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

/* ------------------------------------------------------------------ anim */

static const cli_opt anim_opts[] = {
    { "prefix", 0, CLI_STR, "NAME", "symbol and macro prefix", "the file's stem" },
    { "stride", 0, CLI_INT, "N", "frame value = frame index x N (1: metasprite-table indices)", "1" },
    { "fps", 0, CLI_INT, "N", "frame rate for the ms -> tick conversion", "60" },
    CLI_OPT_OUT,
    CLI_OPT_SAVE,
};

static void anim_warn(const char *msg, void *user) { cli_warn((cli_ctx *)user, sprite_file, "%s", msg); }

static int anim_one(cli_ctx *ctx, const char *in)
{
    int rc = cli_load_settings(ctx, in);
    if (rc != CLI_OK) return rc;
    sprite_file = in;
    int stride = cli_int(ctx, "stride", 1), fps = cli_int(ctx, "fps", 60);
    if (stride < 1) { cli_error(ctx, in, "--stride %d: 1 or more", stride); return CLI_REFUSED; }
    if (fps < 1) { cli_error(ctx, in, "--fps %d: 1 or more", fps); return CLI_REFUSED; }
    char out[1024];
    if (cli_output_path(ctx, in, "_anim.h", out, sizeof out) != CLI_OK) return CLI_IO;
    char tmp[1100];
    snprintf(tmp, sizeof tmp, "%s.part", out);
    FILE *o = fopen(tmp, "wb");
    if (!o) { cli_error(ctx, out, "cannot write — does the directory exist?"); return CLI_IO; }
    char err[512], generator[64];
    snprintf(generator, sizeof generator, "%s %s", ctx->tool->name, ctx->tool->version);
    rc = aseprite_to_clips(in, cli_str(ctx, "prefix", NULL), stride, fps, generator, o, err, sizeof err, anim_warn, ctx);
    fclose(o);
    if (rc) { remove(tmp); cli_error(ctx, in, "%s", err); return CLI_REFUSED; }
    remove(out);
    if (rename(tmp, out) != 0) { cli_error(ctx, out, "cannot write"); return CLI_IO; }
    if (cli_has(ctx, "save") && (rc = cli_save_settings(ctx, in)) != CLI_OK) return rc;
    if (ctx->json) {
        cli_json_object(ctx, NULL); cli_json_str(ctx, "input", in);
        cli_json_array(ctx, "outputs"); cli_json_object(ctx, NULL); cli_json_str(ctx, "path", out); cli_json_str(ctx, "kind", "header"); cli_json_close(ctx); cli_json_close(ctx);
        cli_json_close(ctx);
    } else if (!ctx->quiet) {
        printf("%s: %s -> %s\n", ctx->tool->name, in, out);
    }
    return CLI_OK;
}

static int run_anim(cli_ctx *ctx)
{
    int worst = CLI_OK;
    sprite_ctx = ctx;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "clips"); }
    for (int i = 0; i < ctx->nargs; i++) {
        int rc = anim_one(ctx, ctx->args[i]);
        if (rc != CLI_OK && ctx->json) { cli_json_object(ctx, NULL); cli_json_str(ctx, "input", ctx->args[i]); cli_json_int(ctx, "error", rc); cli_json_close(ctx); }
        if (rc > worst) worst = rc;
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

/* --------------------------------------------------------------- inspect */

static const cli_opt inspect_opts[] = {
    { "size", 0, CLI_INT, "N", "the sprite block in pixels", "16" },
    { "bpp", 0, CLI_INT, "N", "bits per pixel", "4" },
};

static int run_inspect(cli_ctx *ctx)
{
    int worst = CLI_OK;
    sprite_ctx = ctx;
    int size = cli_int(ctx, "size", 16), bpp = cli_int(ctx, "bpp", 4);
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "sheets"); }
    for (int i = 0; i < ctx->nargs; i++) {
        const char *in = ctx->args[i];
        sprite_file = in;
        char inbase[1024];
        const char *type;
        if (!split_input(ctx, in, inbase, sizeof inbase, &type)) { worst = CLI_REFUSED; continue; }
        image_load(inbase, type, &snesimage, true);
        int w = (int)snesimage.header.width, h = (int)snesimage.header.height;
        int blksx = w / size + (w % size ? 1 : 0), blksy = h / size + (h % size ? 1 : 0);
        int used[256] = { 0 }, ncol = 0;
        for (long k = 0; k < (long)w * h; k++) if (!used[snesimage.buffer[k]]) { used[snesimage.buffer[k]] = 1; ncol++; }
        int maxidx = 0;
        for (int k = 255; k >= 0; k--) if (used[k]) { maxidx = k; break; }
        /* what `sheet` writes: blocks laid out in 128-px OBJ raster rows, (size/8) tile rows of 16 tiles each */
        int per_row = 128 / size, rows = (blksx * blksy + per_row - 1) / per_row;
        long tiles = (long)rows * (size / 8) * 16, vram = tiles * 8 * bpp;
        free(snesimage.buffer); snesimage.buffer = NULL;
        if (ctx->json) {
            cli_json_object(ctx, NULL); cli_json_str(ctx, "input", in);
            cli_json_int(ctx, "width", w); cli_json_int(ctx, "height", h); cli_json_int(ctx, "block", size);
            cli_json_int(ctx, "blocks", (long)blksx * blksy); cli_json_int(ctx, "tiles", tiles); cli_json_int(ctx, "vram_bytes", vram);
            cli_json_int(ctx, "colors_used", ncol); cli_json_int(ctx, "highest_index", maxidx);
            cli_json_bool(ctx, "fits_block_grid", !(w % size) && !(h % size));
            cli_json_close(ctx);
        } else {
            printf("%s: %dx%d px, %d blocks of %d (%ld tiles, %ld bytes of VRAM at %d bpp), %d colours used, highest index %d%s\n",
                   in, w, h, blksx * blksy, size, tiles, vram, bpp, ncol, maxidx,
                   (w % size || h % size) ? " — not a multiple of the block size" : "");
        }
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

/* ------------------------------------------------------------------ main */

static const cli_cmd cmds[] = {
    { "sheet", "sprite sheet (indexed PNG or BMP) -> .pic tiles in OBJ VRAM order, .pal, .inc, and the _meta.inc metasprite table",
      sheet_opts, CLI_N(sheet_opts), "sheet res/hero.png --size 16 --colors 16 --metasprite 32 48 --priority 2 --save", 1, run_sheet },
    { "anim", "Aseprite --data --list-tags export -> <stem>_anim.h, one AnimClip per tag",
      anim_opts, CLI_N(anim_opts), "anim res/hero.json --prefix hero", 1, run_anim },
    { "inspect", "what a sheet holds and costs (blocks, tiles, VRAM bytes, colours)", inspect_opts, CLI_N(inspect_opts),
      "inspect res/*.png --size 16", 1, run_inspect },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes-sprite", TOOL_VERSION, "sprite sheets and Aseprite exports -> tiles, palette, metasprites, animation clips",
        cmds, CLI_N(cmds),
    };
    return cli_main(&tool, argc, argv);
}
