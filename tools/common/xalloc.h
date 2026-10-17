/*
 * xalloc.h — allocation that does not return NULL.
 *
 * The converters allocate a few hundred small blocks per run and had no
 * answer to a failed one: the next line wrote through NULL (cppcheck 2.22
 * counts about 180 such paths in smconv and img2snes). A tool that cannot
 * get memory has nothing useful left to do, so these print one line and
 * exit with CLI_IO (3), the code every tool gives an I/O failure. A size of
 * 0 asks for 1 byte, so that NULL always means failure.
 */
#ifndef OPENSNES_XALLOC_H
#define OPENSNES_XALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void *xalloc_check(void *p, size_t n)
{
    if (!p) {
        fprintf(stderr, "out of memory (%zu bytes)\n", n);
        exit(3);
    }
    return p;
}

static inline void *xmalloc(size_t n)
{
    return xalloc_check(malloc(n ? n : 1), n);
}

static inline void *xcalloc(size_t count, size_t size)
{
    if (count == 0 || size == 0) count = size = 1;
    return xalloc_check(calloc(count, size), count * size);
}

static inline void *xrealloc(void *p, size_t n)
{
    return xalloc_check(realloc(p, n ? n : 1), n);
}

static inline char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    return memcpy(xmalloc(n), s, n);
}

#endif /* OPENSNES_XALLOC_H */
