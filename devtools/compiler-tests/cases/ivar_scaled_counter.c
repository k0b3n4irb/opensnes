/* ivar (stage 2, T2, 2026-10-12): a loop counter whose every use is the same
 * scaled index becomes that index — a phi stepping by the scale, tested
 * against the scaled bound, no shift in the body. A counter with another
 * use, two scales or a variable bound is left as it is. */
typedef unsigned short u16;
typedef unsigned char u8;
extern u16 words[64];
extern u8 bytes[64];
extern u16 limit;

/* one scale: the counter becomes 2*k, bound 128, no asl */
void fill_words(u16 v) {
    u16 k;
    for (k = 0; k < 64; k++)
        words[k] = v;
}

/* the step is scaled too: k += 3 becomes j += 6 */
void every_third(u16 v) {
    u16 k;
    for (k = 2; k < 62; k += 3)
        words[k] = v;
}

/* two scales (a word array and a byte array): left alone */
u16 mixed(void) {
    u16 k, s = 0;
    for (k = 0; k < 64; k++)
        s += words[k] + bytes[k];
    return s;
}

/* the counter is also a value: left alone */
void ramp(void) {
    u16 k;
    for (k = 0; k < 64; k++)
        words[k] = k;
}

/* a variable bound: no proof that nothing wraps, left alone */
void fill_some(u16 v) {
    u16 k;
    for (k = 0; k < limit; k++)
        words[k] = v;
}
