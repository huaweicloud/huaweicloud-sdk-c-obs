#include <time.h>
#include "obs_time_util.h"
#include "securec.h"
#define MAX_BUFFER_SIZE 256
#define RFC1123_DATE_SIZE 32
bool check_before_rfc1123_date_format(const struct tm *tm) {
	if (!CheckAndLogNULL((void*)tm, SYMBOL_NAME_STR(tm), __FUNCTION__, __FUNCTION__, __LINE__)) {
		return false;
	}
	if (tm->tm_wday < 0 || tm->tm_wday > 6) {
		COMMLOG(OBS_LOGERROR, "%s(%d) is invalid in %s", SYMBOL_NAME_STR(tm->tm_wday), tm->tm_wday, __FUNCTION__);
		return false;
	}
	if (tm->tm_mon < 0 || tm->tm_mon > 11) {
		COMMLOG(OBS_LOGERROR, "%s(%d) is invalid in %s", SYMBOL_NAME_STR(tm->tm_mon), tm->tm_mon, __FUNCTION__);
		return false;
	}
	return true;
}
void rfc1123_date_format(char *time_buffer, size_t time_buffer_size, const struct tm *tm, const char* before_date, const char* after_date) {
	if (!check_before_rfc1123_date_format(tm)) {
		COMMLOG(OBS_LOGERROR, "%s failed in %s", SYMBOL_NAME_STR(check_before_rfc1123_date_format), __FUNCTION__);
		return;
	}
	static const char *weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	int ret;
	if (before_date == NULL && after_date != NULL) {
		ret = snprintf_s(time_buffer, time_buffer_size, time_buffer_size - 1, "%s, %02d %s %d %02d:%02d:%02d%s",
			weekdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec, after_date);
	} else if (before_date != NULL && after_date != NULL) {
		ret = snprintf_s(time_buffer, time_buffer_size, time_buffer_size - 1, "%s%s, %02d %s %d %02d:%02d:%02d%s",
			before_date, weekdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec, after_date);
	}
	else if (before_date != NULL && after_date == NULL) {
		ret = snprintf_s(time_buffer, time_buffer_size, time_buffer_size - 1, "%s%s, %02d %s %d %02d:%02d:%02d",
			before_date, weekdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
	else {
		ret = snprintf_s(time_buffer, time_buffer_size, time_buffer_size - 1, "%s, %02d %s %d %02d:%02d:%02d",
			weekdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
	CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
	time_buffer[time_buffer_size - 1] = '\0';
}