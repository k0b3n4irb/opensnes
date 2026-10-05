/* asm.c — the two checks that read the build's .c.asm intermediates:
 * bank-blind reads of bank $01+ data (check_bank_reads.py, issue #104) and
 * the WRAM-port write reachable from an NMI callback (check_nmi_wram_race.py,
 * KNOWN_LIMITATIONS red). Ported 2026-10-05; same heuristics, same verdicts. */
#include "checks.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strncasecmp: POSIX, not <string.h> on macOS */

/* ---------------------------------------------------------- string sets */

typedef struct { char **v; size_t n, cap; } str_set;

static int set_has(const str_set *s, const char *x)
{
    for (size_t i = 0; i < s->n; i++) if (strcmp(s->v[i], x) == 0) return 1;
    return 0;
}

static void set_add(str_set *s, const char *x)
{
    if (set_has(s, x)) return;
    if (s->n == s->cap) {
        size_t nc = s->cap ? s->cap * 2 : 64;
        char **nv = realloc(s->v, nc * sizeof *nv);
        if (!nv) return;
        s->v = nv; s->cap = nc;
    }
    s->v[s->n] = malloc(strlen(x) + 1);
    if (!s->v[s->n]) return;
    strcpy(s->v[s->n], x);
    s->n++;
}

static void set_free(str_set *s)
{
    for (size_t i = 0; i < s->n; i++) free(s->v[i]);
    free(s->v);
    memset(s, 0, sizeof *s);
}

static int cmp_str(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

/* The .c.asm files of a directory, sorted, plus combined.asm first when it exists. */
static str_set asm_files(const char *dir, int with_combined)
{
    str_set out = { 0, 0, 0 };
    char path[2048];
    if (with_combined) {
        snprintf(path, sizeof path, "%s/combined.asm", dir);
        FILE *f = fopen(path, "r");
        if (f) { fclose(f); set_add(&out, path); }
    }
    str_set casm = { 0, 0, 0 };
    DIR *d = opendir(dir);
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            size_t l = strlen(e->d_name);
            if (l > 6 && strcmp(e->d_name + l - 6, ".c.asm") == 0) {
                snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
                set_add(&casm, path);
            }
        }
        closedir(d);
    }
    if (casm.n) qsort(casm.v, casm.n, sizeof *casm.v, cmp_str);
    for (size_t i = 0; i < casm.n; i++) set_add(&out, casm.v[i]);
    set_free(&casm);
    return out;
}

static int ident_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int ident_char(char c) { return isalnum((unsigned char)c) || c == '_'; }

/* Copies the identifier at p into out (returns its length, 0 if none). */
static size_t take_ident(const char *p, char *out, size_t n)
{
    if (!ident_start(*p)) return 0;
    size_t l = 0;
    while (ident_char(p[l])) l++;
    if (l >= n) l = n - 1;
    memcpy(out, p, l);
    out[l] = '\0';
    return l;
}

/* ---------------------------------------------------------- bank reads */

static const char *const MEM_MNEMONICS[] = {
    "lda", "sta", "ldx", "stx", "ldy", "sty", "adc", "sbc", "cmp", "cpx", "cpy", "and", "ora", "eor", NULL
};

/* Classify one line: ".w sym" memory read (never exempt), "#sym" address
 * (exempt when the TU also takes "#:sym"), "pea.w sym" / "pea.w :sym". */
