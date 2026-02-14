// =============================================================================
// Unit Test: Sprite Module
// =============================================================================
// Tests the sprite/OAM management functions by verifying oamMemory[] contents
// after function calls.
//
// OAM buffer layout (oamMemory[], 544 bytes):
//   Bytes 0-511: 4 bytes per sprite (128 sprites)
//     offset+0: X position (low 8 bits)
//     offset+1: Y position
//     offset+2: Tile number (low 8 bits)
//     offset+3: Attributes (vhoopppc)
//   Bytes 512-543: High table (2 bits per sprite)
//     bit 0 of each pair: X high bit (bit 8)
//     bit 1 of each pair: size select (0=small, 1=large)
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/sprite.h>
#include <snes/dma.h>
#include <snes/text.h>

// Access the raw OAM hardware buffer from C
extern u8 oamMemory[];

// Test sprite tile data (8x8, 4bpp = 32 bytes)
const u8 testSpriteTiles[] = {
    0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const u8 testPalette[] = {
    0x00, 0x00, 0xFF, 0x7F, 0x00, 0x7C, 0xE0, 0x03,
    0x1F, 0x00, 0xFF, 0x03, 0x1F, 0x7C, 0xE0, 0x7F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

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
// Test: OAM Constants
// =============================================================================
void test_oam_constants(void) {
    TEST("MAX_SPRITES==128", MAX_SPRITES == 128);
    TEST("OBJ_HIDE_Y==240", OBJ_HIDE_Y == 240);
    TEST("SizeConst unique", OBJ_SIZE8_L16 != OBJ_SIZE16_L32);
    TEST("OBJ_SHOW==1", OBJ_SHOW == 1);
    TEST("OBJ_HIDE==0", OBJ_HIDE == 0);
}

// =============================================================================
// Test: oamClear — all sprites hidden
// =============================================================================
void test_oam_clear(void) {
    oamClear();

    // After clear, every sprite should have Y=240 (OBJ_HIDE_Y)
    u8 all_hidden = 1;
    u8 i;
    for (i = 0; i < 128; i++) {
        u16 off = (u16)i << 2;
        if (oamMemory[off + 1] != OBJ_HIDE_Y) {
            all_hidden = 0;
            break;
        }
    }
    TEST("clear: all Y=240", all_hidden);

    // High table: all X high bits set (0x55 pattern = 01010101)
    u8 hitable_ok = 1;
    for (i = 0; i < 32; i++) {
        if (oamMemory[512 + i] != 0x55) {
            hitable_ok = 0;
            break;
        }
    }
    TEST("clear: hitbl=0x55", hitable_ok);
}

// =============================================================================
// Test: oamSet basic — verify position bytes
// =============================================================================
void test_oam_set_position(void) {
    oamClear();

    // Set sprite 0 at (100, 80), tile=5, pal=0, prio=0, no flip
    oamSet(0, 100, 80, 5, 0, 0, 0);

    TEST("set: X lo=100", oamMemory[0] == 100);
    TEST("set: Y=80", oamMemory[1] == 80);
    TEST("set: tile=5", oamMemory[2] == 5);
    // attr = vhoopppc: no flip, prio=0, pal=0, tile hi=0 → 0x00
    TEST("set: attr=0x00", oamMemory[3] == 0x00);

    // X high bit should be clear (x=100 < 256)
    // High table byte 128 (512+0), sprite 0 = bit 0
    u8 ht = oamMemory[512];
    TEST("set: Xhi clear", (ht & 0x01) == 0);
}

// =============================================================================
// Test: oamSet with X > 255 — high bit set
// =============================================================================
void test_oam_set_xhi(void) {
    oamClear();

    // Set sprite 0 at X=300 (0x12C), Y=50
    oamSet(0, 300, 50, 0, 0, 0, 0);

    // X low 8 bits = 300 & 0xFF = 0x2C = 44
    TEST("xhi: X lo=44", oamMemory[0] == 44);
    TEST("xhi: Y=50", oamMemory[1] == 50);

    // X high bit should be set
    u8 ht = oamMemory[512];
    TEST("xhi: Xhi set", (ht & 0x01) == 1);
}

// =============================================================================
// Test: oamSet attributes — priority, palette, flip, tile high bit
// =============================================================================
void test_oam_set_attributes(void) {
    oamClear();

    // Priority 2, palette 3, no flip, tile=0
    // attr = vhoopppc = 00_10_011_0 = 0x26
    oamSet(0, 50, 50, 0, 3, 2, 0);
    TEST("attr: p2 pal3", oamMemory[3] == 0x26);

    // H-flip + V-flip, priority 1, palette 5, tile=256 (high bit=1)
    // attr = 11_01_101_1 = 0xDB
    oamSet(1, 50, 50, 256, 5, 1, OBJ_FLIPX | OBJ_FLIPY);
    TEST("attr: flip+t256", oamMemory[7] == 0xDB);

    // Just H-flip, prio 0, pal 0, tile 0
    // attr = 01_00_000_0 = 0x40
    oamSet(2, 50, 50, 0, 0, 0, OBJ_FLIPX);
    TEST("attr: Hflip", oamMemory[11] == 0x40);

    // Just V-flip
    // attr = 10_00_000_0 = 0x80
    oamSet(3, 50, 50, 0, 0, 0, OBJ_FLIPY);
    TEST("attr: Vflip", oamMemory[15] == 0x80);
}

// =============================================================================
// Test: oamHide — sprite hidden at Y=240 with X high bit set
// =============================================================================
void test_oam_hide(void) {
    oamClear();

    // First set sprite 5 at a visible position
    oamSet(5, 100, 80, 0, 0, 0, 0);
    TEST("hide: pre Y=80", oamMemory[5 * 4 + 1] == 80);

    // Now hide it
    oamHide(5);
    TEST("hide: X lo=0", oamMemory[5 * 4 + 0] == 0);
    TEST("hide: Y=240", oamMemory[5 * 4 + 1] == OBJ_HIDE_Y);

    // X high bit should be set (sprite 5: byte 512+1, bit 2)
    u8 ht = oamMemory[512 + 1]; // sprites 4-7
    TEST("hide: Xhi set", (ht & 0x04) != 0); // sprite 5 = bit 2
}

// =============================================================================
// Test: oamSetVisible(id, OBJ_HIDE) — same as oamHide
// =============================================================================
void test_oam_set_visible(void) {
    oamClear();

    oamSet(0, 100, 80, 0, 0, 0, 0);
    TEST("vis: pre Y=80", oamMemory[1] == 80);

    oamSetVisible(0, OBJ_HIDE);
    TEST("vis: hide Y=240", oamMemory[1] == OBJ_HIDE_Y);

    // OBJ_SHOW does NOT restore Y — documented behavior
    oamSetVisible(0, OBJ_SHOW);
    TEST("vis: show noop", oamMemory[1] == OBJ_HIDE_Y);
}

// =============================================================================
// Test: oamSetX / oamSetY / oamSetXY
// =============================================================================
void test_oam_setxy(void) {
    oamClear();
    oamSet(0, 0, 0, 0, 0, 0, 0);

    oamSetX(0, 200);
    TEST("setX: X=200", oamMemory[0] == 200);

    oamSetY(0, 150);
    TEST("setY: Y=150", oamMemory[1] == 150);

    oamSetXY(0, 75, 120);
    TEST("setXY: X=75", oamMemory[0] == 75);
    TEST("setXY: Y=120", oamMemory[1] == 120);
}

// =============================================================================
// Test: oamSetTile
// =============================================================================
void test_oam_set_tile(void) {
    oamClear();
    oamSet(0, 50, 50, 0, 0, 0, 0);

    oamSetTile(0, 42);
    TEST("tile: lo=42", oamMemory[2] == 42);
}

// =============================================================================
// Test: oamSetSize — large/small bit in high table
// =============================================================================
void test_oam_set_size(void) {
    oamClear();

    // Set sprite 0 to large size
    oamSetSize(0, 1);
    // Size is bit 1 of the 2-bit pair for sprite 0 in high table
    u8 ht = oamMemory[512];
    TEST("size: large", (ht & 0x02) != 0);

    // Set back to small
    oamSetSize(0, 0);
    ht = oamMemory[512];
    TEST("size: small", (ht & 0x02) == 0);
}

// =============================================================================
// Test: Multiple sprites — verify different slots don't interfere
// =============================================================================
void test_multi_sprite(void) {
    oamClear();

    oamSet(0, 10, 20, 1, 0, 0, 0);
    oamSet(1, 30, 40, 2, 1, 0, 0);
    oamSet(2, 50, 60, 3, 2, 0, 0);

    // Verify sprite 0
    TEST("multi: s0 X=10", oamMemory[0] == 10);
    TEST("multi: s0 Y=20", oamMemory[1] == 20);
    TEST("multi: s0 t=1", oamMemory[2] == 1);

    // Verify sprite 1
    TEST("multi: s1 X=30", oamMemory[4] == 30);
    TEST("multi: s1 Y=40", oamMemory[5] == 40);
    TEST("multi: s1 t=2", oamMemory[6] == 2);

    // Verify sprite 2
    TEST("multi: s2 X=50", oamMemory[8] == 50);
    TEST("multi: s2 Y=60", oamMemory[9] == 60);
    TEST("multi: s2 t=3", oamMemory[10] == 3);
}

// =============================================================================
// Test: Stress test — set all 128 sprites, verify a sample
// =============================================================================
void test_all_sprites(void) {
    oamClear();

    u8 i;
    for (i = 0; i < 128; i++) {
        oamSet(i, i, i + 10, 0, i % 8, 0, 0);
    }

    // Spot-check a few sprites
    TEST("all: s0 X=0", oamMemory[0] == 0);
    TEST("all: s0 Y=10", oamMemory[1] == 10);
    TEST("all: s64 X=64", oamMemory[64 * 4] == 64);
    TEST("all: s64 Y=74", oamMemory[64 * 4 + 1] == 74);
    TEST("all: s127 X=127", oamMemory[127 * 4] == 127);
    TEST("all: s127 Y=137", oamMemory[127 * 4 + 1] == 137);
}

// =============================================================================
// Test: oambuffer[] (dynamic sprite engine) structure layout
// =============================================================================
void test_oambuffer_struct(void) {
    // Verify structure size is 16 bytes (PVSnesLib compatible)
    TEST("t_sprites=16B", sizeof(t_sprites) == 16);

    // Verify field offsets via direct write + read
    oambuffer[0].oamx = 123;
    oambuffer[0].oamy = 45;
    oambuffer[0].oamframeid = 7;
    oambuffer[0].oamattribute = 0x2A;
    oambuffer[0].oamrefresh = 1;

    TEST("buf: oamx=123", oambuffer[0].oamx == 123);
    TEST("buf: oamy=45", oambuffer[0].oamy == 45);
    TEST("buf: frameid=7", oambuffer[0].oamframeid == 7);
    TEST("buf: attr=0x2A", oambuffer[0].oamattribute == 0x2A);
    TEST("buf: refresh=1", oambuffer[0].oamrefresh == 1);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    // Initialize sprite graphics
    oamInitGfxSet((u8*)testSpriteTiles, sizeof(testSpriteTiles),
                  (u8*)testPalette, sizeof(testPalette),
                  OBJ_SIZE8_L16, 0x0000, 0x0000);

    textPrintAt(1, 1, "SPRITE MODULE TESTS");
    textPrintAt(1, 2, "-------------------");

    tests_passed = 0;
    tests_failed = 0;
    test_line = 4;  // first line for failure messages

    // Run all tests
    test_oam_constants();
    test_oam_clear();
    test_oam_set_position();
    test_oam_set_xhi();
    test_oam_set_attributes();
    test_oam_hide();
    test_oam_set_visible();
    test_oam_setxy();
    test_oam_set_tile();
    test_oam_set_size();
    test_multi_sprite();
    test_all_sprites();
    test_oambuffer_struct();

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
