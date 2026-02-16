/* Test: A-cache survives pha optimization
 * When pushing the same value twice as args, the second push should
 * not re-load from stack (A-cache hit from first load).
 *
 * Also tests: computation result pushed as arg should use A-cache
 * (dead store + A-cache pha = no intermediate store).
 */

extern unsigned short add_two(unsigned short a, unsigned short b);

/* Push same param twice: second pha should not emit another lda */
unsigned short call_same_twice(unsigned short x) {
    return add_two(x, x);
}

/* Computation result pushed immediately as arg */
extern unsigned short do_something(unsigned short a);

unsigned short call_with_computed(unsigned short x) {
    return do_something(x + 5);
}
