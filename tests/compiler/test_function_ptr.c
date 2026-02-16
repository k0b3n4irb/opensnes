// =============================================================================
// Test: Function pointers (simplified)
// =============================================================================
// Prevents: Incorrect indirect call generation
//
// NOTE: Complex function pointer arrays trigger a QBE bug (assertion failure).
// This simplified test validates basic function pointer functionality.
// TODO: Fix QBE bug and expand this test.
// =============================================================================

unsigned char counter;

// Simple functions
void increment(void) {
    counter++;
}

void decrement(void) {
    counter--;
}

// Function pointer type
typedef void (*ActionFunc)(void);

void test_basic_funcptr(void) {
    ActionFunc f;

    counter = 10;

    f = increment;
    f();  // counter = 11

    f = decrement;
    f();  // counter = 10
}

// Callback pattern
void execute_action(ActionFunc action) {
    action();
}

void test_callback(void) {
    counter = 5;
    execute_action(increment);  // counter = 6
    execute_action(increment);  // counter = 7
}

int main(void) {
    test_basic_funcptr();
    test_callback();
    return (int)counter;
}
