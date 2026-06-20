// =============================================================================
// Test: Static mutable variables must be in RAM, not ROM (Phase 1.3 regression)
// =============================================================================
// BUG: `static int counter = 0;` is placed in `.SECTION ".rodata.1" SUPERFREE`
//      which maps to ROM. Writes to these variables are silently ignored on
//      real hardware. The compiler should place mutable statics in RAMSECTION.
//
// Detection: Search backward from each symbol label to find its section
//            directive. Mutable statics must NOT be in a SUPERFREE section.
// =============================================================================

// Uninitialized static — should be in RAM (BSS)
static unsigned short uninit_counter;

// Zero-initialized static — should be in RAM
static unsigned short zero_counter = 0;

// Non-zero initialized static — should be in RAM (with init value copied from ROM)
static unsigned short init_counter = 100;

// Multiple statics of different types
static unsigned char byte_flag = 0;
static unsigned short word_state = 0;
static unsigned char byte_counter = 42;
static unsigned short word_accumulator = 1000;

// Increment all mutable statics
void increment_all(void) {
    uninit_counter = uninit_counter + 1;
    zero_counter = zero_counter + 1;
    init_counter = init_counter + 1;
    byte_flag = byte_flag + 1;
    word_state = word_state + 1;
    byte_counter = byte_counter + 1;
    word_accumulator = word_accumulator + 1;
}

// Read all statics (forces them to be live)
unsigned short read_all(void) {
    return uninit_counter + zero_counter + init_counter +
           byte_flag + word_state + byte_counter + word_accumulator;
}
