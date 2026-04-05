#include "test_framework.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static test_case_t *g_registry = NULL;
static jmp_buf g_test_jump;
static int g_jump_active = 0;
static test_result_t g_current_result = TEST_RESULT_PASS;
static char g_current_message[512];

int g_test_verbose = 0;
const char *g_test_filter = NULL;

test_case_t *test_register(const char *suite, const char *name, test_func_t func)
{
    test_case_t *test = (test_case_t *)calloc(1, sizeof(test_case_t));
    if (test == NULL) {
        fprintf(stderr, "failed to allocate test case\n");
        exit(1);
    }

    test->suite_name = suite;
    test->test_name = name;
    test->test_func = func;
    test->next = g_registry;
    g_registry = test;
    return test;
}

static int matches_filter(const test_case_t *test)
{
    if (g_test_filter == NULL || g_test_filter[0] == '\0') {
        return 1;
    }

    return strstr(test->suite_name, g_test_filter) != NULL ||
           strstr(test->test_name, g_test_filter) != NULL;
}

static void set_message(const char *file, int line, const char *fmt, va_list args)
{
    int written = snprintf(g_current_message, sizeof(g_current_message), "%s:%d: ", file, line);
    if (written < 0) {
        g_current_message[0] = '\0';
        return;
    }

    vsnprintf(g_current_message + written, sizeof(g_current_message) - (size_t)written, fmt, args);
}

void test_fail_impl(const char *file, int line, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    set_message(file, line, fmt, args);
    va_end(args);

    g_current_result = TEST_RESULT_FAIL;
    if (g_jump_active) {
        longjmp(g_test_jump, 1);
    }
}

void test_skip_impl(const char *file, int line, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    set_message(file, line, fmt, args);
    va_end(args);

    g_current_result = TEST_RESULT_SKIP;
    if (g_jump_active) {
        longjmp(g_test_jump, 1);
    }
}

static test_result_t run_one(const test_case_t *test)
{
    g_current_result = TEST_RESULT_PASS;
    g_current_message[0] = '\0';
    g_jump_active = 1;

    if (setjmp(g_test_jump) == 0) {
        test->test_func();
    }

    g_jump_active = 0;
    return g_current_result;
}

int test_run_all(void)
{
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    test_case_t *test = g_registry;

    while (test != NULL) {
        test_result_t result;

        if (!matches_filter(test)) {
            test = test->next;
            continue;
        }

        result = run_one(test);
        switch (result) {
            case TEST_RESULT_PASS:
                ++passed;
                printf("[PASS] %s.%s\n", test->suite_name, test->test_name);
                break;
            case TEST_RESULT_SKIP:
                ++skipped;
                printf("[SKIP] %s.%s %s\n", test->suite_name, test->test_name, g_current_message);
                break;
            default:
                ++failed;
                printf("[FAIL] %s.%s %s\n", test->suite_name, test->test_name, g_current_message);
                break;
        }

        if (g_test_verbose && g_current_message[0] != '\0' && result == TEST_RESULT_PASS) {
            printf("       %s\n", g_current_message);
        }

        test = test->next;
    }

    printf("\nSummary: %d passed, %d failed, %d skipped\n", passed, failed, skipped);
    return failed == 0 ? 0 : 1;
}
