// =============================================================================
// Test: Function pointers — direct, callback, tables (const / RAM / 2D / struct)
// =============================================================================
// Prevents: Incorrect indirect call generation.
//
// History: the original fixture carried "Complex function pointer arrays
// trigger a QBE bug (assertion failure) — TODO: Fix QBE bug". Re-checked
// 2026-09-11 (gaps review, item C3): every table shape below compiles and the
// indirect call through a table lowers to `jml [tcc__r9]`, while a pointer
// that holds a known constant (`f = increment; f();`) is folded to a direct
// `jsl`. The .checks file pins both: five indirect sites, the folded direct
// call, and the tables emitted as `.dl` far-pointer data. A QBE abort on the
// table shapes (the historical "assertion failure") fails here too.
// =============================================================================

unsigned char counter;

void increment(void) { counter++; }
void decrement(void) { counter--; }
void nothing(void)   { }

typedef void (*ActionFunc)(void);
typedef struct { ActionFunc fn; unsigned char times; } Entry;

// Direct assignment + call
void test_basic_funcptr(void) {
    ActionFunc f;
    counter = 10;
    f = increment;
    f();                    // folded to `jsl increment` (constant pointer)
    f = decrement;
    f();                    // folded likewise
}

// Callback pattern
void execute_action(ActionFunc action) {
    action();               // indirect call 1
}

// const table in ROM
static const ActionFunc rom_table[3] = { increment, decrement, nothing };
void dispatch_rom(unsigned char i) {
    rom_table[i](); // indirect call 2
}

// mutable table in RAM, reassigned at runtime
ActionFunc ram_table[2] = { increment, decrement };
void dispatch_ram(unsigned char i) {
    ram_table[i & 1] = i ? increment : decrement;
    ram_table[i & 1](); // indirect call 3
}

// 2D table
static const ActionFunc grid[2][2] = { { increment, decrement }, { decrement, nothing } };
void dispatch_grid(unsigned char r, unsigned char c) {
    grid[r & 1][c & 1](); // indirect call 4
}

// function pointer inside a struct, in a const array
static const Entry menu[2] = { { increment, 3 }, { decrement, 1 } };
void dispatch_menu(unsigned char i) {
    unsigned char n = menu[i & 1].times;
    while (n--) menu[i & 1].fn(); // indirect call 5
}

int main(void) {
    test_basic_funcptr();
    execute_action(increment);
    dispatch_rom(0);
    dispatch_ram(1);
    dispatch_grid(1, 0);
    dispatch_menu(0);
    return (int)counter;
}
