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
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <ctype.h>
#include <string.h>
#include "util.h"
#include "securec.h"
#include <pthread.h>
#include "log.h"
#include "request.h"
#include <openssl/ssl.h>
#include "pcre.h"
#include <sys/utsname.h>
#include "request_util.h"


#define SSEC_KEY_MD5_LENGTH 64
#ifdef WIN32
#include <windows.h>
#else
#include <locale.h>
#include <iconv.h>
#endif

#if defined __GNUC__ || defined LINUX
static pthread_mutex_t* lockarray;
#else
static HANDLE* lockarray;
#endif

#define do_put_header(params, values, fmt, sourceField, destField, badError, tooLongError)  \
    do {                                                                    \
        if (params->put_properties &&                                        \
            params->put_properties->sourceField &&                          \
            params->put_properties->sourceField[0]) {                       \
            const char *val = params->put_properties-> sourceField;          \
            while (*val && is_blank(*val)) {                                \
                val++;                                                      \
            }                                                               \
            if (!*val) {                                                    \
                return badError;                                            \
            }                                                               \
            int len = snprintf_sec(values->destField,                          \
                               sizeof(values->destField),_TRUNCATE,  fmt, val);       \
            if (len >= (int) sizeof(values->destField) || len < 0) {                  \
                return tooLongError;                                        \
            }                                                               \
            while (is_blank(values-> destField[len])) {                     \
                if (len > 0)                                                \
                {                                                           \
                    len--;                                                  \
                }                                                           \
            }                                                               \
            values-> destField[len] = 0;                                    \
        }                                                                   \
        else {                                                              \
            values-> destField[0] = 0;                                      \
        }                                                                   \
    } while (0)

#define do_get_header(params, values, fmt, sourceField, destField, badError, tooLongError)  \
        do {                                                                            \
            if (params->get_conditions &&                                                \
                params->get_conditions-> sourceField &&                                  \
                params->get_conditions-> sourceField[0]) {                                   \
                const char *val = params->get_conditions-> sourceField;                  \
                while (*val && is_blank(*val)) {                                        \
                    val++;                                                              \
                }                                                                       \
                if (!*val) {                                                             \
                    return badError;                                                    \
                }                                                                       \
                int len = snprintf_sec(values-> destField,                                \
                    sizeof(values-> destField),_TRUNCATE,  fmt, val);                   \
                if (len >= (int) sizeof(values-> destField) || len < 0) {               \
                    return tooLongError;                                                \
                }                                                                       \
                while ((len > 0) && is_blank(values-> destField[len])) {                \
                    len--;                                                              \
                }                                                                       \
                values-> destField[len] = 0;                                            \
            }                                                                           \
            else {                                                                      \
                values-> destField[0] = 0;                                              \
            }                                                                           \
        } while (0)

#define do_gp_header(params, values, fmt, sourceField, destField, badError, tooLongError) \
    do {                                                                                  \
        if (params->put_properties && params->put_properties->get_conditions &&              \
            params->put_properties->get_conditions-> sourceField &&                           \
            params->put_properties->get_conditions-> sourceField[0]) {                        \
            const char *val = params->put_properties->get_conditions-> sourceField;           \
            while (*val && is_blank(*val)) {                                                \
                val++;                                                                      \
            }                                                                               \
            if (!*val) {                                                                    \
                return badError;                                                            \
            }                                                                               \
            int len = snprintf_sec(values-> destField,                                        \
                            sizeof(values-> destField),_TRUNCATE,  fmt, val);               \
            if (len >= (int) sizeof(values-> destField) || len < 0) {                       \
                return tooLongError;                                                        \
        }                                                                                   \
        while ((len > 0) && is_blank(values-> destField[len])) {                            \
            len--;                                                                          \
        }                                                                                   \
        values-> destField[len] = 0;                                                        \
        }                                                                                   \
        else {                                                                              \
            values-> destField[0] = 0;                                                      \
        }                                                                                   \
    } while (0)

void request_headers_done(http_request *request)
{
    if (request->propertiesCallbackMade) {
        return;
    }
    request->propertiesCallbackMade = 1;
    long httpResponseCode = 0;
    request->httpResponseCode = 0;
    if (curl_easy_getinfo(request->curl, CURLINFO_RESPONSE_CODE,
                          &httpResponseCode) != CURLE_OK) {
        request->status = OBS_STATUS_InternalError;
        return;
    }
    else {
        request->httpResponseCode = httpResponseCode;
    }
    response_headers_handler_done(&(request->responseHeadersHandler),
                                  request->curl);
    if (request->properties_callback) {
        (*(request->properties_callback))
            (&(request->responseHeadersHandler.responseProperties),
             request->callback_data);
    }
}


size_t curl_header_func(void *ptr, size_t size, size_t nmemb,
                               void *data)
{
    http_request *request = (http_request *) data;

    size_t len = size * nmemb;

    response_headers_handler_add
        (&(request->responseHeadersHandler), (char *) ptr, len);

    return len;
}

size_t curl_read_func(void *ptr, size_t size, size_t nmemb, void *data)
{
    http_request *request = (http_request *) data;


    int64_t len = (int64_t)size * nmemb;
    if (request->status != OBS_STATUS_OK) {
        return CURL_READFUNC_ABORT;
    }
    if (!request->toS3Callback || !request->toS3CallbackBytesRemaining) {
        return 0;
    }
    if (len > request->toS3CallbackBytesRemaining) {
        len = request->toS3CallbackBytesRemaining;
    }
    int64_t ret = (*(request->toS3Callback))
        ((int)len, (char *) ptr, request->callback_data);
    if (ret < 0) {
        request->status = OBS_STATUS_AbortedByCallback;
        return CURL_READFUNC_ABORT;
    }
    else {
        if (ret > request->toS3CallbackBytesRemaining) {
            ret = request->toS3CallbackBytesRemaining;
        }
        request->toS3CallbackBytesRemaining -= ret;
        return (size_t)ret;
    }
}

size_t curl_write_func(void *ptr, size_t size, size_t nmemb,
                              void *data)
{
    http_request *request = (http_request *) data;

    int64_t len = (int64_t)size * nmemb;

    request_headers_done(request);

    if (request->status != OBS_STATUS_OK) {
        return 0;
    }

    if ((request->httpResponseCode < 200) ||
        (request->httpResponseCode > 299)) {
        request->status = error_parser_add
            (&(request->errorParser), (char *) ptr, (int)len);
    }
    else if (request->fromS3Callback) {
        request->status = (*(request->fromS3Callback))
            ((int)len, (char *) ptr, request->callback_data);
    }
    else {
        request->status = OBS_STATUS_InternalError;
    }

    return ((request->status == OBS_STATUS_OK) ? (size_t)len : 0);
}

CURLcode sslctx_function(CURL *curl, const void *sslctx, void *parm)
{
    (void)curl;

    X509_STORE *store = NULL;
    X509 *cert = NULL;
    BIO *bio = NULL;

    bio = BIO_new_mem_buf((char *)parm, -1);

    PEM_read_bio_X509(bio, &cert, 0, NULL);

    store = SSL_CTX_get_cert_store((SSL_CTX *)sslctx);
    X509_STORE_add_cert(store, cert);
    X509_free(cert);
    BIO_free(bio);
    return CURLE_OK;
}

