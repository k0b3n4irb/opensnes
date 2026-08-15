extern void sink(const char *s);
/* A: ternary of two string literals as a call arg */
void f_arg(int x) { sink(x ? "AAAA" : "BBBB"); }
/* B: ternary result via a local, then call */
void f_local(int x) { const char *p = x ? "AAAA" : "BBBB"; sink(p); }
/* C: ternary of two ints as a call arg (control) */
extern void isink(int v);
void f_int(int x) { isink(x ? 11 : 22); }
