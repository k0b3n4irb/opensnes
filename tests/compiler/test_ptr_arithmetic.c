// =============================================================================
// Test: Pointer arithmetic
// =============================================================================
// Prevents: Incorrect pointer offset calculations
//
// This test verifies that pointer arithmetic correctly accounts for the
// size of the pointed-to type. On 65816:
// - char* increments by 1
// - short* increments by 2
// - long* increments by 4
// - struct* increments by sizeof(struct)
// =============================================================================

typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned short tile;
} Sprite;  // 4 bytes

unsigned char bytes[16];
unsigned short words[8];
Sprite sprites[4];

void test_char_ptr(void) {
    unsigned char *p = bytes;

    *p = 1;       // bytes[0]
    p++;
    *p = 2;       // bytes[1]
    p += 3;
    *p = 5;       // bytes[4]

    // Negative offset
    p--;
    *p = 4;       // bytes[3]
}

void test_short_ptr(void) {
    unsigned short *p = words;

    *p = 0x1234;  // words[0]
    p++;          // Should advance by 2 bytes
    *p = 0x5678;  // words[1]
    p += 2;       // Should advance by 4 bytes
    *p = 0xABCD;  // words[3]
}

void test_struct_ptr(void) {
    Sprite *p = sprites;

    p->x = 10;
    p->y = 20;
    p->tile = 0x100;

    p++;  // Should advance by 4 bytes (sizeof Sprite)
    p->x = 30;
    p->y = 40;
    p->tile = 0x101;

    // Index via pointer
    Sprite *q = &sprites[2];
    q->x = 50;
}

void test_array_indexing(void) {
    // Array indexing is pointer arithmetic
    unsigned char *p = &bytes[5];
    unsigned short *w = &words[3];
    Sprite *s = &sprites[1];

    *p = 100;
    *w = 200;
    s->x = 55;
}

int main(void) {
    test_char_ptr();
    test_short_ptr();
    test_struct_ptr();
    test_array_indexing();
    return 0;
}
