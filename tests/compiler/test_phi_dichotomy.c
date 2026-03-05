/**
 * SSA Phi-Node Dichotomy Test
 *
 * Progressive complexity levels to isolate the phi-node bug.
 * Each function adds one layer of complexity.
 */

typedef unsigned short u16;
typedef unsigned char u8;

/* Prevent constant folding */
extern u16 get_value(void);

/* Level 1: Single accumulator in a counted loop */
u16 level1_single_acc(u16 n) {
    u16 acc = 0;
    u16 i;
    for (i = 0; i < n; i++) {
        acc = acc + 1;
    }
    return acc;
}

/* Level 2: Two accumulators in a loop */
u16 level2_two_acc(u16 n) {
    u16 acc1 = 0;
    u16 acc2 = 0;
    u16 i;
    for (i = 0; i < n; i++) {
        acc1 = acc1 + 1;
        acc2 = acc2 + 2;
    }
    return acc1 + acc2;
}

/* Level 3: OR accumulator (the original bug pattern) */
u16 level3_or_acc(u16 n) {
    u16 acc = 0;
    u16 i;
    u16 val;
    for (i = 0; i < n; i++) {
        val = get_value();
        acc = acc | val;
    }
    return acc;
}

/* Level 4: Two accumulators + OR (closer to maxraw pattern) */
u16 level4_mixed_acc(u16 n) {
    u16 acc_or = 0;
    u16 acc_sum = 0;
    u16 i;
    u16 val;
    for (i = 0; i < n; i++) {
        val = get_value();
        acc_or = acc_or | val;
        acc_sum = acc_sum + val;
    }
    return acc_or + acc_sum;
}

/* Level 5: Loop with conditional modification of accumulator */
u16 level5_cond_acc(u16 n) {
    u16 acc = 0;
    u16 i;
    u16 val;
    for (i = 0; i < n; i++) {
        val = get_value();
        if (val & 0x0080) {
            acc = acc | val;
        }
    }
    return acc;
}

/* Level 6: Multiple accumulators + conditionals (hdma_wave-like) */
u16 level6_multi_cond(u16 n) {
    u16 acc1 = 0;
    u16 acc2 = 0;
    u16 acc3 = 0;
    u16 i;
    u16 val;
    for (i = 0; i < n; i++) {
        val = get_value();
        if (val & 0x0001) {
            acc1 = acc1 + 1;
        }
        if (val & 0x0002) {
            acc2 = acc2 | val;
        }
        if (val & 0x0004) {
            acc3 = acc3 + val;
        }
    }
    return acc1 + acc2 + acc3;
}
