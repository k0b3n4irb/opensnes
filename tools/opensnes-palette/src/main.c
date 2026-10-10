/*
 * opensnes-palette — a project's palettes (the 1.x successor of palplan
 * and img2snes, whose planner and quantizer are its own).
 *
 *   opensnes-palette plan     [--bg FILES] [--sprite FILES] [--hints N] [--cgram] [--out DIR] [--save] [--json] <palettes.toml>
 *   opensnes-palette quantize [--colors N] [--round] [--palette FILE] [--scale PERCENT] [--align N] [--out DIR] [--save] [--json] <art.png>...
 *   opensnes-palette inspect  [--json] <file.pal>...
 *
 * `plan` is a composed asset: palettes.toml names the project's .pal files
 * (bg = [...], sprite = [...], relative to the file) and the plan places
 * them in the SNES's 8 background and 8 sprite slots of 16 colours (CGRAM
 * 0..127 and 128..255): identical palettes share a slot, every palette
 * gets its CGRAM index and slot number as macros in <stem>.inc, a ninth
 * distinct palette in a region is refused, and near-duplicates are named
 * as merge candidates. --cgram also writes the 512-byte CGRAM image.
 *
 * `quantize` turns RGB or RGBA art into the indexed PNG the picture tools
 * eat (median cut, or the colours of a given .pal / PNG), rounded to the
 * SNES's 15-bit colour with --round; `inspect` reads .pal files.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "incfile.h"
#include "lodepng.h"
#include "quantize.h"   /* tools/img2snes/src */
#include "scale.h"

/* ================================================================== plan */

#define MAX_PALS    128
#define SLOT_COLORS 16
#define NUM_SLOTS   8
#define SPR_BASE    128

typedef struct {
    char name[64];            /* the identifier: the .pal's stem */
    int type;                 /* 0 bg, 1 sprite */
    char file[1100];
    uint16_t colors[SLOT_COLORS];
    int ncolors;
    int slot, cgram;          /* -1 while unplaced */
    int merged_into;          /* the owner it shares a slot with, -1 = owner */
} pal_t;

static pal_t pals[MAX_PALS];
static int npals, owners[2];
static const char *TYPE_NAME[2] = { "bg", "sprite" };

static int ends_with(const char *s, const char *suffix)
{
    size_t ls = strlen(s), lx = strlen(suffix);
    return ls > lx && strcmp(s + ls - lx, suffix) == 0;
}

/* `word` as a path relative to the directory of `anchor` (absolute words stay). */
static void join_dir(const char *anchor, const char *word, char *out, size_t n)
{
    const char *slash = strrchr(anchor, '/');
    if (word[0] == '/' || !slash) snprintf(out, n, "%s", word);
    else snprintf(out, n, "%.*s%s", (int)(slash - anchor + 1), anchor, word);
}

static void upper_of(const char *s, char *out, size_t n)
{
    size_t i;
    for (i = 0; s[i] && i + 1 < n; i++) out[i] = (char)((s[i] >= 'a' && s[i] <= 'z') ? s[i] - 32 : s[i]);
    out[i] = '\0';
}

/* Read a .pal (raw little-endian BGR555 words, as every tool of the family writes them). */
static int load_pal(cli_ctx *ctx, pal_t *p)
{
    size_t len = 0;
    unsigned char *buf = cli_read_file(ctx, p->file, &len);
    if (!buf) return CLI_IO;
    if (len < 2 || (len & 1)) {
        cli_error(ctx, p->file, "%zu bytes — a .pal is a non-empty even number of bytes, one BGR555 word per colour", len);
        free(buf); return CLI_REFUSED;
    }
    if (len > SLOT_COLORS * 2) {
        cli_error(ctx, p->file, "%zu colours — a plan places 16-colour slots; a 256-colour (8 bpp) palette is the whole CGRAM and loads at 0 on its own",
                  len / 2);
        free(buf); return CLI_REFUSED;
    }
    p->ncolors = (int)(len / 2);
    for (int i = 0; i < p->ncolors; i++) p->colors[i] = (uint16_t)(buf[i * 2] | (buf[i * 2 + 1] << 8));
    free(buf);
    return CLI_OK;
}

