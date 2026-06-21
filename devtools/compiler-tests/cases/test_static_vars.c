/**
 * Test: Static variable stores
 *
 * This test verifies that stores to static/global variables
 * generate proper symbol references, not literal $000000.
 *
 * BUG: The compiler was generating:
 *   sta.l $000000    ; WRONG
 * Instead of:
 *   sta.l my_static_var    ; CORRECT
 */

static unsigned char my_static_byte;
static unsigned short my_static_word;

int main(void) {
    /* Store to static byte */
    my_static_byte = 42;

    /* Store to static word */
    my_static_word = 0x1234;

    /* Read back */
    return my_static_byte + my_static_word;
}
