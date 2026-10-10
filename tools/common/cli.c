/*
 * cli.c — the shared command line of the opensnes-* tools (see cli.h and
 * docs/tools/CONVENTIONS.md). C11, standard library only.
 */
#include "cli.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define cli_mkdir(p) _mkdir(p)
#else
#include <sys/stat.h>
#define cli_mkdir(p) mkdir(p, 0777)
#endif

/* ------------------------------------------------------------------ help */

static void help_tool(const cli_tool *t, FILE *o)
{
    fprintf(o, "%s %s — %s\n\n", t->name, t->version, t->summary);
    fprintf(o, "Usage: %s <subcommand> [options] <input>...\n", t->name);
    fprintf(o, "       %s <subcommand> --help\n\n", t->name);
    fprintf(o, "Subcommands:\n");
    int w = 0;
    for (int i = 0; i < t->ncmds; i++)
        if ((int)strlen(t->cmds[i].name) > w) w = (int)strlen(t->cmds[i].name);
    for (int i = 0; i < t->ncmds; i++)
        fprintf(o, "  %-*s  %s\n", w, t->cmds[i].name, t->cmds[i].summary);
    fprintf(o, "\nOptions of every subcommand:\n"
               "  --json         the result as one JSON object on stdout (for the build and other tools)\n"
               "  -q, --quiet    no summary line\n"
               "  -v, --verbose  say what is being done\n"
               "  -h, --help     this help; after a subcommand, that subcommand's\n"
               "  --version      print the version and exit\n\n"
               "Exit codes: 0 done; 1 the input was refused (the message names the limit);\n"
               "2 usage; 3 the file system. A refused input writes nothing.\n"
               "Settings beside the asset: <input>.toml (see --save in a subcommand's help).\n");
}

static void help_cmd(const cli_tool *t, const cli_cmd *c, FILE *o)
{
    fprintf(o, "%s %s — %s\n\n", t->name, c->name, c->summary);
    fprintf(o, "Usage: %s %s [options]%s\n\n", t->name, c->name, c->min_args ? " <input>..." : "");
    if (c->nopts) {
        fprintf(o, "Options:\n");
        for (int i = 0; i < c->nopts; i++) {
            const cli_opt *op = &c->opts[i];
            char left[96];
            if (op->shortname)
                snprintf(left, sizeof left, "-%c, --%s%s%s", op->shortname, op->name,
                         op->value_name ? " " : "", op->value_name ? op->value_name : "");
            else
                snprintf(left, sizeof left, "    --%s%s%s", op->name,
                         op->value_name ? " " : "", op->value_name ? op->value_name : "");
            fprintf(o, "  %-28s %s", left, op->help);
            if (op->deflt) fprintf(o, " (default: %s)", op->deflt);
            fprintf(o, "\n");
        }
        fprintf(o, "\n");
    }
    fprintf(o, "Also: --json, -q, -v, -h (see `%s --help`).\n", t->name);
    if (c->example) fprintf(o, "\nExample:\n  %s %s\n", t->name, c->example);
}

/* -------------------------------------------------------------- messages */

static void vmsg(const cli_ctx *ctx, const char *file, const char *level, const char *fmt, va_list ap)
{
    fprintf(stderr, "%s: ", ctx->tool->name);
    if (file) fprintf(stderr, "%s: ", file);
    if (level) fprintf(stderr, "%s: ", level);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

void cli_error(const cli_ctx *ctx, const char *file, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); vmsg(ctx, file, NULL, fmt, ap); va_end(ap);
}

void cli_warn(const cli_ctx *ctx, const char *file, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); vmsg(ctx, file, "warning", fmt, ap); va_end(ap);
}

void cli_note(const cli_ctx *ctx, const char *fmt, ...)
{
    if (!ctx->verbose || ctx->json) return;
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "%s: ", ctx->tool->name);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

/* --------------------------------------------------------------- options */

static int opt_index(const cli_cmd *c, const char *name)
{
    for (int i = 0; i < c->nopts; i++)
        if (strcmp(c->opts[i].name, name) == 0) return i;
    return -1;
}

