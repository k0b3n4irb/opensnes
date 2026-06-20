/* Test: Comparison dead store optimization
 * When a comparison result is used only by jnz (comparison+branch fusion),
 * the store to its stack slot is a dead store. This enables frameless
 * functions with conditional branches.
 */

/* Simple conditional: should be frameless (framesize=0) */
unsigned short max_val(unsigned short a, unsigned short b) {
    if (a > b) return a;
    return b;
}

/* Signed comparison: should also be frameless */
short smax_val(short a, short b) {
    if (a > b) return a;
    return b;
}

/* Equality check: should be frameless */
unsigned short eq_check(unsigned short x, unsigned short y) {
    if (x == y) return 1;
    return 0;
}
