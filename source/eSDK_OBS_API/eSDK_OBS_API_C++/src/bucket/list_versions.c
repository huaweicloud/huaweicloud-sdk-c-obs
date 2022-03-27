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

static void initialize_list_versions(list_bucket_versions *versions)
{
    string_buffer_initialize(versions->key);
    string_buffer_initialize(versions->version_id);
    string_buffer_initialize(versions->is_latest);
    string_buffer_initialize(versions->last_modified);
    string_buffer_initialize(versions->etag);
    string_buffer_initialize(versions->size);
    string_buffer_initialize(versions->owner_id);
    string_buffer_initialize(versions->owner_display_name);
    string_buffer_initialize(versions->storage_class_value);
    string_buffer_initialize(versions->is_delete);
}

static obs_status make_list_versions_callback(list_versions_data *lvData)
{
    int i;
    obs_status iRet = OBS_STATUS_OK;

    int is_truncated = (!strcmp(lvData->is_truncated, "true") ||
        !strcmp(lvData->is_truncated, "1")) ? 1 : 0;

    obs_list_versions *list_versions_info = (obs_list_versions*)malloc(sizeof(obs_list_versions));
    if (NULL == list_versions_info)
    {
        COMMLOG(OBS_LOGERROR, "Malloc obs_list_versions failed!");
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(list_versions_info, sizeof(obs_list_versions), 0, sizeof(obs_list_versions));

    list_versions_info->versions = (obs_version*)malloc(sizeof(obs_version) * lvData->versions_count);
    if (NULL == list_versions_info->versions)
    {
        COMMLOG(OBS_LOGERROR, "Malloc obs_version failed!");
        CHECK_NULL_FREE(list_versions_info);
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(list_versions_info->versions, sizeof(obs_version) * lvData->versions_count, 0,
        sizeof(obs_version) * lvData->versions_count);

    list_versions_info->common_prefixes = (const char**)malloc(sizeof(char*) * lvData->common_prefixes_count);
    if (NULL == list_versions_info->common_prefixes)
    {
        COMMLOG(OBS_LOGERROR, "Malloc common_prefixes failed!");
        CHECK_NULL_FREE(list_versions_info->versions);
        CHECK_NULL_FREE(list_versions_info);
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(list_versions_info->common_prefixes, sizeof(char*) * lvData->common_prefixes_count, 0,
        sizeof(char*) * lvData->common_prefixes_count);

    list_versions_info->bucket_name = lvData->bucket_name;
    list_versions_info->prefix = lvData->prefix;
    list_versions_info->key_marker = lvData->key_marker;
    list_versions_info->delimiter = lvData->delimiter;
    list_versions_info->max_keys = lvData->max_keys;

    list_versions_info->versions_count = lvData->versions_count;
    for (i = 0; i < list_versions_info->versions_count; i++)
    {
        list_bucket_versions *versionSrc = &(lvData->versions[i]);
        list_versions_info->versions[i].key = versionSrc->key;
        list_versions_info->versions[i].version_id = versionSrc->version_id;
        list_versions_info->versions[i].is_latest = versionSrc->is_latest;

        list_versions_info->versions[i].last_modified = parseIso8601Time(versionSrc->last_modified);
        int nTimeZone = getTimeZone();
        list_versions_info->versions[i].last_modified += nTimeZone * SECONDS_TO_AN_HOUR;

        list_versions_info->versions[i].etag = versionSrc->etag;
        list_versions_info->versions[i].size = parseUnsignedInt(versionSrc->size);
        list_versions_info->versions[i].owner_id = versionSrc->owner_id[0] ? versionSrc->owner_id : 0;
        list_versions_info->versions[i].owner_display_name = versionSrc->owner_display_name[0] ?
            versionSrc->owner_display_name : 0;
        list_versions_info->versions[i].storage_class = versionSrc->storage_class_value[0] ?
            versionSrc->storage_class_value : 0;
        list_versions_info->versions[i].is_delete = versionSrc->is_delete;
    }

    list_versions_info->common_prefixes_count = lvData->common_prefixes_count;
    for (i = 0; i < list_versions_info->common_prefixes_count; i++)
    {
        list_versions_info->common_prefixes[i] = lvData->common_prefixes[i].prefix;
    }

    iRet = (*(lvData->listVersionsCallback))(is_truncated, lvData->next_key_marker,
        lvData->next_versionId_marker, list_versions_info, lvData->callback_data);

    CHECK_NULL_FREE(list_versions_info->common_prefixes);
    CHECK_NULL_FREE(list_versions_info->versions);
    CHECK_NULL_FREE(list_versions_info);

    return iRet;
}

static void initialize_list_versions_data(list_versions_data *lvData)
{
    lvData->versions_count = 0;
    initialize_list_versions(lvData->versions);

    lvData->common_prefixes_count = 0;
    initialize_list_common_prefixes(lvData->common_prefixes);
}

obs_status parse_xml_list_versions(list_versions_data *version_data, const char *element_path,
    const char *data, int data_len)
{
    int fit = 1;
    int ret = 0;

    list_bucket_versions *versions = &(version_data->versions[version_data->versions_count]);
    if (!strcmp(element_path, "ListVersionsResult/NextKeyMarker")) {
        string_buffer_append(version_data->next_key_marker, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/NextVersionIdMarker")) {
        string_buffer_append(version_data->next_versionId_marker, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/IsTruncated")) {
        string_buffer_append(version_data->is_truncated, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Name")) {
        string_buffer_append(version_data->bucket_name, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Prefix")) {
        string_buffer_append(version_data->prefix, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/KeyMarker")) {
        string_buffer_append(version_data->key_marker, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Delimiter")) {
        string_buffer_append(version_data->delimiter, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/MaxKeys")) {
        string_buffer_append(version_data->max_keys, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Version/Key") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/Key"))
    {
#ifdef WIN32
        char* strTmpSource = (char*)malloc(sizeof(char) * (data_len + 1));
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(strTmpSource, sizeof(char) * (data_len + 1), 0, data_len + 1);
        if (ret = strncpy_s(strTmpSource, data_len + 1, data, data_len))
        {
            COMMLOG(OBS_LOGERROR, "in %s line %s strncpy_s error, code is %d.", __FUNCTION__, __LINE__, ret);
            return OBS_STATUS_InternalError;
        }
        char* strTmpOut = UTF8_To_String(strTmpSource);
        string_buffer_append(versions->key, strTmpOut, strlen(strTmpOut), fit);
        CHECK_NULL_FREE(strTmpSource);
        CHECK_NULL_FREE(strTmpOut);
#else
        string_buffer_append(versions->key, data, data_len, fit);
#endif
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Version/VersionId"))
    {
        string_buffer_append(versions->version_id, data, data_len, fit);
        string_buffer_append(versions->is_delete, "false", 5, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/DeleteMarker/VersionId"))
    {
        string_buffer_append(versions->version_id, data, data_len, fit);
        string_buffer_append(versions->is_delete, "true", 4, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Version/IsLatest") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/IsLatest")) {
        string_buffer_append(versions->is_latest, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Version/LastModified") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/LastModified")) {
        string_buffer_append(versions->last_modified, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Version/ETag")) {
        string_buffer_append(versions->etag, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Version/Size")) {
        string_buffer_append(versions->size, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Version/Owner/ID") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/Owner/ID")) {
        string_buffer_append(versions->owner_id, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Version/Owner/DisplayName") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/Owner/DisplayName")) {
        string_buffer_append(versions->owner_display_name, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/Version/StorageClass")) {
        string_buffer_append(versions->storage_class_value, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/CommonPrefixes/Prefix")) {
        string_buffer_append(version_data->common_prefixes[version_data->common_prefixes_count].prefix,
            data, data_len, fit);
    }

    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}


static obs_status list_versions_xml_callback(const char *element_path,
    const char *data, int data_len,
    void *callback_data)
{
    list_versions_data *version_data = (list_versions_data *)callback_data;

    if (data)
    {
        return parse_xml_list_versions(version_data, element_path, data, data_len);

    }

    if (!strcmp(element_path, "ListVersionsResult/Version") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker"))
    {
        // Finished a Version
        version_data->versions_count++;
        if (version_data->versions_count == MAX_VERSIONS) {
            // Make the callback
            obs_status status = make_list_versions_callback(version_data);
            if (OBS_STATUS_OK != status) {
                return status;
            }
            initialize_list_versions_data(version_data);
        }
        else {
            // Initialize the next one
            initialize_list_versions(&(version_data->versions[version_data->versions_count]));
        }
    }

    if (!strcmp(element_path, "ListVersionsResult/CommonPrefixes")) {
        // Finished a commonPrefix
        version_data->common_prefixes_count++;
        if (version_data->common_prefixes_count == MAX_VERSION_COMMON_PREFIXES)
        {
            // Make the callback
            obs_status status = make_list_versions_callback(version_data);
            if (OBS_STATUS_OK != status)
            {
                return status;
            }
            initialize_list_versions_data(version_data);
        }
        else
        {
            initialize_list_common_prefixes(&version_data->common_prefixes[version_data->common_prefixes_count]);
        }
    }

    return OBS_STATUS_OK;
}



static obs_status list_versions_properties_callback(
    const obs_response_properties *responseProperties, void *callback_data)
{
    list_versions_data *version_data = (list_versions_data *)callback_data;
    if (version_data->responsePropertiesCallback)
    {
        return (*(version_data->responsePropertiesCallback))(responseProperties,
            version_data->callback_data);
    }

    return OBS_STATUS_OK;
}

static obs_status list_versions_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    list_versions_data *version_data = (list_versions_data *)callback_data;

    return simplexml_add(&(version_data->simpleXml), buffer, buffer_size);
}


static void list_versions_complete_callback(obs_status requestStatus,
    const obs_error_details *obsErrorDetails, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_versions_data *version_data = (list_versions_data *)callback_data;
    if (version_data->versions_count)
    {
        obs_status callback_result = make_list_versions_callback(version_data);
        if (OBS_STATUS_OK != callback_result)
        {
            COMMLOG(OBS_LOGERROR, "make_list_versions_callback failed (%d)!", callback_result);
        }
    }

    (*(version_data->responseCompleteCallback))(requestStatus, obsErrorDetails, version_data->callback_data);

    simplexml_deinitialize(&(version_data->simpleXml));

    free(version_data);
    version_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


static obs_status set_versions_query_params(const char *prefix, const char *key_marker, const char *delimiter,
    int maxkeys, const char *version_id_marker, char* query_params)
{
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);

    int amp = 0;
    if (delimiter)
    {
        safe_append_status("delimiter", delimiter);
    }

    if (key_marker)
    {
        safe_append_status("key-marker", key_marker);
    }

    if (maxkeys)
    {
        maxkeys = maxkeys > 1000 ? 1000 : maxkeys;
        char maxKeysString[64] = { 0 };
        int ret = snprintf_s(maxKeysString, sizeof(maxKeysString), _TRUNCATE, "%d", maxkeys);
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        safe_append_status("max-keys", maxKeysString);
    }

    if (prefix)
    {
        safe_append_status("prefix", prefix);
    }

    if (version_id_marker)
    {
        safe_append_status("version-id-marker", version_id_marker);
    }

    errno_t err = EOK;
    err = memcpy_s(query_params, QUERY_STRING_LEN, queryParams, QUERY_STRING_LEN);
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    return OBS_STATUS_OK;
}


void list_versions(const obs_options *options, const char *prefix, const char *key_marker, const char *delimiter,
    int maxkeys, const char *version_id_marker, obs_list_versions_handler *handler, void *callback_data)
{
    request_params params;
    char queryParams[QUERY_STRING_LEN + 1] = { 0 };
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "list_bucket_versions start !");
    if (version_id_marker && !strlen(version_id_marker))
    {
        COMMLOG(OBS_LOGERROR, "version_id_marker is \"\"!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    obs_status ret_status = set_versions_query_params(prefix, key_marker, delimiter, maxkeys,
        version_id_marker, queryParams);
    if (OBS_STATUS_OK != ret_status)
    {
        (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "set_query_params return %d !", ret_status);
        return;
    }

    list_versions_data *lvData = (list_versions_data *)malloc(sizeof(list_versions_data));
    if (!lvData) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc list_versions_data failed !");
        return;
    }
    memset_s(lvData, sizeof(list_versions_data), 0, sizeof(list_versions_data));

    simplexml_initialize(&(lvData->simpleXml), &list_versions_xml_callback, lvData);

    lvData->responsePropertiesCallback = handler->response_handler.properties_callback;
    lvData->listVersionsCallback = handler->list_versions_callback;
    lvData->responseCompleteCallback = handler->response_handler.complete_callback;
    lvData->callback_data = callback_data;

    string_buffer_initialize(lvData->is_truncated);
    string_buffer_initialize(lvData->next_key_marker);
    string_buffer_initialize(lvData->next_versionId_marker);
    initialize_list_versions_data(lvData);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &list_versions_properties_callback;
    params.fromObsCallback = &list_versions_data_callback;
    params.complete_callback = &list_versions_complete_callback;
    params.callback_data = lvData;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.temp_auth = options->temp_auth;
    params.subResource = "versions";
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "list_bucket_versions finish!");
}
