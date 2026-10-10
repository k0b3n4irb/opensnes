/* checks.c — the three .sym ratchets and the asset inventory (symmap.py's
 * --check-bank0-overflow, --check-ram-budget, --check-data-init and
 * asset_budget.py --oneline, ported 2026-10-05 so a user build needs no
 * interpreter). The numbers and the verdicts are the Python's. */
#include "checks.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* -------------------------------------------------------- bank $00 ROM */

static int starts(const char *s, const char *p) { return strncmp(s, p, strlen(p)) == 0; }

static int is_asset_payload(const char *name)
{
    return starts(name, ".rodata") || starts(name, "asset.") || starts(name, ".asset.");
}

static int cmp_size_desc(const void *a, const void *b)
{
    const sym_section *x = *(const sym_section *const *)a, *y = *(const sym_section *const *)b;
    return x->size < y->size ? 1 : x->size > y->size ? -1 : strcmp(x->name, y->name);
}

static void report_bank0_payload(cli_ctx *ctx, const sym_file *s, const char *rom)
{
    const sym_section *payload[4096];
    int n = 0;
    long total = 0;
    for (size_t i = 0; i < s->nsections && n < 4096; i++)
        if (s->sections[i].bank == 0 && is_asset_payload(s->sections[i].name)) {
            payload[n++] = &s->sections[i];
            total += s->sections[i].size;
        }
    if (total < 1024) return;
    cli_warn(ctx, rom, "%ld bytes of asset data are in bank $00 (the code bank); assets travel as far pointers and can live in any bank (templates/assets.inc)", total);
    qsort(payload, (size_t)n, sizeof payload[0], cmp_size_desc);
    for (int i = 0; i < n && i < 5; i++)
        fprintf(stderr, "  %6u  %s\n", payload[i]->size, payload[i]->name);
    if (n > 5) fprintf(stderr, "  … and %d more\n", n - 5);
}

int check_bank0(cli_ctx *ctx, const sym_file *s, const char *rom, int warn, int fail, check_results *r)
{
    /* The highest bank-$00 ROM label plus its _sizeof_, as symmap does;
     * RAM_USAGE_* markers print with a 00: prefix whatever their bank. */
    const sym_label *highest = NULL;
    for (size_t i = 0; i < s->nlabels; i++) {
        const sym_label *l = &s->labels[i];
        if (l->bank != 0 || l->address < 0x8000 || starts(l->name, "RAM_USAGE_")) continue;
        if (!highest || l->address > highest->address) highest = l;
    }
    long free_bytes = 0x8000;
    if (highest) {
        char sz[512];
        snprintf(sz, sizeof sz, "_sizeof_%s", highest->name);
        const sym_label *d = sym_find_def(s, sz);
        if (!d) d = sym_find(s, sz);
        long size = d ? (long)d->raw : 1;
        free_bytes = 0x10000 - (highest->address + size);
    }
    r->bank0_free = free_bytes;
    /* Code that did not fit bank $00 is in the next banks, where the linker
     * put it: a C function and a library routine are SUPERFREE sections
     * reached by `jsl`, and nothing in them is tied to their bank. */
    long far_code = 0;
    int far_n = 0;
    for (size_t i = 0; i < s->nsections; i++)
        if (s->sections[i].bank > 0 && starts(s->sections[i].name, ".text.")) {
            far_code += s->sections[i].size;
            far_n++;
        }
    if (fail > 0 && free_bytes < fail) {
        cli_error(ctx, rom, "bank $00 ROM: %ld bytes free, under the threshold asked for (%d) — code that does not fit goes to the next banks by itself; this threshold is for a project that wants its code bank watched (.claude/rules/bank0_budget.md)", free_bytes, fail);
        return 1;
    }
    /* Until 2026-10-10 a bank $00 under `warn` bytes free was a warning and,
     * under the build's threshold, a failed link — a ratchet from the time
     * C const data had to live there. Since #127.3 that data is in the asset
     * banks, and a full bank $00 only means the next functions go to bank
     * $01: a game that outgrew 32 KB of code was refused a ROM that worked
     * (issue #168). It is now a line of the report; what cannot move (the
     * sections pinned to bank $00) fails in the linker, loudly. */
    (void)warn;
    if (!ctx->json && !ctx->quiet) {
        if (far_n)
            printf("OK: bank $00 ROM (code): %ld bytes free; %ld bytes of code (%d functions) are in the next banks, where the linker put them\n",
                   free_bytes, far_code, far_n);
        else
            printf("OK: bank $00 ROM (code): %ld bytes free\n", free_bytes);
    }
    report_bank0_payload(ctx, s, rom);
    return 0;
}