static int identical(const pal_t *a, const pal_t *b)
{
    return a->ncolors == b->ncolors && memcmp(a->colors, b->colors, (size_t)a->ncolors * 2) == 0;
}

/* Differing indices over the common range plus the size difference. */
static int distance(const pal_t *a, const pal_t *b, int *diff_idx, int *ndiff)
{
    int common = a->ncolors < b->ncolors ? a->ncolors : b->ncolors, nd = 0;
    for (int i = 0; i < common; i++)
        if (a->colors[i] != b->colors[i]) { if (nd < SLOT_COLORS) diff_idx[nd] = i; nd++; }
    *ndiff = nd;
    return nd + abs(a->ncolors - b->ncolors);
}

static int add_pals(cli_ctx *ctx, const char *anchor, const char *listname, int type)
{
    char buf[4096];
    const char *words[CLI_MAX_ARGS];
    int n = cli_list(ctx, listname, buf, sizeof buf, words, CLI_MAX_ARGS);
    for (int i = 0; i < n; i++) {
        if (npals >= MAX_PALS) { cli_error(ctx, anchor, "more than %d palettes", MAX_PALS); return CLI_REFUSED; }
        pal_t *p = &pals[npals];
        memset(p, 0, sizeof *p);
        p->type = type; p->slot = p->cgram = p->merged_into = -1;
        join_dir(anchor, words[i], p->file, sizeof p->file);
        cli_ident(words[i], p->name, sizeof p->name);
        for (int k = 0; k < npals; k++)
            if (strcmp(pals[k].name, p->name) == 0) {
                cli_error(ctx, anchor, "two palettes named %s (%s and %s) — the macros are named after the file", p->name, pals[k].file, p->file);
                return CLI_REFUSED;
            }
        int rc = load_pal(ctx, p);
        if (rc != CLI_OK) return rc;
        npals++;
    }
    return CLI_OK;
}

/* Merge the identical palettes of a region, then give every owner the next slot. */
static int make_plan(cli_ctx *ctx, const char *anchor)
{
    for (int i = 0; i < npals; i++) {
        if (pals[i].merged_into != -1) continue;
        for (int j = i + 1; j < npals; j++)
            if (pals[j].merged_into == -1 && pals[j].type == pals[i].type && identical(&pals[i], &pals[j]))
                pals[j].merged_into = i;
    }
    int next[2] = { 0, 0 }, over[2] = { 0, 0 };
    owners[0] = owners[1] = 0;
    for (int i = 0; i < npals; i++) {
        if (pals[i].merged_into != -1) continue;
        int t = pals[i].type;
        owners[t]++;
        if (next[t] >= NUM_SLOTS) { over[t]++; continue; }
        pals[i].slot = next[t]++;
        pals[i].cgram = (t ? SPR_BASE : 0) + pals[i].slot * SLOT_COLORS;
    }
    for (int i = 0; i < npals; i++)
        if (pals[i].merged_into != -1) { pals[i].slot = pals[pals[i].merged_into].slot; pals[i].cgram = pals[pals[i].merged_into].cgram; }
    for (int t = 0; t < 2; t++)
        if (over[t])
            cli_error(ctx, anchor, "%d distinct %s palettes, %d slots (CGRAM %d..%d) — merge the near-duplicates the hints name, drop an unused palette, or move one to the other region",
                      owners[t], TYPE_NAME[t], NUM_SLOTS, t ? SPR_BASE : 0, t ? 255 : 127);
    return (over[0] || over[1]) ? CLI_REFUSED : CLI_OK;
}

