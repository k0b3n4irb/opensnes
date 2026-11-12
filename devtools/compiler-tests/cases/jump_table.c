/* L2: a dense switch is a jump table (jt_find / emit_jt in emit.c). */
typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;

u16 g;
u8 st;
u16 tab[8];

/* eight cases from 0 and a default */
u16 dense(u16 v) {
    switch (v) {
    case 0: return 11;
    case 1: return 22;
    case 2: return 33;
    case 3: return 44;
    case 4: return 55;
    case 5: return 66;
    case 6: return 77;
    case 7: return 88;
    default: return 99;
    }
}

/* the first case is not 0 */
u16 based(u16 v) {
    switch (v) {
    case 10: g = 1; break;
    case 11: g = 2; break;
    case 12: g = 3; break;
    case 13: g = 4; break;
    case 14: g = 5; break;
    }
    return g;
}

/* negative cases on a signed value */
u16 negative(s16 v) {
    switch (v) {
    case -3: return 1;
    case -2: return 2;
    case -1: return 3;
    case 0: return 4;
    case 1: return 5;
    case 2: return 6;
    default: return 0;
    }
}

/* a byte, promoted */
void bytes(void) {
    switch (st) {
    case 0: st = 3; break;
    case 1: st = 0; break;
    case 2: st = 5; break;
    case 3: st = 1; break;
    case 4: st = 2; break;
    }
}

/* one variable set by every case, no default: the edges from the tree into
 * the join carry the variable's old value, through a stub */
u16 setter(u16 v, u16 r) {
    switch (v) {
    case 1: r = 10; break;
    case 2: r = 20; break;
    case 4: r = 40; break;
    case 5: r = 50; break;
    case 6: r = 60; break;
    }
    return r + v;
}

/* labels sharing a body, fallthrough, a default in the middle, holes */
u16 mixed(u16 v) {
    u16 r = 0;
    switch (v) {
    case 2: case 3: case 4:
        r += 5;
        /* falls through */
    case 6:
        r += 1;
        break;
    default:
        r = 100;
        break;
    case 8:
        r = 7;
        /* falls through */
    case 9:
        r += 2;
        break;
    }
    return r;
}

/* inside a loop whose counter indexes an array: the dispatch takes X */
void inloop(void) {
    u16 i;
    for (i = 0; i < 8; i++) {
        switch (tab[i]) {
        case 0: tab[i] = 5; break;
        case 1: tab[i] = 9; break;
        case 2: tab[i] = 1; break;
        case 3: tab[i] = 0; break;
        default: tab[i] = 2; break;
        }
    }
}
