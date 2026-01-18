/**
 * Regression test for calling convention stack offset adjustment.
 *
 * This test forces the compiler to use stack-relative loads when
 * pushing arguments, which exercises the sp_adjust fix.
 *
 * The trick: use extern variables to prevent constant folding,
 * then pass them as arguments.
 */

/* External function prototype - takes multiple arguments */
void receiver(int a, int b, int c, int d, int e);

/* External variables - compiler cannot optimize these away */
extern int ext_a;
extern int ext_b;
extern int ext_c;

/* Test that stack offsets are adjusted when pushing args */
void test_stack_adjust(void) {
    /* Load from extern vars into locals - forces stack storage */
    int local_a = ext_a;
    int local_b = ext_b;
    int local_c = ext_c;
    int local_d = ext_a + ext_b;
    int local_e = ext_b + ext_c;

    /* Call with locals as arguments */
    /* As we push args, SP changes, so subsequent loads need offset adjustment */
    receiver(local_a, local_b, local_c, local_d, local_e);
}