static void print_region(int t)
{
    printf("%s palettes (%d / %d slots):\n", t ? "Sprite" : "BG", owners[t], NUM_SLOTS);
    for (int i = 0; i < npals; i++) {
        if (pals[i].type != t || pals[i].merged_into != -1) continue;
        printf("  slot %d  cgram %3d  %-16s (%2d colours)  %s\n", pals[i].slot, pals[i].cgram, pals[i].name, pals[i].ncolors, pals[i].file);
        for (int j = 0; j < npals; j++)
            if (pals[j].merged_into == i) printf("      + %s shares this slot (identical palette)\n", pals[j].name);
    }
}

/* The merge hints, printed or as JSON. Returns how many. */
static int hints(cli_ctx *ctx, int threshold, int json)
{
    int count = 0;
    for (int i = 0; i < npals; i++) {
        if (pals[i].merged_into != -1) continue;
        for (int j = i + 1; j < npals; j++) {
            if (pals[j].merged_into != -1 || pals[j].type != pals[i].type) continue;
            int diff_idx[SLOT_COLORS], nd;
            int d = distance(&pals[i], &pals[j], diff_idx, &nd);
            if (d == 0 || d > threshold) continue;
            int only_transparent = pals[i].type == 1 && nd == 1 && diff_idx[0] == 0 && pals[i].ncolors == pals[j].ncolors;
            if (json) {
                cli_json_object(ctx, NULL);
                cli_json_str(ctx, "a", pals[i].name); cli_json_str(ctx, "b", pals[j].name);
                cli_json_int(ctx, "differing_colors", d); cli_json_bool(ctx, "only_transparent", only_transparent);
                cli_json_close(ctx);
            } else {
                if (count == 0) printf("Merge hints (near-duplicate palettes; a shared palette would free a slot):\n");
                if (only_transparent)
                    printf("  %s ~ %s differ only at colour 0, transparent for sprites — safe to merge into one palette\n", pals[i].name, pals[j].name);
                else {
                    printf("  %s ~ %s differ in %d colour%s", pals[i].name, pals[j].name, d, d == 1 ? "" : "s");
                    if (nd) { printf(" (index%s ", nd == 1 ? "" : "es"); for (int k = 0; k < nd && k < SLOT_COLORS; k++) printf("%s%d", k ? "," : "", diff_idx[k]); printf(")"); }
                    printf(" — consider a shared palette\n");
                }
            }
            count++;
        }
    }
    return count;
}

static const cli_opt plan_opts[] = {
    { "bg", 0, CLI_LIST, "FILES", "the background palettes (.pal), in slot order; relative to the settings file", NULL },
    { "sprite", 0, CLI_LIST, "FILES", "the sprite palettes (.pal), in slot order; relative to the settings file", NULL },
    { "hints", 0, CLI_INT, "N", "name two palettes that differ in N colours or fewer as a merge candidate (0 = never)", "2" },
    { "cgram", 0, CLI_FLAG, NULL, "also write <stem>.pal, the 512-byte CGRAM image of the whole plan, for one dmaCopyCGram", NULL },
    CLI_OPT_OUT,
    CLI_OPT_SAVE,
};

