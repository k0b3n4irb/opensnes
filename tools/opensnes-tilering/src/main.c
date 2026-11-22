/*
 * opensnes-tilering — a map picture whose 8x8 characters exceed VRAM, for
 * the `tilering` library module.
 *
 *   opensnes-tilering convert [--window WxH] [--no-mirrors] [--priority] [--out DIR] [--save] [--json] <map.png>...
 *   opensnes-tilering inspect [--window WxH] [--no-mirrors] [--json] <map.png>...
 *
 * A background tilemap entry names 1,024 characters at most. A map larger
 * than that is still drawable if the characters of the screen in view fit:
 * the `tilering` module keeps a ring of character slots in VRAM and loads
 * the characters of each row and column that comes into view. This tool
 * makes what it reads from an indexed PNG (16 colours a palette, up to 8
 * palettes: a pixel's index / 16 is its palette, index % 16 its colour):
 *
 *   <stem>.chr   the characters, 32 bytes each (4 bpp), each 8x8 cell kept
 *                once, a cell equal to a known character mirrored
 *                (horizontally, vertically or both) merged with it; numbered
 *                in order of first appearance, row by row from the BOTTOM of
 *                the map, left to right: scrolling up, the characters of a
 *                new row follow each other in ROM
 *   <stem>.trm   the map: one 16-bit word a cell, row after row, in parts of
 *                at most 32 KB holding whole rows (a part never splits a row)
 *                  bits 0-11 character (0-4095), bit 12 0,
 *                  bit 13 priority, bit 14 horizontal flip, bit 15 vertical flip
 *   <stem>.tpl   one byte a character: its palette, already shifted to the
 *                tilemap entry's high byte (palette << 2)
 *   <stem>.trh   24 bytes: what the module reads (sizes, the window the
 *                worst figures were measured for, the figures)
 *   <stem>.pal   the PNG's palette, BGR555 (2 bytes a colour)
 *   <stem>.inc, <stem>_data.as   the C declarations and the ASSET_SECTIONs
 *
 * The worst figures are measured over every position of a window of WxH
 * characters (33x29 by default: what a 256x224 screen shows at any scroll):
 *   worst window  distinct characters in view at once (what the ring must hold)
 *   worst row     characters of a row entering the window that no other row
 *                 of the window holds (what one vertical step may load)
 *   worst column  the same for a column entering from the left or the right
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"
#include "incfile.h"
#include "lodepng.h"
#include "xalloc.h"

#define MAX_CHARS   4096
#define PART_BYTES  32768L
#define HDR_BYTES   24

typedef struct {
    int width, height;           /* in characters */
    int nchars, nchars_plain;    /* with the mirrors merged; without */
    int win_w, win_h;
    int worst_window, worst_row, worst_col;
    int worst_window_x, worst_window_y;
    int rows_per_part, map_parts, char_parts;
    int palettes;                /* palettes used (highest + 1) */
    uint16_t *map;               /* width * height entries */
    uint8_t (*chr)[32];          /* nchars characters, 4 bpp planar */
    uint8_t *charpal;            /* nchars bytes */
    uint16_t pal[128];           /* BGR555 */
    int ncolors;
} tr_result;

static void tr_free(tr_result *r)
{
    free(r->map); free(r->chr); free(r->charpal);
    memset(r, 0, sizeof *r);
}

/* ------------------------------------------------------------ dedup */

typedef struct { uint8_t px[64]; uint8_t pal; } cell_t;

static uint32_t cell_hash(const cell_t *c)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < 64; i++) { h ^= c->px[i]; h *= 16777619u; }
    h ^= c->pal; h *= 16777619u;
    return h;
}

static void cell_flip(const cell_t *in, cell_t *out, int flip)
{
    out->pal = in->pal;
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int sx = (flip & 1) ? 7 - x : x, sy = (flip & 2) ? 7 - y : y;
            out->px[y * 8 + x] = in->px[sy * 8 + sx];
        }
}

