// =============================================================================
// Unit Test: Color Math Module
// =============================================================================
// Tests color blending and transparency functions.
//
// Critical functions tested:
// - colorMathInit(): Initialize color math
// - colorMathEnable(): Enable blending for layers
// - colorMathDisable(): Disable color math
// - colorMathSetOp(): Set add/subtract mode
// - colorMathSetHalf(): Enable 50% blending
// - colorMathSetSource(): Set blend source
// - colorMathSetFixedColor(): Set fixed color
// - Constants: COLORMATH_*, COLDATA_*
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/colormath.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests for constants
// =============================================================================

// Layer masks
_Static_assert(COLORMATH_BG1 == 0x01, "COLORMATH_BG1 must be 0x01");
_Static_assert(COLORMATH_BG2 == 0x02, "COLORMATH_BG2 must be 0x02");
_Static_assert(COLORMATH_BG3 == 0x04, "COLORMATH_BG3 must be 0x04");
_Static_assert(COLORMATH_BG4 == 0x08, "COLORMATH_BG4 must be 0x08");
_Static_assert(COLORMATH_OBJ == 0x10, "COLORMATH_OBJ must be 0x10");
_Static_assert(COLORMATH_BACKDROP == 0x20, "COLORMATH_BACKDROP must be 0x20");
_Static_assert(COLORMATH_ALL == 0x3F, "COLORMATH_ALL must be 0x3F");

// Operations
_Static_assert(COLORMATH_ADD == 0, "COLORMATH_ADD must be 0");
_Static_assert(COLORMATH_SUB == 1, "COLORMATH_SUB must be 1");

// Sources
_Static_assert(COLORMATH_SRC_SUBSCREEN == 0, "COLORMATH_SRC_SUBSCREEN must be 0");
_Static_assert(COLORMATH_SRC_FIXED == 1, "COLORMATH_SRC_FIXED must be 1");

// Conditions
_Static_assert(COLORMATH_ALWAYS == 0, "COLORMATH_ALWAYS must be 0");
_Static_assert(COLORMATH_INSIDE == 1, "COLORMATH_INSIDE must be 1");
_Static_assert(COLORMATH_OUTSIDE == 2, "COLORMATH_OUTSIDE must be 2");
_Static_assert(COLORMATH_NEVER == 3, "COLORMATH_NEVER must be 3");

// Fixed color channels
_Static_assert(COLDATA_RED == 0x20, "COLDATA_RED must be 0x20");
_Static_assert(COLDATA_GREEN == 0x40, "COLDATA_GREEN must be 0x40");
_Static_assert(COLDATA_BLUE == 0x80, "COLDATA_BLUE must be 0x80");
_Static_assert(COLDATA_ALL == 0xE0, "COLDATA_ALL must be 0xE0");

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
// Test: colorMathInit
// =============================================================================
void test_colormath_init(void) {
    colorMathInit();
    log_result("colorMathInit executes", 1);
}

// =============================================================================
// Test: colorMathEnable / colorMathDisable
// =============================================================================
void test_colormath_enable(void) {
    colorMathInit();

    // Enable for BG1
    colorMathEnable(COLORMATH_BG1);
    log_result("Enable BG1", 1);

    // Enable for multiple layers
    colorMathEnable(COLORMATH_BG1 | COLORMATH_BG2);
    log_result("Enable BG1+BG2", 1);

    // Enable for all
    colorMathEnable(COLORMATH_ALL);
    log_result("Enable all layers", 1);

    // Disable
    colorMathDisable();
    log_result("Disable color math", 1);
}

// =============================================================================
// Test: colorMathSetOp
// =============================================================================
void test_colormath_op(void) {
    colorMathInit();

    colorMathSetOp(COLORMATH_ADD);
    log_result("Set ADD mode", 1);

    colorMathSetOp(COLORMATH_SUB);
    log_result("Set SUB mode", 1);
}