static int run_plan(cli_ctx *ctx)
{
    if (ctx->nargs != 1 || !ends_with(ctx->args[0], ".toml")) {
        cli_error(ctx, NULL, "plan takes one settings file, named after the plan: `plan res/palettes.toml` (with --bg / --sprite and --save to create it)");
        return CLI_USAGE;
    }
    const char *toml = ctx->args[0];
    int rc = cli_load_settings_path(ctx, toml);
    if (rc != CLI_OK) return rc;
    npals = 0;
    if ((rc = add_pals(ctx, toml, "bg", 0)) != CLI_OK || (rc = add_pals(ctx, toml, "sprite", 1)) != CLI_OK) return rc;
    if (npals == 0) { cli_error(ctx, toml, "no palettes: `bg = [\"a.pal\", ...]` and `sprite = [...]` under [plan], or --bg / --sprite"); return CLI_REFUSED; }
    int threshold = cli_int(ctx, "hints", 2), cgram = cli_has(ctx, "cgram");

    rc = make_plan(ctx, toml);
    if (rc != CLI_OK) { if (!ctx->json && !ctx->quiet) hints(ctx, threshold, 0); return rc; }

    char stem[256], ident[256], upper[256], inc_path[1100], pal_path[1100], as_path[1100];
    cli_stem(toml, stem, sizeof stem);
    cli_ident(toml, ident, sizeof ident);
    upper_of(ident, upper, sizeof upper);
    if (cli_output_path(ctx, toml, ".inc", inc_path, sizeof inc_path) != CLI_OK ||
        cli_output_path(ctx, toml, ".pal", pal_path, sizeof pal_path) != CLI_OK ||
        cli_output_path(ctx, toml, "_data.as", as_path, sizeof as_path) != CLI_OK) return CLI_IO;

    FILE *f = fopen(inc_path, "w");
    if (!f) { cli_error(ctx, inc_path, "cannot write"); return CLI_IO; }
    fprintf(f, "/* %s.inc — generated by %s %s from %s; do not edit.\n"
               " * The CGRAM plan: each palette's first colour index (the startColor of dmaCopyCGram) and its\n"
               " * slot (the OAM palette number, or the palette bank of a tilemap entry). Palettes with\n"
               " * identical colours share a slot. BG slots are CGRAM 0..127, sprite slots 128..255. */\n"
               "#ifndef ASSET_%s_INC\n#define ASSET_%s_INC\n\n",
            ident, ctx->tool->name, ctx->tool->version, toml + (strrchr(toml, '/') ? strrchr(toml, '/') - toml + 1 : 0), ident, ident);
    for (int i = 0; i < npals; i++) {
        char mac[80];
        upper_of(pals[i].name, mac, sizeof mac);
        fprintf(f, "/* %s (%s, %d colours) — %s slot %d%s */\n", pals[i].name, TYPE_NAME[pals[i].type], pals[i].ncolors,
                TYPE_NAME[pals[i].type], pals[i].slot, pals[i].merged_into != -1 ? " (shared)" : "");
        fprintf(f, "#define PAL_%s_CGRAM  %d\n#define PAL_%s_SLOT   %d\n#define PAL_%s_COLORS %d\n\n", mac, pals[i].cgram, mac, pals[i].slot, mac, pals[i].ncolors);
    }
    if (cgram) {
        fprintf(f, "#include <snes.h>\n\n/* the whole plan as one CGRAM image: dmaCopyCGram(%s_cgram, 0, %s_CGRAM_SIZE) */\n"
                   "extern const u8 %s_cgram[], %s_cgram_end[];\n#define %s_CGRAM_SIZE 512\n\n", ident, upper, ident, ident, upper);
    }
    fprintf(f, "#endif\n");
    fclose(f);

    if (cgram) {
        unsigned char img[512] = { 0 };
        for (int i = 0; i < npals; i++) {
            if (pals[i].merged_into != -1) continue;
            for (int c = 0; c < pals[i].ncolors; c++) {
                img[pals[i].cgram * 2 + c * 2] = (unsigned char)(pals[i].colors[c] & 0xFF);
                img[pals[i].cgram * 2 + c * 2 + 1] = (unsigned char)(pals[i].colors[c] >> 8);
            }
        }
        if ((rc = cli_write_file(ctx, pal_path, img, sizeof img)) != CLI_OK) return rc;
        f = fopen(as_path, "w");
        if (!f) { cli_error(ctx, as_path, "cannot write"); return CLI_IO; }
        fprintf(f, "; %s_data.as — generated by %s %s; one ASSET_SECTION per blob, .include it from assets_gen.asm (the build does)\n\n",
                ident, ctx->tool->name, ctx->tool->version);
        incfile_write_blob(f, ident, "cgram", pal_path);
        fclose(f);
    }
    if (cli_has(ctx, "save") && (rc = cli_save_settings_path(ctx, toml, stem)) != CLI_OK) return rc;

    if (ctx->json) {
        cli_json_begin(ctx);
        cli_json_str(ctx, "input", toml);
        cli_json_int(ctx, "palettes", npals); cli_json_int(ctx, "distinct", owners[0] + owners[1]);
        cli_json_int(ctx, "bg_slots_used", owners[0]); cli_json_int(ctx, "sprite_slots_used", owners[1]);
        cli_json_array(ctx, "plan");
        for (int i = 0; i < npals; i++) {
            cli_json_object(ctx, NULL);
            cli_json_str(ctx, "name", pals[i].name); cli_json_str(ctx, "type", TYPE_NAME[pals[i].type]); cli_json_str(ctx, "file", pals[i].file);
            cli_json_int(ctx, "colors", pals[i].ncolors); cli_json_int(ctx, "slot", pals[i].slot); cli_json_int(ctx, "cgram", pals[i].cgram);
            if (pals[i].merged_into != -1) cli_json_str(ctx, "shares_with", pals[pals[i].merged_into].name);
            cli_json_close(ctx);
        }
        cli_json_close(ctx);
        cli_json_array(ctx, "hints"); hints(ctx, threshold, 1); cli_json_close(ctx);
        cli_json_array(ctx, "outputs");
        cli_json_object(ctx, NULL); cli_json_str(ctx, "path", inc_path); cli_json_str(ctx, "kind", "header"); cli_json_close(ctx);
        if (cgram) {
            cli_json_object(ctx, NULL); cli_json_str(ctx, "path", pal_path); cli_json_str(ctx, "kind", "cgram"); cli_json_int(ctx, "bytes", 512); cli_json_close(ctx);
            cli_json_object(ctx, NULL); cli_json_str(ctx, "path", as_path); cli_json_str(ctx, "kind", "asm"); cli_json_close(ctx);
        }
        cli_json_close(ctx);
        cli_json_end(ctx);
    } else if (!ctx->quiet) {
        printf("%s: %s — %d palette%s, %d distinct\n", ctx->tool->name, toml, npals, npals == 1 ? "" : "s", owners[0] + owners[1]);
        print_region(0);
        print_region(1);
        hints(ctx, threshold, 0);
        printf("OK: fits in %d BG + %d sprite slots -> %s%s\n", NUM_SLOTS, NUM_SLOTS, inc_path, cgram ? " + .pal (512 bytes)" : "");
    }
    return CLI_OK;
}

