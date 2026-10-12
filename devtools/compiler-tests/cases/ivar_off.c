/* The same first loop with QBE_NO_IVAR=1: the counter stays k and the body
 * scales it. */
typedef unsigned short u16;
extern u16 words[64];
void fill_words(u16 v) {
    u16 k;
    for (k = 0; k < 64; k++)
        words[k] = v;
}
