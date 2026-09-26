/* A 32-bit multiply by a constant inlines when the product provably fits in
 * 16 bits (2026-09-26).
 *
 * cproc widens an array index to 32 bits and scales it by the row size, so
 * `board[r][c]` with a byte row is `(u32)r * 10`. That multiply used to call
 * tcc_mul32 (~250 cycles per access, 28 call sites across the corpus). A byte
 * times 10 is below 2^12: the low half is the 16-bit shift-add sequence and
 * the high half is 0. A 16-bit value times 10 can overflow 16 bits, so it
 * must keep the real 32-bit multiply. */
unsigned char board[24][10];

unsigned char cell(unsigned char r, unsigned char c) {
    return board[r][c];
}

unsigned long wide(unsigned short x) {
    return (unsigned long)x * 10;
}
