// A field reached through a pointer: X is the pointer, the field's offset is
// the operand (`lda.l $00000N,x`). Until 2026-10-08 each field added its
// offset to the pointer in A, stored the sum, reloaded it and moved it to X.
// X is kept from one access to the next when nothing but accesses lies
// between them (`set` below); any other instruction makes it be reloaded.
//
// A 32-bit LOAD through a pointer keeps the form it always had, through the
// full 24-bit pointer (`lda [tcc__r9]`).
struct Ent { short x, y, vx, vy; };
struct Node { unsigned short v; struct Node *next; };

short sum(struct Ent *e) { return e->x + e->vx + e->y + e->vy; }
void set(struct Ent *e, short v) { e->vx = v; e->vy = v; }
struct Node *next(struct Node *n) { return n->next; }

// `(v >> 15) & 1`: the mask is not redundant (a signed shift gives 0 or -1).
// QBE sized shifts at 32 bits and dropped it; `(u >> 8) & 0xFF` on an
// unsigned value is the case where it IS redundant.
unsigned short sign_bit(short v) { return (v >> 15) & 1; }
unsigned short high_byte(unsigned short u) { return (u >> 8) & 0xFF; }
