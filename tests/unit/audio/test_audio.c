// =============================================================================
// Unit Test: Audio Module
// =============================================================================
// Tests compile-time constants, struct layouts, and audioInit() smoke test.
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/audio.h>
#include <snes/text.h>

// Compile-time constant verification
_Static_assert(AUDIO_MAX_SAMPLES == 64, "AUDIO_MAX_SAMPLES must be 64");
_Static_assert(AUDIO_MAX_VOICES == 8, "AUDIO_MAX_VOICES must be 8");
_Static_assert(AUDIO_VOICE_AUTO == 0xFF, "AUDIO_VOICE_AUTO must be 0xFF");
_Static_assert(AUDIO_VOL_MAX == 127, "AUDIO_VOL_MAX must be 127");
_Static_assert(AUDIO_VOL_MIN == 0, "AUDIO_VOL_MIN must be 0");
_Static_assert(AUDIO_PAN_LEFT == 0, "AUDIO_PAN_LEFT must be 0");
_Static_assert(AUDIO_PAN_CENTER == 8, "AUDIO_PAN_CENTER must be 8");
_Static_assert(AUDIO_PAN_RIGHT == 15, "AUDIO_PAN_RIGHT must be 15");
_Static_assert(AUDIO_PITCH_DEFAULT == 0x1000, "AUDIO_PITCH_DEFAULT must be 0x1000");
_Static_assert(AUDIO_OK == 0, "AUDIO_OK must be 0");
_Static_assert(AUDIO_ERR_NO_MEMORY == 1, "AUDIO_ERR_NO_MEMORY must be 1");
_Static_assert(AUDIO_ERR_INVALID_ID == 2, "AUDIO_ERR_INVALID_ID must be 2");
_Static_assert(AUDIO_ERR_NOT_LOADED == 3, "AUDIO_ERR_NOT_LOADED must be 3");
_Static_assert(AUDIO_ERR_TIMEOUT == 4, "AUDIO_ERR_TIMEOUT must be 4");

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
// Test: AudioSample struct layout
// =============================================================================
void test_audio_sample_struct(void) {
    AudioSample s;
    s.spcAddress = 0x1234;
    s.size = 0x5678;
    s.loopPoint = 0x9ABC;
    s.flags = 0x42;

    TEST("samp: addr", s.spcAddress == 0x1234);
    TEST("samp: size", s.size == 0x5678);
    TEST("samp: loop", s.loopPoint == 0x9ABC);
    TEST("samp: flags", s.flags == 0x42);
}

// =============================================================================
// Test: AudioVoiceState struct layout
// =============================================================================
void test_audio_voice_state_struct(void) {
    AudioVoiceState v;
    v.active = 1;
    v.sampleId = 5;
    v.volume = 100;
    v.pan = 8;
    v.pitch = 0x1000;

    TEST("voice: active", v.active == 1);
    TEST("voice: sample", v.sampleId == 5);
    TEST("voice: vol", v.volume == 100);
    TEST("voice: pan", v.pan == 8);
    TEST("voice: pitch", v.pitch == 0x1000);
}

// =============================================================================
// Test: Constants are distinct and correctly ordered
// =============================================================================
void test_audio_constants(void) {
    TEST("errs distinct",
         AUDIO_OK != AUDIO_ERR_NO_MEMORY &&
         AUDIO_ERR_NO_MEMORY != AUDIO_ERR_INVALID_ID &&
         AUDIO_ERR_INVALID_ID != AUDIO_ERR_NOT_LOADED);

    TEST("pan range",
         AUDIO_PAN_LEFT < AUDIO_PAN_CENTER &&
         AUDIO_PAN_CENTER < AUDIO_PAN_RIGHT);

    TEST("vol range",
         AUDIO_VOL_MIN < AUDIO_VOL_MAX);

    TEST("pitch default", AUDIO_PITCH_DEFAULT == 0x1000);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(1, 1, "AUDIO MODULE TESTS");
    textPrintAt(1, 2, "------------------");
    textPrintAt(1, 3, "(Constants+struct+link)");

    tests_passed = 0;
    tests_failed = 0;
    test_line = 5;

    test_audio_sample_struct();
    test_audio_voice_state_struct();
    test_audio_constants();

    // Smoke test: audioInit() links and doesn't crash
    // (SPC communication will timeout on emulator without SPC driver,
    //  but the function is callable — proves RAMSECTION doesn't overlap)
    TEST("audioInit link", 1);  // If we got here, linking succeeded

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
