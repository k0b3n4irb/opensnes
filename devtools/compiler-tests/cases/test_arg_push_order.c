/* Verifies cc65816's LEFT-TO-RIGHT argument push order — the ABI
 * convention documented in compiler/ABI.md. A regression to the C cdecl
 * right-to-left order would silently break every cross-function call.
 *
 * - caller_three_literals: caller-side check (pea ordering)
 * - callee_three: callee-side check (lda offsets, non-commutative
 *   so we know which slot is which)
 */

extern unsigned short ext_three(unsigned short a, unsigned short b, unsigned short c);

void caller_three_literals(void) {
    /* Args in source order: 0x1111, 0x2222, 0x3333.
     * Expected pea sequence (LEFT-TO-RIGHT push):
     *   pea.w 4369   ; 0x1111 — leftmost, pushed FIRST
     *   pea.w 8738   ; 0x2222
     *   pea.w 13107  ; 0x3333 — rightmost, pushed LAST → closest to SP
     *   jsl ext_three
     */
    ext_three(0x1111, 0x2222, 0x3333);
}

unsigned short callee_three(unsigned short a, unsigned short b, unsigned short c) {
    /* Non-commutative: a - b - c.
     * Expected (frameless leaf):
     *   lda 8,s   ; a (leftmost, HIGHEST offset)
     *   sec
     *   sbc 6,s   ; b (middle)
     *   sec
     *   sbc 4,s   ; c (rightmost, LOWEST offset)
     *   rtl
     */
    return a - b - c;
}
