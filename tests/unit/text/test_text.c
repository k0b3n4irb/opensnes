// =============================================================================
// Unit Test: Text Module
// =============================================================================
// Tests text rendering cursor management and string output.
//
// Testable functions (state readable from C):
//   textInit()      — initializes text_config and cursor
//   textSetPos()    — sets cursor X/Y
//   textGetX()      — returns cursor X
//   textGetY()      — returns cursor Y
//   textInitEx()    — custom config
//
// Smoke-test only (hardware/DMA):
//   textFlush()     — triggers DMA, no read-back
//   textLoadFont()  — writes VRAM
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/text.h>

// text_config is declared extern in text.h
// Fields: tilemap_addr, font_tile, palette, priority, map_width

// Test counters
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
// Test: textInit default configuration
// =============================================================================
void test_text_init(void) {
    textInit();

    // Default config: tilemap at $7000, font tile 0, palette 0
    // Exact values depend on implementation, but should be reasonable
    TEST("init: map_w=32", text_config.map_width == 32);
    TEST("init: palette=0", text_config.palette == 0);
    TEST("init: font_t=0", text_config.font_tile == 0);

    // Cursor should start at (0,0)
    TEST("init: curX=0", textGetX() == 0);
    TEST("init: curY=0", textGetY() == 0);
}

// =============================================================================
// Test: textSetPos / textGetX / textGetY
// =============================================================================
void test_text_cursor(void) {
    textInit();

    textSetPos(5, 10);
    TEST("cur: X=5", textGetX() == 5);
    TEST("cur: Y=10", textGetY() == 10);

    textSetPos(0, 0);
    TEST("cur: X=0", textGetX() == 0);
    TEST("cur: Y=0", textGetY() == 0);

    textSetPos(31, 27);
    TEST("cur: X=31", textGetX() == 31);
    TEST("cur: Y=27", textGetY() == 27);
}

// =============================================================================
// Test: textPutChar advances cursor
// =============================================================================
void test_text_putchar(void) {
    textInit();
    textSetPos(0, 0);

    textPutChar('A');
    TEST("putc: X=1", textGetX() == 1);
    TEST("putc: Y=0", textGetY() == 0);

    textPutChar('B');
    TEST("putc2: X=2", textGetX() == 2);

    // Multiple characters
    textSetPos(10, 5);
    textPutChar('X');
    TEST("putc3: X=11", textGetX() == 11);
    TEST("putc3: Y=5", textGetY() == 5);
}

// =============================================================================
// Test: textPrint advances cursor by string length
// =============================================================================
void test_text_print(void) {
    textInit();
    textSetPos(0, 0);

    textPrint("Hi");
    TEST("print: X=2", textGetX() == 2);
    TEST("print: Y=0", textGetY() == 0);

    textSetPos(0, 1);
    textPrint("Hello");
    TEST("print2: X=5", textGetX() == 5);
}

// =============================================================================
// Test: textPrintAt sets position then prints
// =============================================================================
void test_text_print_at(void) {
    textInit();

    textPrintAt(3, 7, "AB");
    // After printAt(3,7,"AB"), cursor should be at (5, 7)
    TEST("at: X=5", textGetX() == 5);
    TEST("at: Y=7", textGetY() == 7);
}

// =============================================================================
// Test: textInitEx custom configuration
// =============================================================================
void test_text_init_ex(void) {
    textInitEx(0x3800, 128, 2);

    TEST("ex: tmap=$3800", text_config.tilemap_addr == 0x3800);
    TEST("ex: font_t=128", text_config.font_tile == 128);
    TEST("ex: palette=2", text_config.palette == 2);

    // Cursor should reset
    TEST("ex: curX=0", textGetX() == 0);
    TEST("ex: curY=0", textGetY() == 0);
}

// =============================================================================
// Test: textClear (smoke test — clears buffer, can't verify VRAM)
// =============================================================================
void test_text_clear(void) {
    textInit();
    textPrintAt(5, 5, "Test");
    textClear();
    // After clear, cursor position is implementation-defined
    // But the function should not crash
    tests_passed++;
}

// =============================================================================
// Test: textFlush (smoke test — triggers DMA flag)
// =============================================================================
void test_text_flush(void) {
    textInit();
    textPrintAt(0, 0, "Flush");
    textFlush();
    // No crash = pass
    tests_passed++;
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(1, 1, "TEXT MODULE TESTS");
    textPrintAt(1, 2, "-----------------");

    tests_passed = 0;
    tests_failed = 0;
    test_line = 4;

    test_text_init();
    test_text_cursor();
    test_text_putchar();
    test_text_print();
    test_text_print_at();
    test_text_init_ex();
    test_text_clear();
    test_text_flush();

    // Reinitialize text for display (textInitEx may have changed config)
    textInit();

    test_line += 2;
    textPrintAt(1, test_line, "Passed: ");
    textPrintU16(tests_passed);
    test_line++;
    textPrintAt(1, test_line, "Failed: ");
    textPrintU16(tests_failed);
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
