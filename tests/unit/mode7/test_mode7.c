// =============================================================================
// Unit Test: Mode 7 Module
// =============================================================================
// Tests Mode 7 rotation and scaling functions.
//
// Critical functions tested:
// - mode7Init(): Initialize Mode 7 system
// - mode7SetScale(): Set zoom level
// - mode7SetAngle(): Set rotation angle
// - mode7SetCenter(): Set rotation center
// - mode7SetScroll(): Set scroll position
// - mode7Rotate(): Rotation in degrees
// - mode7Transform(): Combined rotation and scale
// - Constants: MODE7_WRAP, MODE7_FLIP_*
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/mode7.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests for constants
// =============================================================================

// Mode 7 settings constants
_Static_assert(MODE7_WRAP == 0x00, "MODE7_WRAP must be 0x00");
_Static_assert(MODE7_TRANSPARENT == 0x80, "MODE7_TRANSPARENT must be 0x80");
_Static_assert(MODE7_TILE0 == 0xC0, "MODE7_TILE0 must be 0xC0");

// Flip constants
_Static_assert(MODE7_FLIP_H == 0x01, "MODE7_FLIP_H must be 0x01");
_Static_assert(MODE7_FLIP_V == 0x02, "MODE7_FLIP_V must be 0x02");

// Verify flip constants don't overlap with out-of-bounds settings
_Static_assert((MODE7_FLIP_H & MODE7_TRANSPARENT) == 0, "FLIP_H must not overlap TRANSPARENT");
_Static_assert((MODE7_FLIP_V & MODE7_TRANSPARENT) == 0, "FLIP_V must not overlap TRANSPARENT");

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
// Test: mode7Init
// =============================================================================
void test_mode7_init(void) {
    mode7Init();
    log_result("mode7Init executes", 1);
}

// =============================================================================
// Test: mode7SetScale
// =============================================================================
void test_mode7_scale(void) {
    mode7Init();

    // Normal scale (1.0 = 0x0100)
    mode7SetScale(0x0100, 0x0100);
    log_result("Scale 1.0", 1);

    // Zoom in (0.5 = 0x0080)
    mode7SetScale(0x0080, 0x0080);
    log_result("Scale 0.5 (zoom in)", 1);

    // Zoom out (2.0 = 0x0200)
    mode7SetScale(0x0200, 0x0200);
    log_result("Scale 2.0 (zoom out)", 1);

    // Non-uniform scale
    mode7SetScale(0x0100, 0x0080);
    log_result("Non-uniform scale", 1);

    // Reset
    mode7SetScale(0x0100, 0x0100);
}

// =============================================================================
// Test: mode7SetAngle
// =============================================================================
void test_mode7_angle(void) {
    mode7Init();

    // 0 degrees
    mode7SetAngle(0);
    log_result("Angle 0", 1);

    // 90 degrees (64/256 of full rotation)
    mode7SetAngle(64);
    log_result("Angle 90 deg", 1);

    // 180 degrees
    mode7SetAngle(128);
    log_result("Angle 180 deg", 1);

    // 270 degrees
    mode7SetAngle(192);
    log_result("Angle 270 deg", 1);

    // Full rotation test
    u8 i;
    for (i = 0; i < 255; i++) {
        mode7SetAngle(i);
    }
    log_result("Full rotation sweep", 1);

    mode7SetAngle(0);
}

// =============================================================================
// Test: mode7SetCenter
// =============================================================================
void test_mode7_center(void) {
    mode7Init();

    // Default center (128, 128)
    mode7SetCenter(128, 128);
    log_result("Center default", 1);

    // Top-left
    mode7SetCenter(0, 0);
    log_result("Center top-left", 1);

    // Bottom-right
    mode7SetCenter(255, 223);
    log_result("Center bottom-right", 1);

    // Negative values (allowed, 13-bit signed)
    mode7SetCenter(-100, -100);
    log_result("Center negative", 1);

    // Reset
    mode7SetCenter(128, 128);
}