/* ============================================================== quantize */

static unsigned char round_snes(unsigned char v)
{
    int r = ((int)v + 4) / 8 * 8;
    return (unsigned char)(r > 248 ? 248 : r);
}

/* The colours of a reference: a .pal (BGR555 words) or a PNG (its palette, or its distinct opaque colours). */
static int load_reference(cli_ctx *ctx, const char *path, rgb_t *palette, int max_colors)
{
    size_t len = 0;
    unsigned char *buf = cli_read_file(ctx, path, &len);
    if (!buf) return -1;
    if (ends_with(path, ".pal") || ends_with(path, ".PAL")) {
        int n = (int)(len / 2);
        if (n > max_colors) n = max_colors;
        for (int i = 0; i < n; i++) {
            unsigned c = buf[i * 2] | (buf[i * 2 + 1] << 8);
            palette[i].r = (unsigned char)((c & 31) << 3);
            palette[i].g = (unsigned char)(((c >> 5) & 31) << 3);
            palette[i].b = (unsigned char)(((c >> 10) & 31) << 3);
        }
        free(buf);
        return n;
    }
    LodePNGState st;
    lodepng_state_init(&st);
    st.info_raw.colortype = LCT_RGBA; st.info_raw.bitdepth = 8;
    unsigned char *pixels = NULL;
    unsigned w, h;
    unsigned err = lodepng_decode(&pixels, &w, &h, &st, buf, len);
    free(buf);
    if (err) { cli_error(ctx, path, "not a .pal or a PNG this tool reads: %s", lodepng_error_text(err)); lodepng_state_cleanup(&st); return -1; }
    int n = 0;
    if (st.info_png.color.colortype == LCT_PALETTE && st.info_png.color.palettesize > 0) {
        n = (int)st.info_png.color.palettesize;
        if (n > max_colors) n = max_colors;
        for (int i = 0; i < n; i++) {
            palette[i].r = st.info_png.color.palette[i * 4];
            palette[i].g = st.info_png.color.palette[i * 4 + 1];
            palette[i].b = st.info_png.color.palette[i * 4 + 2];
        }
    } else {
        for (size_t i = 0; i < (size_t)w * h && n < max_colors; i++) {
            const unsigned char *p = pixels + i * 4;
            if (p[3] < 128) continue;
            int found = 0;
            for (int j = 0; j < n && !found; j++) found = palette[j].r == p[0] && palette[j].g == p[1] && palette[j].b == p[2];
            if (!found) { palette[n].r = p[0]; palette[n].g = p[1]; palette[n].b = p[2]; n++; }
        }
    }
    free(pixels);
    lodepng_state_cleanup(&st);
    return n;
}

