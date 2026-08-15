extern void sink(const char *s);
extern void psink(int *p);
int ga, gb;
const char *gptr;

/* 1. ternary of string literals as arg (known broken) */
void t_str_arg(int x){ sink(x ? "AAAA" : "BBBB"); }
/* 2. ternary of &globals returned (pointer return) */
int *t_addr_ret(int x){ return x ? &ga : &gb; }
/* 3. ternary of string literals stored to a global far pointer */
void t_str_store(int x){ gptr = x ? "AAAA" : "BBBB"; }
/* 4. ternary of &globals as arg */
void t_addr_arg(int x){ psink(x ? &ga : &gb); }
/* 5. control: ternary of ints */
int t_int(int x){ return x ? 11 : 22; }
/* 6. array-of-string-pointers index (the #121 far-read path) */
const char *tbl[2] = { "AAAA", "BBBB" };
void t_tblidx(int x){ sink(tbl[x]); }
