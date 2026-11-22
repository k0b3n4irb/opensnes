/* sframe.c — static frames at link time (L4, step S1: dormant).
 *
 * A function with a static frame keeps its temps (later its parameters) at a
 * fixed place of the direct page, `sf.<fn>.f`, instead of on the stack. Two
 * functions may share bytes only when they can never be active together, and
 * a framed function must never be active twice. Neither can be decided by the
 * compiler, which sees one file: it is decided here, over the whole program,
 * from the objects the link takes (the linkfile `make` writes, never a folder).
 *
 * Every object has a text beside it: X.c.wrap.asm / X.wrap.asm (project),
 * X.c.asm (library C), X-asm.text.asm (library assembly), crt0.text.asm. The
 * compiler writes a side table into every C text (comment lines: `; sframe fn`,
 * `; sframe call`, `; sframe addr`, `; sframe table 1`; compiler/qbe/w65816/
 * emit.c, sframe_dump), frames on or off.
 *
 *   place   builds the call graph from the C TABLES and from the assembly's
 *           text, refuses a re-entry through a framed function and a window too
 *           small, places each frame just past its deepest framed ancestor
 *           (longest path), and writes sframe.asm. On a refusal it writes
 *           nothing and removes an old sframe.asm, so the link fails on an
 *           undefined `sf.*` label whatever the Makefile does with the exit code.
 *   check   builds the same graph from the INSTRUCTION LINES of the C texts
 *           (jsl/jsr/jml/jmp, `jml` tail calls included, indirect calls, the
 *           addresses taken), compares it with the tables (a call the table
 *           omits is a refusal: placer and check do not share their source),
 *           reads each frame's extent from the `sf.<fn>.f+N` operands, reads the
 *           placement back from the .sym, and refuses: a table that differs from
 *           its text, an `sf.*` label of the .sym that is not a table's frame
 *           (or the reverse), a frame used past its declared size, a re-entry
 *           (a cycle through a framed function), two frames that overlap while
 *           one can be active under the other, a frame past $FF or outside its
 *           window or on another direct-page block, a frame an NMI root reaches
 *           past the NMI page.
 *
 * Assembly is read routine by routine (a global label to the next; falling into
 * the next label is an edge; macros of the same text expanded): a routine is
 * closed when its jsl/jml/jsr/jmp reach closed routines, with no indirect jump
 * (but through a table of its own `@` labels) and no pea/per not pulled back
 * before an rtl/rts (a computed return). An open routine reaches UNKNOWN; so
 * does a call to a symbol no linked text defines, an object without a text, and
 * a C text without a table. UNKNOWN reaches every exported C function, every
 * global assembly routine and every address-taken function. The routines whose
 * computed jump was read by hand are listed below (REVIEWED) with the reason and
 * a fingerprint of their text: a routine that changed is open again until it is
 * read again. The runtime helpers the backend calls are named (HELPERS): the
 * only calls a C text may make that its table does not list.
 *
 * S1 is dormant: the compiler claims `sf=0` for every function, so `place`
 * writes an empty sframe.asm and the check has no frame to place. Faults for
 * the tests: OPENSNES_SFRAME_FAULT=overlap (place puts every frame at offset 0)
 * and =reentry (place writes despite a re-entry), so that the check's own
 * refusals can be provoked from a real placement. */
#include "sframe.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* ------------------------------------------------------------- basics */

static void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q) { fprintf(stderr, "opensnes-rom: out of memory\n"); exit(CLI_IO); }
    return q;
}

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = xrealloc(NULL, n);
    memcpy(d, s, n);
    return d;
}

typedef struct { char **v; int n, cap; } svec;

static int sv_has(const svec *s, const char *x)
{
    for (int i = 0; i < s->n; i++) if (strcmp(s->v[i], x) == 0) return 1;
    return 0;
}

static void sv_add(svec *s, const char *x)
{
    if (sv_has(s, x)) return;
    if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 8; s->v = xrealloc(s->v, (size_t)s->cap * sizeof *s->v); }
    s->v[s->n++] = xstrdup(x);
}

/* appends, duplicates kept (a macro body) */
static void sv_push(svec *s, const char *x)
{
    if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 8; s->v = xrealloc(s->v, (size_t)s->cap * sizeof *s->v); }
    s->v[s->n++] = xstrdup(x);
}

/* strcasestr is a GNU extension */
static const char *ci_find(const char *hay, const char *needle)
{
    size_t n = strlen(needle);
    for (; *hay; hay++) if (!strncasecmp(hay, needle, n)) return hay;
    return NULL;
}

static void sv_free(svec *s)
{
    for (int i = 0; i < s->n; i++) free(s->v[i]);
    free(s->v);
    memset(s, 0, sizeof *s);
}

typedef struct { int *v; int n, cap; } ivec;

static void iv_add(ivec *s, int x)
{
    if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 8; s->v = xrealloc(s->v, (size_t)s->cap * sizeof *s->v); }
    s->v[s->n++] = x;
}

static int ident_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int ident_char(char c) { return isalnum((unsigned char)c) || c == '_'; }

/* A symbol: an identifier that may hold dots (`foo.main`, cc65816's name of a
 * static of main.c; `sf.foo.main.f`, a frame label). */
static size_t take_sym(const char *p, char *out, size_t n)
{
    if (!ident_start(*p)) return 0;
    size_t l = 0;
    while (ident_char(p[l]) || (p[l] == '.' && ident_char(p[l + 1]))) l++;
    size_t c = l < n ? l : n - 1;
    memcpy(out, p, c);
    out[c] = '\0';
    return l;
}

static unsigned long long fnv(unsigned long long h, const char *s)
{
    for (; *s; s++) { h ^= (unsigned char)*s; h *= 1099511628211ULL; }
    return h;
}

/* ------------------------------------------------------------ the lists */

/* The runtime helpers compiler/qbe/w65816/emit.c calls by name (grep `jsl __`
 * and `jsl tcc_` there): a C text calls them without a table line. Any other
 * call of the text that the table does not list fails the check. */
static const char *const HELPERS[] = {
    "__mul16", "__div16", "__mod16", "__sdiv16", "__smod16", "__mul32",
    "tcc_mul32", "tcc_udivmod32", "tcc_sdivmod32", NULL
};

static int is_helper(const char *n)
{
    for (int i = 0; HELPERS[i]; i++) if (strcmp(HELPERS[i], n) == 0) return 1;
    return 0;
}

/* Routines of the library whose computed jump was read by hand (L4 S0,
 * 2026-11-13): the jump goes into the routine's own code only. Keyed by the
 * object (a project's own memcpy is not trusted), checked by a fingerprint of
 * the routine's instruction lines (OPENSNES_SFRAME_FINGERPRINTS=1 opensnes-rom
 * place prints them; comments, directives and labels do not count): when the
 * text changes the routine is open again until it is re-read and its
 * fingerprint written here. memcpy and memmove were read again on 2026-11-14
 * (S1); the others are unchanged since S0's reading (git log). */
typedef struct { const char *name, *object, *why; unsigned long long fp; } reviewed_t;
static const reviewed_t REVIEWED[] = {
    { "memcpy", "string-asm", "builds MVN + RTL on the stack and returns into it; `pea.w @back-1` brings it back", 0x5c061acccec0231fULL },
    { "memmove", "string-asm", "the same stub with MVP for the backward case, `jml memcpy` otherwise", 0xb280781025a956a6ULL },
    { "lzssDecodeRam", "lzss-asm", "`jmp (tcc__r3h)` where tcc__r3h is one of #@cpslow / #@cp7e / #@cp7f, its own labels", 0x67dcf928fa6b2266ULL },
    { "sprite_oamset-asm:_ohr_big", "sprite_oamset-asm", "`pea _ohr_lo1-1` + `adc #_ohr_run-1` / rts, into its own store run", 0x287f0e152b166ecaULL },
    { "sprite_oamset-asm:_ohr_lo1", "sprite_oamset-asm", "`pea _ohr_lo2-1` + `adc #_ohr_run-1` / rts, into its own store run", 0x8a76cd0c79e96413ULL },
    { NULL, NULL, NULL, 0 }
};

/* Macros of templates/assets.inc that open a section: not code */
static const char *const SECTION_MACROS[] = { "ASSET_SECTION", "RAM_CODE_SECTION", "GSU_SECTION", NULL };

static const char *const MNEMONICS[] = {
    "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "bra", "brk", "brl", "bvc", "bvs",
    "clc", "cld", "cli", "clv", "cmp", "cop", "cpx", "cpy", "dea", "dec", "dex", "dey", "eor", "ina", "inc",
    "inx", "iny", "jml", "jmp", "jsl", "jsr", "lda", "ldx", "ldy", "lsr", "mvn", "mvp", "nop", "ora", "pea",
    "pei", "per", "pha", "phb", "phd", "phk", "php", "phx", "phy", "pla", "plb", "pld", "plp", "plx", "ply",
    "rep", "rol", "ror", "rti", "rtl", "rts", "sbc", "sec", "sed", "sei", "sep", "sta", "stp", "stx", "sty",
    "stz", "swa", "tad", "tas", "tax", "tay", "tcd", "tcs", "tda", "tdc", "trb", "tsa", "tsb", "tsc", "tsx",
    "txa", "txs", "txy", "tya", "tyx", "wai", "wdm", "xba", "xce", NULL
};

