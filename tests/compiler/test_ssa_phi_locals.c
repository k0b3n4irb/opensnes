// =============================================================================
// Test: SSA phi-node confusion with many locals (Phase 1.3 regression)
// =============================================================================
// BUG: When 5+ local variables are each modified in separate if/else branches
//      based on different input bits, the compiler's SSA phi-node resolution
//      assigns wrong values to wrong stack slots.
//
// Detection: Assembly must contain all 12 expected constants (#10, #20, #30,
//            #40, #50, #60, #1, #2, #3, #4, #5, #6) and use 6+ unique
//            stack slot offsets for sta N,s instructions.
// =============================================================================

// External function — prevents constant folding of input value
extern unsigned short get_input(void);

// Each local is modified in a separate branch, forcing 6 parallel phi-nodes
unsigned short test_many_locals(unsigned short buttons) {
    unsigned short a = 1;
    unsigned short b = 2;
    unsigned short c = 3;
    unsigned short d = 4;
    unsigned short e = 5;
    unsigned short f = 6;

    // Each branch modifies exactly one local based on a different input bit
    if (buttons & 0x0001) {
        a = 10;
    }

    if (buttons & 0x0002) {
        b = 20;
    }

    if (buttons & 0x0004) {
        c = 30;
    }

    if (buttons & 0x0008) {
        d = 40;
    }

    if (buttons & 0x0010) {
        e = 50;
    }

    if (buttons & 0x0020) {
        f = 60;
    }

    // Sum forces all 6 locals to be live simultaneously at the return point
    return a + b + c + d + e + f;
}
