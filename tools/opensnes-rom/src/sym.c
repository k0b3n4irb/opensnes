/* sym.c — the wlalink .sym reader (see sym.h). The blocks it reads:
 *   [labels]       BB:AAAA name
 *   [definitions]  VVVVVVVV name
 *   [sections]     ROMOFF8 BB:AAAA CPUA SIZE8 name
 *   [ramsections]  BB:AAAA ABSA SIZE8 name
 * Rows that do not fit the block's shape are skipped; nothing else in the
 * file is read. */
#include "sym.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sym_rom_bank(int bank)
{
    /* HiROM units carry .BASE $C0 and FastROM units .BASE $80: fold the
     * CPU-visible window back to the linker bank (symmap.rom_bank). */
    if (bank >= 0xC0) return bank - 0xC0;
    if (bank >= 0x80 && bank <= 0xBF) return bank - 0x80;
    return bank;
}

static int hex(const char *s, int n, unsigned *out)
{
    unsigned v = 0;
    for (int i = 0; i < n; i++) {
        char c = s[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
        else return 0;
    }
    *out = v;
    return 1;
}

static char *dup_token(const char *s)
{
    size_t n = strcspn(s, " \t\r\n");
    char *d = malloc(n + 1);
    if (!d) return NULL;
    memcpy(d, s, n);
    d[n] = '\0';
    return d;
}

static int push_label(sym_label **arr, size_t *n, size_t *cap, sym_label l)
{
    if (*n == *cap) {
        size_t nc = *cap ? *cap * 2 : 256;
        sym_label *na = realloc(*arr, nc * sizeof **arr);
        if (!na) return 0;
        *arr = na; *cap = nc;
    }
    (*arr)[(*n)++] = l;
    return 1;
}

static int push_section(sym_section **arr, size_t *n, size_t *cap, sym_section l)
{
    if (*n == *cap) {
        size_t nc = *cap ? *cap * 2 : 64;
        sym_section *na = realloc(*arr, nc * sizeof **arr);
        if (!na) return 0;
        *arr = na; *cap = nc;
    }
    (*arr)[(*n)++] = l;
    return 1;
}

static int cmp_label(const void *a, const void *b)
{
    return strcmp(((const sym_label *)a)->name, ((const sym_label *)b)->name);
}

int sym_read(const char *path, sym_file *s, char *why, size_t why_len)
{
    memset(s, 0, sizeof *s);
    FILE *f = fopen(path, "r");
    if (!f) { snprintf(why, why_len, "cannot open"); return -1; }
    size_t cl = 0, cd = 0, cs = 0, cr = 0;
    enum { NONE, LABELS, DEFS, SECTIONS, RAMSECTIONS } block = NONE;
    char line[1024];
    int ok = 1;
    while (ok && fgets(line, sizeof line, f)) {
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (!*p || *p == ';') continue;
        if (*p == '[') {
            if (!strncmp(p, "[labels]", 8)) block = LABELS;
            else if (!strncmp(p, "[definitions]", 13)) block = DEFS;
            else if (!strncmp(p, "[sections]", 10)) block = SECTIONS;
            else if (!strncmp(p, "[ramsections]", 13)) block = RAMSECTIONS;
            else block = NONE;
            continue;
        }
        unsigned b, a, v, sz, cpua;
        switch (block) {
        case LABELS:
            if (hex(p, 2, &b) && p[2] == ':' && hex(p + 3, 4, &a) && isspace((unsigned char)p[7])) {
                const char *name = p + 7;
                while (isspace((unsigned char)*name)) name++;
                sym_label l = { dup_token(name), sym_rom_bank((int)b), (int)a, (b << 16) | a };
                if (!l.name || !push_label(&s->labels, &s->nlabels, &cl, l)) ok = 0;
            }
            break;
        case DEFS:
            if (hex(p, 8, &v) && isspace((unsigned char)p[8])) {
                const char *name = p + 8;
                while (isspace((unsigned char)*name)) name++;
                sym_label l = { dup_token(name), (int)((v >> 16) & 0xFF), (int)(v & 0xFFFF), v };
                if (!l.name || !push_label(&s->defs, &s->ndefs, &cd, l)) ok = 0;
            }
            break;
        case SECTIONS:
            /* ROMOFF8 BB:AAAA CPUA SIZE8 name */
            if (hex(p, 8, &v) && p[8] == ' ' && hex(p + 9, 2, &b) && p[11] == ':' && hex(p + 12, 4, &a)
                && p[16] == ' ' && hex(p + 17, 4, &cpua) && p[21] == ' ' && hex(p + 22, 8, &sz)
                && isspace((unsigned char)p[30])) {
                const char *name = p + 30;
                while (isspace((unsigned char)*name)) name++;
                sym_section sec = { dup_token(name), sym_rom_bank((int)b), (int)cpua, sz };
                if (!sec.name || !push_section(&s->sections, &s->nsections, &cs, sec)) ok = 0;
            }
            break;
        case RAMSECTIONS:
            /* BB:AAAA ABSA SIZE8 name — the first address is slot-relative, the second absolute */
            if (hex(p, 2, &b) && p[2] == ':' && hex(p + 3, 4, &a) && p[7] == ' ' && hex(p + 8, 4, &cpua)
                && p[12] == ' ' && hex(p + 13, 8, &sz) && isspace((unsigned char)p[21])) {
                const char *name = p + 21;
                while (isspace((unsigned char)*name)) name++;
                sym_section sec = { dup_token(name), sym_rom_bank((int)b), (int)cpua, sz };
                if (!sec.name || !push_section(&s->ramsections, &s->nramsections, &cr, sec)) ok = 0;
            }
            break;
        default:
            break;
        }
    }
    fclose(f);
    if (!ok) { sym_free(s); snprintf(why, why_len, "out of memory"); return -1; }
    if (s->nlabels == 0) { sym_free(s); snprintf(why, why_len, "no [labels] block — not a wlalink .sym file"); return -1; }
    qsort(s->labels, s->nlabels, sizeof *s->labels, cmp_label);
    qsort(s->defs, s->ndefs, sizeof *s->defs, cmp_label);
    return 0;
}

void sym_free(sym_file *s)
{
    for (size_t i = 0; i < s->nlabels; i++) free(s->labels[i].name);
    for (size_t i = 0; i < s->ndefs; i++) free(s->defs[i].name);
    for (size_t i = 0; i < s->nsections; i++) free(s->sections[i].name);
    for (size_t i = 0; i < s->nramsections; i++) free(s->ramsections[i].name);
    free(s->labels); free(s->defs); free(s->sections); free(s->ramsections);
    memset(s, 0, sizeof *s);
}

static const sym_label *find_in(const sym_label *arr, size_t n, const char *name)
{
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        int c = strcmp(arr[mid].name, name);
        if (c == 0) return &arr[mid];
        if (c < 0) lo = mid + 1; else hi = mid;
    }
    return NULL;
}

const sym_label *sym_find(const sym_file *s, const char *name) { return find_in(s->labels, s->nlabels, name); }
const sym_label *sym_find_def(const sym_file *s, const char *name) { return find_in(s->defs, s->ndefs, name); }