static int in_list(const char *const *l, const char *x, int nocase)
{
    for (int i = 0; l[i]; i++) if ((nocase ? strcasecmp(l[i], x) : strcmp(l[i], x)) == 0) return 1;
    return 0;
}

/* ------------------------------------------------------------ the model */

enum { K_C = 1, K_ASM, K_ALIAS };

typedef struct {
    char  *name;
    int    kind, text, line;
    /* table (C) */
    int    tab, exp_tab, addr_tab, cand, stack, params, sf;
    svec   tcalls;
    ivec   targb;
    int    tind;
    /* text */
    svec   xcalls;
    int    xind, open, defined;
    char   why[320];
    int    reviewed;            /* 1 trusted, -1 fingerprint changed */
    unsigned long long fp;
    int    xext;                /* extent of sf.<this>.f in the texts */
    int    lastflow;            /* asm: last instruction ended the flow */
    int    hasdata;             /* asm: data directives after the label */
} sfn;

typedef struct {
    char *obj, *path, stem[256];
    int   is_c, has_table, lib;
} stext;

typedef struct {
    sfn   *f; int nf, capf;
    int   *hash; int hcap;
    stext *t; int nt;
    svec   addr_tab;                       /* `; sframe addr` names */
    svec   refs_c;                         /* names a C text's code or data refers to (not as a call target) */
    svec   refs_asm;                       /* the same in assembly */
    svec  *refs_c_text;                    /* per text, for the comparison */
    svec  *addr_tab_text;
    svec   nmi_lit;                        /* literal nmiSet / nmiSetLag callbacks */
    svec   nmi_why;                        /* why the NMI roots fall back to every exported and address-taken function */
    svec   asm_sf;                         /* assembly lines naming a static frame */
    svec   no_b;                           /* instruction lines naming sf.* without .b */
    svec   problems;                       /* reading problems: missing text, unknown macro */
    int    unk;                            /* the UNKNOWN node */
} sprog;

static unsigned hstr(const char *s) { return (unsigned)fnv(1469598103934665603ULL, s); }

static int find_fn(const sprog *p, const char *name)
{
    if (!p->hcap) return -1;
    unsigned h = hstr(name) & (unsigned)(p->hcap - 1);
    while (p->hash[h] >= 0) {
        if (strcmp(p->f[p->hash[h]].name, name) == 0) return p->hash[h];
        h = (h + 1) & (unsigned)(p->hcap - 1);
    }
    return -1;
}

static void rehash(sprog *p)
{
    int nc = p->hcap ? p->hcap * 2 : 1024;
    free(p->hash);
    p->hash = xrealloc(NULL, (size_t)nc * sizeof *p->hash);
    for (int i = 0; i < nc; i++) p->hash[i] = -1;
    p->hcap = nc;
    for (int i = 0; i < p->nf; i++) {
        unsigned h = hstr(p->f[i].name) & (unsigned)(nc - 1);
        while (p->hash[h] >= 0) h = (h + 1) & (unsigned)(nc - 1);
        p->hash[h] = i;
    }
}

static int add_fn(sprog *p, const char *name, int kind, int text)
{
    int i = find_fn(p, name);
    if (i >= 0) return i;
    if (p->nf == p->capf) { p->capf = p->capf ? p->capf * 2 : 512; p->f = xrealloc(p->f, (size_t)p->capf * sizeof *p->f); }
    i = p->nf++;
    memset(&p->f[i], 0, sizeof p->f[i]);
    p->f[i].name = xstrdup(name);
    p->f[i].kind = kind;
    p->f[i].text = text;
    if (p->nf * 2 > p->hcap) rehash(p);
    else {
        unsigned h = hstr(name) & (unsigned)(p->hcap - 1);
        while (p->hash[h] >= 0) h = (h + 1) & (unsigned)(p->hcap - 1);
        p->hash[h] = i;
    }
    return i;
}

static void set_open(sfn *f, const char *fmt, ...)
{
    if (f->open) return;
    f->open = 1;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(f->why, sizeof f->why, fmt, ap);
    va_end(ap);
}

static void sprog_free(sprog *p)
{
    for (int i = 0; i < p->nf; i++) {
        free(p->f[i].name); sv_free(&p->f[i].tcalls); sv_free(&p->f[i].xcalls); free(p->f[i].targb.v);
    }
    for (int i = 0; i < p->nt; i++) {
        free(p->t[i].obj); free(p->t[i].path);
        if (p->refs_c_text) sv_free(&p->refs_c_text[i]);
        if (p->addr_tab_text) sv_free(&p->addr_tab_text[i]);
    }
    free(p->refs_c_text); free(p->addr_tab_text);
    free(p->f); free(p->hash); free(p->t);
    sv_free(&p->addr_tab); sv_free(&p->refs_c); sv_free(&p->refs_asm); sv_free(&p->nmi_lit);
    sv_free(&p->nmi_why); sv_free(&p->asm_sf); sv_free(&p->no_b); sv_free(&p->problems);
    memset(p, 0, sizeof *p);
}

/* ------------------------------------------------------- line reading */

typedef struct {
    char line[2048];      /* the line without its comment */
    char label[256];      /* a column-0 label ("" when none) */
    char mn[64];          /* the first token after the label, lower case ("" when none) */
    char size;            /* its .b/.w/.l suffix, 0 when none */
    const char *op;       /* the rest, trimmed (points into line) */
} pline;

/* Strip `;` comments (outside quotes) and block comments (slash-star to star-slash) across lines;
 * *in_block carries the state. */
static void strip(const char *in, char *out, size_t n, int *in_block)
{
    size_t o = 0;
    char q = 0;
    for (const char *s = in; *s && *s != '\n' && *s != '\r' && o + 1 < n; s++) {
        if (*in_block) {
            if (s[0] == '*' && s[1] == '/') { *in_block = 0; s++; }
            continue;
        }
        if (q) { out[o++] = *s; if (*s == q) q = 0; continue; }
        if (*s == '"' || *s == '\'') {
            /* a quote in an operand is a character constant only when closed */
            if (*s == '\'' && !strchr(s + 1, '\'')) { out[o++] = *s; continue; }
            q = *s; out[o++] = *s; continue;
        }
        if (*s == ';') break;
        if (s[0] == '/' && s[1] == '*') { *in_block = 1; s++; continue; }
        out[o++] = *s;
    }
    while (o && isspace((unsigned char)out[o - 1])) o--;
    out[o] = '\0';
}

static void parse_line(const char *raw, pline *pl, int *in_block)
{
    strip(raw, pl->line, sizeof pl->line, in_block);
    pl->label[0] = pl->mn[0] = '\0';
    pl->size = 0;
    pl->op = "";
    char *p = pl->line;
    if (ident_start(*p)) {
        char name[256];
        size_t l = take_sym(p, name, sizeof name);
        const char *q = p + l;
        if (*q == ':' || *q == '\0' || isspace((unsigned char)*q)) {
            /* a label ends with ':' at column 0; a column-0 word without one is a label too (WLA-DX) */
            if (*q == ':' || !in_list(MNEMONICS, name, 1)) {
                snprintf(pl->label, sizeof pl->label, "%s", name);
                p += l;
                if (*p == ':') p++;
            }
        }
    }
    while (isspace((unsigned char)*p)) p++;
    /* a child label `@x:` (local to the last global label) before the instruction */
    if (*p == '@') {
        char *q = p + 1;
        while (ident_char(*q) || *q == '.' || *q == '\\' || *q == '@') q++;   /* `@nox\@:` in a macro */
        if (*q == ':') { p = q + 1; while (isspace((unsigned char)*p)) p++; }
    }
    /* an anonymous label before the instruction: `+ lda`, `-- bra` */
    if ((*p == '+' || *p == '-') && !pl->label[0]) {
        char *q = p;
        while (*q == '+' || *q == '-') q++;
        if (*q == '\0' || isspace((unsigned char)*q)) { p = q; while (isspace((unsigned char)*p)) p++; }
    }
    if (!*p) return;
    size_t l = 0;
    while ((ident_char(p[l]) || p[l] == '.' || p[l] == '@' || p[l] == '\\') && l + 1 < sizeof pl->mn) l++;
    if (!l) return;
    memcpy(pl->mn, p, l);
    pl->mn[l] = '\0';
    char *dot = pl->mn[0] == '.' ? NULL : strrchr(pl->mn, '.');
    if (dot && dot[1] && !dot[2] && strchr("bwlBWL", dot[1])) {
        pl->size = (char)tolower((unsigned char)dot[1]);
        *dot = '\0';
    }
    if (pl->mn[0] != '@')
        for (char *c = pl->mn; *c; c++) *c = (char)tolower((unsigned char)*c);
    p += l;
    while (isspace((unsigned char)*p)) p++;
    pl->op = p;
}

