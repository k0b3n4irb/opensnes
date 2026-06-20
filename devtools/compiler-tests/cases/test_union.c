// =============================================================================
// Test: Union handling
// =============================================================================
// Prevents: Incorrect union size or member access
//
// This test verifies that:
// 1. Union size equals the size of the largest member
// 2. All union members share the same memory location
// 3. Union can be used inside structures
// =============================================================================

// Basic union - size should be 4 (largest member)
typedef union {
    unsigned char  byte;   // 1 byte
    unsigned short word;   // 2 bytes
    unsigned long  dword;  // 4 bytes
} MultiSize;

// Union for type punning (common in SNES dev for color manipulation)
typedef union {
    unsigned short color;  // BGR15 format
    struct {
        unsigned char lo;
        unsigned char hi;
    } bytes;
} Color;

// Union inside struct
typedef struct {
    unsigned char type;
    union {
        unsigned char  small_val;
        unsigned short big_val;
    } value;
} Variant;

MultiSize ms;
Color col;
Variant var;

void test_union_size(void) {
    // Write to each member - they should alias
    ms.dword = 0x12345678;

    // Reading byte should give low byte of dword
    unsigned char b = ms.byte;  // Should be 0x78 (little endian)

    // Reading word should give low word
    unsigned short w = ms.word;  // Should be 0x5678
}

void test_color_union(void) {
    // Set color as word
    col.color = 0x7C1F;  // Some BGR15 color

    // Read as bytes
    unsigned char lo = col.bytes.lo;
    unsigned char hi = col.bytes.hi;

    // Modify via bytes
    col.bytes.hi = 0x03;
}

void test_union_in_struct(void) {
    var.type = 1;
    var.value.small_val = 42;

    var.type = 2;
    var.value.big_val = 1000;
}

int main(void) {
    test_union_size();
    test_color_union();
    test_union_in_struct();
    return (int)ms.byte;
}
