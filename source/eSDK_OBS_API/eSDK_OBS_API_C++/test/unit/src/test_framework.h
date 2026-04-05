#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <setjmp.h>
#include <stddef.h>

typedef enum {
    TEST_RESULT_PASS = 0,
    TEST_RESULT_FAIL = 1,
    TEST_RESULT_SKIP = 2
} test_result_t;

typedef void (*test_func_t)(void);

typedef struct test_case {
    const char *suite_name;
    const char *test_name;
    test_func_t test_func;
    struct test_case *next;
} test_case_t;

extern int g_test_verbose;
extern const char *g_test_filter;

test_case_t *test_register(const char *suite, const char *name, test_func_t func);
int test_run_all(void);

void test_fail_impl(const char *file, int line, const char *fmt, ...);
void test_skip_impl(const char *file, int line, const char *fmt, ...);

#define TEST(suite, name) \
    static void test_##suite##_##name(void); \
    static void __attribute__((constructor)) register_##suite##_##name(void) \
    { \
        test_register(#suite, #name, test_##suite##_##name); \
    } \
    static void test_##suite##_##name(void)

#define TEST_ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            test_fail_impl(__FILE__, __LINE__, "Expected true: %s", #expr); \
        } \
    } while (0)

#define TEST_ASSERT_FALSE(expr) \
    do { \
        if (expr) { \
            test_fail_impl(__FILE__, __LINE__, "Expected false: %s", #expr); \
        } \
    } while (0)

#define TEST_ASSERT_EQ_INT(expected, actual) \
    do { \
        int _expected = (expected); \
        int _actual = (actual); \
        if (_expected != _actual) { \
            test_fail_impl(__FILE__, __LINE__, "Expected %d, got %d", _expected, _actual); \
        } \
    } while (0)

#define TEST_ASSERT_EQ_SIZE(expected, actual) \
    do { \
        size_t _expected = (expected); \
        size_t _actual = (actual); \
        if (_expected != _actual) { \
            test_fail_impl(__FILE__, __LINE__, "Expected %zu, got %zu", _expected, _actual); \
        } \
    } while (0)

#define TEST_ASSERT_EQ_UINTPTR(expected, actual) \
    do { \
        unsigned long long _expected = (unsigned long long)(expected); \
        unsigned long long _actual = (unsigned long long)(actual); \
        if (_expected != _actual) { \
            test_fail_impl(__FILE__, __LINE__, "Expected 0x%llx, got 0x%llx", _expected, _actual); \
        } \
    } while (0)

#define TEST_ASSERT_EQ_PTR(expected, actual) \
    TEST_ASSERT_EQ_UINTPTR((expected), (actual))

#define TEST_SKIP(reason) \
    test_skip_impl(__FILE__, __LINE__, "%s", (reason))

#endif