/* -------------------------------------------------------------- C RAM */

int check_ram(cli_ctx *ctx, const sym_file *s, const char *rom, int warn, int fail, check_results *r)
{
    const long BAND_END = 0x2000;
    long used_top = 0, total = 0;
    int nband = 0, bad = 0;
    for (size_t i = 0; i < s->nramsections; i++) {
        const sym_section *sec = &s->ramsections[i];
        if (sec->bank != 0) continue;
        if (sec->address < BAND_END) {
            nband++;
            total += sec->size;
            if (sec->address + (long)sec->size > used_top) used_top = sec->address + (long)sec->size;
            if (sec->address + (long)sec->size > BAND_END) {
                if (!bad) cli_error(ctx, rom, "C RAM outside the $0000-$1FFF band: the compiler's RAM addressing is bank-$00-implicit, so RAM past $1FFF is silently wrong-banked or collides with the hardware registers");
                fprintf(stderr, "  $00:%04X+%04X  %s  (crosses $2000)\n", sec->address, sec->size, sec->name);
                bad = 1;
            }
        } else if (sec->address < 0x8000) {
            if (!bad) cli_error(ctx, rom, "C RAM outside the $0000-$1FFF band: the compiler's RAM addressing is bank-$00-implicit, so RAM past $1FFF is silently wrong-banked or collides with the hardware registers");
            fprintf(stderr, "  $00:%04X+%04X  %s  (entirely past $2000)\n", sec->address, sec->size, sec->name);
            bad = 1;
        }
    }
    if (bad) {
        fprintf(stderr, "  shrink RAM usage below the 8 KB band, or declare bulk buffers FAR (docs/tutorials/far_ram.md)\n");
        return 1;
    }
    if (nband == 0) {
        const sym_label *end = sym_find(s, "RAM_USAGE_SLOT_1_BANK_0_END");
        if (!end) {
            cli_warn(ctx, rom, "no [ramsections] block and no RAM_USAGE marker — the C RAM budget is not measurable");
            return 0;
        }
        used_top = end->address + 1;
        total = used_top;
    }
    long free_bytes = BAND_END - used_top;
    r->ram_free = free_bytes; r->ram_top = used_top; r->ram_total = total; r->ram_sections = nband;
    if (fail > 0 && free_bytes < fail) {
        cli_error(ctx, rom, "C RAM band nearly full: %ld bytes free, fail threshold %d — the next RAMSECTION may not fit below $2000, and C reads of anything placed higher are silently wrong (defect B2)", free_bytes, fail);
        return 1;
    }
    if (free_bytes < warn) {
        cli_warn(ctx, rom, "C RAM band nearly full: %ld bytes free below $2000 (threshold %d)", free_bytes, warn);
        /* the three largest sections: the refactor candidates */
        const sym_section *big[3] = { 0, 0, 0 };
        for (size_t i = 0; i < s->nramsections; i++) {
            const sym_section *sec = &s->ramsections[i];
            if (sec->bank != 0 || sec->address >= BAND_END) continue;
            for (int k = 0; k < 3; k++)
                if (!big[k] || sec->size > big[k]->size) {
                    for (int m = 2; m > k; m--) big[m] = big[m - 1];
                    big[k] = sec;
                    break;
                }
        }
        fprintf(stderr, "  largest RAM sections (refactor candidates, FAR is the way above $2000):\n");
        for (int k = 0; k < 3 && big[k]; k++) fprintf(stderr, "    %5u bytes  %s\n", big[k]->size, big[k]->name);
        return 2;
    }
    if (!ctx->json && !ctx->quiet)
        printf("OK: C RAM band $0000-$1FFF: %ld bytes free (top at $%04lX; %ld bytes in %d sections)\n", free_bytes, used_top, total, nband);

    /* the far band $7E:2000-$FFFF: the number is the instrument, no threshold */
    long ceiling = 0x10000, far_top = 0x2000, far_total = 0, far_c = 0;
    int nfar = 0, has_window = 0;
    for (size_t i = 0; i < s->nramsections; i++) {
        const sym_section *sec = &s->ramsections[i];
        if (sec->bank != 0x7E || sec->address < 0x2000 || sec->address >= 0x10000) continue;
        nfar++;
        far_total += sec->size;
        if (starts(sec->name, ".far.")) far_c += sec->size;
        if (strcmp(sec->name, ".ram_code_space") == 0) { has_window = 1; if (sec->address < ceiling) ceiling = sec->address; }
        else if (sec->address + (long)sec->size > far_top) far_top = sec->address + (long)sec->size;
    }
    if (nfar) {
        r->far_free = ceiling - far_top; r->far_top = far_top; r->far_sections = nfar;
        if (!ctx->json && !ctx->quiet) {
            printf("OK: far RAM band $7E:2000-$FFFF: %ld bytes free (top at $%04lX; %ld bytes in %d sections, %ld bytes of C FAR objects",
                   ceiling - far_top, far_top, far_total, nfar, far_c);
            if (has_window) printf("; RAM code window %ld bytes at $%04lX", 0x10000 - ceiling, ceiling);
            printf(")\n");
        }
    }
    return 0;
}

