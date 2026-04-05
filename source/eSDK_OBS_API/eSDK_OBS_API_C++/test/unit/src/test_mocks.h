#ifndef TEST_MOCKS_H
#define TEST_MOCKS_H

#include <stdint.h>
#include <stddef.h>

#include <curl/curl.h>
#include <openssl/ssl.h>

typedef struct {
    CURLoption option;
    uintptr_t value;
} mock_curl_call_t;

void mock_reset(void);
void mock_set_curl_result(CURLoption option, CURLcode result);
size_t mock_curl_call_count(CURLoption option);
uintptr_t mock_curl_call_value(CURLoption option, size_t occurrence);
size_t mock_curl_total_call_count(void);

size_t mock_openssl_cleanse_call_count(void);

CURLcode ut_curl_easy_setopt(CURL *curl, CURLoption option, ...);
const char *ut_curl_easy_strerror(CURLcode code);
void ut_OPENSSL_cleanse(void *ptr, size_t len);

#endif
