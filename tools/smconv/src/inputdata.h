#ifndef INPUTDATA_H
#define INPUTDATA_H

#include <stdbool.h>
#include "basetypes.h"

#define SMCONV_MAX_FILES 128
#define SMCONV_MAX_PATH  1024

typedef struct {
    char output[SMCONV_MAX_PATH];
    bool hirom;
    const char *files[SMCONV_MAX_FILES];
    int file_count;
    bool spc_mode;
    bool verbose_mode;
    bool show_help;
    bool show_version;
    bool check_effect_size;
    int banknumber;
    bool no_header;
    char symbol_prefix[256];
} smconv_args_t;

void smconv_args_init(smconv_args_t *args);
void smconv_args_parse(smconv_args_t *args, int argc, char *argv[]);

#endif
