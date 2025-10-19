#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// Test constants for the old framework - avoid conflicts with Common.h
#ifndef TEST_START
#define TEST_START "🧪 STARTING: "
#endif
#ifndef TEST_PASS
#define TEST_PASS "✅ PASSED: "
#endif
#ifndef TEST_FAIL
#define TEST_FAIL "❌ FAILED: "
#endif
#ifndef TEST_OK
#define TEST_OK "  ✓ "
#endif
#ifndef TEST_ERR
#define TEST_ERR "  ✗ "
#endif

#define TEST_EQUAL_EPSILON 0.001f
#define TEST_ERROR_DELTA_PERCENT 5.0f

// Old test framework structure
typedef struct {
    const char* name;
    bool passes;
} test_t;

// Old test framework functions
test_t *test_start(const char *name);
void test_end(test_t *t);
void test_assert(test_t *t, const char *pass_message, const char *fail_message, bool passes);
void test_assert_equal(test_t *t, const char *a_label, const char *b_label, float a, float b);
void test_assert_within_error(test_t *t, const char *actual_label, const char *expected_label, float actual, float expected);

// New test framework globals (for compatibility)
extern int tests_run;
extern int tests_passed;
extern int tests_failed;
extern bool verbose_output;

// New test framework macros (for new tests)
#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            if (verbose_output) printf("    ✅ PASS: %s\n", message); \
        } else { \
            tests_failed++; \
            printf("    ❌ FAIL: %s (line %d)\n", message, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            if (verbose_output) printf("    ✅ PASS: %s\n", message); \
        } else { \
            tests_failed++; \
            printf("    ❌ FAIL: %s: expected %d, got %d (line %d)\n", message, expected, actual, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(expected, actual, tolerance, message) \
    do { \
        tests_run++; \
        if (fabs((expected) - (actual)) < (tolerance)) { \
            tests_passed++; \
            if (verbose_output) printf("    ✅ PASS: %s\n", message); \
        } else { \
            tests_failed++; \
            printf("    ❌ FAIL: %s: expected %.3f, got %.3f (line %d)\n", message, expected, actual, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_STR_EQ(expected, actual, message) \
    do { \
        tests_run++; \
        if (strcmp(expected, actual) == 0) { \
            tests_passed++; \
            if (verbose_output) printf("    ✅ PASS: %s\n", message); \
        } else { \
            tests_failed++; \
            printf("    ❌ FAIL: %s: expected '%s', got '%s' (line %d)\n", message, expected, actual, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr, message) \
    TEST_ASSERT((ptr) == NULL, message)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != NULL, message)

// Test suite macros for new framework
#define TEST_SUITE_START(name) \
    do { \
        printf("\n"); \
        printf("🧪 ================================\n"); \
        printf("🧪 RUNNING: %s Test Suite\n", name); \
        printf("🧪 ================================\n"); \
        tests_run = tests_passed = tests_failed = 0; \
        verbose_output = false; \
    } while(0)

#define TEST_SUITE_END() \
    do { \
        printf("🧪 ================================\n"); \
        printf("🧪 RESULTS: %d passed, %d failed, %d total\n", tests_passed, tests_failed, tests_run); \
        if (tests_failed > 0) { \
            printf("🧪 STATUS: ❌ FAILED\n"); \
            printf("🧪 ================================\n"); \
            exit(EXIT_FAILURE); \
        } else { \
            printf("🧪 STATUS: ✅ PASSED\n"); \
            printf("🧪 ================================\n"); \
        } \
    } while(0)

// Silent test running
#define TEST_SILENT_START() \
    do { \
        freopen("/dev/null", "w", stdout); \
    } while(0)

#define TEST_SILENT_END() \
    do { \
        freopen("/dev/tty", "w", stdout); \
    } while(0)

// Test utilities
void test_init(void);
void test_cleanup(void);
void test_set_verbose(bool verbose);

#endif // TEST_H
