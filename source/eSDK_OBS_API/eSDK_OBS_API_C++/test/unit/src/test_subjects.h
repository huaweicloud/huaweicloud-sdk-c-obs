#ifndef TEST_SUBJECTS_H
#define TEST_SUBJECTS_H

#include "eSDKOBS.h"
#include "request_util.h"

obs_status setup_client_certificate(CURL *curl, const obs_http_request_option *request_options);
obs_status setup_CA(CURL *curl, const obs_http_request_option *request_options);
obs_status setup_CheckCA(CURL *curl, const obs_http_request_option *request_options, char *certificate_info);

#endif
