#ifndef _SHIM_STDDEF_H
#define _SHIM_STDDEF_H
typedef unsigned int size_t;
typedef int ptrdiff_t;
#define NULL ((void *)0)
#define offsetof(t, m) ((size_t)&(((t *)0)->m))
#endif
