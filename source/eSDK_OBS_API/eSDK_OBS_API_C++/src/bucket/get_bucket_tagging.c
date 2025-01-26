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

void get_bucket_tagging_xml_callback_existdata(get_bucket_tagging_data* tagging_data, const char* element_path, const char* data, int data_len)
{
    int fit = 1;
    if (tagging_data->tagging_count < MAX_NUM_TAGGING)
    {
        if (!strcmp(element_path, "Tagging/TagSet/Tag/Key"))
        {
            string_buffer_append(tagging_data->tagging_list[tagging_data->tagging_count].key,
                data, data_len, fit);
        }
        else if (!strcmp(element_path, "Tagging/TagSet/Tag/Value"))
        {
            string_buffer_append(tagging_data->tagging_list[tagging_data->tagging_count].value,
                data, data_len, fit);
        }
    }

    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
}

obs_status get_bucket_tagging_xml_callback_nodata(get_bucket_tagging_data* tagging_data, const char* element_path)
{
    if (!strcmp(element_path, "Tagging/TagSet/Tag"))
    {
        tagging_data->tagging_count++;
        if (tagging_data->tagging_count > MAX_NUM_TAGGING)
        {
            COMMLOG(OBS_LOGERROR, "etag number exceed the max[10]!");
            return OBS_STATUS_InternalError;
        }
        else if (tagging_data->tagging_count == MAX_NUM_TAGGING)
        {
            COMMLOG(OBS_LOGINFO, "already get the max[10] tags!");
            return OBS_STATUS_OK;
        }
        else
        {
            memset_s(&tagging_data->tagging_list[tagging_data->tagging_count], sizeof(tagging_kv), 0, sizeof(tagging_kv));
            return OBS_STATUS_OK;
        }
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_tagging_xml_callback(const char *element_path,
    const char *data, int data_len, void *callback_data)
{
    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *)callback_data;

    if (data)
    {
        get_bucket_tagging_xml_callback_existdata(tagging_data, element_path, data, data_len);
    }
    else
    {
        return get_bucket_tagging_xml_callback_nodata(tagging_data, element_path);
    }

    return OBS_STATUS_OK;
}

obs_status get_bucket_tagging_properties_callback(
    const obs_response_properties *response_properties,
    void *callback_data)
{
    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *)callback_data;
    if (tagging_data->response_properties_callback)
    {
        return (*(tagging_data->response_properties_callback))
            (response_properties, tagging_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status make_list_tagging_callback(get_bucket_tagging_data *tagging_data)
{
    obs_status status = OBS_STATUS_OK;
    obs_name_value *tagging_list = NULL;
    int tagging_count = 0;
    int i = 0;
    if (tagging_data->tagging_count > 0)
    {
        tagging_list = (obs_name_value*)malloc(sizeof(obs_name_value) * tagging_data->tagging_count);
        if (NULL == tagging_list)
        {
            COMMLOG(OBS_LOGERROR, "malloc tagging_list failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(tagging_list, sizeof(obs_name_value) * tagging_data->tagging_count,
            0, sizeof(obs_name_value) * tagging_data->tagging_count);

        tagging_count = tagging_data->tagging_count;
        for (i = 0; i < tagging_count; i++)
        {
            obs_name_value *content_dest = &(tagging_list[i]);
            tagging_kv *content_src = &(tagging_data->tagging_list[i]);
            content_dest->name = content_src->key;
            content_dest->value = content_src->value;
        }
    }

    if (tagging_data->response_tagging_list_callback)
    {
        status = (*(tagging_data->response_tagging_list_callback))
            (tagging_count, tagging_list, tagging_data->callback_data);
    }
    CHECK_NULL_FREE(tagging_list);

    return status;
}

void get_bucket_tagging_complete_callback(obs_status status,
    const obs_error_details *error_details,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *)callback_data;

    if (tagging_data->tagging_count)
    {
        obs_status result = make_list_tagging_callback(tagging_data);
        if (result != OBS_STATUS_OK)
        {
            COMMLOG(OBS_LOGERROR, "make_list_tagging_callback failed!");
        }
    }
    else if (tagging_data->response_tagging_list_callback)
    {
        (*(tagging_data->response_tagging_list_callback))(0, NULL, tagging_data->callback_data);
    }

    (void)(*(tagging_data->response_complete_callback))(status, error_details,
        tagging_data->callback_data);
    simplexml_deinitialize(&(tagging_data->simpleXml));

    free(tagging_data);
    tagging_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

obs_status get_bucket_tagging_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *)callback_data;
    return simplexml_add(&(tagging_data->simpleXml), buffer, buffer_size);
}


void get_bucket_tagging(const obs_options *options,
    obs_get_bucket_tagging_handler *handler,
    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get bucket tagging start!");
    get_bucket_tagging_data *tagging_data =
        (get_bucket_tagging_data*)malloc(sizeof(get_bucket_tagging_data));
    if (!tagging_data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc get_storage_class_data failed !");
        return;
    }
    memset_s(tagging_data, sizeof(get_bucket_tagging_data),
        0, sizeof(get_bucket_tagging_data));

    simplexml_initialize(&(tagging_data->simpleXml), &get_bucket_tagging_xml_callback, tagging_data);
    tagging_data->response_properties_callback = handler->response_handler.properties_callback;
    tagging_data->response_complete_callback = handler->response_handler.complete_callback;
    tagging_data->response_tagging_list_callback = handler->get_bucket_tagging_callback;
    tagging_data->callback_data = callback_data;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_tagging_properties_callback;
    params.fromObsCallback = &get_bucket_tagging_data_callback;
    params.complete_callback = &get_bucket_tagging_complete_callback;
    params.callback_data = tagging_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "tagging";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket tagging finish!");
}