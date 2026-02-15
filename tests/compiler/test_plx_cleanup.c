/* Test: PLX stack cleanup optimization
 * Verifies that small stack cleanups use PLX instead of tax/tsa/clc/adc/tas/txa.
 */

extern unsigned short add_one(unsigned short x);

/* Single-arg wrapper: cleanup = 2 bytes → should use PLX */
unsigned short wrapper_one(unsigned short x) {
    return add_one(x);
}

extern unsigned short add_two(unsigned short a, unsigned short b);

/* Two-arg wrapper: cleanup = 4 bytes → should use PLX (non-void) */
unsigned short wrapper_two(unsigned short a, unsigned short b) {
    return add_two(a, b);
}

extern void do_something(unsigned short x);

/* Void call, single arg: cleanup = 2 bytes → should use PLX */
void void_wrapper(unsigned short x) {
    do_something(x);
}
