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
#include <string.h>
#include "error_parser.h"
#include "request_util.h"
#include "json_error_alias.h"

#ifdef WIN32
# pragma warning (disable:4127)
#endif
static obs_status parse_error_extra_detail(error_parser *errorParser,
    const char *element_path, const char *data, int data_len)
{
    if (strncmp(element_path, "Error/", sizeof("Error/") - 1)) {
        return OBS_STATUS_OK;
    }
    const char *elementName = &(element_path[sizeof("Error/") - 1]);
    if (errorParser->obsErrorDetails.extra_details_count &&
        !strcmp(elementName, errorParser->obsErrorDetails.extra_details[errorParser->obsErrorDetails.extra_details_count - 1].name)) {
        int fit;
        string_multibuffer_append(errorParser->extraDetailsNamesValues, data, data_len, fit);
        if (!fit) {
            errorParser->obsErrorDetails.extra_details_count--;
        }
        return OBS_STATUS_OK;
    }
    if (errorParser->obsErrorDetails.extra_details_count >= EXTRA_DETAILS_SIZE) {
        return OBS_STATUS_OK;
    }
    if (errorParser->extraDetailsNamesValuesSize + (int)strlen(elementName) + data_len + 2 >= (int)sizeof(errorParser->extraDetailsNamesValues)) {
        return OBS_STATUS_OK;
    }
    int fit;
    char *name = string_multibuffer_current(errorParser->extraDetailsNamesValues);
    int nameLen = strlen(elementName);
    string_multibuffer_add(errorParser->extraDetailsNamesValues, elementName, nameLen, fit);
    if (!fit) {
        return OBS_STATUS_OK;
    }
    char *value = string_multibuffer_current(errorParser->extraDetailsNamesValues);
    string_multibuffer_add(errorParser->extraDetailsNamesValues, data, data_len, fit);
    if (!fit) {
        return OBS_STATUS_OK;
    }
    obs_name_value *nv = &(errorParser->extra_details[errorParser->obsErrorDetails.extra_details_count++]);
    nv->name = name;
    nv->value = value;

    return OBS_STATUS_OK;
}

static obs_status errorXmlCallback(const char *elementPath, const char *data, int dataLen, void *callback_data)
{
    if (!data) {
        return OBS_STATUS_OK;
    }

    error_parser *errorParser = (error_parser *)callback_data;

    int fit;

    COMMLOG(OBS_LOGERROR, "%s errorXml : %s : %.*s", __FUNCTION__, elementPath, dataLen, data);

    if (!strcmp(elementPath, "Error")) {
    } else if (!strcmp(elementPath, "Error/Code")) {
        string_buffer_append(errorParser->code, data, dataLen, fit);
    } else if (!strcmp(elementPath, "Error/Message")) {
        string_buffer_append(errorParser->message, data, dataLen, fit);
        errorParser->obsErrorDetails.message = errorParser->message;
    } else if (!strcmp(elementPath, "Error/Resource")) {
        string_buffer_append(errorParser->resource, data, dataLen, fit);
        errorParser->obsErrorDetails.resource = errorParser->resource;
    } else if (!strcmp(elementPath, "Error/FurtherDetails")) {
        string_buffer_append(errorParser->further_details, data, dataLen, fit);
        errorParser->obsErrorDetails.further_details = errorParser->further_details;
    } else {
        return parse_error_extra_detail(errorParser, elementPath, data, dataLen);
    }

    return OBS_STATUS_OK;
}


void error_parser_initialize(error_parser *errorParser)
{
    errorParser->obsErrorDetails.message = 0;
    errorParser->obsErrorDetails.resource = 0;
    errorParser->obsErrorDetails.further_details = 0;
    errorParser->obsErrorDetails.extra_details_count = 0;
    errorParser->obsErrorDetails.error_headers_count = 0;
    errorParser->obsErrorDetails.extra_details = errorParser->extra_details;
	errorParser->obsErrorDetails.error_headers = errorParser->error_headers;
    errorParser->errorXmlParserInitialized = 0;
    errorParser->isJsonError = 0;
    errorParser->jsonErrorBuf = NULL;
    errorParser->jsonErrorBufLen = 0;
    errorParser->jsonErrorBufSize = 0;
    string_buffer_initialize(errorParser->code);
    string_buffer_initialize(errorParser->message);
    string_buffer_initialize(errorParser->resource);
    string_buffer_initialize(errorParser->further_details);
    string_multibuffer_initialize(errorParser->extraDetailsNamesValues);
	string_multibuffer_initialize(errorParser->errorHeadersNamesValues);
}


/**
 * @brief 从JSON对象中提取字符串字段并追加到string_buffer
 * (用于code/message/resource字段)
 */
