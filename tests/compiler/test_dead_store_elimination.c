/* Test: Dead store elimination (Phase 7a)
 *
 * global_increment: should be frameless (no tsa/sec/sbc) and
 * should not have intermediate sta to stack slots.
 *
 * phi_loop: has a phi node — frame must be preserved, phi args
 * must keep their stack slots for inter-block moves.
 */

extern unsigned short g_counter;

void global_increment(void) {
    g_counter = g_counter + 1;
}

unsigned short phi_loop(unsigned short n) {
    unsigned short sum = 0;
    unsigned short i;
    for (i = 0; i < n; i++) {
        sum = sum + i;
    }
    return sum;
}