/* ---------------------------------------------------------- data-init */

int check_data_init(cli_ctx *ctx, const sym_file *s, const char *rom)
{
    const sym_section *sec = NULL;
    for (size_t i = 0; i < s->nsections; i++)
        if (strcmp(s->sections[i].name, ".data_init") == 0) { sec = &s->sections[i]; break; }
    const sym_label *end = sym_find(s, "DataInitEnd");
    if (!sec || !end) {
        if (!ctx->json && !ctx->quiet) printf("data-init: no .data_init section or no DataInitEnd label — nothing to check\n");
        return 0;
    }
    long expected = sec->address + (long)sec->size;
    if (end->address != expected) {
        cli_error(ctx, rom, "data-init sentinel is not last: DataInitEnd at $%04X, .data_init ends at $%04lX — %ld byte(s) of init records sit past the terminator and are never copied at boot (an object is linked after data_init_end.o)",
                  end->address, expected, expected - end->address);
        return 1;
    }
    if (!ctx->json && !ctx->quiet)
        printf("OK: data-init sentinel closes .data_init ($%04X+$%X = DataInitEnd $%04X)\n", sec->address, sec->size, end->address);
    return 0;
}

/* ------------------------------------------------------------- assets */

static void scan_assets(const char *dir, check_results *r, int depth)
{
    DIR *d = opendir(dir);
    if (!d || depth > 16) { if (d) closedir(d); return; }
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char path[2048];
        snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) { scan_assets(path, r, depth + 1); continue; }
        if (!S_ISREG(st.st_mode)) continue;
        const char *dot = strrchr(e->d_name, '.');
        if (!dot) continue;
        char ext[8];
        snprintf(ext, sizeof ext, "%s", dot);
        for (char *p = ext; *p; p++) *p = (char)tolower((unsigned char)*p);
        if (!strcmp(ext, ".pic") || !strcmp(ext, ".pc7") || !strcmp(ext, ".map") || !strcmp(ext, ".mp7")) {
            r->asset_vram += (long)st.st_size; r->asset_files++;
        } else if (!strcmp(ext, ".pal")) {
            r->asset_colours += (long)st.st_size / 2; r->asset_files++;
        }
    }
    closedir(d);
}

void report_assets(cli_ctx *ctx, const char *dir, check_results *r)
{
    scan_assets(dir, r, 0);
    if (!r->asset_files || ctx->json || ctx->quiet) return;
    /* an inventory, not a gate: a streaming game legitimately ships more
     * than fits at once (asset_budget.py --oneline) */
    printf("[ASSETS] VRAM %.1fK/64K (%.0f%%) · CGRAM %ld/256 (%.0f%%)\n",
           r->asset_vram / 1024.0, 100.0 * r->asset_vram / 65536.0, r->asset_colours, 100.0 * r->asset_colours / 256.0);
}