static int is_call_mn(const char *mn)
{
    return !strcmp(mn, "jsl") || !strcmp(mn, "jsr") || !strcmp(mn, "jml") || !strcmp(mn, "jmp");
}

static int ends_flow(const char *mn)
{
    static const char *const E[] = { "rts", "rtl", "rti", "jmp", "jml", "bra", "brl", "stp", NULL };
    return in_list(E, mn, 0);
}

static int is_data_directive(const char *mn)
{
    static const char *const D[] = { ".dw", ".dl", ".db", ".dsw", ".dsl", ".word", ".long", ".addr", ".faraddr",
                                     ".dwm", ".dlm", ".dbm", ".dd", NULL };
    return in_list(D, mn, 1);
}

/* every symbol named in an operand, not a register, not a local label */
static void refs_of(const char *op, svec *out)
{
    char name[256];
    for (const char *p = op; *p; ) {
        if (*p == '"' ) { p = strchr(p + 1, '"'); if (!p) return; p++; continue; }
        if (ident_start(*p) && (p == op || !(ident_char(p[-1]) || p[-1] == '@' || p[-1] == '.' || p[-1] == '$'))) {
            size_t l = take_sym(p, name, sizeof name);
            int reg = (strlen(name) == 1 && strchr("axysAXYS", name[0]));
            if (!reg) sv_add(out, name);
            p += l;
            continue;
        }
        p++;
    }
}

/* `sf.<fn>.f[+N]` in an operand: records the extent on fn (or the line, for
 * assembly), and whether the instruction says .b */
static void frame_operands(sprog *p, const pline *pl, int is_c, const char *where, int lineno)
{
    const char *op = pl->op;
    for (const char *s = strstr(op, "sf."); s; s = strstr(s + 1, "sf.")) {
        if (s > op && (ident_char(s[-1]) || s[-1] == '.')) continue;
        char sym[256];
        size_t l = take_sym(s, sym, sizeof sym);
        size_t sl = strlen(sym);
        if (sl < 6 || strcmp(sym + sl - 2, ".f") != 0) continue;   /* sf.X.f, X not empty */
        char msg[512];
        snprintf(msg, sizeof msg, "%s:%d: %s%s%s %s", where, lineno, pl->mn, pl->size ? "." : "",
                 pl->size ? (pl->size == 'b' ? "b" : pl->size == 'w' ? "w" : "l") : "", op);
        if (!is_c) { sv_add(&p->asm_sf, msg); continue; }
        if (pl->size != 'b') sv_add(&p->no_b, msg);
        char fn[256];
        snprintf(fn, sizeof fn, "%.*s", (int)(sl - 5), sym + 3);
        long off = 0;
        const char *q = s + l;
        while (isspace((unsigned char)*q)) q++;
        if (*q == '+') {
            q++;
            while (isspace((unsigned char)*q)) q++;
            off = *q == '$' ? strtol(q + 1, NULL, 16) : strtol(q, NULL, 0);
        }
        int width = (s > op && strchr(op, '[') && strchr(op, '[') < s) ? 3 : 2;
        int i = add_fn(p, fn, K_C, -1);
        if ((int)off + width > p->f[i].xext) p->f[i].xext = (int)off + width;
    }
}

/* ------------------------------------------------------------ C texts */

static void read_c_text(sprog *p, int ti, char **lines, int nl)
{
    stext *t = &p->t[ti];
    int cur = -1, in_body = 0, in_block = 0;
    char expect[256] = "";
    char last_mn[2][64] = { "", "" }, last_op[2][256] = { "", "" };
    svec *refs = &p->refs_c_text[ti];
    pline pl;
    for (int k = 0; k < nl; k++) {
        const char *raw = lines[k];
        if (!strncmp(raw, "; Function: ", 12)) {
            sscanf(raw + 12, "%255s", expect);
            continue;
        }
        if (!strncmp(raw, "; sframe ", 9)) {
            char a[256], b[256];
            int n;
            if (sscanf(raw + 9, "fn %255s", a) == 1) {
                int i = add_fn(p, a, K_C, ti);
                sfn *f = &p->f[i];
                f->tab = 1;
                const char *kv = strstr(raw + 12, " ");
                char key[32], val[64];
                for (const char *s = kv; s && *s; ) {
                    while (*s == ' ') s++;
                    if (sscanf(s, "%31[^=]=%63s", key, val) != 2) break;
                    if (!strcmp(key, "export")) f->exp_tab = atoi(val);
                    else if (!strcmp(key, "addrtaken")) f->addr_tab = atoi(val);
                    else if (!strcmp(key, "stack")) f->stack = atoi(val);
                    else if (!strcmp(key, "params")) f->params = atoi(val);
                    else if (!strcmp(key, "sf")) f->sf = atoi(val);
                    else if (!strcmp(key, "cand")) f->cand = !strcmp(val, "yes");
                    s = strchr(s, ' ');
                }
            } else if (sscanf(raw + 9, "call %255s %255s %d", a, b, &n) == 3) {
                int i = add_fn(p, a, K_C, ti);
                if (!strcmp(b, "*")) p->f[i].tind++;
                else if (!sv_has(&p->f[i].tcalls, b)) { sv_add(&p->f[i].tcalls, b); iv_add(&p->f[i].targb, n); }
                else {
                    /* a second call to the same callee: keep its largest argument block */
                    for (int k = 0; k < p->f[i].tcalls.n; k++)
                        if (!strcmp(p->f[i].tcalls.v[k], b) && n > p->f[i].targb.v[k]) p->f[i].targb.v[k] = n;
                }
            } else if (sscanf(raw + 9, "addr %255s", a) == 1) {
                sv_add(&p->addr_tab, a);
                sv_add(&p->addr_tab_text[ti], a);
            } else if (!strncmp(raw + 9, "table ", 6)) {
                t->has_table = 1;
            }
            continue;
        }
        parse_line(raw, &pl, &in_block);
        if (pl.label[0] && !in_body && !strcmp(pl.label, expect)) {
            cur = add_fn(p, pl.label, K_C, ti);
            p->f[cur].kind = K_C;
            p->f[cur].text = ti;
            p->f[cur].defined = 1;
            p->f[cur].line = k + 1;
            in_body = 1;
            expect[0] = '\0';
            last_mn[0][0] = last_mn[1][0] = '\0';
            continue;
        }
        if (!pl.mn[0]) continue;
        if (!strcasecmp(pl.mn, ".ends")) { in_body = 0; continue; }
        if (is_data_directive(pl.mn)) { refs_of(pl.op, refs); continue; }
        if (pl.mn[0] == '.') continue;
        if (!in_body || cur < 0) continue;
        sfn *f = &p->f[cur];
        frame_operands(p, &pl, 1, t->path, k + 1);
        if (is_call_mn(pl.mn)) {
            const char *o = pl.op;
            if (*o == '[' || *o == '(') {
                if (strncmp(o, "(@", 2) != 0) f->xind++;
            } else if (*o == '@' || *o == '+' || *o == '-' || *o == ':') {
                /* a local label */
            } else {
                char name[256];
                size_t l = take_sym(o, name, sizeof name);
                const char *rest = o + l;
                while (isspace((unsigned char)*rest)) rest++;
                if (!l || *rest) f->xind++;                  /* an expression: not a named target */
                else sv_add(&f->xcalls, name);
            }
            if (!strcmp(pl.mn, "jsl") || !strcmp(pl.mn, "jml")) {
                char name[256];
                if (take_sym(pl.op, name, sizeof name) && (!strcmp(name, "nmiSet") || !strcmp(name, "nmiSetLag"))) {
                    /* the callback: the two pushes right before the call (`pea.w :fn` / `pea.w fn`) */
                    char n0[256] = "", n1[256] = "";
                    int lit = !strcmp(last_mn[0], "pea") && !strcmp(last_mn[1], "pea");
                    if (lit) {
                        const char *a0 = last_op[0][0] == ':' ? last_op[0] + 1 : last_op[0];
                        const char *a1 = last_op[1][0] == ':' ? last_op[1] + 1 : last_op[1];
                        take_sym(a0, n0, sizeof n0);
                        take_sym(a1, n1, sizeof n1);
                    }
                    if (lit && n0[0] && !strcmp(n0, n1)) sv_add(&p->nmi_lit, n0);
                    else if (lit && strspn(last_op[0], "$0") == strlen(last_op[0]) && strspn(last_op[1], "$0") == strlen(last_op[1])) {
                        /* NULL: no callback */
                    } else {
                        char why[512];
                        snprintf(why, sizeof why, "%s passes a callback that is not a literal function", f->name);
                        sv_add(&p->nmi_why, why);
                    }
                }
            }
        } else {
            refs_of(pl.op, refs);
        }
        memcpy(last_mn[0], last_mn[1], sizeof last_mn[0]);
        memcpy(last_op[0], last_op[1], sizeof last_op[0]);
        snprintf(last_mn[1], sizeof last_mn[1], "%s", pl.mn);
        snprintf(last_op[1], sizeof last_op[1], "%s", pl.op);
    }
    for (int i = 0; i < refs->n; i++) sv_add(&p->refs_c, refs->v[i]);
}