static int save_indexed(cli_ctx *ctx, const char *path, const unsigned char *indices, int w, int h, const rgb_t *palette, int n, int has_alpha)
{
    LodePNGState st;
    lodepng_state_init(&st);
    st.encoder.auto_convert = 0;
    st.info_png.color.colortype = LCT_PALETTE; st.info_png.color.bitdepth = 8;
    st.info_raw.colortype = LCT_PALETTE; st.info_raw.bitdepth = 8;
    for (int i = 0; i < n; i++) {
        unsigned char a = (has_alpha && i == 0) ? 0 : 255;
        lodepng_palette_add(&st.info_png.color, palette[i].r, palette[i].g, palette[i].b, a);
        lodepng_palette_add(&st.info_raw, palette[i].r, palette[i].g, palette[i].b, a);
    }
    unsigned char *out = NULL;
    size_t outsize = 0;
    unsigned err = lodepng_encode(&out, &outsize, indices, (unsigned)w, (unsigned)h, &st);
    lodepng_state_cleanup(&st);
    if (err) { cli_error(ctx, path, "cannot encode: %s", lodepng_error_text(err)); free(out); return CLI_IO; }
    int rc = cli_write_file(ctx, path, out, outsize);
    free(out);
    return rc;
}

static const cli_opt quantize_opts[] = {
    { "colors", 0, CLI_INT, "N", "colours of the result (2..256): 4 for 2 bpp, 16 for 4 bpp, 256 for 8 bpp", "16" },
    { "round", 0, CLI_FLAG, NULL, "round the palette to the SNES's 15-bit colour (multiples of 8), so the picture shows what the hardware will", NULL },
    { "palette", 0, CLI_STR, "FILE", "map the pixels onto these colours instead of choosing: a .pal, or a PNG (its palette, or its colours)", NULL },
    { "scale", 0, CLI_INT, "PERCENT", "resize first, nearest neighbour (50 = half size, 200 = double)", "100" },
    { "align", 0, CLI_INT, "N", "pad the size up to a multiple of N pixels (8 for tiles)", "0" },
    CLI_OPT_OUT,
    CLI_OPT_SAVE,
};