typedef struct {
    cell_t *cells;     /* the characters, by number */
    int    *slot;      /* open addressing: character number + 1, 0 = empty */
    int     size, n;
} table_t;

static int table_find(const table_t *t, const cell_t *c)
{
    uint32_t i = cell_hash(c) & (uint32_t)(t->size - 1);
    while (t->slot[i]) {
        const cell_t *k = &t->cells[t->slot[i] - 1];
        if (k->pal == c->pal && !memcmp(k->px, c->px, 64)) return t->slot[i] - 1;
        i = (i + 1) & (uint32_t)(t->size - 1);
    }
    return -1;
}

static int table_add(table_t *t, const cell_t *c, int cap)
{
    if (t->n >= cap) return -1;
    uint32_t i = cell_hash(c) & (uint32_t)(t->size - 1);
    while (t->slot[i]) i = (i + 1) & (uint32_t)(t->size - 1);
    t->cells[t->n] = *c;
    t->slot[i] = ++t->n;
    return t->n - 1;
}

static void encode_4bpp(const cell_t *c, uint8_t out[32])
{
    memset(out, 0, 32);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int v = c->px[y * 8 + x], bit = 7 - x;
            if (v & 1) out[y * 2]      |= (uint8_t)(1 << bit);
            if (v & 2) out[y * 2 + 1]  |= (uint8_t)(1 << bit);
            if (v & 4) out[16 + y * 2] |= (uint8_t)(1 << bit);
            if (v & 8) out[17 + y * 2] |= (uint8_t)(1 << bit);
        }
}

/* ------------------------------------------------------------ the worst figures */

/* Characters of the cells (x0, y0) + k * (dx, dy), k < n, that the window
 * counts (cnt[]) hold only in those cells. */
static int entering(const uint16_t *map, int width, const int *cnt, int *seen,
                    int x0, int y0, int dx, int dy, int n)
{
    int fresh = 0;
    for (int k = 0; k < n; k++) seen[map[(y0 + k * dy) * width + x0 + k * dx] & 0x0FFF]++;
    for (int k = 0; k < n; k++) {
        int c = map[(y0 + k * dy) * width + x0 + k * dx] & 0x0FFF;
        if (seen[c] > 0) {
            if (cnt[c] == seen[c]) fresh++;
            seen[c] = -seen[c];               /* counted once */
        }
    }
    for (int k = 0; k < n; k++) seen[map[(y0 + k * dy) * width + x0 + k * dx] & 0x0FFF] = 0;
    return fresh;
}

static void measure(tr_result *r)
{
    int W = r->win_w, H = r->win_h, w = r->width;
    int *cnt = xcalloc((size_t)r->nchars, sizeof *cnt);
    int *seen = xcalloc((size_t)r->nchars, sizeof *seen);
    r->worst_window = r->worst_row = r->worst_col = 0;
    for (int y = 0; y + H <= r->height; y++) {
        int distinct = 0;
        memset(cnt, 0, (size_t)r->nchars * sizeof *cnt);
        for (int yy = y; yy < y + H; yy++)
            for (int xx = 0; xx < W; xx++)
                if (cnt[r->map[yy * w + xx] & 0x0FFF]++ == 0) distinct++;
        for (int x = 0; ; x++) {
            if (distinct > r->worst_window) { r->worst_window = distinct; r->worst_window_x = x; r->worst_window_y = y; }
            int n;
            n = entering(r->map, w, cnt, seen, x, y, 1, 0, W);          if (n > r->worst_row) r->worst_row = n;   /* up */
            n = entering(r->map, w, cnt, seen, x, y + H - 1, 1, 0, W);  if (n > r->worst_row) r->worst_row = n;   /* down */
            n = entering(r->map, w, cnt, seen, x, y, 0, 1, H);          if (n > r->worst_col) r->worst_col = n;   /* left */
            n = entering(r->map, w, cnt, seen, x + W - 1, y, 0, 1, H);  if (n > r->worst_col) r->worst_col = n;   /* right */
            if (x + W >= w) break;
            for (int yy = y; yy < y + H; yy++) {
                if (--cnt[r->map[yy * w + x] & 0x0FFF] == 0) distinct--;
                if (cnt[r->map[yy * w + x + W] & 0x0FFF]++ == 0) distinct++;
            }
        }
    }
    free(cnt); free(seen);
}

