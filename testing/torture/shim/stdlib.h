#ifndef _SHIM_STDLIB_H
#define _SHIM_STDLIB_H
typedef unsigned int size_t;
void abort(void);
void exit(int);
void *malloc(size_t);
void free(void *);
void *calloc(size_t, size_t);
int abs(int);
#define NULL ((void *)0)
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif
