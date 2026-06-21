/* Test: Commutative operand swap optimization
 * When r1 is in A-cache from the previous instruction, commutative ops
 * (add, and, or, xor) swap operands to avoid an extra load.
 */

/* shift_and_add: idx<<1 stays in A-cache, then the add swaps to use
 * base (param alias) as emitop2 operand. The shl result is a dead store
 * (Case 3: arg[1] of next commutative op). Function should be frameless. */
unsigned short shift_and_add(unsigned short idx, unsigned short base) {
    return base + (idx << 1);
}

/* or_shifted: similar pattern with OR (commutative) */
unsigned short or_shifted(unsigned short a, unsigned short b) {
    return b | (a << 2);
}
