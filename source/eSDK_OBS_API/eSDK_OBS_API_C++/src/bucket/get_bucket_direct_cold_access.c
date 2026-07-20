/*********************************************************************************
* Copyright 2019 Huawei Technologies Co.,Ltd.
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use
* this file except in compliance with the License.  You may obtain a copy of the
* License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software distributed
* under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations under the License.
**********************************************************************************
*/
#include "bucket.h"
#include "request_util.h"
#include <openssl/md5.h>

typedef struct get_bucket_direct_cold_access_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    obs_get_bucket_direct_cold_access_callback *response_direct_cold_access_callback;
    void *callback_data;

    int enabled_return_size;
    char *enabled_return;

    string_buffer(enabled, 64);
} get_bucket_direct_cold_access_data;

void get_bucket_direct_cold_access_xml_callback_existdata(get_bucket_direct_cold_access_data* direct_cold_access_data,
    const char* element_path, const char* data, int data_len)
{
    int fit = 1;
    if (!strcmp(element_path, "DirectColdAccessConfiguration/Status"))
    {
        string_buffer_append(direct_cold_access_data->enabled, data, data_len, fit);
    }

    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
}

obs_status get_bucket_direct_cold_access_xml_callback_nodata(get_bucket_direct_cold_access_data* direct_cold_access_data,
    const char* element_path)
{
    (void)direct_cold_access_data;
    (void)element_path;
    return OBS_STATUS_OK;
}

obs_status get_bucket_direct_cold_access_xml_callback(const char *element_path,
    const char *data, int data_len, void *callback_data)
{
    get_bucket_direct_cold_access_data *direct_cold_access_data = (get_bucket_direct_cold_access_data *)callback_data;

    if (data)
    {
        get_bucket_direct_cold_access_xml_callback_existdata(direct_cold_access_data, element_path, data, data_len);
    }
    else
    {
        return get_bucket_direct_cold_access_xml_callback_nodata(direct_cold_access_data, element_path);
    }

    return OBS_STATUS_OK;
}

obs_status get_bucket_direct_cold_access_properties_callback(
    const obs_response_properties *response_properties,
    void *callback_data)
{
    get_bucket_direct_cold_access_data *direct_cold_access_data = (get_bucket_direct_cold_access_data *)callback_data;
    if (direct_cold_access_data->response_properties_callback)
    {
        return (*(direct_cold_access_data->response_properties_callback))
            (response_properties, direct_cold_access_data->callback_data);
    }
    return OBS_STATUS_OK;
}

void get_bucket_direct_cold_access_complete_callback(obs_status status,
    const obs_error_details *error_details,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_direct_cold_access_data *direct_cold_access_data = (get_bucket_direct_cold_access_data *)callback_data;

    if (direct_cold_access_data->response_direct_cold_access_callback)
    {
        (*(direct_cold_access_data->response_direct_cold_access_callback))
            (direct_cold_access_data->enabled, direct_cold_access_data->callback_data);
    }

    (void)(*(direct_cold_access_data->response_complete_callback))(status, error_details,
        direct_cold_access_data->callback_data);
    simplexml_deinitialize(&(direct_cold_access_data->simpleXml));

    free(direct_cold_access_data);
    direct_cold_access_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

obs_status get_bucket_direct_cold_access_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_bucket_direct_cold_access_data *direct_cold_access_data = (get_bucket_direct_cold_access_data *)callback_data;
    return simplexml_add(&(direct_cold_access_data->simpleXml), buffer, buffer_size);
}

void get_bucket_direct_cold_access(const obs_options *options,
    obs_get_bucket_direct_cold_access_handler *handler,
    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get bucket direct cold access start!");

    if (!handler)
    {
        COMMLOG(OBS_LOGERROR, "handler is NULL!");
        return;
    }

    get_bucket_direct_cold_access_data *direct_cold_access_data =
        (get_bucket_direct_cold_access_data*)malloc(sizeof(get_bucket_direct_cold_access_data));
    if (!direct_cold_access_data)
    {
        if (handler && handler->response_handler.complete_callback)
        {
            (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        }
        COMMLOG(OBS_LOGERROR, "malloc get_bucket_direct_cold_access_data failed !");
        return;
    }
    memset_s(direct_cold_access_data, sizeof(get_bucket_direct_cold_access_data),
        0, sizeof(get_bucket_direct_cold_access_data));

    simplexml_initialize(&(direct_cold_access_data->simpleXml), &get_bucket_direct_cold_access_xml_callback, direct_cold_access_data);
    direct_cold_access_data->response_properties_callback = handler->response_handler.properties_callback;
    direct_cold_access_data->response_complete_callback = handler->response_handler.complete_callback;
    direct_cold_access_data->response_direct_cold_access_callback = handler->get_bucket_direct_cold_access_callback;
    direct_cold_access_data->callback_data = callback_data;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_direct_cold_access_properties_callback;
    params.fromObsCallback = &get_bucket_direct_cold_access_data_callback;
    params.complete_callback = &get_bucket_direct_cold_access_complete_callback;
    params.callback_data = direct_cold_access_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "directcoldaccess";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket direct cold access finish!");
}