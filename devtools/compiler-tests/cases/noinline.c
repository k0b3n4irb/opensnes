/* __attribute__((noinline)) and [[gnu::noinline]] (I1 S3, 2026-11-14): the
 * function stays a function. No inliner copies it: not the one-site rule,
 * not the small inliner of `static inline`, not the copy of `static inline`
 * at several sites. Until then cproc skipped the attribute like any it did
 * not know, and a static with one call site was absorbed all the same.
 * On an object the attribute means nothing and is ignored, as GCC does. */
typedef unsigned short u16;
u16 tab[8];

/* one call site, static, small: everything says "absorb", the attribute says no */
static u16 __attribute__((noinline)) keep(u16 x) { if (x > 9) return x - 9; return x + 1; }
u16 one(u16 a) { return keep(a); }

/* the same function without the attribute: absorbed */
static u16 gone(u16 x) { if (x > 9) return x - 9; return x + 1; }
u16 one_gone(u16 a) { return gone(a); }

/* the C23 spelling */
[[gnu::noinline]] static u16 keep23(u16 x) { return x ^ 0x5A; }
u16 one23(u16 a) { return keep23(a); }

/* on the prototype only: it holds for the definition (GCC merges them) */
static u16 keep_proto(u16 x) __attribute__((noinline));
static u16 keep_proto(u16 x) { return x + tab[x & 7]; }
u16 one_proto(u16 a) { return keep_proto(a); }

/* `static inline` small enough for the small inliner, and noinline: a call */
static inline u16 __attribute__((noinline)) tiny(u16 x) { return x + 3; }
u16 small(u16 a) { return tiny(a); }

/* `static inline` with two call sites, and noinline: two calls */
static inline u16 __attribute__((noinline)) pick(u16 x) { if (x & 1) return x + 9; return x >> 1; }
u16 both(u16 a) { return pick(a) + pick(a + 2); }

/* an attribute on a parameter is not the function's: absorbed */
static u16 parm(u16 __attribute__((noinline)) x) { if (x > 9) return x - 9; return x + 1; }
u16 one_parm(u16 a) { return parm(a); }

/* on an object: ignored, the object is an ordinary initialised global */
u16 __attribute__((noinline)) obj = 4;
u16 read_obj(void) { return obj + 1; }
