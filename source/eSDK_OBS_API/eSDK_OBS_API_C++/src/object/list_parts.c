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
#include "object.h"
#include "request_util.h"
#include <openssl/md5.h> 

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


static obs_status make_list_parts_callback(list_parts_data *lpData)
{
    int i;
    obs_status iRet = OBS_STATUS_OK;
    int is_truncated = (!strcmp(lpData->is_truncated, "true") ||
        !strcmp(lpData->is_truncated, "1")) ? 1 : 0;
    char *initiator_id = lpData->initiator_id;
    char *initiator_display_name = lpData->initiator_display_name;
    char *owner_id = lpData->owner_id;
    char *owner_display_name = lpData->owner_display_name;

    if (lpData->parts_count < 1)
    {
        COMMLOG(OBS_LOGERROR, "Invalid Malloc Parameter!");
        return OBS_STATUS_InternalError;
    }
    obs_list_parts *parts = (obs_list_parts*)malloc(sizeof(obs_list_parts) * lpData->parts_count);
    if (NULL == parts)
    {
        COMMLOG(OBS_LOGERROR, "Malloc S3ListParts failed!");
        return OBS_STATUS_InternalError;
    }
    memset_s(parts, sizeof(obs_list_parts) * lpData->parts_count, 0, sizeof(obs_list_parts) * lpData->parts_count);

    int parts_count = lpData->parts_count;
    for (i = 0; i < parts_count; i++) {
        obs_list_parts *partsDest = &(parts[i]);
        parts_info *partsSrc = &(lpData->parts[i]);
        partsDest->part_number = partsSrc->part_number;

        partsDest->last_modified =
            parseIso8601Time(partsSrc->last_modified);
        int nTimeZone = getTimeZone();
        partsDest->last_modified += nTimeZone * SECONDS_TO_AN_HOUR;
        partsDest->etag = partsSrc->etag;
        partsDest->size = parseUnsignedInt(partsSrc->size);

    }

    if (lpData->list_parts_callback_ex)
    {
        obs_uploaded_parts_total_info uploadedPartsInfo;
        uploadedPartsInfo.is_truncated = is_truncated;
        uploadedPartsInfo.initiator_id = initiator_id;
        uploadedPartsInfo.nextpart_number_marker = lpData->nextpart_number_marker;
        uploadedPartsInfo.initiator_display_name = initiator_display_name;
        uploadedPartsInfo.owner_id = owner_id;
        uploadedPartsInfo.owner_display_name = owner_display_name;
        uploadedPartsInfo.sorage_class = lpData->storage_class;
        uploadedPartsInfo.parts_count = parts_count;

        iRet = (*(lpData->list_parts_callback_ex))(&uploadedPartsInfo, parts, lpData->callback_data);
    }

    CHECK_NULL_FREE(parts);
    return iRet;
}

static void initialize_parts(parts_info *parts)
{
    parts->part_number = 0;
    string_buffer_initialize(parts->last_modified);
    string_buffer_initialize(parts->etag);
    string_buffer_initialize(parts->size);
}


static void initialize_list_parts_data(list_parts_data *lpData)
{
    lpData->parts_count = 0;
    initialize_parts(lpData->parts);
}


int parse_xmlnode_list_parts_withPart(list_parts_data *lpData, const char *elementPath,
    const char *data, int dataLen, int fit)
{
    if (!strcmp(elementPath, "ListPartsResult/Part/PartaNumber"))
    {
        parts_info *parts = &(lpData->parts[lpData->parts_count]);
        parts->part_number = atoi(data);
    }
    else if (!strcmp(elementPath, "ListPartsResult/Part/LastModified"))
    {
        parts_info *parts = &(lpData->parts[lpData->parts_count]);
        string_buffer_append(parts->last_modified, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "ListPartsResult/Part/ETag"))
    {
        parts_info *parts = &(lpData->parts[lpData->parts_count]);
        string_buffer_append(parts->etag, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "ListPartsResult/Part/Size"))
    {
        parts_info *parts = &(lpData->parts[lpData->parts_count]);
        string_buffer_append(parts->size, data, dataLen, fit);
    }
    return fit;
}

int parse_xmlnode_list_parts_notPart(list_parts_data *lpData, const char *elementPath,
    const char *data, int dataLen, int fit)
{
    if (!strcmp(elementPath, "ListPartsResult/Initiator/ID"))
    {
        string_buffer_append(lpData->initiator_id, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "ListPartsResult/Initiator/DisplayName"))
    {
        string_buffer_append(lpData->initiator_display_name, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "ListPartsResult/Owner/ID"))
    {
        string_buffer_append(lpData->owner_id, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "ListPartsResult/Owner/DisplayName"))
    {
        string_buffer_append(lpData->owner_display_name, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "ListPartsResult/StorageClass"))
    {
        string_buffer_append(lpData->storage_class, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "ListPartResult/NextPartNumberMarker"))
    {
        lpData->nextpart_number_marker = atoi(data);
    }
    else if (!strcmp(elementPath, "ListPartResult/IsTruncated"))
    {
        string_buffer_append(lpData->is_truncated, data, dataLen, fit);
    }
    return fit;
}


