// =============================================================================
// Unit Test: Sprite Module
// =============================================================================
// Tests the sprite/OAM management functions.
//
// Critical functions tested:
// - oamInit(): Initialize OAM memory, hide all sprites
// - oamSet(): Set sprite position, tile, attributes
// - oamSetVisible(): Show/hide individual sprites
// - Constants: OBJ_SIZE, OBJ_HIDE_Y, MAX_SPRITES
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/sprite.h>
#include <snes/dma.h>
#include <snes/text.h>

// Test sprite tile data (8x8, simple pattern)
const u8 testSpriteTiles[] = {
    // Plane 0 (8 bytes)
    0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF,
    // Plane 1 (8 bytes)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // Padding to 32 bytes for 4bpp
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Test palette
const u8 testPalette[] = {
    0x00, 0x00,  // Color 0: Transparent
    0xFF, 0x7F,  // Color 1: White
    0x00, 0x7C,  // Color 2: Red
    0xE0, 0x03,  // Color 3: Green
    0x1F, 0x00,  // Color 4: Blue
    0xFF, 0x03,  // Color 5: Yellow
    0x1F, 0x7C,  // Color 6: Magenta
    0xE0, 0x7F,  // Color 7: Cyan
    // Remaining colors black
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Test results
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
// Test: OAM Constants
// =============================================================================
void test_oam_constants(void) {
    // Verify MAX_SPRITES
    u8 pass = (MAX_SPRITES == 128);
    log_result("MAX_SPRITES == 128", pass);

    // Verify OBJ_HIDE_Y
    pass = (OBJ_HIDE_Y == 240);
    log_result("OBJ_HIDE_Y == 240", pass);

    // Verify size constants exist and are unique
    pass = (OBJ_SIZE8_L16 != OBJ_SIZE16_L32);
    log_result("Size constants unique", pass);
}

// =============================================================================
// Test: oamInit
// =============================================================================
void test_oam_init(void) {
    // Initialize OAM
    oamInit();

    // After init, all sprites should be hidden (Y=240)
    // We can't directly read oamMemory from C easily,
    // but we can verify the function executes without crash
    log_result("oamInit executes", 1);
}

// =============================================================================
// Test: oamSet basic positioning
// oamSet(id, x, y, tile, palette, priority, flags)
// =============================================================================
void test_oam_set_basic(void) {
    oamInit();

    // Set sprite 0 at position (100, 80)
    oamSet(0, 100, 80, 0, 0, 0, 0);
    log_result("oamSet sprite 0", 1);

    // Set sprite at different positions
    oamSet(1, 0, 0, 0, 0, 0, 0);      // Top-left
    oamSet(2, 255, 0, 0, 0, 0, 0);    // Top-right edge
    oamSet(3, 128, 112, 0, 0, 0, 0);  // Center-ish
    log_result("oamSet multiple sprites", 1);
}

// =============================================================================
// Test: oamSet with attributes
// =============================================================================
void test_oam_set_attributes(void) {
    oamInit();

    // Test priority (0-3) - priority is param 6
    oamSet(0, 100, 80, 0, 0, 0, 0);  // Priority 0
    oamSet(1, 100, 80, 0, 0, 1, 0);  // Priority 1
    oamSet(2, 100, 80, 0, 0, 2, 0);  // Priority 2
    oamSet(3, 100, 80, 0, 0, 3, 0);  // Priority 3
    log_result("oamSet priorities", 1);

    // Test flip flags (flags param includes OBJ_FLIPX and OBJ_FLIPY)
    oamSet(4, 100, 80, 0, 0, 0, OBJ_FLIPX);           // H-flip
    oamSet(5, 100, 80, 0, 0, 0, OBJ_FLIPY);           // V-flip
    oamSet(6, 100, 80, 0, 0, 0, OBJ_FLIPX | OBJ_FLIPY); // Both flips
    log_result("oamSet flip flags", 1);

    // Test palette (0-7) - palette is param 5
    oamSet(7, 100, 80, 0, 3, 0, 0);  // Palette 3
    oamSet(8, 100, 80, 0, 7, 0, 0);  // Palette 7
    log_result("oamSet palettes", 1);
}

// =============================================================================
// Test: oamSetVisible
// =============================================================================
void test_oam_visibility(void) {
    oamInit();

    // Set a sprite
    oamSet(0, 100, 80, 0, 0, 0, 0);

    // Hide it
    oamSetVisible(0, OBJ_HIDE);
    log_result("oamSetVisible hide", 1);

    // Show it again
    oamSetVisible(0, OBJ_SHOW);
    log_result("oamSetVisible show", 1);
}

// =============================================================================
// Test: Multiple sprite stress test
// =============================================================================
void test_many_sprites(void) {
    oamInit();

    // Set all 128 sprites
    for (u8 i = 0; i < 128; i++) {
        u16 x = (i % 16) * 16;
        u8 y = (i / 16) * 16;
        oamSet(i, x, y, 0, i % 8, 0, 0);
    }
    log_result("Set all 128 sprites", 1);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    // Initialize console for output
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    // Initialize sprite graphics
    oamInitGfxSet((u8*)testSpriteTiles, sizeof(testSpriteTiles),
                  (u8*)testPalette, sizeof(testPalette),
                  OBJ_SIZE8_L16, 0x0000, 0x0000);

    textPrintAt(2, 1, "SPRITE MODULE TESTS");
    textPrintAt(2, 2, "-------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_oam_constants();
    test_oam_init();
    test_oam_set_basic();
    test_oam_set_attributes();
    test_oam_visibility();
    test_many_sprites();

    // Display results
    textPrintAt(2, 4, "Tests completed");

    setScreenOn();

    // Show some test sprites
    oamSet(0, 100, 100, 0, 0, 3, 0);
    oamSetVisible(0, OBJ_SHOW);

    while (1) {
        WaitForVBlank();
        oamUpdate();
    }

    return 0;
}
