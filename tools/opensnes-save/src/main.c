/*
 * opensnes-save — battery save files (.srm), the raw image of a cartridge's
 * save RAM that emulators and flash carts read and write.
 *
 *   opensnes-save new     [--fill N] [--out DIR] [--json] <game.sfc>...
 *   opensnes-save inspect [--rom FILE] [--json] <file.srm>...
 *   opensnes-save get     --at OFFSET [--count N] [--to FILE] [--json] <file.srm>
 *   opensnes-save set     --at OFFSET (--hex "34 12 ..." | --from FILE) [--json] <file.srm>
 *   opensnes-save diff    [--json] <a.srm> <b.srm>
 *
 * `new` makes the blank save a ROM expects: its size is read from the ROM
 * header ($FFD8, 1 KB << n; for a Super FX cartridge the expansion RAM of
 * $FFBD, which the battery keeps). `get` / `set` read and patch bytes, so
 * a test can start from a prepared save (`srm_in` of a luna manifest) and
 * `inspect` / `diff` say what a run wrote (`srm_out`). The file has no
 * structure of its own: offsets are the ones the game passes to
 * sramSaveOffset() / sramLoadOffset().
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"

/* ---------------------------------------------------------- the ROM header */

typedef struct {
    long header;            /* file offset of $FFC0 */
    char title[22];
    int map_mode, cart_type, sram_code, exp_code;
    long sram_bytes, exp_bytes, save_bytes;   /* save_bytes: what the .srm holds */
    const char *mapping;
} rom_info;

static long size_of_code(int code) { return (code >= 1 && code <= 7) ? 1024L << code : 0; }

/* Find the header: LoROM at $7FC0, HiROM at $FFC0 (after a 512-byte copier
 * header if the file has one). The candidate whose map-mode nibble agrees
 * with its place and whose checksum and complement add up wins. */
static int read_rom(cli_ctx *ctx, const char *path, rom_info *r)
{
    size_t len = 0;
    unsigned char *rom = cli_read_file(ctx, path, &len);
    if (!rom) return CLI_IO;
    long skip = (len % 1024 == 512) ? 512 : 0;
    static const long at[2] = { 0x7FC0, 0xFFC0 };
    int best = -1, best_score = 0;
    for (int i = 0; i < 2; i++) {
        long h = skip + at[i];
        if ((size_t)h + 0x40 > len) continue;
        const unsigned char *p = rom + h;
        int score = 0, mode = p[0x15] & 0x0F;
        unsigned chk = p[0x1E] | (p[0x1F] << 8), cmp = p[0x1C] | (p[0x1D] << 8);
        if ((chk ^ cmp) == 0xFFFF) score += 4;
        if ((p[0x15] & 0xE0) == 0x20) score += 2;
        if (i == 0 ? (mode == 0 || mode == 3) : (mode == 1 || mode == 5)) score += 2;
        int printable = 1;
        for (int k = 0; k < 21; k++) if (p[k] < 0x20 || p[k] > 0x7E) printable = 0;
        score += printable;
        if (score > best_score) { best_score = score; best = i; }
    }
    if (best < 0 || best_score < 4) {
        cli_error(ctx, path, "no SNES header found at $7FC0 or $FFC0 — is this a ROM?");
        free(rom);
        return CLI_REFUSED;
    }
    const unsigned char *p = rom + skip + at[best];
    r->header = skip + at[best];
    memcpy(r->title, p, 21);
    r->title[21] = '\0';
    for (int k = 20; k >= 0 && r->title[k] == ' '; k--) r->title[k] = '\0';
    r->map_mode = p[0x15];
    r->cart_type = p[0x16];
    r->sram_code = p[0x18];
    r->exp_code = (skip + at[best] >= 3) ? p[-3] : 0;      /* $FFBD, in the expanded header */
    r->sram_bytes = size_of_code(r->sram_code);
    r->exp_bytes = size_of_code(r->exp_code);
    r->save_bytes = r->sram_bytes ? r->sram_bytes : r->exp_bytes;
    int mode = r->map_mode & 0x0F;
    r->mapping = mode == 3 ? "SA-1" : mode == 1 ? "HiROM" : mode == 5 ? "ExHiROM" : "LoROM";
    free(rom);
    return CLI_OK;
}

static void json_rom(cli_ctx *ctx, const rom_info *r)
{
    cli_json_str(ctx, "title", r->title); cli_json_str(ctx, "mapping", r->mapping);
    cli_json_int(ctx, "map_mode", r->map_mode); cli_json_int(ctx, "cartridge_type", r->cart_type);
    cli_json_int(ctx, "sram_bytes", r->sram_bytes); cli_json_int(ctx, "expansion_ram_bytes", r->exp_bytes);
    cli_json_int(ctx, "save_bytes", r->save_bytes);
}

