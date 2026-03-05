/**
 * SSA Phi-Node Test: Large frame with many variables
 *
 * Tests if phi-node resolution breaks with large frames (100+ bytes).
 * Uses many local variables + accumulators in a while(1) loop
 * with conditionals and function calls — mimicking the original
 * button_test that had a 136-byte frame.
 */

typedef unsigned short u16;
typedef unsigned char u8;

extern u16 get_value(void);
extern void use_value(u16 val);

/* Level E: 8 accumulators + conditionals in while loop.
 * This should create a frame of ~80+ bytes with many phi-nodes. */
void levelE_many_acc(void) {
    u16 raw;
    u16 acc1 = 0;
    u16 acc2 = 0;
    u16 acc3 = 0;
    u16 acc4 = 0;
    u16 acc5 = 0;
    u16 acc6 = 0;
    u16 acc7 = 0;
    u16 acc8 = 0;
    u16 tmp1, tmp2, tmp3, tmp4;

    while (1) {
        raw = get_value();

        /* Accumulate OR per bit */
        if (raw & 0x0001) acc1 = acc1 + 1;
        if (raw & 0x0002) acc2 = acc2 + 1;
        if (raw & 0x0004) acc3 = acc3 + 1;
        if (raw & 0x0008) acc4 = acc4 + 1;
        if (raw & 0x0010) acc5 = acc5 | raw;
        if (raw & 0x0020) acc6 = acc6 | raw;
        if (raw & 0x0040) acc7 = acc7 + raw;
        if (raw & 0x0080) acc8 = acc8 + raw;

        /* Intermediate calculations to bloat the frame */
        tmp1 = acc1 + acc2;
        tmp2 = acc3 + acc4;
        tmp3 = acc5 + acc6;
        tmp4 = acc7 + acc8;

        use_value(tmp1 + tmp2);
        use_value(tmp3 + tmp4);
    }
}

/* Level F: 10 accumulators + nested conditionals.
 * Should push frame to 120+ bytes and stress phi resolution. */
void levelF_stress(void) {
    u16 raw, lib;
    u16 a = 0, b = 0, c = 0, d = 0, e = 0;
    u16 f = 0, g = 0, h = 0, j = 0, k = 0;
    u16 sum1, sum2;

    while (1) {
        raw = get_value();
        lib = get_value();

        if (raw & 0x0001) {
            a = a + 1;
            if (raw & 0x0100) {
                b = b | raw;
            }
        }
        if (raw & 0x0002) {
            c = c + 1;
            if (lib & 0x0001) {
                d = d + lib;
            }
        }
        if (raw & 0x0004) {
            e = e + raw;
        }
        if (raw & 0x0008) {
            f = f | lib;
        }
        if (raw & 0x0010) {
            g = g + 1;
        }
        if (raw & 0x0020) {
            h = h + raw;
        }
        if (raw & 0x0040) {
            j = j | raw;
        }
        if (raw & 0x0080) {
            k = k + lib;
        }

        sum1 = a + b + c + d + e;
        sum2 = f + g + h + j + k;

        use_value(sum1);
        use_value(sum2);
    }
}
