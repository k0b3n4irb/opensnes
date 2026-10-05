/*
 * cli.h — the shared command line of the opensnes-* tools.
 *
 * docs/tools/CONVENTIONS.md is the contract; this is its implementation,
 * so every tool of the family gets the same subcommands, long options,
 * --help, --version, --json, -q / -v, message shape, exit codes and
 * settings file without re-deciding any of it.
 *
 *   static const cli_opt encode_opts[] = {
 *       { "loop", 0, CLI_INT2, "START END", "loop between two sample indices", NULL },
 *       CLI_OPT_OUT, CLI_OPT_SAVE,
 *   };
 *   static const cli_cmd cmds[] = {
 *       { "encode", "WAV → BRR", encode_opts, CLI_N(encode_opts), "encode res/jump.wav", 1, run_encode },
 *       { "inspect", "what a .wav or .brr holds and costs", NULL, 0, "inspect res/jump.brr", 1, run_inspect },
 *   };
 *   int main(int argc, char **argv) {
 *       static const cli_tool tool = { "opensnes-sample", TOOL_VERSION, "WAV → BRR samples", cmds, CLI_N(cmds) };
 *       return cli_main(&tool, argc, argv);
 *   }
 *
 * A run function receives the parsed context: cli_has / cli_str / cli_int /
 * cli_int2 read an option (command line first, then the input's settings
 * file), ctx->args are the inputs. It returns an exit code: CLI_OK,
 * CLI_REFUSED (the input, with the limit named), CLI_IO. Usage errors
 * (CLI_USAGE) are the parser's.
 */
#ifndef OPENSNES_CLI_H
#define OPENSNES_CLI_H

#include <stdio.h>

enum { CLI_OK = 0, CLI_REFUSED = 1, CLI_USAGE = 2, CLI_IO = 3 };

/* How an option takes its value. */
typedef enum { CLI_FLAG, CLI_STR, CLI_INT, CLI_INT2 } cli_kind;

typedef struct {
    const char *name;        /* long name, without the dashes */
    char        shortname;   /* 0 for none; reserved for -o -q -v -h */
    cli_kind    kind;
    const char *value_name;  /* shown in --help, e.g. "DIR" or "START END" */
    const char *help;
    const char *deflt;       /* shown in --help; NULL = none */
} cli_opt;

#define CLI_N(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* The options every converting subcommand offers (same meaning everywhere). */
#define CLI_OPT_OUT  { "out",  'o', CLI_STR,  "DIR", "write the outputs in DIR", "beside the input" }
#define CLI_OPT_SAVE { "save", 0,   CLI_FLAG, NULL,  "record the effective settings beside the input (<input>.toml)", NULL }

struct cli_ctx;

typedef struct {
    const char    *name;
    const char    *summary;
    const cli_opt *opts;
    int            nopts;
    const char    *example;   /* one example, shown by --help */
    int            min_args;  /* inputs required (0 = none) */
    int          (*run)(struct cli_ctx *ctx);
} cli_cmd;

typedef struct {
    const char    *name;      /* "opensnes-sample" */
    const char    *version;
    const char    *summary;
    const cli_cmd *cmds;
    int            ncmds;
} cli_tool;

#define CLI_MAX_OPTS 24
#define CLI_MAX_ARGS 64

typedef struct cli_ctx {
    const cli_tool *tool;
    const cli_cmd  *cmd;
    int   json, quiet, verbose;
    int   nargs;
    const char *args[CLI_MAX_ARGS];
    /* option values as given on the command line (NULL = not given) */
    const char *given[CLI_MAX_OPTS];
    const char *given2[CLI_MAX_OPTS];      /* second value of a CLI_INT2 */
    /* the settings file of the input being processed (see cli_load_settings) */
    const char *set_key[CLI_MAX_OPTS];
    char        set_val[CLI_MAX_OPTS][256];
    int         nset;
    char        settings_path[1024];
    int         json_depth, json_first[8], json_is_array[8];
} cli_ctx;

/* Parse and dispatch. Prints help / version / usage errors itself. */
int cli_main(const cli_tool *tool, int argc, char **argv);

/* Reading options (command line, then the loaded settings file). */
int         cli_has (const cli_ctx *ctx, const char *name);
const char *cli_str (const cli_ctx *ctx, const char *name, const char *deflt);
int         cli_int (const cli_ctx *ctx, const char *name, int deflt);
int         cli_int2(const cli_ctx *ctx, const char *name, int *a, int *b); /* 1 if present */

/* Messages: "<tool>: <file>: <message>" on stderr (file may be NULL). */
void cli_error(const cli_ctx *ctx, const char *file, const char *fmt, ...);
void cli_warn (const cli_ctx *ctx, const char *file, const char *fmt, ...);
/* Progress: only with -v, never with --json. */
void cli_note (const cli_ctx *ctx, const char *fmt, ...);

/* Paths. */
void cli_stem(const char *path, char *out, size_t n);              /* "res/jump.wav" → "jump" */
void cli_ident(const char *path, char *out, size_t n);             /* "res/my-jump.wav" → "my_jump" */
int  cli_output_path(const cli_ctx *ctx, const char *input, const char *ext, char *out, size_t n);
int  cli_write_file(const cli_ctx *ctx, const char *path, const void *data, size_t len); /* CLI_OK / CLI_IO */
unsigned char *cli_read_file(const cli_ctx *ctx, const char *path, size_t *len);  /* NULL on error (said) */

/* Settings beside the input: <input>.toml, `tool = "<name>"` then one
 * [<subcommand>] table. Returns CLI_OK (loaded or absent) or CLI_REFUSED
 * (wrong tool, unknown key, bad syntax — said). cli_save_settings writes
 * the effective values of the subcommand's options. */
int cli_load_settings(cli_ctx *ctx, const char *input);
int cli_save_settings(const cli_ctx *ctx, const char *input);

/* --json: one object on stdout. Keys are written in call order. */
void cli_json_begin(cli_ctx *ctx);                 /* the top-level object, with "tool" and "version" */
void cli_json_object(cli_ctx *ctx, const char *key);   /* NULL key inside an array */
void cli_json_array (cli_ctx *ctx, const char *key);
void cli_json_close (cli_ctx *ctx);                /* closes the innermost object or array */
void cli_json_str (cli_ctx *ctx, const char *key, const char *val);
void cli_json_int (cli_ctx *ctx, const char *key, long val);
void cli_json_bool(cli_ctx *ctx, const char *key, int val);
void cli_json_end (cli_ctx *ctx);                  /* closes everything and prints the newline */

#endif
