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

static obs_status get_bucket_storageInfo_xml_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    get_bucket_storageInfo_data *gbsiData = (get_bucket_storageInfo_data *)callback_data;

    int fit = 1;
    if (data)
    {
        if (!strcmp(element_path, "GetBucketStorageInfoResult/Size")) {
            string_buffer_append(gbsiData->size, data, data_len, fit);
        }
        else if (!strcmp(element_path, "GetBucketStorageInfoResult/ObjectNumber")) {
            string_buffer_append(gbsiData->objectnumber, data, data_len, fit);
        }
    }

    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}

static obs_status get_bucket_storageInfo_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_bucket_storageInfo_data *gbsiData = (get_bucket_storageInfo_data *)callback_data;
    return simplexml_add(&(gbsiData->simpleXml), buffer, buffer_size);
}

static void get_bucket_storageInfo_complete_callback(obs_status requestStatus,
    const obs_error_details *obsErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_storageInfo_data *gbsiData = (get_bucket_storageInfo_data *)callback_data;

    int ret = snprintf_s(gbsiData->capacity_return, sizeof(gbsiData->size), gbsiData->capacity_length, "%s",
        gbsiData->size);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
    ret = snprintf_s(gbsiData->object_number_return, sizeof(gbsiData->objectnumber),
        gbsiData->object_number_length, "%s", gbsiData->objectnumber);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);

    (void)(*(gbsiData->complete_callback))(requestStatus, obsErrorDetails, gbsiData->callback_data);

    simplexml_deinitialize(&(gbsiData->simpleXml));
    free(gbsiData);
    gbsiData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
    return;
}

static obs_status get_bucket_storageInfo_properties_callback(
    const obs_response_properties *response_properties, void *callback_data)
{
    get_bucket_storageInfo_data *gbsiData = (get_bucket_storageInfo_data *)callback_data;
    if (gbsiData->properties_callback)
    {
        return (*(gbsiData->properties_callback))(response_properties, gbsiData->callback_data);
    }
    return OBS_STATUS_OK;
}


void get_bucket_storage_info(const obs_options *options, int capacity_length, char *capacity,
    int object_number_length, char *object_number,
    obs_response_handler *handler, void *callback_data)
{
    request_params              params;
    get_bucket_storageInfo_data *gbsiData;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get_bucket_storage_info start.");
    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    if (NULL == capacity || NULL == object_number)
    {
        COMMLOG(OBS_LOGERROR, "capacity(%p) or object_number(%p) is invalid.", capacity, object_number);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }

    gbsiData = (get_bucket_storageInfo_data *)malloc(sizeof(get_bucket_storageInfo_data));
    if (!gbsiData) {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "Malloc GetBucketStorageInfoData failed.");
        return;
    }
    memset_s(gbsiData, sizeof(get_bucket_storageInfo_data), 0, sizeof(get_bucket_storageInfo_data));

    simplexml_initialize(&(gbsiData->simpleXml), &get_bucket_storageInfo_xml_callback, gbsiData);
    gbsiData->complete_callback = handler->complete_callback;
    gbsiData->capacity_return = capacity;
    gbsiData->capacity_length = capacity_length;
    gbsiData->object_number_return = object_number;
    gbsiData->object_number_length = object_number_length;
    gbsiData->callback_data = callback_data;
    gbsiData->properties_callback = handler->properties_callback;
    string_buffer_initialize(gbsiData->size);
    string_buffer_initialize(gbsiData->objectnumber);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_storageInfo_properties_callback;
    params.fromObsCallback = &get_bucket_storageInfo_data_callback;
    params.complete_callback = &get_bucket_storageInfo_complete_callback;
    params.callback_data = gbsiData;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "storageinfo";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_storage_info finish.");
}