/* ------------------------------------------------------- assembly texts */

typedef struct { char name[64]; svec args; svec body; } macro_t;
typedef struct { macro_t *v; int n, cap; } macros_t;

typedef struct {
    sprog *p;
    int    ti, cur, in_code, depth;
    int    pushed[16], npushed;          /* bytes of each pea/per not yet pulled back */
    const char *where;
    macros_t mac;
} asmstate;

static macro_t *find_macro(macros_t *m, const char *name)
{
    for (int i = 0; i < m->n; i++) if (!strcasecmp(m->v[i].name, name)) return &m->v[i];
    return NULL;
}

static void asm_instruction(asmstate *st, const pline *pl, int lineno);

/* a macro invocation: the body with the arguments put in, read as the routine's own lines */
static void expand(asmstate *st, macro_t *m, const char *argline, int lineno)
{
    if (st->depth > 8) return;
    char args[16][256];
    int na = 0;
    for (const char *s = argline; *s && na < 16; ) {
        while (isspace((unsigned char)*s)) s++;
        size_t l = strcspn(s, ",");
        size_t c = l < 255 ? l : 255;
        memcpy(args[na], s, c);
        args[na][c] = '\0';
        while (c && isspace((unsigned char)args[na][c - 1])) args[na][--c] = '\0';
        if (c) na++;
        s += l;
        if (*s == ',') s++;
    }
    st->depth++;
    for (int k = 0; k < m->body.n; k++) {
        char out[2048];
        size_t o = 0;
        const char *s = m->body.v[k];
        while (*s && o + 260 < sizeof out) {
            if (*s == '\\' && s[1] >= '1' && s[1] <= '9') {
                int a = s[1] - '1';
                if (a < na) o += (size_t)snprintf(out + o, sizeof out - o, "%s", args[a]);
                s += 2;
                continue;
            }
            if (ident_start(*s) && (s == m->body.v[k] || !ident_char(s[-1]))) {
                size_t l = 0;
                while (ident_char(s[l])) l++;
                int hit = -1;
                for (int a = 0; a < m->args.n && a < na; a++)
                    if (strlen(m->args.v[a]) == l && !strncmp(m->args.v[a], s, l)) hit = a;
                if (hit >= 0) o += (size_t)snprintf(out + o, sizeof out - o, "%s", args[hit]);
                else { memcpy(out + o, s, l); o += l; }
                s += l;
                continue;
            }
            out[o++] = *s++;
        }
        out[o] = '\0';
        pline pl;
        int blk = 0;
        parse_line(out, &pl, &blk);
        pl.label[0] = '\0';                    /* labels of a macro body are its own */
        asm_instruction(st, &pl, lineno);
    }
    st->depth--;
}

static void asm_instruction(asmstate *st, const pline *pl, int lineno)
{
    sprog *p = st->p;
    if (!pl->mn[0]) return;
    if (pl->mn[0] == '.') {
        if (is_data_directive(pl->mn)) {
            refs_of(pl->op, &p->refs_asm);
            if (st->cur >= 0) p->f[st->cur].hasdata = 1;
        }
        return;
    }
    if (st->cur < 0 || !st->in_code) return;
    sfn *f = &p->f[st->cur];
    if (!in_list(MNEMONICS, pl->mn, 0)) {
        macro_t *m = find_macro(&st->mac, pl->mn);
        if (m) { expand(st, m, pl->op, lineno); return; }
        if (in_list(SECTION_MACROS, pl->mn, 1)) return;
        set_open(f, "%s:%d: `%s` is not an instruction nor a macro of this file", st->where, lineno, pl->mn);
        return;
    }
    char ins[2400];
    snprintf(ins, sizeof ins, "%s %s", pl->mn, pl->op);
    f->fp = fnv(f->fp ? f->fp : 1469598103934665603ULL, ins);
    f->lastflow = ends_flow(pl->mn);
    frame_operands(p, pl, 0, st->where, lineno);
    const char *mn = pl->mn, *o = pl->op;
    if (is_call_mn(mn)) {
        if (*o == '[' || *o == '(') {
            if (strncmp(o, "(@", 2) != 0) { f->xind++; set_open(f, "%s:%d: %s %s (a jump through a pointer)", st->where, lineno, mn, o); }
        } else if (*o == '@' || *o == '+' || *o == '-') {
            /* a local label */
        } else {
            char name[256], full[512];
            size_t l = take_sym(o, name, sizeof name);
            const char *rest = o + l;
            while (isspace((unsigned char)*rest)) rest++;
            if (!l || *rest) {
                f->xind++;
                set_open(f, "%s:%d: %s %s (a target computed by an expression)", st->where, lineno, mn, o);
            } else {
                if (name[0] == '_') { snprintf(full, sizeof full, "%s:%s", p->t[st->ti].stem, name); sv_add(&f->xcalls, full); }
                else sv_add(&f->xcalls, name);
            }
        }
    } else {
        refs_of(o, &p->refs_asm);
        if ((!strcmp(mn, "sta") || !strcmp(mn, "stx") || !strcmp(mn, "sty") || !strcmp(mn, "stz"))
            && (strstr(o, "nmi_callback") || strstr(o, "nmi_lag_callback")) && strcmp(p->t[st->ti].stem, "crt0") != 0) {
            char why[600];
            snprintf(why, sizeof why, "%s:%d writes the NMI callback (%s %s)", st->where, lineno, mn, o);
            sv_add(&p->nmi_why, why);
        }
    }
    /* a computed return: bytes pushed by pea/per, not pulled back, at an rtl/rts */
    if (!strcmp(mn, "pea") || !strcmp(mn, "per")) {
        if (st->npushed < 16) st->pushed[st->npushed++] = 2;
    } else if (!strcmp(mn, "plb") && st->npushed) {
        st->pushed[st->npushed - 1] -= 1;
    } else if ((!strcmp(mn, "pla") || !strcmp(mn, "plx") || !strcmp(mn, "ply") || !strcmp(mn, "pld")) && st->npushed) {
        st->pushed[st->npushed - 1] -= 2;
    }
    while (st->npushed && st->pushed[st->npushed - 1] <= 0) st->npushed--;
    if ((!strcmp(mn, "rtl") || !strcmp(mn, "rts")) && st->npushed) {
        f->xind++;
        set_open(f, "%s:%d: %s after a pea/per not pulled back (a computed return)", st->where, lineno, mn);
    }
    if (!strcmp(mn, "jsl") || !strcmp(mn, "jsr") || !strcmp(mn, "rtl") || !strcmp(mn, "rts")) st->npushed = 0;
}

static void read_asm_text(sprog *p, int ti, char **lines, int nl)
{
    stext *t = &p->t[ti];
    asmstate st;
    memset(&st, 0, sizeof st);
    st.p = p; st.ti = ti; st.cur = -1; st.in_code = 1; st.where = strrchr(t->path, '/') ? strrchr(t->path, '/') + 1 : t->path;
    macro_t *recording = NULL;
    int in_block = 0;
    pline pl;
    for (int k = 0; k < nl; k++) {
        parse_line(lines[k], &pl, &in_block);
        if (recording) {
            if (!strcasecmp(pl.mn, ".endm")) { recording = NULL; continue; }
            sv_push(&recording->body, pl.line);
            continue;
        }
        if (!strcasecmp(pl.mn, ".macro")) {
            if (st.mac.n == st.mac.cap) { st.mac.cap = st.mac.cap ? st.mac.cap * 2 : 8; st.mac.v = xrealloc(st.mac.v, (size_t)st.mac.cap * sizeof *st.mac.v); }
            recording = &st.mac.v[st.mac.n++];
            memset(recording, 0, sizeof *recording);
            char name[64] = "";
            sscanf(pl.op, "%63[^ \t]", name);
            snprintf(recording->name, sizeof recording->name, "%s", name);
            const char *a = ci_find(pl.op, " ARGS");
            if (a) {
                char tmp[512];
                snprintf(tmp, sizeof tmp, "%s", a + 5);
                for (char *tok = strtok(tmp, ", \t"); tok; tok = strtok(NULL, ", \t")) {
                    /* keep order and duplicates out: argument names are distinct */
                    sv_add(&recording->args, tok);
                }
            }
            continue;
        }
        if (pl.mn[0] == '.') {
            const char *d = pl.mn;
            if (!strcasecmp(d, ".ramsection") || !strcasecmp(d, ".struct") || !strcasecmp(d, ".enum")) { st.in_code = 0; st.cur = -1; }
            else if (!strcasecmp(d, ".ends") || !strcasecmp(d, ".ende")) { st.in_code = 1; st.cur = -1; }
            else if (!strcasecmp(d, ".section") || !strcasecmp(d, ".orga") || !strcasecmp(d, ".org")) st.cur = -1;
            else if (!strcasecmp(d, ".define") || !strcasecmp(d, ".def")) {
                /* `.DEFINE __mul16 tcc_mul16`: a second name of a routine. Only a plain symbol and nothing
                 * after it (`.DEFINE MS_ID tcc__lf+0` names a byte of the direct page, not code) */
                char a[256], b[256], tmp[256], rest[8];
                if (sscanf(pl.op, "%255s %255s %7s", a, b, rest) == 2 && ident_start(a[0]) && ident_start(b[0])
                    && take_sym(b, tmp, sizeof tmp) == strlen(b)) {
                    int i = add_fn(p, a, K_ALIAS, ti);
                    if (p->f[i].kind == K_ALIAS && !p->f[i].defined) { p->f[i].defined = 1; sv_add(&p->f[i].xcalls, b); }
                }
            }
        }
        if (pl.label[0] && strcmp(pl.label, "__") != 0) {
            char name[512];
            if (pl.label[0] == '_') snprintf(name, sizeof name, "%s:%s", t->stem, pl.label);
            else snprintf(name, sizeof name, "%s", pl.label);
            if (st.in_code) {
                int prev = st.cur;
                int i = find_fn(p, name);
                if (i >= 0 && (p->f[i].kind == K_C || p->f[i].defined)) {
                    st.cur = -1;                          /* a C function (or a first definition) keeps its name */
                } else {
                    i = add_fn(p, name, K_ASM, ti);
                    p->f[i].kind = K_ASM; p->f[i].text = ti; p->f[i].defined = 1; p->f[i].line = k + 1;
                    st.cur = i;
                }
                /* the routine before falls into this one: its last instruction does not end the flow, or it
                 * has none (two names for one routine); a label of data before it does not run */
                if (prev >= 0 && !p->f[prev].lastflow && (p->f[prev].fp || !p->f[prev].hasdata))
                    sv_add(&p->f[prev].xcalls, name);
                st.npushed = 0;
            }
        }
        asm_instruction(&st, &pl, k + 1);
    }
    for (int i = 0; i < st.mac.n; i++) { sv_free(&st.mac.v[i].args); sv_free(&st.mac.v[i].body); }
    free(st.mac.v);
}

