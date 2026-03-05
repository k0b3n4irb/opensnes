/**
 * Test: SSA phi-node confusion in complex loop
 *
 * This mimics the hdma_wave main loop structure:
 * - A loop with many local variables
 * - Conditional modifications inside the loop
 * - Complex expressions combining multiple variables
 * - Static variable reads/writes mixed with stack locals
 *
 * The goal is to reproduce the SSA phi-node confusion where
 * the compiler uses wrong stack slots for variables deep in the loop body.
 */

typedef unsigned short u16;
typedef unsigned char u8;

/* Static variables (like hdma_wave's state) */
static u8 state_a;
static u8 state_b;
static u8 state_c;
static u8 state_d;

/* Extern to prevent constant folding */
extern u16 read_input(void);
extern void wait_frame(void);

/* A lookup table (like amp_offsets) */
static const u16 offsets[7] = { 0, 100, 200, 300, 400, 500, 600 };

/* A large const array (like hdma_tables) */
extern const u8 big_table[];

u16 complex_loop(void) {
    u16 input, pressed, prev;
    u16 result;
    u16 tmp_offset;
    u16 base_addr;

    state_a = 0;
    state_b = 3;
    state_c = 0;
    state_d = 0;

    prev = 0;
    result = 0;

    /* Loop iteration (unrolled once for analysis) */
    wait_frame();
    input = read_input();
    pressed = input & ~prev;
    prev = input;

    /* Conditional A: toggle state_a */
    if (pressed & 0x0080) {
        if (state_a == 0) {
            state_a = 1;
        } else {
            state_a = 0;
            state_c = 0;
            state_d = 0;
        }
    }

    /* Conditional B: increment state_b */
    if (pressed & 0x0100) {
        if (state_b < 6) {
            state_b = state_b + 1;
        }
    }

    /* Conditional C: decrement state_b */
    if (pressed & 0x0200) {
        if (state_b > 0) {
            state_b = state_b - 1;
        }
    }

    /* Conditional D: set state_c */
    if (pressed & 0x0800) {
        state_c = 1;
    }

    /* Conditional E: clear state_c */
    if (pressed & 0x0400) {
        state_c = 0;
    }

    /* Phase advance (reads state_c, modifies state_d) */
    if (state_c) {
        state_d = state_d + 1;
        if (state_d >= 112) {
            state_d = 0;
        }
    }

    /* Complex expression combining multiple variables.
     * This is where the SSA bug would manifest:
     * tmp_offset should be state_d * 3 (now inline),
     * base_addr should use offsets[state_b],
     * result should combine them.
     */
    if (state_a) {
        tmp_offset = (u16)state_d * 3;
        base_addr = offsets[(u16)state_b] + tmp_offset;
        result = base_addr;
    }

    return result;
}