static int opt_index_short(const cli_cmd *c, char s)
{
    for (int i = 0; i < c->nopts; i++)
        if (c->opts[i].shortname == s) return i;
    return -1;
}

/* The value of an option: the command line first, then the settings file. */
static const char *value_of(const cli_ctx *ctx, const char *name, const char **second)
{
    int i = opt_index(ctx->cmd, name);
    if (second) *second = NULL;
    if (i < 0) return NULL;
    if (ctx->given[i]) {
        if (second) *second = ctx->given2[i];
        return ctx->given[i];
    }
    for (int k = 0; k < ctx->nset; k++)
        if (strcmp(ctx->set_key[k], name) == 0) {
            if (second && ctx->cmd->opts[i].kind == CLI_INT2) {
                /* stored as "a b" */
                const char *sp = strchr(ctx->set_val[k], ' ');
                *second = sp ? sp + 1 : NULL;
            }
            return ctx->set_val[k];
        }
    return NULL;
}

/* A path-valued option (value_name FILE): as given on the command line; from the
 * settings file, relative to that file's directory (docs/tools/CONVENTIONS.md:
 * "paths relative to the file"), so `palette = "town.pal"` beside the picture
 * works from any working directory. */
const char *cli_path(const cli_ctx *ctx, const char *name, char *buf, size_t n)
{
    int i = opt_index(ctx->cmd, name);
    if (i < 0) return NULL;
    if (ctx->given[i]) return ctx->given[i];
    const char *v = value_of(ctx, name, NULL);
    if (!v || v[0] == '/' || !ctx->settings_path[0]) return v;
    const char *slash = strrchr(ctx->settings_path, '/');
    if (!slash) return v;
    snprintf(buf, n, "%.*s/%s", (int)(slash - ctx->settings_path), ctx->settings_path, v);
    return buf;
}

int cli_has(const cli_ctx *ctx, const char *name)
{
    return value_of(ctx, name, NULL) != NULL;
}

const char *cli_str(const cli_ctx *ctx, const char *name, const char *deflt)
{
    const char *v = value_of(ctx, name, NULL);
    return v ? v : deflt;
}

int cli_int(const cli_ctx *ctx, const char *name, int deflt)
{
    const char *v = value_of(ctx, name, NULL);
    return v ? atoi(v) : deflt;
}

