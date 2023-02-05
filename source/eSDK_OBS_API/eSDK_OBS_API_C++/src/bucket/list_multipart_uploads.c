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

#define OBS_MAX_STR_TMP_SIZE 65536

static obs_status set_multipart_query_params(const char *prefix, const char *marker, const char *delimiter,
    const char* uploadid_marke, int max_uploads, char* query_params)
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
        safe_append_status("key-marker", marker, strlen(marker));
    }

    if (max_uploads)
    {
        max_uploads = max_uploads > 1000 ? 1000 : max_uploads;
        char max_upload_string[64] = { 0 };
        int ret = snprintf_s(max_upload_string, sizeof(max_upload_string), _TRUNCATE, "%d", max_uploads);
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        safe_append_status("max-uploads", max_upload_string, sizeof(max_upload_string));
    }

    if (prefix)
    {
        safe_append_status("prefix", prefix, strlen(prefix));
    }

    if (uploadid_marke)
    {
        safe_append_status("upload-id-marke", uploadid_marke, strlen(uploadid_marke));
    }

    errno_t err = EOK;
    err = memcpy_s(query_params, QUERY_STRING_LEN, queryParams, QUERY_STRING_LEN);
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    return OBS_STATUS_OK;
}


static void initialize_list_multipart_uploads(multipart_upload_info *uploads)
{
    string_buffer_initialize(uploads->key);
    string_buffer_initialize(uploads->upload_id);
    string_buffer_initialize(uploads->initiator_id);
    string_buffer_initialize(uploads->initiator_display_name);
    string_buffer_initialize(uploads->owner_id);
    string_buffer_initialize(uploads->owner_display_name);
    string_buffer_initialize(uploads->storage_class);
    string_buffer_initialize(uploads->initiated);
}

static void initialize_list_multipart_uploads_data(list_multipart_uploads_data *lmu_data)
{
    string_buffer_initialize(lmu_data->is_truncated);
    string_buffer_initialize(lmu_data->next_marker);
    string_buffer_initialize(lmu_data->next_uploadId_marker);
    lmu_data->uploads_count = 0;
    initialize_list_multipart_uploads(lmu_data->uploads);
    initialize_list_common_prefixes(lmu_data->common_prefixes);
}