void parse_xmlnode_list_parts(list_parts_data *lpData, const char *elementPath,
    const char *data, int dataLen)
{
    int fit = 1;
    if (strstr(elementPath, "ListPartsResult/Part"))
    {
        fit = parse_xmlnode_list_parts_withPart(lpData, elementPath, data, dataLen, fit);
    }
    else
    {
        fit = parse_xmlnode_list_parts_notPart(lpData, elementPath, data, dataLen, fit);
    }

    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return;
}


static obs_status ListPartsXmlCallback(const char *elementPath,
    const char *data, int dataLen,
    void *callback_data)
{
    list_parts_data *lpData = (list_parts_data *)callback_data;
    if (data)
    {
        parse_xmlnode_list_parts(lpData, elementPath, data, dataLen);
    }
    else
    {
        if (strcmp(elementPath, "ListPartsResult/Part"))
        {
            return OBS_STATUS_OK;
        }

        lpData->parts_count++;
        if (lpData->parts_count == MAX_PARTS) {
            obs_status status = make_list_parts_callback(lpData);
            if (status != OBS_STATUS_OK) {
                return status;
            }
            initialize_list_parts_data(lpData);
        }
        else {
            initialize_parts(&(lpData->parts[lpData->parts_count]));
        }
    }

    return OBS_STATUS_OK;
}


static obs_status ListPartsPropertiesCallback(
    const obs_response_properties *responseProperties, void *callback_data)
{
    list_parts_data *lpData = (list_parts_data *)callback_data;
    if (lpData->responsePropertiesCallback)
    {
        return (*(lpData->responsePropertiesCallback))(responseProperties,
            lpData->callback_data);
    }

    return OBS_STATUS_OK;
}


static void ListPartsCompleteCallback(obs_status requestStatus,
    const obs_error_details *s3ErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_parts_data *lpData = (list_parts_data *)callback_data;
    if (lpData->parts_count) {
        obs_status ret = make_list_parts_callback(lpData);
        if (ret != OBS_STATUS_OK) {
            COMMLOG(OBS_LOGDEBUG, "Failed to call make_list_parts_callback, status: %d.", ret);
        }
    }

    (*(lpData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, lpData->callback_data);

    simplexml_deinitialize(&(lpData->simpleXml));
    free(lpData);
    lpData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

static obs_status ListPartsDataCallback(int buffer_size, const char *buffer,
    void *callback_data)
{
    list_parts_data *lpData = (list_parts_data *)callback_data;
    return simplexml_add(&(lpData->simpleXml), buffer, buffer_size);
}


void list_parts(const obs_options *options, char *key, list_part_info *listpart,
    obs_list_parts_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    COMMLOG(OBS_LOGINFO, "Enter list_parts successfully !");
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    int amp = 0;
    if (listpart->upload_id) {
        safe_append_with_interface_log("uploadId", listpart->upload_id,
            handler->response_handler.complete_callback);
    }
    else
    {
        COMMLOG(OBS_LOGERROR, "upload_id is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }

    char max_parts_string[64] = { 0 };
    int ret = snprintf_s(max_parts_string, sizeof(max_parts_string), _TRUNCATE, "%u",
        listpart->max_parts);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
    safe_append_with_interface_log("max-parts", max_parts_string,
        handler->response_handler.complete_callback);

    char part_number_string[64] = { 0 };
    ret = snprintf_s(part_number_string, sizeof(part_number_string), _TRUNCATE, "%u",
        listpart->part_number_marker);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
    safe_append_with_interface_log("part-number-marker", part_number_string,
        handler->response_handler.complete_callback);

    list_parts_data *lpData = (list_parts_data *)malloc(sizeof(list_parts_data));
    if (!lpData) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGINFO, "Malloc ListPartsData failed !");
        return;
    }
    memset_s(lpData, sizeof(list_parts_data), 0, sizeof(list_parts_data));
    simplexml_initialize(&(lpData->simpleXml), &ListPartsXmlCallback, lpData);

    lpData->responsePropertiesCallback =
        handler->response_handler.properties_callback;
    lpData->list_parts_callback_ex = handler->list_parts_callback_ex;
    lpData->responseCompleteCallback =
        handler->response_handler.complete_callback;
    lpData->callback_data = callback_data;

    string_buffer_initialize(lpData->initiator_id);
    string_buffer_initialize(lpData->initiator_display_name);
    string_buffer_initialize(lpData->owner_id);
    string_buffer_initialize(lpData->owner_display_name);
    string_buffer_initialize(lpData->storage_class);
    string_buffer_initialize(lpData->is_truncated);
    initialize_list_parts_data(lpData);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_get;
    params.key = key;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.fromObsCallback = &ListPartsDataCallback;
    params.properties_callback = &ListPartsPropertiesCallback;
    params.complete_callback = &ListPartsCompleteCallback;
    params.callback_data = lpData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave list_parts successfully !");
}