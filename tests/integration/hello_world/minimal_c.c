/**
 * Minimal C test - just return 0
 * Tests if crt0 tile writes work without C interference
 */

int main(void) {
    /* Do nothing - crt0 should have written solid white tiles */
    /* If we see white squares, crt0 works */
    /* If we see random colors, something in crt0 is wrong */

    while (1) {
        /* Infinite loop */
    }

    return 0;
}