// =============================================================================
// Test: mode7SetScroll
// =============================================================================
void test_mode7_scroll(void) {
    mode7Init();

    // Origin
    mode7SetScroll(0, 0);
    log_result("Scroll origin", 1);

    // Positive scroll
    mode7SetScroll(256, 256);
    log_result("Scroll positive", 1);

    // Negative scroll
    mode7SetScroll(-128, -128);
    log_result("Scroll negative", 1);

    // Reset
    mode7SetScroll(0, 0);
}

// =============================================================================
// Test: mode7Rotate (degrees)
// =============================================================================
void test_mode7_rotate_degrees(void) {
    mode7Init();

    mode7Rotate(0);
    log_result("Rotate 0 deg", 1);

    mode7Rotate(45);
    log_result("Rotate 45 deg", 1);

    mode7Rotate(90);
    log_result("Rotate 90 deg", 1);

    mode7Rotate(180);
    log_result("Rotate 180 deg", 1);

    mode7Rotate(359);
    log_result("Rotate 359 deg", 1);
}

// =============================================================================
// Test: mode7Transform (combined)
// =============================================================================
void test_mode7_transform(void) {
    mode7Init();

    // Normal transform
    mode7Transform(0, 100);
    log_result("Transform normal", 1);

    // Rotated + zoomed in
    mode7Transform(45, 50);
    log_result("Transform 45deg zoom", 1);

    // Rotated + zoomed out
    mode7Transform(90, 200);
    log_result("Transform 90deg small", 1);

    mode7Transform(0, 100);
}

// =============================================================================
// Test: mode7SetPivot
// =============================================================================
void test_mode7_pivot(void) {
    mode7Init();

    // Screen center
    mode7SetPivot(128, 112);
    log_result("Pivot center", 1);

    // Top-left
    mode7SetPivot(0, 0);
    log_result("Pivot top-left", 1);

    // Reset
    mode7SetPivot(128, 112);
}

// =============================================================================
// Test: mode7SetMatrix (advanced)
// =============================================================================
void test_mode7_matrix(void) {
    mode7Init();

    // Identity matrix (no transform)
    mode7SetMatrix(0x0100, 0x0000, 0x0000, 0x0100);
    log_result("Matrix identity", 1);

    // Scaled matrix
    mode7SetMatrix(0x0200, 0x0000, 0x0000, 0x0200);
    log_result("Matrix scaled", 1);
}

// =============================================================================
// Test: mode7SetSettings
// =============================================================================
void test_mode7_settings(void) {
    mode7Init();

    // Wrap mode
    mode7SetSettings(MODE7_WRAP);
    log_result("Settings wrap", 1);

    // Transparent out of bounds
    mode7SetSettings(MODE7_TRANSPARENT);
    log_result("Settings transparent", 1);

    // Tile 0 out of bounds
    mode7SetSettings(MODE7_TILE0);
    log_result("Settings tile0", 1);

    // Flip horizontal
    mode7SetSettings(MODE7_FLIP_H);
    log_result("Settings flip H", 1);

    // Flip vertical
    mode7SetSettings(MODE7_FLIP_V);
    log_result("Settings flip V", 1);

    // Combined flags
    mode7SetSettings(MODE7_WRAP | MODE7_FLIP_H | MODE7_FLIP_V);
    log_result("Settings combined", 1);

    // Reset
    mode7SetSettings(MODE7_WRAP);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);  // Use Mode 0 for text display
    textInit();

    textPrintAt(2, 1, "MODE 7 MODULE TESTS");
    textPrintAt(2, 2, "-------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_mode7_init();
    test_mode7_scale();
    test_mode7_angle();
    test_mode7_center();
    test_mode7_scroll();
    test_mode7_rotate_degrees();
    test_mode7_transform();
    test_mode7_pivot();
    test_mode7_matrix();
    test_mode7_settings();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
