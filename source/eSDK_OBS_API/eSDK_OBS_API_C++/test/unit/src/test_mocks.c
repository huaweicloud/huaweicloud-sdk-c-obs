#include "test_mocks.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "request_util.h"

#define MOCK_MAX_CALLS 128
#define MOCK_MAX_RESULTS 32

typedef struct {
    CURLoption option;
    CURLcode result;
} mock_result_t;

typedef struct {
    mock_curl_call_t calls[MOCK_MAX_CALLS];
    size_t call_count;
    mock_result_t results[MOCK_MAX_RESULTS];
    size_t result_count;
    size_t cleanse_calls;
} mock_state_t;

static mock_state_t g_mock_state;

void mock_reset(void)
{
    memset(&g_mock_state, 0, sizeof(g_mock_state));
}

static CURLcode lookup_result(CURLoption option)
{
    size_t i = 0;

    for (i = 0; i < g_mock_state.result_count; ++i) {
        if (g_mock_state.results[i].option == option) {
            return g_mock_state.results[i].result;
        }
    }

    return CURLE_OK;
}

void mock_set_curl_result(CURLoption option, CURLcode result)
{
    size_t i = 0;

    for (i = 0; i < g_mock_state.result_count; ++i) {
        if (g_mock_state.results[i].option == option) {
            g_mock_state.results[i].result = result;
            return;
        }
    }

    if (g_mock_state.result_count < MOCK_MAX_RESULTS) {
        g_mock_state.results[g_mock_state.result_count].option = option;
        g_mock_state.results[g_mock_state.result_count].result = result;
        ++g_mock_state.result_count;
    }
}

static int option_uses_long(CURLoption option)
{
    return option == CURLOPT_SSL_VERIFYPEER ||
           option == CURLOPT_SSL_VERIFYHOST ||
           option == CURLOPT_SSLVERSION;
}

static uintptr_t read_option_value(CURLoption option, va_list args)
{
    if (option == CURLOPT_SSL_CTX_FUNCTION) {
        union {
            curl_ssl_ctx_callback fn;
            uintptr_t value;
        } callback_value;
        callback_value.fn = va_arg(args, curl_ssl_ctx_callback);
        return callback_value.value;
    }

    if (option_uses_long(option)) {
        long value = va_arg(args, long);
        return (uintptr_t)value;
    }

    return (uintptr_t)va_arg(args, void *);
}

CURLcode ut_curl_easy_setopt(CURL *curl, CURLoption option, ...)
{
    va_list args;
    uintptr_t value = 0;

    (void)curl;

    va_start(args, option);
    value = read_option_value(option, args);
    va_end(args);

    if (g_mock_state.call_count < MOCK_MAX_CALLS) {
        g_mock_state.calls[g_mock_state.call_count].option = option;
        g_mock_state.calls[g_mock_state.call_count].value = value;
        ++g_mock_state.call_count;
    }

    return lookup_result(option);
}

const char *ut_curl_easy_strerror(CURLcode code)
{
    switch (code) {
        case CURLE_OK:
            return "CURLE_OK";
        case CURLE_SSL_CERTPROBLEM:
            return "CURLE_SSL_CERTPROBLEM";
        case CURLE_BAD_FUNCTION_ARGUMENT:
            return "CURLE_BAD_FUNCTION_ARGUMENT";
        default:
            return "mock curl error";
    }
}

size_t mock_curl_call_count(CURLoption option)
{
    size_t count = 0;
    size_t i = 0;

    for (i = 0; i < g_mock_state.call_count; ++i) {
        if (g_mock_state.calls[i].option == option) {
            ++count;
        }
    }

    return count;
}

uintptr_t mock_curl_call_value(CURLoption option, size_t occurrence)
{
    size_t seen = 0;
    size_t i = 0;

    for (i = 0; i < g_mock_state.call_count; ++i) {
        if (g_mock_state.calls[i].option != option) {
            continue;
        }
        if (seen == occurrence) {
            return g_mock_state.calls[i].value;
        }
        ++seen;
    }

    return 0;
}

size_t mock_curl_total_call_count(void)
{
    return g_mock_state.call_count;
}

void ut_OPENSSL_cleanse(void *ptr, size_t len)
{
    ++g_mock_state.cleanse_calls;
    if (ptr != NULL && len > 0) {
        memset(ptr, 0, len);
    }
}

size_t mock_openssl_cleanse_call_count(void)
{
    return g_mock_state.cleanse_calls;
}

CURLcode sslctx_function(CURL *curl, const void *sslctx, void *parm)
{
    (void)curl;
    (void)sslctx;
    (void)parm;
    return CURLE_OK;
}

void COMMLOG(OBS_LOGLEVEL level, const char *format, ...)
{
    (void)level;
    (void)format;
}

OBS_LOGLEVEL getRunLogLevel(void)
{
    return OBS_LOGDEBUG;
}
