/* L2: switches that keep their compare tree with the jump table on, and a
 * range test (two compares, a hundred values): not a dispatch. */
typedef unsigned short u16;
typedef unsigned long u32;

u16 g;

/* three cases: below JT_MIN_CASES */
u16 few(u16 v) {
    switch (v) {
    case 0: return 5;
    case 1: return 6;
    case 2: return 7;
    default: return 0;
    }
}

/* sparse: 5 cases over a window of 401 values */
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

/* a 32-bit value: 32-bit compares, not handled */
u16 wide(u32 v) {
    switch (v) {
    case 0: return 1;
    case 1: return 2;
    case 2: return 3;
    case 3: return 4;
    case 4: return 5;
    case 5: return 6;
    default: return 0;
    }
}

/* a dense cluster inside a sparse switch: no subtree holds the cluster
 * whole, the tree stays */
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

/* a range test: two compares and a hundred values on one side */
u16 inrange(u16 c) {
    if (c >= 32 && c < 128)
        return c - 32;
    return 0;
}