/* ------------------------------------------------------------ loading */

static char **read_lines(const char *path, int *nl)
{
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    char **v = NULL;
    int n = 0, cap = 0;
    char buf[4096];
    while (fgets(buf, sizeof buf, f)) {
        if (n == cap) { cap = cap ? cap * 2 : 1024; v = xrealloc(v, (size_t)cap * sizeof *v); }
        size_t l = strlen(buf);
        while (l && (buf[l - 1] == '\n' || buf[l - 1] == '\r')) buf[--l] = '\0';
        v[n++] = xstrdup(buf);
    }
    fclose(f);
    *nl = n;
    return v;
}

static int file_exists(const char *p) { FILE *f = fopen(p, "r"); if (!f) return 0; fclose(f); return 1; }

/* Reads the linkfile's objects and their texts. Returns 0, or CLI_IO. */
static int load(cli_ctx *ctx, const char *linkfile, sprog *p)
{
    memset(p, 0, sizeof *p);
    int nl = 0;
    char **lines = read_lines(linkfile, &nl);
    if (!lines) { cli_error(ctx, linkfile, "cannot open the linkfile (make writes it beside the ROM)"); return CLI_IO; }
    char dir[1024] = ".";
    const char *sl = strrchr(linkfile, '/');
    if (sl) snprintf(dir, sizeof dir, "%.*s", (int)(sl - linkfile), linkfile);
    int in_objects = 0;
    for (int i = 0; i < nl; i++) {
        char *s = lines[i];
        while (isspace((unsigned char)*s)) s++;
        if (*s == '[') { in_objects = !strncmp(s, "[objects]", 9); continue; }
        if (!in_objects || !*s || *s == ';') continue;
        char obj[1024], base[1024], path[1100];
        if (*s == '/' || (isalpha((unsigned char)s[0]) && s[1] == ':')) snprintf(obj, sizeof obj, "%s", s);
        else snprintf(obj, sizeof obj, "%s/%s", dir, s);
        size_t ol = strlen(obj);
        snprintf(base, sizeof base, "%.*s", (int)(ol > 2 && !strcmp(obj + ol - 2, ".o") ? ol - 2 : ol), obj);
        const char *found = NULL;
        static const char *const SUFFIX[] = { ".wrap.asm", ".c.asm", ".text.asm", NULL };
        for (int k = 0; SUFFIX[k] && !found; k++) {
            snprintf(path, sizeof path, "%s%s", base, SUFFIX[k]);
            if (file_exists(path)) found = path;
        }
        p->t = xrealloc(p->t, (size_t)(p->nt + 1) * sizeof *p->t);
        stext *t = &p->t[p->nt++];
        memset(t, 0, sizeof *t);
        t->obj = xstrdup(obj);
        t->path = found ? xstrdup(found) : NULL;
        t->lib = strstr(obj, "lib/build/") != NULL;
        const char *b = strrchr(base, '/') ? strrchr(base, '/') + 1 : base;
        snprintf(t->stem, sizeof t->stem, "%s", b);
        if (!found) {
            char m[1200];
            snprintf(m, sizeof m, "%s has no text beside it (X.wrap.asm, X.c.asm, X.text.asm): what it defines is UNKNOWN", obj);
            sv_add(&p->problems, m);
        }
    }
    for (int i = 0; i < nl; i++) free(lines[i]);
    free(lines);
    p->refs_c_text = xrealloc(NULL, (size_t)(p->nt ? p->nt : 1) * sizeof *p->refs_c_text);
    p->addr_tab_text = xrealloc(NULL, (size_t)(p->nt ? p->nt : 1) * sizeof *p->addr_tab_text);
    memset(p->refs_c_text, 0, (size_t)(p->nt ? p->nt : 1) * sizeof *p->refs_c_text);
    memset(p->addr_tab_text, 0, (size_t)(p->nt ? p->nt : 1) * sizeof *p->addr_tab_text);
    /* C texts first: a C function keeps its name against a label of the same name in assembly */
    for (int pass = 0; pass < 2; pass++)
        for (int ti = 0; ti < p->nt; ti++) {
            stext *t = &p->t[ti];
            if (!t->path) continue;
            int n = 0;
            char **ls = read_lines(t->path, &n);
            if (!ls) continue;
            int is_c = 0;
            for (int k = 0; k < n && !is_c; k++)
                if (!strncmp(ls[k], "; Function: ", 12) || !strncmp(ls[k], "; sframe ", 9)) is_c = 1;
            t->is_c = is_c;
            if (pass == 0 && is_c) read_c_text(p, ti, ls, n);
            if (pass == 1 && !is_c) read_asm_text(p, ti, ls, n);
            for (int k = 0; k < n; k++) free(ls[k]);
            free(ls);
        }
    /* the reviewed routines */
    for (int r = 0; REVIEWED[r].name; r++) {
        int i = find_fn(p, REVIEWED[r].name);
        if (i < 0 || p->f[i].kind != K_ASM || p->f[i].text < 0) continue;
        if (strcmp(p->t[p->f[i].text].stem, REVIEWED[r].object) != 0) continue;
        if (getenv("OPENSNES_SFRAME_FINGERPRINTS"))
            fprintf(stderr, "fingerprint %s (%s) 0x%016llxULL\n", REVIEWED[r].name, REVIEWED[r].object, p->f[i].fp);
        p->f[i].reviewed = p->f[i].fp == REVIEWED[r].fp ? 1 : -1;
        /* OPENSNES_SFRAME_FAULT=noreview: the strict closure (S0's `closure` model), every computed jump open */
        if (getenv("OPENSNES_SFRAME_FAULT") && !strcmp(getenv("OPENSNES_SFRAME_FAULT"), "noreview")) p->f[i].reviewed = -1;
    }
    p->unk = p->nf;
    return 0;
}

/* ------------------------------------------------------------ the graph */

enum { G_TABLE, G_TEXT };

typedef struct { ivec *succ; int n; char *addrtaken; } graph;

static int is_exported_name(const sfn *f, int mode)
{
    if (f->kind != K_C) return 0;
    return mode == G_TABLE ? f->exp_tab : strchr(f->name, '.') == NULL;
}

static int resolve(const sprog *p, const char *name)
{
    int i = find_fn(p, name);
    if (i < 0 || !p->f[i].defined) return p->unk;
    return i;
}

static void edge(graph *g, int a, int b)
{
    for (int k = 0; k < g->succ[a].n; k++) if (g->succ[a].v[k] == b) return;
    iv_add(&g->succ[a], b);
}

