// =============================================================================
// Test: 2D array access
// =============================================================================
// Prevents: Incorrect stride calculation in multidimensional arrays
//
// This test verifies that the compiler correctly calculates offsets when
// accessing elements in 2D arrays. The stride for arr[i][j] should be:
// base + (i * row_size + j) * element_size
//
// For grid[4][8] accessing grid[2][5]:
// Offset = (2 * 8 + 5) * 1 = 21 bytes from base
// =============================================================================

// 4 rows, 8 columns = 32 bytes total
unsigned char grid[4][8];

// Verify 2D array with different dimensions
unsigned short wide[2][16];

void test_2d_access(void) {
    // Write to various positions
    grid[0][0] = 1;      // offset 0
    grid[0][7] = 2;      // offset 7
    grid[1][0] = 3;      // offset 8
    grid[2][5] = 42;     // offset 21
    grid[3][7] = 99;     // offset 31 (last element)
}

void test_2d_wide(void) {
    // 16-bit elements: stride is 32 bytes per row
    wide[0][0] = 0x1234;
    wide[1][15] = 0xABCD;  // offset (1*16 + 15) * 2 = 62 bytes
}

int main(void) {
    test_2d_access();
    test_2d_wide();
    return (int)grid[2][5];  // Should return 42
}
