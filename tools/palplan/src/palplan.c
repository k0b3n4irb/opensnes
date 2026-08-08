/*
 * palplan — project shared-palette planner for OpenSNES
 *
 * The SNES holds 256 CGRAM colours: 8 background palettes (indices 0-127)
 * and 8 sprite palettes (indices 128-255), 16 colours each. A real game has
 * far more .pal files than slots, and today the developer hand-picks the
 * `startColor` offset passed to dmaCopyCGram() for every one of them — with
 * nothing to catch a collision or a wasted slot.
 *
 * palplan reads a project manifest of named .pal files (each tagged bg or
 * sprite), and:
 *   - merges byte-identical palettes so they share one slot (always safe),
 *   - assigns every distinct palette a collision-free slot + CGRAM index,
 *   - fails loudly if you need more than 8 bg / 8 sprite slots,
 *   - flags near-duplicates as candidates for a manual shared palette,
 *   - emits a C header of named CGRAM offsets and, optionally, one combined
 *     512-byte CGRAM image you can DMA in a single shot.
 *
 * v1 is an allocator over existing .pal files: it never rewrites tile pixel
 * data, so a merge it performs is always the byte-identical case (zero risk).
 * Tight-packing sub-16-colour (2bpp) palettes and re-quantising to merge
 * non-identical palettes are documented v2 work — see tools/palplan/README.md.
 *
 * .pal format (as written by gfx4snes): raw, headerless, little-endian
 * BGR555, 2 bytes per colour, exactly ncolors*2 bytes.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdarg.h>

#ifndef VERSION
#define VERSION "1.0.0"
#endif

#define MAX_PALETTES 128
#define SLOT_COLORS  16          /* colours per CGRAM palette slot */
#define NUM_SLOTS    8           /* per region (bg / sprite) */
#define BG_BASE      0           /* CGRAM index of bg region */
#define SPR_BASE     128         /* CGRAM index of sprite region */
#define CGRAM_TOTAL  256

typedef enum { PT_BG = 0, PT_SPRITE = 1 } paltype;

typedef struct {
    char     name[64];
    paltype  type;
    char     file[512];          /* resolved path, for diagnostics */
    uint16_t colors[SLOT_COLORS];
    int      ncolors;
    int      slot;               /* 0..7 within its region, -1 if merged */
    int      cgram;              /* assigned CGRAM colour index */
    int      merged_into;        /* index of the owner it shares with, -1 = owner */
} Palette;

static Palette pals[MAX_PALETTES];
static int     npals = 0;

static const char *TYPE_NAME[2] = { "bg", "sprite" };

/*----------------------------------------------------------------------------
 * helpers
 *--------------------------------------------------------------------------*/

