/* Test: Phi-move A-cache correctness
 * When multiple phi moves are emitted in sequence, the A-cache must
 * track which Ref is actually in A. A stale cache causes phi moves
 * to skip loads, writing the wrong value to phi destination slots.
 *
 * Regression test for: writestring() loop with if/else branches
 * where sp, pos, and st are updated via phi nodes.
 */

extern unsigned short tilemap[];

void writestring(const char *st, unsigned short pos, unsigned short offset) {
    unsigned short sp = pos;
    unsigned char ch;
    while ((ch = *st)) {
        if (ch == '\n') {
            sp += 32;
            pos = sp;
        } else {
            tilemap[pos] = ch + offset;
            pos++;
        }
        st++;
    }
}
