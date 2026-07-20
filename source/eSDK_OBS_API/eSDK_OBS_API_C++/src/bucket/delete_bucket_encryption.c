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

typedef struct delete_encryption_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    void *callback_data;
} delete_encryption_data;

/**
 * 属性回调函数
 */
static obs_status delete_encryption_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    delete_encryption_data *deData = (delete_encryption_data *)callback_data;
    if (deData->response_properties_callback)
    {
        return (*(deData->response_properties_callback))(response_properties,
            deData->callback_data);
    }
    return OBS_STATUS_OK;
}

/**
 * 完成回调函数
 */
static void delete_encryption_complete_callback(obs_status requestStatus,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    delete_encryption_data *deData = (delete_encryption_data *)callback_data;

    (void)(*(deData->response_complete_callback))(requestStatus, obs_error_info, deData->callback_data);

    free(deData);
    deData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

/**
 * 初始化删除桶加密配置的数据结构
 */
static delete_encryption_data* init_delete_encryption_data(obs_response_handler *handler,
    void *callback_data)
{
    delete_encryption_data *deData = NULL;

    deData = (delete_encryption_data *)malloc(sizeof(delete_encryption_data));
    if (!deData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc delete_encryption_data failed.");
        return NULL;
    }
    memset_s(deData, sizeof(delete_encryption_data), 0, sizeof(delete_encryption_data));

    deData->response_complete_callback = handler->complete_callback;
    deData->response_properties_callback = handler->properties_callback;
    deData->callback_data = callback_data;

    return deData;
}

/**
 * 删除桶加密配置
 */
void delete_bucket_encryption(const obs_options *options,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    delete_encryption_data *deData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "delete_bucket_encryption start !");

    // 参数校验
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    // 初始化数据结构
    deData = init_delete_encryption_data(handler, callback_data);
    if (!deData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc delete_encryption_data failed.");
        return;
    }

    // 初始化请求参数
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_delete;
    params.properties_callback = &delete_encryption_properties_callback;
    params.complete_callback = &delete_encryption_complete_callback;
    params.callback_data = deData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "encryption";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;

    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "delete_bucket_encryption finish.");
}
