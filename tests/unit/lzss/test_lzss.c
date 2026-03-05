// =============================================================================
// Unit Test: LZSS Module (Smoke Test)
// =============================================================================
// LZSS decompresses directly to VRAM, which cannot be read back from C.
// This smoke test verifies:
//   1. The module links correctly
//   2. LzssDecodeVram can be called without crashing
//   3. Invalid data (wrong tag byte) returns immediately without hanging
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/lzss.h>
#include <snes/text.h>

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

// Invalid data (wrong tag byte — should make LzssDecodeVram return immediately)
static const u8 invalid_data[] = { 0x00, 0x00, 0x00, 0x00 };

// Minimal valid LZ77 header with zero length (tag=0x10, length=0)
static const u8 empty_lz77[] = { 0x10, 0x00, 0x00, 0x00 };

// =============================================================================
// Test: Module links and function is callable
// =============================================================================
void test_lzss_link(void) {
    // Verify function pointer is not null (linker resolved it)
    void (*fn)(u8*, u16) = LzssDecodeVram;
    TEST("lzss: linked", fn != 0);
}

// =============================================================================
// Test: Invalid tag byte — should return immediately
// =============================================================================
void test_lzss_invalid_tag(void) {
    // Call with invalid data (tag byte 0x00, not 0x10)
    // Should return immediately without hanging
    LzssDecodeVram((u8*)invalid_data, 0x0000);
    TEST("lzss: bad tag ok", 1);  // If we got here, it didn't hang
}

// =============================================================================
// Test: Empty LZ77 data — zero length
// =============================================================================
void test_lzss_empty(void) {
    // Tag is 0x10 but length is 0 — should decompress nothing
    LzssDecodeVram((u8*)empty_lz77, 0x0000);
    TEST("lzss: empty ok", 1);  // If we got here, it didn't hang
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(1, 1, "LZSS MODULE TESTS");
    textPrintAt(1, 2, "-----------------");

    tests_passed = 0;
    tests_failed = 0;
    test_line = 4;

    // Run all tests
    test_lzss_link();
    test_lzss_invalid_tag();
    test_lzss_empty();

    // Show summary
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