int cli_list(const cli_ctx *ctx, const char *name, char *buf, size_t n, const char **words, int max)
{
    const char *v = value_of(ctx, name, NULL);
    if (!v) return 0;
    snprintf(buf, n, "%s", v);
    int count = 0;
    for (char *p = buf; *p && count < max;) {
        while (*p == ' ') p++;
        if (!*p) break;
        words[count++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    return count;
}

int cli_int2(const cli_ctx *ctx, const char *name, int *a, int *b)
{
    const char *second = NULL;
    const char *v = value_of(ctx, name, &second);
    if (!v || !second) return 0;
    *a = atoi(v);
    *b = atoi(second);
    return 1;
}

/* ----------------------------------------------------------------- paths */

static const char *basename_of(const char *path)
{
    const char *base = path;
    for (const char *p = path; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    return base;
}

void cli_stem(const char *path, char *out, size_t n)
{
    const char *base = basename_of(path);
    const char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    if (len >= n) len = n - 1;
    memcpy(out, base, len);
    out[len] = '\0';
}

void cli_ident(const char *path, char *out, size_t n)
{
    char stem[256];
    cli_stem(path, stem, sizeof stem);
    size_t j = 0;
    for (const char *p = stem; *p && j + 1 < n; p++) {
        unsigned char c = (unsigned char)*p;
        out[j++] = isalnum(c) ? (char)c : '_';
    }
    if (j == 0 || isdigit((unsigned char)out[0])) {
        /* an identifier cannot start with a digit: prefix it */
        if (j + 1 < n) { memmove(out + 1, out, j); out[0] = '_'; j++; }
    }
    out[j] = '\0';
}

int cli_output_path(const cli_ctx *ctx, const char *input, const char *ext, char *out, size_t n)
{
    const char *dir = cli_str(ctx, "out", NULL);
    char stem[256];
    cli_stem(input, stem, sizeof stem);
    int w;
    if (dir && *dir) {
        size_t dl = strlen(dir);
        int slash = dir[dl - 1] == '/' || dir[dl - 1] == '\\';
        /* --out names a directory the build may not have made yet: make it
         * (one level; a deeper path is the caller's to create). */
        cli_mkdir(dir);
        w = snprintf(out, n, "%s%s%s%s", dir, slash ? "" : "/", stem, ext);
    } else {
        size_t dl = (size_t)(basename_of(input) - input);
        w = snprintf(out, n, "%.*s%s%s", (int)dl, input, stem, ext);
    }
    if (w < 0 || (size_t)w >= n) {
        cli_error(ctx, input, "output path too long");
        return CLI_IO;
    }
    return CLI_OK;
}

int cli_write_file(const cli_ctx *ctx, const char *path, const void *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    if (!f) { cli_error(ctx, path, "cannot write — does the directory exist?"); return CLI_IO; }
    if (len && fwrite(data, 1, len, f) != len) {
        cli_error(ctx, path, "write error");
        fclose(f);
        return CLI_IO;
    }
    fclose(f);
    return CLI_OK;
}

unsigned char *cli_read_file(const cli_ctx *ctx, const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) { cli_error(ctx, path, "cannot open"); return NULL; }
    if (fseek(f, 0, SEEK_END) != 0) { cli_error(ctx, path, "cannot seek"); fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { cli_error(ctx, path, "cannot tell its size"); fclose(f); return NULL; }
    rewind(f);
    unsigned char *buf = malloc((size_t)sz + 1);
    if (!buf) { cli_error(ctx, path, "out of memory"); fclose(f); return NULL; }
    if (sz && fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        cli_error(ctx, path, "read error");
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    buf[sz] = 0;
    *len = (size_t)sz;
    return buf;
}

/* -------------------------------------------------------------- settings */

static void trim(char *s)
{
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
    char *b = s;
    while (*b && isspace((unsigned char)*b)) b++;
    if (b != s) memmove(s, b, strlen(b) + 1);
}

/* Strip a trailing comment that is not inside a string. */
static void strip_comment(char *s)
{
    int in_str = 0;
    for (char *p = s; *p; p++) {
        if (*p == '"') in_str = !in_str;
        else if (*p == '#' && !in_str) { *p = '\0'; return; }
    }
}

/* A TOML value as the option would have been typed: "x" → x, [1, 2] → "1 2",
 * true → "1", false → "0", 42 → 42. Returns 0 on a shape we do not read. */
static int toml_value(const char *v, char *out, size_t n)
{
    size_t len = strlen(v);
    if (len >= 2 && v[0] == '"' && v[len - 1] == '"') {
        if (len - 2 >= n) return 0;
        memcpy(out, v + 1, len - 2);
        out[len - 2] = '\0';
        return 1;
    }
    if (strcmp(v, "true") == 0) { snprintf(out, n, "1"); return 1; }
    if (strcmp(v, "false") == 0) { snprintf(out, n, "0"); return 1; }
    if (len >= 2 && v[0] == '[' && v[len - 1] == ']') {
        size_t j = 0;
        int in_str = 0;
        for (size_t i = 1; i + 1 < len; i++) {
            char c = v[i];
            if (c == '"') { in_str = !in_str; continue; }   /* ["a", "b"] → a b */
            if (!in_str && c == ',') c = ' ';
            if (!in_str && isspace((unsigned char)c) && (j == 0 || out[j - 1] == ' ')) continue;
            if (j + 1 >= n) return 0;
            out[j++] = c;
        }
        while (j && out[j - 1] == ' ') j--;
        out[j] = '\0';
        return 1;
    }
    const char *p = v;
    if (*p == '-' || *p == '+') p++;
    if (!*p) return 0;
    for (; *p; p++) if (!isdigit((unsigned char)*p) && *p != '_') return 0;
    if (len >= n) return 0;
    memcpy(out, v, len + 1);
    return 1;
}

int cli_load_settings(cli_ctx *ctx, const char *input)
{
    char path[1024];
    snprintf(path, sizeof path, "%s.toml", input);
    return cli_load_settings_path(ctx, path);
}

int cli_load_settings_path(cli_ctx *ctx, const char *path)
{
    ctx->nset = 0;
    snprintf(ctx->settings_path, sizeof ctx->settings_path, "%s", path);
    FILE *f = fopen(ctx->settings_path, "r");
    if (!f) return CLI_OK;                     /* no settings: nothing to do */
    cli_note(ctx, "settings: %s", ctx->settings_path);

    char line[512], table[512] = "";
    int lineno = 0, saw_tool = 0;
    while (fgets(line, sizeof line, f)) {
        lineno++;
        strip_comment(line);
        trim(line);
        if (!*line) continue;
        if (line[0] == '[') {
            char *e = strchr(line, ']');
            if (!e) goto syntax;
            *e = '\0';
            snprintf(table, sizeof table, "%s", line + 1);
            trim(table);
            if (strcmp(table, ctx->cmd->name) != 0) {
                cli_error(ctx, ctx->settings_path, "line %d: table [%s] is not [%s] — one table, named after the subcommand",
                          lineno, table, ctx->cmd->name);
                fclose(f);
                return CLI_REFUSED;
            }
            continue;
        }
        char *eq = strchr(line, '=');
        if (!eq) goto syntax;
        *eq = '\0';
        char key[512], val[256];
        snprintf(key, sizeof key, "%s", line);
        trim(key);
        char raw[512];
        snprintf(raw, sizeof raw, "%s", eq + 1);
        trim(raw);
        if (!toml_value(raw, val, sizeof val)) goto syntax;
        if (!*table) {
            if (strcmp(key, "tool") != 0) {
                cli_error(ctx, ctx->settings_path, "line %d: '%s' before any table — only `tool = \"%s\"` goes there",
                          lineno, key, ctx->tool->name);
                fclose(f);
                return CLI_REFUSED;
            }
            if (strcmp(val, ctx->tool->name) != 0) {
                cli_error(ctx, ctx->settings_path, "line %d: these settings are for %s, not %s",
                          lineno, val, ctx->tool->name);
                fclose(f);
                return CLI_REFUSED;
            }
            saw_tool = 1;
            continue;
        }
        int i = opt_index(ctx->cmd, key);
        if (i < 0 || strcmp(key, "out") == 0 || strcmp(key, "save") == 0) {
            cli_error(ctx, ctx->settings_path, "line %d: unknown key '%s' for %s — see `%s %s --help`",
                      lineno, key, ctx->cmd->name, ctx->tool->name, ctx->cmd->name);
            fclose(f);
            return CLI_REFUSED;
        }
        if (ctx->nset >= CLI_MAX_OPTS) goto syntax;
        ctx->set_key[ctx->nset] = ctx->cmd->opts[i].name;
        snprintf(ctx->set_val[ctx->nset], sizeof ctx->set_val[0], "%s", val);
        ctx->nset++;
    }
    fclose(f);
    if (!saw_tool) {
        cli_error(ctx, ctx->settings_path, "no `tool = \"%s\"` line — the build needs it to pick the tool", ctx->tool->name);
        return CLI_REFUSED;
    }
    return CLI_OK;
syntax:
    cli_error(ctx, ctx->settings_path, "line %d: cannot read this line — `key = value` with a string, a number, true/false or [a, b]", lineno);
    fclose(f);
    return CLI_REFUSED;
}

int cli_save_settings(const cli_ctx *ctx, const char *input)
{
    char path[1024];
    snprintf(path, sizeof path, "%s.toml", input);
    return cli_save_settings_path(ctx, path, basename_of(input));
}

int cli_save_settings_path(const cli_ctx *ctx, const char *path, const char *asset_name)
{
    FILE *f = fopen(path, "w");
    if (!f) { cli_error(ctx, path, "cannot write the settings"); return CLI_IO; }
    fprintf(f, "# import settings of %s, written by %s --save; the build reads `tool`\n",
            asset_name, ctx->tool->name);
    fprintf(f, "tool = \"%s\"\n\n[%s]\n", ctx->tool->name, ctx->cmd->name);
    for (int i = 0; i < ctx->cmd->nopts; i++) {
        const cli_opt *op = &ctx->cmd->opts[i];
        if (strcmp(op->name, "out") == 0 || strcmp(op->name, "save") == 0) continue;
        const char *second = NULL;
        const char *v = value_of(ctx, op->name, &second);
        if (!v) continue;
        switch (op->kind) {
        case CLI_FLAG: fprintf(f, "%s = true\n", op->name); break;
        case CLI_INT:  fprintf(f, "%s = %s\n", op->name, v); break;
        case CLI_INT2: fprintf(f, "%s = [%s, %s]\n", op->name, v, second ? second : "0"); break;
        case CLI_STR:
            if (op->value_name && strcmp(op->value_name, "FILE") == 0) {
                /* a path: written relative to the settings file's directory, the way
                 * cli_path reads it back (docs/tools/CONVENTIONS.md) */
                const char *slash = strrchr(path, '/');
                size_t dl = slash ? (size_t)(slash - path) + 1 : 0;
                if (dl && strncmp(v, path, dl) == 0) v += dl;
            }
            fprintf(f, "%s = \"%s\"\n", op->name, v);
            break;
        case CLI_LIST: {
            fprintf(f, "%s = [", op->name);
            int first = 1;
            for (const char *p = v; *p;) {
                while (*p == ' ') p++;
                if (!*p) break;
                const char *e = strchr(p, ' ');
                size_t wl = e ? (size_t)(e - p) : strlen(p);
                fprintf(f, "%s\"%.*s\"", first ? "" : ", ", (int)wl, p);
                first = 0;
                p += wl;
            }
            fprintf(f, "]\n");
            break;
        }
        }
    }
    fclose(f);
    cli_note(ctx, "wrote %s", path);
    return CLI_OK;
}

/* ------------------------------------------------------------------ json */

static void json_sep(cli_ctx *ctx)
{
    if (ctx->json_depth > 0) {
        if (!ctx->json_first[ctx->json_depth - 1]) fputs(", ", stdout);
        ctx->json_first[ctx->json_depth - 1] = 0;
    }
}

static void json_key(cli_ctx *ctx, const char *key)
{
    json_sep(ctx);
    if (key) printf("\"%s\": ", key);
}

static void json_quote(const char *s)
{
    putchar('"');
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { putchar('\\'); putchar(c); }
        else if (c < 0x20) printf("\\u%04x", c);
        else putchar(c);
    }
    putchar('"');
}

void cli_json_begin(cli_ctx *ctx)
{
    ctx->json_depth = 0;
    putchar('{');
    ctx->json_first[0] = 1;
    ctx->json_is_array[0] = 0;
    ctx->json_depth = 1;
    cli_json_str(ctx, "tool", ctx->tool->name);
    cli_json_str(ctx, "version", ctx->tool->version);
}

void cli_json_object(cli_ctx *ctx, const char *key)
{
    json_key(ctx, key);
    putchar('{');
    if (ctx->json_depth < 8) { ctx->json_is_array[ctx->json_depth] = 0; ctx->json_first[ctx->json_depth++] = 1; }
}

void cli_json_array(cli_ctx *ctx, const char *key)
{
    json_key(ctx, key);
    putchar('[');
    if (ctx->json_depth < 8) { ctx->json_is_array[ctx->json_depth] = 1; ctx->json_first[ctx->json_depth++] = 1; }
}

void cli_json_close(cli_ctx *ctx)
{
    if (ctx->json_depth <= 1) return;
    ctx->json_depth--;
    putchar(ctx->json_is_array[ctx->json_depth] ? ']' : '}');
}

void cli_json_str(cli_ctx *ctx, const char *key, const char *val)
{
    json_key(ctx, key);
    json_quote(val ? val : "");
}

void cli_json_int(cli_ctx *ctx, const char *key, long val)
{
    json_key(ctx, key);
    printf("%ld", val);
}

void cli_json_bool(cli_ctx *ctx, const char *key, int val)
{
    json_key(ctx, key);
    fputs(val ? "true" : "false", stdout);
}

void cli_json_end(cli_ctx *ctx)
{
    while (ctx->json_depth > 1) cli_json_close(ctx);
    puts("}");
    ctx->json_depth = 0;
}

/* ----------------------------------------------------------------- parse */

static int usage_error(const cli_ctx *ctx, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "%s: ", ctx->tool->name);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, " — see `%s %s%s--help`\n", ctx->tool->name,
            ctx->cmd ? ctx->cmd->name : "", ctx->cmd ? " " : "");
    return CLI_USAGE;
}

int cli_main(const cli_tool *tool, int argc, char **argv)
{
    static cli_ctx ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.tool = tool;

    if (argc < 2) { help_tool(tool, stderr); return CLI_USAGE; }
    const char *first = argv[1];
    if (!strcmp(first, "-h") || !strcmp(first, "--help")) { help_tool(tool, stdout); return CLI_OK; }
    if (!strcmp(first, "--version")) { printf("%s %s\n", tool->name, tool->version); return CLI_OK; }
    for (int i = 0; i < tool->ncmds; i++)
        if (strcmp(tool->cmds[i].name, first) == 0) ctx.cmd = &tool->cmds[i];
    if (!ctx.cmd) {
        if (first[0] == '-') return usage_error(&ctx, "the subcommand comes first ('%s' is an option)", first);
        return usage_error(&ctx, "unknown subcommand '%s'", first);
    }
    if (ctx.cmd->nopts > CLI_MAX_OPTS) { fprintf(stderr, "%s: too many options declared\n", tool->name); return CLI_USAGE; }

    int only_args = 0;
    for (int i = 2; i < argc; i++) {
        const char *a = argv[i];
        if (only_args || a[0] != '-' || !a[1]) {
            if (ctx.nargs >= CLI_MAX_ARGS) return usage_error(&ctx, "too many inputs (%d at most)", CLI_MAX_ARGS);
            ctx.args[ctx.nargs++] = a;
            continue;
        }
        if (!strcmp(a, "--")) { only_args = 1; continue; }
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) { help_cmd(tool, ctx.cmd, stdout); return CLI_OK; }
        if (!strcmp(a, "--version")) { printf("%s %s\n", tool->name, tool->version); return CLI_OK; }
        if (!strcmp(a, "--json")) { ctx.json = 1; continue; }
        if (!strcmp(a, "-q") || !strcmp(a, "--quiet")) { ctx.quiet = 1; continue; }
        if (!strcmp(a, "-v") || !strcmp(a, "--verbose")) { ctx.verbose = 1; continue; }
        int idx;
        if (a[1] == '-') idx = opt_index(ctx.cmd, a + 2);
        else if (!a[2]) idx = opt_index_short(ctx.cmd, a[1]);
        else idx = -1;
        if (idx < 0) return usage_error(&ctx, "unknown option '%s' for %s", a, ctx.cmd->name);
        const cli_opt *op = &ctx.cmd->opts[idx];
        switch (op->kind) {
        case CLI_FLAG:
            ctx.given[idx] = "1";
            break;
        case CLI_STR: case CLI_INT: case CLI_LIST:
            if (i + 1 >= argc) return usage_error(&ctx, "--%s needs %s", op->name, op->value_name ? op->value_name : "a value");
            ctx.given[idx] = argv[++i];
            if (op->kind == CLI_INT) {
                char *end; strtol(ctx.given[idx], &end, 10);
                if (*end) return usage_error(&ctx, "--%s needs a number, not '%s'", op->name, ctx.given[idx]);
            }
            break;
        case CLI_INT2:
            if (i + 2 >= argc) return usage_error(&ctx, "--%s needs %s", op->name, op->value_name ? op->value_name : "two values");
            ctx.given[idx] = argv[++i];
            ctx.given2[idx] = argv[++i];
            for (int k = 0; k < 2; k++) {
                const char *v = k ? ctx.given2[idx] : ctx.given[idx];
                char *end; strtol(v, &end, 10);
                if (*end) return usage_error(&ctx, "--%s needs two numbers, not '%s'", op->name, v);
            }
            break;
        }
    }
    if (ctx.nargs < ctx.cmd->min_args)
        return usage_error(&ctx, "%s needs %s", ctx.cmd->name, ctx.cmd->min_args == 1 ? "an input" : "inputs");
    return ctx.cmd->run(&ctx);
}
