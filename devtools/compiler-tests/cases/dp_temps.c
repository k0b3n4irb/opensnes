/* In a function that CALLS, a temporary that is not live across any call
 * lives in the direct page (tcc__lf), like a leaf's; one that a call
 * crosses stays on the stack (2026-10-10, issue #166). */
typedef unsigned short u16;
typedef short s16;
extern u16 tab[18], out[18];
u16 ext(u16 v);
void sink(u16 a, u16 b);

/* nothing crosses the call: no stack frame at all */
u16 short_lived(u16 id) {
    u16 a = tab[id] + 3;
    if (a > 100) a = (a >> 1) + tab[id + 1];
    out[id] = a ^ tab[id + 2];
    return ext(id);
}

/* `keep` is read after the call: stack. The rest: direct page. */
u16 crosses(u16 id) {
    u16 keep = tab[id] * 3 + 1;
    u16 t = (tab[id + 1] ^ 0x55) + id;
    out[id] = t + (t >> 2);
    return ext(t) + keep;
}

/* arguments read from the direct page while others are being pushed */
void pushes(u16 id) {
    u16 a = tab[id] + 1, b = tab[id + 1] + 2;
    sink(a + b, a ^ b);
}

/* a loop variable that lives across the call in the loop: stack */
u16 loop(void) {
    u16 k, acc = 0;
    for (k = 0; k < 18; k++) acc += ext(tab[k] + k);
    return acc;
}
