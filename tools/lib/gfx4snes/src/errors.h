
#ifndef _GFX4SNES_ERRORS_H
#define _GFX4SNES_ERRORS_H

//-------------------------------------------------------------------------------------------------
extern void info (const char *format, ...);
extern void warning (const char *format, ...);
extern void note (const char *format, ...);
extern void fatal (const char *, ...) __attribute__((noreturn, format(printf, 1, 2)));
extern void errorcontinue (const char *format, ...);

#endif