static obs_status make_list_multipart_uploads_callback(list_multipart_uploads_data *lmu_data)
{
    int             i = 0;
    int             uploads_count = 0;
    obs_status      iRet = OBS_STATUS_OK;
    obs_list_multipart_upload *uploads = NULL;
    int is_truncated = (!strcmp(lmu_data->is_truncated, "true") ||
        !strcmp(lmu_data->is_truncated, "1")) ? 1 : 0;

    if (lmu_data->uploads_count > 0)
    {
        uploads = (obs_list_multipart_upload*)malloc(sizeof(obs_list_multipart_upload)
            * lmu_data->uploads_count);
        if (NULL == uploads)
        {
            COMMLOG(OBS_LOGERROR, "malloc obs_list_multipart_upload failed!");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(uploads, sizeof(obs_list_multipart_upload) * lmu_data->uploads_count, 0,
            sizeof(obs_list_multipart_upload) * lmu_data->uploads_count);

        uploads_count = lmu_data->uploads_count;
        for (i = 0; i < uploads_count; i++) {
            obs_list_multipart_upload   *upload_dest = &(uploads[i]);
            multipart_upload_info       *uploadSrc = &(lmu_data->uploads[i]);
            upload_dest->key = uploadSrc->key;
            upload_dest->upload_id = uploadSrc->upload_id;
            upload_dest->owner_id = uploadSrc->owner_id[0] ? uploadSrc->owner_id : 0;
            upload_dest->owner_display_name = (uploadSrc->owner_display_name[0] ?
                uploadSrc->owner_display_name : 0);
            upload_dest->initiator_id = uploadSrc->initiator_id[0] ? uploadSrc->initiator_id : 0;
            upload_dest->initiator_display_name = (uploadSrc->initiator_display_name[0] ?
                uploadSrc->initiator_display_name : 0);
            upload_dest->storage_class = uploadSrc->storage_class;
            upload_dest->initiated = parseIso8601Time(uploadSrc->initiated);
            int nTimeZone = getTimeZone();
            upload_dest->initiated += nTimeZone * SECONDS_TO_AN_HOUR;
        }
    }

    int common_prefixesCount = lmu_data->common_prefixes_count;
    char **common_prefixes = NULL;
    if (common_prefixesCount > 0)
    {
        common_prefixes = (char**)malloc(sizeof(char *) * common_prefixesCount);
        if (NULL == common_prefixes)
        {
            COMMLOG(OBS_LOGERROR, "Malloc common_prefixes failed!");
            CHECK_NULL_FREE(uploads);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(common_prefixes, sizeof(char *) * common_prefixesCount, 0,
            sizeof(char *) * common_prefixesCount);

        for (i = 0; i < common_prefixesCount; i++) {
            common_prefixes[i] = lmu_data->common_prefixes[i].prefix;
        }
    }

    iRet = (*(lmu_data->list_mulpu_callback))(is_truncated, lmu_data->next_marker,
        lmu_data->next_uploadId_marker, uploads_count, uploads, common_prefixesCount,
        (const char **)common_prefixes, lmu_data->callback_data);

    CHECK_NULL_FREE(uploads);
    CHECK_NULL_FREE(common_prefixes);
    return iRet;
}

obs_status parse_xml_list_multipart_uploads(list_multipart_uploads_data *lmu_data,
    const char *element_path, const char *data, int data_len)
{
    int fit = 1;
    int ret;
    multipart_upload_info  *uploads = &(lmu_data->uploads[lmu_data->uploads_count]);

    if (!strcmp(element_path, "ListMultipartUploadsResult/IsTruncated")) {
        string_buffer_append(lmu_data->is_truncated, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/NextKeyMarker")) {
        string_buffer_append(lmu_data->next_marker, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/NextUploadIdMarker")) {
        string_buffer_append(lmu_data->next_uploadId_marker, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Key")) {
#ifdef WIN32
        int strTmpSourceLen = data_len + 1;
        if (strTmpSourceLen <= 0 || strTmpSourceLen > OBS_MAX_STR_TMP_SIZE) {
            COMMLOG(OBS_LOGERROR, "require too much memory in function: %s,line %d", __FUNCTION__, __LINE__);
            return OBS_STATUS_OutOfMemory;
        }
        char* strTmpSource = (char*)malloc(sizeof(char) * strTmpSourceLen);
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(strTmpSource, sizeof(char) * strTmpSourceLen, 0, strTmpSourceLen);
        if (ret = strncpy_s(strTmpSource, strTmpSourceLen, data, data_len))
        {
            COMMLOG(OBS_LOGERROR, "in %s line %s strncpy_s error, code is %d.", __FUNCTION__, __LINE__, ret);
            return OBS_STATUS_InternalError;
        }
        char* strTmpOut = UTF8_To_String(strTmpSource);
        string_buffer_append(uploads->key, strTmpOut, strlen(strTmpOut), fit);
        CHECK_NULL_FREE(strTmpSource);
        CHECK_NULL_FREE(strTmpOut);
#else
        string_buffer_append(uploads->key, data, data_len, fit);
#endif
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/UploadId")) {
        string_buffer_append(uploads->upload_id, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Initiator/ID")) {
        string_buffer_append(uploads->initiator_id, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Initiator/DisplayName")) {
        string_buffer_append(uploads->initiator_display_name, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/StorageClass")) {
        string_buffer_append(uploads->storage_class, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Initiated")) {
        string_buffer_append(uploads->initiated, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Owner/ID")) {
        string_buffer_append(uploads->owner_id, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Owner/DisplayName")) {
        string_buffer_append(uploads->owner_display_name, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/CommonPrefixes/Prefix")) {
        string_buffer_append(lmu_data->common_prefixes[lmu_data->common_prefixes_count].prefix,
            data, data_len, fit);
    }

    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}

static obs_status list_multipart_uploads_xml_callback(const char *element_path,
    const char *data, int data_len, void *callback_data)
{
    list_multipart_uploads_data *lmu_data = (list_multipart_uploads_data *)callback_data;

    if (data) {
        return parse_xml_list_multipart_uploads(lmu_data, element_path, data, data_len);
    }

    if (!strcmp(element_path, "ListMultipartUploadsResult/Upload")) {
        // Finished a Contents
        lmu_data->uploads_count++;
        if (lmu_data->uploads_count == MAX_UPLOADS) {
            // Make the callback
            obs_status status = make_list_multipart_uploads_callback(lmu_data);
            if (OBS_STATUS_OK != status) {
                return status;
            }
            initialize_list_multipart_uploads_data(lmu_data);
        }
        else {
            // Initialize the next one
            initialize_list_multipart_uploads(&(lmu_data->uploads[lmu_data->uploads_count]));
        }
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/CommonPrefixes/Prefix")) {
        // Finished a Prefix
        lmu_data->common_prefixes_count++;
        if (lmu_data->common_prefixes_count == MAX_COMMON_PREFIXES) {
            // Make the callback
            obs_status status = make_list_multipart_uploads_callback(lmu_data);
            if (OBS_STATUS_OK != status) {
                return status;
            }
            initialize_list_multipart_uploads_data(lmu_data);
        }
        else {
            // Initialize the next one
            initialize_list_common_prefixes(
                &lmu_data->common_prefixes[lmu_data->common_prefixes_count]);
        }
    }

    return OBS_STATUS_OK;
}

static obs_status list_multipart_uploads_properties_callback
(const obs_response_properties *response_properties, void *callback_data)
{
    list_multipart_uploads_data *lmu_data = (list_multipart_uploads_data *)callback_data;
    if (lmu_data->response_properties_callback)
    {
        return (*(lmu_data->response_properties_callback))(response_properties,
            lmu_data->callback_data);
    }

    return OBS_STATUS_OK;
}

static obs_status list_multipart_uploads_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    list_multipart_uploads_data *lmu_data = (list_multipart_uploads_data *)callback_data;

    return simplexml_add(&(lmu_data->simpleXml), buffer, buffer_size);
}

static void list_multipart_uploads_complete_callback(obs_status requestStatus,
    const obs_error_details *obsErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_multipart_uploads_data *lmu_data = (list_multipart_uploads_data *)callback_data;

    if (lmu_data->uploads_count || lmu_data->common_prefixes_count) {

        obs_status callbackResult = make_list_multipart_uploads_callback(lmu_data);
        if (OBS_STATUS_OK != callbackResult)
        {
            COMMLOG(OBS_LOGERROR, "make_list_multipart_uploads_callback failed!");
        }
    }

    (*(lmu_data->response_complete_callback))(requestStatus, obsErrorDetails,
        lmu_data->callback_data);

    simplexml_deinitialize(&(lmu_data->simpleXml));

    free(lmu_data);
    lmu_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
    return;
}


void list_multipart_uploads(const obs_options *options, const char *prefix, const char *marker, const char *delimiter,
    const char* uploadid_marke, int max_uploads, obs_list_multipart_uploads_handler *handler,
    void *callback_data)
{
    request_params params;
    char queryParams[QUERY_STRING_LEN + 1] = { 0 };
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "list_multipart_uploads start !");

    obs_status ret_status = set_multipart_query_params(prefix, marker, delimiter, uploadid_marke,
        max_uploads, queryParams);
    if (OBS_STATUS_OK != ret_status)
    {
        (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "set_multipart_query_params return %d !", ret_status);
        return;
    }

    list_multipart_uploads_data *lmu_data =
        (list_multipart_uploads_data *)malloc(sizeof(list_multipart_uploads_data));
    if (!lmu_data) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc ListMultipartUploadsData failed !");
        return;
    }
    memset_s(lmu_data, sizeof(list_multipart_uploads_data), 0, sizeof(list_multipart_uploads_data));

    simplexml_initialize(&(lmu_data->simpleXml), &list_multipart_uploads_xml_callback, lmu_data);
    lmu_data->response_properties_callback = handler->response_handler.properties_callback;
    lmu_data->list_mulpu_callback = handler->list_mulpu_callback;
    lmu_data->response_complete_callback = handler->response_handler.complete_callback;
    lmu_data->callback_data = callback_data;
    initialize_list_multipart_uploads_data(lmu_data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &list_multipart_uploads_properties_callback;
    params.fromObsCallback = &list_multipart_uploads_data_callback;
    params.complete_callback = &list_multipart_uploads_complete_callback;
    params.callback_data = lmu_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.subResource = "uploads";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "list_multipart_uploads finish!");
}
