/*
 * opensnes-music — Impulse Tracker modules → a SNESMOD soundbank for the
 * SPC700 (the 1.x successor of smconv; same converter, same bytes).
 *
 *   opensnes-music bank    [--name NAME] [--bank N] [--same-size] [--out DIR] [--json] <a.it> [<b.it>...]
 *   opensnes-music spc     [--out DIR] [--json] <a.it>...
 *   opensnes-music inspect [--json] <a.it>...
 *
 * The converter is smconv's (tools/smconv/src: itloader, it2spc, brr —
 * SNESMOD by Mukunda Johnson, (C) 2009): a soundbank from here is byte for
 * byte what `smconv -s -o NAME -b N -n -p NAME` wrote. `bank` writes
 * NAME.asm, NAME.h and NAME.bnk; `spc` writes a standalone .spc per module
 * for a player; `inspect` says what each module costs in SPC RAM.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "itloader.h"
#include "it2spc.h"
#include "report.h"

/* The converter's diagnostics, in the family's shape: errors and warnings
 * on stderr with the tool's name, the verbose report only under -v, and
 * every warning kept for the --json "warnings" array. */
static cli_ctx *g_ctx;
static char g_warnings[16][256];
static int g_nwarnings;

static void music_report(smc_level level, const char *file, const char *msg, void *user)
{
    (void)user;
    switch (level) {
    case SMC_INFO:
        cli_note(g_ctx, "%s", msg);
        break;
    case SMC_NOTE: case SMC_WARNING:
        cli_warn(g_ctx, file, "%s", msg);
        if (g_nwarnings < 16) snprintf(g_warnings[g_nwarnings++], sizeof g_warnings[0], "%s", msg);
        break;
    case SMC_ERROR: case SMC_FATAL:
        cli_error(g_ctx, file, "%s", msg);
        break;
    }
}

static void json_warnings(cli_ctx *ctx)
{
    cli_json_array(ctx, "warnings");
    for (int i = 0; i < g_nwarnings; i++) cli_json_str(ctx, NULL, g_warnings[i]);
    cli_json_close(ctx);
}

/* Bytes a module may take: 64 KB minus the driver and its module base
 * ($18CA), minus the two 616-byte headers it2spc.c subtracts when it
 * reports `bytesfree` under -V — the same figure smconv prints. */
#define SPC_RAM_FOR_MUSIC (65535 - 0x18ca - 616 - 616)

static itl_bank_t *load_modules(cli_ctx *ctx, const char **files, int n, int *rc)
{
    itl_bank_t *bank = itl_bank_create(files, n);
    *rc = CLI_OK;
    if (!bank) { cli_error(ctx, files[0], "cannot load the modules"); *rc = CLI_IO; return NULL; }
    for (int i = 0; i < bank->module_count; i++)
        if (bank->modules[i]->invalid) *rc = CLI_REFUSED;   /* the converter has said why, in the family's shape */
    if (*rc != CLI_OK) { itl_bank_destroy(bank); return NULL; }
    return bank;
}

static void json_modules(cli_ctx *ctx, const itl_bank_t *bank, const spc_bank_t *out)
{
    cli_json_array(ctx, "modules");
    for (int i = 0; i < bank->module_count; i++) {
        const itl_module_t *m = bank->modules[i];
        cli_json_object(ctx, NULL);
        cli_json_str(ctx, "input", m->filename);
        cli_json_str(ctx, "title", m->title);
        cli_json_int(ctx, "patterns", m->pattern_count);
        cli_json_int(ctx, "instruments", m->instrument_count);
        cli_json_int(ctx, "samples", m->sample_count);
        cli_json_int(ctx, "orders", m->length);
        if (out && i < out->module_count) {
            cli_json_int(ctx, "spc_ram_bytes", (long)out->modules[i]->totalsize);
            cli_json_int(ctx, "spc_ram_free", (long)SPC_RAM_FOR_MUSIC - (long)out->modules[i]->totalsize);
            cli_json_int(ctx, "sources", out->modules[i]->source_list_count);
        }
        cli_json_close(ctx);
    }
    cli_json_close(ctx);
    if (out) cli_json_int(ctx, "brr_sources", out->source_count);
}

/* ------------------------------------------------------------------ bank */

static int ends_with(const char *s, const char *suffix)
{
    size_t ls = strlen(s), lx = strlen(suffix);
    return ls > lx && strcmp(s + ls - lx, suffix) == 0;
}

static const cli_opt bank_opts[] = {
    { "inputs", 0, CLI_LIST, "FILES", "the modules of a composed soundbank, in NAME.toml (`bank NAME.toml`); relative to that file", NULL },
    { "name", 0, CLI_STR, "NAME", "base name of the outputs and prefix of the symbols", "the first module's name, or soundbank for several" },
    { "bank", 0, CLI_INT, "N", "ROM bank the soundbank data is linked in", "1" },
    { "same-size", 0, CLI_FLAG, NULL, "check every module's SPC RAM size against the first one's (a sound-effect bank)", NULL },
    CLI_OPT_OUT,
    CLI_OPT_SAVE,
};

