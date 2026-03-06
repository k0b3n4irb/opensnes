#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "inputdata.h"

#define ERRORRED(STRING) "\x1B[31m" STRING "\033[0m"
#define ERRORBRIGHT(STRING) "\x1B[97m" STRING "\033[0m"

enum {
    ARG_OPTION,
    ARG_INPUT,
    ARG_INVALID
};

static int get_arg_type(const char *str)
{
    if (str[0]) {
        if (str[0] == '-')
            return ARG_OPTION;
        else
            return ARG_INPUT;
    }
    return ARG_INVALID;
}

static bool strmatch(const char *source, const char *test)
{
    for (int i = 0; test[i]; i++)
        if (source[i] != test[i])
            return false;
    return true;
}

void smconv_args_init(smconv_args_t *args)
{
    memset(args, 0, sizeof(*args));
    args->spc_mode = true;
    args->banknumber = 5;
    strcpy(args->symbol_prefix, "SOUNDBANK__");
}

void smconv_args_parse(smconv_args_t *args, int argc, char *argv[])
{
    smconv_args_init(args);

    for (int arg = 1; arg < argc;) {
        int ptype = get_arg_type(argv[arg]);
        switch (ptype) {
        case ARG_INVALID:
            arg++;
            break;
        case ARG_OPTION:

#define TESTARG2(str1, str2) (strmatch(argv[arg], str1) || strmatch(argv[arg], str2))

            if (TESTARG2("--soundbank", "-s")) {
                args->spc_mode = false;
            } else if (TESTARG2("--output", "-o")) {
                arg++;
                if (arg == argc) {
                    printf("%s: " ERRORRED("fatal error") ": No output file specified\n", ERRORBRIGHT("smconv"));
                    return;
                }
                strncpy(args->output, argv[arg], SMCONV_MAX_PATH - 1);
                args->output[SMCONV_MAX_PATH - 1] = '\0';
            } else if (TESTARG2("--hirom", "-i")) {
                args->hirom = true;
            } else if (TESTARG2("--verbose", "-V")) {
                args->verbose_mode = true;
            } else if (TESTARG2("--version", "-v")) {
                args->show_version = true;
            } else if (TESTARG2("--help", "-h")) {
                args->show_help = true;
            } else if (TESTARG2("--effectsize", "-f")) {
                args->check_effect_size = true;
            } else if (TESTARG2("--no-header", "-n")) {
                args->no_header = true;
            } else if (TESTARG2("--prefix", "-p")) {
                arg++;
                if (arg == argc) {
                    printf("%s: " ERRORRED("fatal error") ": No symbol prefix specified\n", ERRORBRIGHT("smconv"));
                    return;
                }
                strncpy(args->symbol_prefix, argv[arg], sizeof(args->symbol_prefix) - 1);
                args->symbol_prefix[sizeof(args->symbol_prefix) - 1] = '\0';
            } else if (strmatch(argv[arg], "-b")) {
                arg++;
                if (arg == argc) {
                    printf("%s: " ERRORRED("fatal error") ": No bank number specified\n", ERRORBRIGHT("smconv"));
                    return;
                }
                if (isdigit(argv[arg][0])) {
                    args->banknumber = atoi(argv[arg]);
                } else {
                    printf("%s: " ERRORRED("fatal error") ": Incorrect bank number\n", ERRORBRIGHT("smconv"));
                    return;
                }
            }
            arg++;
            break;
        case ARG_INPUT: {
            FILE *file = fopen(argv[arg], "r");
            if (file) {
                fclose(file);
            } else {
                return;
            }
            if (args->file_count < SMCONV_MAX_FILES)
                args->files[args->file_count++] = argv[arg];
            arg++;
            break;
        }
        }
    }

    if (args->spc_mode) {
        if (args->output[0] == '\0') {
            if (args->file_count > 0) {
                strncpy(args->output, args->files[0], SMCONV_MAX_PATH - 1);
                args->output[SMCONV_MAX_PATH - 1] = '\0';
                /* Replace extension with .spc */
                char *dot = strrchr(args->output, '.');
                if (dot)
                    *dot = '\0';
                strncat(args->output, ".spc",
                        SMCONV_MAX_PATH - 1 - strlen(args->output));
            }
        }
    }
}
