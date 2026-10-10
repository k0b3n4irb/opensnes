/* checks.h — the post-link checks of opensnes-rom, one function each. Every
 * check prints its lines through the reporter in main.c and returns 0
 * (pass), 1 (fail) or 2 (warning). */
#ifndef OPENSNES_ROM_CHECKS_H
#define OPENSNES_ROM_CHECKS_H

#include "cli.h"
#include "sym.h"

typedef struct {
    long bank0_free, ram_free, ram_top, ram_total, far_free, far_top;
    int  ram_sections, far_sections;
    long asset_vram, asset_colours;
    int  asset_files;
    int  bank0, ram, datainit, bankreads, nmirace;   /* each check's verdict: 0 / 1 / 2 */
} check_results;

int check_bank0(cli_ctx *ctx, const sym_file *s, const char *rom, int warn, int fail, check_results *r);
int check_ram(cli_ctx *ctx, const sym_file *s, const char *rom, int warn, int fail, check_results *r);
int check_data_init(cli_ctx *ctx, const sym_file *s, const char *rom);
int check_bank_reads(cli_ctx *ctx, const sym_file *s, const char *dir);
int check_nmi_race(cli_ctx *ctx, const char *dir);
void report_assets(cli_ctx *ctx, const char *dir, check_results *r);

#endif
