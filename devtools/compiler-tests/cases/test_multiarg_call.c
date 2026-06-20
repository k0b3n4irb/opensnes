/**
 * Regression test for calling convention stack offset bug.
 *
 * This test verifies that when calling functions with multiple arguments,
 * the stack-relative offsets for local variables are correctly adjusted
 * as arguments are pushed to the stack.
 *
 * The bug was: when pushing arguments, SP changes but subsequent loads
 * from local variables didn't account for this, causing wrong values
 * to be passed to functions.
 */

/* External function prototype - takes multiple arguments */
void testfunc(int a, int b, int c, int d, int e, int f, int g);

/* Test that local variables are correctly passed as arguments */
void test_multiarg_call(void) {
    int v1 = 100;
    int v2 = 200;
    int v3 = 300;
    int v4 = 400;
    int v5 = 500;
    int v6 = 600;
    int v7 = 700;

    /* Call with all local variables as arguments */
    /* This tests that stack offsets are correctly adjusted during arg pushes */
    testfunc(v1, v2, v3, v4, v5, v6, v7);
}