static void scan_bank_line(const char *line, str_set *mem, str_set *imm, str_set *banked)
{
    const char *p = line;
    while (isspace((unsigned char)*p)) p++;
    char mn[8];
    size_t ml = take_ident(p, mn, sizeof mn);
    if (!ml) return;
    for (char *q = mn; *q; q++) *q = (char)tolower((unsigned char)*q);
    p += ml;
    char name[256];
    if (strcmp(mn, "pea") == 0) {
        if (p[0] == '.' && (p[1] == 'w' || p[1] == 'W')) p += 2;
        if (!isspace((unsigned char)*p)) return;
        while (isspace((unsigned char)*p)) p++;
        if (*p == ':') { if (take_ident(p + 1, name, sizeof name)) set_add(banked, name); }
        else if (take_ident(p, name, sizeof name)) set_add(imm, name);
        return;
    }
    int is_mem = 0;
    for (int i = 0; MEM_MNEMONICS[i]; i++) if (strcmp(mn, MEM_MNEMONICS[i]) == 0) is_mem = 1;
    if (!is_mem) return;
    if (!(p[0] == '.' && (p[1] == 'w' || p[1] == 'W'))) return;   /* only the 16-bit absolute forms */
    p += 2;
    if (!isspace((unsigned char)*p)) return;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '#') {
        if (p[1] == ':') { if (take_ident(p + 2, name, sizeof name)) set_add(banked, name); }
        else if (take_ident(p + 1, name, sizeof name)) set_add(imm, name);
    } else if (*p != '$' && *p != ':') {
        if (take_ident(p, name, sizeof name)) set_add(mem, name);
    }
}

int check_bank_reads(cli_ctx *ctx, const sym_file *s, const char *dir)
{
    str_set files = asm_files(dir, 0);
    int bad = 0;
    for (size_t fi = 0; fi < files.n; fi++) {
        FILE *f = fopen(files.v[fi], "r");
        if (!f) continue;
        str_set mem = { 0, 0, 0 }, imm = { 0, 0, 0 }, banked = { 0, 0, 0 };
        char line[2048];
        while (fgets(line, sizeof line, f)) scan_bank_line(line, &mem, &imm, &banked);
        fclose(f);
        /* blind = (imm - banked) ∪ mem, sorted for a stable report */
        str_set blind = { 0, 0, 0 };
        for (size_t i = 0; i < imm.n; i++) if (!set_has(&banked, imm.v[i])) set_add(&blind, imm.v[i]);
        for (size_t i = 0; i < mem.n; i++) set_add(&blind, mem.v[i]);
        if (blind.n) qsort(blind.v, blind.n, sizeof *blind.v, cmp_str);
        const char *base = strrchr(files.v[fi], '/') ? strrchr(files.v[fi], '/') + 1 : files.v[fi];
        for (size_t i = 0; i < blind.n; i++) {
            const sym_label *l = sym_find(s, blind.v[i]);
            if (!l) continue;                       /* registers, defines: not linked symbols */
            if (l->bank >= 0x01 && l->address >= 0x8000) {
                if (!bad) cli_error(ctx, base, "C code reads bank $01+ data with bank-$00 addressing (the read returns garbage at runtime: cc65816 near deref). Keep C-read data in bank $00 or RAM, or pass it to a lib function as a far pointer:");
                fprintf(stderr, "  $%02X:%04X  %s  (read bank-blind in %s)\n", l->bank, l->address, blind.v[i], base);
                bad = 1;
            }
        }
        set_free(&mem); set_free(&imm); set_free(&banked); set_free(&blind);
    }
    set_free(&files);
    if (!bad && !ctx->json && !ctx->quiet) printf("OK: no bank-blind C reads of bank $01+ data\n");
    return bad;
}

/* ------------------------------------------------------------ NMI race */

typedef struct {
    char   name[128];
    char   file[512];
    int    start_line;
    str_set callees;
    str_set writes;          /* "line N: text" */
    int    seen;
} func_t;

typedef struct { func_t *v; size_t n, cap; } funcs_t;

static func_t *func_find(funcs_t *fs, const char *name)
{
    for (size_t i = 0; i < fs->n; i++) if (strcmp(fs->v[i].name, name) == 0) return &fs->v[i];
    return NULL;
}

static func_t *func_add(funcs_t *fs, const char *name, const char *file, int line)
{
    if (fs->n == fs->cap) {
        size_t nc = fs->cap ? fs->cap * 2 : 128;
        func_t *nv = realloc(fs->v, nc * sizeof *nv);
        if (!nv) return NULL;
        fs->v = nv; fs->cap = nc;
    }
    func_t *f = &fs->v[fs->n++];
    memset(f, 0, sizeof *f);
    snprintf(f->name, sizeof f->name, "%s", name);
    snprintf(f->file, sizeof f->file, "%s", file);
    f->start_line = line;
    return f;
}

