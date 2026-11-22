/* Runtime of the torture runner (testing/torture/run.py): a result word the host reads from WRAM,
 * and the few libc functions the tests call. Plain C, ours. */
unsigned int torture_result;
typedef unsigned int size_t;
void exit(int code) { torture_result = (code == 0) ? 0xC0DE : (0xBA00 | (code & 0xFF)); for (;;) {} }
void abort(void) { torture_result = 0xDEAD; for (;;) {} }
void *memcpy(void *d, const void *s, size_t n) { char *a = d; const char *b = s; while (n--) *a++ = *b++; return d; }
void *memmove(void *d, const void *s, size_t n) {
    char *a = d; const char *b = s;
    if (a < b) { while (n--) *a++ = *b++; } else { a += n; b += n; while (n--) *--a = *--b; }
    return d;
}
void *memset(void *d, int c, size_t n) { char *a = d; while (n--) *a++ = (char)c; return d; }
int memcmp(const void *x, const void *y, size_t n) {
    const unsigned char *a = x, *b = y;
    while (n--) { if (*a != *b) return *a - *b; a++; b++; }
    return 0;
}
size_t strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
int strcmp(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return (unsigned char)*a - (unsigned char)*b; }
char *strcpy(char *d, const char *s) { char *r = d; while ((*d++ = *s++)) {} return r; }