/* ------------------------------------------------------------ conversion */

static int parse_window(cli_ctx *ctx, const char *in, int *ww, int *wh)
{
    const char *s = cli_str(ctx, "window", "33x29");
    if (sscanf(s, "%dx%d", ww, wh) != 2 || *ww < 1 || *wh < 1 || *ww > 64 || *wh > 32) {
        cli_error(ctx, in, "--window %s: WxH in characters, at most 64x32 (the module keeps a 64x32 tilemap)", s);
        return 0;
    }
    return 1;
}

static int convert_png(cli_ctx *ctx, const char *in, tr_result *r)
{
    memset(r, 0, sizeof *r);
    if (!parse_window(ctx, in, &r->win_w, &r->win_h)) return CLI_REFUSED;
    size_t len;
    unsigned char *buf = cli_read_file(ctx, in, &len);
    if (!buf) return CLI_IO;
    LodePNGState st;
    lodepng_state_init(&st);
    st.info_raw.colortype = LCT_PALETTE; st.info_raw.bitdepth = 8;
    unsigned char *px = NULL;
    unsigned w, h;
    unsigned err = lodepng_inspect(&w, &h, &st, buf, len);
    if (!err && st.info_png.color.colortype != LCT_PALETTE) {
        cli_error(ctx, in, "not an indexed PNG: save it with a palette (16 colours a palette, index / 16 = the palette)");
        free(buf); lodepng_state_cleanup(&st); return CLI_REFUSED;
    }
    if (!err) err = lodepng_decode(&px, &w, &h, &st, buf, len);
    free(buf);
    if (err) { cli_error(ctx, in, "not a PNG this tool reads: %s", lodepng_error_text(err)); lodepng_state_cleanup(&st); return CLI_REFUSED; }
    r->ncolors = (int)st.info_png.color.palettesize;
    if (r->ncolors > 128) r->ncolors = 128;
    for (int i = 0; i < r->ncolors; i++) {
        const unsigned char *c = st.info_png.color.palette + i * 4;
        r->pal[i] = (uint16_t)((c[0] >> 3) | ((c[1] >> 3) << 5) | ((c[2] >> 3) << 10));
    }
    lodepng_state_cleanup(&st);

    int rc = CLI_REFUSED;
    if (w % 8 || h % 8) { cli_error(ctx, in, "%ux%u pixels: the width and the height must be multiples of 8", w, h); free(px); return rc; }
    r->width = (int)w / 8; r->height = (int)h / 8;
    if (r->width < r->win_w || r->height < r->win_h) {
        cli_error(ctx, in, "%dx%d characters is smaller than the window (%dx%d): a map that fits the screen needs no ring", r->width, r->height, r->win_w, r->win_h);
        free(px); return rc;
    }
    if (2L * r->width > PART_BYTES) { cli_error(ctx, in, "%d characters wide: a row must fit a 32 KB part (16384 at most)", r->width); free(px); return rc; }

    int ncells = r->width * r->height, mirrors = !cli_has(ctx, "no-mirrors");
    table_t t = { xmalloc(sizeof(cell_t) * MAX_CHARS), NULL, 1, 0 };
    while (t.size < 4 * MAX_CHARS) t.size <<= 1;
    t.slot = xcalloc((size_t)t.size, sizeof *t.slot);
    table_t plain = { xmalloc(sizeof(cell_t) * MAX_CHARS), xcalloc((size_t)t.size, sizeof(int)), t.size, 0 };
    r->map = xmalloc(sizeof(uint16_t) * (size_t)ncells);
    uint16_t prio = cli_has(ctx, "priority") ? 0x2000 : 0;
    int plain_over = 0;

    for (int cy = r->height - 1; cy >= 0; cy--)
        for (int cx = 0; cx < r->width; cx++) {
            cell_t c;
            int pal = -1;
            for (int y = 0; y < 8; y++)
                for (int x = 0; x < 8; x++) {
                    int v = px[(size_t)(cy * 8 + y) * w + cx * 8 + x];
                    if (v >= 128) {
                        cli_error(ctx, in, "pixel (%d, %d) uses colour %d: 8 palettes of 16 colours, indices 0-127", cx * 8 + x, cy * 8 + y, v);
                        goto out;
                    }
                    c.px[y * 8 + x] = (uint8_t)(v & 15);
                    if (v & 15) {
                        if (pal >= 0 && pal != v >> 4) {
                            cli_error(ctx, in, "the character at (%d, %d) mixes palettes %d and %d: one palette per 8x8 cell (colour 0 of any palette is transparent)", cx * 8, cy * 8, pal, v >> 4);
                            goto out;
                        }
                        pal = v >> 4;
                    }
                }
            c.pal = (uint8_t)(pal < 0 ? 0 : pal);
            if (c.pal + 1 > r->palettes) r->palettes = c.pal + 1;
            if (!plain_over && table_find(&plain, &c) < 0 && table_add(&plain, &c, MAX_CHARS) < 0) plain_over = 1;
            int found = -1, flip = 0;
            for (int f = 0; f < (mirrors ? 4 : 1) && found < 0; f++) {
                cell_t m;
                cell_flip(&c, &m, f);
                found = table_find(&t, &m);
                flip = f;
            }
            if (found < 0) {
                flip = 0;
                found = table_add(&t, &c, MAX_CHARS);
                if (found < 0) {
                    cli_error(ctx, in, "more than %d distinct characters (row %d from the top still to read): the map entry holds 12 bits", MAX_CHARS, cy);
                    goto out;
                }
            }
            r->map[cy * r->width + cx] = (uint16_t)(found | prio | ((flip & 1) ? 0x4000 : 0) | ((flip & 2) ? 0x8000 : 0));
        }

    r->nchars = t.n;
    r->nchars_plain = plain_over ? -1 : plain.n;
    r->chr = xmalloc(32 * (size_t)t.n);
    r->charpal = xmalloc((size_t)t.n);
    for (int i = 0; i < t.n; i++) {
        encode_4bpp(&t.cells[i], r->chr[i]);
        r->charpal[i] = (uint8_t)(t.cells[i].pal << 2);
    }
    r->rows_per_part = (int)(PART_BYTES / (2L * r->width));
    if (r->rows_per_part > r->height) r->rows_per_part = r->height;
    r->map_parts = (r->height + r->rows_per_part - 1) / r->rows_per_part;
    r->char_parts = (t.n + 1023) / 1024;
    measure(r);
    rc = CLI_OK;
out:
    free(px);
    free(t.cells); free(t.slot); free(plain.cells); free(plain.slot);
    if (rc != CLI_OK) tr_free(r);
    return rc;
}

