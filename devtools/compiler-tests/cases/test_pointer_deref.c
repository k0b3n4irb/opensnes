/**
 * @file test_pointer_deref.c
 * @brief Regression test for pointer dereference
 *
 * Tests that pointer dereference generates correct code.
 * This catches bugs where ldw/stw with pointer operands fail.
 */

typedef unsigned short u16;
typedef unsigned char u8;

u16 array[4] = {100, 200, 300, 400};
u16 *ptr;

u16 read_via_pointer(void) {
    ptr = &array[2];
    return *ptr;  // Should be 300
}

void write_via_pointer(u16 value) {
    ptr = &array[1];
    *ptr = value;  // Should write to array[1]
}

// Test array access via pointer arithmetic
u16 pointer_arithmetic(void) {
    u16 *p = array;
    p = p + 2;
    return *p;  // Should be 300
}

// Test indirect call through function pointer
typedef void (*callback_t)(void);
u8 callback_called;

void target_func(void) {
    callback_called = 1;
}

void call_indirect(callback_t cb) {
    cb();  // Indirect call
}
