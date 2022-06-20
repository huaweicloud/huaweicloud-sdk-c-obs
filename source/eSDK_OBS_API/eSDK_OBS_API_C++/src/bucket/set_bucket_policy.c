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

void set_bucket_policy(const obs_options *options, const char *policy, obs_response_handler *handler,
    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set bucket policy start!");
    update_bucket_common_data *policy_data = (update_bucket_common_data*)malloc(sizeof(update_bucket_common_data));
    if (!policy_data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc set_policy_data failed !");
        return;
    }
    memset_s(policy_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));
    policy_data->properties_callback = handler->properties_callback;
    policy_data->complete_callback = handler->complete_callback;
    policy_data->callback_data = callback_data;
    policy_data->docBytesWritten = 0;
    policy_data->docLen =
        snprintf_s(policy_data->doc, sizeof(policy_data->doc), _TRUNCATE, "%s", policy);
    CheckAndLogNeg(policy_data->docLen, "snprintf_s", __FUNCTION__, __LINE__);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &update_bucket_common_properties_callback;
    params.toObsCallback = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = policy_data->docLen;
    params.complete_callback = &update_bucket_common_complete_callback;
    params.callback_data = policy_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "policy";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "set bucket policy finish!");
}