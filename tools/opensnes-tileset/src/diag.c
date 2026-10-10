/* diag.c — gfx4snes's errors.h, implemented in the family's voice. The
 * converter modules (images, palettes, tiles, maps, metasprites) call
 * info / warning / note / errorcontinue / fatal; here they reach the
 * shared command line: progress under -v only, warnings and errors on
 * stderr as "opensnes-tileset: file: message", fatal ends the run with
 * exit code 1 (the input was refused). */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "errors.h"

cli_ctx *tileset_ctx;            /* set by main before any converter call */
const char *tileset_file;        /* the input being converted, for the messages */

static void fmt(char *buf, size_t n, const char *format, va_list ap) { vsnprintf(buf, n, format, ap); }

void info(const char *format, ...)
{
    char m[1024]; va_list ap; va_start(ap, format); fmt(m, sizeof m, format, ap); va_end(ap);
    if (tileset_ctx) cli_note(tileset_ctx, "%s", m);
}

void warning(const char *format, ...)
{
    char m[1024]; va_list ap; va_start(ap, format); fmt(m, sizeof m, format, ap); va_end(ap);
    if (tileset_ctx) cli_warn(tileset_ctx, tileset_file, "%s", m); else fprintf(stderr, "warning: %s\n", m);
}

void note(const char *format, ...)
{
    char m[1024]; va_list ap; va_start(ap, format); fmt(m, sizeof m, format, ap); va_end(ap);
    fprintf(stderr, "  %s\n", m);     /* the continuation line of the warning above */
}

void errorcontinue(const char *format, ...)
{
    char m[1024]; va_list ap; va_start(ap, format); fmt(m, sizeof m, format, ap); va_end(ap);
    if (tileset_ctx) cli_error(tileset_ctx, tileset_file, "%s", m); else fprintf(stderr, "error: %s\n", m);
}

void fatal(const char *format, ...)
{
    char m[1024]; va_list ap; va_start(ap, format); fmt(m, sizeof m, format, ap); va_end(ap);
    /* gfx4snes ends some messages with "\nconversion terminated.": one line is enough here */
    for (char *p = m; *p; p++) if (*p == '\n') { *p = '\0'; break; }
    if (tileset_ctx) cli_error(tileset_ctx, tileset_file, "%s", m); else fprintf(stderr, "fatal: %s\n", m);
    exit(CLI_REFUSED);
}
