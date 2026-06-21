/* Test: Mutable struct instance via typedef must be in RAM, not ROM.
 *
 * BUG: cproc's mktype() did not initialize the type->qual field.
 * When a #line directive before a typedef struct changed the heap layout,
 * garbage in type->qual could have QUALCONST set, causing the backend
 * to emit the variable as .rodata (ROM) instead of RAMSECTION (RAM).
 *
 * The #line directive below simulates what cc -E generates when comments
 * appear between includes and a typedef struct.
 */

typedef unsigned short u16;
typedef short s16;
typedef unsigned char u8;

/* Simulated extern declarations like snes.h would generate */
extern u8 dummy_data[];

# 50 "test_struct_typedef_rodata.c"
typedef struct {
    s16 x;
    s16 y;
    u8 flag;
} TestState;

TestState state = {10, 20, 1};

void update(void) {
    state.x = state.x + 1;
    state.y = state.y - 1;
    state.flag = 0;
}

int main(void) {
    update();
    return state.x;
}
