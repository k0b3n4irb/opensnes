// =============================================================================
// Test: Switch statement
// =============================================================================
// Prevents: Incorrect jump table generation or fall-through handling
//
// This test verifies switch statements work correctly, including:
// - Simple cases
// - Fall-through
// - Default case
// - Sparse values (may use if-else chain instead of jump table)
// =============================================================================

unsigned char result;

void test_simple_switch(unsigned char val) {
    switch (val) {
        case 0:
            result = 10;
            break;
        case 1:
            result = 20;
            break;
        case 2:
            result = 30;
            break;
        default:
            result = 0;
            break;
    }
}

void test_fallthrough(unsigned char val) {
    result = 0;
    switch (val) {
        case 0:
        case 1:
        case 2:
            result = 100;  // Cases 0, 1, 2 all set result to 100
            break;
        case 3:
            result = 200;
            break;
    }
}

void test_sparse_values(unsigned short val) {
    // Sparse values - likely compiled as if-else chain
    switch (val) {
        case 0x0001:
            result = 1;
            break;
        case 0x0010:
            result = 2;
            break;
        case 0x0100:
            result = 3;
            break;
        case 0x1000:
            result = 4;
            break;
        default:
            result = 0;
            break;
    }
}

void test_no_default(unsigned char val) {
    result = 255;  // Set default before switch
    switch (val) {
        case 5:
            result = 50;
            break;
        case 10:
            result = 100;
            break;
    }
    // If val != 5 and val != 10, result stays 255
}

int main(void) {
    test_simple_switch(1);
    test_fallthrough(0);
    test_sparse_values(0x0100);
    test_no_default(7);
    return (int)result;
}
