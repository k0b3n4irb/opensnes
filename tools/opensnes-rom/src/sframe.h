/* sframe.h — static frames at link time (L4): `opensnes-rom place` writes the
 * window of the static frames from the program's call graph, and `check
 * --objects` verifies the linked ROM against an independent reading of the
 * same objects. See sframe.c. */
#ifndef OPENSNES_ROM_SFRAME_H
#define OPENSNES_ROM_SFRAME_H

#include "cli.h"
#include "sym.h"

/* `opensnes-rom place LINKFILE`: writes sframe.asm, or refuses (and removes
 * an old one) on a re-entry or a window too small. */
int run_place(cli_ctx *ctx);

/* The frame checks of `check --objects LINKFILE` (0 pass, 1 fail). */
int check_sframe(cli_ctx *ctx, const sym_file *s, const char *linkfile);

#endif
