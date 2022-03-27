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

int obs_options_obj_or_bucket_optionsJudgment(char(*request_method)[OBS_COMMON_LEN_256],
    int is_bucket, char* key, char* origin, obs_response_handler *handler,
    const obs_options *options)
{
    int ret = 0;
    if (NULL == request_method || NULL == origin)
    {
        COMMLOG(OBS_LOGERROR, "requestMethod or origin is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, 0);
        ret = 1;
        return ret;
    }
    if (0 == is_bucket && (NULL == key || !strlen(key))) {
        COMMLOG(OBS_LOGERROR, "Key is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, 0);
        ret = 1;
        return ret;
    }
    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, 0);
        ret = 1;
        return ret;
    }
    return ret;
}

void obs_options_obj_or_bucket(const obs_options *options, int is_bucket, char* key, char* origin,
    char(*request_method)[OBS_COMMON_LEN_256], unsigned int method_number,
    char(*request_header)[OBS_COMMON_LEN_256], unsigned int header_number,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    if (obs_options_obj_or_bucket_optionsJudgment(request_method, is_bucket, key,
        origin, handler, options)) {
        return;
    }
    unsigned int i = 0;
    obs_cors_conf corsConf;
    corsConf.origin = origin;
    corsConf.rmNumber = method_number;
    corsConf.rhNumber = header_number;
    for (i = 0; i < method_number; i++)
    {
        corsConf.requestMethod[i] = request_method[i];
    }

    for (i = 0; i < header_number; i++)
    {
        corsConf.requestHeader[i] = request_header[i];
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
    params.httpRequestType = http_request_type_options;
    params.key = key;
    params.corsConf = &corsConf;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void obs_options_object(const obs_options *options, char* key, char* origin,
    char(*request_method)[OBS_COMMON_LEN_256], unsigned int method_number,
    char(*request_header)[OBS_COMMON_LEN_256], unsigned int header_number,
    obs_response_handler *handler, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    obs_options_obj_or_bucket(options, 0, key, origin, request_method, method_number,
        request_header, header_number, handler, callback_data);

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}