static void build(const sprog *p, int mode, graph *g)
{
    g->n = p->nf + 1;
    g->succ = xrealloc(NULL, (size_t)g->n * sizeof *g->succ);
    memset(g->succ, 0, (size_t)g->n * sizeof *g->succ);
    g->addrtaken = xrealloc(NULL, (size_t)g->n);
    memset(g->addrtaken, 0, (size_t)g->n);
    const svec *a = mode == G_TABLE ? &p->addr_tab : &p->refs_c;
    for (int k = 0; k < a->n; k++) { int i = find_fn(p, a->v[k]); if (i >= 0 && p->f[i].defined) g->addrtaken[i] = 1; }
    for (int k = 0; k < p->refs_asm.n; k++) { int i = find_fn(p, p->refs_asm.v[k]); if (i >= 0 && p->f[i].defined) g->addrtaken[i] = 1; }
    for (int i = 0; i < p->nf; i++) {
        const sfn *f = &p->f[i];
        if (!f->defined) continue;
        int indirect = 0;
        if (f->kind == K_C) {
            int table = mode == G_TABLE;
            if (table && !(p->t[f->text].has_table && f->tab)) { edge(g, i, p->unk); continue; }
            const svec *cs = table ? &f->tcalls : &f->xcalls;
            for (int k = 0; k < cs->n; k++) {
                if (table && is_helper(cs->v[k])) continue;
                edge(g, i, resolve(p, cs->v[k]));
            }
            /* place trusts the named helpers to be closed; the check follows them through their text */
            indirect = table ? f->tind > 0 : f->xind > 0;
        } else {
            for (int k = 0; k < f->xcalls.n; k++) edge(g, i, resolve(p, f->xcalls.v[k]));
            if (f->kind == K_ASM && (f->open || f->xind) && f->reviewed != 1) edge(g, i, p->unk);
        }
        if (indirect)
            for (int j = 0; j < p->nf; j++) if (g->addrtaken[j]) edge(g, i, j);
    }
    for (int j = 0; j < p->nf; j++) {
        const sfn *f = &p->f[j];
        if (!f->defined) continue;
        if (g->addrtaken[j] || is_exported_name(f, mode) || (f->kind == K_ASM && strchr(f->name, ':') == NULL))
            edge(g, p->unk, j);
    }
}

static void graph_free(graph *g)
{
    for (int i = 0; i < g->n; i++) free(g->succ[i].v);
    free(g->succ); free(g->addrtaken);
    memset(g, 0, sizeof *g);
}

/* the nodes reachable from the successors of `from` (and from `from` itself when self is set) */
static void reach(const graph *g, int from, int self, char *seen, int *par)
{
    memset(seen, 0, (size_t)g->n);
    int *st = xrealloc(NULL, (size_t)g->n * sizeof *st);
    int n = 0;
    if (self) { seen[from] = 1; st[n++] = from; if (par) par[from] = -1; }
    else
        for (int k = 0; k < g->succ[from].n; k++) {
            int c = g->succ[from].v[k];
            if (!seen[c]) { seen[c] = 1; if (par) par[c] = from; st[n++] = c; }
        }
    /* breadth first, so that a path read back is a shortest one */
    for (int h = 0; h < n; h++) {
        int x = st[h];
        for (int k = 0; k < g->succ[x].n; k++) {
            int c = g->succ[x].v[k];
            if (!seen[c]) { seen[c] = 1; if (par) par[c] = x; st[n++] = c; }
        }
    }
    free(st);
}

static const char *node_name(const sprog *p, int i) { return i == p->unk ? "UNKNOWN" : p->f[i].name; }

/* "f -> a -> b -> f" from the BFS parents (a path from `from` back to `to`) */
static void path_text(const sprog *p, const int *par, int from, int to, char *out, size_t n)
{
    int chain[256], k = 0, x = to;
    chain[k++] = to;
    while (k < 255) {
        x = par[x];
        if (x < 0 || x == from) break;
        chain[k++] = x;
    }
    size_t o = (size_t)snprintf(out, n, "%s", node_name(p, from));
    int prev = from;
    for (int i = k - 1; i >= 0 && o < n; i--) {
        /* why a node reaches UNKNOWN: its open reason, or the callee no text defines */
        if (chain[i] == p->unk && prev != p->unk && o < n) {
            const sfn *f = &p->f[prev];
            if (f->open) o += (size_t)snprintf(out + o, n - o, " [%s]", f->why);
            else if (f->kind == K_C && !(f->text >= 0 && p->t[f->text].has_table && f->tab))
                o += (size_t)snprintf(out + o, n - o, " [a C text without a table]");
            else {
                const svec *cs = f->kind == K_C && f->tab ? &f->tcalls : &f->xcalls;
                for (int c = 0; c < cs->n; c++)
                    if (find_fn(p, cs->v[c]) < 0 || !p->f[find_fn(p, cs->v[c])].defined) {
                        o += (size_t)snprintf(out + o, n - o, " [calls %s, which no linked text defines]", cs->v[c]);
                        break;
                    }
            }
        }
        if (o < n) o += (size_t)snprintf(out + o, n - o, " -> %s", node_name(p, chain[i]));
        prev = chain[i];
    }
}

/* ------------------------------------------------------------ frames */

typedef struct { int fn, size, off; } frame_t;

/* the framed functions: the table's claim, or (what-if) every compiler candidate of the project */
static int frames_of(const sprog *p, int whatif, frame_t **out)
{
    frame_t *v = NULL;
    int n = 0;
    if (!whatif) {
        for (int i = 0; i < p->nf; i++)
            if (p->f[i].kind == K_C && p->f[i].defined && p->f[i].sf > 0) {
                v = xrealloc(v, (size_t)(n + 1) * sizeof *v);
                v[n].fn = i; v[n].size = p->f[i].sf; v[n].off = 0; n++;
            }
        *out = v;
        return n;
    }
    /* what-if (the S0 count's model): a candidate on a cycle of its own file's direct calls stays on the stack;
     * its frame is its stack temps plus its parameters (its own count, or the largest block a caller sends) */
    int *argmax = xrealloc(NULL, (size_t)(p->nf + 1) * sizeof *argmax);
    memset(argmax, 0, (size_t)(p->nf + 1) * sizeof *argmax);
    for (int i = 0; i < p->nf; i++)
        for (int k = 0; k < p->f[i].tcalls.n; k++) {
            int c = find_fn(p, p->f[i].tcalls.v[k]);
            if (c >= 0 && p->f[i].targb.v[k] > argmax[c]) argmax[c] = p->f[i].targb.v[k];
        }
    char *seen = xrealloc(NULL, (size_t)p->nf + 1);
    for (int i = 0; i < p->nf; i++) {
        const sfn *f = &p->f[i];
        if (f->kind != K_C || !f->defined || !f->cand || p->t[f->text].lib) continue;
        /* on a cycle of the file: f reaches itself through direct calls inside its text */
        memset(seen, 0, (size_t)p->nf + 1);
        int st[4096], sn = 0, cyc = 0;
        for (int k = 0; k < f->tcalls.n; k++) {
            int c = find_fn(p, f->tcalls.v[k]);
            if (c >= 0 && p->f[c].kind == K_C && p->f[c].text == f->text && !seen[c] && sn < 4096) { seen[c] = 1; st[sn++] = c; }
        }
        while (sn && !cyc) {
            int x = st[--sn];
            if (x == i) { cyc = 1; break; }
            for (int k = 0; k < p->f[x].tcalls.n; k++) {
                int c = find_fn(p, p->f[x].tcalls.v[k]);
                if (c >= 0 && p->f[c].kind == K_C && p->f[c].text == f->text && !seen[c] && sn < 4096) { seen[c] = 1; st[sn++] = c; }
            }
        }
        if (cyc) continue;
        v = xrealloc(v, (size_t)(n + 1) * sizeof *v);
        v[n].fn = i;
        v[n].size = f->stack + (f->params > argmax[i] ? f->params : argmax[i]);
        v[n].off = 0;
        n++;
    }
    free(seen); free(argmax);
    *out = v;
    return n;
}

typedef struct {
    int nref;
    char **refusal;         /* one line each */
} refusals;

static void refuse(refusals *r, const char *fmt, ...)
{
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    r->refusal = xrealloc(r->refusal, (size_t)(r->nref + 1) * sizeof *r->refusal);
    r->refusal[r->nref++] = xstrdup(buf);
}

static void refusals_free(refusals *r)
{
    for (int i = 0; i < r->nref; i++) free(r->refusal[i]);
    free(r->refusal);
    memset(r, 0, sizeof *r);
}

/* re-entry: a framed function reachable from itself */
static void reentries(const sprog *p, const graph *g, frame_t *fr, int nfr, refusals *r, char *alive)
{
    char *seen = xrealloc(NULL, (size_t)g->n);
    int *par = xrealloc(NULL, (size_t)g->n * sizeof *par);
    for (int a = 0; a < nfr; a++) {
        alive[a] = 1;
        reach(g, fr[a].fn, 0, seen, par);
        if (seen[fr[a].fn]) {
            char path[4096];
            path_text(p, par, fr[a].fn, fr[a].fn, path, sizeof path);
            refuse(r, "reentry: %s has a static frame and can be active twice: %s", p->f[fr[a].fn].name, path);
            alive[a] = 0;
        }
    }
    free(seen); free(par);
}

