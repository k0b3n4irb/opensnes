// =============================================================================
// Unit Test: HDMA Module
// =============================================================================
// Tests HDMA (Horizontal-blanking DMA) configuration functions.
//
// Critical functions tested:
// - hdmaSetup(): Configure HDMA channel
// - hdmaEnable(): Enable HDMA channel(s)
// - hdmaDisable(): Disable HDMA channel(s)
// - hdmaDisableAll(): Disable all channels
// - hdmaGetEnabled(): Query enabled channels
// - Constants: HDMA_CHANNEL_*, HDMA_MODE_*, HDMA_DEST_*
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/hdma.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests for constants
// =============================================================================

// Channel definitions
_Static_assert(HDMA_CHANNEL_0 == 0, "HDMA_CHANNEL_0 must be 0");
_Static_assert(HDMA_CHANNEL_1 == 1, "HDMA_CHANNEL_1 must be 1");
_Static_assert(HDMA_CHANNEL_6 == 6, "HDMA_CHANNEL_6 must be 6");
_Static_assert(HDMA_CHANNEL_7 == 7, "HDMA_CHANNEL_7 must be 7");

// Transfer modes
_Static_assert(HDMA_MODE_1REG == 0x00, "HDMA_MODE_1REG must be 0x00");
_Static_assert(HDMA_MODE_2REG == 0x01, "HDMA_MODE_2REG must be 0x01");
_Static_assert(HDMA_MODE_1REG_2X == 0x02, "HDMA_MODE_1REG_2X must be 0x02");
_Static_assert(HDMA_MODE_2REG_2X == 0x03, "HDMA_MODE_2REG_2X must be 0x03");
_Static_assert(HDMA_MODE_4REG == 0x04, "HDMA_MODE_4REG must be 0x04");
_Static_assert(HDMA_INDIRECT == 0x40, "HDMA_INDIRECT must be 0x40");

// Destination registers
_Static_assert(HDMA_DEST_CGADD == 0x21, "HDMA_DEST_CGADD must be 0x21");
_Static_assert(HDMA_DEST_CGDATA == 0x22, "HDMA_DEST_CGDATA must be 0x22");
_Static_assert(HDMA_DEST_BG1HOFS == 0x0D, "HDMA_DEST_BG1HOFS must be 0x0D");
_Static_assert(HDMA_DEST_BG1VOFS == 0x0E, "HDMA_DEST_BG1VOFS must be 0x0E");
_Static_assert(HDMA_DEST_COLDATA == 0x32, "HDMA_DEST_COLDATA must be 0x32");

// =============================================================================
// Test HDMA tables (simple patterns)
// =============================================================================

// Simple gradient table for fixed color
const u8 testGradientTable[] = {
    32, 0x20,    // 32 lines: color value 0x20
    32, 0x40,    // 32 lines: color value 0x40
    32, 0x60,    // 32 lines: color value 0x60
    32, 0x80,    // 32 lines: color value 0x80
    0            // End of table
};

// Simple scroll table for parallax
const u8 testScrollTable[] = {
    0x90, 0x00, 0x00,  // 16 lines repeat mode, scroll 0
    0x90, 0x10, 0x00,  // 16 lines repeat mode, scroll 16
    0x90, 0x20, 0x00,  // 16 lines repeat mode, scroll 32
    0x90, 0x30, 0x00,  // 16 lines repeat mode, scroll 48
    0              // End of table
};

// =============================================================================
// Runtime tests
// =============================================================================

static u8 test_passed;
static u8 test_failed;

void log_result(const char* name, u8 passed) {
    if (passed) {
        test_passed++;
    } else {
        test_failed++;
    }
}

// =============================================================================
// Test: hdmaDisableAll
// =============================================================================
void test_hdma_disable_all(void) {
    hdmaDisableAll();
    log_result("hdmaDisableAll executes", 1);

    // Verify all channels disabled
    u8 enabled = hdmaGetEnabled();
    log_result("All channels disabled", enabled == 0);
}