static void parse_json_simple_field(error_parser *errorParser,
                                    cJSON *root, const char *field_name,
                                    const char **detail_ptr)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, field_name);
    if (!item || !cJSON_IsString(item) || !item->valuestring) {
        return;
    }
    int fit;
    if (!strcmp(field_name, "code")) {
        string_buffer_append(errorParser->code, item->valuestring,
                             (int)strlen(item->valuestring), fit);
    } else if (!strcmp(field_name, "message")) {
        string_buffer_append(errorParser->message, item->valuestring,
                             (int)strlen(item->valuestring), fit);
        errorParser->obsErrorDetails.message = errorParser->message;
    } else if (!strcmp(field_name, "resource")) {
        string_buffer_append(errorParser->resource, item->valuestring,
                             (int)strlen(item->valuestring), fit);
        errorParser->obsErrorDetails.resource = errorParser->resource;
    }
    (void)fit;
}

/**
 * @brief 从JSON对象中提取request_id并追加到extra_details
 */
static void parse_json_request_id(error_parser *errorParser, cJSON *root)
{
    cJSON *request_id = cJSON_GetObjectItemCaseSensitive(root, "request_id");
    if (!request_id || !cJSON_IsString(request_id) || !request_id->valuestring) {
        return;
    }
    if (errorParser->obsErrorDetails.extra_details_count >= EXTRA_DETAILS_SIZE) {
        return;
    }
    int req_id_len = (int)strlen(request_id->valuestring);
    if (errorParser->extraDetailsNamesValuesSize + (int)strlen("request_id") + req_id_len + 2
        >= (int)sizeof(errorParser->extraDetailsNamesValues)) {
        return;
    }
    char *name = string_multibuffer_current(errorParser->extraDetailsNamesValues);
    int nameLen = (int)strlen("request_id");
    int fit;
    string_multibuffer_add(errorParser->extraDetailsNamesValues,
                           "request_id", nameLen, fit);
    if (fit) {
        char *value = string_multibuffer_current(errorParser->extraDetailsNamesValues);
        string_multibuffer_add(errorParser->extraDetailsNamesValues,
                               request_id->valuestring,
                               req_id_len, fit);
        if (fit) {
            obs_name_value *nv = &(errorParser->extra_details[
                errorParser->obsErrorDetails.extra_details_count++]);
            nv->name = name;
            nv->value = value;
        }
    }
}

static void parse_json_error(error_parser *errorParser)
{
    if (!errorParser->jsonErrorBuf || errorParser->jsonErrorBufLen <= 0) {
        return;
    }

    cJSON *root = cJSON_ParseWithLength(errorParser->jsonErrorBuf,
                                         (size_t)errorParser->jsonErrorBufLen);
    if (!root) {
        COMMLOG(OBS_LOGERROR, "%s: Failed to parse JSON error response", __FUNCTION__);
        return;
    }

    /* OEF JSON error format: {"message":"...", "code":"...", "request_id":"..."} */
    parse_json_simple_field(errorParser, root, "code", NULL);
    parse_json_simple_field(errorParser, root, "message", NULL);
    parse_json_simple_field(errorParser, root, "resource", NULL);
    parse_json_request_id(errorParser, root);

    cJSON_Delete(root);
}


/**
 * @brief 检测错误响应是否为JSON格式(首字节为'{'，跳过前导空白)
 * @return 1 表示检测到JSON, 0 表示非JSON
 */
static int detect_json_error(const char *buffer, int buffer_size)
{
    const char *p = buffer;
    int remaining = buffer_size;
    while (remaining > 0 && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
        p++;
        remaining--;
    }
    return (remaining > 0 && *p == '{');
}

/**
 * @brief 累积JSON缓冲区数据,必要时扩展缓冲区
 */
