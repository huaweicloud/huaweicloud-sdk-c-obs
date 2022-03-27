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

obs_status get_bucket_storage_policy_xml_callback(const char *element_path,
    const char *data, int data_len, void *callback_data)
{
    get_bucket_storage_class_policy_data *storage_class_data =
        (get_bucket_storage_class_policy_data *)callback_data;

    int fit = 1;

    if (data)
    {
        if (storage_class_data->use_api == OBS_USE_API_S3) {
            if (!strcmp(element_path, "StoragePolicy/DefaultStorageClass"))
            {
                string_buffer_append(storage_class_data->storage_class_policy, data, data_len, fit);
            }
        }
        else {
            if (!strcmp(element_path, "StorageClass"))
            {
                string_buffer_append(storage_class_data->storage_class_policy, data, data_len, fit);
            }
        }
    }

    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_storage_class_properties_callback(
    const obs_response_properties *response_properties, void *callback_data)
{
    get_bucket_storage_class_policy_data *storage_class_data =
        (get_bucket_storage_class_policy_data *)callback_data;
    if (storage_class_data->response_properties_callback)
    {
        return (*(storage_class_data->response_properties_callback))
            (response_properties, storage_class_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_storage_class_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_bucket_storage_class_policy_data *storage_class_data =
        (get_bucket_storage_class_policy_data *)callback_data;

    return simplexml_add(&(storage_class_data->simpleXml), buffer, buffer_size);
}

void get_bucket_storage_class_complete_callback(obs_status status,
    const obs_error_details *error_details, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_storage_class_policy_data *storage_class_data =
        (get_bucket_storage_class_policy_data *)callback_data;

    obs_status callback_result = (*(storage_class_data->response_bucket_sorage_policy_callback))
        (storage_class_data->storage_class_policy, storage_class_data->callback_data);
    if (callback_result != OBS_STATUS_OK)
    {
        COMMLOG(OBS_LOGERROR, "make_storage_policy_callback failed!");
    }

    (void)(*(storage_class_data->response_complete_callback))(status, error_details,
        storage_class_data->callback_data);
    simplexml_deinitialize(&(storage_class_data->simpleXml));

    if (storage_class_data)
    {
        free(storage_class_data);
        storage_class_data = NULL;
    }
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


void get_bucket_storage_class_policy(const obs_options *options,
    obs_get_bucket_storage_class_handler *handler,
    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get bucket storage class policy start!");
    get_bucket_storage_class_policy_data *storage_class_data =
        (get_bucket_storage_class_policy_data*)malloc(sizeof(get_bucket_storage_class_policy_data));
    if (!storage_class_data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc get_storage_class_data failed !");
        return;
    }
    memset_s(storage_class_data, sizeof(get_bucket_storage_class_policy_data),
        0, sizeof(get_bucket_storage_class_policy_data));
    storage_class_data->use_api = use_api;
    simplexml_initialize(&(storage_class_data->simpleXml), &get_bucket_storage_policy_xml_callback, storage_class_data);
    storage_class_data->response_properties_callback = handler->response_handler.properties_callback;
    storage_class_data->response_complete_callback = handler->response_handler.complete_callback;
    storage_class_data->response_bucket_sorage_policy_callback = handler->get_bucket_sorage_class_callback;
    storage_class_data->callback_data = callback_data;
    memset_s(storage_class_data->storage_class_policy, sizeof(storage_class_data->storage_class_policy), 0, sizeof(storage_class_data->storage_class_policy));

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_storage_class_properties_callback;
    params.fromObsCallback = &get_bucket_storage_class_data_callback;
    params.complete_callback = &get_bucket_storage_class_complete_callback;
    params.callback_data = storage_class_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    if (use_api == OBS_USE_API_S3) {
        params.subResource = "storagePolicy";
    }
    else {
        params.subResource = "storageClass";
    }
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket storage class policy finish!");
}