/* File-scope statics carry the translation unit's stem so two sources of one
 * project may define the same static (cproc, 2026-10-05): `counter` here is
 * emitted `counter.static_tu_suffix`, a static function likewise; an extern
 * stays bare, a block-scope static keeps the `.L<tu>_name.N` form. */
typedef unsigned short u16;
static u16 counter = 7;
u16 shared = 9;
static u16 bump(u16 x) { static u16 calls; calls++; return x + calls; }
u16 entry(void) { counter = bump(counter); return counter + shared; }
/* a second call site: with one, `bump` would be inlined (2026-10-10) */
u16 entry2(void) { return bump(3); }
