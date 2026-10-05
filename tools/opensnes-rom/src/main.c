/*
 * opensnes-rom — is the ROM good? The post-link checks a user build runs
 * (the family's successor of five Python scripts, so that `make` in a
 * project needs no interpreter; .claude/rules/two_audiences.md).
 *
 *   opensnes-rom check [--bank0-fail N] [--bank0-warn N] [--ram-fail N] [--ram-warn N]
 *                      [--no-bank-reads] [--no-nmi-race] [--no-assets] [--json] <game.sfc | game.sym>
 *
 * Reads the wlalink .sym beside the ROM and the .c.asm intermediates in
 * its directory, and runs: the bank $00 ROM ratchet, the C RAM band
 * budget (and the far band figure), the data-init sentinel, the bank-blind
 * read guard, the NMI / WRAM-port race lint, and the asset inventory.
 * Exit 1 when a check fails; warnings do not fail. Header, checksum and
 * region (`finalize`, `inspect`) follow in a later lot.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "sym.h"
#include "checks.h"

static const cli_opt check_opts[] = {
    { "bank0-fail", 0, CLI_INT, "N", "fail when bank $00 ROM has fewer than N bytes free (0 = never)", "0" },
    { "bank0-warn", 0, CLI_INT, "N", "warn when bank $00 ROM has fewer than N bytes free", "2048" },
    { "ram-fail", 0, CLI_INT, "N", "fail when the C RAM band ($0000-$1FFF) has fewer than N bytes free (0 = never)", "0" },
    { "ram-warn", 0, CLI_INT, "N", "warn when the C RAM band has fewer than N bytes free", "1024" },
    { "no-bank-reads", 0, CLI_FLAG, NULL, "skip the bank-blind read guard", NULL },
    { "no-nmi-race", 0, CLI_FLAG, NULL, "skip the NMI / WRAM-port race lint", NULL },
    { "no-assets", 0, CLI_FLAG, NULL, "skip the asset inventory line", NULL },
};

static int run_check(cli_ctx *ctx)
{
    const char *in = ctx->args[0];
    char sym_path[1024], dir[1024];
    size_t l = strlen(in);
    if (l > 4 && strcmp(in + l - 4, ".sfc") == 0) snprintf(sym_path, sizeof sym_path, "%.*s.sym", (int)(l - 4), in);
    else snprintf(sym_path, sizeof sym_path, "%s", in);
    const char *slash = strrchr(in, '/');
    if (slash) snprintf(dir, sizeof dir, "%.*s", (int)(slash - in), in); else snprintf(dir, sizeof dir, ".");

    sym_file s;
    char why[160];
    if (sym_read(sym_path, &s, why, sizeof why) != 0) {
        cli_error(ctx, sym_path, "%s — link the ROM first (wlalink writes the .sym beside it)", why);
        return CLI_IO;
    }
    check_results r;
    memset(&r, 0, sizeof r);
    r.bank0 = check_bank0(ctx, &s, in, cli_int(ctx, "bank0-warn", 2048), cli_int(ctx, "bank0-fail", 0), &r);
    r.ram = check_ram(ctx, &s, in, cli_int(ctx, "ram-warn", 1024), cli_int(ctx, "ram-fail", 0), &r);
    r.datainit = check_data_init(ctx, &s, in);
    if (!cli_has(ctx, "no-bank-reads")) r.bankreads = check_bank_reads(ctx, &s, dir);
    if (!cli_has(ctx, "no-nmi-race")) r.nmirace = check_nmi_race(ctx, dir);
    if (!cli_has(ctx, "no-assets")) report_assets(ctx, dir, &r);
    sym_free(&s);

    int failed = r.bank0 == 1 || r.ram == 1 || r.datainit == 1 || r.bankreads == 1 || r.nmirace == 1;
    if (ctx->json) {
        cli_json_begin(ctx);
        cli_json_str(ctx, "input", in);
        cli_json_bool(ctx, "ok", !failed);
        cli_json_object(ctx, "bank0"); cli_json_int(ctx, "free", r.bank0_free); cli_json_int(ctx, "verdict", r.bank0); cli_json_close(ctx);
        cli_json_object(ctx, "ram"); cli_json_int(ctx, "free", r.ram_free); cli_json_int(ctx, "top", r.ram_top);
        cli_json_int(ctx, "used", r.ram_total); cli_json_int(ctx, "sections", r.ram_sections); cli_json_int(ctx, "verdict", r.ram); cli_json_close(ctx);
        cli_json_object(ctx, "far_ram"); cli_json_int(ctx, "free", r.far_free); cli_json_int(ctx, "top", r.far_top); cli_json_int(ctx, "sections", r.far_sections); cli_json_close(ctx);
        cli_json_int(ctx, "data_init", r.datainit);
        cli_json_int(ctx, "bank_reads", r.bankreads);
        cli_json_int(ctx, "nmi_race", r.nmirace);
        cli_json_object(ctx, "assets"); cli_json_int(ctx, "vram_bytes", r.asset_vram); cli_json_int(ctx, "colours", r.asset_colours);
        cli_json_int(ctx, "files", r.asset_files); cli_json_close(ctx);
        cli_json_end(ctx);
    }
    return failed ? CLI_REFUSED : CLI_OK;
}

static const cli_cmd cmds[] = {
    { "check", "the post-link checks: bank $00 ROM, C RAM band, data-init sentinel, bank-blind reads, NMI / WRAM-port race, asset inventory",
      check_opts, CLI_N(check_opts), "check game.sfc --bank0-fail 1024 --ram-fail 512", 1, run_check },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes-rom", TOOL_VERSION, "is the ROM good? post-link checks, and later the header, checksum and region", cmds, CLI_N(cmds),
    };
    return cli_main(&tool, argc, argv);
}