static void put16(uint8_t *p, int v) { p[0] = (uint8_t)(v & 255); p[1] = (uint8_t)((v >> 8) & 255); }

static int write_outputs(cli_ctx *ctx, const char *in, const char *outbase, const char *ident, const tr_result *r)
{
    char path[1200];
    int rc;

    /* the map, each part a whole number of rows, padded to 32 KB but the last */
    size_t row_bytes = 2 * (size_t)r->width;
    size_t map_len = (size_t)(r->map_parts - 1) * PART_BYTES + row_bytes * (size_t)(r->height - (r->map_parts - 1) * r->rows_per_part);
    uint8_t *m = xcalloc(map_len, 1);
    for (int y = 0; y < r->height; y++) {
        size_t at = (size_t)(y / r->rows_per_part) * PART_BYTES + (size_t)(y % r->rows_per_part) * row_bytes;
        for (int x = 0; x < r->width; x++) put16(m + at + 2 * (size_t)x, r->map[y * r->width + x]);
    }
    snprintf(path, sizeof path, "%s.trm", outbase);
    rc = cli_write_file(ctx, path, m, map_len);
    free(m);
    if (rc != CLI_OK) return rc;

    snprintf(path, sizeof path, "%s.chr", outbase);
    if ((rc = cli_write_file(ctx, path, r->chr, 32 * (size_t)r->nchars)) != CLI_OK) return rc;
    snprintf(path, sizeof path, "%s.tpl", outbase);
    if ((rc = cli_write_file(ctx, path, r->charpal, (size_t)r->nchars)) != CLI_OK) return rc;

    uint8_t hdr[HDR_BYTES] = { 0 };
    put16(hdr + 0, 0x5254);             /* "TR" */
    put16(hdr + 2, 1);
    put16(hdr + 4, r->width);  put16(hdr + 6, r->height);
    put16(hdr + 8, r->nchars); put16(hdr + 10, r->rows_per_part);
    put16(hdr + 12, r->win_w); put16(hdr + 14, r->win_h);
    put16(hdr + 16, r->worst_row); put16(hdr + 18, r->worst_col);
    put16(hdr + 20, r->worst_window);
    snprintf(path, sizeof path, "%s.trh", outbase);
    if ((rc = cli_write_file(ctx, path, hdr, sizeof hdr)) != CLI_OK) return rc;

    uint8_t pal[256];
    for (int i = 0; i < r->ncolors; i++) put16(pal + 2 * i, r->pal[i]);
    snprintf(path, sizeof path, "%s.pal", outbase);
    if ((rc = cli_write_file(ctx, path, pal, 2 * (size_t)(r->ncolors ? r->ncolors : 1))) != CLI_OK) return rc;

    /* the glue */
    snprintf(path, sizeof path, "%s.inc", outbase);
    FILE *f = fopen(path, "w");
    if (!f) { cli_error(ctx, in, "cannot write %s", path); return CLI_IO; }
    fprintf(f, "/* %s.inc — generated by %s %s; do not edit. The data labels live in %s_data.as,\n"
               " * which the build gathers into assets_gen.asm (docs/tools/CONVENTIONS.md).\n"
               " * Include it from one C file: it defines the two tables of parts. */\n"
               "#ifndef ASSET_%s_INC\n#define ASSET_%s_INC\n\n#include <snes/types.h>\n\n",
            ident, ctx->tool->name, ctx->tool->version, ident, ident, ident);
    fprintf(f, "/* %dx%d characters, %d distinct (mirrors %s); window %dx%d: worst %d in view, %d a row, %d a column */\n",
            r->width, r->height, r->nchars, cli_has(ctx, "no-mirrors") ? "kept apart" : "merged", r->win_w, r->win_h,
            r->worst_window, r->worst_row, r->worst_col);
    fprintf(f, "#define %s_WIDTH         %d\n#define %s_HEIGHT        %d\n#define %s_CHARS         %d\n",
            ident, r->width, ident, r->height, ident, r->nchars);
    fprintf(f, "#define %s_WORST_WINDOW  %d\n#define %s_WORST_ROW     %d\n#define %s_WORST_COLUMN  %d\n\n",
            ident, r->worst_window, ident, r->worst_row, ident, r->worst_col);
    fprintf(f, "extern const u8 %s_header[], %s_header_end[];     /* .trh: what tileRingInit() reads */\n", ident, ident);
    fprintf(f, "extern const u8 %s_charpal[], %s_charpal_end[];   /* .tpl: one byte a character, its palette << 2 */\n", ident, ident);
    fprintf(f, "extern const u8 %s_pal[], %s_pal_end[];           /* .pal: %d colours, BGR555 */\n", ident, ident, r->ncolors);
    for (int i = 0; i < r->char_parts; i++)
        fprintf(f, "extern const u8 %s_chars%s%.0d[];\n", ident, i ? "_" : "", i);
    for (int i = 0; i < r->map_parts; i++)
        fprintf(f, "extern const u8 %s_map%s%.0d[];\n", ident, i ? "_" : "", i);
    fprintf(f, "\n/* 1,024 characters (32 KB) a part; %d rows of the map a part */\n", r->rows_per_part);
    fprintf(f, "static const u8 *const %s_char_parts[%d] = {", ident, r->char_parts);
    for (int i = 0; i < r->char_parts; i++) fprintf(f, "%s %s_chars%s%.0d", i ? "," : "", ident, i ? "_" : "", i);
    fprintf(f, " };\nstatic const u8 *const %s_map_parts[%d] = {", ident, r->map_parts);
    for (int i = 0; i < r->map_parts; i++) fprintf(f, "%s %s_map%s%.0d", i ? "," : "", ident, i ? "_" : "", i);
    fprintf(f, " };\n\n/* static const TileRingMap map = %s_TILERING_MAP;  then  tileRingInit(&ring, &map, ...) */\n", ident);
    fprintf(f, "#define %s_TILERING_MAP { %s_header, %s_charpal, %s_char_parts, %s_map_parts }\n\n#endif\n",
            ident, ident, ident, ident, ident);
    fclose(f);

    snprintf(path, sizeof path, "%s_data.as", outbase);
    f = fopen(path, "w");
    if (!f) { cli_error(ctx, in, "cannot write %s", path); return CLI_IO; }
    fprintf(f, "; %s_data.as — generated by %s %s; one ASSET_SECTION per blob, .include it from assets_gen.asm (the build does)\n\n",
            ident, ctx->tool->name, ctx->tool->version);
    const char *exts[] = { ".trh", ".tpl", ".pal", ".chr", ".trm" };
    const char *whats[] = { "header", "charpal", "pal", "chars", "map" };
    char blob[1300];
    for (int i = 0; i < 5; i++) {
        snprintf(blob, sizeof blob, "%s%s", outbase, exts[i]);
        /* the .incbin path is relative to the project: the build runs from there */
        incfile_write_blob(f, ident, whats[i], blob);
    }
    fclose(f);
    return CLI_OK;
}

