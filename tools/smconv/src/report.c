#include "report.h"

#include <stdarg.h>
#include <stdio.h>

#define RED(STRING) "\x1B[31m" STRING "\033[0m"
#define BRIGHT(STRING) "\x1B[97m" STRING "\033[0m"

static smconv_report_fn g_fn = 0;
static void *g_user = 0;

void smconv_set_reporter(smconv_report_fn fn, void *user)
{
    g_fn = fn;
    g_user = user;
}

/* smconv's own voice: the verbose report on stdout, notes on stderr, and
 * the coloured "smconv: error:" lines its tests look for. */
static void default_report(smc_level level, const char *file, const char *msg)
{
    (void)file;
    switch (level) {
    case SMC_INFO:    printf("%s\n", msg); fflush(stdout); break;
    case SMC_NOTE:    fprintf(stderr, "smconv: note: %s\n", msg); break;
    case SMC_WARNING: printf("%s: " RED("warning") ": %s\n", BRIGHT("smconv"), msg); break;
    case SMC_ERROR:   printf("%s: " RED("error") ": %s\n", BRIGHT("smconv"), msg); break;
    case SMC_FATAL:   printf("%s: " RED("fatal error") ": %s\n", BRIGHT("smconv"), msg); break;
    }
}

void smconv_report(smc_level level, const char *file, const char *fmt, ...)
{
    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (g_fn) g_fn(level, file, msg, g_user);
    else default_report(level, file, msg);
}
