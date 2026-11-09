/* Prepended to every gcc.c-torture test: what its libc and its compiler would provide.
 * The host preprocessor predefines the HOST's limits (32-bit int); they are replaced by the
 * target's. Written for this runner; no gcc text here. */
typedef unsigned int size_t;
void abort(void);
void exit(int);
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
size_t strlen(const char *);
int strcmp(const char *, const char *);
char *strcpy(char *, const char *);
#define __builtin_abort abort
#define __builtin_exit exit
#define __builtin_memcpy memcpy
#define __builtin_memmove memmove
#define __builtin_memset memset
#define __builtin_memcmp memcmp
#define __builtin_strlen strlen
#define __builtin_strcmp strcmp
#define __builtin_strcpy strcpy
#undef __CHAR_BIT__
#define __CHAR_BIT__ 8
#undef __SCHAR_MAX__
#define __SCHAR_MAX__ 127
#undef __SHRT_MAX__
#define __SHRT_MAX__ 32767
#undef __INT_MAX__
#define __INT_MAX__ 32767
#undef __LONG_MAX__
#define __LONG_MAX__ 2147483647L
#undef __SIZEOF_SHORT__
#define __SIZEOF_SHORT__ 2
#undef __SIZEOF_INT__
#define __SIZEOF_INT__ 2
#undef __SIZEOF_LONG__
#define __SIZEOF_LONG__ 4
#undef __ORDER_LITTLE_ENDIAN__
#undef __ORDER_BIG_ENDIAN__
#undef __BYTE_ORDER__
#define __ORDER_LITTLE_ENDIAN__ 1234
#define __ORDER_BIG_ENDIAN__ 4321
#define __BYTE_ORDER__ 1234