static void report(cli_ctx *ctx, const char *in, const tr_result *r, const char *outbase)
{
    long chr_bytes = 32L * r->nchars;
    if (ctx->json) {
        cli_json_object(ctx, NULL);
        cli_json_str(ctx, "input", in);
        cli_json_int(ctx, "width", r->width); cli_json_int(ctx, "height", r->height);
        cli_json_int(ctx, "cells", (long)r->width * r->height);
        cli_json_int(ctx, "characters", r->nchars);
        cli_json_int(ctx, "characters_without_mirrors", r->nchars_plain);
        cli_json_int(ctx, "palettes", r->palettes);
        cli_json_int(ctx, "character_bytes", chr_bytes);
        cli_json_int(ctx, "map_bytes", 2L * r->width * r->height);
        cli_json_object(ctx, "window");
        cli_json_int(ctx, "width", r->win_w); cli_json_int(ctx, "height", r->win_h);
        cli_json_int(ctx, "worst_characters", r->worst_window);
        cli_json_int(ctx, "worst_at_x", r->worst_window_x); cli_json_int(ctx, "worst_at_y", r->worst_window_y);
        cli_json_int(ctx, "worst_row", r->worst_row);
        cli_json_int(ctx, "worst_column", r->worst_col);
        cli_json_close(ctx);
        if (outbase) {
            char path[1200];
            const char *exts[] = { ".chr", ".trm", ".tpl", ".trh", ".pal", ".inc", "_data.as" };
            cli_json_array(ctx, "outputs");
            for (int i = 0; i < 7; i++) {
                snprintf(path, sizeof path, "%s%s", outbase, exts[i]);
                cli_json_object(ctx, NULL); cli_json_str(ctx, "path", path); cli_json_close(ctx);
            }
            cli_json_close(ctx);
        }
        cli_json_close(ctx);
    } else if (!ctx->quiet) {
        printf("%s: %dx%d characters, %d distinct", in, r->width, r->height, r->nchars);
        if (r->nchars_plain >= 0 && r->nchars_plain != r->nchars) printf(" (%d without merging mirrors)", r->nchars_plain);
        printf(", %ld bytes; window %dx%d: worst %d in view (at %d, %d), %d new a row, %d a column\n",
               chr_bytes, r->win_w, r->win_h, r->worst_window, r->worst_window_x, r->worst_window_y, r->worst_row, r->worst_col);
    }
}