static obs_status accumulate_json_buffer(error_parser *errorParser,
                                         const char *buffer, int buffer_size)
{
    if (!errorParser->jsonErrorBuf) {
        errorParser->jsonErrorBufSize = buffer_size + 1;
        errorParser->jsonErrorBuf = (char*)malloc(errorParser->jsonErrorBufSize);
        if (!errorParser->jsonErrorBuf) {
            return OBS_STATUS_OutOfMemory;
        }
        errorParser->jsonErrorBufLen = 0;
    }
    /* Grow buffer if needed */
    if (errorParser->jsonErrorBufLen + buffer_size >= errorParser->jsonErrorBufSize) {
        int new_size = errorParser->jsonErrorBufLen + buffer_size + 1;
        char *new_buf = (char*)malloc(new_size);
        if (!new_buf) {
            return OBS_STATUS_OutOfMemory;
        }
        if (memcpy_s(new_buf, new_size, errorParser->jsonErrorBuf,
                 errorParser->jsonErrorBufLen) != EOK) {
            free(new_buf);
            return OBS_STATUS_Security_Function_Failed;
        }
        free(errorParser->jsonErrorBuf);
        errorParser->jsonErrorBuf = new_buf;
        errorParser->jsonErrorBufSize = new_size;
    }
    if (errorParser->jsonErrorBufLen + buffer_size < errorParser->jsonErrorBufSize) {
        if (memcpy_s(errorParser->jsonErrorBuf + errorParser->jsonErrorBufLen,
                 errorParser->jsonErrorBufSize - errorParser->jsonErrorBufLen,
                 buffer, buffer_size) == EOK) {
            errorParser->jsonErrorBufLen += buffer_size;
        }
    }
    if (errorParser->jsonErrorBufLen < errorParser->jsonErrorBufSize) {
        errorParser->jsonErrorBuf[errorParser->jsonErrorBufLen] = '\0';
    }
    return OBS_STATUS_OK;
}

obs_status error_parser_add(error_parser *errorParser, const char *buffer,
                          int buffer_size)
{/*lint !e101 */
    if (!errorParser->isJsonError && !errorParser->errorXmlParserInitialized) {
        /* Detect if the error response is JSON (OEF proxy returns JSON instead of XML).
         * Standard OBS errors start with '<?xml' or '<Error', while OEF errors start
         * with '{'. Skip leading whitespace for robustness. */
        if (detect_json_error(buffer, buffer_size)) {
            errorParser->isJsonError = 1;
            errorParser->jsonErrorBufSize = buffer_size + 1;
            errorParser->jsonErrorBuf = (char*)malloc(errorParser->jsonErrorBufSize);
            if (errorParser->jsonErrorBuf) {
                if (memcpy_s(errorParser->jsonErrorBuf, errorParser->jsonErrorBufSize,
                         buffer, buffer_size) == EOK) {
                    errorParser->jsonErrorBufLen = buffer_size;
                }
                errorParser->jsonErrorBuf[errorParser->jsonErrorBufLen] = '\0';
            }
            return OBS_STATUS_OK;
        }
    }

    /* If we've already detected JSON, accumulate the rest of the buffer */
    if (errorParser->isJsonError) {
        return accumulate_json_buffer(errorParser, buffer, buffer_size);
    }

    /* Standard XML error path */
    if (!errorParser->errorXmlParserInitialized) {
        simplexml_initialize(&(errorParser->errorXmlParser), &errorXmlCallback,
                             errorParser);/*lint !e119 */
        errorParser->errorXmlParserInitialized = 1;
    }

    return simplexml_add(&(errorParser->errorXmlParser), buffer, buffer_size);
}


