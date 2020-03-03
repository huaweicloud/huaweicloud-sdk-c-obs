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
#include <ctype.h>
#include <string.h>
#include "request.h"
#include "simplexml.h"
#include "util.h"
#include "log.h"
#include "securec.h"
#include <libxml/parser.h>
#include <curl/curl.h>
#include <openssl/md5.h> 

#if defined __GNUC__ || defined LINUX
#include <pthread.h>
#endif

#define CERTIFICATE_SIZE 2048
#define CERTIFICATE_NAME "/client.pem"
#define PATH_LENGTH 1024

char g_ca_info[CERTIFICATE_SIZE] = {0};
obs_protocol g_protocol = OBS_PROTOCOL_HTTPS;

const char *obs_get_status_name(obs_status status)
{
    switch (status) {
#define handlecase(s)                           \
        case OBS_STATUS_##s:                    \
            return #s
        handlecase(OK);
        handlecase(InitCurlFailed);
        handlecase(InternalError);
        handlecase(OutOfMemory);
        handlecase(Interrupted);
        handlecase(QueryParamsTooLong);
        handlecase(FailedToIInitializeRequest);
        handlecase(MetadataHeadersTooLong);
        handlecase(BadContentType);
        handlecase(ContentTypeTooLong);
        handlecase(BadMd5);
        handlecase(Md5TooLong);
        handlecase(BadCacheControl);
        handlecase(CacheControlTooLong);
        handlecase(BadContentDispositionFilename);
        handlecase(ContentDispositionFilenameTooLong);
        handlecase(BadContentEncoding);
        handlecase(ContentEncodingTooLong);
        handlecase(BadIfMatchEtag);
        handlecase(IfMatchEtagTooLong);
        handlecase(BadIfNotMatchEtag);
        handlecase(IfNotMatchEtagTooLong);
        handlecase(UriTooLong);
        handlecase(XmlParseFailure);
        handlecase(UserIdTooLong);
        handlecase(UserDisplayNameTooLong);
        handlecase(EmailAddressTooLong);
        handlecase(GroupUriTooLong);
        handlecase(PermissionTooLong);
        handlecase(TooManyGrants);
        handlecase(BadGrantee);
        handlecase(BadPermission);
        handlecase(XmlDocumentTooLarge);
        handlecase(NameLookupError);
        handlecase(FailedToConnect);
        handlecase(ServerFailedVerification);
        handlecase(ConnectionFailed);
        handlecase(AbortedByCallback);
        handlecase(PartialFile);
        handlecase(InvalidParameter);
        handlecase(NoToken);
        handlecase(AccessDenied);
        handlecase(AccountProblem);
        handlecase(AmbiguousGrantByEmailAddress);
        handlecase(BadDigest);
        handlecase(BucketAlreadyExists);
        handlecase(BucketAlreadyOwnedByYou);
        handlecase(BucketNotEmpty);
        handlecase(CredentialsNotSupported);
        handlecase(CrossLocationLoggingProhibited);
        handlecase(EntityTooSmall);
        handlecase(EntityTooLarge);
        handlecase(ExpiredToken);
        handlecase(IllegalVersioningConfigurationException); 
        handlecase(IncompleteBody);
        handlecase(IncorrectNumberOfFilesInPostRequest);
        handlecase(InlineDataTooLarge);
        handlecase(InvalidAccessKeyId);
        handlecase(InvalidAddressingHeader);
        handlecase(InvalidArgument);
        handlecase(InvalidBucketName);
        handlecase(InvalidKey);
        handlecase(InvalidBucketState);
        handlecase(InvalidDigest);
        handlecase(InvalidLocationConstraint);
        handlecase(InvalidObjectState); 
        handlecase(InvalidPart);
        handlecase(InvalidPartOrder);
        handlecase(InvalidPayer);
        handlecase(InvalidPolicyDocument);
        handlecase(InvalidRange);
        handlecase(InvalidRedirectLocation);
        handlecase(InvalidRequest);
        handlecase(InvalidSecurity);
        handlecase(InvalidSOAPRequest);
        handlecase(InvalidStorageClass);
        handlecase(InvalidTargetBucketForLogging);
        handlecase(InvalidToken);
        handlecase(InvalidURI);
        handlecase(MalformedACLError);
        handlecase(MalformedPolicy);
        handlecase(MalformedPOSTRequest);
        handlecase(MalformedXML);
        handlecase(MaxMessageLengthExceeded);
        handlecase(MaxPostPreDataLengthExceededError);
        handlecase(MetadataTooLarge);
        handlecase(MethodNotAllowed);
        handlecase(MissingAttachment);
        handlecase(MissingContentLength);
        handlecase(MissingRequestBodyError);
        handlecase(MissingSecurityElement);
        handlecase(MissingSecurityHeader);
        handlecase(NoLoggingStatusForKey);
        handlecase(NoSuchBucket);
        handlecase(NoSuchKey);
        handlecase(NoSuchLifecycleConfiguration);
        handlecase(NoSuchUpload); 
        handlecase(NoSuchVersion); 
        handlecase(NotImplemented);
        handlecase(NotSignedUp);
        handlecase(NotSuchBucketPolicy);
        handlecase(OperationAborted);
        handlecase(PermanentRedirect);
        handlecase(PreconditionFailed);
        handlecase(Redirect);
        handlecase(RestoreAlreadyInProgress);
        handlecase(RequestIsNotMultiPartContent);
        handlecase(RequestTimeout);
        handlecase(RequestTimeTooSkewed);
        handlecase(RequestTorrentOfBucketError);
        handlecase(SignatureDoesNotMatch);
        handlecase(ServiceUnavailable);
        handlecase(SlowDown);
        handlecase(TemporaryRedirect);
        handlecase(TokenRefreshRequired);
        handlecase(TooManyBuckets);
        handlecase(UnexpectedContent);
        handlecase(UnresolvableGrantByEmailAddress);
        handlecase(UserKeyMustBeSpecified);
        handlecase(ErrorUnknown);    
        handlecase(HttpErrorMovedTemporarily);
        handlecase(HttpErrorBadRequest);
        handlecase(HttpErrorForbidden);
        handlecase(HttpErrorNotFound);
        handlecase(HttpErrorConflict);
        handlecase(InsufficientStorageSpace);
        handlecase(NoSuchWebsiteConfiguration);
        handlecase(NoSuchBucketPolicy);
        handlecase(NoSuchCORSConfiguration);
        handlecase(HttpErrorUnknown);
        handlecase(InArrearOrInsufficientBalance);
        handlecase(NoSuchTagSet);
        handlecase(OpenFileFailed);
        handlecase(EmptyFile);
        handlecase(QuotaTooSmall);
        handlecase(MetadataNameDuplicate);
        handlecase(BUTT);
    }

    return "Unknown";
}