/* A WRAM-port access: sta/stz [.w|.l|.b] $2180-$2183 (with or without a 00
 * prefix), ldx #$218x, sta.l/stz.l $00218x — the Python's five patterns. */
static int is_port_write(const char *line)
{
    const char *p = line;
    while (isspace((unsigned char)*p)) p++;
    char mn[8];
    size_t ml = take_ident(p, mn, sizeof mn);
    if (!ml) return 0;
    for (char *q = mn; *q; q++) *q = (char)tolower((unsigned char)*q);
    int store = !strcmp(mn, "sta") || !strcmp(mn, "stz"), ldx = !strcmp(mn, "ldx");
    if (!store && !ldx) return 0;
    p += ml;
    if (*p == '.' && strchr("wlbWLB", p[1]) && p[1]) p += 2;
    if (!isspace((unsigned char)*p)) return 0;
    while (isspace((unsigned char)*p)) p++;
    if (ldx) { if (*p != '#') return 0; p++; }
    if (*p != '$') return 0;
    p++;
    if (p[0] == '0' && p[1] == '0') p += 2;
    if (!(p[0] == '2' && p[1] == '1' && p[2] == '8' && p[3] >= '0' && p[3] <= '3')) return 0;
    return !ident_char(p[4]);
}

/* "jsl|jsr|jml|jmp [.w|.l|.b] name" → name (a label, not a local @label) */
static int call_target(const char *line, char *out, size_t n)
{
    const char *p = line;
    while (isspace((unsigned char)*p)) p++;
    char mn[8];
    size_t ml = take_ident(p, mn, sizeof mn);
    if (!ml) return 0;
    for (char *q = mn; *q; q++) *q = (char)tolower((unsigned char)*q);
    if (strcmp(mn, "jsl") && strcmp(mn, "jsr") && strcmp(mn, "jml") && strcmp(mn, "jmp")) return 0;
    p += ml;
    if (*p == '.' && p[1] && strchr("wlbWLB", p[1])) p += 2;
    if (!isspace((unsigned char)*p)) return 0;
    while (isspace((unsigned char)*p)) p++;
    size_t l = take_ident(p, out, n);
    if (!l) return 0;
    p += l;
    while (isspace((unsigned char)*p)) p++;
    return *p == '\0' || *p == ';';
}

/* "pea[.w] label" → label */
static int pea_label(const char *line, char *out, size_t n)
{
    const char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (strncasecmp(p, "pea", 3) != 0) return 0;
    p += 3;
    if (p[0] == '.' && (p[1] == 'w' || p[1] == 'W')) p += 2;
    if (!isspace((unsigned char)*p)) return 0;
    while (isspace((unsigned char)*p)) p++;
    size_t l = take_ident(p, out, n);
    if (!l) return 0;
    p += l;
    while (isspace((unsigned char)*p)) p++;
    return *p == '\0' || *p == ';';
}

static int is_jsl_nmiset(const char *line)
{
    char t[128];
    if (!call_target(line, t, sizeof t)) return 0;
    const char *p = line;
    while (isspace((unsigned char)*p)) p++;
    return strncasecmp(p, "jsl", 3) == 0 && strcmp(t, "nmiSet") == 0;
}

static void chomp(char *s) { size_t l = strlen(s); while (l && (s[l - 1] == '\n' || s[l - 1] == '\r')) s[--l] = '\0'; }

