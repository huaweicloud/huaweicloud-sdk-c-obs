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

obs_status get_bucket_policy_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    get_bucket_policy_data *policy_data = (get_bucket_policy_data *)callback_data;

    if (policy_data->responsePropertiesCallback)
    {
        return (*(policy_data->responsePropertiesCallback))
            (response_properties, policy_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_policy_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    get_bucket_policy_data *policy_data = (get_bucket_policy_data *)callback_data;
	int ret = memcpy_s(policy_data->policy + policy_data->currentPolicyLength, 
		sizeof(policy_data->policy) - (policy_data->currentPolicyLength * sizeof(char)), 
			buffer, buffer_size);
	if (ret < 0) {
		COMMLOG(OBS_LOGWARN, "%s failed in function: %s, line (%ld)!", "memcpy_s", __FUNCTION__, __LINE__);
		return OBS_STATUS_Security_Function_Failed;
	}
	policy_data->currentPolicyLength += buffer_size;

    return OBS_STATUS_OK;
}

void get_bucket_policy_complete_callback(obs_status status,
    const obs_error_details *error_details,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_policy_data *policy_data = (get_bucket_policy_data *)callback_data;
    int ret = snprintf_s(policy_data->policyReturn, sizeof(policy_data->policy),
        policy_data->policyReturnSize, "%s",
        policy_data->policy);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);

    (void)(*(policy_data->responseCompleteCallback))(status, error_details, policy_data->callback_data);

    free(policy_data);
    policy_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void get_bucket_policy(const obs_options *options, int policy_return_size,
    char *policy_return, obs_response_handler *handler,
    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get bucket policy start!");
    get_bucket_policy_data *policy_data = (get_bucket_policy_data*)malloc(sizeof(get_bucket_policy_data));
    if (!policy_data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc get_policy_data failed !");
        return;
    }
    memset_s(policy_data, sizeof(get_bucket_policy_data), 0, sizeof(get_bucket_policy_data));
    policy_data->responsePropertiesCallback = handler->properties_callback;
    policy_data->responseCompleteCallback = handler->complete_callback;
    policy_data->callback_data = callback_data;
    policy_data->policyReturn = policy_return;
    policy_data->policyReturnSize = policy_return_size;
    string_buffer_initialize(policy_data->policy);


    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_policy_properties_callback;
    params.fromObsCallback = &get_bucket_policy_data_callback;
    params.complete_callback = &get_bucket_policy_complete_callback;
    params.callback_data = policy_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "policy";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket policy finish!");
}