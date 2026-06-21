/* Test: Phase 5b extends param alias propagation to non-leaf functions.
 * - Non-leaf functions benefit from param aliasing (fewer param copies)
 * - But only LEAF functions can be frameless (non-leaf need stack for intermediates)
 * - Non-leaf with intermediates across calls MUST keep their frame */

extern void external_func(unsigned short a, unsigned short b);
extern unsigned short external_add(unsigned short a, unsigned short b);

/* Non-leaf with intermediate values across a call — MUST keep frame (sbc in prologue).
 * This is the pattern that caused the Phase 5b regression: oamSetXY, oamSetVisible etc.
 * Without a frame, sta 14,s writes into the CALLER's frame, corrupting everything. */
unsigned short compute_across_call(unsigned short x, unsigned short y) {
    unsigned short sum = x + y;
    external_func(x, y);  /* call — sum must survive on stack */
    return sum;
}

/* Non-leaf with param aliasing — should still have fewer param copies than
 * without Phase 5b, even though it keeps its frame */
void forward_with_work(unsigned short a, unsigned short b) {
    unsigned short c = a + b;
    external_func(a, c);
}
