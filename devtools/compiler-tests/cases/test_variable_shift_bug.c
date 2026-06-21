// =============================================================================
// Test: Variable shift with u8 params in compound expressions
// =============================================================================
// BUG: `(1 << bg)` generates broken code when:
//   1. bg is unsigned char (u8) parameter
//   2. bg is also used for array indexing in the same function
//   3. The shift result is used in a compound expression (|=)
//
// The existing tests use u16 params and work. This test exercises the exact
// pattern from bgSetScrollX() that triggers the bug in real-world code.
//
// Detection: Compare assembly output for u8 vs u16 versions. The u8 version
//            must correctly load the shift count from the right stack slot
//            after the pha instruction.
// =============================================================================

volatile unsigned char dirty_flags;
unsigned short scroll_x[4];

// Pattern 1: u8 param + array store + variable shift + OR-assign
// This is the exact bgSetScrollX pattern that triggers the bug
void set_scroll_u8(unsigned char bg, unsigned short x) {
    scroll_x[bg] = x;
    dirty_flags |= (unsigned char)(1 << bg);
}

// Pattern 2: Same but with u16 param (known working)
void set_scroll_u16(unsigned short bg, unsigned short x) {
    scroll_x[bg] = x;
    dirty_flags |= (unsigned char)(1 << bg);
}

// Pattern 3: u8 shift-only (no array, no OR-assign)
unsigned short shift_only_u8(unsigned char bg) {
    return (unsigned short)(1 << bg);
}

// Pattern 4: u8 param used twice (array + shift) without OR-assign
unsigned short array_then_shift_u8(unsigned char bg, unsigned short x) {
    scroll_x[bg] = x;
    return (unsigned short)(1 << bg);
}

// Pattern 5: Multiple u8 uses in one expression
void multi_use_u8(unsigned char bg, unsigned short x, unsigned short y) {
    scroll_x[bg] = x;
    scroll_x[bg] = y;
    dirty_flags |= (unsigned char)(1 << bg);
}
