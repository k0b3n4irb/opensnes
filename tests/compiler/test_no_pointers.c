/**
 * @file test_no_pointers.c
 * @brief Minimal C file without pointers or local variables
 *
 * Tests basic compiler output without triggering storel type issues.
 */

/* Simple function that returns a constant */
int get_value(void) {
    return 42;
}

/* Function with arithmetic */
int add_numbers(int a, int b) {
    return a + b;
}

/* Global variable */
int global_counter = 0;

/* Function using global */
int get_counter(void) {
    return global_counter;
}

/* Function incrementing global */
int inc_counter(void) {
    global_counter = global_counter + 1;
    return global_counter;
}