/* ------------------------------------------------------------------- new */

static const cli_opt new_opts[] = {
    { "fill", 0, CLI_INT, "N", "the byte a blank save holds (0..255)", "0" },
    CLI_OPT_OUT,
};

static int run_new(cli_ctx *ctx)
{
    int worst = CLI_OK, fill = cli_int(ctx, "fill", 0);
    if (fill < 0 || fill > 255) { cli_error(ctx, NULL, "--fill %d: 0 to 255", fill); return CLI_REFUSED; }
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "saves"); }
    for (int i = 0; i < ctx->nargs; i++) {
        const char *in = ctx->args[i];
        rom_info r;
        int rc = read_rom(ctx, in, &r);
        if (rc == CLI_OK && r.save_bytes == 0) {
            cli_error(ctx, in, "the header declares no save RAM ($FFD8 = $%02X, $FFBD = $%02X) — set USE_SRAM := 1 in the Makefile", r.sram_code, r.exp_code);
            rc = CLI_REFUSED;
        }
        char out[1100];
        if (rc == CLI_OK && cli_output_path(ctx, in, ".srm", out, sizeof out) != CLI_OK) rc = CLI_IO;
        if (rc == CLI_OK) {
            unsigned char *buf = malloc((size_t)r.save_bytes);
            if (!buf) { cli_error(ctx, in, "out of memory"); rc = CLI_IO; }
            else { memset(buf, fill, (size_t)r.save_bytes); rc = cli_write_file(ctx, out, buf, (size_t)r.save_bytes); free(buf); }
        }
        if (rc != CLI_OK) {
            if (ctx->json) { cli_json_object(ctx, NULL); cli_json_str(ctx, "input", in); cli_json_int(ctx, "error", rc); cli_json_close(ctx); }
            if (rc > worst) worst = rc;
            continue;
        }
        if (ctx->json) {
            cli_json_object(ctx, NULL); cli_json_str(ctx, "input", in); json_rom(ctx, &r);
            cli_json_str(ctx, "output", out); cli_json_int(ctx, "fill", fill); cli_json_close(ctx);
        } else if (!ctx->quiet) {
            printf("%s: %s (%s, \"%s\") -> %s, %ld bytes of $%02X\n", ctx->tool->name, in, r.mapping, r.title, out, r.save_bytes, fill);
        }
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

/* --------------------------------------------------------------- inspect */

static const cli_opt inspect_opts[] = {
    { "rom", 0, CLI_STR, "FILE", "the ROM this save belongs to: its declared save RAM is compared with the file's size", NULL },
};

static int power_of_two(size_t n) { return n && !(n & (n - 1)); }

static int run_inspect(cli_ctx *ctx)
{
    int worst = CLI_OK;
    rom_info r;
    int have_rom = 0;
    if (cli_has(ctx, "rom")) {
        int rc = read_rom(ctx, cli_str(ctx, "rom", ""), &r);
        if (rc != CLI_OK) return rc;
        have_rom = 1;
    }
    if (ctx->json) { cli_json_begin(ctx); if (have_rom) { cli_json_object(ctx, "rom"); json_rom(ctx, &r); cli_json_close(ctx); } cli_json_array(ctx, "saves"); }
    for (int i = 0; i < ctx->nargs; i++) {
        const char *in = ctx->args[i];
        size_t len = 0;
        unsigned char *buf = cli_read_file(ctx, in, &len);
        if (!buf) { worst = CLI_IO; continue; }
        /* blank = the byte most of the file holds; the rest is what the game wrote */
        size_t hist[256] = { 0 };
        for (size_t k = 0; k < len; k++) hist[buf[k]]++;
        int blank = 0;
        for (int v = 1; v < 256; v++) if (hist[v] > hist[blank]) blank = v;
        long first = -1, last = -1;
        size_t used = 0;
        for (size_t k = 0; k < len; k++) if (buf[k] != blank) { if (first < 0) first = (long)k; last = (long)k; used++; }
        int fits = have_rom ? (long)len == r.save_bytes : 1, rc = CLI_OK;
        if (len == 0) { cli_error(ctx, in, "empty file"); rc = CLI_REFUSED; }
        else if (have_rom && !fits) {
            cli_error(ctx, in, "%zu bytes, but \"%s\" declares %ld bytes of save RAM — a save of another build or another game", len, r.title, r.save_bytes);
            rc = CLI_REFUSED;
        } else if (!power_of_two(len) || len < 2048)
            cli_warn(ctx, in, "%zu bytes is not a save RAM size (2 KB, 4 KB, ... 128 KB)", len);
        if (rc > worst) worst = rc;
        if (ctx->json) {
            cli_json_object(ctx, NULL); cli_json_str(ctx, "input", in); cli_json_int(ctx, "bytes", (long)len);
            cli_json_int(ctx, "blank", blank); cli_json_int(ctx, "written_bytes", (long)used);
            cli_json_int(ctx, "first_written", first); cli_json_int(ctx, "last_written", last);
            if (have_rom) cli_json_bool(ctx, "matches_rom", fits);
            cli_json_close(ctx);
        } else if (len) {
            if (used) printf("%s: %zu bytes, %zu written (offsets %ld to %ld), the rest $%02X\n", in, len, used, first, last, blank);
            else printf("%s: %zu bytes, blank ($%02X)\n", in, len, blank);
        }
        free(buf);
    }
    if (ctx->json) cli_json_end(ctx);
    return worst;
}

/* ------------------------------------------------------------- get / set */

static const cli_opt get_opts[] = {
    { "at", 0, CLI_INT, "OFFSET", "the first byte to read (the offset the game gives sramLoadOffset)", "0" },
    { "count", 0, CLI_INT, "N", "how many bytes", "16" },
    { "to", 0, CLI_STR, "FILE", "write the bytes to FILE instead of printing them in hex", NULL },
};

static int run_get(cli_ctx *ctx)
{
    const char *in = ctx->args[0];
    if (ctx->nargs != 1) { cli_error(ctx, NULL, "get reads one save"); return CLI_USAGE; }
    size_t len = 0;
    unsigned char *buf = cli_read_file(ctx, in, &len);
    if (!buf) return CLI_IO;
    int at = cli_int(ctx, "at", 0), count = cli_int(ctx, "count", 16);
    if (at < 0 || count < 1 || (size_t)at + (size_t)count > len) {
        cli_error(ctx, in, "--at %d --count %d is outside the file (%zu bytes)", at, count, len);
        free(buf); return CLI_REFUSED;
    }
    int rc = CLI_OK;
    if (cli_has(ctx, "to")) {
        rc = cli_write_file(ctx, cli_str(ctx, "to", ""), buf + at, (size_t)count);
    }
    if (ctx->json) {
        char *hex = malloc((size_t)count * 2 + 1);
        if (hex) for (int k = 0; k < count; k++) sprintf(hex + k * 2, "%02X", buf[at + k]);
        cli_json_begin(ctx); cli_json_str(ctx, "input", in); cli_json_int(ctx, "at", at); cli_json_int(ctx, "count", count);
        cli_json_str(ctx, "hex", hex ? hex : ""); cli_json_end(ctx);
        free(hex);
    } else if (!cli_has(ctx, "to")) {
        for (int k = 0; k < count; k++) printf("%02X%s", buf[at + k], (k % 16 == 15 || k == count - 1) ? "\n" : " ");
    }
    free(buf);
    return rc;
}

static const cli_opt set_opts[] = {
    { "at", 0, CLI_INT, "OFFSET", "the first byte to write (the offset the game gives sramSaveOffset)", "0" },
    { "hex", 0, CLI_STR, "BYTES", "the bytes, in hex: \"34 12 78 56\" or \"34127856\"", NULL },
    { "from", 0, CLI_STR, "FILE", "the bytes of FILE", NULL },
};

static int run_set(cli_ctx *ctx)
{
    const char *in = ctx->args[0];
    if (ctx->nargs != 1) { cli_error(ctx, NULL, "set patches one save"); return CLI_USAGE; }
    if (cli_has(ctx, "hex") == cli_has(ctx, "from")) { cli_error(ctx, in, "give the bytes with --hex or with --from, one of the two"); return CLI_USAGE; }
    size_t len = 0, n = 0;
    unsigned char *buf = cli_read_file(ctx, in, &len), *data = NULL;
    if (!buf) return CLI_IO;
    if (cli_has(ctx, "from")) {
        data = cli_read_file(ctx, cli_str(ctx, "from", ""), &n);
        if (!data) { free(buf); return CLI_IO; }
    } else {
        const char *h = cli_str(ctx, "hex", "");
        data = malloc(strlen(h) / 2 + 1);
        int hi = -1;
        for (const char *p = h; data && *p; p++) {
            if (isspace((unsigned char)*p) || *p == ',') continue;
            if (!isxdigit((unsigned char)*p)) { cli_error(ctx, in, "--hex: '%c' is not a hex digit", *p); free(buf); free(data); return CLI_REFUSED; }
            int v = isdigit((unsigned char)*p) ? *p - '0' : (tolower((unsigned char)*p) - 'a' + 10);
            if (hi < 0) hi = v; else { data[n++] = (unsigned char)(hi << 4 | v); hi = -1; }
        }
        if (!data || hi >= 0 || n == 0) { cli_error(ctx, in, "--hex needs whole bytes, two digits each"); free(buf); free(data); return CLI_REFUSED; }
    }
    int at = cli_int(ctx, "at", 0);
    if (at < 0 || (size_t)at + n > len) {
        cli_error(ctx, in, "%zu bytes at offset %d do not fit the file (%zu bytes) — nothing written", n, at, len);
        free(buf); free(data); return CLI_REFUSED;
    }
    memcpy(buf + at, data, n);
    int rc = cli_write_file(ctx, in, buf, len);
    if (rc == CLI_OK) {
        if (ctx->json) { cli_json_begin(ctx); cli_json_str(ctx, "input", in); cli_json_int(ctx, "at", at); cli_json_int(ctx, "count", (long)n); cli_json_end(ctx); }
        else if (!ctx->quiet) printf("%s: %s, %zu bytes written at offset %d\n", ctx->tool->name, in, n, at);
    }
    free(buf); free(data);
    return rc;
}

/* ------------------------------------------------------------------ diff */

static int run_diff(cli_ctx *ctx)
{
    if (ctx->nargs != 2) { cli_error(ctx, NULL, "diff compares two saves"); return CLI_USAGE; }
    size_t la = 0, lb = 0;
    unsigned char *a = cli_read_file(ctx, ctx->args[0], &la);
    if (!a) return CLI_IO;
    unsigned char *b = cli_read_file(ctx, ctx->args[1], &lb);
    if (!b) { free(a); return CLI_IO; }
    size_t n = la < lb ? la : lb, differing = 0;
    int ranges = 0;
    if (ctx->json) { cli_json_begin(ctx); cli_json_int(ctx, "a_bytes", (long)la); cli_json_int(ctx, "b_bytes", (long)lb); cli_json_array(ctx, "ranges"); }
    for (size_t k = 0; k < n;) {
        if (a[k] == b[k]) { k++; continue; }
        size_t e = k;
        while (e < n && a[e] != b[e]) e++;
        differing += e - k; ranges++;
        if (ctx->json) { cli_json_object(ctx, NULL); cli_json_int(ctx, "at", (long)k); cli_json_int(ctx, "count", (long)(e - k)); cli_json_close(ctx); }
        else {
            printf("offset %zu, %zu byte%s:", k, e - k, e - k == 1 ? "" : "s");
            size_t show = e - k > 16 ? 16 : e - k;
            for (size_t j = 0; j < show; j++) printf(" %02X", a[k + j]);
            printf("%s ->", e - k > show ? " ..." : "");
            for (size_t j = 0; j < show; j++) printf(" %02X", b[k + j]);
            printf("%s\n", e - k > show ? " ..." : "");
        }
        k = e;
    }
    if (ctx->json) { cli_json_close(ctx); cli_json_int(ctx, "differing_bytes", (long)differing); cli_json_bool(ctx, "same_size", la == lb); cli_json_end(ctx); }
    else {
        if (la != lb) printf("sizes differ: %zu and %zu bytes\n", la, lb);
        if (!differing && la == lb) printf("identical (%zu bytes)\n", la);
        else printf("%zu byte%s differ in %d range%s\n", differing, differing == 1 ? "" : "s", ranges, ranges == 1 ? "" : "s");
    }
    free(a); free(b);
    return (differing || la != lb) ? CLI_REFUSED : CLI_OK;
}

static const cli_cmd cmds[] = {
    { "new", "the blank .srm a ROM expects: the size its header declares, filled with --fill", new_opts, CLI_N(new_opts), "new game.sfc", 1, run_new },
    { "inspect", "what a .srm holds: size, how much is written and where; with --rom, whether it is that ROM's size", inspect_opts, CLI_N(inspect_opts),
      "inspect game.srm --rom game.sfc", 1, run_inspect },
    { "get", "bytes of a .srm, in hex or to a file", get_opts, CLI_N(get_opts), "get game.srm --at 0 --count 8", 1, run_get },
    { "set", "patch bytes into a .srm (a prepared save for a test)", set_opts, CLI_N(set_opts), "set game.srm --at 0 --hex \"09 00 0A 00\"", 1, run_set },
    { "diff", "the byte ranges in which two .srm differ (exit 1 when they do)", NULL, 0, "diff before.srm after.srm", 2, run_diff },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes-save", TOOL_VERSION, "battery save files (.srm): create, read, patch, compare", cmds, CLI_N(cmds),
    };
    return cli_main(&tool, argc, argv);
}
