// A whole-struct copy from a named const object reads the asset bank. Until
// 2026-10-17 funccopy emitted its loads with no access flag, so `r = kl;`
// read kl with bank-$00 addressing (`lda.w kl`) while `s = kl.k` read it
// far (audit 1.0, 04-2). Same for an element of a const array of structs,
// a const union, and a local initialised from a const object; a volatile
// source keeps its `volat` loads (IR only, not checked here).
struct L { unsigned long v; unsigned k; unsigned char c; };
union U { unsigned long v; unsigned k; };
static const struct L kl = { 0x11223344, 0x5566, 0x77 };
static const struct L ktab[2] = { { 1, 2, 3 }, { 4, 5, 6 } };
static const union U ku = { 0x8899AABB };
struct L r, rb; union U ru; unsigned s;
extern void sink(struct L *p);

void field(void) { s = kl.k; }
void whole(void) { r = kl; }
void elem(unsigned i) { rb = ktab[i]; }
void uni(void) { ru = ku; }
void init(void) { struct L t = kl; sink(&t); }
