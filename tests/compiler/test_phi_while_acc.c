/**
 * SSA Phi-Node Test: while(1) loop with |= accumulator
 *
 * This reproduces the exact pattern from the button_test where
 * maxraw |= raw was observed to write to a different stack slot
 * than where the next iteration reads maxraw from.
 *
 * Pattern:
 *   while (1) {
 *       raw = get_value();
 *       maxraw = maxraw | raw;
 *       use(maxraw);
 *   }
 */

typedef unsigned short u16;
typedef unsigned char u8;

extern u16 get_value(void);
extern void use_value(u16 val);

/* Level A: Simplest while(1) accumulator */
void levelA_while_or(void) {
    u16 acc = 0;
    u16 val;
    while (1) {
        val = get_value();
        acc = acc | val;
        use_value(acc);
    }
}

/* Level B: Two accumulators in while(1) */
void levelB_while_two(void) {
    u16 raw;
    u16 maxraw = 0;
    u16 frame = 0;
    while (1) {
        raw = get_value();
        maxraw = maxraw | raw;
        frame = frame + 1;
        use_value(maxraw);
        use_value(frame);
    }
}

/* Level C: Accumulator with conditional (closer to button_test) */
void levelC_while_cond(void) {
    u16 raw;
    u16 maxraw = 0;
    u16 frame = 0;
    while (1) {
        raw = get_value();
        maxraw = maxraw | raw;
        frame = frame + 1;
        if (raw & 0x0080) {
            use_value(maxraw);
        }
        use_value(frame);
    }
}

/* Level D: Multiple accumulators + conditionals + function calls */
void levelD_while_complex(void) {
    u16 raw;
    u16 lib;
    u16 maxraw = 0;
    u16 frame = 0;
    u16 toggled = 0;
    while (1) {
        raw = get_value();
        lib = get_value();
        maxraw = maxraw | raw;
        frame = frame + 1;
        if (raw & 0x0080) {
            toggled = toggled + 1;
        }
        use_value(maxraw);
        use_value(frame);
        use_value(toggled);
    }
}
