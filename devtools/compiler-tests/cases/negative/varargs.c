// Variadic functions: cproc parses the builtins, QBE's w65816 backend has no
// va_start / va_arg lowering. Must be refused, not miscompiled.
typedef __builtin_va_list va_list;
unsigned int sum(unsigned int n, ...) {
    va_list ap; unsigned int t = 0;
    __builtin_va_start(ap, n);
    while (n--) t += __builtin_va_arg(ap, unsigned int);
    __builtin_va_end(ap);
    return t;
}
unsigned int r;
int main(void) { r = sum(3, 1u, 2u, 3u); return 0; }