static int quantize_one(cli_ctx *ctx, const char *in)
{
    int rc = cli_load_settings(ctx, in);
    if (rc != CLI_OK) return rc;
    int colors = cli_int(ctx, "colors", 16), percent = cli_int(ctx, "scale", 100), align = cli_int(ctx, "align", 0);
    if (colors < 2 || colors > 256) { cli_error(ctx, in, "--colors %d: 2 to 256", colors); return CLI_REFUSED; }
    if (percent <= 0) { cli_error(ctx, in, "--scale %d: a positive percentage", percent); return CLI_REFUSED; }
    if (align < 0) { cli_error(ctx, in, "--align %d: 0 or a positive size", align); return CLI_REFUSED; }

    unsigned char *raw = NULL;
    unsigned w, h;
    unsigned err = lodepng_decode32_file(&raw, &w, &h, in);
    if (err) { cli_error(ctx, in, "not a PNG this tool reads: %s", lodepng_error_text(err)); return CLI_REFUSED; }
    rgba_t *pixels = (rgba_t *)raw, *scaled = NULL;
    int cw = (int)w, ch = (int)h;
    if (percent != 100 || align > 0) {
        int dw, dh;
        compute_dimensions(cw, ch, percent / 100.0, align, &dw, &dh);
        if (dw != cw || dh != ch) {
            scaled = scale_nearest(pixels, cw, ch, dw, dh);
            if (!scaled) { free(raw); cli_error(ctx, in, "out of memory"); return CLI_IO; }
            pixels = scaled; cw = dw; ch = dh;
        }
    }
    unsigned char *indices = malloc((size_t)cw * ch);
    if (!indices) { free(scaled); free(raw); cli_error(ctx, in, "out of memory"); return CLI_IO; }
    int has_alpha = 0;
    for (int i = 0; i < cw * ch && !has_alpha; i++) has_alpha = pixels[i].a < 128;

    rgb_t palette[256];
    int n;
    char refbuf[1100];
    const char *ref = cli_has(ctx, "palette") ? cli_path(ctx, "palette", refbuf, sizeof refbuf) : NULL;
    if (ref) {
        n = load_reference(ctx, ref, palette, colors);
        if (n < 0) { free(indices); free(scaled); free(raw); return CLI_REFUSED; }
        if (n == 0) { cli_error(ctx, ref, "no colours in the reference"); free(indices); free(scaled); free(raw); return CLI_REFUSED; }
        quantize_map_to_palette(pixels, cw, ch, palette, n, indices);
    } else {
        n = quantize_median_cut(pixels, cw, ch, colors, palette, indices);
    }
    if (cli_has(ctx, "round"))
        for (int i = has_alpha ? 1 : 0; i < n; i++) { palette[i].r = round_snes(palette[i].r); palette[i].g = round_snes(palette[i].g); palette[i].b = round_snes(palette[i].b); }

    char out_path[1100];
    if (cli_output_path(ctx, in, "_indexed.png", out_path, sizeof out_path) != CLI_OK) { free(indices); free(scaled); free(raw); return CLI_IO; }
    rc = save_indexed(ctx, out_path, indices, cw, ch, palette, n, has_alpha);
    free(indices); free(scaled); free(raw);
    if (rc != CLI_OK) return rc;
    if (cli_has(ctx, "save") && (rc = cli_save_settings(ctx, in)) != CLI_OK) return rc;

    if (ctx->json) {
        cli_json_object(ctx, NULL);
        cli_json_str(ctx, "input", in);
        cli_json_int(ctx, "source_width", w); cli_json_int(ctx, "source_height", h);
        cli_json_int(ctx, "width", cw); cli_json_int(ctx, "height", ch);
        cli_json_int(ctx, "colors", n); cli_json_bool(ctx, "transparent", has_alpha);
        if (ref) cli_json_str(ctx, "palette", ref);
        cli_json_array(ctx, "outputs");
        cli_json_object(ctx, NULL); cli_json_str(ctx, "path", out_path); cli_json_str(ctx, "kind", "indexed_png"); cli_json_close(ctx);
        cli_json_close(ctx);
        cli_json_close(ctx);
    } else if (!ctx->quiet) {
        printf("%s: %s -> %s (%dx%d, %d colours%s%s)\n", ctx->tool->name, in, out_path, cw, ch, n,
               has_alpha ? ", index 0 transparent" : "", ref ? ", from the reference" : "");
    }
    return CLI_OK;
}