static int run_bank(cli_ctx *ctx)
{
    int rc;
    g_ctx = ctx;
    /* A composed asset: `bank soundbank.toml` — the file names the modules
     * (inputs = [...], relative to it) and carries the settings; the outputs
     * take its name and sit beside it. */
    const char *files[CLI_MAX_ARGS];
    int nfiles = ctx->nargs;
    char listbuf[2048], resolved[CLI_MAX_ARGS][1024], name[256];
    const char *anchor = ctx->args[0];
    int composed = ctx->nargs == 1 && ends_with(ctx->args[0], ".toml");
    if (composed) {
        rc = cli_load_settings_path(ctx, ctx->args[0]);
        if (rc != CLI_OK) return rc;
        const char *words[CLI_MAX_ARGS];
        nfiles = cli_list(ctx, "inputs", listbuf, sizeof listbuf, words, CLI_MAX_ARGS);
        if (nfiles == 0) { cli_error(ctx, ctx->args[0], "no `inputs = [\"a.it\", ...]` under [bank] — nothing to convert"); return CLI_REFUSED; }
        const char *slash = strrchr(ctx->args[0], '/');
        int dirlen = slash ? (int)(slash - ctx->args[0] + 1) : 0;
        for (int i = 0; i < nfiles; i++) {
            if (words[i][0] == '/') snprintf(resolved[i], sizeof resolved[i], "%s", words[i]);
            else snprintf(resolved[i], sizeof resolved[i], "%.*s%s", dirlen, ctx->args[0], words[i]);
            files[i] = resolved[i];
        }
        cli_stem(ctx->args[0], name, sizeof name);
    } else {
        for (int i = 0; i < nfiles; i++) files[i] = ctx->args[i];
    }
    itl_bank_t *bank = load_modules(ctx, files, nfiles, &rc);
    if (!bank) return rc;

    if (!composed) {
        if (cli_has(ctx, "name")) snprintf(name, sizeof name, "%s", cli_str(ctx, "name", "soundbank"));
        else if (nfiles == 1) cli_stem(files[0], name, sizeof name);
        else snprintf(name, sizeof name, "soundbank");
    }
    int banknum = cli_int(ctx, "bank", 1);
    if (banknum < 1 || banknum > 255) {
        cli_error(ctx, anchor, "--bank %d is not a ROM bank the data can live in (1 to 255; bank 0 holds the code)", banknum);
        itl_bank_destroy(bank);
        return CLI_REFUSED;
    }
    char base[1024], probe[1024];
    /* the base path: cli_output_path on a fake "<name>.x" beside the anchor (first input or the .toml) or in --out */
    snprintf(probe, sizeof probe, "%s", anchor);
    char *slash = strrchr(probe, '/');
    if (slash) snprintf(slash + 1, sizeof probe - (size_t)(slash + 1 - probe), "%s.it", name);
    else snprintf(probe, sizeof probe, "%s.it", name);
    if (cli_output_path(ctx, probe, "", base, sizeof base) != CLI_OK) { itl_bank_destroy(bank); return CLI_IO; }

    spc_bank_t *out = spc_bank_create(bank, false, cli_has(ctx, "same-size"));
    if (!out) { cli_error(ctx, ctx->args[0], "the conversion produced nothing"); itl_bank_destroy(bank); return CLI_REFUSED; }
    for (int i = 0; i < out->module_count; i++)
        if (out->modules[i]->totalsize > SPC_RAM_FOR_MUSIC)
            cli_warn(ctx, bank->modules[i]->filename, "needs %u bytes of SPC RAM, more than the %d a module may take — drop samples or patterns",
                     out->modules[i]->totalsize, SPC_RAM_FOR_MUSIC);
    spc_bank_export(out, base, true, name, banknum);

    if (cli_has(ctx, "save") && !composed) {
        /* the composed form of this run: NAME.toml beside the outputs, listing the modules */
        char toml[1100], list[2048] = "";
        snprintf(toml, sizeof toml, "%s.toml", base);
        for (int i = 0; i < nfiles; i++) {
            const char *bn = strrchr(files[i], '/') ? strrchr(files[i], '/') + 1 : files[i];
            size_t l = strlen(list);
            snprintf(list + l, sizeof list - l, "%s%s", l ? " " : "", bn);
        }
        int idx = -1;
        for (int i = 0; i < ctx->cmd->nopts; i++) if (strcmp(ctx->cmd->opts[i].name, "inputs") == 0) idx = i;
        if (idx >= 0) ctx->given[idx] = list;
        rc = cli_save_settings_path(ctx, toml, name);
        if (rc != CLI_OK) { spc_bank_destroy(out); itl_bank_destroy(bank); return rc; }
    }

    if (ctx->json) {
        cli_json_begin(ctx);
        cli_json_str(ctx, "name", name);
        cli_json_int(ctx, "bank", banknum);
        cli_json_array(ctx, "outputs");
        const char *exts[] = { ".asm", ".h", ".bnk" };
        const char *kinds[] = { "asm", "header", "bank" };
        for (int i = 0; i < 3; i++) {
            char path[1100]; snprintf(path, sizeof path, "%s%s", base, exts[i]);
            cli_json_object(ctx, NULL); cli_json_str(ctx, "path", path); cli_json_str(ctx, "kind", kinds[i]); cli_json_close(ctx);
        }
        cli_json_close(ctx);
        json_modules(ctx, bank, out);
        json_warnings(ctx);
        cli_json_end(ctx);
    } else if (!ctx->quiet) {
        printf("%s: %s.asm, %s.h, %s.bnk (%d module%s, %d BRR source%s, bank %d)\n", ctx->tool->name,
               base, base, base, out->module_count, out->module_count == 1 ? "" : "s",
               out->source_count, out->source_count == 1 ? "" : "s", banknum);
    }
    spc_bank_destroy(out);
    itl_bank_destroy(bank);
    return CLI_OK;
}

