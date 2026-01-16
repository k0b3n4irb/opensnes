/**
 * @file test_minimal.c
 * @brief Minimal C file to test compiler output
 *
 * This is the simplest possible C file that should compile.
 * Used to verify the compiler toolchain works.
 */

/* Simple function that returns a constant */
int get_value(void) {
    return 42;
}

/* Function with a local variable */
int add_one(int x) {
    int result = x + 1;
    return result;
}

/* Function with static variable (tests .dsb generation) */
static int counter = 0;

int increment(void) {
    counter = counter + 1;
    return counter;
}
