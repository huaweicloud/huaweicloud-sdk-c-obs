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

obs_status init_set_bucket_quota_cbdata(uint64_t storage_quota, update_bucket_common_data **data)
{
    update_bucket_common_data *quota_data = (update_bucket_common_data *)malloc(sizeof(update_bucket_common_data));
    if (!quota_data)
    {
        *data = NULL;
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(quota_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));


    quota_data->docLen = snprintf_s(quota_data->doc, sizeof(quota_data->doc), _TRUNCATE,
        "<Quota><StorageQuota>%lu</StorageQuota></Quota>", storage_quota);
    if (quota_data->docLen < 0)
    {
        *data = NULL;
        CHECK_NULL_FREE(quota_data);
        return OBS_STATUS_InternalError;
    }

    quota_data->docBytesWritten = 0;
    *data = quota_data;

    return OBS_STATUS_OK;
}


void set_bucket_quota(const obs_options *options, uint64_t storage_quota,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    obs_put_properties  properties;
    update_bucket_common_data  *quota_data = NULL;
    obs_status status = OBS_STATUS_OK;

    COMMLOG(OBS_LOGINFO, "set bucket quota start!");

    status = init_set_bucket_quota_cbdata(storage_quota, &quota_data);
    if (status != OBS_STATUS_OK)
    {
        COMMLOG(OBS_LOGERROR, "Malloc update_bucket_common_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    quota_data->complete_callback = handler->complete_callback;
    quota_data->callback_data = callback_data;
    quota_data->properties_callback = handler->properties_callback;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.put_properties = &properties;
    params.httpRequestType = http_request_type_put;
    params.properties_callback = &update_bucket_common_properties_callback;
    params.toObsCallback = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = quota_data->docLen;
    params.complete_callback = &update_bucket_common_complete_callback;
    params.callback_data = quota_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "quota";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket quota finish!");
}