/* ------------------------------------------------------------------- spc */

static const cli_opt spc_opts[] = { CLI_OPT_OUT };

static int run_spc(cli_ctx *ctx)
{
    int worst = CLI_OK;
    g_ctx = ctx;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "files"); }
    for (int i = 0; i < ctx->nargs; i++) {
        int rc;
        itl_bank_t *bank = load_modules(ctx, &ctx->args[i], 1, &rc);
        if (!bank) { if (rc > worst) worst = rc; continue; }
        char path[1024];
        if (cli_output_path(ctx, ctx->args[i], ".spc", path, sizeof path) != CLI_OK) { itl_bank_destroy(bank); worst = CLI_IO; continue; }
        spc_bank_t *out = spc_bank_create(bank, false, false);
        if (!out) { cli_error(ctx, ctx->args[i], "the conversion produced nothing"); itl_bank_destroy(bank); worst = CLI_REFUSED; continue; }
        spc_bank_make_spc(out, path);
        if (ctx->json) {
            cli_json_object(ctx, NULL); cli_json_str(ctx, "input", ctx->args[i]); cli_json_str(ctx, "path", path);
            cli_json_int(ctx, "spc_ram_bytes", (long)out->modules[0]->totalsize); cli_json_close(ctx);
        } else if (!ctx->quiet) {
            printf("%s: %s -> %s\n", ctx->tool->name, ctx->args[i], path);
        }
        spc_bank_destroy(out);
        itl_bank_destroy(bank);
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

/* --------------------------------------------------------------- inspect */

static int run_inspect(cli_ctx *ctx)
{
    int rc;
    g_ctx = ctx;
    itl_bank_t *bank = load_modules(ctx, ctx->args, ctx->nargs, &rc);
    if (!bank) return rc;
    spc_bank_t *out = spc_bank_create(bank, false, false);
    if (ctx->json) {
        cli_json_begin(ctx);
        json_modules(ctx, bank, out);
        json_warnings(ctx);
        cli_json_end(ctx);
    } else {
        for (int i = 0; i < bank->module_count; i++) {
            const itl_module_t *m = bank->modules[i];
            printf("%s: \"%s\", %u patterns, %u instruments, %u samples, %u orders", m->filename, m->title,
                   m->pattern_count, m->instrument_count, m->sample_count, m->length);
            if (out && i < out->module_count)
                printf(" -> %u bytes of SPC RAM (%ld free of %d), %d BRR sources", out->modules[i]->totalsize,
                       (long)SPC_RAM_FOR_MUSIC - (long)out->modules[i]->totalsize, SPC_RAM_FOR_MUSIC,
                       out->modules[i]->source_list_count);
            printf("\n");
        }
        if (out && bank->module_count > 1)
            printf("%d BRR sources in all (shared samples are stored once)\n", out->source_count);
    }
    if (out) spc_bank_destroy(out);
    itl_bank_destroy(bank);
    return CLI_OK;
}

/* ------------------------------------------------------------------ main */

static const cli_cmd cmds[] = {
    { "bank", "Impulse Tracker modules -> NAME.asm + NAME.h + NAME.bnk, the soundbank the SNESMOD driver plays",
      bank_opts, CLI_N(bank_opts), "bank music/theme.it music/jingle.it --name soundbank --save   (then: bank music/soundbank.toml)", 1, run_bank },
    { "spc", "one standalone .spc per module, for an SPC player", spc_opts, CLI_N(spc_opts),
      "spc music/theme.it --out build/", 1, run_spc },
    { "inspect", "what each module holds and costs in SPC RAM", NULL, 0, "inspect music/*.it --json", 1, run_inspect },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes-music", TOOL_VERSION,
        "Impulse Tracker (.it) -> SNESMOD soundbank for the SPC700", cmds, CLI_N(cmds),
    };
    smconv_set_reporter(music_report, NULL);
    return cli_main(&tool, argc, argv);
}