/* --------------------------------------------------------------- convert */
#define OPT_WINDOW  { "window", 0, CLI_STR, "WxH", "the window the worst figures are measured for, in characters", "33x29" }
#define OPT_MIRRORS { "no-mirrors", 0, CLI_FLAG, NULL, "keep a mirrored cell apart instead of merging it with its character", NULL }
static const cli_opt convert_opts[] = {
    OPT_WINDOW, OPT_MIRRORS,
    { "priority", 0, CLI_FLAG, NULL, "set the priority bit of every cell", NULL },
    CLI_OPT_OUT,
    CLI_OPT_SAVE,
};
static const cli_opt inspect_opts[] = { OPT_WINDOW, OPT_MIRRORS };

static int convert_one(cli_ctx *ctx, const char *in)
{
    int rc = cli_load_settings(ctx, in);
    if (rc != CLI_OK) return rc;
    char outbase[1024], ident[256];
    if (cli_output_path(ctx, in, "", outbase, sizeof outbase) != CLI_OK) return CLI_IO;
    cli_ident(in, ident, sizeof ident);
    tr_result r;
    if ((rc = convert_png(ctx, in, &r)) != CLI_OK) return rc;
    rc = write_outputs(ctx, in, outbase, ident, &r);
    if (rc == CLI_OK && cli_has(ctx, "save")) rc = cli_save_settings(ctx, in);
    if (rc == CLI_OK) report(ctx, in, &r, outbase);
    tr_free(&r);
    return rc;
}

