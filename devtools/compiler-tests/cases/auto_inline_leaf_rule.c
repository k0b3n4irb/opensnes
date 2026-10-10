/* The same source as auto_inline.c, compiled with CC_INLINE_LEAF_MAX=16:
 * the rule of the morning of 2026-10-10, kept behind its variable. */
/* A static function with ONE call site, whose address is not taken, is its
 * call site's code (2026-10-10, issue #166): its body, control flow
 * included, replaces the call and the function is not emitted. */
typedef unsigned short u16;
typedef unsigned char u8;
u16 tab[18];
u16 seen;

/* control flow, two returns: absorbed, and its caller becomes a leaf */
static u16 clamp(u16 x) { if (x > 500) return 500; if (x < 10) return 10; return x; }
void once(u16 id) { tab[id] = clamp(tab[id] + 3); }

/* a chain: inner into middle, middle into chain */
static void inner(u16 id) { if (tab[id]) seen |= 1; }
static void middle(u16 id) { u16 k; for (k = 0; k < 3; k++) inner(id + k); }
void chain(void) { middle(4); }

/* a byte parameter: the value is cut to the parameter's type */
static u16 low(u8 v) { return v + 1; }
u16 cut(u16 v) { return low(v); }

/* two call sites: stays a function */
static u16 twice(u16 x) { return x ^ 0x55; }
u16 two(u16 a) { return twice(a) + twice(a + 1); }

/* its address is used: stays a function */
static u16 taken(u16 x) { return x + 7; }
u16 (*hook)(u16) = taken;
u16 direct(u16 a) { return taken(a); }

/* calls itself: stays a function */
static u16 down(u16 n) { if (n == 0) return 0; return down(n - 1) + 1; }
u16 rec(u16 n) { return down(n) + 2; }

/* a local whose address leaves: stays a function */
void fill(u16 *p);
static u16 local(u16 x) { u16 v[2]; fill(v); return v[0] + x; }
u16 arr(u16 a) { return local(a); }

/* several call sites and `inline`: copied at each, and not emitted */
static inline u16 pick(u16 x) { if (x & 1) return x + 9; return x >> 1; }
u16 both(u16 a) { return pick(a) + pick(a + 2); }

/* a leaf of some size, called from a function that calls something else:
 * stays a function (it keeps its direct-page frame) */
static void sweep(void) {
    u16 k;
    for (k = 0; k < 18; k++) { tab[k] = (tab[k] ^ k) + (seen << 1); if (tab[k] > 400) seen++; }
}
void boot(void) { sweep(); fill(tab); }

/* the same leaf shape with no other call around: absorbed, the caller is a leaf */
static void sweep2(void) {
    u16 k;
    for (k = 0; k < 18; k++) { tab[k] = (tab[k] ^ k) + (seen << 2); if (tab[k] > 300) seen++; }
}
void alone(void) { sweep2(); }
