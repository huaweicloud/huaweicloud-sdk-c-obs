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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "request.h"
#include "request_context.h"
#include "response_headers_handler.h"
#include "util.h"
#include "request_util.h"
#include "pcre.h"
#include <openssl/ssl.h>
#include "eSDKOBS.h"
#if defined __GNUC__ || defined LINUX
#include <sys/utsname.h>
#include <pthread.h>
#endif
#define countof(array) (sizeof(array)/sizeof(array[0]))
#define REQUEST_STACK_SIZE 100
#define ARRAY_LENGTH_1024 1024

int API_STACK_SIZE = 100;
static char userAgentG[256];
static uint32_t requestStackCountG  = 0;
static uint32_t current_request_cnt = 0;
obs_openssl_switch g_switch_openssl =OBS_OPENSSL_CLOSE; 
obs_http_request_option *obs_default_http_request_option = NULL;
uint32_t request_online_max = 1000;    
http_request *requestStackG[REQUEST_STACK_SIZE] = {0};
int use_api_index = -1;
obs_s3_switch *api_switch=NULL;

#if defined __GNUC__ || defined LINUX
#include <unistd.h>
static pthread_mutex_t requestStackMutexG;
static pthread_mutex_t use_api_mutex;
#else
static HANDLE hmutex;
static HANDLE use_api_mutex;
#endif

void init_request_most_count(uint32_t online_request_max)
{
    request_online_max = online_request_max;
    return ;
}

static int sockopt_callback(const void *clientp, curl_socket_t curlfd, curlsocktype purpose)
{
    (void)purpose;
    int val = *(int*)clientp;
    setsockopt(curlfd, SOL_SOCKET, SO_RCVBUF, (const char *)&val, sizeof(val));
    return CURL_SOCKOPT_OK;
}

static void request_deinitialize(http_request *request)
{
    if (request->headers) {
        curl_slist_free_all(request->headers);
    }

    request->pause_handle = NULL;
    error_parser_deinitialize(&(request->errorParser));

    curl_easy_reset(request->curl);
}

void request_destroy(http_request *request)
{
    request_deinitialize(request);
    curl_easy_cleanup(request->curl);
    free(request);
    request = NULL;
}
void set_openssl_callback(obs_openssl_switch switch_flag)
{
    g_switch_openssl = switch_flag;
}

void request_api_deinitialize()
{
#if defined __GNUC__ || defined LINUX
    pthread_mutex_destroy(&requestStackMutexG);
    pthread_mutex_destroy(&use_api_mutex);
#else
    CloseHandle(hmutex);
    hmutex = NULL;
    CloseHandle(use_api_mutex);
    use_api_mutex = NULL;
#endif
    if(OBS_OPENSSL_CLOSE == g_switch_openssl)
    {
        kill_locks();
    }

    while (requestStackCountG--) {
        request_destroy(requestStackG[requestStackCountG]);
    }
    current_request_cnt = 0;
    use_api_index = -1;
    free(api_switch);
    api_switch =NULL;
}

obs_status request_api_initialize_global(unsigned int flags)
{
	if (curl_global_init(CURL_GLOBAL_ALL &
		~((flags & OBS_INIT_WINSOCK) ? 0 : CURL_GLOBAL_WIN32))
		!= CURLE_OK)
	{
		return OBS_STATUS_InternalError;
	}

	if (OBS_OPENSSL_CLOSE == g_switch_openssl)
	{
		init_locks();
	}
	return OBS_STATUS_OK;
}

void request_api_initialize_setPlatform()
{
#if defined __GNUC__ || defined LINUX
	pthread_mutex_init(&requestStackMutexG, 0);
	pthread_mutex_init(&use_api_mutex, 0);
#else
	hmutex = CreateMutexA(NULL, FALSE, "");
	use_api_mutex = CreateMutexA(NULL, FALSE, "");
#endif
	requestStackCountG = 0;
	current_request_cnt = 0;
	char platform[96];
#if defined __GNUC__ || defined LINUX
	struct utsname utsn;
	if (uname(&utsn))
	{
		strncpy_s(platform, sizeof(platform), "Unknown", sizeof("Unknown"));
		platform[sizeof(platform) - 1] = 0;
	}
	else
	{
		int ret = snprintf_s(platform, sizeof(platform), _TRUNCATE, "%s%s%s", utsn.sysname,
			utsn.machine[0] ? " " : "", utsn.machine);
		CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
	}
#else
	OSVERSIONINFOEX os;
	if (GetVersionEx((OSVERSIONINFO *)&os))
	{
	}
	else
	{
		int ret = strncpy_s(platform, sizeof(platform), "Unknown", sizeof("Unknown"));
		CheckAndLogNoneZero(ret, "strncpy_s", __FUNCTION__, __LINE__);
		platform[sizeof(platform) - 1] = 0;
	}
#endif
}

obs_status request_api_initialize(unsigned int flags)
{
	if (request_api_initialize_global(flags) != OBS_STATUS_OK)
	{
		return OBS_STATUS_InternalError;
	}

	request_api_initialize_setPlatform();

    api_switch = (obs_s3_switch *)malloc(sizeof(obs_s3_switch)*API_STACK_SIZE);
    if (NULL == api_switch)
    {
        return OBS_STATUS_OutOfMemory;
    }
    use_api_index = -1;
    int ret = snprintf_s(userAgentG, sizeof(userAgentG),_TRUNCATE,
    "%s%s%s.%s",PRODUCT, "/",LIBOBS_VER_MAJOR, LIBOBS_VER_MINOR);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);

    return OBS_STATUS_OK;
}

obs_status compose_obs_headers_withCondition(const obs_put_properties *properties,
	const server_side_encryption_params *encryption_params, const obs_cors_conf *corsConf,
	const request_params *params, request_computed_values *values, int *len)
{
	obs_status status = OBS_STATUS_OK;

    request_compose_limit(values, params, len);

	if (properties) {
		status = request_compose_properties(values, params, len);
		if (status != OBS_STATUS_OK)
		{
			return status;
		}
	}
	if (encryption_params)
	{
		status = request_compose_encrypt_params(values, params, len);
		if (status != OBS_STATUS_OK)
		{
			return status;
		}
	}
	if (corsConf)
	{
		status = request_compose_cors_conf(values, params, len);
		if (status != OBS_STATUS_OK)
		{
			return status;
		}
	}
	if (params->temp_auth == NULL)
	{
		status = request_compose_data(values, len, params);
		if (status !=OBS_STATUS_OK)
		{
			return status;
		}
	}
	return status;
}

static obs_status compose_obs_headers(const request_params *params,
                                    request_computed_values *values)
{
    const obs_put_properties *properties = params->put_properties;
    const obs_cors_conf *corsConf = params->corsConf;
    const server_side_encryption_params *encryption_params = params->encryption_params;

    values->amzHeadersCount = 0;
    values->amzHeadersRaw[0] = 0;
    int len = 0;
    obs_status status = OBS_STATUS_OK;

    if ((status = headers_append_list_bucket_type(params->bucketContext.bucket_list_type, 
                            values, &len)) != OBS_STATUS_OK)
    {
        return status;
    }
    
    if ((status = request_compose_token_and_httpcopy(values, params, &len)) != OBS_STATUS_OK) {
        return status;
    }
    
	status = compose_obs_headers_withCondition(properties, encryption_params, 
		corsConf, params, values, &len);
    return status;
}

