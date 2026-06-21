// =============================================================================
// Test: Large local variables
// =============================================================================
// Prevents: Stack frame corruption with large local arrays
//
// This test verifies that the compiler correctly handles local variables
// that exceed 256 bytes, which requires special handling on the 65816
// due to the limited addressing modes.
//
// The 65816 direct page addressing only reaches 256 bytes, so larger
// locals must use stack-relative addressing with 16-bit offsets.
// =============================================================================

// Test with various sizes around the 256-byte boundary
void test_small_local(void) {
    unsigned char small[64];  // Well within direct page
    small[0] = 1;
    small[63] = 2;
}

void test_boundary_local(void) {
    unsigned char boundary[256];  // Exactly 256 bytes
    boundary[0] = 1;
    boundary[128] = 2;
    boundary[255] = 3;
}

void test_large_local(void) {
    unsigned char large[512];  // Exceeds direct page
    large[0] = 1;
    large[256] = 2;  // Must use 16-bit offset
    large[511] = 3;
}

void test_very_large_local(void) {
    unsigned char huge[1024];  // Much larger
    huge[0] = 0xAA;
    huge[512] = 0xBB;
    huge[1023] = 0xCC;
}

// Multiple large locals in same function
void test_multiple_large(void) {
    unsigned char buf1[256];
    unsigned char buf2[256];

    buf1[0] = 1;
    buf2[0] = 2;
    buf1[255] = buf2[0];  // Cross-reference
}

int main(void) {
    test_small_local();
    test_boundary_local();
    test_large_local();
    test_very_large_local();
    test_multiple_large();
    return 0;
}
