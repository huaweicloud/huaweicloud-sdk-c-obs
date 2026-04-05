#include "test_mocks.h"

#ifdef curl_easy_setopt
#undef curl_easy_setopt
#endif
#define curl_easy_setopt ut_curl_easy_setopt
#define curl_easy_strerror ut_curl_easy_strerror
#define OPENSSL_cleanse ut_OPENSSL_cleanse

#include "../../../src/request.c"
