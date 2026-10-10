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
 * Exit 1 when a check fails; warnings do not fail.
 *
 *   opensnes-rom inspect [--json] <game.sfc>...
 *
 * The cartridge header (title, mapping, chip, sizes, region, version), the
 * checksum stored against the one recomputed, and the CRC32 and SHA-1 of
 * the image. Exit 1 when there is no header or the checksum is wrong.
 *
 *   opensnes-rom budget [--json] <game.sfc>
 *
 * One report of what the game uses of what the console has: ROM bank by
 * bank, the C RAM band and the far band, the VRAM and CGRAM its assets
 * need. Never fails on a figure (the ratchets are `check`'s).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "sym.h"
#include "checks.h"
#include "header.h"

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

/* ---------------------------------------------------------------- inspect */

static int inspect_one(cli_ctx *ctx, const char *in)
{
    size_t len;
    unsigned char *rom = cli_read_file(ctx, in, &len);
    if (!rom) return CLI_IO;
    rom_header h;
    char why[160], sha[41];
    if (rom_header_read(rom, len, &h, why, sizeof why) != 0) {
        cli_error(ctx, in, "%s", why);
        free(rom);
        return CLI_REFUSED;
    }
    const unsigned char *img = rom + h.copier;
    size_t img_len = len - h.copier;
    unsigned crc = rom_crc32(img, img_len);
    rom_sha1(img, img_len, sha);
    int sum_ok = h.checksum == h.computed && ((h.checksum + h.complement) & 0xFFFF) == 0xFFFF;
    int size_ok = h.declared_bytes >= (long)img_len && h.declared_bytes < 2 * (long)img_len;

    if (ctx->json) {
        cli_json_object(ctx, NULL);
        cli_json_str(ctx, "input", in);
        cli_json_str(ctx, "title", h.title);
        cli_json_str(ctx, "mapping", h.mapping);
        cli_json_bool(ctx, "fastrom", h.fast);
        cli_json_str(ctx, "chip", h.chip ? h.chip : "");
        cli_json_int(ctx, "bytes", (long)img_len);
        cli_json_int(ctx, "declared_bytes", h.declared_bytes);
        cli_json_int(ctx, "ram_bytes", h.ram_bytes);
        cli_json_bool(ctx, "battery", h.has_battery);
        cli_json_int(ctx, "country", (long)h.country);
        cli_json_str(ctx, "region", h.region);
        cli_json_int(ctx, "version", (long)h.version);
        cli_json_int(ctx, "checksum", (long)h.checksum);
        cli_json_int(ctx, "checksum_computed", (long)h.computed);
        cli_json_bool(ctx, "checksum_ok", sum_ok);
        cli_json_bool(ctx, "size_ok", size_ok);
        cli_json_int(ctx, "copier_header", (long)h.copier);
        char hex[16];
        snprintf(hex, sizeof hex, "%08x", crc);
        cli_json_str(ctx, "crc32", hex);
        cli_json_str(ctx, "sha1", sha);
        cli_json_close(ctx);
    } else {
        printf("%s: \"%s\" — %s%s%s%s, %ld KB", in, h.title, h.mapping, h.fast ? " FastROM" : "",
               h.chip ? " + " : "", h.chip ? h.chip : "", (long)img_len / 1024);
        if (h.ram_bytes) printf(", %ld KB %s RAM", h.ram_bytes / 1024, h.has_battery ? "battery" : "volatile");
        printf(", %s (country $%02X), version 1.%u\n", h.region, h.country, h.version);
        printf("  checksum $%04X %s", h.checksum, sum_ok ? "ok" : "WRONG");
        if (!sum_ok) printf(" (the image sums to $%04X, complement $%04X)", h.computed, h.complement);
        printf(" · crc32 %08x · sha1 %s\n", crc, sha);
    }
    if (h.copier) cli_warn(ctx, in, "a 512-byte copier header precedes the image; flash carts and databases want it removed");
    if (!size_ok) cli_warn(ctx, in, "the header declares %ld KB and the image is %ld KB", h.declared_bytes / 1024, (long)img_len / 1024);
    if (!sum_ok) cli_error(ctx, in, "the checksum in the header does not match the image — relink it (wlalink writes the checksum), nothing may edit the ROM after the link");
    free(rom);
    return sum_ok ? CLI_OK : CLI_REFUSED;
}

static int run_inspect(cli_ctx *ctx)
{
    int rc = CLI_OK;
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "roms"); }
    for (int i = 0; i < ctx->nargs; i++) {
        int r = inspect_one(ctx, ctx->args[i]);
        if (r != CLI_OK && rc == CLI_OK) rc = r;
    }
    if (ctx->json) cli_json_end(ctx);
    return rc;
}

