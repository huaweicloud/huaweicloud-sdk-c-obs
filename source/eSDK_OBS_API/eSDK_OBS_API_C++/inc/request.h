/*********************************************************************************
* Copyright 2019 Huawei Technologies Co.,Ltd.
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
#ifndef REQUEST_H
#define REQUEST_H

#include "eSDKOBS.h"
#include "error_parser.h"
#include "response_headers_handler.h"
#include "util.h"
#include "log.h"
#include "common.h"


#ifdef WIN32
#define LIBOBS_VER_MAJOR "3.19"
#define LIBOBS_VER_MINOR "9"
#endif

#if defined __GNUC__ || defined LINUX
#endif

#define HEAD_NORMAL_LEN 128
#define HEAD_WEBSITE_LEN 2200
#define HEAD_AUTH_LEN 1028
#define BUCKET_LEN 65
#define DOMAIN_LEN 254

#define OBS_SDK_VERSION "3.19.9.3"
#define USER_AGENT_VALUE  "obs-sdk-c-3.19.9.3" ;

#define DEFAULT_LOW_SPEED_LIMIT    (1)
#define DEFAULT_LOW_SPEED_TIME_S   (300)
#define DEFAULT_CONNECTTIMEOUT_MS  (60000)
#define DEFAULT_TIMEOUT_S          (0)

#define signbuf_append(format, ...)                             \
    if (snprintf_sec(&(signbuf[len]), buf_len - (len), _TRUNCATE,format, __VA_ARGS__) > 0) \
    {\
        (len) += snprintf_sec(&(signbuf[len]), buf_len - (len), _TRUNCATE,      \
            format, __VA_ARGS__);                                                     \
    }\

#define uri_append(fmt, ...)                                                 \
        do {                                                                     \
            len += snprintf_sec(&(buffer[len]), buffer_size - len, _TRUNCATE,  fmt, __VA_ARGS__); \
            if (len >= buffer_size) {                                             \
                return OBS_STATUS_UriTooLong;                                       \
            }                                                                    \
        } while (0)

#define curl_easy_setopt_safe(opt, val)                                 \
                if ((status = curl_easy_setopt                                      \
                     (request->curl, opt, val)) != CURLE_OK) {                      \
                    return OBS_STATUS_FailedToIInitializeRequest;                       \
                }
                
#define append_standard_header(fieldName)                               \
                    if (values-> fieldName [0]) {                                       \
                        request->headers = curl_slist_append(request->headers,          \
                                                             values-> fieldName);       \
                    }
                    
#define append_request(str) len += sprintf_sec(&(buffer[len]), buffer_size-len, "%s", str)

#define return_status(status)                                           \
    (*(params->complete_callback))(status, 0, params->callback_data);     \
    COMMLOG(OBS_LOGWARN, "%s status = %d", __FUNCTION__,status);\
    return


typedef enum
{
    http_request_type_get,
    http_request_type_head,
    http_request_type_put,
    http_request_type_copy,
    http_request_type_delete,
    http_request_type_post,
    http_request_type_options
} http_request_type;

typedef enum
{
   no_need_storage_class,
   default_storage_class, 
   storage_class
}obs_storage_class_format;


typedef struct http_request
{
    struct http_request *prev, *next;
    obs_status status;
    int httpResponseCode;
    struct curl_slist *headers;
    CURL *curl;
    char uri[MAX_URI_SIZE + 1];
    obs_response_properties_callback *properties_callback;
    obs_put_object_data_callback *toS3Callback;
    int64_t toS3CallbackBytesRemaining;
    obs_get_object_data_callback *fromS3Callback;
    obs_response_complete_callback *complete_callback;
    void *callback_data;
    response_headers_handler responseHeadersHandler;
    int propertiesCallbackMade;
    error_parser errorParser;
} http_request;

typedef struct obs_cors_conf
{
    char *origin;
    char *requestMethod[100];
    unsigned int rmNumber;
    char *requestHeader[100];
    unsigned int rhNumber;
}obs_cors_conf;

typedef struct request_params
{
    http_request_type httpRequestType;

    obs_bucket_context bucketContext;

    obs_http_request_option request_option;

    temp_auth_configure *temp_auth;

    char *key;

    char *queryParams;

    char *subResource;

    char *copySourceBucketName;

    char *copySourceKey;

    obs_get_conditions *get_conditions;

    obs_cors_conf *corsConf;

    obs_put_properties *put_properties;

    server_side_encryption_params *encryption_params;

    obs_response_properties_callback *properties_callback;

    obs_put_object_data_callback *toObsCallback;

    int64_t toObsCallbackTotalSize;

    obs_get_object_data_callback *fromObsCallback;

    obs_response_complete_callback *complete_callback;

    void *callback_data;

    int isCheckCA;

    obs_storage_class_format storageClassFormat;

    obs_use_api use_api;

} request_params;

typedef struct request_computed_values
{
    char *amzHeaders[OBS_MAX_METADATA_COUNT + 3];

    int amzHeadersCount;

    char amzHeadersRaw[30000+COMPACTED_METADATA_BUFFER_SIZE + 256 + 1];

    string_multibuffer(canonicalizedAmzHeaders,
                       COMPACTED_METADATA_BUFFER_SIZE + 30000 + 256 + 1);

    char urlEncodedKey[MAX_URLENCODED_KEY_SIZE + 1];

    char canonicalizedResource[MAX_CANONICALIZED_RESOURCE_SIZE + 1];

    char cacheControlHeader[HEAD_NORMAL_LEN];

    char contentTypeHeader[HEAD_NORMAL_LEN];

    char md5Header[HEAD_NORMAL_LEN];

    char contentDispositionHeader[HEAD_NORMAL_LEN];

    char contentEncodingHeader[HEAD_NORMAL_LEN];

    char websiteredirectlocationHeader[HEAD_WEBSITE_LEN];

    char expiresHeader[HEAD_NORMAL_LEN];

    char ifModifiedSinceHeader[HEAD_NORMAL_LEN];

    char ifUnmodifiedSinceHeader[HEAD_NORMAL_LEN];

    char ifMatchHeader[HEAD_NORMAL_LEN];

    char ifNoneMatchHeader[HEAD_NORMAL_LEN];

    char rangeHeader[HEAD_NORMAL_LEN];

    char authorizationHeader[HEAD_AUTH_LEN];

    char tokenHeader[HEAD_AUTH_LEN];

    char userAgent[HEAD_NORMAL_LEN];

} request_computed_values;

typedef struct __temp_auth_info
{
    char * tempAuthParams;
    char * temp_auth_headers;
}temp_auth_info;

typedef struct obs_s3_switch
{
    time_t time_switch;
    char bucket_name[BUCKET_LEN];
    char host_name[DOMAIN_LEN]; 
    obs_use_api use_api;
} obs_s3_switch;

void init_request_most_count(uint32_t online_request_max);

obs_status request_api_initialize(unsigned int flags);

obs_status request_curl_code_to_status(CURLcode code);

void request_destroy();

void request_finish(http_request * request);

void request_api_deinitialize();

void request_perform(const request_params *params);

void set_use_api_switch(const obs_options *options ,obs_use_api *use_api_temp);

obs_use_api get_api_protocol(char *bucket_name, char *host_name);

#endif /* REQUEST_H */