static obs_status compose_standard_headers(const request_params *params,
                                         request_computed_values *values)
{
    obs_status status = OBS_STATUS_OK;
    if ((status = compose_put_header(params, values)) != OBS_STATUS_OK) {
        return status;
    }

    if ((status = compose_get_put_header(params, values)) != OBS_STATUS_OK) {
        return status;
    }

    if ((status = compose_range_header(params, values)) != OBS_STATUS_OK) {
        return status;
    }
    return OBS_STATUS_OK;
}

static void canonicalize_obs_headers(request_computed_values *values, obs_use_api use_api)
{
    const char *sortedHeaders[OBS_MAX_METADATA_COUNT] = {0};
    int iLoop = 0;
    int nCount = 0;

    for(iLoop = 0; iLoop < values->amzHeadersCount; iLoop++)
    {
        if (use_api == OBS_USE_API_S3) {
            if(0 == strncmp("x-amz-",values->amzHeaders[iLoop],strlen("x-amz-"))) {
                sortedHeaders[nCount] = values->amzHeaders[iLoop];
                nCount++;
            }
        } else {
            if(0 == strncmp("x-obs-",values->amzHeaders[iLoop],strlen("x-obs-"))) {
                sortedHeaders[nCount] = values->amzHeaders[iLoop];
                nCount++;
            }
        }
    }
    pre_compute_header(sortedHeaders, values, &nCount, use_api);
    header_gnome_sort(sortedHeaders, nCount);
    canonicalize_headers(values, sortedHeaders, nCount);
}

void canonicalize_resource_subResource(const request_params *params, const char * subResource,
	char * buffer, int buffer_size, int len)
{
	if (strcmp(subResource, "truncate") == 0)
	{
		if (params->queryParams && strstr(params->queryParams, "length") != NULL)
		{
			append_request("?", sizeof("?"));
			append_request(params->queryParams, strlen(params->queryParams));
			append_request("&", sizeof("&"));
			append_request(subResource, strlen(subResource));
		}
	}
	else if (strcmp(subResource, "rename") == 0)
	{
		if (params->queryParams && strstr(params->queryParams, "name") != NULL)
		{
			append_request("?", sizeof("?"));
			char decoded[3 * 1024];
			urlDecode(decoded, params->queryParams, strlen(params->queryParams));
			append_request(decoded, sizeof(decoded));
			append_request("&", sizeof("&"));
			append_request(subResource, strlen(subResource));
		}
	}
	else
	{
		append_request("?", sizeof("?"));
		append_request(subResource, strlen(subResource));
	}

}

static void canonicalize_resource(const request_params *params,
                                  const char *urlEncodedKey,
                                  char *buffer ,int buffer_size)
{
    int len = 0;
    *buffer = 0;
    const obs_bucket_context  * bucketContext = &params->bucketContext;
    const char * bucket_name = bucketContext->bucket_name;
    const char * subResource = params->subResource;

    if (bucket_name && bucket_name[0]) {
        buffer[len++] = '/';
        append_request(bucket_name, strlen(bucket_name));
    }

    append_request("/", sizeof("/"));
    if (urlEncodedKey && urlEncodedKey[0]) {
        append_request(urlEncodedKey, strlen(urlEncodedKey));
    }

    if (subResource && subResource[0]) {
		canonicalize_resource_subResource(params, subResource, buffer, buffer_size, len);
    }
}



static obs_status compose_uri(char *buffer, int buffer_size,
                            const obs_bucket_context *bucketContext,
                            const char *urlEncodedKey,
                            const char *subResource, const char *queryParams ,
                            temp_auth_info *tmpAuth, int temp_auth_flag)
{
    int len = 0;

    enum
    {
        Mark_Question, 
        Mark_And         
    };
    int appendBeforeTempSignature = Mark_Question;

    uri_append("http%s://", (bucketContext->protocol == OBS_PROTOCOL_HTTP) ? "" : "s");
    const char *host_name = bucketContext->host_name;
    if (bucketContext->bucket_name && bucketContext->bucket_name[0]) 
    {
        if (bucketContext->uri_style == OBS_URI_STYLE_VIRTUALHOST) {
            uri_append("%s.%s", bucketContext->bucket_name, host_name);
        }
        else {
            uri_append("%s/%s", host_name, bucketContext->bucket_name);
        }
    }
    else {
        uri_append("%s", host_name);
    }
    uri_append("%s", "/");
    uri_append("%s", urlEncodedKey);

    if (subResource && subResource[0]) 
    {
        uri_append("?%s", subResource);
        appendBeforeTempSignature = Mark_And;
    }

    if (queryParams) {
        uri_append("%s%s", (subResource && subResource[0]) ? "&" : "?",
                   queryParams);
        appendBeforeTempSignature = Mark_And;
    }
    if (temp_auth_flag == 1) {
        uri_append("%s%s", (appendBeforeTempSignature == Mark_And)? "&" : "?",
                          tmpAuth->tempAuthParams);
    }
    return OBS_STATUS_OK;
}


