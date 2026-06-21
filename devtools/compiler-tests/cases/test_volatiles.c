// =============================================================================
// Test: Volatile variables
// =============================================================================
// Prevents: Incorrect optimization of hardware register access
//
// Volatile is critical for SNES development because:
// - Hardware registers must be read/written every time
// - The compiler cannot cache or reorder accesses
// - Interrupt handlers share data with main code
// =============================================================================

// Simulated hardware registers (in SNES these would be at fixed addresses)
volatile unsigned char hw_status;
volatile unsigned char hw_data;
volatile unsigned short hw_addr;

// Shared between main and interrupt
volatile unsigned char vblank_flag;
volatile unsigned short frame_count;

void test_volatile_read(void) {
    unsigned char a, b, c;

    // Each read must actually access the variable
    // Compiler cannot optimize to: a = b = c = hw_status
    a = hw_status;
    b = hw_status;
    c = hw_status;

    // Use values to prevent dead code elimination
    hw_data = a + b + c;
}

void test_volatile_write(void) {
    // Each write must actually happen
    // Compiler cannot optimize away redundant writes
    hw_data = 0x01;
    hw_data = 0x02;
    hw_data = 0x03;
    hw_data = 0x04;
}

void test_volatile_sequence(void) {
    // Order must be preserved (common for SNES register setup)
    hw_addr = 0x1000;   // Set address first
    hw_data = 0x55;     // Then write data
    hw_data = 0xAA;     // Write more data
}

void test_wait_for_flag(void) {
    // Busy-wait loop (waiting for VBlank, etc.)
    // Without volatile, compiler might optimize to infinite loop
    vblank_flag = 0;

    // Simulated wait - in real code this waits for NMI
    while (vblank_flag == 0) {
        // Compiler must re-read vblank_flag each iteration
    }
}

void test_read_modify_write(void) {
    // Read-modify-write must read fresh value
    hw_status = hw_status | 0x80;  // Set bit 7
    hw_status = hw_status & 0x7F;  // Clear bit 7
}

// Simulated interrupt handler
void nmi_handler(void) {
    vblank_flag = 1;
    frame_count++;
}

void test_shared_data(void) {
    frame_count = 0;
    vblank_flag = 0;

    // Main code reads counter that interrupt updates
    unsigned short start = frame_count;

    // Do some work...
    hw_data = 0x00;

    // Check if frame advanced
    if (frame_count != start) {
        hw_data = 0xFF;
    }
}

int main(void) {
    test_volatile_read();
    test_volatile_write();
    test_volatile_sequence();
    // test_wait_for_flag(); // Would hang without interrupt
    test_read_modify_write();
    test_shared_data();
    return 0;
}