obs_status obs_initialize(int win32_flags)
{
    CURLcode retCode = CURLE_OK;
    obs_status ret = OBS_STATUS_OK;
    
    SYSTEMTIME reqTime;
    GetLocalTime(&reqTime);
    
    LOG_INIT();
    xmlInitParser();
    COMMLOG(OBS_LOGWARN, "%s OBS SDK Version= %s", __FUNCTION__, OBS_SDK_VERSION);
    retCode = curl_global_init(CURL_GLOBAL_ALL);
    if (retCode != CURLE_OK)
    {
        COMMLOG(OBS_LOGWARN, "%s curl_global_init failed retcode = %d", __FUNCTION__,retCode);
        return OBS_STATUS_InitCurlFailed;
    } 
    
    ret = request_api_initialize(win32_flags);

    SYSTEMTIME rspTime;
    GetLocalTime(&rspTime);      
    INTLOG(reqTime, rspTime, ret, "");

    return ret;
}

void init_obs_options(obs_options *options)
{
    options->request_options.speed_time = DEFAULT_LOW_SPEED_TIME_S;
    options->request_options.max_connected_time = DEFAULT_TIMEOUT_S;
    options->request_options.connect_time = DEFAULT_CONNECTTIMEOUT_MS;
    options->request_options.speed_limit = DEFAULT_LOW_SPEED_LIMIT;
    options->request_options.proxy_auth = NULL;
    options->request_options.proxy_host = NULL;
    options->request_options.ssl_cipher_list = NULL;
    options->request_options.http2_switch = OBS_HTTP2_CLOSE;
    options->request_options.bbr_switch = OBS_BBR_CLOSE;
	options->request_options.auth_switch = OBS_NEGOTIATION_TYPE;
    options->request_options.buffer_size = 16 * 1024L;
        
    options->bucket_options.access_key = NULL;
    options->bucket_options.secret_access_key =NULL;
    options->bucket_options.bucket_name = NULL;
    options->bucket_options.certificate_info = g_ca_info[0] ? g_ca_info : NULL;
    options->bucket_options.host_name = NULL;
    options->bucket_options.protocol = g_protocol;
    options->bucket_options.storage_class = OBS_STORAGE_CLASS_STANDARD;
    options->bucket_options.token = NULL;
    options->bucket_options.uri_style = OBS_URI_STYLE_VIRTUALHOST;
    options->bucket_options.epid = NULL;
    options->temp_auth = NULL;
}

