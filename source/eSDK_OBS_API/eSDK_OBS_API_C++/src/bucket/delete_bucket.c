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

void delete_bucket(const obs_options *options, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    COMMLOG(OBS_LOGINFO, "delete_bucket start!");
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    params.httpRequestType = http_request_type_delete;
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.temp_auth = options->temp_auth;
    params.callback_data = callback_data;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "delete_bucket finish!");
}