// =============================================================================
// Unit Test: Type Definitions
// =============================================================================
// Validates that OpenSNES types have correct sizes and behavior on 65816.
// Uses inline test pattern (no test_harness.h) to avoid WLA-DX underscore
// label issues.
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/text.h>

// Compile-time type size checks
_Static_assert(sizeof(u8) == 1, "u8 must be 1 byte");
_Static_assert(sizeof(u16) == 2, "u16 must be 2 bytes");
_Static_assert(sizeof(s16) == 2, "s16 must be 2 bytes");
// Note: s32 = signed long long = 8 bytes on cproc/65816
// (types.h comment says "32-bit" but long long is actually 64-bit on cproc)
_Static_assert(sizeof(bool) == 1, "bool must be 1 byte");

static u8 tests_passed;
static u8 tests_failed;
static u8 test_line;

#define TEST(name, condition) do { \
    if (condition) { \
        tests_passed++; \
    } else { \
        tests_failed++; \
        textPrintAt(1, test_line, "FAIL:"); \
        textPrintAt(7, test_line, name); \
        test_line++; \
    } \
} while(0)

// =============================================================================
// Test: u8 range and overflow
// =============================================================================
void test_u8_range(void) {
    u8 val = 0;
    TEST("u8: zero", val == 0);

    val = 255;
    TEST("u8: max", val == 255);

    val = 255;
    val++;
    TEST("u8: overflow", val == 0);
}

// =============================================================================
// Test: u16 range and overflow
// =============================================================================
void test_u16_range(void) {
    u16 val = 0;
    TEST("u16: zero", val == 0);

    val = 65535;
    TEST("u16: max", val == 65535);

    val++;
    TEST("u16: overflow", val == 0);
}

// =============================================================================
// Test: s16 range
// =============================================================================
void test_s16_range(void) {
    s16 val = 0;
    TEST("s16: zero", val == 0);

    val = 32767;
    TEST("s16: max", val == 32767);

    val = -32768;
    TEST("s16: min", val == -32768);

    val = -1;
    TEST("s16: neg1", val == -1);
}

// =============================================================================
// Test: s32 range
// =============================================================================
void test_s32_size(void) {
    // s32 = signed long long on cproc/65816
    // Verify it's at least 4 bytes (can hold 32-bit values)
    TEST("s32: >=4B", sizeof(s32) >= 4);

    s32 val = 0;
    TEST("s32: zero", val == 0);

    val = 100000;
    TEST("s32: 100K", val == 100000);

    val = -100000;
    TEST("s32: -100K", val == -100000);
}

// =============================================================================
// Test: u16 arithmetic
// =============================================================================
void test_u16_arithmetic(void) {
    u16 a = 100;
    u16 b = 200;

    TEST("u16: add", (u16)(a + b) == 300);
    TEST("u16: sub", (u16)(b - a) == 100);
    TEST("u16: div", (u16)(b / a) == 2);
}

// =============================================================================
// Test: s16 arithmetic
// =============================================================================
void test_s16_arithmetic(void) {
    s16 a = 100;
    s16 b = -50;

    TEST("s16: add", (s16)(a + b) == 50);
    TEST("s16: sub", (s16)(a - b) == 150);
}

// =============================================================================
// Test: boolean values
// =============================================================================
void test_boolean_values(void) {
    bool t = TRUE;
    bool f = FALSE;

    TEST("bool: TRUE", t != 0);
    TEST("bool: FALSE", f == 0);
    TEST("bool: !FALSE", !f);
}

// =============================================================================
// Test: BIT macro
// =============================================================================
void test_bit_macro(void) {
    TEST("BIT(0)=1", BIT(0) == 0x0001);
    TEST("BIT(1)=2", BIT(1) == 0x0002);
    TEST("BIT(7)=80", BIT(7) == 0x0080);
    TEST("BIT(15)=8000", BIT(15) == 0x8000);
}

// =============================================================================
// Test: BYTE macros
// =============================================================================
void test_byte_macros(void) {
    u16 val = 0x1234;

    TEST("LO=0x34", LO_BYTE(val) == 0x34);
    TEST("HI=0x12", HI_BYTE(val) == 0x12);
    TEST("MAKE_WORD", MAKE_WORD(0x34, 0x12) == 0x1234);
}

// =============================================================================
// Test: MIN/MAX/CLAMP macros
// =============================================================================
void test_minmax_macros(void) {
    TEST("MIN(5,10)=5", MIN(5, 10) == 5);
    TEST("MIN(10,5)=5", MIN(10, 5) == 5);
    TEST("MAX(5,10)=10", MAX(5, 10) == 10);
    TEST("MAX(10,5)=10", MAX(10, 5) == 10);
    TEST("CLAMP low", CLAMP(3, 5, 10) == 5);
    TEST("CLAMP mid", CLAMP(7, 5, 10) == 7);
    TEST("CLAMP high", CLAMP(15, 5, 10) == 10);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(1, 1, "TYPE DEFINITION TESTS");
    textPrintAt(1, 2, "---------------------");

    tests_passed = 0;
    tests_failed = 0;
    test_line = 4;

    test_u8_range();
    test_u16_range();
    test_s16_range();
    test_s32_size();
    test_u16_arithmetic();
    test_s16_arithmetic();
    test_boolean_values();
    test_bit_macro();
    test_byte_macros();
    test_minmax_macros();

    test_line += 2;
    textPrintAt(1, test_line, "Passed: ");
    textPrintU16(tests_passed);
    test_line++;
    textPrintAt(1, test_line, "Failed: ");
    textPrintU16(tests_failed);
    test_line++;

    textPrintAt(1, test_line, "Static asserts: OK");
    test_line++;

    if (tests_failed == 0) {
        textPrintAt(1, test_line, "ALL TESTS PASSED");
    }

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
