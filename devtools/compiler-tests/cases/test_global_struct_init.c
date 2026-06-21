// =============================================================================
// Test: Global struct initialization
// =============================================================================
// Prevents: Incorrect .data_init generation for initialized global structs
//
// Global variables with initializers must be:
// 1. Allocated in RAMSECTION (runtime storage)
// 2. Have initial values in .data_init section (ROM)
// 3. CopyInitData copies ROM->RAM at startup
//
// This test verifies complex initialization patterns:
// - Structs with mixed member types
// - Nested struct initialization
// - Array of initialized structs
// - Partial initialization (remaining fields zero)
// =============================================================================

typedef unsigned char u8;
typedef unsigned short u16;

// Simple initialized struct
typedef struct {
    u8  id;
    u16 value;
} Entry;

// Struct with multiple fields
typedef struct {
    u8  type;
    u8  flags;
    u16 x;
    u16 y;
    u8  health;
} Entity;

// Nested struct
typedef struct {
    u8  r;
    u8  g;
    u8  b;
} Color;

typedef struct {
    Color fg;
    Color bg;
    u8    bold;
} Style;

// Initialized globals
Entry entry1 = { 1, 1000 };
Entry entry2 = { 2, 2000 };

Entity player = { 1, 0x03, 128, 96, 100 };
Entity enemy  = { 2, 0x01, 200, 50, 50 };

Style default_style = { {31, 31, 31}, {0, 0, 0}, 0 };

// Array of initialized structs
Entry table[4] = {
    { 0, 100 },
    { 1, 200 },
    { 2, 300 },
    { 3, 400 }
};

// Partially initialized (rest should be zero)
Entity partial = { 3, 0, 0, 0, 0 };

// Zero-initialized (should be in .bss, no init data needed)
Entity zeroed;

// Read initialized values to verify
u16 test_read_entry(void) {
    return entry1.value + entry2.value;
}

u16 test_read_entity(void) {
    return player.x + player.y + player.health;
}

u8 test_read_style(void) {
    return default_style.fg.r + default_style.bg.r;
}

u16 test_read_table(void) {
    return table[0].value + table[3].value;
}

int main(void) {
    u16 sum = 0;
    sum = test_read_entry();
    sum = sum + test_read_entity();
    sum = sum + test_read_style();
    sum = sum + test_read_table();
    return (int)sum;
}
