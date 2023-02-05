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
#include "request_util.h"
#include <openssl/md5.h> 

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int  get_object_conditions(obs_get_conditions *get_conditions, char *strToAppend, int append_len)
{
    int ret = 0;
    int isImageProcModeValid = 0;
    if (get_conditions->image_process_config->image_process_mode == obs_image_process_cmd)
    {
        ret = snprintf_s(strToAppend, sizeof(char)*append_len, _TRUNCATE, "image%s", "/");
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        isImageProcModeValid = 1;
    }
    else if (get_conditions->image_process_config->image_process_mode == obs_image_process_style)
    {
        ret = snprintf_s(strToAppend, sizeof(char)*append_len, _TRUNCATE, "style%s", "/");
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        isImageProcModeValid = 1;
    }
    else
    {
        COMMLOG(OBS_LOGWARN, "Image Process Mode is Not valid!");
        isImageProcModeValid = 0;
    }
    return isImageProcModeValid;
}

void get_object(const obs_options *options, obs_object_info *object_info,
    obs_get_conditions *get_conditions,
    server_side_encryption_params *encryption_params,
    obs_get_object_handler *handler, void *callback_data)
{

    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "Enter get_object successfully!");

    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);

    int amp = 0;
    if (object_info->version_id) {
        safe_append("versionId", object_info->version_id, strlen(object_info->version_id), handler->response_handler.complete_callback);
    }

    if (get_conditions && get_conditions->image_process_config)
    {
        char strToAppend[1024] = { 0 };
        int isImageProcModeValid = 0;
        int ret = 0;
        isImageProcModeValid = get_object_conditions(get_conditions, strToAppend, 1024);

        if (isImageProcModeValid == 1)
        {
            ret = snprintf_s(strToAppend, sizeof(strToAppend), _TRUNCATE,
                "%s%s", strToAppend, get_conditions->image_process_config->cmds_stylename);
            CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);

            safe_append("x-image-process", strToAppend, sizeof(strToAppend), handler->response_handler.complete_callback);
        }
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_get;
    params.key = object_info->key;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.get_conditions = get_conditions;
    params.encryption_params = encryption_params;
    params.fromObsCallback = handler->get_object_data_callback;
    params.properties_callback = handler->response_handler.properties_callback;
    params.complete_callback = handler->response_handler.complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave get_object successfully!");
}