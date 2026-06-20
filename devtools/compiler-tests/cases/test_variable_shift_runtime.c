// =============================================================================
// Test: Variable shift stack offset correctness after pha
// =============================================================================
// BUG: After 'pha' pushes 2 bytes onto the stack, the shift count loaded via
//      'lda N,s' must use offset+2 to account for the push. Before the fix,
//      emitload() was used instead of emitload_adj(r1, fn, 2), causing the
//      shift count to read the wrong stack slot.
//
// This test exercises all 3 variable shift types:
//   - shl (shift left): value << count
//   - shr (shift right unsigned): value >> count
//   - sar (shift right signed): svalue >> count
//
// Detection: The generated ASM must show that after 'pha', the 'lda N,s'
//            instruction uses an offset adjusted by +2 from the pre-pha offset.
// =============================================================================

// Shift left by variable amount
unsigned short var_shl(unsigned short value, unsigned short count) {
    return value << count;
}

// Unsigned shift right by variable amount
unsigned short var_shr(unsigned short value, unsigned short count) {
    return value >> count;
}

// Signed arithmetic shift right by variable amount
short var_sar(short value, short count) {
    return value >> count;
}

// Multi-shift: uses all three shift types with the same variable count
unsigned short multi_shift(unsigned short a, short b, unsigned short cnt) {
    unsigned short left = a << cnt;
    unsigned short right = a >> cnt;
    short arith = b >> cnt;
    return left + right + (unsigned short)arith;
}
