/**
 * Test: Function return values preserved through epilogue
 *
 * BUG: The function epilogue used `tsa` to adjust the stack pointer,
 * which overwrites the return value in the A register. This affected
 * EVERY non-void C function with a stack frame (framesize > 2).
 *
 * Fix: Save/restore A around stack cleanup using tax/txa.
 *
 * Functions that were broken: padPressed(), padHeld(), getFrameCount(),
 * rand(), and any user function returning a value with local variables.
 */

typedef unsigned short u16;
typedef unsigned char u8;

/* Extern to prevent inlining and constant folding */
extern u16 get_value(void);

/* Simple function with locals — return value must survive epilogue */
u16 compute_with_locals(u16 a, u16 b) {
    u16 sum = a + b;
    u16 diff = a - b;
    u16 result = sum + diff;
    return result;
}

/* Function with more locals — larger frame, same bug pattern */
u16 compute_complex(u16 x) {
    u16 a = x + 1;
    u16 b = x + 2;
    u16 c = x + 3;
    u16 d = a + b + c;
    return d;
}

/* Function that calls another function (creates stack frame for arguments) */
u16 call_and_return(void) {
    u16 val = get_value();
    u16 result = val + 42;
    return result;
}

/* u8 return value — must also be preserved */
u8 compute_byte(u8 a, u8 b) {
    u8 sum = a + b;
    return sum;
}