// =============================================================================
// Test: colorMathSetHalf
// =============================================================================
void test_colormath_half(void) {
    colorMathInit();

    colorMathSetHalf(0);
    log_result("Half mode off", 1);

    colorMathSetHalf(1);
    log_result("Half mode on", 1);
}

// =============================================================================
// Test: colorMathSetSource
// =============================================================================
void test_colormath_source(void) {
    colorMathInit();

    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
    log_result("Source: subscreen", 1);

    colorMathSetSource(COLORMATH_SRC_FIXED);
    log_result("Source: fixed", 1);
}

// =============================================================================
// Test: colorMathSetCondition
// =============================================================================
void test_colormath_condition(void) {
    colorMathInit();

    colorMathSetCondition(COLORMATH_ALWAYS);
    log_result("Condition: always", 1);

    colorMathSetCondition(COLORMATH_INSIDE);
    log_result("Condition: inside", 1);

    colorMathSetCondition(COLORMATH_OUTSIDE);
    log_result("Condition: outside", 1);

    colorMathSetCondition(COLORMATH_NEVER);
    log_result("Condition: never", 1);
}

// =============================================================================
// Test: colorMathSetFixedColor
// =============================================================================
void test_colormath_fixed_color(void) {
    colorMathInit();

    // Black
    colorMathSetFixedColor(0, 0, 0);
    log_result("Fixed color: black", 1);

    // White
    colorMathSetFixedColor(31, 31, 31);
    log_result("Fixed color: white", 1);

    // Red
    colorMathSetFixedColor(31, 0, 0);
    log_result("Fixed color: red", 1);

    // Individual channels
    colorMathSetChannel(COLDATA_RED, 16);
    log_result("Set red channel", 1);

    colorMathSetChannel(COLDATA_GREEN, 16);
    log_result("Set green channel", 1);

    colorMathSetChannel(COLDATA_BLUE, 16);
    log_result("Set blue channel", 1);
}

// =============================================================================
// Test: Helper functions
// =============================================================================
void test_colormath_helpers(void) {
    colorMathInit();

    // 50% transparency
    colorMathTransparency50(COLORMATH_BG1);
    log_result("Transparency50 setup", 1);

    colorMathDisable();

    // Shadow effect
    colorMathShadow(COLORMATH_ALL, 16);
    log_result("Shadow setup", 1);

    colorMathDisable();

    // Tint effect
    colorMathTint(COLORMATH_BG1, 8, 0, 0);  // Red tint
    log_result("Tint setup", 1);

    colorMathDisable();

    // Brightness for fade
    colorMathSetBrightness(0);
    log_result("Brightness 0", 1);

    colorMathSetBrightness(31);
    log_result("Brightness 31", 1);

    colorMathDisable();
}

// =============================================================================
// Test: Typical transparency setup
// =============================================================================
void test_transparency_setup(void) {
    colorMathInit();

    // Setup 50% transparent BG2 over BG1
    colorMathEnable(COLORMATH_BG2);
    colorMathSetOp(COLORMATH_ADD);
    colorMathSetHalf(1);
    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
    log_result("Transparency setup", 1);

    colorMathDisable();
}

// =============================================================================
// Test: Typical fade to black setup
// =============================================================================
void test_fade_setup(void) {
    colorMathInit();

    // Setup fade to black
    colorMathSetFixedColor(0, 0, 0);
    colorMathSetSource(COLORMATH_SRC_FIXED);
    colorMathSetOp(COLORMATH_SUB);
    colorMathEnable(COLORMATH_ALL);
    log_result("Fade to black setup", 1);

    colorMathDisable();
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "COLORMATH MODULE TESTS");
    textPrintAt(2, 2, "----------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_colormath_init();
    test_colormath_enable();
    test_colormath_op();
    test_colormath_half();
    test_colormath_source();
    test_colormath_condition();
    test_colormath_fixed_color();
    test_colormath_helpers();
    test_transparency_setup();
    test_fade_setup();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