static void die(const char *fmt, ...)
{
    va_list ap;
    fputs("palplan: error: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

/* Directory portion of a path (everything up to and including the last '/'),
 * copied into dst. Empty string if the path has no directory component. */
static void dirname_of(const char *path, char *dst, size_t dstsz)
{
    const char *slash = strrchr(path, '/');
    if (!slash) {
        dst[0] = '\0';
        return;
    }
    size_t n = (size_t)(slash - path) + 1;
    if (n >= dstsz)
        n = dstsz - 1;
    memcpy(dst, path, n);
    dst[n] = '\0';
}

/* Uppercase, non-alnum -> '_', for a C macro fragment. */
static void sanitize_macro(const char *in, char *out, size_t outsz)
{
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 1 < outsz; i++) {
        unsigned char c = (unsigned char)in[i];
        out[j++] = (char)(isalnum(c) ? toupper(c) : '_');
    }
    out[j] = '\0';
}

/*----------------------------------------------------------------------------
 * .pal loading — mirrors gfx4snes palette_impose(): raw LE BGR555 words
 *--------------------------------------------------------------------------*/

static void load_pal(Palette *p)
{
    FILE *fp = fopen(p->file, "rb");
    if (!fp)
        die("cannot open palette '%s' (for '%s')", p->file, p->name);

    /* True file size, so the too-many-colours diagnostic reports the real
     * count (a 256-colour .pal must say 256, not a truncated read length). */
    if (fseek(fp, 0, SEEK_END) != 0)
        die("'%s': cannot seek palette file", p->file);
    long fsz = ftell(fp);
    if (fsz < 0)
        die("'%s': cannot size palette file", p->file);
    rewind(fp);

    if (fsz < 2 || (fsz & 1L)) {
        fclose(fp);
        die("'%s': .pal must be a non-empty even number of bytes (got %ld)",
            p->file, fsz);
    }
    if (fsz > (long)SLOT_COLORS * 2) {
        fclose(fp);
        die("'%s': %ld-colour palette — palplan v1 plans 16-colour slots, so "
            "a palette must be <= 16 colours (32 bytes). 256-colour (8bpp) "
            "palettes span the whole CGRAM and are v2 work.",
            p->file, fsz / 2);
    }

    unsigned char raw[SLOT_COLORS * 2];
    size_t got = fread(raw, 1, (size_t)fsz, fp);
    fclose(fp);
    if (got != (size_t)fsz)
        die("'%s': short read (%zu of %ld bytes)", p->file, got, fsz);

    p->ncolors = (int)(fsz / 2);
    for (int i = 0; i < p->ncolors; i++)
        p->colors[i] = (uint16_t)(raw[i * 2] | (raw[i * 2 + 1] << 8));
}

/* Exact byte-for-byte identity — the only merge v1 performs (always safe). */
static int identical(const Palette *a, const Palette *b)
{
    if (a->ncolors != b->ncolors)
        return 0;
    return memcmp(a->colors, b->colors, (size_t)a->ncolors * 2) == 0;
}

/* Count colour indices that differ over the common range, and record the set
 * (used for the "only transparent index 0 differs" sprite hint). Returns the
 * total distance = differing common indices + |ncolors difference|. */
static int distance(const Palette *a, const Palette *b, int *diff_idx, int *ndiff)
{
    int common = a->ncolors < b->ncolors ? a->ncolors : b->ncolors;
    int nd = 0;
    for (int i = 0; i < common; i++) {
        if (a->colors[i] != b->colors[i]) {
            if (nd < SLOT_COLORS)
                diff_idx[nd] = i;
            nd++;
        }
    }
    *ndiff = nd;
    return nd + abs(a->ncolors - b->ncolors);
}

/*----------------------------------------------------------------------------
 * manifest parsing — line-oriented "name  type  file", # comments
 *--------------------------------------------------------------------------*/

static void parse_manifest(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        die("cannot open manifest '%s'", path);

    char mdir[512];
    dirname_of(path, mdir, sizeof(mdir));

    char line[1024];
    int lineno = 0;
    while (fgets(line, sizeof(line), fp)) {
        lineno++;
        char *hash = strchr(line, '#');
        if (hash)
            *hash = '\0';

        char name[64], type[32], file[512];
        int n = sscanf(line, "%63s %31s %511s", name, type, file);
        if (n <= 0)
            continue;              /* blank / comment-only line */
        if (n != 3)
            die("%s:%d: expected 3 columns 'name type file', got %d",
                path, lineno, n);

        if (npals >= MAX_PALETTES)
            die("too many palettes (max %d)", MAX_PALETTES);

        paltype pt;
        if (!strcmp(type, "bg"))
            pt = PT_BG;
        else if (!strcmp(type, "sprite") || !strcmp(type, "obj"))
            pt = PT_SPRITE;
        else
            die("%s:%d: type must be 'bg' or 'sprite' (got '%s')",
                path, lineno, type);

        for (int i = 0; i < npals; i++)
            if (!strcmp(pals[i].name, name))
                die("%s:%d: duplicate palette name '%s'", path, lineno, name);

        Palette *p = &pals[npals++];
        memset(p, 0, sizeof(*p));
        snprintf(p->name, sizeof(p->name), "%s", name);
        p->type = pt;
        p->slot = -1;
        p->merged_into = -1;
        /* resolve file relative to the manifest directory unless absolute */
        if (file[0] == '/')
            snprintf(p->file, sizeof(p->file), "%s", file);
        else
            snprintf(p->file, sizeof(p->file), "%s%s", mdir, file);

        load_pal(p);
    }
    fclose(fp);

    if (npals == 0)
        die("manifest '%s' lists no palettes", path);
}

/*----------------------------------------------------------------------------
 * planning: merge identicals, then allocate slots per region
 *--------------------------------------------------------------------------*/

static int owners_of_type[2];   /* distinct (owner) palette count per region */

static void plan(void)
{
    /* merge byte-identical palettes within the same region */
    for (int i = 0; i < npals; i++) {
        if (pals[i].merged_into != -1)
            continue;
        for (int j = i + 1; j < npals; j++) {
            if (pals[j].merged_into != -1 || pals[j].type != pals[i].type)
                continue;
            if (identical(&pals[i], &pals[j]))
                pals[j].merged_into = i;
        }
    }

    /* allocate slots for owners, in manifest order, per region */
    int next_slot[2] = { 0, 0 };
    int overflow[2]  = { 0, 0 };
    owners_of_type[0] = owners_of_type[1] = 0;

    for (int i = 0; i < npals; i++) {
        if (pals[i].merged_into != -1)
            continue;
        int t = pals[i].type;
        owners_of_type[t]++;
        if (next_slot[t] >= NUM_SLOTS) {
            overflow[t]++;
            pals[i].slot = -1;
            pals[i].cgram = -1;
            continue;
        }
        int slot = next_slot[t]++;
        pals[i].slot = slot;
        pals[i].cgram = (t == PT_BG ? BG_BASE : SPR_BASE) + slot * SLOT_COLORS;
    }

    /* propagate owner assignment to merged palettes */
    for (int i = 0; i < npals; i++) {
        if (pals[i].merged_into != -1) {
            Palette *o = &pals[pals[i].merged_into];
            pals[i].slot = o->slot;
            pals[i].cgram = o->cgram;
        }
    }

    if (overflow[PT_BG] || overflow[PT_SPRITE]) {
        for (int t = 0; t < 2; t++)
            if (overflow[t])
                fprintf(stderr,
                    "palplan: error: %d distinct %s palettes but only %d slots "
                    "(%s region, colours %d-%d).\n",
                    owners_of_type[t], TYPE_NAME[t], NUM_SLOTS,
                    TYPE_NAME[t],
                    t == PT_BG ? 0 : SPR_BASE,
                    t == PT_BG ? 127 : 255);
        fprintf(stderr,
            "palplan: merge near-duplicates (see hints), drop unused "
            "palettes, or move some assets to the other region.\n");
        exit(1);
    }
}

/*----------------------------------------------------------------------------
 * reporting
 *--------------------------------------------------------------------------*/

static void report_region(paltype t)
{
    printf("\n%s palettes (%d / %d slots used):\n",
           t == PT_BG ? "BG" : "Sprite", owners_of_type[t], NUM_SLOTS);
    for (int i = 0; i < npals; i++) {
        if (pals[i].type != t || pals[i].merged_into != -1)
            continue;
        printf("  slot %d  cgram %3d  %-16s (%2d colours)  %s\n",
               pals[i].slot, pals[i].cgram, pals[i].name,
               pals[i].ncolors, pals[i].file);
        /* list palettes sharing this owner's slot */
        for (int j = 0; j < npals; j++)
            if (pals[j].merged_into == i)
                printf("      + shares this slot: %s (identical palette)\n",
                       pals[j].name);
    }
}

static void report_hints(int threshold)
{
    if (threshold <= 0)
        return;

    int header_done = 0;
    for (int i = 0; i < npals; i++) {
        if (pals[i].merged_into != -1)
            continue;
        for (int j = i + 1; j < npals; j++) {
            if (pals[j].merged_into != -1 || pals[j].type != pals[i].type)
                continue;
            int diff_idx[SLOT_COLORS], ndiff;
            int d = distance(&pals[i], &pals[j], diff_idx, &ndiff);
            if (d == 0 || d > threshold)
                continue;

            if (!header_done) {
                printf("\nMerge hints (near-duplicate palettes — a shared "
                       "palette would free a slot):\n");
                header_done = 1;
            }

            /* strong case: sprite palettes that differ only at the
             * transparent colour 0, which is never displayed */
            if (pals[i].type == PT_SPRITE && ndiff == 1 && diff_idx[0] == 0 &&
                pals[i].ncolors == pals[j].ncolors) {
                printf("  %s ~ %s differ ONLY at colour 0 (transparent for "
                       "sprites) — safe to merge into one palette.\n",
                       pals[i].name, pals[j].name);
                continue;
            }

            printf("  %s ~ %s differ in %d colour%s", pals[i].name,
                   pals[j].name, d, d == 1 ? "" : "s");
            if (ndiff > 0) {
                printf(" (index%s ", ndiff == 1 ? "" : "es");
                for (int k = 0; k < ndiff; k++)
                    printf("%s%d", k ? "," : "", diff_idx[k]);
                printf(")");
            }
            printf(" — consider a shared palette.\n");
        }
    }
}

static void report(int quiet, int threshold)
{
    int distinct = owners_of_type[0] + owners_of_type[1];
    if (!quiet) {
        printf("palplan — %d palette%s, %d distinct\n",
               npals, npals == 1 ? "" : "s", distinct);
        report_region(PT_BG);
        report_region(PT_SPRITE);
    }
    report_hints(threshold);
    if (!quiet)
        printf("\nOK: fits in %d BG + %d sprite slots.\n", NUM_SLOTS, NUM_SLOTS);
}

/*----------------------------------------------------------------------------
 * outputs: C header + combined CGRAM image
 *--------------------------------------------------------------------------*/

static void write_header(const char *path, const char *manifest)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
        die("cannot write header '%s'", path);

    fprintf(fp,
        "/* Generated by palplan v%s — do not edit by hand.\n"
        " * Source manifest: %s\n"
        " *\n"
        " * Each palette gets its CGRAM colour index (pass as `startColor` to\n"
        " * dmaCopyCGram) and its slot number (the palette bank for gfx4snes -e\n"
        " * or OBJ_CGRAM_PAL()). Palettes with identical colours share a slot.\n"
        " */\n"
        "#ifndef PALPLAN_H\n"
        "#define PALPLAN_H\n\n",
        VERSION, manifest);

    for (int i = 0; i < npals; i++) {
        char mac[80];
        sanitize_macro(pals[i].name, mac, sizeof(mac));
        const char *shared = pals[i].merged_into != -1 ? " (shared)" : "";
        fprintf(fp, "/* %s (%s, %d colours) \xe2\x80\x94 %s slot %d%s */\n",
                pals[i].name, TYPE_NAME[pals[i].type], pals[i].ncolors,
                TYPE_NAME[pals[i].type], pals[i].slot, shared);
        fprintf(fp, "#define PAL_%s_CGRAM  %d\n", mac, pals[i].cgram);
        fprintf(fp, "#define PAL_%s_SLOT   %d\n", mac, pals[i].slot);
        fprintf(fp, "#define PAL_%s_COLORS %d\n\n", mac, pals[i].ncolors);
    }

    fprintf(fp, "#endif /* PALPLAN_H */\n");
    fclose(fp);
}

