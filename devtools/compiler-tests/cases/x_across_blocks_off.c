/* The same function with the rule off: the index is reloaded in its blocks. */
typedef unsigned short u16;
extern u16 ch_pc[4], ch_vol[4], ch_sent[4], ch_flag[4];
extern u16 acc;
void hardware(u16 c) {
    if (!ch_pc[c] && !ch_sent[c]) return;
    if (ch_flag[c]) {
        ch_flag[c] = 0;
        acc ^= ch_vol[c];
        return;
    }
    if (ch_vol[c] != ch_sent[c]) ch_sent[c] = ch_vol[c];
}
