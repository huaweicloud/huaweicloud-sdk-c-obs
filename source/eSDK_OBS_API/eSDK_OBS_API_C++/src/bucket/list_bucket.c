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

void xml_callback_existdata(const char* element_path, const char* data, xml_callback_data* cbData, int data_len) {
    int fit = 1;
    if (!strcmp(element_path, "ListAllMyBucketsResult/Owner/ID")) {
        string_buffer_append(cbData->owner_id, data, data_len, fit);
    }
    else if (!strcmp(element_path,
        "ListAllMyBucketsResult/Owner/DisplayName")) {
        string_buffer_append(cbData->owner_display_name, data, data_len, fit);
    }
    else if (!strcmp(element_path,
        "ListAllMyBucketsResult/Buckets/Bucket/Name")) {
        string_buffer_append(cbData->bucket_name, data, data_len, fit);
    }
    else if (!strcmp
    (element_path,
        "ListAllMyBucketsResult/Buckets/Bucket/CreationDate")) {
        string_buffer_append(cbData->creationDate, data, data_len, fit);
    }
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
}

obs_status xml_callback_nodata(const char* element_path, xml_callback_data* cbData)
{
    if (!strcmp(element_path, "ListAllMyBucketsResult/Buckets/Bucket")) {
        time_t creationDate = parseIso8601Time(cbData->creationDate);
        int nTimeZone = getTimeZone();
        creationDate += nTimeZone * SECONDS_TO_AN_HOUR;

        obs_status status = (*(cbData->listServiceCallback))
            (cbData->owner_id, cbData->owner_display_name,
                cbData->bucket_name, creationDate, cbData->callback_data);

        string_buffer_initialize(cbData->bucket_name);
        string_buffer_initialize(cbData->creationDate);

        return status;
    }
    return OBS_STATUS_OK;
}

void xml_obs_callback_existdata(const char* element_path, const char* data, xml_obs_callback_data* cbData, int data_len)
{
    int fit = 1;
    if (!strcmp(element_path, "ListAllMyBucketsResult/Owner/ID")) {
        string_buffer_append(cbData->owner_id, data, data_len, fit);
    }
    else if (!strcmp(element_path,
        "ListAllMyBucketsResult/Buckets/Bucket/Name")) {
        string_buffer_append(cbData->bucket_name, data, data_len, fit);
    }
    else if (!strcmp(element_path,
        "ListAllMyBucketsResult/Buckets/Bucket/CreationDate")) {
        string_buffer_append(cbData->creationDate, data, data_len, fit);
    }
    else if (!strcmp(element_path,
        "ListAllMyBucketsResult/Buckets/Bucket/Location")) {
        string_buffer_append(cbData->location, data, data_len, fit);
    }
    else if (!strcmp(element_path,
        "ListAllMyBucketsResult/Buckets/Bucket/BucketType")) {
        string_buffer_append(cbData->bucketType, data, data_len, fit);
    }

    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
}

obs_status xml_obs_callback_nodata(const char* element_path, xml_obs_callback_data* cbData)
{
    if (!strcmp(element_path, "ListAllMyBucketsResult/Buckets/Bucket")) {
        time_t creationDate = parseIso8601Time(cbData->creationDate);
        int nTimeZone = getTimeZone();
        creationDate += nTimeZone * SECONDS_TO_AN_HOUR;

        obs_status status = (*(cbData->listServiceCallback))
            (cbData->owner_id, cbData->bucket_name,
                creationDate, cbData->location, cbData->callback_data);

        string_buffer_initialize(cbData->bucket_name);
        string_buffer_initialize(cbData->creationDate);
        string_buffer_initialize(cbData->location);
        string_buffer_initialize(cbData->bucketType);
        return status;
    }
    return OBS_STATUS_OK;
}

static obs_status xml_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    xml_callback_data *cbData = (xml_callback_data *)callback_data;


    if (data) {
        xml_callback_existdata(element_path, data, cbData, data_len);
    }
    else {
        return xml_callback_nodata(element_path, cbData);
    }
    return OBS_STATUS_OK;
}

static obs_status xml_obs_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    xml_obs_callback_data *cbData = (xml_obs_callback_data *)callback_data;

    if (data) {
        xml_obs_callback_existdata(element_path, data, cbData, data_len);
    }
    else {
        return xml_obs_callback_nodata(element_path, cbData);
    }

    return OBS_STATUS_OK;
}

