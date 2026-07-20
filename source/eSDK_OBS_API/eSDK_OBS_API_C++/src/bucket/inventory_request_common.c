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
#include "inventory_request_common.h"
#include "request_util.h"

obs_status inventory_request_properties_callback(
    const obs_response_properties *response_properties, void *callback_data)
{
    inventory_request_base *base = (inventory_request_base *)callback_data;
    if (base->response_properties_callback)
    {
        return (*(base->response_properties_callback))(response_properties,
            base->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status inventory_request_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    inventory_request_base *base = (inventory_request_base *)callback_data;
    return simplexml_add(&(base->simpleXml), buffer, buffer_size);
}

void setup_inventory_request_params(const obs_options *options,
    obs_use_api use_api, request_params *params)
{
    errno_t err = EOK;
    err = memcpy_s(&params->bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params->request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params->httpRequestType = http_request_type_get;
    params->properties_callback = &inventory_request_properties_callback;
    params->fromObsCallback = &inventory_request_data_callback;
    params->isCheckCA = is_check_ca(options);
    params->storageClassFormat = no_need_storage_class;
    params->subResource = "inventory";
    params->temp_auth = options->temp_auth;
    params->use_api = use_api;
}
