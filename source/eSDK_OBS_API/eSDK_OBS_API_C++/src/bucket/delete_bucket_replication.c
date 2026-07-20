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

typedef struct delete_replication_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    void *callback_data;
} delete_replication_data;

/**
 * 属性回调函数
 */
static obs_status delete_replication_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    delete_replication_data *drData = (delete_replication_data *)callback_data;
    if (drData->response_properties_callback)
    {
        return (*(drData->response_properties_callback))(response_properties,
            drData->callback_data);
    }
    return OBS_STATUS_OK;
}

/**
 * 完成回调函数
 */
static void delete_replication_complete_callback(obs_status requestStatus,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    delete_replication_data *drData = (delete_replication_data *)callback_data;

    (void)(*(drData->response_complete_callback))(requestStatus, obs_error_info, drData->callback_data);

    free(drData);
    drData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

/**
 * 初始化删除跨区域复制配置的数据结构
 */
static delete_replication_data* init_delete_replication_data(obs_response_handler *handler,
    void *callback_data)
{
    delete_replication_data *drData = NULL;

    drData = (delete_replication_data *)malloc(sizeof(delete_replication_data));
    if (!drData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc delete_replication_data failed.");
        return NULL;
    }
    memset_s(drData, sizeof(delete_replication_data), 0, sizeof(delete_replication_data));

    drData->response_complete_callback = handler->complete_callback;
    drData->response_properties_callback = handler->properties_callback;
    drData->callback_data = callback_data;

    return drData;
}

/**
 * 删除桶跨区域复制配置
 */
void delete_bucket_replication(const obs_options *options,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    delete_replication_data *drData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "delete_bucket_replication start !");

    // 参数校验
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    // 初始化数据结构
    drData = init_delete_replication_data(handler, callback_data);
    if (!drData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc delete_replication_data failed.");
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
    params.properties_callback = &delete_replication_properties_callback;
    params.complete_callback = &delete_replication_complete_callback;
    params.callback_data = drData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "replication";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;

    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "delete_bucket_replication finish.");
}