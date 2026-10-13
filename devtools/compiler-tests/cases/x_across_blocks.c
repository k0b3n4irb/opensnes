/* X holds the index across the blocks of a function (peephole_xflow,
 * 2026-10-13): a channel number indexing parallel arrays is scaled and put
 * in X once, not at the top of every block. */
typedef unsigned short u16;
extern u16 ch_pc[4], ch_vol[4], ch_sent[4], ch_flag[4];
extern u16 acc;
extern void note(u16 c);

/* four blocks, one tax */
void hardware(u16 c) {
    if (!ch_pc[c] && !ch_sent[c]) return;
    if (ch_flag[c]) {
        ch_flag[c] = 0;
        acc ^= ch_vol[c];
        return;
    }
    if (ch_vol[c] != ch_sent[c]) ch_sent[c] = ch_vol[c];
}

/* a call clobbers X: the index is loaded again after it */
void with_call(u16 c) {
    if (ch_pc[c]) note(c);
    ch_flag[c] = 1;
}
