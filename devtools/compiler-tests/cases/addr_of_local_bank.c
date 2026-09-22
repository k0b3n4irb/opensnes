/* The address of a local is a far pointer: its bank half must be WRITTEN.
 *
 * Until 2026-09-21 the alloc emission stored the 16-bit stack address and
 * left the pointer's bank word untouched, so `&local` handed to a
 * bank-honouring reader (any `const T *` parameter since #121, any lib asm
 * routine that reads the bank byte) carried whatever the stack held at that
 * slot. Zero on a fresh power-on — which is why it survived for months — and
 * a wild bank once earlier calls had dirtied the slot: the lib fixture's
 * collideRect(&a, &b) started returning 0 when an unrelated audio vector
 * moved the stack residue. */
typedef struct { int x, y; } Rect2;
extern unsigned char overlap(const Rect2 *a, const Rect2 *b);

unsigned char two_locals(void) {
    Rect2 a, b;
    a.x = 1;
    b.x = 2;
    return overlap(&a, &b);
}
