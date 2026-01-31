/**
 * @file test_animation.c
 * @brief Unit tests for animation system
 */

#include <snes.h>
#include <snes/animation.h>
#include <snes/console.h>
#include <snes/text.h>

static u8 tests_passed;
static u8 tests_failed;

#define TEST(name, condition) do { \
    if (condition) { \
        tests_passed++; \
        textPrintAt(0, tests_passed + tests_failed, "PASS: " name); \
    } else { \
        tests_failed++; \
        textPrintAt(0, tests_passed + tests_failed, "FAIL: " name); \
    } \
} while(0)

// Test animation frames
u8 testFrames[] = { 10, 20, 30, 40 };

Animation testAnim = {
    .frames = testFrames,
    .frameCount = 4,
    .frameDelay = 2,  // 2 frames per animation frame
    .loop = 1
};

u8 oneshotFrames[] = { 5, 6, 7 };

Animation oneshotAnim = {
    .frames = oneshotFrames,
    .frameCount = 3,
    .frameDelay = 1,
    .loop = 0  // One-shot
};

void test_anim_init(void) {
    animInit(0, &testAnim);

    // After init, animation should be stopped
    TEST("init_stopped", animGetState(0) == ANIM_STATE_STOPPED);
    TEST("init_frame0", animGetFrame(0) == 10);  // First frame
}

void test_anim_play(void) {
    animInit(0, &testAnim);
    animPlay(0);

    TEST("play_state", animGetState(0) == ANIM_STATE_PLAYING);
    TEST("play_playing", animIsPlaying(0) == 1);
    TEST("play_frame0", animGetFrame(0) == 10);

    // After one update (delay=2, so still frame 0)
    animUpdate();
    TEST("play_frame0_1", animGetFrame(0) == 10);

    // After second update, should advance
    animUpdate();
    TEST("play_frame1", animGetFrame(0) == 20);
}

void test_anim_loop(void) {
    animInit(0, &testAnim);
    animPlay(0);

    // Advance through all frames (4 frames * 2 delay = 8 updates)
    for (u8 i = 0; i < 8; i++) {
        animUpdate();
    }

    // Should loop back to first frame
    TEST("loop_wrap", animGetFrame(0) == 10);
    TEST("loop_playing", animIsPlaying(0) == 1);
}

void test_anim_oneshot(void) {
    animInit(1, &oneshotAnim);
    animPlay(1);

    // Frame 0
    TEST("oneshot_f0", animGetFrame(1) == 5);

    // Advance through all frames
    animUpdate();  // Frame 1
    TEST("oneshot_f1", animGetFrame(1) == 6);

    animUpdate();  // Frame 2
    TEST("oneshot_f2", animGetFrame(1) == 7);

    animUpdate();  // Should finish
    TEST("oneshot_done", animIsFinished(1) == 1);
    TEST("oneshot_last", animGetFrame(1) == 7);  // Stays on last
}

void test_anim_pause_resume(void) {
    animInit(0, &testAnim);
    animPlay(0);

    animUpdate();  // Still frame 0 (delay 2)

    animPause(0);
    TEST("pause_state", animGetState(0) == ANIM_STATE_PAUSED);

    // Updates should not advance
    animUpdate();
    animUpdate();
    animUpdate();
    TEST("pause_frozen", animGetFrame(0) == 10);

    // Resume
    animResume(0);
    TEST("resume_state", animGetState(0) == ANIM_STATE_PLAYING);

    animUpdate();  // Now should advance
    TEST("resume_advance", animGetFrame(0) == 20);
}

void test_anim_stop(void) {
    animInit(0, &testAnim);
    animPlay(0);

    // Advance to frame 2
    for (u8 i = 0; i < 4; i++) animUpdate();
    TEST("stop_at2", animGetFrame(0) == 30);

    animStop(0);
    TEST("stop_state", animGetState(0) == ANIM_STATE_STOPPED);
    TEST("stop_reset", animGetFrame(0) == 10);  // Reset to first
}

void test_anim_speed(void) {
    animInit(0, &testAnim);
    animPlay(0);

    // Default delay is 2
    animUpdate();
    TEST("speed_delay2_1", animGetFrame(0) == 10);
    animUpdate();
    TEST("speed_delay2_2", animGetFrame(0) == 20);

    // Change to faster (delay 1)
    animSetSpeed(0, 1);
    animUpdate();
    TEST("speed_delay1", animGetFrame(0) == 30);
}

void main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();
    setScreenOn();

    textPrintAt(0, 0, "=== Animation Tests ===");

    tests_passed = 0;
    tests_failed = 0;

    test_anim_init();
    test_anim_play();
    test_anim_loop();
    test_anim_oneshot();
    test_anim_pause_resume();
    test_anim_stop();
    test_anim_speed();

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
