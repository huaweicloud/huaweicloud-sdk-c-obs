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
#include "bucket.h"
#include "request_util.h"
#include <openssl/md5.h> 
#define OBS_MAX_PREFIX_SIZE 65536

obs_status parse_xml_list_objects(list_objects_data *lbData, const char *element_path,
    const char *data, int data_len)
{
    int fit = 1;
    int ret = 0;

    if (!strcmp(element_path, "ListBucketResult/IsTruncated")) {
        string_buffer_append(lbData->is_truncated, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/NextMarker")) {
        string_buffer_append(lbData->next_marker, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/Key"))
    {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);

#ifdef WIN32
        int strTmpSourceLen = data_len + 1;
        if (strTmpSourceLen <= 0 || strTmpSourceLen > OBS_MAX_PREFIX_SIZE){
            COMMLOG(OBS_LOGERROR, "parameter of malloc is out of range in function: %s,line %d", __FUNCTION__, __LINE__);
            return OBS_STATUS_OutOfMemory;
        }
        char* strTmpSource = (char*)malloc(sizeof(char) * strTmpSourceLen);
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(strTmpSource, strTmpSourceLen, 0, strTmpSourceLen);
        if (ret = strncpy_s(strTmpSource, strTmpSourceLen, data, data_len))
        {
            COMMLOG(OBS_LOGERROR, "in %s line %d strncpy_s error, code is %d.", __FUNCTION__, __LINE__, ret);
            CHECK_NULL_FREE(strTmpSource);
            return OBS_STATUS_InternalError;
        }
        char* strTmpOut = UTF8_To_String(strTmpSource);
        string_buffer_append(contents->key, strTmpOut, strlen(strTmpOut), fit);
        CHECK_NULL_FREE(strTmpSource);
        CHECK_NULL_FREE(strTmpOut);
#else
        string_buffer_append(contents->key, data, data_len, fit);
#endif
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/LastModified"))
    {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->last_modified, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/ETag"))
    {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->etag, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/Size")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->size, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/Type")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->type, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/Owner/ID")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->owner_id, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/Owner/DisplayName")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->owner_display_name, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/StorageClass")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->storage_class, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/CommonPrefixes/Prefix")) {
        int which = lbData->common_prefixes_count;
        lbData->commonPrefixLens[which] += data_len;
        if (lbData->commonPrefixLens[which] >= (int)sizeof(lbData->common_prefixes[which]))
        {
            COMMLOG(OBS_LOGERROR, "prefix length more than 1024.");
            return OBS_STATUS_XmlParseFailure;
        }
        int prefix_size = data_len + 1;
        if (prefix_size <= 0 || prefix_size > OBS_MAX_PREFIX_SIZE){
            COMMLOG(OBS_LOGERROR, "parameter of malloc is out of range in function: %s,line %d", __FUNCTION__, __LINE__);
            return OBS_STATUS_OutOfMemory;
        }
        char* common_Prefix = (char*)malloc(sizeof(char) * prefix_size);
        if (NULL == common_Prefix) {
            COMMLOG(OBS_LOGERROR, "In prefix , common_prefixes is NULL.");
            return OBS_STATUS_XmlParseFailure;
        }
        memset_s(common_Prefix, prefix_size, 0, prefix_size);
        int str_ret = 0;
        snprintf_s(common_Prefix, prefix_size, data_len, "%.*s", data_len, data);
        char* strTmpOut = UTF8_To_String(common_Prefix);
        str_ret = strcat_s(lbData->common_prefixes[which], sizeof(lbData->common_prefixes[which]), strTmpOut);
        CHECK_NULL_FREE(common_Prefix);
        CHECK_NULL_FREE(strTmpOut);
        if (str_ret) {
            if (EINVAL == str_ret) {
                COMMLOG(OBS_LOGERROR, "In prefix , common_prefixes is uninit.");
                return OBS_STATUS_XmlParseFailure;
            }
            else {
                COMMLOG(OBS_LOGERROR, "prefix length more than 1024.");
                return OBS_STATUS_XmlParseFailure;
            }
        }
    }

    /* Avoid compiler error about variable set but not used */
    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}

static obs_status make_list_bucket_callback(list_objects_data *lbData)
{

    obs_status iRet = OBS_STATUS_OK;

    int is_truncated = (!strcmp(lbData->is_truncated, "true") ||
        !strcmp(lbData->is_truncated, "1")) ? 1 : 0;

    obs_list_objects_content *contents = NULL;
    int contents_count = 0;
    int i = 0;
    if (lbData->contents_count > 0)
    {
        contents = (obs_list_objects_content*)malloc(sizeof(obs_list_objects_content) * lbData->contents_count);
        if (NULL == contents)
        {
            COMMLOG(OBS_LOGERROR, "Malloc obs_list_objects_content failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(contents, sizeof(obs_list_objects_content) * lbData->contents_count, 0, sizeof(obs_list_objects_content) * lbData->contents_count);  //secure function

        contents_count = lbData->contents_count;
        for (i = 0; i < contents_count; i++)
        {
            obs_list_objects_content *contentDest = &(contents[i]);
            one_object_content *contentSrc = &(lbData->contents[i]);
            contentDest->key = contentSrc->key;
            contentDest->last_modified = parseIso8601Time(contentSrc->last_modified);
            int nTimeZone = getTimeZone();
            contentDest->last_modified += nTimeZone * SECONDS_TO_AN_HOUR;
            contentDest->etag = contentSrc->etag;
            contentDest->size = parseUnsignedInt(contentSrc->size);
            contentDest->owner_id = contentSrc->owner_id[0] ? contentSrc->owner_id : 0;
            contentDest->owner_display_name = (contentSrc->owner_display_name[0] ?
                contentSrc->owner_display_name : 0);
            contentDest->storage_class = (contentSrc->storage_class[0] ?
                contentSrc->storage_class : 0);
            contentDest->type = (contentSrc->type[0] ? contentSrc->type : 0);
        }
    }

    // Make the common prefixes array
    int common_prefixes_count = lbData->common_prefixes_count;
    char **common_prefixes = NULL;
    if (common_prefixes_count > 0)
    {
        common_prefixes = (char**)malloc(sizeof(char *) * common_prefixes_count);
        if (NULL == common_prefixes)
        {
            COMMLOG(OBS_LOGERROR, "Malloc common_prefixes failed!");
            CHECK_NULL_FREE(contents);
            return OBS_STATUS_InternalError;
        }
        memset_s(common_prefixes, sizeof(char *) * common_prefixes_count, 0, sizeof(char *) * common_prefixes_count);

        for (i = 0; i < common_prefixes_count; i++)
        {
            common_prefixes[i] = lbData->common_prefixes[i];
        }
    }

    iRet = (*(lbData->listObjectCallback))(is_truncated, lbData->next_marker, contents_count,
        contents, common_prefixes_count, (const char **)common_prefixes, lbData->callback_data);

    CHECK_NULL_FREE(contents);
    CHECK_NULL_FREE(common_prefixes);

    return iRet;
}

static void initialize_list_objects_contents(one_object_content *contents)
{
    string_buffer_initialize(contents->key);
    string_buffer_initialize(contents->last_modified);
    string_buffer_initialize(contents->etag);
    string_buffer_initialize(contents->size);
    string_buffer_initialize(contents->owner_id);
    string_buffer_initialize(contents->owner_display_name);
    string_buffer_initialize(contents->storage_class);
}

static void initialize_list_objects_data(list_objects_data *lbData)
{
    lbData->contents_count = 0;
    initialize_list_objects_contents(lbData->contents);
    lbData->common_prefixes_count = 0;
    lbData->common_prefixes[0][0] = 0;
    lbData->commonPrefixLens[0] = 0;
}


static obs_status list_objects_xml_callback(const char *element_path,
    const char *data, int data_len,
    void *callback_data)
{
    COMMLOG(OBS_LOGDEBUG, "Enter %s successfully !", __FUNCTION__);
    list_objects_data *lbData = (list_objects_data *)callback_data;

    if (data) {
        return parse_xml_list_objects(lbData, element_path, data, data_len);
    }

    if (!strcmp(element_path, "ListBucketResult/Contents")) {
        // Finished a Contents
        lbData->contents_count++;
        if (lbData->contents_count == MAX_CONTENTS) {
            // Make the callback
            obs_status status = make_list_bucket_callback(lbData);
            if (status != OBS_STATUS_OK) {
                return status;
            }
            initialize_list_objects_data(lbData);
        }
        else {
            // Initialize the next one
            initialize_list_objects_contents(&(lbData->contents[lbData->contents_count]));
        }
    }
    else if (!strcmp(element_path, "ListBucketResult/CommonPrefixes/Prefix")) {
        // Finished a Prefix
        lbData->common_prefixes_count++;
        if (lbData->common_prefixes_count == MAX_COMMON_PREFIXES) {
            // Make the callback
            obs_status status = make_list_bucket_callback(lbData);
            if (status != OBS_STATUS_OK) {
                return status;
            }
            initialize_list_objects_data(lbData);
        }
        else {
            // Initialize the next one
            lbData->common_prefixes[lbData->common_prefixes_count][0] = 0;
            lbData->commonPrefixLens[lbData->common_prefixes_count] = 0;
        }
    }

    return OBS_STATUS_OK;
}

static obs_status set_objects_query_params(const char *prefix, const char *marker, const char *delimiter, int maxkeys,
    char* query_params)
{
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);

    int amp = 0;
    if (delimiter)
    {
        safe_append_status("delimiter", delimiter, strlen(delimiter));
    }

    if (marker)
    {
        safe_append_status("marker", marker, strlen(marker));
    }

    if (maxkeys)
    {
        maxkeys = maxkeys > 1000 ? 1000 : maxkeys;
        char maxKeysString[64] = { 0 };
        int ret = snprintf_s(maxKeysString, sizeof(maxKeysString), _TRUNCATE, "%d", maxkeys);
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        safe_append_status("max-keys", maxKeysString, sizeof(maxKeysString));
    }

    if (prefix)
    {
        safe_append_status("prefix", prefix, strlen(prefix));
    }
    errno_t err = EOK;
    err = memcpy_s(query_params, QUERY_STRING_LEN, queryParams, QUERY_STRING_LEN);
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    return OBS_STATUS_OK;
}

static obs_status list_objects_properties_callback(const obs_response_properties *responseProperties,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    list_objects_data *lbData = (list_objects_data *)callback_data;
    if (lbData->responsePropertiesCallback)
    {
        return (*(lbData->responsePropertiesCallback))(responseProperties, lbData->callback_data);
    }

    return OBS_STATUS_OK;
}

static obs_status list_objects_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_objects_data *lbData = (list_objects_data *)callback_data;

    return simplexml_add(&(lbData->simpleXml), buffer, buffer_size);
}


static void list_objects_complete_callback(obs_status requestStatus,
    const obs_error_details *obsErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_objects_data *lbData = (list_objects_data*)callback_data;
    if (0 == lbData->contents_count)
    {
        COMMLOG(OBS_LOGWARN, "listObjects contents_count = %d !", lbData->contents_count);
    }

    // Make the callback if there is anything
    if ((lbData->contents_count || lbData->common_prefixes_count) && OBS_STATUS_OK == requestStatus)
    {
        requestStatus = make_list_bucket_callback(lbData);
    }

    (*(lbData->responseCompleteCallback))(requestStatus, obsErrorDetails, lbData->callback_data);

    simplexml_deinitialize(&(lbData->simpleXml));

    free(lbData);
    lbData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

    return;
}


void list_bucket_objects(const obs_options *options, const char *prefix, const char *marker, const char *delimiter,
    int maxkeys, obs_list_objects_handler *handler, void *callback_data)
{
    request_params params;
    char queryParams[QUERY_STRING_LEN + 1] = { 0 };
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "list bucket objects start!");

    obs_status ret_status = set_objects_query_params(prefix, marker, delimiter, maxkeys, queryParams);
    if (OBS_STATUS_OK != ret_status)
    {
        (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "set_query_params return %d !", ret_status);
        return;
    }

    list_objects_data *data = (list_objects_data *)malloc(sizeof(list_objects_data));
    if (!data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc ListBucketData failed !");
        return;
    }
    memset_s(data, sizeof(list_objects_data), 0, sizeof(list_objects_data));

    simplexml_initialize(&(data->simpleXml), &list_objects_xml_callback, data);

    data->responsePropertiesCallback = handler->response_handler.properties_callback;
    data->listObjectCallback = handler->list_Objects_callback;
    data->responseCompleteCallback = handler->response_handler.complete_callback;
    data->callback_data = callback_data;

    string_buffer_initialize(data->is_truncated);
    string_buffer_initialize(data->next_marker);
    initialize_list_objects_data(data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &list_objects_properties_callback;
    params.fromObsCallback = &list_objects_data_callback;
    params.complete_callback = &list_objects_complete_callback;
    params.callback_data = data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "list bucket objects finish !");
}