/**
 * @file test_harness.h
 * @brief OpenSNES Test Harness - C API
 *
 * Provides macros and functions for writing automated tests that run
 * on SNES hardware (via emulator) and report results to the test runner.
 *
 * ## Usage
 *
 * ```c
 * #include "test_harness.h"
 *
 * void test_addition(void) {
 *     TEST_ASSERT_EQUAL(4, 2 + 2);
 * }
 *
 * int main(void) {
 *     test_init();
 *     RUN_TEST(test_addition);
 *     test_report();
 *     return 0;
 * }
 * ```
 *
 * ## Memory Layout
 *
 * Test results are written to bank $7F (Work RAM):
 * - $7F0000: Status (0=running, 1=pass, 2=fail)
 * - $7F0001-02: Tests run (u16)
 * - $7F0003-04: Tests passed (u16)
 * - $7F0005-06: Tests failed (u16)
 * - $7F0010-4F: Failure message (64 bytes, null-terminated)
 *
 * @author OpenSNES Team
 * @copyright MIT License
 *
 * Originally inspired by: Unity Test Framework (ThrowTheSwitch)
 */

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <snes.h>

/*============================================================================
 * Test Result Memory Addresses
 *============================================================================*/

/** Test status: 0=running, 1=pass, 2=fail */
#define TEST_STATUS_ADDR     ((volatile u8*)0x7F0000)

/** Number of tests executed */
#define TEST_RUN_COUNT_ADDR  ((volatile u16*)0x7F0001)

/** Number of tests passed */
#define TEST_PASS_COUNT_ADDR ((volatile u16*)0x7F0003)

/** Number of tests failed */
#define TEST_FAIL_COUNT_ADDR ((volatile u16*)0x7F0005)

/** Failure message buffer (64 bytes) */
#define TEST_MESSAGE_ADDR    ((volatile char*)0x7F0010)

/** Status values */
#define TEST_STATUS_RUNNING  0
#define TEST_STATUS_PASS     1
#define TEST_STATUS_FAIL     2

/*============================================================================
 * Test State (internal)
 *============================================================================*/

/** Current test name for error reporting */
extern const char* _test_current_name;

/** Flag set when current test has failed */
extern u8 _test_current_failed;

/*============================================================================
 * Initialization
 *============================================================================*/

/**
 * @brief Initialize the test harness
 *
 * Must be called before running any tests. Initializes SNES hardware
 * and clears test result memory.
 *
 * @code
 * int main(void) {
 *     test_init();
 *     // ... run tests ...
 *     test_report();
 * }
 * @endcode
 */
void test_init(void);

/*============================================================================
 * Test Execution
 *============================================================================*/

/**
 * @brief Run a single test function
 *
 * Executes the test function, tracks pass/fail status, and updates
 * the test counters.
 *
 * @param test_func Pointer to test function (void -> void)
 *
 * @code
 * void test_foo(void) {
 *     TEST_ASSERT(1 == 1);
 * }
 *
 * RUN_TEST(test_foo);
 * @endcode
 */
#define RUN_TEST(test_func) do { \
    _test_current_name = #test_func; \
    _test_current_failed = 0; \
    (*TEST_RUN_COUNT_ADDR)++; \
    test_func(); \
    if (!_test_current_failed) { \
        (*TEST_PASS_COUNT_ADDR)++; \
    } \
} while(0)

/*============================================================================
 * Assertions
 *============================================================================*/

/**
 * @brief Assert that a condition is true
 *
 * @param condition Expression that should evaluate to non-zero
 *
 * @code
 * TEST_ASSERT(value > 0);
 * TEST_ASSERT(ptr != NULL);
 * @endcode
 */
#define TEST_ASSERT(condition) do { \
    if (!(condition)) { \
        _test_fail(__FILE__, __LINE__, #condition); \
    } \
} while(0)

/**
 * @brief Assert that two integers are equal
 *
 * @param expected The expected value
 * @param actual The actual value to test
 *
 * @code
 * TEST_ASSERT_EQUAL(10, calculate_result());
 * @endcode
 */
#define TEST_ASSERT_EQUAL(expected, actual) do { \
    s32 _exp = (s32)(expected); \
    s32 _act = (s32)(actual); \
    if (_exp != _act) { \
        _test_fail_equal(__FILE__, __LINE__, _exp, _act); \
    } \
} while(0)

/**
 * @brief Assert that two unsigned 16-bit values are equal
 *
 * @param expected The expected value
 * @param actual The actual value to test
 */
#define TEST_ASSERT_EQUAL_U16(expected, actual) do { \
    u16 _exp = (u16)(expected); \
    u16 _act = (u16)(actual); \
    if (_exp != _act) { \
        _test_fail_equal(__FILE__, __LINE__, (s32)_exp, (s32)_act); \
    } \
} while(0)

/**
 * @brief Assert that two pointers are equal
 *
 * @param expected The expected pointer
 * @param actual The actual pointer to test
 */
#define TEST_ASSERT_EQUAL_PTR(expected, actual) do { \
    void* _exp = (void*)(expected); \
    void* _act = (void*)(actual); \
    if (_exp != _act) { \
        _test_fail(__FILE__, __LINE__, "pointers not equal"); \
    } \
} while(0)

/**
 * @brief Assert that a pointer is NULL
 *
 * @param ptr The pointer to test
 */
#define TEST_ASSERT_NULL(ptr) do { \
    if ((ptr) != ((void*)0)) { \
        _test_fail(__FILE__, __LINE__, "expected NULL"); \
    } \
} while(0)

/**
 * @brief Assert that a pointer is not NULL
 *
 * @param ptr The pointer to test
 */
#define TEST_ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == ((void*)0)) { \
        _test_fail(__FILE__, __LINE__, "unexpected NULL"); \
    } \
} while(0)

/**
 * @brief Assert that two memory regions are equal
 *
 * @param expected Pointer to expected data
 * @param actual Pointer to actual data
 * @param size Number of bytes to compare
 */
#define TEST_ASSERT_MEM_EQUAL(expected, actual, size) do { \
    if (!_test_mem_equal((expected), (actual), (size))) { \
        _test_fail(__FILE__, __LINE__, "memory not equal"); \
    } \
} while(0)

/**
 * @brief Unconditionally fail the current test
 *
 * @param message Failure message
 *
 * @code
 * if (error_condition) {
 *     TEST_FAIL("Should not reach here");
 * }
 * @endcode
 */
#define TEST_FAIL(message) _test_fail(__FILE__, __LINE__, message)

/**
 * @brief Mark current test as passed (explicit)
 *
 * Normally not needed - tests pass if no assertions fail.
 */
#define TEST_PASS() /* no-op, test passes if no failures */

/*============================================================================
 * Reporting
 *============================================================================*/

/**
 * @brief Finalize tests and report results
 *
 * Writes final status to test result memory and enters infinite loop.
 * The Mesen2 test runner reads results from memory.
 *
 * @code
 * int main(void) {
 *     test_init();
 *     RUN_TEST(test_a);
 *     RUN_TEST(test_b);
 *     test_report();  // Never returns
 * }
 * @endcode
 */
void test_report(void);

/*============================================================================
 * Internal Functions (do not call directly)
 *============================================================================*/

/** @internal Record a test failure */
void _test_fail(const char* file, u16 line, const char* message);

/** @internal Record a test failure with expected/actual values */
void _test_fail_equal(const char* file, u16 line, s32 expected, s32 actual);

/** @internal Compare memory regions */
u8 _test_mem_equal(const void* expected, const void* actual, u16 size);

#endif /* TEST_HARNESS_H */
