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

obs_status copy_options_and_init_params(const obs_options *options, request_params* params,
        obs_use_api* use_api, obs_response_handler *handler, void *callback_data){
    set_use_api_switch(options, use_api);

    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return OBS_STATUS_InvalidBucketName;
    }

    errno_t err = memset_s(params, sizeof(request_params), 0, sizeof(request_params));
    CheckAndLogNoneZero(err, "memset_s", __FUNCTION__, __LINE__);
	if (err != 0) {
		(void)(*(handler->complete_callback))(OBS_STATUS_Security_Function_Failed, 0, callback_data);
		return OBS_STATUS_Security_Function_Failed;
	}
    err = memcpy_s(&params->bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
	if (err != 0) {
		(void)(*(handler->complete_callback))(OBS_STATUS_Security_Function_Failed, 0, callback_data);
		return OBS_STATUS_Security_Function_Failed;
	}
    err = memcpy_s(&params->request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
	if (err != 0) {
		(void)(*(handler->complete_callback))(OBS_STATUS_Security_Function_Failed, 0, callback_data);
		return OBS_STATUS_Security_Function_Failed;
	}
	return OBS_STATUS_OK;
}

void delete_bucket_lifecycle_configuration(const obs_options *options, obs_response_handler *handler,
    void *callback_data)
{
    request_params params;
    COMMLOG(OBS_LOGINFO, "%s start!", __FUNCTION__);
    obs_use_api use_api = OBS_USE_API_S3;
	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, handler, callback_data)) {
		return;
	}

    params.httpRequestType = http_request_type_delete;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "lifecycle";
    params.temp_auth = options->temp_auth;
    params.callback_data = callback_data;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "%s finish!", __FUNCTION__);
}