/* anc[a][b] = frame b can be active when frame a runs (b reaches a) */
static char *ancestors(const graph *g, frame_t *fr, int nfr)
{
    char *anc = xrealloc(NULL, (size_t)(nfr ? nfr * nfr : 1));
    memset(anc, 0, (size_t)(nfr ? nfr * nfr : 1));
    char *seen = xrealloc(NULL, (size_t)g->n);
    for (int b = 0; b < nfr; b++) {
        reach(g, fr[b].fn, 0, seen, NULL);
        for (int a = 0; a < nfr; a++) if (a != b && seen[fr[a].fn]) anc[a * nfr + b] = 1;
    }
    free(seen);
    return anc;
}

/* offsets: each frame just past its deepest framed ancestor (longest path) */
static int place_offsets(frame_t *fr, int nfr, const char *anc, const char *alive)
{
    char *done = xrealloc(NULL, (size_t)(nfr ? nfr : 1));
    memset(done, 0, (size_t)(nfr ? nfr : 1));
    int left = 0, deep = 0;
    for (int a = 0; a < nfr; a++) if (alive[a]) left++;
    while (left) {
        int progress = 0;
        for (int a = 0; a < nfr; a++) {
            if (!alive[a] || done[a]) continue;
            int ready = 1, off = 0;
            for (int b = 0; b < nfr; b++) {
                if (!alive[b] || !anc[a * nfr + b]) continue;
                if (!done[b]) { ready = 0; break; }
                if (fr[b].off + fr[b].size > off) off = fr[b].off + fr[b].size;
            }
            if (!ready) continue;
            fr[a].off = off;
            done[a] = 1; left--; progress = 1;
            if (off + fr[a].size > deep) deep = off + fr[a].size;
        }
        if (!progress) break;           /* cannot happen once the re-entries are out */
    }
    free(done);
    return deep;
}

/* ------------------------------------------------------------ place */

static long parse_num(const char *s, long deflt)
{
    if (!s || !*s) return deflt;
    char *end;
    long v = *s == '$' ? strtol(s + 1, &end, 16) : strtol(s, &end, 0);
    return *end ? -1 : v;
}

static int cmp_frame_off(const void *a, const void *b)
{
    const frame_t *x = a, *y = b;
    return x->off != y->off ? x->off - y->off : x->fn - y->fn;
}

int run_place(cli_ctx *ctx)
{
    const char *linkfile = ctx->args[0];
    sprog p;
    int rc = load(ctx, linkfile, &p);
    if (rc) return rc;
    char outbuf[1100];
    const char *out = cli_str(ctx, "out", NULL);
    if (!out) {
        const char *sl = strrchr(linkfile, '/');
        if (sl) snprintf(outbuf, sizeof outbuf, "%.*s/sframe.asm", (int)(sl - linkfile), linkfile);
        else snprintf(outbuf, sizeof outbuf, "sframe.asm");
        out = outbuf;
    }
    long base = parse_num(cli_str(ctx, "window-base", "$A8"), 0xA8);
    if (base < 0 || base > 0xFF) { cli_error(ctx, NULL, "--window-base wants an address of the direct page ($00-$FF)"); sprog_free(&p); return CLI_USAGE; }
    int whatif = cli_has(ctx, "what-if");
    const char *fault = getenv("OPENSNES_SFRAME_FAULT");
    fault = fault ? fault : "";

    graph g;
    build(&p, G_TABLE, &g);
    frame_t *fr;
    int nfr = frames_of(&p, whatif, &fr);
    char *alive = xrealloc(NULL, (size_t)(nfr ? nfr : 1));
    refusals r = { 0, NULL };
    reentries(&p, &g, fr, nfr, &r, alive);
    if (!strcmp(fault, "reentry")) for (int a = 0; a < nfr; a++) alive[a] = 1;
    char *anc = ancestors(&g, fr, nfr);
    /* with a re-entry the ancestor relation has a cycle: a refused frame is out of the placement */
    int deep = place_offsets(fr, nfr, anc, alive);
    if (!strcmp(fault, "overlap")) for (int a = 0; a < nfr; a++) fr[a].off = 0;
    long room = 0x100 - base;
    if (!whatif && deep > room)
        refuse(&r, "the deepest chain of static frames takes %d bytes and the window $%02lX-$FF holds %ld", deep, base, room);

    int refused = r.nref > 0 && strcmp(fault, "reentry") != 0;
    if (whatif) {
        /* the count of the S0 script, for the comparison: refused names, deepest chain */
        if (ctx->json) {
            cli_json_begin(ctx);
            cli_json_str(ctx, "linkfile", linkfile);
            cli_json_int(ctx, "candidates", nfr);
            cli_json_int(ctx, "deepest", deep);
            cli_json_array(ctx, "refused");
            for (int a = 0; a < nfr; a++) if (!alive[a]) cli_json_str(ctx, NULL, p.f[fr[a].fn].name);
            cli_json_close(ctx);
            cli_json_end(ctx);
        } else {
            int nref = 0;
            for (int a = 0; a < nfr; a++) nref += !alive[a];
            printf("what-if: %d candidates, %d refused, deepest chain %d bytes\n", nfr, nref, deep);
            for (int i = 0; i < r.nref; i++) printf("  refused: %s\n", r.refusal[i]);
        }
        rc = CLI_OK;
    } else if (refused) {
        cli_error(ctx, linkfile, "static frames refused, sframe.asm not written (the link fails on its labels):");
        for (int i = 0; i < r.nref; i++) fprintf(stderr, "  %s\n", r.refusal[i]);
        fprintf(stderr, "  A function with a static frame must never be active twice: build its file without static frames.\n");
        if (remove(out) != 0 && errno != ENOENT) cli_warn(ctx, out, "could not remove the old window: %s", strerror(errno));
        rc = CLI_REFUSED;
    } else {
        FILE *f = fopen(out, "w");
        if (!f) { cli_error(ctx, out, "cannot write: %s", strerror(errno)); rc = CLI_IO; }
        else {
            int nlive = 0;
            for (int a = 0; a < nfr; a++) nlive += alive[a];
            fprintf(f, "; sframe.asm, written by opensnes-rom place from %s: the window of the static frames.\n", linkfile);
            if (!nlive) fprintf(f, "; No function has a static frame: no section, no sf.* label.\n");
            else {
                frame_t *sorted = xrealloc(NULL, (size_t)nfr * sizeof *sorted);
                int ns = 0;
                for (int a = 0; a < nfr; a++) if (alive[a]) sorted[ns++] = fr[a];
                qsort(sorted, (size_t)ns, sizeof *sorted, cmp_frame_off);
                fprintf(f, "; %d framed functions, deepest chain %d bytes, from $%02lX.\n", ns, deep, base);
                fprintf(f, ".RAMSECTION \".sframe\" BANK 0 SLOT 1 ORGA $%04lX FORCE\n", base);
                for (int a = 0; a < ns; a++) {
                    int next = a + 1 < ns ? sorted[a + 1].off : deep;
                    if (next < sorted[a].off) next = sorted[a].off;
                    fprintf(f, "sf.%s.f DSB %d   ; %d bytes at +%d\n", p.f[sorted[a].fn].name, next - sorted[a].off,
                            sorted[a].size, sorted[a].off);
                }
                fprintf(f, ".ENDS\n");
                free(sorted);
            }
            fclose(f);
            if (!ctx->json && !ctx->quiet) printf("OK: %s: %d static frame%s, deepest chain %d bytes\n", out, nlive, nlive == 1 ? "" : "s", deep);
        }
    }
    for (int i = 0; i < p.problems.n && ctx->verbose; i++) cli_note(ctx, "%s", p.problems.v[i]);
    free(anc); free(alive); free(fr);
    refusals_free(&r);
    graph_free(&g);
    sprog_free(&p);
    return rc;
}

/* ------------------------------------------------------------ check */

static int sym_ram_section(const sym_file *s, const char *name, long *addr, long *size)
{
    for (size_t i = 0; i < s->nramsections; i++)
        if (!strcmp(s->ramsections[i].name, name) && s->ramsections[i].bank == 0) {
            *addr = s->ramsections[i].address; *size = (long)s->ramsections[i].size;
            return 1;
        }
    return 0;
}

