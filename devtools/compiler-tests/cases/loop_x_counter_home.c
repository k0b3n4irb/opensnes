/* A loop counter whose address is taken has a home on the stack too, and
 * its step stores the new value twice: `lda S / inc a / sta S / sta 2,s`.
 * peephole_loop_x put the counter in X and kept `sta 2,s`, which then
 * stored A's stale value: after the loop the variable read 0, not 3
 * (difftest_stmt seed 105, 2026-10-15). The conversion is refused. */
typedef unsigned short u16;
u16 sink(u16 *p);

u16 counted(void) {
    u16 i;
    u16 *q = &i;
    for (i = 0; i < 3; i++) {
    }
    return sink(q);
}