void error_parser_convert_status(error_parser *errorParser, obs_status *status)
{
    /* If JSON error was detected, parse it now to extract code and message */
    if (errorParser->isJsonError) {
        parse_json_error(errorParser);
    }

    if (!errorParser->codeLen) {
        return;
    }

#define HANDLE_CODE(name)                                       \
    do {                                                        \
        if (!strcmp(errorParser->code, #name)) {                \
            *status = OBS_STATUS_##name;                      \
            goto code_set;                                      \
        }                                                       \
    } while (0)
    
    HANDLE_CODE(AccessDenied);
    HANDLE_CODE(AccountProblem);
    HANDLE_CODE(AmbiguousGrantByEmailAddress);
    HANDLE_CODE(BadDigest);
    HANDLE_CODE(BucketAlreadyExists);
    HANDLE_CODE(BucketAlreadyOwnedByYou);
    HANDLE_CODE(BucketNotEmpty);
    HANDLE_CODE(CredentialsNotSupported);
    HANDLE_CODE(CrossLocationLoggingProhibited);
    HANDLE_CODE(EntityTooSmall);
    HANDLE_CODE(EntityTooLarge);
    HANDLE_CODE(ExpiredToken);
    HANDLE_CODE(IllegalVersioningConfigurationException); 
    HANDLE_CODE(IncompleteBody);
    HANDLE_CODE(IncorrectNumberOfFilesInPostRequest);
    HANDLE_CODE(InlineDataTooLarge);
    HANDLE_CODE(InternalError);
    HANDLE_CODE(InvalidAccessKeyId);
    HANDLE_CODE(InvalidAddressingHeader);
    HANDLE_CODE(InvalidArgument);
    HANDLE_CODE(InvalidBucketName);
    HANDLE_CODE(InvalidBucketState); 
    HANDLE_CODE(InvalidDigest);
    HANDLE_CODE(InvalidLocationConstraint);
    HANDLE_CODE(InvalidObjectState); 
    HANDLE_CODE(InvalidPart); 
    HANDLE_CODE(InvalidPartOrder);
    HANDLE_CODE(InvalidPayer);
    HANDLE_CODE(InvalidPolicyDocument);
    HANDLE_CODE(InvalidRange);
    HANDLE_CODE(InvalidRedirectLocation);
    HANDLE_CODE(InvalidRequest);
    HANDLE_CODE(InvalidSecurity);
    HANDLE_CODE(InvalidSOAPRequest);
    HANDLE_CODE(InvalidStorageClass);
    HANDLE_CODE(InvalidTargetBucketForLogging);
    HANDLE_CODE(InvalidToken);
    HANDLE_CODE(InvalidURI);
    HANDLE_CODE(MalformedACLError);
    HANDLE_CODE(MalformedPolicy);
    HANDLE_CODE(MalformedPOSTRequest);
    HANDLE_CODE(MalformedXML);
    HANDLE_CODE(MaxMessageLengthExceeded);
    HANDLE_CODE(MaxPostPreDataLengthExceededError);
    HANDLE_CODE(MetadataTooLarge);
    HANDLE_CODE(MethodNotAllowed);
    HANDLE_CODE(MissingAttachment);
    HANDLE_CODE(MissingContentLength);
    HANDLE_CODE(MissingRequestBodyError);
    HANDLE_CODE(MissingSecurityElement);
    HANDLE_CODE(MissingSecurityHeader);
    HANDLE_CODE(NoLoggingStatusForKey);
    HANDLE_CODE(NoSuchBucket);
    HANDLE_CODE(NoSuchKey);
    HANDLE_CODE(NoSuchLifecycleConfiguration);
    HANDLE_CODE(NoSuchUpload);
    HANDLE_CODE(NoSuchVersion);
    HANDLE_CODE(NotImplemented);
    HANDLE_CODE(NotSignedUp);
    HANDLE_CODE(NotSuchBucketPolicy);
    HANDLE_CODE(OperationAborted);
    HANDLE_CODE(PermanentRedirect);
    HANDLE_CODE(PreconditionFailed);
    HANDLE_CODE(Redirect);
    HANDLE_CODE(RestoreAlreadyInProgress);
    HANDLE_CODE(RequestIsNotMultiPartContent);
    HANDLE_CODE(RequestTimeout);
    HANDLE_CODE(RequestTimeTooSkewed);
    HANDLE_CODE(RequestTorrentOfBucketError);
    HANDLE_CODE(SignatureDoesNotMatch);
    HANDLE_CODE(ServiceUnavailable);
    HANDLE_CODE(SlowDown);
    HANDLE_CODE(TemporaryRedirect);
    HANDLE_CODE(TokenRefreshRequired);
    HANDLE_CODE(TooManyBuckets);
    HANDLE_CODE(UnexpectedContent);
    HANDLE_CODE(UnresolvableGrantByEmailAddress);
    HANDLE_CODE(UserKeyMustBeSpecified);
    HANDLE_CODE(InsufficientStorageSpace);
    HANDLE_CODE(NoSuchWebsiteConfiguration);
    HANDLE_CODE(NoSuchCORSConfiguration);
    HANDLE_CODE(InArrearOrInsufficientBalance);
    HANDLE_CODE(NoSuchTagSet);
	HANDLE_CODE(BadAccessLabel);
	HANDLE_CODE(FsNotSupport);
	HANDLE_CODE(AccessLabelNotFound);
	HANDLE_CODE(NoSuchTrashConfiguration);
	HANDLE_CODE(InvalidRequestBody);
	HANDLE_CODE(NoSuchReplicationConfiguration);
	HANDLE_CODE(NoSuchInventoryConfiguration);
	HANDLE_CODE(NoSuchEncryptionConfiguration);
	HANDLE_CODE(NoSuchDisConfiguration);
	HANDLE_CODE(NoSuchCompressConfiguration);

    // JSON error code alias lookup (table defined in json_error_alias.h)
    for (int i = 0; i < g_json_alias_table_size; i++) {
        if (!strcmp(errorParser->code, g_json_alias_table[i].code)) {
            *status = g_json_alias_table[i].status;
            goto code_set;
        }
    }

    *status = OBS_STATUS_ErrorUnknown;

 code_set:

    return;
}

void error_parser_deinitialize(error_parser *errorParser)
{
    if (errorParser->errorXmlParserInitialized) {
        simplexml_deinitialize(&(errorParser->errorXmlParser));
    }
    CHECK_NULL_FREE(errorParser->jsonErrorBuf);
    errorParser->jsonErrorBufLen = 0;
    errorParser->jsonErrorBufSize = 0;
}