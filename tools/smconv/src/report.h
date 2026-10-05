/*
 * report.h — the one place the converter speaks from (2026-10-05).
 *
 * smconv prints its diagnostics itself; opensnes-music, which runs the
 * same converter, installs a reporter so they come out in the family's
 * shape (stderr, "tool: file: message", collected under --json). With no
 * reporter installed the text is what smconv has always printed.
 */
#ifndef SMCONV_REPORT_H
#define SMCONV_REPORT_H

typedef enum { SMC_INFO, SMC_NOTE, SMC_WARNING, SMC_ERROR, SMC_FATAL } smc_level;

/* file may be NULL; msg is formatted, without a trailing newline. */
typedef void (*smconv_report_fn)(smc_level level, const char *file, const char *msg, void *user);

void smconv_set_reporter(smconv_report_fn fn, void *user);
void smconv_report(smc_level level, const char *file, const char *fmt, ...);

#endif
