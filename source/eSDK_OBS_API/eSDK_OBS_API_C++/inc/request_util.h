/*********************************************************************************
* Copyright 2022 Huawei Technologies Co.,Ltd.
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use
* this file except in compliance with the License.  You may obtain a copy of the
* License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software distributed
* under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
* CONDITIONS OF ANY KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations under the License.
**********************************************************************************
*/

#ifndef REQUEST_UTIL_H
#define REQUEST_UTIL_H

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "eSDKOBS.h"
#include "request.h"
#include "error_parser.h"
#include "response_headers_handler.h"
#include "util.h"
#include "log.h"
#include <openssl/ssl.h>


#ifdef _MSC_VER
#define snprintf_s _snprintf_s
#endif

#define signbuf_attach(format, ...)                             \
        do{\
            int lenAdded = snprintf_s(&(signbuf[len]), sizeof(signbuf) - len,_TRUNCATE,format, __VA_ARGS__);\
            if (lenAdded > 0)  \
            {\
                len += lenAdded;\
            }else\
            {\
                COMMLOG(OBS_LOGERROR, "attch string failed in compose_authV2_temp_header, lenAdded is[%d]",lenAdded);\
            }\
        }while(0)


void request_headers_done(http_request *request);

size_t curl_header_func(void *ptr, size_t size, size_t nmemb,
                               void *data);
size_t curl_read_func(void *ptr, size_t size, size_t nmemb, void *data);

size_t curl_write_func(void *ptr, size_t size, size_t nmemb,void *data);

CURLcode sslctx_function(CURL *curl, const void *sslctx, void *parm);

void init_locks(void);

void kill_locks(void);

int headerle(const char *header1, const char *header2);

void header_gnome_sort(const char **headers, int size);

const char *http_request_type_to_verb(http_request_type requestType);

obs_status encode_key(const char * pSrc, uint64_t pSrcLen, char *pValue);

obs_status request_curl_code_to_status(CURLcode code);

obs_status request_compose_limit(request_computed_values *values,
    const request_params *params, int *len);

obs_status request_compose_properties(request_computed_values *values, 
                const request_params *params, int *len);

obs_status request_compose_encrypt_params(request_computed_values *values, 
                const request_params *params, int *len);

obs_status request_compose_cors_conf(request_computed_values *values, 
                const request_params *params, int *len);

obs_status request_compose_data(request_computed_values *values, int *len,const request_params *params);

obs_status request_compose_token_and_httpcopy(request_computed_values *values, 
                const request_params *params, int *len);

obs_status compose_put_header(const request_params *params,
                            request_computed_values *values);

obs_status compose_get_put_header(const request_params *params,
                                request_computed_values *values);

obs_status compose_range_header(const request_params *params,
                              request_computed_values *values);

obs_status compose_callback_header(const request_params *params,
    request_computed_values *values, int *len);

int compose_callback_params_header(char * param_str,
    const char* param_name, char *callback_str, int charllback_index);

void pre_compute_header(const char **sortedHeaders, 
                request_computed_values *values, int *nCount, obs_use_api use_api);
void canonicalize_headers(request_computed_values *values, 
                const char **sortedHeaders, int nCount);
obs_status response_to_status(http_request *request);

obs_storage_class get_storage_class_enum(const char * str_storage_class,obs_use_api use_api);

obs_status compose_temp_header(const request_params* params,
                               request_computed_values* values,
                               temp_auth_info *stTempAuthInfo);
obs_status headers_append_list_bucket_type(obs_bucket_list_type bucket_list_type,
            request_computed_values *values, int *len);

bool is_check_ca(const obs_options *options);

obs_status copy_options_and_init_params(const obs_options *options, request_params* params,
        obs_use_api* use_api, obs_response_handler *handler, void *callback_data);

int debug_libcurl_callback(CURL *handle,
	curl_infotype type,
	char *data,
	size_t size,
	void *userptr);

#endif /* REQUEST_UTIL_H */


