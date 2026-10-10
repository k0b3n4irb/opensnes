/* The late peephole pass (2026-10-10, issue #166): four forms the emitter
 * never writes, put in once the ordinary rules have settled.
 *   an index goes straight to X      ldx.b S
 *   a store of 0                     stz
 *   a counter                        inc.b S / dec.b S
 *   a constant stored twice          loaded once
 * And where the accumulator is still read afterwards, nothing changes. */
typedef unsigned short u16;
typedef unsigned char u8;
extern u16 tab[32], out[32], flag_a, flag_b;
extern u8 small;
u16 ext(u16 v);

/* the index is kept in a slot across the first load: ldx from the slot */
u16 index_x(u16 i) {
    u16 a = tab[i];
    if (a > 9) a = tab[i] + out[i];
    return a;
}

/* two stores of 0, nothing reads A after them */
void clear_two(void) {
    flag_a = 0;
    flag_b = 0;
    out[3] = tab[4];
}

/* the same constant to two places */
void same_const(void) {
    flag_a = 7;
    flag_b = 7;
    out[3] = tab[4];
}

/* the loop counter is one inc of its slot */
u16 count(u16 n) {
    u16 i, acc = 0;
    for (i = 0; i < n; i++) acc += tab[i & 31];
    return acc;
}

/* the 0 is returned: A is still needed, the load stays */
u16 zero_and_return(void) {
    flag_a = 0;
    return 0;
}

/* an 8-bit store of 0: left as the emitter wrote it */
void clear_byte(void) {
    small = 0;
    out[3] = tab[4];
}
