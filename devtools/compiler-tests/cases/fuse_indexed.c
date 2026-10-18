/* An indexed load that only feeds the next operation is its operand
 * (peephole_fuse_indexed, 2026-10-13): cmp sym,x / adc sym,x instead of a
 * temporary. The operand's place is kept, so a compare or a subtraction is
 * not reversed. */
typedef unsigned short u16;
extern u16 base[8], adj[8], sent[8], out[8];

u16 sum(u16 c) { return base[c] + adj[c]; }
u16 diff(u16 c) { return base[c] - adj[c]; }
void changed(u16 c, u16 v) { if (v != sent[c]) out[c] = v; }