int check_sframe(cli_ctx *ctx, const sym_file *s, const char *linkfile)
{
    sprog p;
    if (load(ctx, linkfile, &p)) return 1;
    refusals r = { 0, NULL };

    /* 1. each C table against its text */
    int nc = 0, mism = 0;
    for (int i = 0; i < p.nf; i++) {
        sfn *f = &p.f[i];
        if (f->kind != K_C || f->text < 0) continue;
        stext *t = &p.t[f->text];
        if (!t->has_table) continue;
        nc++;
        if (f->defined && !f->tab) { refuse(&r, "table/text: %s (%s) has no table line", f->name, t->path); mism++; continue; }
        if (!f->defined) { refuse(&r, "table/text: the table of %s names %s, which its text does not define", t->path, f->name); mism++; continue; }
        for (int k = 0; k < f->xcalls.n; k++)
            if (!is_helper(f->xcalls.v[k]) && !sv_has(&f->tcalls, f->xcalls.v[k])) {
                refuse(&r, "table/text: %s calls %s and its table does not say so (%s)", f->name, f->xcalls.v[k], t->path);
                mism++;
            }
        for (int k = 0; k < f->tcalls.n; k++)
            if (!sv_has(&f->xcalls, f->tcalls.v[k])) {
                refuse(&r, "table/text: the table of %s lists a call to %s that its text does not make (%s)", f->name, f->tcalls.v[k], t->path);
                mism++;
            }
        if ((f->tind > 0) != (f->xind > 0)) {
            refuse(&r, "table/text: %s calls through a pointer %d time(s) by its table and %d by its text (%s)", f->name, f->tind, f->xind, t->path);
            mism++;
        }
    }
    for (int ti = 0; ti < p.nt; ti++) {
        if (!p.t[ti].is_c || !p.t[ti].has_table) continue;
        svec *tx = &p.refs_c_text[ti], *tb = &p.addr_tab_text[ti];
        for (int k = 0; k < tx->n; k++) {
            int i = find_fn(&p, tx->v[k]);
            if (i >= 0 && p.f[i].defined && p.f[i].kind != K_ALIAS && !sv_has(tb, tx->v[k])) {
                refuse(&r, "table/text: %s takes the address of %s and its table does not say so", p.t[ti].path, tx->v[k]);
                mism++;
            }
        }
        for (int k = 0; k < tb->n; k++) {
            int i = find_fn(&p, tb->v[k]);
            if (i >= 0 && p.f[i].defined && p.f[i].kind != K_ALIAS && !sv_has(tx, tb->v[k])) {
                refuse(&r, "table/text: the table of %s says the address of %s is taken and its text does not take it", p.t[ti].path, tb->v[k]);
                mism++;
            }
        }
    }

    /* 2. the frames' extent, read from the operands */
    for (int i = 0; i < p.asm_sf.n; i++) refuse(&r, "frame: assembly names a static frame: %s", p.asm_sf.v[i]);
    for (int i = 0; i < p.no_b.n; i++) refuse(&r, "frame: an access to a static frame without .b (a direct-page operand): %s", p.no_b.v[i]);
    for (int i = 0; i < p.nf; i++) {
        sfn *f = &p.f[i];
        if (!f->xext) continue;
        if (f->kind != K_C || !f->defined) refuse(&r, "frame: sf.%s.f is used and %s is not a C function of the program", f->name, f->name);
        else if (f->xext > f->sf) refuse(&r, "frame: %s uses %d bytes of its static frame and its table declares %d", f->name, f->xext, f->sf);
    }

    /* 3. the .sym's sf.* labels equal the tables */
    long win_a = 0, win_s = 0;
    int has_win = sym_ram_section(s, ".sframe", &win_a, &win_s);
    frame_t *fr = NULL;
    int nfr = frames_of(&p, 0, &fr);
    long *addr = xrealloc(NULL, (size_t)(nfr ? nfr : 1) * sizeof *addr);
    for (int a = 0; a < nfr; a++) {
        char lab[600];
        snprintf(lab, sizeof lab, "sf.%s.f", p.f[fr[a].fn].name);
        const sym_label *l = sym_find(s, lab);
        addr[a] = -1;
        if (!l) refuse(&r, "sym: %s has a static frame of %d bytes and the .sym has no %s", p.f[fr[a].fn].name, fr[a].size, lab);
        else addr[a] = (long)l->bank << 16 | l->address;
    }
    for (size_t k = 0; k < s->nlabels; k++) {
        const char *n = s->labels[k].name;
        size_t l = strlen(n);
        if (l < 6 || strncmp(n, "sf.", 3) != 0 || strcmp(n + l - 2, ".f") != 0) continue;
        char fn[512];
        snprintf(fn, sizeof fn, "%.*s", (int)(l - 5), n + 3);
        int i = find_fn(&p, fn);
        if (i < 0 || p.f[i].kind != K_C || p.f[i].sf <= 0)
            refuse(&r, "sym: the .sym defines %s and no table gives %s a static frame", n, fn);
    }

    /* 4. where the frames are */
    if (has_win) {
        if (win_a + win_s > 0x100) refuse(&r, "window: .sframe ($%04lX, %ld bytes) ends past $FF", win_a, win_s);
        for (size_t k = 0; k < s->nramsections; k++) {
            const sym_section *o = &s->ramsections[k];
            if (o->bank != 0 || !strcmp(o->name, ".sframe") || !o->size || !win_s) continue;
            if (o->address < win_a + win_s && win_a < o->address + (long)o->size)
                refuse(&r, "window: .sframe ($%04lX-$%04lX) overlaps %s ($%04X-$%04lX)", win_a, win_a + win_s - 1, o->name,
                       o->address, o->address + (long)o->size - 1);
        }
    }
    long nmi_a = 0, nmi_page = 0;
    int has_nmi = sym_ram_section(s, ".nmi_registers", &nmi_a, &nmi_page);
    (void)nmi_a;
    for (int a = 0; a < nfr; a++) {
        if (addr[a] < 0) continue;
        const char *n = p.f[fr[a].fn].name;
        long A = addr[a], E = addr[a] + fr[a].size;
        if ((A >> 16) != 0 || E > 0x100) { refuse(&r, "window: the frame of %s ($%06lX, %d bytes) ends past $FF", n, A, fr[a].size); continue; }
        if (!has_win || A < win_a || E > win_a + win_s) refuse(&r, "window: the frame of %s ($%04lX-$%04lX) is not inside .sframe", n, A, E - 1);
        for (size_t k = 0; k < s->nramsections; k++) {
            const sym_section *o = &s->ramsections[k];
            if (o->bank != 0 || !strcmp(o->name, ".sframe") || !o->size) continue;
            if (o->address < E && A < o->address + (long)o->size)
                refuse(&r, "window: the frame of %s ($%04lX-$%04lX) lies on %s", n, A, E - 1, o->name);
        }
    }

    /* 5. the graph from the text: re-entry, overlap, the NMI */
    graph g;
    build(&p, G_TEXT, &g);
    char *alive = xrealloc(NULL, (size_t)(nfr ? nfr : 1));
    reentries(&p, &g, fr, nfr, &r, alive);
    char *anc = ancestors(&g, fr, nfr);
    for (int a = 0; a < nfr; a++)
        for (int b = 0; b < nfr; b++) {
            if (!anc[a * nfr + b] || addr[a] < 0 || addr[b] < 0) continue;
            if (addr[a] < addr[b] + fr[b].size && addr[b] < addr[a] + fr[a].size)
                refuse(&r, "overlap: %s ($%04lX, %d bytes) can run while %s ($%04lX, %d bytes) is active, and their frames overlap",
                       p.f[fr[a].fn].name, addr[a], fr[a].size, p.f[fr[b].fn].name, addr[b], fr[b].size);
        }
    if (nfr) {
        char *seen = xrealloc(NULL, (size_t)g.n), *nmi = xrealloc(NULL, (size_t)g.n);
        memset(nmi, 0, (size_t)g.n);
        int fallback = p.nmi_why.n > 0;
        for (int i = 0; i <= p.nf; i++) {
            int root = 0;
            if (i < p.nf && p.f[i].defined) {
                if (fallback) root = g.addrtaken[i] || is_exported_name(&p.f[i], G_TEXT);
                else root = sv_has(&p.nmi_lit, p.f[i].name);
            }
            if (!root) continue;
            reach(&g, i, 1, seen, NULL);
            for (int k = 0; k < g.n; k++) nmi[k] |= seen[k];
        }
        for (int a = 0; a < nfr; a++) {
            if (!nmi[fr[a].fn] || addr[a] < 0) continue;
            long E = (addr[a] & 0xFFFF) + fr[a].size;
            if (!has_nmi || E > nmi_page)
                refuse(&r, "nmi: %s can run in the NMI (%s) and its frame ends at $%04lX, past the NMI page (%ld bytes)",
                       p.f[fr[a].fn].name, fallback ? p.nmi_why.v[0] : "an nmiSet callback reaches it", E, nmi_page);
        }
        free(seen); free(nmi);
    }

    int bad = r.nref > 0;
    if (bad) {
        cli_error(ctx, linkfile, "static frames: the link is refused (%d finding%s):", r.nref, r.nref == 1 ? "" : "s");
        for (int i = 0; i < r.nref; i++) fprintf(stderr, "  %s\n", r.refusal[i]);
    } else if (!ctx->json && !ctx->quiet) {
        int ntext = 0;
        for (int ti = 0; ti < p.nt; ti++) ntext += p.t[ti].path != NULL;
        printf("OK: static frames: %d framed, %d C functions' tables equal their text, %d of %d objects read\n",
               nfr, nc, ntext, p.nt);
    }
    if (ctx->verbose) for (int i = 0; i < p.problems.n; i++) cli_note(ctx, "%s", p.problems.v[i]);
    free(anc); free(alive); free(addr); free(fr);
    graph_free(&g);
    refusals_free(&r);
    sprog_free(&p);
    (void)mism;
    return bad;
}
