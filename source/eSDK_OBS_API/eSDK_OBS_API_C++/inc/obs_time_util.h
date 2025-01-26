#ifndef OBS_TIME_UTIL_H
#define OBS_TIME_UTIL_H
#include "eSDKOBS.h"
#include "log.h"
void rfc1123_date_format(char *time_buffer, size_t time_buffer_size, const struct tm *tm, const char* before_date, const char* after_date);
#endif /* OBS_TIME_UTIL_H */