obs_status set_curl_easy_setopt_safe(http_request *request, const request_params *params)
{
    CURLcode status = CURLE_OK;
    switch (params->httpRequestType) {
        case http_request_type_head:
            curl_easy_setopt_safe(CURLOPT_NOBODY, 1);
            break;
        case http_request_type_put:
        case http_request_type_copy:
            curl_easy_setopt_safe(CURLOPT_UPLOAD, 1);
            break;
        case http_request_type_delete:
            curl_easy_setopt_safe(CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        case http_request_type_post:
            curl_easy_setopt_safe(CURLOPT_POST, 1L);
            break;
        case http_request_type_options:
            curl_easy_setopt_safe(CURLOPT_CUSTOMREQUEST, "OPTIONS");
            break;
        default:
            break;
    }

    return OBS_STATUS_OK;
}

obs_status setup_CA(http_request *request,
    const request_params *params,
    const request_computed_values *values)
{
    CURLcode status = CURLE_OK;
    curl_easy_setopt_safe(CURLOPT_SSL_VERIFYPEER, 1);
    curl_easy_setopt_safe(CURLOPT_SSL_VERIFYHOST, 0);
    if (params->bucketContext.certificate_info)
    {
        curl_easy_setopt_safe(CURLOPT_SSL_CTX_DATA, (void *)params->bucketContext.certificate_info);
        curl_easy_setopt_safe(CURLOPT_SSL_CTX_FUNCTION, *sslctx_function);
    }
    if (params->request_option.server_cert_path)
    {
        curl_easy_setopt_safe(CURLOPT_CAINFO, params->request_option.server_cert_path);
    }
    return OBS_STATUS_OK;
}


obs_status setup_CheckCA(http_request *request,
    const request_params *params,
    const request_computed_values *values)
{
    CURLcode status = CURLE_OK;
    if (params->isCheckCA) {
        return setup_CA(request, params, values);
    }
    else {
        curl_easy_setopt_safe(CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt_safe(CURLOPT_SSL_VERIFYHOST, 0);
    }
    return OBS_STATUS_OK;
}

static obs_status setup_curl(http_request *request,
                           const request_params *params,
                           const request_computed_values *values)
{
    CURLcode status = CURLE_OK;
    curl_easy_setopt_safe(CURLOPT_PRIVATE, request);
    curl_easy_setopt_safe(CURLOPT_HEADERDATA, request);
    curl_easy_setopt_safe(CURLOPT_HEADERFUNCTION, &curl_header_func);
    curl_easy_setopt_safe(CURLOPT_READFUNCTION, &curl_read_func);
    curl_easy_setopt_safe(CURLOPT_READDATA, request);
    curl_easy_setopt_safe(CURLOPT_WRITEFUNCTION, &curl_write_func);
    curl_easy_setopt_safe(CURLOPT_WRITEDATA, request);
    curl_easy_setopt_safe(CURLOPT_FILETIME, 1);
    curl_easy_setopt_safe(CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt_safe(CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt_safe(CURLOPT_TCP_NODELAY, 1);
    if (params->request_option.keep_alive)
    {
        curl_easy_setopt_safe(CURLOPT_TCP_KEEPALIVE, 1);
        curl_easy_setopt_safe(CURLOPT_TCP_KEEPIDLE, params->request_option.keep_idle);
        curl_easy_setopt_safe(CURLOPT_TCP_KEEPINTVL, params->request_option.keep_intvl);
    }
    if(params->request_option.ssl_cipher_list != NULL) {
         curl_easy_setopt_safe(CURLOPT_SSL_CIPHER_LIST, params->request_option.ssl_cipher_list);
    }
    if(params->request_option.proxy_host != NULL) {
         curl_easy_setopt_safe(CURLOPT_PROXY, params->request_option.proxy_host);
    }
    if(params->request_option.proxy_auth != NULL) {
         curl_easy_setopt_safe(CURLOPT_PROXYUSERPWD, params->request_option.proxy_auth);
    }
    curl_easy_setopt_safe(CURLOPT_NETRC, CURL_NETRC_IGNORED);
    setup_CheckCA(request, params, values);

    curl_easy_setopt_safe(CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt_safe(CURLOPT_MAXREDIRS, 10);
    curl_easy_setopt_safe(CURLOPT_USERAGENT, userAgentG);
    curl_easy_setopt_safe(CURLOPT_LOW_SPEED_LIMIT, params->request_option.speed_limit);
    curl_easy_setopt_safe(CURLOPT_LOW_SPEED_TIME, params->request_option.speed_time);
    curl_easy_setopt_safe(CURLOPT_CONNECTTIMEOUT_MS, params->request_option.connect_time);
    curl_easy_setopt_safe(CURLOPT_TIMEOUT, params->request_option.max_connected_time);
    curl_easy_setopt_safe(CURLOPT_BUFFERSIZE, params->request_option.buffer_size);

    if ((params->httpRequestType == http_request_type_put) || (params->httpRequestType == http_request_type_post)) {
        size_t headerLen = 256;
        char* header = (char *)malloc(sizeof(char) * headerLen);
        if (header == NULL) {
            COMMLOG(OBS_LOGERROR, "malloc failed in function: %s, line: %d", __FUNCTION__, __LINE__);
            return OBS_STATUS_OutOfMemory;
        } else {
            memset_s(header, headerLen, 0, headerLen);
        }
        int ret = snprintf_s(header, headerLen,_TRUNCATE, "Content-Length: %llu",
                 (unsigned long long) params->toObsCallbackTotalSize);
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        request->headers = curl_slist_append(request->headers, header);
        CHECK_NULL_FREE(header);
        request->headers = curl_slist_append(request->headers, "Transfer-Encoding:");
    }
    else if (params->httpRequestType == http_request_type_copy) {
        request->headers = curl_slist_append(request->headers, "Transfer-Encoding:");
    }
    append_standard_header(cacheControlHeader);
    if (values->contentTypeHeader[0]) {
        request->headers = curl_slist_append(request->headers, values->contentTypeHeader);
    }
    else {
        request->headers = curl_slist_append(request->headers, "Content-Type:");
    }
    append_standard_header(md5Header);
    append_standard_header(contentDispositionHeader);
    append_standard_header(contentEncodingHeader);
    append_standard_header(expiresHeader);
    append_standard_header(userAgent);
    append_standard_header(ifModifiedSinceHeader);
    append_standard_header(ifUnmodifiedSinceHeader);
    append_standard_header(ifMatchHeader);
    append_standard_header(ifNoneMatchHeader);
    append_standard_header(rangeHeader);
    append_standard_header(authorizationHeader);
    append_standard_header(websiteredirectlocationHeader);
    int i;
    for (i = 0; i < values->amzHeadersCount; i++) {
        request->headers = curl_slist_append(request->headers, values->amzHeaders[i]);
    }
    curl_easy_setopt_safe(CURLOPT_HTTPHEADER, request->headers);
    COMMLOG(OBS_LOGINFO, "%s request_perform setup_url: uri request_get = %s", __FUNCTION__,request->uri);
    curl_easy_setopt_safe(CURLOPT_URL, request->uri);
    int recvbuffersize = 256 * 1024;
    if( params->request_option.bbr_switch == OBS_BBR_OPEN ) {
        curl_easy_setopt_safe( CURLOPT_SOCKOPTFUNCTION, sockopt_callback );
        curl_easy_setopt_safe( CURLOPT_SOCKOPTDATA, &recvbuffersize);
    }
    /*if( params->request_option.http2_switch == OBS_HTTP2_OPEN )
    {
        curl_easy_setopt_safe(CURLOPT_HTTP_VERSION ,CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);
    }*/

    curl_easy_setopt_safe(CURLOPT_HTTP_VERSION ,CURL_HTTP_VERSION_1_1);
    return set_curl_easy_setopt_safe(request, params);
}

static void release_token()
{
#if defined __GNUC__ || defined LINUX
    pthread_mutex_lock(&requestStackMutexG);
#else
    WaitForSingleObject(hmutex, INFINITE);
#endif
    if (current_request_cnt > 0)
    {
        current_request_cnt--;
    }
#if defined __GNUC__ || defined LINUX
    pthread_mutex_unlock(&requestStackMutexG);
#else
    ReleaseMutex(hmutex);
#endif
}

static obs_status request_get(const request_params *params,
                            const request_computed_values *values,
                            http_request **reqReturn,
                            temp_auth_info *stTempAuthInfo)
{
    http_request *request = 0;
    int temp_auth_flag = 0;
    int is_no_token = 0;
    if (params->temp_auth)
    {
        temp_auth_flag = 1;
    }
#if defined __GNUC__ || defined LINUX
    pthread_mutex_lock(&requestStackMutexG);
#else
    WaitForSingleObject(hmutex, INFINITE);
#endif
    if ((current_request_cnt + 1) > request_online_max){
        is_no_token = 1;
    }else {
         current_request_cnt++;
        if (requestStackCountG) {
            request = requestStackG[--requestStackCountG];
        }
    }
#if defined __GNUC__ || defined LINUX
    pthread_mutex_unlock(&requestStackMutexG);
#else
    ReleaseMutex(hmutex);
#endif

    if (is_no_token)
    {
        COMMLOG(OBS_LOGWARN, "request is no token,cur token num=%u", current_request_cnt);
        return OBS_STATUS_NoToken;
    }
    
    if (request) {
        request_deinitialize(request);
    }
    else {
        if ((request = (http_request *) malloc(sizeof(http_request))) == NULL) {
            release_token();
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(request,sizeof(http_request), 0, sizeof(http_request));
        if ((request->curl = curl_easy_init()) == NULL) {
            free(request);  
            request = NULL; 
            release_token();
            return OBS_STATUS_FailedToIInitializeRequest;
        }
    }
    
    request->prev = 0;
    request->next = 0;
    request->status = OBS_STATUS_OK;
    obs_status status = OBS_STATUS_OK;
    request->headers = 0;
    if ((status = compose_uri(request->uri, sizeof(request->uri),
          &(params->bucketContext), values->urlEncodedKey,
          params->subResource, params->queryParams, stTempAuthInfo, temp_auth_flag)) != OBS_STATUS_OK) {
        curl_easy_cleanup(request->curl);
        free(request); 
        request = NULL;
        release_token();
        return status;
    }
    if ((status = setup_curl(request, params, values)) != OBS_STATUS_OK) {
        curl_easy_cleanup(request->curl);
        free(request); 
        request = NULL;
        release_token();
        return status;
    }
    request->properties_callback = params->properties_callback;
    request->toS3Callback = params->toObsCallback;
    request->toS3CallbackBytesRemaining = params->toObsCallbackTotalSize;
    request->progress_total_size = params->toObsCallbackTotalSize;
    request->fromS3Callback = params->fromObsCallback;
    request->complete_callback = params->complete_callback;
    request->progressCallback = params->progressCallback;
    request->callback_data = params->callback_data;
    response_headers_handler_initialize(&(request->responseHeadersHandler));
    request->propertiesCallbackMade = 0;
    request->pause_handle = params->pause_handle;
    error_parser_initialize(&(request->errorParser));
    *reqReturn = request;
    return OBS_STATUS_OK;
}

void set_query_params_ID(const request_params *params, char *signbuf,
	const char *pos, char *tmp, int tmp_len, int buf_len, int ret, int *len)
{
	if ((pos = strstr(params->queryParams, "uploadId=")) != NULL)
	{
		int len1 = pos - params->queryParams;
		if ((pos = strstr(params->queryParams + len1, "&")) != NULL)
		{
			len1 = pos - params->queryParams;
		}
		else
		{
			len1 = strlen(params->queryParams);
		}
		ret = strncpy_s(tmp, tmp_len, params->queryParams, len1);
		CheckAndLogNoneZero(ret, "strncpy_s", __FUNCTION__, __LINE__);
		signbuf_append("?%s", tmp);
	}

	if ((pos = strstr(params->queryParams, "versionId=")) != NULL)
	{
		if (params->subResource)
		{
			signbuf_append("&%s", params->queryParams);
		}
		else
		{
			signbuf_append("?%s", params->queryParams);
		}
	}
}

static obs_status set_query_params(const request_params *params, char *signbuf, 
            int *buf_now_len, int buf_len)
{
    int *len = buf_now_len;
    const char* pos = NULL;
    char tmp[1024]={0};
    int ret = 0;

	set_query_params_ID(params, signbuf, pos, tmp, 1024, buf_len, ret, len);
    
    if((pos=strstr(params->queryParams,"position=")) != NULL) 
    {
        if(params->subResource) 
        {
            signbuf_append("&%s", params->queryParams);
        }
        else 
        {
            signbuf_append("?%s", params->queryParams);
        }
    }
    
    if((pos=strstr(params->queryParams,"x-image-process=")) != NULL) 
    {
        int len1 = pos - params->queryParams;
        const char * pos2 = NULL;
        int len2 = strlen(params->queryParams + len1);
        char * decodedStr = NULL;
        if((pos2 = strstr(params->queryParams + len1,"&")) != NULL) 
        {
            len2 = pos2 - pos;
        }
        
        if(len2>0) 
        {
            ret = strncpy_s(tmp, sizeof(tmp),params->queryParams,len2);
            CheckAndLogNoneZero(ret, "strncpy_s", __FUNCTION__, __LINE__);
            decodedStr = (char*)malloc(len2+1);
			
			if (decodedStr == NULL)
			{
				COMMLOG(OBS_LOGWARN, "set_query_params: malloc failed!\n");
                return OBS_STATUS_InternalError;
			}
			
            memset_s(decodedStr,len2+1,0,len2+1);
            urlDecode(decodedStr,tmp,len2+1);
            ret = strncpy_s(tmp, ARRAY_LENGTH_1024, decodedStr, strlen(decodedStr)+1);
            CheckAndLogNoneZero(ret, "strncpy_s", __FUNCTION__, __LINE__);
            CHECK_NULL_FREE(decodedStr);   
            signbuf_append("?%s", tmp); 
        }
    }
    
    return OBS_STATUS_OK;
}

obs_status compose_auth_header_append(const request_params *params,
	request_computed_values *values, char *signbuf, int *len,
	int buf_len)
{
	signbuf_append("%s\n",http_request_type_to_verb(params->httpRequestType));
	signbuf_append("%s\n", values->md5Header[0] ?
		&(values->md5Header[sizeof("Content-MD5: ") - 1]) : "");
	signbuf_append("%s\n", values->contentTypeHeader[0] ?
		&(values->contentTypeHeader[sizeof("Content-Type: ") - 1]) : "");
	signbuf_append("%s", "\n");
	signbuf_append("%s", values->canonicalizedAmzHeaders);
	signbuf_append("%s", values->canonicalizedResource);
	if (NULL != params->queryParams) {
		obs_status ret_status = set_query_params(params, signbuf, len, buf_len);
		if (ret_status != OBS_STATUS_OK)
		{
			COMMLOG(OBS_LOGERROR, "set_query_params return %d !", ret_status);
			return ret_status;
		}
	}
	return OBS_STATUS_OK;
}

static obs_status compose_auth_header(const request_params *params,
                                    request_computed_values *values)
{
    char signbuf[17 + 129 + 129 + 1 +
                 (sizeof(values->canonicalizedAmzHeaders) - 1) +
                 (sizeof(values->canonicalizedResource) - 1) + 1];
    int buf_len = sizeof(signbuf);
    int lencount = 0;
	int *len = &lencount;
    
	obs_status status = compose_auth_header_append(params, values, signbuf, len, buf_len);
	if (status != OBS_STATUS_OK)
	{
		return status;
	}
    
    unsigned char hmac[20] = {0};
    HMAC_SHA1(hmac, (unsigned char *) params->bucketContext.secret_access_key,
              strlen(params->bucketContext.secret_access_key),
              (unsigned char *) signbuf, *len);
    char b64[((20 + 1) * 4) / 3 + 1] = {0};
    int b64Len = base64Encode(hmac, 20, b64);

    char *sts_marker;
    if(params->use_api == OBS_USE_API_S3) {
        sts_marker = "x-amz-security-token:";
    }
    else {
        sts_marker = "x-obs-security-token:";
    }
    char *secutiry_token = strstr(signbuf, sts_marker);
    if (NULL != secutiry_token) {
        char *secutiry_token_begin = secutiry_token + strlen(sts_marker);
        char *secutiry_token_end = strchr(secutiry_token_begin, '\n');
        if (NULL != secutiry_token_end) {
            for (int i = 0; i < secutiry_token_end - secutiry_token_begin; i++) {
                secutiry_token_begin[i] = '*';
            }
        }
    }
    //COMMLOG(OBS_LOGWARN, "%s request_perform : StringToSign:  %.*s", __FUNCTION__, buf_len, signbuf);

    if(params->use_api == OBS_USE_API_S3)
    {
        int ret = snprintf_s(values->authorizationHeader, sizeof(values->authorizationHeader),_TRUNCATE,
                 "Authorization: AWS %s:%.*s", params->bucketContext.access_key,
                 b64Len, b64);
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        COMMLOG(OBS_LOGINFO, "%s request_perform : Authorization: AWS %s:*****************", __FUNCTION__,params->bucketContext.access_key);
    }
    else
    {
        int ret = snprintf_s(values->authorizationHeader, sizeof(values->authorizationHeader),_TRUNCATE,
                 "Authorization: OBS %s:%.*s", params->bucketContext.access_key,
                 b64Len, b64);
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        COMMLOG(OBS_LOGINFO, "%s request_perform : Authorization: OBS %s:*****************", __FUNCTION__,params->bucketContext.access_key);
    }

    char * userAgent = USER_AGENT_VALUE;
    int strLen = (int)(strlen(userAgent));
    int ret = snprintf_s(values->userAgent, sizeof(values->userAgent),_TRUNCATE,"User-Agent: %.*s", strLen, userAgent);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);

    return OBS_STATUS_OK;
}



static void request_release(http_request **p_request)
{
    http_request *request = *p_request;
#if defined __GNUC__ || defined LINUX
    pthread_mutex_lock(&requestStackMutexG);
#else
    WaitForSingleObject(hmutex, INFINITE);
#endif

    if (requestStackCountG == REQUEST_STACK_SIZE || request->status != OBS_STATUS_OK) {
        if (current_request_cnt > 0)
        {
			current_request_cnt--;
        }
#if defined __GNUC__ || defined LINUX
        pthread_mutex_unlock(&requestStackMutexG);
#else
        ReleaseMutex(hmutex);
#endif
        request_destroy(request);
        request = NULL;
    }
    else {
        requestStackG[requestStackCountG++] = request;
		if (current_request_cnt > 0)
		{
			current_request_cnt--;
		}
#if defined __GNUC__ || defined LINUX
        pthread_mutex_unlock(&requestStackMutexG);
#else
        ReleaseMutex(hmutex);
#endif
    }
}

void request_finish_log_amz(struct curl_slist* tmp, OBS_LOGLEVEL logLevel)
{
	if (0 == strncmp(tmp->data, "x-amz-server-side-encryption-customer-key:", 42))
	{
		COMMLOG(logLevel, "x-amz-server-side-encryption-customer-key:***********");
	}
	else if ( 0 == strncmp(tmp->data, "x-amz-server-side-encryption-customer-key-md5:",46))
	{
		COMMLOG(logLevel, "x-amz-server-side-encryption-customer-key-md5:**********");
	}
	else if (0 == strncmp(tmp->data, "x-amz-copy-source-server-side-encryption-customer-key:", 54))
	{
		COMMLOG(logLevel, "x-amz-copy-source-server-side-encryption-customer-key:**********");
	}
	else if (0 == strncmp(tmp->data, "x-amz-copy-source-server-side-encryption-customer-key-md5:", 58))
	{
		COMMLOG(logLevel, "x-amz-copy-source-server-side-encryption-customer-key-md5:************");
	}
	else if (0 == strncmp(tmp->data, "x-amz-security-token:", strlen("x-amz-security-token:"))) {
		COMMLOG(logLevel, "x-amz-security-token:************");
	}
	else {
		COMMLOG(logLevel, "%s", tmp->data);
	}
}

void request_finish_log_obs(struct curl_slist* tmp, OBS_LOGLEVEL logLevel)
{
	if (0 == strncmp(tmp->data, "x-obs-server-side-encryption-customer-key:", 42))
	{
		COMMLOG(logLevel, "x-obs-server-side-encryption-customer-key:***********");
	}
	else if (0 == strncmp(tmp->data, "x-obs-server-side-encryption-customer-key-md5:", 46))
	{
		COMMLOG(logLevel, "x-obs-server-side-encryption-customer-key-md5:**********");
	}
	else if (0 == strncmp(tmp->data, "x-obs-copy-source-server-side-encryption-customer-key:", 54))
	{
		COMMLOG(logLevel, "x-obs-copy-source-server-side-encryption-customer-key:**********");
	}
	else if (0 == strncmp(tmp->data, "x-obs-copy-source-server-side-encryption-customer-key-md5:", 58))
	{
		COMMLOG(logLevel, "x-obs-copy-source-server-side-encryption-customer-key-md5:************");
	}
	else if (0 == strncmp(tmp->data, "x-obs-security-token:", strlen("x-obs-security-token:"))) {
		COMMLOG(logLevel, "x-obs-security-token:************");
	}
	else {
		COMMLOG(logLevel, "%s", tmp->data);
	}
}

void request_finish_log(struct curl_slist* tmp, OBS_LOGLEVEL logLevel) {
	if (0 == strncmp(tmp->data, "x-amz", 5))
	{
		request_finish_log_amz(tmp, logLevel);
	}
	else if(0 == strncmp(tmp->data, "x-obs", 5))
	{
		request_finish_log_obs(tmp, logLevel);
	}
	else
	{
		COMMLOG(logLevel, "%s", tmp->data);
	}
}

obs_status compose_headers(const request_params *params,
    request_computed_values *values)
{
    obs_status status = OBS_STATUS_OK;
    uint64_t copySourceKeyLen=0;
    uint64_t keyLen=0;
    if(params->copySourceKey){
        copySourceKeyLen = strlen(params->copySourceKey);
    }
    if(params->key){
        keyLen = strlen(params->key);
    }
    if ((status = encode_key(params->copySourceKey, copySourceKeyLen, values->urlEncodedSrcKey)) != OBS_STATUS_OK) {
        return_obs_status(status);
    }
    if ((status = compose_obs_headers(params, values)) != OBS_STATUS_OK) {
        return_obs_status(status);
    }
    if ((status = compose_standard_headers(params, values)) != OBS_STATUS_OK) {
        return_obs_status(status);
    }
    if ((status = encode_key(params->key, keyLen, values->urlEncodedKey)) != OBS_STATUS_OK) {
        return_obs_status(status);
    }
    return status;
}

int is_retry(http_request *request, int is_retry)
{
    if (request)
    {
        bool curl_retry = obs_status_is_retryable(request->status) ? true : false;
        bool request_retry = (request->httpResponseCode == 408 || request->httpResponseCode > 499) ? true : false;
        if (curl_retry || request_retry)
        {
            if (is_retry)
            {
                uint64_t wait_time = pow(2, RETRY_NUM - is_retry + 1) * 50;
#ifdef WIN32
                Sleep(wait_time);
#else
                usleep(wait_time * LINUX_USTOMS);
#endif
            }
            return --is_retry;

        }
        else {
            is_retry = 0;
        }
    }
    else
    {
        is_retry = 0;
    }
    return is_retry;
}

void request_finish(http_request **p_request)
{
    http_request *request= *p_request;
    request_headers_done(request);
    OBS_LOGLEVEL logLevel;
    int is_true = 0;

    is_true = ((request->status != OBS_STATUS_OK) || (((request->httpResponseCode < 200) || (request->httpResponseCode > 299)) 
        && (100 != request->httpResponseCode)));
    logLevel = is_true ? OBS_LOGWARN : OBS_LOGINFO;

    struct curl_slist* tmp = request->headers;
    while (NULL != tmp)
    {
        request_finish_log(tmp, logLevel);
        tmp = tmp->next;
    }
    COMMLOG(logLevel, "%s request_finish status = %d,httpResponseCode = %d", __FUNCTION__,
        request->status, request->httpResponseCode);
    COMMLOG(logLevel, "Message: %s", request->errorParser.obsErrorDetails.message);
    COMMLOG(logLevel, "Request Id: %s", request->responseHeadersHandler.responseProperties.request_id);
	COMMLOG(logLevel, "Reserved Indicator: %s", request->responseHeadersHandler.responseProperties.reserved_indicator);
    if(request->errorParser.codeLen) {
        COMMLOG(logLevel, "Code: %s", request->errorParser.code);
    }
    if (request->status == OBS_STATUS_OK) {
        error_parser_convert_status(&(request->errorParser),&(request->status));
        is_true = ((request->status == OBS_STATUS_OK) && ((request->httpResponseCode < 200) ||
             (request->httpResponseCode > 299)) && request->httpResponseCode != 100);
        if (is_true) {
            request->status = response_to_status(request);
        }
    }
    (*(request->complete_callback))
        (request->status, &(request->errorParser.obsErrorDetails),
         request->callback_data);
    request_release(p_request);
    *p_request = NULL;
}

int request_progress_callback(void *clientp,   curl_off_t  dltotal,   curl_off_t  dlnow,   curl_off_t  ultotal,   curl_off_t  ulnow) {
    http_request *request = (http_request *)clientp;
    static int i = 0;
    i++;

    if (i % 1000 == 0 && ulnow > 0) {
        COMMLOG(OBS_LOGDEBUG,"\nUP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
        "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
        "\r\n",ulnow, ultotal, dlnow, dltotal);
        COMMLOG(OBS_LOGDEBUG, "\nulnow %d", (int)ulnow);
    }
    if(ulnow > 0 && request->progressCallback) {
        request->progressCallback((uint64_t)ulnow, request->progress_total_size,request->callback_data);
    }

    if (dlnow > 0 && request->progressCallback) {
        request->progressCallback((uint64_t)dlnow, request->progress_total_size, request->callback_data);
    }

#ifdef WIN32
    HANDLE arrEvent = request->pause_handle;
    DWORD waitRet = WaitForSingleObject(arrEvent, SLEEP_TIMES_FOR_WAIT);
    if (waitRet == WAIT_OBJECT_0) {
        return -1;
    }
#endif

    return 0;
}

void request_set_opt_for_progress(http_request *request) {
    CURLcode setoptResult;
    setoptResult = curl_easy_setopt(request->curl, CURLOPT_XFERINFODATA, request);
    if (setoptResult != CURLE_OK){
        COMMLOG(OBS_LOGWARN, "%s curl_easy_setopt CURLOPT_PROGRESSDATA failed! CURLcode = %d", __FUNCTION__,setoptResult);
    }

    setoptResult =  curl_easy_setopt(request->curl, CURLOPT_XFERINFOFUNCTION, &request_progress_callback);
    if (setoptResult != CURLE_OK){
        COMMLOG(OBS_LOGWARN, "%s curl_easy_setopt CURLOPT_PROGRESSFUNCTION failed! CURLcode = %d", __FUNCTION__,setoptResult);
    }

    setoptResult = curl_easy_setopt(request->curl, CURLOPT_NOPROGRESS, 0L);
    if (setoptResult != CURLE_OK){
        COMMLOG(OBS_LOGWARN, "%s curl_easy_setopt CURLOPT_NOPROGRESS failed! CURLcode = %d", __FUNCTION__,setoptResult);
    }
}

void request_perform(const request_params *params)
{
    COMMLOG(OBS_LOGWARN, "enter request perform!!!");
    http_request *request = NULL;
    obs_status status = OBS_STATUS_OK;
    int is_true = 0;
    int retry = RETRY_NUM;
    COMMLOG(OBS_LOGINFO, "Ente request_perform object key= %s\n!", params->key);
    request_computed_values computed;
    memset_s(&computed, sizeof(request_computed_values), 0, sizeof(request_computed_values));
    char *errorBuffer = (char*)malloc(CURL_ERROR_SIZE*sizeof(char));
    if(errorBuffer == NULL){
        COMMLOG(OBS_LOGERROR, "malloc failed in function: %s, line: %d", __FUNCTION__, __LINE__);
        return;
    }
    int ret = memset_s(errorBuffer, CURL_ERROR_SIZE, 0, CURL_ERROR_SIZE);
    if(ret != 0){
        COMMLOG(OBS_LOGERROR, "memset_s failed in function: %s, line: %d", __FUNCTION__, __LINE__);
        CHECK_NULL_FREE(errorBuffer);
        return;
    }
    char authTmpParams[1024] = { 0 };
    char authTmpActualHeaders[1024] = { 0 };
    temp_auth_info stTempAuthInfo;
    ret = memset_s(&stTempAuthInfo, sizeof(temp_auth_info), 0, sizeof(temp_auth_info));
    if(ret != 0){
        COMMLOG(OBS_LOGERROR, "memset_s failed in function: %s, line: %d", __FUNCTION__, __LINE__);
        CHECK_NULL_FREE(errorBuffer);
        return;
    }
    stTempAuthInfo.temp_auth_headers = authTmpActualHeaders;
    stTempAuthInfo.tempAuthParams = authTmpParams;


    if ((status = compose_headers(params, &computed)) != OBS_STATUS_OK){
        COMMLOG(OBS_LOGERROR, "compose_headers failed in function: %s, line: %d", __FUNCTION__, __LINE__);
        CHECK_NULL_FREE(errorBuffer);
        return;
    }
        

    COMMLOG(OBS_LOGINFO, "Enter request_perform object computed key= %s\n!", computed.urlEncodedKey);
    canonicalize_obs_headers(&computed, params->use_api);
    canonicalize_resource(params, computed.urlEncodedKey, computed.canonicalizedResource,
        sizeof(computed.canonicalizedResource));
    if (params->temp_auth)
    {
        if ((status = compose_temp_header(params, &computed, &stTempAuthInfo)) != OBS_STATUS_OK) {
            CHECK_NULL_FREE(errorBuffer);
            return_status(status);
        }
    }
    else if ((status = compose_auth_header(params, &computed)) != OBS_STATUS_OK)
    {
        CHECK_NULL_FREE(errorBuffer);
        return_status(status);
    }

    if ((status = request_get(params, &computed, &request, &stTempAuthInfo)) != OBS_STATUS_OK) {
        CHECK_NULL_FREE(errorBuffer);
        return_status(status);
    }

    is_true = ((params->temp_auth) && (params->temp_auth->temp_auth_callback != NULL));
    if (is_true) {
        (params->temp_auth->temp_auth_callback)(request->uri,
            sizeof(request->uri),
            authTmpActualHeaders,
            sizeof(authTmpActualHeaders),
            params->temp_auth->callback_data);
        request_release(&request);
        CHECK_NULL_FREE(errorBuffer);
        return_status(status);
    }
    CURLcode setoptResult = curl_easy_setopt(request->curl, CURLOPT_ERRORBUFFER, errorBuffer);
    if (setoptResult != CURLE_OK) {
        COMMLOG(OBS_LOGWARN, "%s curl_easy_setopt failed! CURLcode = %d", __FUNCTION__, setoptResult);
    }

    request_set_opt_for_progress(request);

    char* accessmode = "Virtual Hosting";
    if (params->bucketContext.uri_style == OBS_URI_STYLE_PATH)
    {
        accessmode = "Path";
    }
    char* urlPrefix = params->bucketContext.protocol == OBS_PROTOCOL_HTTPS ? "https" : "http";
    COMMLOG(OBS_LOGINFO, "%s OBS SDK Version= %s; Endpoint = %s://%s; Access Mode = %s", __FUNCTION__, OBS_SDK_VERSION,
		urlPrefix, params->bucketContext.host_name, accessmode);
    COMMLOG(OBS_LOGINFO, "%s start curl_easy_perform now", __FUNCTION__);
    while (retry > 0)
    {
        CURLcode code = curl_easy_perform(request->curl);
        is_true = ((code != CURLE_OK) && (request->status == OBS_STATUS_OK));
        if (is_true) {
            request->status = request_curl_code_to_status(code);
            char *proxyBuf = strstr(errorBuffer, "proxy:");
            if (NULL != proxyBuf) {
                errno_t err = strcpy_s(proxyBuf, CURL_ERROR_SIZE - (proxyBuf - errorBuffer), "proxy: *****");
                CheckAndLogNoneZero(err, "strcpy_s", __FUNCTION__, __LINE__);
            }
            COMMLOG(OBS_LOGERROR, "In function :(%s) curl_easy_perform failed, CURLcode = %d"
				", curl_error_message is '%s', obs_status = %s(%d), curlErrorBuffer = %s"
				, __FUNCTION__, code, curl_easy_strerror(code)
				, obs_get_status_name(request->status), request->status, errorBuffer);
        }
        request_finish(&request);
        retry = is_retry(request, retry);
    }
    CHECK_NULL_FREE(errorBuffer);
}

static obs_status compose_api_version_uri(char *buffer, int buffer_size,
                                          const char *bucket_name, const char *host_name, 
                                          const char *subResource, obs_protocol protocol)
{
    int len = 0;
    uri_append("http%s://", (protocol == OBS_PROTOCOL_HTTP) ? "" : "s");
    if (bucket_name && bucket_name[0]) {
       uri_append("%s.%s", bucket_name, host_name);
    }
    else {
        uri_append("%s", host_name);
    }
    uri_append("%s", "/");
    uri_append("?%s", subResource);
    return OBS_STATUS_OK;
}

size_t api_header_func(void *ptr, size_t size, size_t nmemb,
                               void *data)
{
    obs_status *status = (obs_status*) data;
     if (!strncmp((char*)ptr, "x-obs-api: 3.0", 14)) {
        *status = OBS_STATUS_OK;
        COMMLOG(OBS_LOGINFO, "get api version !!!  %s",(char*)ptr);
    }
     return size*nmemb;
}

obs_status get_api_version(char *bucket_name,char *host_name,obs_protocol protocol, const obs_http_request_option *request_options)
{
    COMMLOG(OBS_LOGINFO, "get api version start!");
    obs_status status = OBS_STATUS_ErrorUnknown;
    uint64_t uriSize = MAX_URI_SIZE + 1;
    char *uri = (char *)malloc(sizeof(char) * uriSize);
    if (uri == NULL) {
        COMMLOG(OBS_LOGERROR, "malloc failed in function: %s, line: %d", __FUNCTION__, __LINE__);
        return status;
    } else {
        int ret = memset_s(uri, uriSize, 0, uriSize);
        if (ret != 0) {
            CHECK_NULL_FREE(uri);
            COMMLOG(OBS_LOGERROR, "memset_s failed in function: %s, line: %d", __FUNCTION__, __LINE__);
            return status;
        }
    }

    CURL *curl = NULL;
    long httpResponseCode = 0;
    char *errorBuffer = (char*)malloc(sizeof(char)*CURL_ERROR_SIZE);
    if(errorBuffer == NULL){
        CHECK_NULL_FREE(uri);
        COMMLOG(OBS_LOGERROR, "malloc failed in function: %s, line: %d", __FUNCTION__, __LINE__);
        return status;
    }else{
        int ret = memset_s(errorBuffer, CURL_ERROR_SIZE, 0, CURL_ERROR_SIZE);
        if (ret != 0) {
            CHECK_NULL_FREE(uri);
            CHECK_NULL_FREE(errorBuffer);
            COMMLOG(OBS_LOGERROR, "memset_s failed in function: %s, line: %d", __FUNCTION__, __LINE__);
            return status;
        }
    }
    
#define easy_setopt_safe(opt, val)                                 \
        if (curl_easy_setopt(curl, opt, val) != CURLE_OK) {                      \
            curl_easy_cleanup(curl);                                            \
            CHECK_NULL_FREE(uri);                                                          \
            CHECK_NULL_FREE(errorBuffer);                                                  \
            return OBS_STATUS_FailedToIInitializeRequest;                       \
        }
    
    if ((curl = curl_easy_init()) == NULL) {
        CHECK_NULL_FREE(uri);                                                          
        CHECK_NULL_FREE(errorBuffer);
        return OBS_STATUS_FailedToIInitializeRequest;
    }
    obs_status statu = OBS_STATUS_OK;
    if (( statu =compose_api_version_uri(uri,uriSize,bucket_name,host_name,"apiversion",protocol)) != OBS_STATUS_OK) {
        curl_easy_cleanup(curl);
        CHECK_NULL_FREE(uri);                                                          
        CHECK_NULL_FREE(errorBuffer);
        return statu;
    }
    if (protocol == OBS_PROTOCOL_HTTPS) {
        easy_setopt_safe(CURLOPT_SSL_VERIFYPEER, 0);
        easy_setopt_safe(CURLOPT_SSL_VERIFYHOST, 0);
    }
    
    easy_setopt_safe(CURLOPT_HEADERDATA, &status);
    easy_setopt_safe(CURLOPT_HEADERFUNCTION, &api_header_func);
    
    easy_setopt_safe(CURLOPT_NOSIGNAL, 1);
    easy_setopt_safe(CURLOPT_TCP_NODELAY, 1);
    easy_setopt_safe(CURLOPT_NOPROGRESS, 1);
    easy_setopt_safe(CURLOPT_FOLLOWLOCATION, 1);
    easy_setopt_safe(CURLOPT_URL, uri);
    easy_setopt_safe(CURLOPT_NOBODY, 1);

    easy_setopt_safe(CURLOPT_LOW_SPEED_LIMIT, request_options->speed_limit);
    easy_setopt_safe(CURLOPT_LOW_SPEED_TIME, request_options->speed_time);
    easy_setopt_safe(CURLOPT_CONNECTTIMEOUT_MS, request_options->connect_time);
    easy_setopt_safe(CURLOPT_TIMEOUT, request_options->max_connected_time);

    
    if (request_options->proxy_host != NULL) {
		easy_setopt_safe(CURLOPT_PROXY, request_options->proxy_host);
	}
	if (request_options->proxy_auth != NULL) {
		easy_setopt_safe(CURLOPT_PROXYUSERPWD, request_options->proxy_auth);
	}

    CURLcode setoptResult =  curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,errorBuffer);
    COMMLOG(OBS_LOGWARN, "curl_easy_setopt curl path= %s",uri);
    if (setoptResult != CURLE_OK){
        COMMLOG(OBS_LOGWARN, "%s curl_easy_setopt failed! CURLcode = %d", __FUNCTION__,setoptResult);
    }
    CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        obs_status status = request_curl_code_to_status(code);
		COMMLOG(OBS_LOGERROR, "In function :(%s) curl_easy_perform failed, CURLcode = %d"
			", curl_error_message is '%s', obs_status = %s(%d), curlErrorBuffer = %s"
			, __FUNCTION__, code, curl_easy_strerror(code)
			, obs_get_status_name(status), status, errorBuffer);
        curl_easy_cleanup(curl); 
        CHECK_NULL_FREE(uri);                                                          
        CHECK_NULL_FREE(errorBuffer);       
        return status;
    }
       
    if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                          &httpResponseCode) != CURLE_OK) {
        curl_easy_cleanup(curl); 
        CHECK_NULL_FREE(uri);                                                          
        CHECK_NULL_FREE(errorBuffer);
        return OBS_STATUS_InternalError;
    }
    
    COMMLOG(OBS_LOGINFO, "curl_easy_setopt curl with httpResponseCode = %d",httpResponseCode);
    if(status == OBS_STATUS_OK && httpResponseCode == 200)
    {
		curl_easy_cleanup(curl); 
        CHECK_NULL_FREE(uri);                                                          
        CHECK_NULL_FREE(errorBuffer);
        return OBS_STATUS_OK;
    }
    else {
		curl_easy_cleanup(curl); 
        CHECK_NULL_FREE(uri);                                                          
        CHECK_NULL_FREE(errorBuffer);
        return status;
    }
}

static int sort_bucket_name(const char *bucket_name, const char *host_name)
{   
    int index = -1;
    for( int i=0; i<(use_api_index+1); i++)
    {
        if( !strncmp(api_switch[i].bucket_name,bucket_name,strlen(bucket_name) ) && 
            !strncmp(api_switch[i].host_name,host_name,strlen(host_name) ) )
            {
                index = i;
                break;
            }
    }
    if (index == -1) {
       if (use_api_index == API_STACK_SIZE*3/4) {   
            API_STACK_SIZE = 2*API_STACK_SIZE;
            obs_s3_switch *temp_api_switch = api_switch;
            api_switch = (obs_s3_switch *)malloc(sizeof(obs_s3_switch)*API_STACK_SIZE);
            //api_switch = (obs_s3_switch *)realloc(api_switch,sizeof(obs_s3_switch)*API_STACK_SIZE);
            if ( api_switch == NULL ) {
	   	        use_api_index--;
                API_STACK_SIZE = API_STACK_SIZE/2;
                api_switch = temp_api_switch;
            }
            else {
                errno_t err = memcpy_s(api_switch, sizeof(obs_s3_switch)*API_STACK_SIZE, temp_api_switch, sizeof(obs_s3_switch)*API_STACK_SIZE / 2);
                if (err != EOK)
                {
                    COMMLOG(OBS_LOGWARN, "%s(%d): memcpy_s failed!", __FUNCTION__, __LINE__);
                    use_api_index--;
                    API_STACK_SIZE = API_STACK_SIZE / 2;
                    free(api_switch);
                    api_switch = temp_api_switch;
                }
                else {
                    free(temp_api_switch);
                }
            }
       } 
    }
    return index;
}

void set_use_api_switch( const obs_options *options, obs_use_api *use_api_temp)
{
    if (options->bucket_options.uri_style == OBS_URI_STYLE_PATH)
    {
        return ;
    }
	
    if (options->request_options.auth_switch == OBS_OBS_TYPE)
    {
        *use_api_temp = OBS_USE_API_OBS;
        return;
    }
	
    if (options->request_options.auth_switch == OBS_S3_TYPE)
    {
        *use_api_temp = OBS_USE_API_S3;
        return;
    }
	
    int index = -1;
#if defined __GNUC__ || defined LINUX
		pthread_mutex_lock(&use_api_mutex);
#else
		WaitForSingleObject(use_api_mutex, INFINITE);
#endif
    time_t time_obs = time(NULL);
    errno_t err = EOK;
    if (use_api_index == -1) {
        use_api_index++;
        if(get_api_version(options->bucket_options.bucket_name,options->bucket_options.host_name,
                           options->bucket_options.protocol, &options->request_options) == OBS_STATUS_OK )
        {   
			err = memcpy_s(api_switch[use_api_index].bucket_name, BUCKET_LEN-1, options->bucket_options.bucket_name, strlen(options->bucket_options.bucket_name));
            CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

            api_switch[use_api_index].bucket_name[strlen(options->bucket_options.bucket_name)] = '\0';
            
			err = memcpy_s(api_switch[use_api_index].host_name, DOMAIN_LEN-1, options->bucket_options.host_name, strlen(options->bucket_options.host_name));
            CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

            api_switch[use_api_index].host_name[strlen(options->bucket_options.host_name)] = '\0';
            api_switch[use_api_index].use_api = OBS_USE_API_OBS;
            api_switch[use_api_index].time_switch = time_obs;
            *use_api_temp = OBS_USE_API_OBS;
        }
        else {
			err = memcpy_s(api_switch[use_api_index].bucket_name, BUCKET_LEN-1, options->bucket_options.bucket_name, strlen(options->bucket_options.bucket_name));
            CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

            api_switch[use_api_index].bucket_name[strlen(options->bucket_options.bucket_name)] = '\0';
			
			err = memcpy_s(api_switch[use_api_index].host_name, DOMAIN_LEN-1, options->bucket_options.host_name, strlen(options->bucket_options.host_name));
            CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

            api_switch[use_api_index].host_name[strlen(options->bucket_options.host_name)] = '\0';

            api_switch[use_api_index].use_api = OBS_USE_API_S3;
            api_switch[use_api_index].time_switch = time_obs;
                
            *use_api_temp = OBS_USE_API_S3;
        }       
    }
    else {
        if((index = sort_bucket_name(options->bucket_options.bucket_name,options->bucket_options.host_name)) > -1)
        {
           if ( difftime(time_obs , api_switch[index].time_switch) > 900.00)
           {
                if(get_api_version(options->bucket_options.bucket_name,options->bucket_options.host_name,
                                   options->bucket_options.protocol, &options->request_options) == OBS_STATUS_OK )
                {
                    api_switch[index].use_api = OBS_USE_API_OBS;
                    api_switch[index].time_switch = time_obs;
                    *use_api_temp = OBS_USE_API_OBS;

                }
                else {
                    api_switch[index].use_api = OBS_USE_API_S3;
                    api_switch[index].time_switch = time_obs;
                    *use_api_temp = OBS_USE_API_S3;
                }
           }
           else {
                api_switch[index].time_switch = time_obs;
           }
        }
        else {
            use_api_index++;
            if(get_api_version(options->bucket_options.bucket_name,options->bucket_options.host_name,
                               options->bucket_options.protocol, &options->request_options) == OBS_STATUS_OK )
            {
				err = memcpy_s(api_switch[use_api_index].bucket_name, BUCKET_LEN-1, options->bucket_options.bucket_name, strlen(options->bucket_options.bucket_name));
                CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

                api_switch[use_api_index].bucket_name[strlen(options->bucket_options.bucket_name)] = '\0';
				
				err = memcpy_s(api_switch[use_api_index].host_name, DOMAIN_LEN-1, options->bucket_options.host_name, strlen(options->bucket_options.host_name));
                CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

                api_switch[use_api_index].host_name[strlen(options->bucket_options.host_name)] = '\0';

                api_switch[use_api_index].use_api = OBS_USE_API_OBS;
                api_switch[use_api_index].time_switch = time_obs;
                *use_api_temp = OBS_USE_API_OBS;

            }
            else {
				err = memcpy_s(api_switch[use_api_index].bucket_name, BUCKET_LEN-1, options->bucket_options.bucket_name, strlen(options->bucket_options.bucket_name));
                CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

                api_switch[use_api_index].bucket_name[strlen(options->bucket_options.bucket_name)] = '\0';

				err = memcpy_s(api_switch[use_api_index].host_name, DOMAIN_LEN-1, options->bucket_options.host_name, strlen(options->bucket_options.host_name));
                CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

                api_switch[use_api_index].host_name[strlen(options->bucket_options.host_name)] = '\0';

                api_switch[use_api_index].use_api = OBS_USE_API_S3;
                api_switch[use_api_index].time_switch = time_obs;
                *use_api_temp = OBS_USE_API_S3;
            }
        }  
    }
	
#if defined __GNUC__ || defined LINUX
    pthread_mutex_unlock(&use_api_mutex);
#else
    ReleaseMutex(use_api_mutex);
#endif
}


