/**
 * @file test_harness.c
 * @brief OpenSNES Test Harness - Implementation
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include "test_harness.h"

/*============================================================================
 * Test State
 *============================================================================*/

const char* _test_current_name = "";
u8 _test_current_failed = 0;

/*============================================================================
 * String Helpers
 *============================================================================*/

static void _strcpy_safe(volatile char* dest, const char* src, u16 max) {
    u16 i;
    for (i = 0; i < max - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static u16 _strlen(const char* s) {
    u16 len = 0;
    while (s[len] != '\0') len++;
    return len;
}

/*============================================================================
 * Initialization
 *============================================================================*/

void test_init(void) {
    /* Initialize SNES hardware */
    consoleInit();

    /* Clear test result memory */
    *TEST_STATUS_ADDR = TEST_STATUS_RUNNING;
    *TEST_RUN_COUNT_ADDR = 0;
    *TEST_PASS_COUNT_ADDR = 0;
    *TEST_FAIL_COUNT_ADDR = 0;

    /* Clear message buffer */
    u8 i;
    for (i = 0; i < 64; i++) {
        TEST_MESSAGE_ADDR[i] = 0;
    }
}

/*============================================================================
 * Failure Reporting
 *============================================================================*/

void _test_fail(const char* file, u16 line, const char* message) {
    if (!_test_current_failed) {
        _test_current_failed = 1;
        (*TEST_FAIL_COUNT_ADDR)++;

        /* Build failure message: "test_name: message" */
        _strcpy_safe(TEST_MESSAGE_ADDR, _test_current_name, 32);

        u16 len = _strlen(_test_current_name);
        if (len < 30) {
            TEST_MESSAGE_ADDR[len] = ':';
            TEST_MESSAGE_ADDR[len + 1] = ' ';
            _strcpy_safe(TEST_MESSAGE_ADDR + len + 2, message, 64 - len - 2);
        }
    }
}

void _test_fail_equal(const char* file, u16 line, s32 expected, s32 actual) {
    /* For now, just report generic failure */
    /* TODO: Format expected vs actual into message */
    _test_fail(file, line, "values not equal");
}

/*============================================================================
 * Memory Comparison
 *============================================================================*/

u8 _test_mem_equal(const void* expected, const void* actual, u16 size) {
    const u8* exp = (const u8*)expected;
    const u8* act = (const u8*)actual;
    u16 i;

    for (i = 0; i < size; i++) {
        if (exp[i] != act[i]) {
            return 0;  /* Not equal */
        }
    }
    return 1;  /* Equal */
}

/*============================================================================
 * Reporting
 *============================================================================*/

void test_report(void) {
    /* Set final status based on results */
    if (*TEST_FAIL_COUNT_ADDR > 0) {
        *TEST_STATUS_ADDR = TEST_STATUS_FAIL;
    } else {
        *TEST_STATUS_ADDR = TEST_STATUS_PASS;
    }

    /* Infinite loop - emulator reads results from memory */
    while (1) {
        WaitForVBlank();
    }
}