obs_status headers_append(int *len, request_computed_values *values, int isNewHeader,
    const char *format, const char *chstr1, const char *chstr2 )
{
    if (isNewHeader) 
    {                                              
        values->amzHeaders[values->amzHeadersCount++] = &(values->amzHeadersRaw[*len]);                          
    }
    if(chstr2)
    {
        if (snprintf_sec(&(values->amzHeadersRaw[*len]), sizeof(values->amzHeadersRaw) - (*len),
                    _TRUNCATE, format, chstr1, chstr2) > 0) 
        {
            (*len) += snprintf_sec(&(values->amzHeadersRaw[*len]),                  
            sizeof(values->amzHeadersRaw) - (*len),_TRUNCATE, format, chstr1, chstr2);                           
        }
    }
    else{
        if (snprintf_sec(&(values->amzHeadersRaw[*len]), sizeof(values->amzHeadersRaw) - (*len),
            _TRUNCATE, format, chstr1) > 0) 
        {
            (*len) += snprintf_sec(&(values->amzHeadersRaw[*len]),                  
            sizeof(values->amzHeadersRaw) - (*len), _TRUNCATE, format, chstr1);                           
        }
    }
    if (*len >= (int) sizeof(values->amzHeadersRaw)) {               
        return OBS_STATUS_MetadataHeadersTooLong;                      
    }                                                               
    while ((*len > 0) && (values->amzHeadersRaw[*len - 1] == ' ')) {  
        (*len)--;                                                      
    }                                                               
    values->amzHeadersRaw[(*len)++] = 0;
    return OBS_STATUS_OK;
}