/* ----------------------------------------------------------------- budget */

static void bar_line(const char *label, long used, long cap, const char *unit, const char *note)
{
    long pct = cap ? (used * 100 + cap / 2) / cap : 0;
    printf("  %-26s %7ld / %-7ld %-7s %3ld%%   %ld free%s%s\n", label, used, cap, unit, pct, cap - used, note ? "   " : "", note ? note : "");
}

static int run_budget(cli_ctx *ctx)
{
    const char *in = ctx->args[0];
    char sym_path[1024], dir[1024], why[160];
    size_t l = strlen(in);
    if (l < 5 || strcmp(in + l - 4, ".sfc") != 0) {
        cli_error(ctx, in, "budget reads a built ROM: give it the .sfc (its .sym is read beside it)");
        return CLI_USAGE;
    }
    snprintf(sym_path, sizeof sym_path, "%.*s.sym", (int)(l - 4), in);
    const char *slash = strrchr(in, '/');
    if (slash) snprintf(dir, sizeof dir, "%.*s", (int)(slash - in), in); else snprintf(dir, sizeof dir, ".");

    size_t len;
    unsigned char *rom = cli_read_file(ctx, in, &len);
    if (!rom) return CLI_IO;
    rom_header h;
    if (rom_header_read(rom, len, &h, why, sizeof why) != 0) { cli_error(ctx, in, "%s", why); free(rom); return CLI_REFUSED; }
    long image = (long)(len - h.copier);
    free(rom);
    sym_file s;
    if (sym_read(sym_path, &s, why, sizeof why) != 0) {
        cli_error(ctx, sym_path, "%s — link the ROM first (wlalink writes the .sym beside it)", why);
        return CLI_IO;
    }

    /* the figures of `check`, without its lines */
    cli_ctx quiet = *ctx;
    quiet.quiet = 1;
    quiet.json = 0;
    check_results r;
    memset(&r, 0, sizeof r);
    r.bank0 = check_bank0(&quiet, &s, in, 0, 0, &r);
    r.ram = check_ram(&quiet, &s, in, 0, 0, &r);
    report_assets(&quiet, dir, &r);

    enum { MAXBANK = 256 };
    static long used[MAXBANK];
    memset(used, 0, sizeof used);
    int nbanks = (int)(image / h.bank_size);
    if (nbanks < 1) nbanks = 1;
    if (nbanks > MAXBANK) nbanks = MAXBANK;
    long rom_used = 0;
    for (size_t i = 0; i < s.nsections; i++) {
        int b = s.sections[i].bank;
        if (b < 0 || b >= MAXBANK) continue;
        used[b] += (long)s.sections[i].size;
        rom_used += (long)s.sections[i].size;
    }
    int far_used_any = r.far_sections > 0;
    long far_cap = 0x10000 - 0x2000, far_free = far_used_any ? r.far_free : far_cap;
    long ram_cap = 0x2000;

    if (ctx->json) {
        cli_json_begin(ctx);
        cli_json_str(ctx, "input", in);
        cli_json_str(ctx, "mapping", h.mapping);
        cli_json_object(ctx, "rom");
        cli_json_int(ctx, "bytes", image); cli_json_int(ctx, "used", rom_used); cli_json_int(ctx, "free", image - rom_used);
        cli_json_int(ctx, "bank_bytes", h.bank_size); cli_json_int(ctx, "bank0_code_free", r.bank0_free);
        cli_json_array(ctx, "banks");
        for (int b = 0; b < nbanks; b++) {
            cli_json_object(ctx, NULL);
            cli_json_int(ctx, "bank", b); cli_json_int(ctx, "used", used[b]); cli_json_int(ctx, "free", h.bank_size - used[b]);
            cli_json_close(ctx);
        }
        cli_json_close(ctx);
        cli_json_close(ctx);
        cli_json_object(ctx, "ram"); cli_json_int(ctx, "bytes", ram_cap); cli_json_int(ctx, "used", ram_cap - r.ram_free);
        cli_json_int(ctx, "free", r.ram_free); cli_json_close(ctx);
        cli_json_object(ctx, "far_ram"); cli_json_int(ctx, "bytes", far_cap); cli_json_int(ctx, "used", far_cap - far_free);
        cli_json_int(ctx, "free", far_free); cli_json_close(ctx);
        cli_json_object(ctx, "vram"); cli_json_int(ctx, "bytes", 0x10000); cli_json_int(ctx, "assets", r.asset_vram); cli_json_close(ctx);
        cli_json_object(ctx, "cgram"); cli_json_int(ctx, "colours", 256); cli_json_int(ctx, "assets", r.asset_colours); cli_json_close(ctx);
        cli_json_int(ctx, "save_ram_bytes", h.ram_bytes);
        cli_json_end(ctx);
    } else {
        printf("%s — \"%s\", %s%s%s, %ld KB in %d banks of %d KB\n", in, h.title, h.mapping, h.chip ? " + " : "", h.chip ? h.chip : "",
               image / 1024, nbanks, h.bank_size / 1024);
        printf("ROM\n");
        int empty = 0;
        for (int b = 0; b < nbanks; b++) {
            if (!used[b]) { empty++; continue; }
            char label[40];
            snprintf(label, sizeof label, "bank $%02X%s", b, b == 0 ? " (code)" : "");
            bar_line(label, used[b], h.bank_size, "bytes", NULL);
        }
        if (empty) printf("  %d empty bank%s\n", empty, empty == 1 ? "" : "s");
        bar_line("whole ROM", rom_used, image, "bytes", NULL);
        printf("RAM\n");
        bar_line("C variables $0000-$1FFF", ram_cap - r.ram_free, ram_cap, "bytes", NULL);
        bar_line("FAR $7E:2000-$FFFF", far_cap - far_free, far_cap, "bytes", NULL);
        if (h.ram_bytes) printf("  save RAM on the cartridge: %ld bytes%s\n", h.ram_bytes, h.has_battery ? " (battery)" : "");
        printf("Video (the assets of the project, if all were loaded at once)\n");
        bar_line("VRAM", r.asset_vram, 0x10000, "bytes", NULL);
        bar_line("CGRAM", r.asset_colours, 256, "colours", NULL);
    }
    sym_free(&s);
    return CLI_OK;
}

static const cli_cmd cmds[] = {
    { "check", "the post-link checks: bank $00 ROM, C RAM band, data-init sentinel, bank-blind reads, NMI / WRAM-port race, asset inventory",
      check_opts, CLI_N(check_opts), "check game.sfc --bank0-fail 1024 --ram-fail 512", 1, run_check },
    { "inspect", "the cartridge header, the checksum against the image, CRC32 and SHA-1 (exit 1 on a wrong checksum)",
      NULL, 0, "inspect game.sfc", 1, run_inspect },
    { "budget", "what the game uses of the console: ROM bank by bank, RAM, and the VRAM and CGRAM of its assets",
      NULL, 0, "budget game.sfc", 1, run_budget },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes-rom", TOOL_VERSION, "is the ROM good? the post-link checks, the cartridge header and checksum, the budgets", cmds, CLI_N(cmds),
    };
    return cli_main(&tool, argc, argv);
}
