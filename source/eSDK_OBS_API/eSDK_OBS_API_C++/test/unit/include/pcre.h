#ifndef TEST_UNIT_PCRE_H
#define TEST_UNIT_PCRE_H

/*
 * request.c only includes pcre.h but the SSL/GM test paths do not use PCRE.
 * This stub keeps the unit-test build self-contained on environments where
 * libpcre headers are not installed.
 */

#endif