obs_status header_name_tolower_copy(request_computed_values *values, int *len, const char *str, int l) {
     values->amzHeaders[values->amzHeadersCount++] = &(values->amzHeadersRaw[*len]);                              
     if (((*len) + l) >= (int) sizeof(values->amzHeadersRaw)) {        
            return OBS_STATUS_MetadataHeadersTooLong;                      
        }                                                               
      int todo = l;                                                   
      while (todo--) {                                                
          if ((*(str) >= 'A') && (*(str) <= 'Z')) {                   
                values->amzHeadersRaw[(*len)++] = 'a' + (*(str) - 'A');    
          }                                                           
          else {                                                      
              values->amzHeadersRaw[(*len)++] = *(str);                  
          }                                                           
          (str)++;                                                    
      }
    return OBS_STATUS_OK;
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static void lock_callback(int mode, int type, const char *file, int line)
{
    (void)file;
    (void)line;
    if (((unsigned int)(mode)) & CRYPTO_LOCK) {
#if defined __GNUC__ || defined LINUX
        pthread_mutex_lock(&(lockarray[type]));
#else
        WaitForSingleObject(lockarray[type], INFINITE);
#endif
    }
    else
    {
#if defined __GNUC__ || defined LINUX
        pthread_mutex_unlock(&(lockarray[type]));
#else
        ReleaseMutex(lockarray[type]);
#endif
    }
}

static unsigned long thread_id(void)
{
    unsigned long ret;

#if defined __GNUC__ || defined LINUX
    ret = (unsigned long)pthread_self();
#else
    ret = (unsigned long)GetCurrentThreadId();
#endif

    return(ret);
}
#endif

void init_locks(void)
{
    int i;

#if defined __GNUC__ || defined LINUX
    lockarray = (pthread_mutex_t*)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    for (i=0; i<CRYPTO_num_locks(); i++)
    {
        pthread_mutex_init(&(lockarray[i]), NULL);
    }
#else
    lockarray = (HANDLE *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(HANDLE));
    for (i=0; i<CRYPTO_num_locks(); i++)
    {
        lockarray[i] = CreateMutexA(NULL, false, "");
    }
#endif

    CRYPTO_set_id_callback((unsigned long (*)())thread_id);
    CRYPTO_set_locking_callback((void (*)(int, int, const char*, int))lock_callback);
}

void kill_locks(void)
{
    int i;

    CRYPTO_set_locking_callback(NULL);
    for (i=0; i<CRYPTO_num_locks(); i++)
    {
#if defined __GNUC__ || defined LINUX
        pthread_mutex_destroy(&(lockarray[i]));
#else
        CloseHandle(lockarray[i]);
#endif
    }

    OPENSSL_free(lockarray);
}

int headerle(const char *header1, const char *header2)
{
    while (1) {
        if (*header1 == ':') {
            return (*header2 != ':');
        }
        else if (*header2 == ':') {
            return 0;
        }
        else if (*header2 < *header1) {
            return 0;
        }
        else if (*header2 > *header1) {
            return 1;
        }
        header1++, header2++;
    }
}

void header_gnome_sort(const char **headers, int size)
{
    int i = 0, last_highest = 0;

    while (i < size) {
        if ((i == 0) || headerle(headers[i - 1], headers[i])) {
            i = ++last_highest;
        }
        else {
            const char *tmp = headers[i];
            headers[i] = headers[i - 1];
            headers[--i] = tmp;
        }
    }
}

const char *http_request_type_to_verb(http_request_type requestType)
{
    switch (requestType) {
    case http_request_type_get:
        return "GET";
    case http_request_type_head:
        return "HEAD";
    case http_request_type_put:
    case http_request_type_copy:
        return "PUT";
    case http_request_type_post:
        return "POST";
    case http_request_type_options:
        return "OPTIONS";
    default: //http_request_type_delete
        return "DELETE";
    }
}

/*
** encode_key 对object_key做encode
** 目前的特别处理逻辑为：默认不对/字符encode(分享的时候m3u8需要依赖目录结构)，
** 但是有./的还是需要encode，原因在于./在libcurl去request的时候会被自动去掉，
** 从而会导致sdk计算的CanonicalizedResource和服务端计算的不一致，最终会签名不匹配
**/
obs_status encode_key(const request_params *params,
                    request_computed_values *values)
{
    char ingoreChar = '/';
    if (NULL != params->key)
    {
#ifdef WIN32
        char *pStrKeyUTF8 = string_To_UTF8(params->key);
        if (NULL != strstr(pStrKeyUTF8, "./"))
        {
            ingoreChar = 0;
        }
        int nRet = urlEncode(values->urlEncodedKey, pStrKeyUTF8, OBS_MAX_KEY_SIZE, ingoreChar);
        CHECK_NULL_FREE(pStrKeyUTF8);
        return (nRet ? OBS_STATUS_OK : OBS_STATUS_UriTooLong);
#else
        if (NULL != strstr(params->key, "./"))
        {
            ingoreChar = 0;
        }
        return (urlEncode(values->urlEncodedKey, params->key, OBS_MAX_KEY_SIZE, ingoreChar) ?
            OBS_STATUS_OK : OBS_STATUS_UriTooLong);
#endif
    }
    else
    {
        return (urlEncode(values->urlEncodedKey, params->key, OBS_MAX_KEY_SIZE, ingoreChar) ?
            OBS_STATUS_OK : OBS_STATUS_UriTooLong);
    }
}

obs_status request_curl_code_to_status(CURLcode code) {
    switch (code) {
    case CURLE_OUT_OF_MEMORY:
        return OBS_STATUS_OutOfMemory;
    case CURLE_COULDNT_RESOLVE_PROXY:
    case CURLE_COULDNT_RESOLVE_HOST:
        return OBS_STATUS_NameLookupError;
    case CURLE_COULDNT_CONNECT:
        return OBS_STATUS_FailedToConnect;
    case CURLE_WRITE_ERROR:
    case CURLE_OPERATION_TIMEDOUT:
        return OBS_STATUS_ConnectionFailed;
    case CURLE_PARTIAL_FILE:
        return OBS_STATUS_PartialFile;
    case CURLE_SSL_CACERT:
        return OBS_STATUS_ServerFailedVerification;
    default:
        return OBS_STATUS_InternalError;
    }
}

obs_status headers_append_acl(obs_canned_acl acl, request_computed_values *values, int *len, const request_params *params)
{
    char *cannedAclString = NULL;
    switch (acl)
    {
        case OBS_CANNED_ACL_PRIVATE:
            cannedAclString = "private";
            break;
        case OBS_CANNED_ACL_PUBLIC_READ:
            cannedAclString = "public-read";
            break;
        case OBS_CANNED_ACL_PUBLIC_READ_WRITE:
            cannedAclString = "public-read-write";
            break;
        case OBS_CANNED_ACL_AUTHENTICATED_READ:
            cannedAclString = "authenticated-read";
            break;
        case OBS_CANNED_ACL_BUCKET_OWNER_READ:
            cannedAclString = "bucket-owner-read";
            break;
        case OBS_CANNED_ACL_BUCKET_OWNER_FULL_CONTROL:
            cannedAclString = "bucket-owner-full-control";
            break;
        case OBS_CANNED_ACL_LOG_DELIVERY_WRITE:
            cannedAclString = "log-delivery-write";
            break;
        case OBS_CANNED_ACL_PUBLIC_READ_DELIVERED:
            cannedAclString = "public-read-delivered";
            break;
        case OBS_CANNED_ACL_PUBLIC_READ_WRITE_DELIVERED:
            cannedAclString = "public-read-write-delivered";
            break;
        case OBS_CANNED_ACL_BUTT:
            cannedAclString = NULL;
            break;
        default: 
            cannedAclString = "authenticated-read";
            break;
    }
    
    if (params->use_api == OBS_USE_API_S3) {
        return headers_append(len, values, 1, "x-amz-acl: %s", cannedAclString, NULL);
    }
    else
    {
        return headers_append(len, values, 1, "x-obs-acl: %s", cannedAclString, NULL);
    }
    
    return OBS_STATUS_OK;
}

static obs_status headers_append_az_redundancy(obs_az_redundancy az_redundancy, request_computed_values *values, int *len, const request_params *params)
{
    char *azRedundancyString = NULL;
    switch (az_redundancy)
    {
        case OBS_REDUNDANCY_3AZ:
            azRedundancyString = "3az";
            break;
        default: 
            break;
    }
    
    if (params->use_api == OBS_USE_API_OBS && azRedundancyString != NULL) 
    {
        return headers_append(len, values, 1, "x-obs-az-redundancy: %s", azRedundancyString, NULL);
    }
    
    return OBS_STATUS_OK;
}

obs_status headers_append_domin(const obs_put_properties *properties,
            request_computed_values *values, int *len)
{
    char *grant_domain = NULL;
    if (!properties->domain_config)
    {
        return OBS_STATUS_OK;
    }
    
    switch (properties->domain_config->grant_domain)
    {
        case OBS_GRANT_READ:
            grant_domain = "x-obs-grant-read: %s";
            break;
        case OBS_GRANT_WRITE:
            grant_domain = "x-obs-grant-write: %s";
            break;
        case OBS_GRANT_READ_ACP:
            grant_domain = "x-obs-grant-read-acp: %s";
            break;
        case OBS_GRANT_WRITE_ACP:
            grant_domain = "x-obs-grant-write-acp: %s";
            break;
        case OBS_GRANT_FULL_CONTROL:
            grant_domain = "x-obs-grant-full-control: %s";
            break;
        case OBS_GRANT_FULL_CONTROL_DELIVERED:
            grant_domain = "x-obs-grant-full-control-delivered: %s";
            break;
        case OBS_GRANT_READ_DELIVERED:
            grant_domain = "x-obs-grant-read-delivered: %s";
            break;
        case OBS_GRANT_BUTT:
            grant_domain = NULL;
            break;
        default: 
            grant_domain = NULL;
            break;
    }
    
    if (NULL != grant_domain) {
        return headers_append(len, values, 1, grant_domain, properties->domain_config->domain, NULL);
    }

    return OBS_STATUS_OK;
}

obs_status headers_append_storage_class(obs_storage_class input_storage_class,
            request_computed_values *values, const request_params *params, int *len)
{
    const char *storageClassString = NULL;
    if (params->use_api == OBS_USE_API_S3) {
        switch (input_storage_class) {
            case OBS_STORAGE_CLASS_STANDARD_IA:
                storageClassString = "STANDARD_IA";
                break;
            case OBS_STORAGE_CLASS_GLACIER:
                storageClassString = "GLACIER";
                break;
            default:
                storageClassString = "STANDARD";
                break;
        }
    } else {
        switch (input_storage_class) {
            case OBS_STORAGE_CLASS_STANDARD_IA:
                storageClassString = "WARM";
                break;
            case OBS_STORAGE_CLASS_GLACIER:
                storageClassString = "COLD";
                break;
            default:
                storageClassString = "STANDARD";
                break;
        }
    }

    if (params->use_api == OBS_USE_API_S3) {
        if(params->storageClassFormat == storage_class){  
            return headers_append(len, values, 1, "x-amz-storage-class: %s", storageClassString, NULL);
        }
        else if (params->storageClassFormat == default_storage_class){
            return headers_append(len, values, 1, "x-default-storage-class: %s", storageClassString, NULL);
        }
    } 
    else 
    {
        if(params->storageClassFormat != no_need_storage_class) {
            return headers_append(len, values, 1, "x-obs-storage-class: %s", storageClassString, NULL);
        }
    }

    return OBS_STATUS_OK;
}


obs_status headers_append_bucket_type(obs_bucket_type bucket_type,
            request_computed_values *values, int *len)
{
    if(bucket_type == OBS_BUCKET_PFS) {
        return headers_append(len, values, 1, "x-obs-fs-file-interface: %s", "Enabled", NULL);
    }
    return OBS_STATUS_OK;
}
obs_status headers_append_list_bucket_type(obs_bucket_list_type bucket_list_type,
            request_computed_values *values, int *len)
{
    if (bucket_list_type == OBS_BUCKET_LIST_OBJECT)
    {  
        return headers_append(len, values, 1, "x-obs-bucket-type: %s", "OBJECT", NULL);
    }
    else if (bucket_list_type == OBS_BUCKET_LIST_PFS)
    {
        return headers_append(len, values, 1, "x-obs-bucket-type: %s", "POSIX", NULL);
    }
    return OBS_STATUS_OK;
}
obs_status headers_append_epid(const char *epid, request_computed_values *values, const request_params *params, int *len) 
{
    if (params->use_api == OBS_USE_API_S3) {
         return headers_append(len, values, 1, "x-amz-epid: %s", epid, NULL);
    } 
    else 
    {
         return headers_append(len, values, 1, "x-obs-epid: %s", epid, NULL);
    }

    return OBS_STATUS_OK;
}


obs_status request_compose_properties(request_computed_values *values, const request_params *params, int *len)
{
    const obs_put_properties *properties = params->put_properties;
    int i;
    int j;
	obs_status ret_status;
	
	if (properties != NULL)
	{
		// The name field of the user-defined metadata cannot be duplicated
		for  (i = 0; i < properties->meta_data_count; i++) 
		{
			for (j = i+1; j<  properties->meta_data_count; j++)
		 	{
		 		if (!strcmp(properties->meta_data[i].name, properties->meta_data[j].name))
				{
					return  OBS_STATUS_MetadataNameDuplicate;
				}
		 	}
		}

		for (i = 0; i < properties->meta_data_count; i++) {
			const obs_name_value *property = &(properties->meta_data[i]);
			char headerName[OBS_MAX_METADATA_SIZE - sizeof(": v")];
			int l = 0;
			if ( params->use_api == OBS_USE_API_S3) {
				l = snprintf_sec(headerName, sizeof(headerName),_TRUNCATE,
							OBS_METADATA_HEADER_NAME_PREFIX "%s",
							property->name);
			} else {
				l = snprintf_sec(headerName, sizeof(headerName),_TRUNCATE,
							"x-obs-meta-%s",
							property->name);
			}
			char *hn = headerName;
			if (header_name_tolower_copy(values, len, hn, l) != OBS_STATUS_OK){
				return header_name_tolower_copy(values, len, hn, l);
			}
			if (headers_append(len ,values, 0, ": %s", property->value, NULL) != OBS_STATUS_OK){
				return headers_append(len ,values, 0, ": %s", property->value, NULL);
			}
		}

		ret_status = headers_append_acl(properties->canned_acl, values, len, params);
		if (OBS_STATUS_OK != ret_status) {
			return ret_status;
		}

        ret_status = headers_append_az_redundancy(properties->az_redundancy, values, len, params);
        if (OBS_STATUS_OK != ret_status) {
			return ret_status;
		}

		ret_status = headers_append_domin(properties, values, len);
		if (OBS_STATUS_OK != ret_status) {
			return ret_status;
		}
	}

    ret_status = headers_append_bucket_type(params->bucketContext.bucket_type, 
                            values,len);
    ret_status = headers_append_storage_class(params->bucketContext.storage_class, 
                            values, params, len);
	if (OBS_STATUS_OK != ret_status) 
    {
        COMMLOG(OBS_LOGERROR, "compose_properties err,return %d.\n",ret_status);
        return ret_status;
    }
	
    if (params->bucketContext.epid != NULL) {
        ret_status = headers_append_epid(params->bucketContext.epid, values, params, len);
    }
    return ret_status;
}

obs_status request_compose_encrypt_params_s3(request_computed_values *values, const request_params *params, int *len)
{
    obs_status status = OBS_STATUS_OK;
    if(params->encryption_params->encryption_type == OBS_ENCRYPTION_KMS) {
        if(params->encryption_params->kms_server_side_encryption) {
            if ((status = headers_append(len, values, 1, 
                             "x-amz-server-side-encryption: %s", 
                             params->encryption_params->kms_server_side_encryption, NULL)) != OBS_STATUS_OK) {
                return status;
            }
        }
        if(params->encryption_params->kms_key_id)
            if ((status = headers_append(len, values, 1, 
                             "x-amz-server-side-encryption-aws-kms-key-id: %s", 
                             params->encryption_params->kms_key_id, NULL)) != OBS_STATUS_OK) {
                return status;
        }
    }
    
    if(params->encryption_params->encryption_type == OBS_ENCRYPTION_SSEC) {
        if(params->encryption_params->ssec_customer_algorithm)
            if ((status = headers_append(len, values, 1, 
                                 "x-amz-server-side-encryption-customer-algorithm: %s", 
                                 params->encryption_params->ssec_customer_algorithm, NULL)) != OBS_STATUS_OK) {
                return status;
        }
        if(params->encryption_params->ssec_customer_key)
        {    
            if ((status = headers_append(len, values, 1, 
                             "x-amz-server-side-encryption-customer-key: %s", 
                             params->encryption_params->ssec_customer_key, NULL)) != OBS_STATUS_OK) {
                return status;
            }
            char buffer[SSEC_KEY_MD5_LENGTH] = {0};
            char ssec_key_md5[SSEC_KEY_MD5_LENGTH]={0};
            base64Decode(params->encryption_params->ssec_customer_key,
                strlen(params->encryption_params->ssec_customer_key),buffer, SSEC_KEY_MD5_LENGTH);
            compute_md5(buffer,strlen(buffer),ssec_key_md5);
            if ((status = headers_append(len, values,1, 
                             "x-amz-server-side-encryption-customer-key-md5: %s", 
                             ssec_key_md5, NULL)) !=OBS_STATUS_OK) {
                return status;
            }
        }
        if(params->encryption_params->des_ssec_customer_algorithm)
            if ((status = headers_append(len, values, 1, 
                             "x-amz-copy-source-server-side-encryption-customer-algorithm: %s",
                             params->encryption_params->des_ssec_customer_algorithm, NULL)) != OBS_STATUS_OK) {
            return status;
        }
        if(params->encryption_params->des_ssec_customer_key)
        { 
            if ((status = headers_append(len, values, 1,
                             "x-amz-copy-source-server-side-encryption-customer-key: %s", 
                             params->encryption_params->des_ssec_customer_key, NULL)) != OBS_STATUS_OK) {
                return status;
            }
            char buffer[SSEC_KEY_MD5_LENGTH] = {0};
            char ssec_key_md5[SSEC_KEY_MD5_LENGTH]={0};
            base64Decode(params->encryption_params->ssec_customer_key,
                strlen(params->encryption_params->ssec_customer_key),buffer, SSEC_KEY_MD5_LENGTH);
            compute_md5(buffer,strlen(buffer),ssec_key_md5);
            status = headers_append(len, values,1, 
                             "x-amz-copy-source-server-side-encryption-customer-key-md5: %s", 
                             ssec_key_md5, NULL);
        }
    }
    return status;
}

obs_status request_compose_encrypt_params_obs(request_computed_values *values, const request_params *params, int *len)
{
    obs_status status = OBS_STATUS_OK;
    if(params->encryption_params->encryption_type == OBS_ENCRYPTION_KMS) {
        if(params->encryption_params->kms_server_side_encryption) {
            if ((status = headers_append(len, values, 1, 
                             "x-obs-server-side-encryption: %s", 
                             params->encryption_params->kms_server_side_encryption, NULL)) != OBS_STATUS_OK) {
                return status;
            }
        }
        if(params->encryption_params->kms_key_id)
            if ((status = headers_append(len, values, 1, 
                             "x-obs-server-side-encryption-aws-kms-key-id: %s", 
                             params->encryption_params->kms_key_id, NULL)) != OBS_STATUS_OK) {
                return status;
        }
    }
    
    if(params->encryption_params->encryption_type == OBS_ENCRYPTION_SSEC) {
        if(params->encryption_params->ssec_customer_algorithm)
            if ((status = headers_append(len, values, 1, 
                                 "x-obs-server-side-encryption-customer-algorithm: %s", 
                                 params->encryption_params->ssec_customer_algorithm, NULL)) != OBS_STATUS_OK) {
                return status;
        }
        if(params->encryption_params->ssec_customer_key)
        {    
            if ((status = headers_append(len, values, 1, 
                             "x-obs-server-side-encryption-customer-key: %s", 
                             params->encryption_params->ssec_customer_key, NULL)) != OBS_STATUS_OK) {
                return status;
            }
            char buffer[SSEC_KEY_MD5_LENGTH] = {0};
            char ssec_key_md5[SSEC_KEY_MD5_LENGTH]={0};
            base64Decode(params->encryption_params->ssec_customer_key,
                strlen(params->encryption_params->ssec_customer_key),buffer, SSEC_KEY_MD5_LENGTH);
            compute_md5(buffer,strlen(buffer),ssec_key_md5);
            if ((status = headers_append(len, values,1, 
                             "x-obs-server-side-encryption-customer-key-md5: %s", 
                             ssec_key_md5, NULL)) !=OBS_STATUS_OK) {
                return status;
            }
        }
        if(params->encryption_params->des_ssec_customer_algorithm)
            if ((status = headers_append(len, values, 1, 
                             "x-obs-copy-source-server-side-encryption-customer-algorithm: %s",
                             params->encryption_params->des_ssec_customer_algorithm, NULL)) != OBS_STATUS_OK) {
            return status;
        }
        if(params->encryption_params->des_ssec_customer_key)
        { 
            if ((status = headers_append(len, values, 1,
                             "x-obs-copy-source-server-side-encryption-customer-key: %s", 
                             params->encryption_params->des_ssec_customer_key, NULL)) != OBS_STATUS_OK) {
                return status;
            }
            char buffer[SSEC_KEY_MD5_LENGTH] = {0};
            char ssec_key_md5[SSEC_KEY_MD5_LENGTH]={0};
            base64Decode(params->encryption_params->ssec_customer_key,
                strlen(params->encryption_params->ssec_customer_key),buffer, SSEC_KEY_MD5_LENGTH);
            compute_md5(buffer,strlen(buffer),ssec_key_md5);
            status = headers_append(len, values,1, 
                             "x-obs-copy-source-server-side-encryption-customer-key-md5: %s", 
                             ssec_key_md5, NULL);
        }
    }
    return status;
}

obs_status request_compose_encrypt_params(request_computed_values *values, const request_params *params, int *len)
{
    if (params->use_api == OBS_USE_API_S3) {
        return request_compose_encrypt_params_s3(values, params, len);
    } else {
        return request_compose_encrypt_params_obs(values, params, len);
    }
}

obs_status request_compose_cors_conf(request_computed_values *values, const request_params *params, int *len)
{
    obs_status status=OBS_STATUS_OK;
    const obs_cors_conf *corsConf = params->corsConf;
    if(corsConf->origin){
        if ((status = headers_append(len, values, 1,"Origin: %s",corsConf->origin, NULL)) != OBS_STATUS_OK) {
            return status;
        }
    }
    unsigned int i;
    for(i = 0; i < corsConf->rmNumber; i++)
    {
        if ((status = headers_append(len, values, 1,"Access-Control-Request-Method: %s",
                         corsConf->requestMethod[i], NULL)) != OBS_STATUS_OK) {
            return status;
        }
    }
    for(i = 0; i < corsConf->rhNumber; i++)
    {
        if ((status = headers_append(len, values, 1,"Access-Control-Request-Headers: %s",
                         corsConf->requestHeader[i], NULL)) != OBS_STATUS_OK) {
            return status;
        }
    }
    return status;
}

obs_status request_compose_data(request_computed_values *values, int *len,const request_params *params)
{
    time_t now = time(NULL);
    char date[64] = {0};
    struct tm flagTemp;
    struct tm* flag = NULL;
#if defined __GNUC__ || defined LINUX            
    flag  = gmtime_r(&now,&flagTemp);
#else
    if(_gmtime64_s(&flagTemp,&now)==0) {
         flag = &flagTemp;               
    }           
#endif

    if (flag != NULL) {
         strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", &flagTemp);
    }
    else {
         COMMLOG(OBS_LOGWARN, "in request_compose_data, gmtime failed\n");
    }
    if (params->use_api == OBS_USE_API_S3) {
        if (headers_append(len, values, 1, "x-amz-date: %s", date, NULL) != OBS_STATUS_OK){
            return headers_append(len, values, 1, "x-amz-date: %s", date, NULL);
        }
    } else {
        if (headers_append(len, values, 1, "x-obs-date: %s", date, NULL) != OBS_STATUS_OK){
            return headers_append(len, values, 1, "x-obs-date: %s", date, NULL);
        }
    }
    return OBS_STATUS_OK;
}

obs_status request_compose_token_and_httpcopy_s3(request_computed_values *values, const request_params *params, int *len)
{
    obs_status status = OBS_STATUS_OK;	
    const obs_put_properties *properties = params->put_properties;
    obs_bucket_context bucketContext = params->bucketContext;
    if((bucketContext.token)&&(bucketContext.token[0]))
    {
        if ((status = headers_append(len, values, 1, "x-amz-security-token: %s",bucketContext.token, NULL)) != OBS_STATUS_OK) {
            return status;
        }
    }
    if (params->httpRequestType == http_request_type_copy) {
        if (params->copySourceBucketName && params->copySourceBucketName[0] &&
            params->copySourceKey && params->copySourceKey[0]) {
            if ((status = headers_append(len, values, 1, "x-amz-copy-source: /%s/%s",
                           params->copySourceBucketName,
                           params->copySourceKey)) != OBS_STATUS_OK) {
                return status;
            }
        }
        if (properties && 0 != properties->meta_data_count) {
            if ((status = headers_append(len, values, 1, "%s", "x-amz-metadata-directive: REPLACE", NULL)) != OBS_STATUS_OK) {
                return status;
            }
        }
    }
	else if(params->subResource != NULL && !strcmp(params->subResource, "metadata") && params->put_properties && params->put_properties->metadata_action == OBS_REPLACE)
	{
		if ((status = headers_append(len, values, 1, "%s", "x-amz-metadata-directive: REPLACE", NULL)) != OBS_STATUS_OK) {
			return status;
		}
	}
	else if(params->subResource != NULL && !strcmp(params->subResource, "metadata") && params->put_properties && params->put_properties->metadata_action == OBS_REPLACE_NEW)
	{
		if ((status = headers_append(len, values, 1, "%s", "x-amz-metadata-directive: REPLACE_NEW", NULL)) != OBS_STATUS_OK) {
			return status;
		}
	}
    return status;
}

obs_status request_compose_token_and_httpcopy_obs(request_computed_values *values, const request_params *params, int *len)
{
    obs_status status = OBS_STATUS_OK;	
    const obs_put_properties *properties = params->put_properties;
    obs_bucket_context bucketContext = params->bucketContext;
    if((bucketContext.token)&&(bucketContext.token[0]))
    {
        if ((status = headers_append(len, values, 1, "x-obs-security-token: %s",bucketContext.token, NULL)) != OBS_STATUS_OK) {
            return status;
        }
    }
    if (params->httpRequestType == http_request_type_copy) {
        if (params->copySourceBucketName && params->copySourceBucketName[0] &&
            params->copySourceKey && params->copySourceKey[0]) {
            if ((status = headers_append(len, values, 1, "x-obs-copy-source: /%s/%s",
                           params->copySourceBucketName,
                           params->copySourceKey)) != OBS_STATUS_OK) {
                return status;
            }
        }
        if (properties && 0 != properties->meta_data_count) {
            if ((status = headers_append(len, values, 1, "%s", "x-obs-metadata-directive: REPLACE", NULL)) != OBS_STATUS_OK) {
                return status;
            }
        }
    }
	else if(params->subResource != NULL && !strcmp(params->subResource, "metadata") && params->put_properties && params->put_properties->metadata_action == OBS_REPLACE)
	{
		if ((status = headers_append(len, values, 1, "%s", "x-obs-metadata-directive: REPLACE", NULL)) != OBS_STATUS_OK) {
			return status;
		}
	}
	else if(params->subResource != NULL && !strcmp(params->subResource, "metadata") && params->put_properties && params->put_properties->metadata_action == OBS_REPLACE_NEW)
	{
		if ((status = headers_append(len, values, 1, "%s", "x-obs-metadata-directive: REPLACE_NEW", NULL)) != OBS_STATUS_OK) {
			return status;
		}
	}
	
    return status;
}

obs_status request_compose_token_and_httpcopy(request_computed_values *values, const request_params *params, int *len)
{
    if (params->use_api == OBS_USE_API_S3) {
        return request_compose_token_and_httpcopy_s3(values, params, len);
    } else {
        return request_compose_token_and_httpcopy_obs(values, params, len);
    }
}

obs_status compose_put_header(const request_params *params,
                            request_computed_values *values)
{
    do_put_header(params, values, "Cache-Control: %s", cache_control, cacheControlHeader,
                  OBS_STATUS_BadCacheControl, OBS_STATUS_CacheControlTooLong);
    do_put_header(params, values, "Content-Type: %s", content_type, contentTypeHeader,
                  OBS_STATUS_BadContentType, OBS_STATUS_ContentTypeTooLong);
    do_put_header(params, values, "Content-MD5: %s", md5, md5Header, OBS_STATUS_BadMd5,
                  OBS_STATUS_Md5TooLong);
    do_put_header(params, values, "Content-Disposition: attachment; file_name=\"%s\"",
                  content_disposition_filename, contentDispositionHeader,
                  OBS_STATUS_BadContentDispositionFilename,
                  OBS_STATUS_ContentDispositionFilenameTooLong);
    do_put_header(params, values, "Content-Encoding: %s", content_encoding,
                  contentEncodingHeader, OBS_STATUS_BadContentEncoding,
                  OBS_STATUS_ContentEncodingTooLong);
    if (params->put_properties && (params->put_properties->expires >= 0)) {
        time_t t = (time_t) params->put_properties->expires;
        struct tm *flag = gmtime(&t);
        if(flag != NULL){
            strftime(values->expiresHeader, sizeof(values->expiresHeader),
                    "Expires: %a, %d %b %Y %H:%M:%S UTC", flag);
        }
    }
    else {
        values->expiresHeader[0] = 0;
    }
    if (params->use_api == OBS_USE_API_S3) {
        do_put_header(params, values, "x-amz-website-redirect-location: %s", website_redirect_location,
                      websiteredirectlocationHeader, OBS_STATUS_BadContentEncoding,
                      OBS_STATUS_ContentEncodingTooLong);
    } else {
        do_put_header(params, values, "x-obs-website-redirect-location: %s", website_redirect_location,
                      websiteredirectlocationHeader, OBS_STATUS_BadContentEncoding,
                      OBS_STATUS_ContentEncodingTooLong);
    }
    return OBS_STATUS_OK;
}

obs_status compose_get_put_header_s3(const request_params *params,
                            request_computed_values *values)
{
    int is_true1 = 0;
    int is_true2 = 0;
    is_true1 = (params->get_conditions && (params->get_conditions->if_modified_since >= 0));
    is_true2 = (params->put_properties && params->put_properties->get_conditions &&
        (params->put_properties->get_conditions->if_modified_since >= 0));
    if (is_true1) {
        time_t t = (time_t) params->get_conditions->if_modified_since;
        struct tm *flag = gmtime(&t);
        if(flag != NULL){
            strftime(values->ifModifiedSinceHeader, sizeof(values->ifModifiedSinceHeader),
                "If-Modified-Since: %a, %d %b %Y %H:%M:%S UTC", flag);
        }
    }
    else if (is_true2) {
        time_t t = (time_t) params->put_properties->get_conditions->if_modified_since;
        struct tm *flag = gmtime(&t);
        if(flag != NULL){
            strftime(values->ifModifiedSinceHeader, sizeof(values->ifModifiedSinceHeader),
                "x-amz-copy-source-if-modified-since: %a, %d %b %Y %H:%M:%S UTC", flag);
        }
    }
    else {
        values->ifModifiedSinceHeader[0] = 0;
    }

    is_true1 = (params->get_conditions && (params->get_conditions->if_not_modified_since >= 0)); 
    is_true2 = (params->put_properties&&params->put_properties->get_conditions &&
            (params->put_properties->get_conditions->if_not_modified_since >= 0));
    if (is_true1) {
        time_t t = (time_t) params->get_conditions->if_not_modified_since;
        struct tm *flag = gmtime(&t);
        if(flag != NULL)
        {
            strftime(values->ifUnmodifiedSinceHeader, sizeof(values->ifUnmodifiedSinceHeader),
                "If-Unmodified-Since: %a, %d %b %Y %H:%M:%S UTC", flag);
        }
    }
    else if (is_true2) {
      time_t t = (time_t) params->put_properties->get_conditions->if_not_modified_since;

      struct tm *flag = gmtime(&t);
      if(flag != NULL){
        strftime(values->ifUnmodifiedSinceHeader, sizeof(values->ifUnmodifiedSinceHeader),
            "x-amz-copy-source-if-unmodified-since: %a, %d %b %Y %H:%M:%S UTC", flag);
      }
    }
    else {
      values->ifUnmodifiedSinceHeader[0] = 0;
    }
    do_get_header(params, values, "If-Match: %s", if_match_etag, ifMatchHeader,
        OBS_STATUS_BadIfMatchEtag, OBS_STATUS_IfMatchEtagTooLong);
    if(!values->ifMatchHeader[0]){
        do_gp_header(params, values, "x-amz-copy-source-if-match: %s", if_match_etag, ifMatchHeader,
            OBS_STATUS_BadIfMatchEtag, OBS_STATUS_IfMatchEtagTooLong);
    }
    do_get_header(params, values, "If-None-Match: %s", if_not_match_etag, ifNoneMatchHeader,
        OBS_STATUS_BadIfNotMatchEtag, OBS_STATUS_IfNotMatchEtagTooLong);
    if(!values->ifNoneMatchHeader[0]){
        do_gp_header(params, values, "x-amz-copy-source-if-none-match: %s", if_not_match_etag, ifNoneMatchHeader,
            OBS_STATUS_BadIfNotMatchEtag, OBS_STATUS_IfNotMatchEtagTooLong);
    }
    return OBS_STATUS_OK;
}

obs_status compose_get_put_header_obs(const request_params *params,
                            request_computed_values *values)
{
    int is_true1 = 0;
    int is_true2 = 0;
    is_true1 = (params->get_conditions && (params->get_conditions->if_modified_since >= 0));
    is_true2 = (params->put_properties && params->put_properties->get_conditions &&
        (params->put_properties->get_conditions->if_modified_since >= 0));
    if (is_true1) {
        time_t t = (time_t) params->get_conditions->if_modified_since;
        struct tm *flag = gmtime(&t);
        if(flag != NULL){
            strftime(values->ifModifiedSinceHeader, sizeof(values->ifModifiedSinceHeader),
                "If-Modified-Since: %a, %d %b %Y %H:%M:%S UTC", flag);
        }
    }
    else if (is_true2) {
        time_t t = (time_t) params->put_properties->get_conditions->if_modified_since;
        struct tm *flag = gmtime(&t);
        if(flag != NULL){
            strftime(values->ifModifiedSinceHeader, sizeof(values->ifModifiedSinceHeader),
                "x-obs-copy-source-if-modified-since: %a, %d %b %Y %H:%M:%S UTC", flag);
        }
    }
    else {
        values->ifModifiedSinceHeader[0] = 0;
    }

    is_true1 = (params->get_conditions && (params->get_conditions->if_not_modified_since >= 0)); 
    is_true2 = (params->put_properties&&params->put_properties->get_conditions &&
            (params->put_properties->get_conditions->if_not_modified_since >= 0));
    if (is_true1) {
        time_t t = (time_t) params->get_conditions->if_not_modified_since;
        struct tm *flag = gmtime(&t);
        if(flag != NULL)
        {
            strftime(values->ifUnmodifiedSinceHeader, sizeof(values->ifUnmodifiedSinceHeader),
                "If-Unmodified-Since: %a, %d %b %Y %H:%M:%S UTC", flag);
        }
    }
    else if (is_true2) {
      time_t t = (time_t) params->put_properties->get_conditions->if_not_modified_since;

      struct tm *flag = gmtime(&t);
      if(flag != NULL){
        strftime(values->ifUnmodifiedSinceHeader, sizeof(values->ifUnmodifiedSinceHeader),
            "x-obs-copy-source-if-unmodified-since: %a, %d %b %Y %H:%M:%S UTC", flag);
      }
    }
    else {
      values->ifUnmodifiedSinceHeader[0] = 0;
    }
    do_get_header(params, values, "If-Match: %s", if_match_etag, ifMatchHeader,
        OBS_STATUS_BadIfMatchEtag, OBS_STATUS_IfMatchEtagTooLong);
    if(!values->ifMatchHeader[0]){
        do_gp_header(params, values, "x-obs-copy-source-if-match: %s", if_match_etag, ifMatchHeader,
            OBS_STATUS_BadIfMatchEtag, OBS_STATUS_IfMatchEtagTooLong);
    }
    do_get_header(params, values, "If-None-Match: %s", if_not_match_etag, ifNoneMatchHeader,
        OBS_STATUS_BadIfNotMatchEtag, OBS_STATUS_IfNotMatchEtagTooLong);
    if(!values->ifNoneMatchHeader[0]){
        do_gp_header(params, values, "x-obs-copy-source-if-none-match: %s", if_not_match_etag, ifNoneMatchHeader,
            OBS_STATUS_BadIfNotMatchEtag, OBS_STATUS_IfNotMatchEtagTooLong);
    }
    return OBS_STATUS_OK;
}

obs_status compose_get_put_header(const request_params *params,
                            request_computed_values *values)
{
    if (params->use_api == OBS_USE_API_S3) {
        return compose_get_put_header_s3(params, values);
    } else {
        return compose_get_put_header_obs(params, values);
    }
}

obs_status compose_range_header(const request_params *params,
                            request_computed_values *values)
{
    if (params->get_conditions && (params->get_conditions->start_byte || params->get_conditions->byte_count)) {
        if (params->get_conditions->byte_count) {
            snprintf_sec(values->rangeHeader, sizeof(values->rangeHeader),_TRUNCATE,
                     "Range: bytes=%llu-%llu",
                     (unsigned long long) params->get_conditions->start_byte,
                     (unsigned long long) (params->get_conditions->start_byte +
                                           params->get_conditions->byte_count - 1));
        }
        else {
            snprintf_sec(values->rangeHeader, sizeof(values->rangeHeader),_TRUNCATE,
                     "Range: bytes=%llu-",
                     (unsigned long long) params->get_conditions->start_byte);
        }
    }
    else  if (params->put_properties&&( params->put_properties->start_byte || params->put_properties->byte_count)) {
        if (params->use_api == OBS_USE_API_S3) {
            if (params->put_properties->byte_count) {
                snprintf_sec(values->rangeHeader, sizeof(values->rangeHeader),_TRUNCATE,
                    "x-amz-copy-source-range: bytes=%llu-%llu", 
                    (unsigned long long) params->put_properties->start_byte,
                    (unsigned long long) (params->put_properties->start_byte +
                    params->put_properties->byte_count - 1));
            }
            else {
                snprintf_sec(values->rangeHeader, sizeof(values->rangeHeader),_TRUNCATE,
                    "x-amz-copy-source-range: bytes=%llu-",
                    (unsigned long long) params->put_properties->start_byte);
            }
        } else {
            if (params->put_properties->byte_count) {
                snprintf_sec(values->rangeHeader, sizeof(values->rangeHeader),_TRUNCATE,
                    "x-obs-copy-source-range: bytes=%llu-%llu", 
                    (unsigned long long) params->put_properties->start_byte,
                    (unsigned long long) (params->put_properties->start_byte +
                    params->put_properties->byte_count - 1));
            }
            else {
                snprintf_sec(values->rangeHeader, sizeof(values->rangeHeader),_TRUNCATE,
                    "x-obs-copy-source-range: bytes=%llu-",
                    (unsigned long long) params->put_properties->start_byte);
            }
        }
    }
    else {
     values->rangeHeader[0] = 0;
    }

    return OBS_STATUS_OK;
}

void pre_compute_header(const char **sortedHeaders, request_computed_values *values, int *nCount, obs_use_api use_api)
{
    char match_str[7];
    int is_true = 0;
    if (use_api == OBS_USE_API_S3) {
        strcpy(match_str, "x-amz-");
    } else {
        strcpy(match_str, "x-obs-");
    }
    is_true = (0 != values->rangeHeader[0] 
                  && strlen(values->rangeHeader) >= strlen(match_str)
                  && 0 == strncmp(match_str,values->rangeHeader,strlen(match_str)));
    if (is_true) {
        sortedHeaders[*nCount] = values->rangeHeader;
        (*nCount)++;
    }

    is_true = (0 != values->ifModifiedSinceHeader[0]
          && strlen(values->ifModifiedSinceHeader) >= strlen(match_str)
          && 0 == strncmp(match_str,values->ifModifiedSinceHeader,strlen(match_str)));
    if (is_true) {
        sortedHeaders[*nCount] = values->ifModifiedSinceHeader;
        (*nCount)++;
    }

    is_true = (0 != values->ifUnmodifiedSinceHeader[0] 
      && strlen(values->ifUnmodifiedSinceHeader) >= strlen(match_str)
      && 0 == strncmp(match_str,values->ifUnmodifiedSinceHeader,strlen(match_str)));
    if (is_true) {
        sortedHeaders[*nCount] = values->ifUnmodifiedSinceHeader;
        (*nCount)++;
    }

    is_true = (0 != values->ifMatchHeader[0] 
      && strlen(values->ifMatchHeader) >= strlen(match_str)
      && 0 == strncmp(match_str,values->ifMatchHeader,strlen(match_str)));
    
    if (is_true) {
        sortedHeaders[*nCount] = values->ifMatchHeader;
        (*nCount)++;
    }

    is_true = (0 != values->ifNoneMatchHeader[0] 
      && strlen(values->ifNoneMatchHeader) >= strlen(match_str)
      && 0 == strncmp(match_str,values->ifNoneMatchHeader,strlen(match_str)));
    if (is_true) {
        sortedHeaders[*nCount] = values->ifNoneMatchHeader;
        (*nCount)++;
    }

    is_true = (0 != values->websiteredirectlocationHeader[0] 
      && strlen(values->websiteredirectlocationHeader) >= strlen(match_str)
      && 0 == strncmp(match_str,values->websiteredirectlocationHeader,strlen(match_str)));
    if (is_true) {
        sortedHeaders[*nCount] = values->websiteredirectlocationHeader;
        (*nCount)++;
    }

    is_true = (0 != values->tokenHeader[0] 
      && strlen(values->tokenHeader) >= strlen(match_str)
      && 0 == strncmp(match_str,values->tokenHeader,strlen(match_str)));
    if (is_true) {
        sortedHeaders[*nCount] = values->tokenHeader;
        (*nCount)++;
    }
}

void canonicalize_headers(request_computed_values *values, const char **sortedHeaders, int nCount)
{
    int lastHeaderLen = 0, i;
    char *buffer = values->canonicalizedAmzHeaders;
    for (i = 0; i < nCount; i++) {
        const char *header = sortedHeaders[i];
        const char *c = header;
        if ((i > 0) &&
            !strncmp(header, sortedHeaders[i - 1], lastHeaderLen)) {
            *(buffer - 1) = ',';
            c += (lastHeaderLen + 1);
        }
        else {
            while (*c != ' ') {
                *buffer++ = *c++;
            }
            lastHeaderLen = c - header;
            c++;
        }
        while (*c) {
            if ((*c == '\r') && (*(c + 1) == '\n') && is_blank(*(c + 2))) {
                c += 3;
                while (is_blank(*c)) {
                    c++;
                }
                while (is_blank(*(buffer - 1))) {
                    buffer--;
                }
                continue;
            }
            *buffer++ = *c++;
        }
        *buffer++ = '\n';
    }
    *buffer = 0;
}

obs_status response_to_status(http_request *request)
{
    switch (request->httpResponseCode) {
        case 0:
            return OBS_STATUS_ConnectionFailed;
        case 301:
            return OBS_STATUS_PermanentRedirect;
        case 307:
            return OBS_STATUS_HttpErrorMovedTemporarily;
        case 400:
            return OBS_STATUS_HttpErrorBadRequest;
        case 403:
            return OBS_STATUS_HttpErrorForbidden;
        case 404:
            return OBS_STATUS_HttpErrorNotFound;
        case 405:
            return OBS_STATUS_MethodNotAllowed;
        case 409:
            return OBS_STATUS_HttpErrorConflict;
        case 411:
            return OBS_STATUS_MissingContentLength;
        case 412:
            return OBS_STATUS_PreconditionFailed;
        case 416:
            return OBS_STATUS_InvalidRange;
        case 500:
            return OBS_STATUS_InternalError;
        case 501:
            return OBS_STATUS_NotImplemented;
        case 503:
            return OBS_STATUS_SlowDown;
        default:
            return OBS_STATUS_HttpErrorUnknown;
	}
}

obs_storage_class get_storage_class_enum_s3(const char * str_storage_class)
{
    if(!strcmp(str_storage_class,"STANDARD"))
    {
        return OBS_STORAGE_CLASS_STANDARD;
    }
    else if(!strcmp(str_storage_class,"STANDARD_IA"))
    {
        return OBS_STORAGE_CLASS_STANDARD_IA;
    }
    else if(!strcmp(str_storage_class,"GLACIER"))
    {
        return OBS_STORAGE_CLASS_GLACIER;
    }
    else
    {
       return OBS_STORAGE_CLASS_BUTT;
    }
}

obs_storage_class get_storage_class_enum_obs(const char * str_storage_class)
{
    if(!strcmp(str_storage_class,"STANDARD"))
    {
        return OBS_STORAGE_CLASS_STANDARD;
    }
    else if(!strcmp(str_storage_class,"WARM"))
    {
        return OBS_STORAGE_CLASS_STANDARD_IA;
    }
    else if(!strcmp(str_storage_class,"COLD"))
    {
        return OBS_STORAGE_CLASS_GLACIER;
    }
    else
    {
       return OBS_STORAGE_CLASS_BUTT;
    }
}

obs_storage_class get_storage_class_enum(const char * str_storage_class, obs_use_api use_api)
{
    if(use_api == OBS_USE_API_S3) {
        return get_storage_class_enum_s3(str_storage_class);
    } else {
        return get_storage_class_enum_obs(str_storage_class);
    }
    
}

obs_status compose_temp_header(const request_params* params,
                               request_computed_values* values,
                               temp_auth_info *stTempAuthInfo)
{
    COMMLOG(OBS_LOGINFO, "enter compose_temp_header successful");
    int is_true = 0;
    char signbuf[17 + 129 + 129 + 64 +
                 (sizeof(values->canonicalizedAmzHeaders) - 1) +
                 (sizeof(values->canonicalizedResource) - 1) + 1] = {0};
    int len = 0;
    int64_t local_expires = 0;
    char * pString = NULL;
        
    local_expires = (params->temp_auth == NULL)?0:params->temp_auth->expires;
    local_expires = (int64_t)(local_expires + time(NULL));
    
    signbuf_attach("%s\n", http_request_type_to_verb(params->httpRequestType));
    signbuf_attach("%s\n", values->md5Header[0] ?
                   & (values->md5Header[sizeof("Content-MD5: ") - 1]) : "");
    signbuf_attach("%s\n", values->contentTypeHeader[0] ?
     & (values->contentTypeHeader[sizeof("Content-Type: ") - 1]) : "");

    signbuf_attach("%lld\n",(long long int)local_expires); 

    pString = values->canonicalizedAmzHeaders;
    is_true = ((pString != NULL)&&(strlen(pString)>0));
    if(is_true)
    {    
        signbuf_attach("%s", pString);
    }    
    pString = values->canonicalizedResource;

    is_true = ((pString != NULL)&&(strlen(pString)>0));
    if(is_true)
    {    
        signbuf_attach("%s", pString);
    }
    
    if ( NULL != params->queryParams)
    {
        const char* pos;
        char tmp[1024] = {0};
        if ((pos = strstr(params->queryParams, "uploadId")) != NULL)
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
            strncpy_sec(tmp, sizeof(tmp), params->queryParams, len1); 
            signbuf_attach("?%s", tmp);
        }
        if ((pos = strstr(params->queryParams, "versionId")) != NULL)
        {
            if (params->subResource)
            {
                signbuf_attach("&%s", params->queryParams);
            }
            else
            {
                signbuf_attach("?%s", params->queryParams);
            }
        }
        if((pos=strstr(params->queryParams,"x-image-process")) != NULL)
        {
            int len1 = pos - params->queryParams;
            const char * pos2 = NULL;
            int len2 = strlen(params->queryParams + len1);
            char * decodedStr = NULL;
            pos2 = strstr(params->queryParams + len1,"&");
            if (pos2 != NULL)
            {
                len2 = pos2 - pos;
            }

            if(len2>0)
            {
                strncpy_sec(tmp, sizeof(tmp),params->queryParams,len2);
                decodedStr = (char*)malloc(len2+1);
				
				if (decodedStr == NULL)
				{
					COMMLOG(OBS_LOGWARN, "compose_temp_header : decodedStr malloc failed!\n");
				}
				
                memset_s(decodedStr,len2+1,0,len2+1);
                urlDecode(decodedStr,tmp,len2+1);
                strncpy_sec(tmp, 1024, decodedStr, strlen(decodedStr)+1);
                CHECK_NULL_FREE(decodedStr);   
                signbuf_attach("?%s", tmp);           
            }
        }
    }
    unsigned char hmac[20] = {0};
    HMAC_SHA1(hmac, (unsigned char*) params->bucketContext.secret_access_key,
              strlen(params->bucketContext.secret_access_key),
              (unsigned char*) signbuf, len);
    char b64[((20 + 1) * 4) / 3] = {0};
    (void)base64Encode(hmac, 20, b64);
    char cUrlEncode[512] = {0};
    (void)urlEncode(cUrlEncode, b64, 28, 0);
    snprintf_sec(stTempAuthInfo->tempAuthParams, 1024, _TRUNCATE,
               "AWSAccessKeyId=%s&Expires=%lld&Signature=%s", params->bucketContext.access_key,
               (long long int)local_expires, cUrlEncode);
    
    snprintf_sec(stTempAuthInfo->temp_auth_headers,1024,_TRUNCATE,"%s",
               values->canonicalizedAmzHeaders);
    COMMLOG(OBS_LOGINFO, "Leave compose_temp_header successful \n");
    return OBS_STATUS_OK;
}
