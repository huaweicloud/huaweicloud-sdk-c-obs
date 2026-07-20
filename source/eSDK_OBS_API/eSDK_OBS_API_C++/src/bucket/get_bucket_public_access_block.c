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
**********************************************************************************/
#include "bucket.h"
#include "request_util.h"

typedef struct get_bucket_public_access_block_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_response_complete_callback *responseCompleteCallback;
    obs_get_bucket_public_access_block_callback *get_bucket_public_access_block_callback;
    void *callback_data;

    obs_bucket_public_access_block bpa_config;
    obs_use_api use_api;
} get_bucket_public_access_block_data;

static obs_status get_bucket_public_access_block_xml_callback(
    const char *element_path,
    const char *data,
    int data_len,
    void *callback_data)
{
    get_bucket_public_access_block_data *bpa_data =
        (get_bucket_public_access_block_data *)callback_data;

    if (!data || !element_path) {
        return OBS_STATUS_OK;
    }

    // Parse XML elements
    if (!strcmp(element_path, "PublicAccessBlockConfiguration/BlockPublicAcls")) {
        bpa_data->bpa_config.block_public_acls = (strncmp(data, "true", 4) == 0);
    }
    else if (!strcmp(element_path, "PublicAccessBlockConfiguration/IgnorePublicAcls")) {
        bpa_data->bpa_config.ignore_public_acls = (strncmp(data, "true", 4) == 0);
    }
    else if (!strcmp(element_path, "PublicAccessBlockConfiguration/BlockPublicPolicy")) {
        bpa_data->bpa_config.block_public_policy = (strncmp(data, "true", 4) == 0);
    }
    else if (!strcmp(element_path, "PublicAccessBlockConfiguration/RestrictPublicBuckets")) {
        bpa_data->bpa_config.restrict_public_buckets = (strncmp(data, "true", 4) == 0);
    }

    return OBS_STATUS_OK;
}

static obs_status get_bucket_public_access_block_properties_callback(
    const obs_response_properties *response_properties,
    void *callback_data)
{
    get_bucket_public_access_block_data *bpa_data =
        (get_bucket_public_access_block_data *)callback_data;

    if (bpa_data->responsePropertiesCallback)
    {
        return (*(bpa_data->responsePropertiesCallback))
            (response_properties, bpa_data->callback_data);
    }
    return OBS_STATUS_OK;
}

static obs_status get_bucket_public_access_block_data_callback(
    int buffer_size,
    const char *buffer,
    void *callback_data)
{
    get_bucket_public_access_block_data *bpa_data =
        (get_bucket_public_access_block_data *)callback_data;
    return simplexml_add(&(bpa_data->simpleXml), buffer, buffer_size);
}

static void get_bucket_public_access_block_complete_callback(
    obs_status status,
    const obs_error_details *error_details,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_public_access_block_data *bpa_data =
        (get_bucket_public_access_block_data *)callback_data;

    if (OBS_STATUS_OK == status)
    {
        // Call user callback with BPA configuration
        if (bpa_data->get_bucket_public_access_block_callback)
        {
            (void)(*(bpa_data->get_bucket_public_access_block_callback))(
                &(bpa_data->bpa_config),
                bpa_data->callback_data);
        }
    }

    if (bpa_data->responseCompleteCallback)
    {
        (void)(*(bpa_data->responseCompleteCallback))(status, error_details,
            bpa_data->callback_data);
    }

    simplexml_deinitialize(&(bpa_data->simpleXml));

    free(bpa_data);
    bpa_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void get_bucket_public_access_block(
    const obs_options *options,
    obs_get_bucket_public_access_block_handler *handler,
    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    // Validate parameters
    if (!options || !handler) {
        COMMLOG(OBS_LOGERROR, "Invalid parameters: options or handler is NULL");
        return;
    }
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get bucket public access block start!");

    get_bucket_public_access_block_data *bpa_data =
        (get_bucket_public_access_block_data *)malloc(sizeof(get_bucket_public_access_block_data));
    if (!bpa_data)
    {
        if (handler && handler->response_handler.complete_callback) {
            (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        }
        COMMLOG(OBS_LOGERROR, "malloc get_bucket_public_access_block_data failed !");
        return;
    }
    memset_s(bpa_data, sizeof(get_bucket_public_access_block_data), 0,
             sizeof(get_bucket_public_access_block_data));

    // Initialize BPA config to default values
    bpa_data->bpa_config.block_public_acls = false;
    bpa_data->bpa_config.ignore_public_acls = false;
    bpa_data->bpa_config.block_public_policy = false;
    bpa_data->bpa_config.restrict_public_buckets = false;

    simplexml_initialize(&(bpa_data->simpleXml),
                        &get_bucket_public_access_block_xml_callback,
                        bpa_data);

    bpa_data->responsePropertiesCallback = handler->response_handler.properties_callback;
    bpa_data->responseCompleteCallback = handler->response_handler.complete_callback;
    bpa_data->get_bucket_public_access_block_callback = handler->get_bucket_public_access_block_callback;
    bpa_data->callback_data = callback_data;
    bpa_data->use_api = use_api;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_public_access_block_properties_callback;
    params.fromObsCallback = &get_bucket_public_access_block_data_callback;
    params.complete_callback = &get_bucket_public_access_block_complete_callback;
    params.callback_data = bpa_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "publicAccessBlock";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get bucket public access block finish!");
}