int check_nmi_race(cli_ctx *ctx, const char *dir)
{
    str_set files = asm_files(dir, 1);
    if (files.n == 0) {
        cli_error(ctx, dir, "no .c.asm intermediates — build the project first");
        set_free(&files);
        return 1;
    }
    funcs_t fs = { 0, 0, 0 };
    str_set callbacks = { 0, 0, 0 };
    char recent[12][128];      /* the last symbolic pea labels, for nmiSet's first argument */
    for (size_t fi = 0; fi < files.n; fi++) {
        FILE *f = fopen(files.v[fi], "r");
        if (!f) continue;
        const char *base = strrchr(files.v[fi], '/') ? strrchr(files.v[fi], '/') + 1 : files.v[fi];
        func_t *cur = NULL;
        int nrecent = 0, lineno = 0;
        char line[2048], name[128];
        while (fgets(line, sizeof line, f)) {
            lineno++;
            chomp(line);
            const char *p = line;
            while (isspace((unsigned char)*p)) p++;
            if (!*p || *p == ';') continue;
            /* a function: "name:" at column 0 and nothing else */
            if (ident_start(line[0])) {
                size_t l = take_ident(line, name, sizeof name);
                const char *q = line + l;
                if (*q == ':') {
                    q++;
                    while (isspace((unsigned char)*q)) q++;
                    if (!*q) { cur = func_find(&fs, name); if (!cur) cur = func_add(&fs, name, base, lineno); nrecent = 0; continue; }
                }
            }
            if (!cur) continue;
            if (call_target(line, name, sizeof name)) set_add(&cur->callees, name);
            if (pea_label(line, name, sizeof name)) {
                if (nrecent == 12) { memmove(recent[0], recent[1], sizeof recent[0] * 11); nrecent = 11; }
                snprintf(recent[nrecent++], sizeof recent[0], "%s", name);
            }
            if (is_jsl_nmiset(line) && nrecent) set_add(&callbacks, recent[nrecent - 1]);
            if (is_port_write(line)) {
                char w[2200];
                snprintf(w, sizeof w, "line %d: %s", lineno, p);
                set_add(&cur->writes, w);
            }
        }
        fclose(f);
    }
    /* the closure from the roots; functions not defined here (the lib) are not followed */
    str_set stack = { 0, 0, 0 };
    set_add(&stack, "NmiHandler"); set_add(&stack, "DefaultNmiCallback");
    for (size_t i = 0; i < callbacks.n; i++) set_add(&stack, callbacks.v[i]);
    for (size_t i = 0; i < stack.n; i++) {
        func_t *f = func_find(&fs, stack.v[i]);
        if (!f || f->seen) continue;
        f->seen = 1;
        for (size_t k = 0; k < f->callees.n; k++) set_add(&stack, f->callees.v[k]);
    }
    int bad = 0;
    for (size_t i = 0; i < fs.n; i++) {
        func_t *f = &fs.v[i];
        if (!f->seen || !f->writes.n) continue;
        if (!bad) cli_error(ctx, dir, "NMI / WRAM port race: a function reachable from an NMI callback writes $2180-$2183. The port shares state with the main thread; a mid-sequence interrupt corrupts the address pointer and the main thread silently writes to the wrong place (KNOWN_LIMITATIONS.md). Route the data through DMA, or move the work out of the callback:");
        fprintf(stderr, "  %s  (%s:%d)\n", f->name, f->file, f->start_line);
        for (size_t k = 0; k < f->writes.n; k++) fprintf(stderr, "    %s\n", f->writes.v[k]);
        bad = 1;
    }
    if (bad && callbacks.n) {
        fprintf(stderr, "  NMI callbacks found:");
        for (size_t i = 0; i < callbacks.n; i++) fprintf(stderr, " %s%s", callbacks.v[i], func_find(&fs, callbacks.v[i]) ? "" : " [external, not analysed]");
        fprintf(stderr, "\n");
    }
    for (size_t i = 0; i < fs.n; i++) { set_free(&fs.v[i].callees); set_free(&fs.v[i].writes); }
    free(fs.v);
    set_free(&stack); set_free(&callbacks); set_free(&files);
    if (!bad && !ctx->json && !ctx->quiet) printf("OK: no WRAM-port write reachable from an NMI callback\n");
    return bad;
}
