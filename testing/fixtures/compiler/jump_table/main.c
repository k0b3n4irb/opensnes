/* L2: a dense switch is a jump table (jt_find / emit_jt in
 * compiler/qbe/w65816/emit.c). Every shape the recogniser takes or refuses,
 * each function run on a sweep of values and folded into a checksum that
 * test_jump_table.py computes with a model of the same C. The table must
 * give what the compare tree gave: a wrong slot, a stale X, a phi move lost
 * on a stub is a wrong checksum. One sweep runs in the main loop and in the
 * NMI callback, each side counting its wrong results. */
#include <snes.h>

u16 g;
u16 tab[8];
u8 st;

/* eight cases from 0, a default */
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

/* the first case is 10, no default: g keeps its value */
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

/* negative cases, the window wraps on 16 bits */
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

/* the same window on an unsigned value: 0xFFFD..1 wraps */
u16 negu(u16 v) {
    switch (v) {
    case 0xFFFD: return 1;
    case 0xFFFE: return 2;
    case 0xFFFF: return 3;
    case 0: return 4;
    case 1: return 5;
    default: return 0;
    }
}

/* a byte, promoted; it is also the result */
u16 bytes(u8 v) {
    st = v;
    switch (st) {
    case 0: st = 3; break;
    case 1: st = 0; break;
    case 2: st = 5; break;
    case 3: st = 1; break;
    case 4: st = 2; break;
    }
    return st;
}

/* one variable set by every case, no default: the tree's edges into the
 * join carry r's old value */
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

/* the value read again after the switch */
u16 after(u16 v) {
    u16 r;
    switch (v) {
    case 3: r = 1; break;
    case 4: r = 2; break;
    case 5: r = 3; break;
    case 6: r = 4; break;
    case 7: r = 5; break;
    default: r = 6; break;
    }
    return r * 100 + v;
}

/* trees: three cases, sparse, 32 bits, a dense cluster in a sparse switch */
u16 few(u16 v) {
    switch (v) {
    case 0: return 5;
    case 1: return 6;
    case 2: return 7;
    default: return 0;
    }
}

u16 sparse(u16 v) {
    switch (v) {
    case 0: return 1;
    case 100: return 2;
    case 200: return 3;
    case 300: return 4;
    case 400: return 5;
    default: return 0;
    }
}

u16 wide(u32 v) {
    switch (v) {
    case 0: return 1;
    case 1: return 2;
    case 2: return 3;
    case 3: return 4;
    case 70000: return 5;
    case 5: return 6;
    default: return 0;
    }
}

u16 cluster(u16 v) {
    switch (v) {
    case 1000: return 9;
    case 0: return 1;
    case 1: return 2;
    case 2: return 3;
    case 3: return 4;
    case 4: return 5;
    case 5: return 6;
    case 6: return 7;
    case 7: return 8;
    case 2000: return 10;
    default: return 0;
    }
}

/* inside a loop indexed by its counter: the dispatch takes X */
u16 inloop(u16 seed) {
    u16 i, s = 0;
    for (i = 0; i < 8; i++)
        tab[i] = (seed + i * 3) & 7;
    for (i = 0; i < 8; i++) {
        switch (tab[i]) {
        case 0: tab[i] = 5; break;
        case 1: tab[i] = 9; break;
        case 2: tab[i] = 1; break;
        case 3: tab[i] = 0; break;
        case 5: tab[i] += i; break;
        default: tab[i] = 2; break;
        }
    }
    for (i = 0; i < 8; i++)
        s = s * 3 + tab[i];
    return s;
}

/* a loop whose counter is not an index, the switch on it */
u16 counted(u16 n) {
    u16 i, s = 0;
    for (i = 0; i < n; i++) {
        switch (i & 7) {
        case 0: s += 1; break;
        case 1: s += 10; break;
        case 2: s ^= 0x55; break;
        case 3: s <<= 1; break;
        case 4: s -= 3; break;
        default: s += i; break;
        }
    }
    return s;
}

#define NFN 15
u16 res[NFN];
u16 res_nmi;
u16 want_nmi;
u16 done, runs_main, runs_nmi, bad_main, bad_nmi;

static u16 fold(u16 s, u16 x) { return (u16)(s * 31 + x); }

/* sweeps of v over -20..1100 and a few far values */
static const s16 far_vals[] = { 2000, 1999, 2001, 32767, -32768, -1, -2, -3, -4, 70 };

static u16 sweep(u16 which) {
    u16 s = 0;
    s16 v;
    u16 k;
    for (v = -20; v <= 1100; v++) {
        switch (which) {
        case 0: s = fold(s, dense((u16)v)); break;
        case 1: s = fold(s, based((u16)v)); break;
        case 2: s = fold(s, negative(v)); break;
        case 3: s = fold(s, negu((u16)v)); break;
        case 4: s = fold(s, bytes((u8)v)); break;
        case 5: s = fold(s, setter((u16)v, (u16)(v * 7))); break;
        case 6: s = fold(s, mixed((u16)v)); break;
        case 7: s = fold(s, after((u16)v)); break;
        case 8: s = fold(s, few((u16)v)); break;
        case 9: s = fold(s, sparse((u16)v)); break;
        case 10: s = fold(s, wide((u32)(s32)v)); break;
        case 11: s = fold(s, cluster((u16)v)); break;
        case 12: if ((v & 63) == 0) s = fold(s, inloop((u16)v)); break;
        case 13: if ((v & 63) == 0) s = fold(s, counted((u16)v & 127)); break;
        default: s = fold(s, wide(70000UL + (u32)(s32)v)); break;
        }
    }
    for (k = 0; k < sizeof far_vals / sizeof far_vals[0]; k++) {
        v = far_vals[k];
        s = fold(s, dense((u16)v) + negative(v) + negu((u16)v) + cluster((u16)v));
    }
    return s;
}

/* what the NMI callback runs too: no global written (the main loop may be
 * inside the same sweep when the NMI comes), and short — the callback must
 * end within its frame with the table off too (QBE_NO_JUMP_TABLE=1), or
 * the next NMI interrupts it and the stack runs down */
static u16 sweep_small(void) {
    u16 s = 0;
    s16 v;
    for (v = -3; v <= 9; v++) {
        s = fold(s, dense((u16)v));
        s = fold(s, negative(v));
        s = fold(s, mixed((u16)v));
    }
    return fold(s, counted(9));
}

void in_nmi(void) {
    if (done || runs_nmi >= 16)
        return;
    if (sweep_small() != want_nmi)
        bad_nmi++;
    runs_nmi++;
}

int main(void) {
    u16 k;

    consoleInit();
    for (k = 0; k < NFN; k++)
        res[k] = sweep(k);
    want_nmi = sweep_small();
    nmiSet(in_nmi);
    setScreenOn();
    while (runs_main < 16 || runs_nmi < 16) {
        if (sweep_small() != want_nmi)
            bad_main++;
        WaitForVBlank();
        runs_main++;
    }
    done = 0xBEEF;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