static obs_status properties_callback
(const obs_response_properties *responseProperties, void *callback_data)
{
    xml_callback_data *cbData = (xml_callback_data *)callback_data;
    if (cbData->responsePropertiesCallback)
    {
        return (*(cbData->responsePropertiesCallback))
            (responseProperties, cbData->callback_data);
    }
    return OBS_STATUS_OK;
}

static void complete_callback(obs_status requestStatus, const obs_error_details *s3ErrorDetails, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    xml_callback_data *cbData = (xml_callback_data *)callback_data;

    (void)(*(cbData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, cbData->callback_data);

    simplexml_deinitialize(&(cbData->simpleXml));

    free(cbData);
    cbData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

static obs_status data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    xml_callback_data *cbData = (xml_callback_data *)callback_data;

    return simplexml_add(&(cbData->simpleXml), buffer, buffer_size);
}


void list_bucket(const obs_options *options, obs_list_service_handler *handler, void *callback_data)
{
    obs_use_api use_api = OBS_USE_API_S3;

    if (options->request_options.auth_switch == OBS_OBS_TYPE)
    {
        use_api = OBS_USE_API_OBS;
    }
    else if (options->request_options.auth_switch == OBS_S3_TYPE)
    {
        use_api = OBS_USE_API_S3;
    }

    request_params      params;

    COMMLOG(OBS_LOGINFO, "Enter list_bucket successfully !");

    xml_callback_data *data = (xml_callback_data *)malloc(sizeof(xml_callback_data));
    if (!data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory,
            0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc XmlCallbackData failed !");
        return;
    }
    memset_s(data, sizeof(xml_callback_data), 0, sizeof(xml_callback_data));

    simplexml_initialize(&(data->simpleXml), &xml_callback, data);

    data->responsePropertiesCallback = handler->response_handler.properties_callback;
    data->listServiceCallback = handler->listServiceCallback;
    data->responseCompleteCallback = handler->response_handler.complete_callback;
    data->callback_data = callback_data;

    string_buffer_initialize(data->owner_id);
    string_buffer_initialize(data->owner_display_name);
    string_buffer_initialize(data->bucket_name);
    string_buffer_initialize(data->creationDate);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));

    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &properties_callback;
    params.fromObsCallback = &data_callback;
    params.complete_callback = &complete_callback;
    params.callback_data = data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave list_bucket successfully !");
}

void list_bucket_obs(const obs_options *options, obs_list_service_obs_handler *handler, void *callback_data)
{
    request_params      params;
    obs_use_api use_api = OBS_USE_API_S3;

    if (options->request_options.auth_switch == OBS_NEGOTIATION_TYPE)
    {
        if (get_api_version(NULL, options->bucket_options.host_name, options->bucket_options.protocol, &options->request_options) ==
            OBS_STATUS_OK)
        {
            use_api = OBS_USE_API_OBS;
        }
        else
        {
            use_api = OBS_USE_API_S3;
        }
    }
    else if (options->request_options.auth_switch == OBS_OBS_TYPE)
    {
        use_api = OBS_USE_API_OBS;
    }
    else if (options->request_options.auth_switch == OBS_S3_TYPE)
    {
        use_api = OBS_USE_API_S3;
    }

    COMMLOG(OBS_LOGINFO, "Enter list_bucket obs successfully !");
    xml_obs_callback_data *data = (xml_obs_callback_data *)malloc(sizeof(xml_obs_callback_data));
    if (!data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory,
            0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc XmlCallbackData failed !");
        return;
    }
    memset_s(data, sizeof(xml_obs_callback_data), 0, sizeof(xml_obs_callback_data));

    simplexml_initialize(&(data->simpleXml), &xml_obs_callback, data);

    data->responsePropertiesCallback = handler->response_handler.properties_callback;
    data->listServiceCallback = handler->listServiceCallback;
    data->responseCompleteCallback = handler->response_handler.complete_callback;
    data->callback_data = callback_data;

    string_buffer_initialize(data->owner_id);
    string_buffer_initialize(data->bucket_name);
    string_buffer_initialize(data->creationDate);
    string_buffer_initialize(data->location);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));

    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &properties_callback;
    params.fromObsCallback = &data_callback;
    params.complete_callback = &complete_callback;
    params.callback_data = data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave list_bucket_obs successfully !");
}
