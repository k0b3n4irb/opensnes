/******************************************************************************
 * snesmod converter (pure C rewrite)
 * (C) 2009 Mukunda Johnson
 * Modifications by Alekmaul, 2012-2016
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inputdata.h"
#include "itloader.h"
#include "it2spc.h"

#define ERRORRED(STRING) "\x1B[31m" STRING "\033[0m"
#define ERRORBRIGHT(STRING) "\x1B[97m" STRING "\033[0m"

#define SMCONVVERSION __BUILD_VERSION
#define SMCONVDATE __BUILD_DATE

static const char USAGE[] =
    "\nUsage : smconv [options] [input]...\n"
    "\n\nSoundbank options:"
    "\n-s                Soundbank creation mode"
    "\n                  (Can specify multiple files with soundbank mode.)"
    "\n                  (Otherwise specify only one file for SPC creation.)"
    "\n                  (Default is SPC creation mode)"
    "\n-b [num]          Bank number specification (Default is 5)"
    "\n-n, --no-header   Skip .include \"hdr.asm\" in output"
    "\n-p, --prefix [name] Set symbol prefix (Default is SOUNDBANK__)"
    "\n\nFile options:"
    "\n-o [file]         Specify output file or file base"
    "\n                  (Specify SPC file for -s option)"
    "\n                  (Specify filename base for soundbank creation)"
    "\n                  (Required for soundbank mode)\n"
    "\n\nMemory options:"
    "\n-i                Use HIROM mapping mode for soundbank."
    "\n-f                Check size of IT files with 1st IT file (useful for effects\n"
    "\n\nMisc options"
    "\n-V                Enable verbose output"
    "\n-h                Show help"
    "\n-v                Show version"
    "\n\nTips:"
    "\nTypical options to create soundbank for project:"
    "\n  smconv -s -o build/soundbank input1.it input2.it"
    "\n\nFor OpenSNES projects (no header, custom prefix):"
    "\n  smconv -s -o soundbank -b 1 -n -p soundbank input.it"
    "\n\nAnd for IT->SPC:"
    "\n  smconv input.it"
    "\n\nUse -V to view how much RAM the modules will use.\n";

static const char VERSION[] =
    "smconv (" SMCONVDATE ") version " SMCONVVERSION ""
    "\nCopyright (c) 2012-2024 Alekmaul "
    "\nBased on SNESMOD (C) 2009 Mukunda Johnson (www.mukunda.com)\n";

int main(int argc, char *argv[])
{
    smconv_args_t od;
    smconv_args_parse(&od, argc, argv);

    spc_bank_set_verbose(od.verbose_mode);

    if (od.show_help) {
        printf("%s", USAGE);
        exit(0);
    }

    if (od.show_version) {
        printf("%s", VERSION);
        exit(0);
    }

    if (argc < 2) {
        printf("%s", USAGE);
        exit(EXIT_FAILURE);
    }

    if (od.output[0] == '\0') {
        printf("%s: " ERRORRED("fatal error") ": missing output file\n", ERRORBRIGHT("smconv"));
        exit(EXIT_FAILURE);
    }

    if (od.file_count == 0) {
        printf("%s: " ERRORRED("fatal error") ": missing input file\n", ERRORBRIGHT("smconv"));
        exit(EXIT_FAILURE);
    }

    if (od.verbose_mode) {
        printf("%s: Loading modules...\n", ERRORBRIGHT("smconv"));
        fflush(stdout);
    }

    itl_bank_t *bank = itl_bank_create(od.files, od.file_count);

    if (od.verbose_mode) {
        printf("%s: Starting conversion...\n", ERRORBRIGHT("smconv"));
        fflush(stdout);
    }

    spc_bank_t *result = spc_bank_create(bank, od.hirom, od.check_effect_size);

    if (od.spc_mode) {
        spc_bank_make_spc(result, od.output);
    } else {
        spc_bank_export(result, od.output, od.no_header, od.symbol_prefix, od.banknumber);
    }

    spc_bank_destroy(result);
    itl_bank_destroy(bank);

    return 0;
}
