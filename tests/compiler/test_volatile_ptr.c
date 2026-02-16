// =============================================================================
// Test: Volatile pointer dereference
// =============================================================================
// Prevents: Optimizer removing hardware register reads/writes
//
// SNES hardware registers are accessed via memory-mapped I/O.
// Volatile pointers ensure the compiler does not:
// 1. Eliminate "redundant" reads (register value may change between reads)
// 2. Eliminate "dead" writes (writes have hardware side effects)
// 3. Reorder reads/writes (hardware timing sensitive)
//
// On 65816, volatile accesses must generate actual lda.l/sta.l instructions.
// =============================================================================

typedef unsigned char u8;
typedef unsigned short u16;

// Volatile pointer types (common SNES pattern)
typedef volatile u8*  vu8;
typedef volatile u16* vu16;

// SNES register addresses (Bank $00)
#define REG_INIDISP  (*(vu8)0x2100)
#define REG_VMAIN    (*(vu8)0x2115)
#define REG_VMADDL   (*(vu8)0x2116)
#define REG_VMADDH   (*(vu8)0x2117)
#define REG_VMDATAL  (*(vu8)0x2118)
#define REG_VMDATAH  (*(vu8)0x2119)
#define REG_JOY1     (*(vu16)0x4218)

u8 result8;
u16 result16;

// Test: Write to hardware register (must not be optimized away)
void test_write_register(void) {
    // Each write has a side effect - none should be eliminated
    REG_INIDISP = 0x0F;   // Screen on, full brightness
    REG_INIDISP = 0x80;   // Force blank
    REG_INIDISP = 0x0F;   // Screen on again
}

// Test: Read from hardware register (must not be cached)
void test_read_register(void) {
    u16 joy1, joy2;

    // Two consecutive reads - both must execute (value may change)
    joy1 = REG_JOY1;
    joy2 = REG_JOY1;

    result16 = joy1 | joy2;
}

// Test: Read-modify-write pattern
void test_read_modify_write(void) {
    u8 val;

    // Read current value, modify, write back
    val = REG_INIDISP;
    val = val | 0x80;     // Set force blank bit
    REG_INIDISP = val;
}

// Test: Sequential register writes (order matters)
void test_vram_write_sequence(void) {
    // VRAM write sequence - order is critical
    REG_VMAIN  = 0x80;    // Increment after high byte
    REG_VMADDL = 0x00;    // VRAM address low
    REG_VMADDH = 0x00;    // VRAM address high
    REG_VMDATAL = 0xFF;   // Data low byte
    REG_VMDATAH = 0x00;   // Data high byte (triggers write)
}

// Test: Volatile pointer arithmetic
void test_volatile_ptr_arith(void) {
    vu8 base = (vu8)0x2100;

    // Access registers via pointer arithmetic
    *(base + 0) = 0x0F;   // $2100 = INIDISP
    *(base + 1) = 0x00;   // $2101 = OBSEL
    *(base + 5) = 0x01;   // $2105 = BGMODE

    result8 = *(base + 0);
}

// Test: Loop writing to same volatile address
void test_volatile_loop(void) {
    u8 i;

    // Each iteration writes to the same address
    // None must be optimized away
    for (i = 0; i < 4; i++) {
        REG_VMDATAL = i;
        REG_VMDATAH = 0;
    }
}

int main(void) {
    test_write_register();
    test_read_register();
    test_read_modify_write();
    test_vram_write_sequence();
    test_volatile_ptr_arith();
    test_volatile_loop();
    return (int)result8;
}