/* Full 512-byte CGRAM image: every owner's colours at its assigned index,
 * unused entries left black. Load with dmaCopyCGram(img, 0, 512), or a
 * sub-range with dmaCopyCGram(img + start*2, start, count*2). */
static void write_combined(const char *path)
{
    unsigned char img[CGRAM_TOTAL * 2];
    memset(img, 0, sizeof(img));

    for (int i = 0; i < npals; i++) {
        if (pals[i].merged_into != -1)
            continue;           /* owner writes; merged ones alias it */
        int base = pals[i].cgram * 2;
        for (int c = 0; c < pals[i].ncolors; c++) {
            img[base + c * 2]     = (unsigned char)(pals[i].colors[c] & 0xff);
            img[base + c * 2 + 1] = (unsigned char)(pals[i].colors[c] >> 8);
        }
    }

    FILE *fp = fopen(path, "wb");
    if (!fp)
        die("cannot write combined palette '%s'", path);
    fwrite(img, 1, sizeof(img), fp);
    fclose(fp);
}

/*----------------------------------------------------------------------------
 * CLI
 *--------------------------------------------------------------------------*/

static void usage(FILE *fp)
{
    fprintf(fp,
        "palplan v%s — project shared-palette planner for OpenSNES\n\n"
        "Usage: palplan [options] <manifest>\n\n"
        "Plans how a project's .pal files pack into the SNES's 8 BG + 8 sprite\n"
        "palette slots (CGRAM 0-127 / 128-255), merging identical palettes and\n"
        "reporting collisions and near-duplicates.\n\n"
        "Manifest: one palette per line, '# ...' comments, 3 whitespace columns:\n"
        "    name   type   file.pal        (type = bg | sprite)\n"
        "  paths are resolved relative to the manifest's directory.\n\n"
        "Options:\n"
        "  -o FILE   write a C header of named CGRAM offsets (PAL_<NAME>_CGRAM)\n"
        "  -b FILE   write a combined 512-byte CGRAM image (one-shot DMA)\n"
        "  -t N      near-duplicate hint threshold in colours (default 2, 0=off)\n"
        "  -q        quiet: suppress the allocation table (hints/errors stay)\n"
        "  -h        this help\n"
        "  -v        version\n",
        VERSION);
}