obs_status init_certificate_by_path(obs_protocol protocol, obs_certificate_conf ca_conf, 
                                    const char *path, int path_length)
{
    char ca_path[PATH_LENGTH] = {0};
    obs_status status = OBS_STATUS_OK;
    int length = 0;
    
    g_protocol =  protocol;
    if (OBS_PROTOCOL_HTTP == protocol)
    {
        return status;
    }

    memset_s(ca_path, PATH_LENGTH, 0, PATH_LENGTH);
    if (OBS_DEFAULT_CERTIFICATE == ca_conf)
    {
    #if defined __GNUC__ || defined LINUX
        getCurrentPath(ca_path);
        strcat_s(ca_path, PATH_LENGTH, CERTIFICATE_NAME);
    #else
        GetModuleFileNameA(NULL,ca_path,MAX_MSG_SIZE-1);
        if(NULL != strrchr(ca_path, '\\')){ 
            *(strrchr(ca_path, '\\') + 1) = '\0'; 
        }
        strcat_s(ca_path, PATH_LENGTH, "client.crt");
    #endif
    }
    else if ((OBS_DEFINED_CERTIFICATE == ca_conf) && path && (path_length > 0))
    {
        memcpy_s(ca_path, sizeof(ca_path), path, path_length);
    }
    else
    {
        return OBS_STATUS_InvalidParameter;
    }
        
    FILE *fp = fopen(ca_path, "r");
    if (!fp)
    {
        COMMLOG(OBS_LOGERROR, "fopen failed path = %s", ca_path);
        return OBS_STATUS_OpenFileFailed;  
    }
    while(1)
    {
        int rc = fread(g_ca_info, sizeof(char), CERTIFICATE_SIZE, fp);
        if (rc <= 0)
        {
            break; 
        }
        length += rc;
    }
    fclose(fp);
    
    if (length <= 0)
    {
        COMMLOG(OBS_LOGERROR, "fread failed length = %d\n", length);
        return OBS_STATUS_EmptyFile;  
    }
    return status;
}

obs_status init_certificate_by_buffer(const char *buffer, int buffer_length)
{
    if (NULL == buffer)
    {
        return OBS_STATUS_InvalidArgument;
    }
    if (buffer_length > (int)sizeof(g_ca_info))
    {
        return OBS_STATUS_InvalidArgument;
    }
    
    memcpy_s(&g_ca_info, sizeof(g_ca_info), buffer, buffer_length);
    g_protocol = OBS_PROTOCOL_HTTPS;
    return OBS_STATUS_OK;
}


void init_put_properties(obs_put_properties *put_properties)
{
    put_properties->byte_count=0;
    put_properties->cache_control=0;
    put_properties->canned_acl= OBS_CANNED_ACL_PRIVATE;
    put_properties->content_disposition_filename = 0;
    put_properties->content_encoding = 0;
    put_properties->content_type =0;
    put_properties->expires = -1;
    put_properties->file_object_config=0;
    put_properties->get_conditions=0;
    put_properties->md5=0;
    put_properties->meta_data=0;
    put_properties->meta_data_count=0;
    put_properties->start_byte=0;
    put_properties->website_redirect_location=0;
    put_properties->domain_config = NULL;
	put_properties->metadata_action = OBS_NO_METADATA_ACTION;
}

void init_get_properties(obs_get_conditions *get_conditions)
{
    get_conditions->byte_count=0;
    get_conditions->start_byte=0;
    get_conditions->if_match_etag=NULL;
    get_conditions->if_modified_since = -1;
    get_conditions->if_not_match_etag = NULL;
    get_conditions->if_not_modified_since = -1;
    get_conditions->image_process_config = NULL;
}


void obs_deinitialize()
{
    LOG_EXIT();
    request_api_deinitialize();
    xmlCleanupParser();
    curl_global_cleanup();
}

int obs_status_is_retryable(obs_status status) //lint !e578
{
    switch (status) {
        case OBS_STATUS_NameLookupError:
        case OBS_STATUS_FailedToConnect:
        case OBS_STATUS_ConnectionFailed:
        case OBS_STATUS_InternalError:
        case OBS_STATUS_AbortedByCallback:
        case OBS_STATUS_RequestTimeout:
        case OBS_STATUS_PartialFile:
        case OBS_STATUS_NoToken:
            return 1;
        default:
            return 0;
    }
}

void compute_md5(const char *buffer, int64_t buffer_size, char *outbuffer)
{
    unsigned char buffer_md5[16] = {0};
    char base64_md5[64] = {0};
    MD5((unsigned char*)buffer, (size_t)buffer_size, buffer_md5);
    base64Encode(buffer_md5, sizeof(buffer_md5), base64_md5);
    strcpy(outbuffer,base64_md5);
}

obs_status set_online_request_max_count(uint32_t online_request_max)
{
    if (online_request_max <= 0)
    {
        return OBS_STATUS_InvalidParameter;
    }
    init_request_most_count(online_request_max);
    return OBS_STATUS_OK;
}


