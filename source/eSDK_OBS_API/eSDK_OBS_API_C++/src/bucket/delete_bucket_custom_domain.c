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
#include "object.h"
#include "bucket.h"
#include "request_util.h"
#include <openssl/md5.h> 

static obs_status handle_error_with_logging(char* errMsg, obs_status obsErrNo, obs_response_handler* handler, void* callback_data)
{
    COMMLOG(OBS_LOGERROR, errMsg);
    (void)(*(handler->complete_callback))(obsErrNo, 0, callback_data);
    return obsErrNo;
}

static void safe_memcpy_and_logging(void* dest, size_t dest_size, const void* src, size_t count, const char* func, int line) {
    errno_t err = memcpy_s(dest, dest_size, src, count);
    CheckAndLogNoneZero(err, "memcpy_s", func, line);
}

obs_status delete_bucket_custom_domain(const obs_options* options, obs_custom_domain* custom_domain,
    obs_response_handler* handler, void* callback_data)
{
    request_params     params;
    obs_use_api use_api = OBS_USE_API_S3;
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    (void)queryParamsLen; // 去除set but not used编译告警

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "delete_bucket_custom_domain start!");

    if (handler == NULL || handler->complete_callback == NULL) {
        COMMLOG(OBS_LOGERROR, "delete_bucket_custom_domain complate_callback is NULL.");
        return OBS_STATUS_InvalidParameter;
    }

    if (!options->bucket_options.bucket_name) {
        return handle_error_with_logging("delete_bucket_custom_domain bucket_name is NULL.",
            OBS_STATUS_InvalidBucketName, handler, callback_data);
    }

    if (custom_domain == NULL) {
        return handle_error_with_logging("delete_bucket_custom_domain custom_domain is NULL!",
            OBS_STATUS_InvalidCustomDomain, handler, callback_data);
    }

    if (custom_domain->domain_name == NULL) {
        return handle_error_with_logging("delete_bucket_custom_domain domain_name is invalid!",
            OBS_STATUS_InvalidDomainName, handler, callback_data);
    }

    if (snprintf_s(queryParams, sizeof(queryParams),
        _TRUNCATE, "customdomain=%s", custom_domain->domain_name) < 0) {
        return handle_error_with_logging("delete_bucket_custom_domain domain_name processing failed!",
            OBS_STATUS_InvalidParameter, handler, callback_data);
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    safe_memcpy_and_logging(&params.bucketContext, sizeof(obs_bucket_context),
        &options->bucket_options, sizeof(obs_bucket_context), __FUNCTION__, __LINE__);
    safe_memcpy_and_logging(&params.request_option, sizeof(obs_http_request_option),
        &options->request_options, sizeof(obs_http_request_option), __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_delete;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.temp_auth = options->temp_auth;
    params.queryParams = queryParams[0] ? queryParams : 0;
    //params.subResource = "customdomain";
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "delete_bucket_custom_domain finish.");
    return OBS_STATUS_OK;
}