int main(int argc, char **argv)
{
    const char *manifest = NULL;
    const char *header_out = NULL;
    const char *combined_out = NULL;
    int threshold = 2;
    int quiet = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else if (!strcmp(a, "-v") || !strcmp(a, "--version")) {
            printf("palplan v%s\n", VERSION);
            return 0;
        } else if (!strcmp(a, "-q")) {
            quiet = 1;
        } else if (!strcmp(a, "-o")) {
            if (++i >= argc) die("-o needs a file argument");
            header_out = argv[i];
        } else if (!strcmp(a, "-b")) {
            if (++i >= argc) die("-b needs a file argument");
            combined_out = argv[i];
        } else if (!strcmp(a, "-t")) {
            if (++i >= argc) die("-t needs a number argument");
            threshold = atoi(argv[i]);
        } else if (a[0] == '-' && a[1] != '\0') {
            die("unknown option '%s' (try -h)", a);
        } else {
            if (manifest)
                die("multiple manifests given ('%s' and '%s')", manifest, a);
            manifest = a;
        }
    }

    if (!manifest) {
        usage(stderr);
        return 2;
    }

    parse_manifest(manifest);
    plan();
    report(quiet, threshold);

    if (header_out)
        write_header(header_out, manifest);
    if (combined_out)
        write_combined(combined_out);

    return 0;
}