static int run_each(cli_ctx *ctx, int convert)
{
    int worst = CLI_OK;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "maps"); }
    for (int i = 0; i < ctx->nargs; i++) {
        int rc;
        if (convert) rc = convert_one(ctx, ctx->args[i]);
        else {
            tr_result r;
            rc = cli_load_settings(ctx, ctx->args[i]);
            if (rc == CLI_OK && (rc = convert_png(ctx, ctx->args[i], &r)) == CLI_OK) { report(ctx, ctx->args[i], &r, NULL); tr_free(&r); }
        }
        if (rc != CLI_OK && ctx->json) { cli_json_object(ctx, NULL); cli_json_str(ctx, "input", ctx->args[i]); cli_json_int(ctx, "error", rc); cli_json_close(ctx); }
        if (rc > worst) worst = rc;
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

static int run_convert(cli_ctx *ctx) { return run_each(ctx, 1); }
static int run_inspect(cli_ctx *ctx) { return run_each(ctx, 0); }

static const cli_cmd cmds[] = {
    { "convert", "indexed PNG -> characters (.chr, mirrors merged), 16-bit map (.trm), palettes (.tpl, .pal), header (.trh) for the tilering module",
      convert_opts, CLI_N(convert_opts), "convert res/world.png --save", 1, run_convert },
    { "inspect", "what a map picture holds: distinct characters, the worst window, row and column", inspect_opts, CLI_N(inspect_opts), "inspect res/world.png --window 33x29", 1, run_inspect },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes-tilering", TOOL_VERSION, "maps with more characters than VRAM holds -> data for the tilering module", cmds, CLI_N(cmds),
    };
    return cli_main(&tool, argc, argv);
}
