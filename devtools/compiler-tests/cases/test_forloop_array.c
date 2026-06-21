/**
 * @file test_forloop_array.c
 * @brief Test for-loop with struct array access
 *
 * This tests the compiler bug where for-loop array[i].field access
 * generates incorrect code for the first iteration (i=0).
 *
 * The bug manifests as incorrect stack offsets after push operations
 * in complex expressions involving multiply (for index scaling).
 */

/* Struct sized to require multiply for index calculation (48 bytes) */
typedef struct {
    short x;           /* offset 0 */
    short y;           /* offset 2 */
    short frame;       /* offset 4 */
    short size;        /* offset 6 */
    short gfx_addr;    /* offset 8 */
    short gfx_bank;    /* offset 10 */
    short padding[18]; /* offset 12-47 (36 bytes) */
} TestEntry;           /* Total: 48 bytes (requires multiply for indexing) */

/* Global array - 4 entries, 48 bytes each = 192 bytes total */
TestEntry entries[4];

/* Initialize array using for-loop - this triggers the bug */
void init_with_loop(void) {
    unsigned char i;
    for (i = 0; i < 4; i++) {
        entries[i].x = 64 + (i * 16);
        entries[i].y = 100;
        entries[i].frame = i;
        entries[i].size = 16;
    }
}

/* Initialize array without loop - this works correctly */
void init_without_loop(void) {
    entries[0].x = 64;
    entries[0].y = 100;
    entries[0].frame = 0;
    entries[0].size = 16;

    entries[1].x = 80;
    entries[1].y = 100;
    entries[1].frame = 1;
    entries[1].size = 16;

    entries[2].x = 96;
    entries[2].y = 100;
    entries[2].frame = 2;
    entries[2].size = 16;

    entries[3].x = 112;
    entries[3].y = 100;
    entries[3].frame = 3;
    entries[3].size = 16;
}

/* Verify that entry 0 has correct values */
int verify_entry0(void) {
    /* Expected: x=64, y=100, frame=0, size=16 */
    if (entries[0].x != 64) return 1;
    if (entries[0].y != 100) return 2;
    if (entries[0].frame != 0) return 3;
    if (entries[0].size != 16) return 4;
    return 0;
}

/* Test function that initializes with loop and verifies */
int test_forloop(void) {
    init_with_loop();
    return verify_entry0();
}
