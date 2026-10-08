// Conditions must branch, not compute 0 or 1 and test it. Until 2026-10-08
// `a && b` stored 0 or 1 on each side and tested the stored value again,
// the compare a block branched on was materialised whenever the optimizer
// had scheduled something after it, and `x < 0` was a subtraction of zero
// with an overflow fix-up.
unsigned short arr[64];

unsigned short both(unsigned short a, unsigned short b) {
    if (a > 3 && b > 5) return 1;
    return 2;
}

unsigned short either(unsigned short a, unsigned short b, unsigned short c) {
    if (a == 1 || b == 2 || c == 3) return 7;
    return 9;
}

void sort_step(unsigned short key, unsigned short j) {
    while (j > 0 && arr[j - 1] > key) {
        arr[j] = arr[j - 1];
        j--;
    }
    arr[j] = key;
}

short magnitude(short x) {
    if (x < 0) x = -x;
    return x;
}

struct Ent { short x, y; };
struct Ent ent[16];
unsigned short walk(void) {
    struct Ent *e;
    unsigned short n = 0;
    for (e = ent; e != ent + 16; e++) n += e->x;
    return n;
}