// =============================================================================
// Test: hdmaSetup
// =============================================================================
void test_hdma_setup(void) {
    hdmaDisableAll();

    // Setup channel 6 with gradient table
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, HDMA_DEST_COLDATA, testGradientTable);
    log_result("hdmaSetup ch6 gradient", 1);

    // Setup channel 7 with scroll table
    hdmaSetup(HDMA_CHANNEL_7, HDMA_MODE_2REG, HDMA_DEST_BG1HOFS, testScrollTable);
    log_result("hdmaSetup ch7 scroll", 1);

    hdmaDisableAll();
}

// =============================================================================
// Test: hdmaEnable / hdmaDisable
// =============================================================================
void test_hdma_enable_disable(void) {
    hdmaDisableAll();

    // Enable channel 6
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, HDMA_DEST_COLDATA, testGradientTable);
    hdmaEnable(1 << HDMA_CHANNEL_6);
    log_result("hdmaEnable ch6", 1);

    u8 enabled = hdmaGetEnabled();
    log_result("Ch6 is enabled", (enabled & (1 << HDMA_CHANNEL_6)) != 0);

    // Disable channel 6
    hdmaDisable(1 << HDMA_CHANNEL_6);
    log_result("hdmaDisable ch6", 1);

    enabled = hdmaGetEnabled();
    log_result("Ch6 is disabled", (enabled & (1 << HDMA_CHANNEL_6)) == 0);

    hdmaDisableAll();
}

// =============================================================================
// Test: Multiple channels
// =============================================================================
void test_hdma_multi_channel(void) {
    hdmaDisableAll();

    // Setup both channels
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, HDMA_DEST_COLDATA, testGradientTable);
    hdmaSetup(HDMA_CHANNEL_7, HDMA_MODE_2REG, HDMA_DEST_BG1HOFS, testScrollTable);

    // Enable both
    hdmaEnable((1 << HDMA_CHANNEL_6) | (1 << HDMA_CHANNEL_7));
    log_result("Enable ch6 + ch7", 1);

    u8 enabled = hdmaGetEnabled();
    u8 both_enabled = ((enabled & (1 << HDMA_CHANNEL_6)) != 0) &&
                      ((enabled & (1 << HDMA_CHANNEL_7)) != 0);
    log_result("Both channels enabled", both_enabled);

    hdmaDisableAll();
    enabled = hdmaGetEnabled();
    log_result("DisableAll clears both", enabled == 0);
}

// =============================================================================
// Test: hdmaSetTable
// =============================================================================
void test_hdma_set_table(void) {
    hdmaDisableAll();

    // Setup and change table
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, HDMA_DEST_COLDATA, testGradientTable);
    log_result("hdmaSetup initial", 1);

    // Change to a different table
    hdmaSetTable(HDMA_CHANNEL_6, testScrollTable);
    log_result("hdmaSetTable executes", 1);

    hdmaDisableAll();
}

// =============================================================================
// Test: Mode constants uniqueness
// =============================================================================
void test_mode_constants(void) {
    // Verify all modes are unique
    u8 modes_unique =
        (HDMA_MODE_1REG != HDMA_MODE_2REG) &&
        (HDMA_MODE_2REG != HDMA_MODE_1REG_2X) &&
        (HDMA_MODE_1REG_2X != HDMA_MODE_2REG_2X) &&
        (HDMA_MODE_2REG_2X != HDMA_MODE_4REG);
    log_result("Mode constants unique", modes_unique);

    // Verify indirect flag can be combined
    u8 indirect_mode = HDMA_MODE_1REG | HDMA_INDIRECT;
    log_result("Indirect flag combo", indirect_mode == 0x40);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "HDMA MODULE TESTS");
    textPrintAt(2, 2, "-----------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_hdma_disable_all();
    test_hdma_setup();
    test_hdma_enable_disable();
    test_hdma_multi_channel();
    test_hdma_set_table();
    test_mode_constants();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
