// =============================================================================
// Test: Variable-count shift operations (Phase 1.3 regression)
// =============================================================================
// BUG: `1 << variable` compiles to just `lda.w #1` — the shift is dropped.
//      Constant shifts (`1 << 3`) are folded at compile time, masking the bug.
//      This test uses PARAMETERS (not constants) to force runtime shifts.
//
// Detection: If bug is present, the generated assembly for shift_left_var()
//            will contain NO asl/lsr/__shl/__shr instructions.
// =============================================================================

// Shift left by variable amount (parameter prevents constant folding)
unsigned short shift_left_var(unsigned short value, unsigned short count) {
    return value << count;
}

// Shift right by variable amount
unsigned short shift_right_var(unsigned short value, unsigned short count) {
    return value >> count;
}

// Compute bitmask: 1 << idx (common SNES pattern for button/channel selection)
unsigned short compute_bitmask(unsigned short idx) {
    return 1 << idx;
}

// Combined: shift and mask (used in HDMA channel selection, DMA, etc.)
unsigned short extract_field(unsigned short reg, unsigned short shift, unsigned short mask) {
    return (reg >> shift) & mask;
}
