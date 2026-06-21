/**
 * Test: Dead store elimination for inline constant multiplies
 *
 * When a temp is produced and immediately consumed by an inline constant
 * multiply (values 2-15, powers of 2, composite), the slot store is dead
 * because the A-cache serves the value. This test verifies such stores
 * are eliminated.
 */

typedef unsigned short u16;

/* Chain: a*3 + b, then *5. The intermediate (a*3+b) slot store should be dead. */
u16 chain_mul(u16 a, u16 b) {
    u16 t = a * 3;
    t = t + b;
    t = t * 5;
    return t;
}

/* Chain with power-of-2: intermediate should be dead. */
u16 chain_shift(u16 a, u16 b) {
    u16 t = a + b;
    t = t * 8;
    return t;
}

/* Chain with composite: intermediate should be dead. */
u16 chain_composite(u16 a, u16 b) {
    u16 t = a + b;
    t = t * 24;
    return t;
}
