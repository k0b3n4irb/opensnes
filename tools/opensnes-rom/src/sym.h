/* sym.h — the wlalink .sym file, read once for every post-link check. */
#ifndef OPENSNES_ROM_SYM_H
#define OPENSNES_ROM_SYM_H

#include <stddef.h>

typedef struct {
    char    *name;
    int      bank;        /* linker bank: the $C0/$80 HiROM/FastROM windows folded back */
    int      address;     /* 16-bit */
    unsigned raw;         /* [definitions]: the 32-bit value (for _sizeof_*) */
} sym_label;

typedef struct {
    char    *name;
    int      bank;
    int      address;     /* CPU address for [sections]; absolute for [ramsections] */
    unsigned size;
} sym_section;

typedef struct {
    sym_label   *labels;      size_t nlabels;       /* [labels], sorted by name */
    sym_label   *defs;        size_t ndefs;         /* [definitions], sorted by name */
    sym_section *sections;    size_t nsections;     /* [sections] */
    sym_section *ramsections; size_t nramsections;  /* [ramsections] */
} sym_file;

/* Returns 0 and fills *s, or -1 with the reason in why[]. */
int  sym_read(const char *path, sym_file *s, char *why, size_t why_len);
void sym_free(sym_file *s);
const sym_label *sym_find(const sym_file *s, const char *name);       /* a label */
const sym_label *sym_find_def(const sym_file *s, const char *name);   /* a definition */
int  sym_rom_bank(int bank);                                          /* the folding */

#endif
