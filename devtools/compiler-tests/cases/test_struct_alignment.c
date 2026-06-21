// =============================================================================
// Test: Struct alignment and padding
// =============================================================================
// Prevents: Incorrect struct layout, wrong field offsets, bad sizeof
//
// cproc follows standard C alignment rules:
// - u8 fields: 1-byte aligned
// - u16 fields: 2-byte aligned (padding inserted after u8 if needed)
// This test verifies the compiler generates consistent struct layouts.
//
// Expected layouts (with standard alignment):
//   Simple:     {u8 a, [pad], u16 b}         = 4 bytes
//   Mixed:      {u8 x, [pad], u16 y, u8 z, u8 w} = 6 bytes
//   Nested:     {Simple s(4), [pad?], Mixed m(6)} = 10 bytes
//   ThreeWords: {u16 a, u16 b, u16 c}        = 6 bytes
//   OneByte:    {u8 val}                      = 1 byte
// =============================================================================

typedef unsigned char u8;
typedef unsigned short u16;

// Simple struct: u8 + pad + u16 = 4 bytes
typedef struct {
    u8  a;     // offset 0
    u16 b;     // offset 2 (aligned)
} Simple;

// Mixed sizes: u8 + pad + u16 + u8 + u8 = 6 bytes
typedef struct {
    u8  x;     // offset 0
    u16 y;     // offset 2 (aligned)
    u8  z;     // offset 4
    u8  w;     // offset 5
} Mixed;

// Nested struct: Simple(4) + Mixed(6) = 10 bytes
typedef struct {
    Simple s;  // offset 0, size 4
    Mixed  m;  // offset 4, size 6
} Nested;

// Struct with all same-size members: no padding needed
typedef struct {
    u16 a;     // offset 0
    u16 b;     // offset 2
    u16 c;     // offset 4
} ThreeWords;

// Single byte struct
typedef struct {
    u8 val;    // offset 0
} OneByte;

Simple simple;
Mixed mixed;
Nested nested;
ThreeWords three;
OneByte one;

// Test: write to each field of Simple struct
void test_simple_access(void) {
    simple.a = 0x11;
    simple.b = 0x2233;
}

// Test: write to each field of Mixed struct
void test_mixed_access(void) {
    mixed.x = 0xAA;
    mixed.y = 0xBBCC;
    mixed.z = 0xDD;
    mixed.w = 0xEE;
}

// Test: nested struct field access
void test_nested_access(void) {
    nested.s.a = 0x01;
    nested.s.b = 0x0203;
    nested.m.x = 0x04;
    nested.m.y = 0x0506;
    nested.m.z = 0x07;
    nested.m.w = 0x08;
}

// Test: array of structs with stride
void test_array_stride(void) {
    Simple arr[3];
    arr[0].a = 1;
    arr[0].b = 100;
    arr[1].a = 2;
    arr[1].b = 200;
    arr[2].a = 3;
    arr[2].b = 300;
}

// Test: struct assignment through pointer
void test_ptr_access(void) {
    Simple *p = &simple;
    p->a = 0x55;
    p->b = 0x6677;
}

// Test: ThreeWords to verify word-aligned access
void test_threewords(void) {
    three.a = 0x1111;
    three.b = 0x2222;
    three.c = 0x3333;
}

int main(void) {
    test_simple_access();
    test_mixed_access();
    test_nested_access();
    test_array_stride();
    test_ptr_access();
    test_threewords();
    return (int)simple.a;
}
