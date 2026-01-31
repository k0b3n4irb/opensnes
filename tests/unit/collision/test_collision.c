/**
 * @file test_collision.c
 * @brief Unit tests for collision detection functions
 *
 * Tests compile and run on SNES to verify collision logic.
 * Results displayed via console output.
 */

#include <snes.h>
#include <snes/collision.h>
#include <snes/console.h>
#include <snes/text.h>

// Test counters
static u8 tests_passed;
static u8 tests_failed;

// Simple test assertion
#define TEST(name, condition) do { \
    if (condition) { \
        tests_passed++; \
        textPrintAt(0, tests_passed + tests_failed, "PASS: " name); \
    } else { \
        tests_failed++; \
        textPrintAt(0, tests_passed + tests_failed, "FAIL: " name); \
    } \
} while(0)

void test_rect_collision(void) {
    Rect a, b;

    // Test: No collision (separated horizontally)
    rectInit(&a, 0, 0, 16, 16);
    rectInit(&b, 32, 0, 16, 16);
    TEST("no_overlap_horiz", collideRect(&a, &b) == 0);

    // Test: No collision (separated vertically)
    rectInit(&a, 0, 0, 16, 16);
    rectInit(&b, 0, 32, 16, 16);
    TEST("no_overlap_vert", collideRect(&a, &b) == 0);

    // Test: Overlap (partial)
    rectInit(&a, 0, 0, 16, 16);
    rectInit(&b, 8, 8, 16, 16);
    TEST("partial_overlap", collideRect(&a, &b) == 1);

    // Test: Full containment
    rectInit(&a, 0, 0, 32, 32);
    rectInit(&b, 8, 8, 8, 8);
    TEST("containment", collideRect(&a, &b) == 1);

    // Test: Edge touching (should NOT collide - edge case)
    rectInit(&a, 0, 0, 16, 16);
    rectInit(&b, 16, 0, 16, 16);
    TEST("edge_touch", collideRect(&a, &b) == 0);

    // Test: Same position
    rectInit(&a, 50, 50, 16, 16);
    rectInit(&b, 50, 50, 16, 16);
    TEST("same_position", collideRect(&a, &b) == 1);
}

void test_point_collision(void) {
    Rect r;
    rectInit(&r, 100, 100, 32, 32);

    // Test: Point inside
    TEST("point_inside", collidePoint(110, 110, &r) == 1);

    // Test: Point outside (left)
    TEST("point_left", collidePoint(50, 110, &r) == 0);

    // Test: Point outside (right)
    TEST("point_right", collidePoint(150, 110, &r) == 0);

    // Test: Point outside (above)
    TEST("point_above", collidePoint(110, 50, &r) == 0);

    // Test: Point outside (below)
    TEST("point_below", collidePoint(110, 150, &r) == 0);

    // Test: Point on edge (should be inside)
    TEST("point_edge", collidePoint(100, 100, &r) == 1);
}

void test_tile_collision(void) {
    // 8x4 tilemap (8 tiles wide, 4 tiles tall)
    // 0 = empty, 1 = solid
    u8 tilemap[32] = {
        0, 0, 0, 0, 0, 0, 0, 0,  // Row 0: all empty
        0, 1, 1, 0, 0, 0, 0, 0,  // Row 1: platform
        0, 0, 0, 0, 0, 1, 0, 0,  // Row 2: block
        1, 1, 1, 1, 1, 1, 1, 1   // Row 3: floor
    };

    // Test: Empty tile (row 0)
    TEST("tile_empty", collideTile(4, 4, tilemap, 8) == 0);

    // Test: Solid tile (row 1, tile 1)
    TEST("tile_solid", collideTile(8, 8, tilemap, 8) == 1);

    // Test: Floor (row 3)
    TEST("tile_floor", collideTile(32, 24, tilemap, 8) == 1);

    // Test: Edge of solid tile
    TEST("tile_edge", collideTile(8, 8, tilemap, 8) == 1);
}

void test_rect_helpers(void) {
    Rect r;
    s16 cx, cy;

    // Test rectInit
    rectInit(&r, 10, 20, 30, 40);
    TEST("rectInit_x", r.x == 10);
    TEST("rectInit_y", r.y == 20);
    TEST("rectInit_w", r.width == 30);
    TEST("rectInit_h", r.height == 40);

    // Test rectSetPos
    rectSetPos(&r, 50, 60);
    TEST("rectSetPos", r.x == 50 && r.y == 60);

    // Test rectGetCenter
    rectInit(&r, 0, 0, 32, 32);
    rectGetCenter(&r, &cx, &cy);
    TEST("rectGetCenter", cx == 16 && cy == 16);
}

void main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();
    setScreenOn();

    textPrintAt(0, 0, "=== Collision Tests ===");

    tests_passed = 0;
    tests_failed = 0;

    test_rect_collision();
    test_point_collision();
    test_tile_collision();
    test_rect_helpers();

    // Summary
    textPrintAt(0, 26, "--------------------");
    if (tests_failed == 0) {
        textPrintAt(0, 27, "ALL TESTS PASSED!");
    } else {
        textPrintAt(0, 27, "SOME TESTS FAILED");
    }

    while (1) {
        WaitForVBlank();
    }
}
