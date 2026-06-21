// =============================================================================
// Test: Loop constructs
// =============================================================================
// Prevents: Incorrect loop code generation
//
// Tests various loop patterns common in game development:
// - for loops with different increments
// - while loops
// - do-while loops
// - nested loops
// - break and continue
// =============================================================================

unsigned char buffer[64];
unsigned short counter;

void test_for_basic(void) {
    // Basic for loop - count up
    for (unsigned char i = 0; i < 10; i++) {
        buffer[i] = i;
    }
}

void test_for_countdown(void) {
    // Count down (common for sprite iteration)
    for (unsigned char i = 9; i > 0; i--) {
        buffer[i] = 10 - i;
    }
    buffer[0] = 10;  // Handle i=0 separately since unsigned can't go negative
}

void test_for_step(void) {
    // Step by 2
    for (unsigned char i = 0; i < 20; i += 2) {
        buffer[i] = i / 2;
    }

    // Step by 4
    for (unsigned char i = 0; i < 64; i += 4) {
        buffer[i] = 0xFF;
    }
}

void test_while(void) {
    unsigned char i = 0;
    while (i < 10) {
        buffer[i] = i * 2;
        i++;
    }
}

void test_do_while(void) {
    // Do-while always executes at least once
    unsigned char i = 0;
    do {
        buffer[i] = 100 + i;
        i++;
    } while (i < 5);
}

void test_nested_loops(void) {
    // Common pattern: iterate over 2D grid
    for (unsigned char y = 0; y < 4; y++) {
        for (unsigned char x = 0; x < 8; x++) {
            buffer[y * 8 + x] = y + x;
        }
    }
}

void test_break(void) {
    counter = 0;
    for (unsigned char i = 0; i < 100; i++) {
        counter++;
        if (i == 10) {
            break;  // Exit early
        }
    }
    // counter should be 11
}

void test_continue(void) {
    counter = 0;
    for (unsigned char i = 0; i < 10; i++) {
        if (i == 5) {
            continue;  // Skip i=5
        }
        counter += i;
    }
    // counter = 0+1+2+3+4+6+7+8+9 = 40 (skipped 5)
}

void test_infinite_with_break(void) {
    counter = 0;
    while (1) {
        counter++;
        if (counter >= 20) {
            break;
        }
    }
}

void test_multiple_conditions(void) {
    unsigned char x = 0;
    unsigned char y = 0;

    // Loop with multiple exit conditions
    while (x < 10 && y < 5) {
        x++;
        if (x % 2 == 0) {
            y++;
        }
    }
    counter = x + y;
}

int main(void) {
    test_for_basic();
    test_for_countdown();
    test_for_step();
    test_while();
    test_do_while();
    test_nested_loops();
    test_break();
    test_continue();
    test_infinite_with_break();
    test_multiple_conditions();
    return (int)counter;
}