static int run_quantize(cli_ctx *ctx)
{
    int worst = CLI_OK;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "pictures"); }
    for (int i = 0; i < ctx->nargs; i++) {
        int rc = quantize_one(ctx, ctx->args[i]);
        if (rc != CLI_OK && ctx->json) { cli_json_object(ctx, NULL); cli_json_str(ctx, "input", ctx->args[i]); cli_json_int(ctx, "error", rc); cli_json_close(ctx); }
        if (rc > worst) worst = rc;
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

/* =============================================================== inspect */

static int run_inspect(cli_ctx *ctx)
{
    int worst = CLI_OK;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "palettes"); }
    for (int i = 0; i < ctx->nargs; i++) {
        const char *in = ctx->args[i];
        size_t len = 0;
        unsigned char *buf = cli_read_file(ctx, in, &len);
        if (!buf) { worst = CLI_IO; continue; }
        if (len < 2 || (len & 1)) { cli_error(ctx, in, "%zu bytes — a .pal is a non-empty even number of bytes", len); free(buf); worst = CLI_REFUSED; continue; }
        int n = (int)(len / 2), distinct = 0, black = 0;
        uint16_t seen[256];
        for (int k = 0; k < n && k < 256; k++) {
            uint16_t c = (uint16_t)(buf[k * 2] | (buf[k * 2 + 1] << 8));
            if ((c & 0x7FFF) == 0) black++;
            int dup = 0;
            for (int j = 0; j < distinct && !dup; j++) dup = seen[j] == c;
            if (!dup) seen[distinct++] = c;
        }
        int slots = (n + SLOT_COLORS - 1) / SLOT_COLORS;
        if (ctx->json) {
            cli_json_object(ctx, NULL); cli_json_str(ctx, "input", in);
            cli_json_int(ctx, "colors", n); cli_json_int(ctx, "distinct", distinct); cli_json_int(ctx, "black", black);
            cli_json_int(ctx, "slots", slots); cli_json_int(ctx, "bytes", (long)len);
            cli_json_close(ctx);
        } else {
            printf("%s: %d colours (%d distinct, %d black), %zu bytes — %s\n", in, n, distinct, black, len,
                   n > SLOT_COLORS ? (n == 256 ? "the whole CGRAM (8 bpp)" : "more than one 16-colour slot") : slots == 1 ? "one 16-colour slot" : "");
            if (ctx->verbose) {
                printf("  ");
                for (int k = 0; k < n && k < 256; k++) printf("%s%04X", k ? " " : "", buf[k * 2] | (buf[k * 2 + 1] << 8));
                printf("\n");
            }
        }
        free(buf);
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

static const cli_cmd cmds[] = {
    { "plan", "a project's .pal files -> their CGRAM slots: identical ones shared, a ninth refused, near-duplicates named; <stem>.inc of PAL_<NAME>_CGRAM / _SLOT / _COLORS",
      plan_opts, CLI_N(plan_opts), "plan res/palettes.toml --sprite \"hero.pal npc.pal\" --save", 1, run_plan },
    { "quantize", "RGB or RGBA art -> an indexed PNG of --colors colours (median cut, or a given palette), for opensnes-sprite and opensnes-tileset",
      quantize_opts, CLI_N(quantize_opts), "quantize art/hero.png --colors 16 --round", 1, run_quantize },
    { "inspect", "what a .pal holds: colours, distinct ones, slots", NULL, 0, "inspect res/*.pal", 1, run_inspect },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes-palette", TOOL_VERSION, "a project's palettes: the CGRAM plan, the quantization of RGB art", cmds, CLI_N(cmds),
    };
    return cli_main(&tool, argc, argv);
}
