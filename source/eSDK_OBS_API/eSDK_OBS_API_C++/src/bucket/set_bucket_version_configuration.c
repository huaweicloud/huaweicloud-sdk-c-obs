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

obs_status init_set_bucket_version_data(const char *version_status, update_bucket_common_data **out_data)
{
    int tmplen = 0;
    int mark = 0;
    char *replace_status = 0;

    update_bucket_common_data *version_data = (update_bucket_common_data *)malloc(sizeof(update_bucket_common_data));
    if (!version_data)
    {
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(version_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));

    version_data->docLen = snprintf_s(version_data->doc, sizeof(version_data->doc), _TRUNCATE, "<VersioningConfiguration>");
    if (version_data->docLen < 0)
    {
        CHECK_NULL_FREE(version_data);
        return OBS_STATUS_InternalError;
    }
    if (!version_status) {
        COMMLOG(OBS_LOGERROR, "version_status for init_set_bucket_version_data is NULL");
        CHECK_NULL_FREE(version_data);
        return OBS_STATUS_InvalidArgument;
    }
    mark = pcre_replace(version_status, &replace_status);
    if (mark)
    {
        free(replace_status);
        replace_status = NULL;
    }

    tmplen = snprintf_s((version_data->doc) + (version_data->docLen), sizeof((version_data->doc)) - (version_data->docLen),
        _TRUNCATE, "<Status>%s</Status></VersioningConfiguration>", mark ? replace_status : version_status);

    version_data->docLen += tmplen;
    if (tmplen < 0)
    {
        CHECK_NULL_FREE(version_data);
        return OBS_STATUS_InternalError;
    }

    version_data->docBytesWritten = 0;
    *out_data = version_data;

    return OBS_STATUS_OK;
}


void set_bucket_version_configuration(const obs_options *options, const char *version_status,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    update_bucket_common_data  *version_data = NULL;
    obs_status status = OBS_STATUS_OK;

    COMMLOG(OBS_LOGINFO, "set bucket version configuration start!");
    status = init_set_bucket_version_data(version_status, &version_data);
    if (status != OBS_STATUS_OK)
    {
        COMMLOG(OBS_LOGERROR, "init set version data failed!");
        (void)(*(handler->complete_callback))(status, 0, 0);
        return;
    }
    version_data->complete_callback = handler->complete_callback;
    version_data->properties_callback = handler->properties_callback;
    version_data->callback_data = callback_data;

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
    params.toObsCallbackTotalSize = version_data->docLen;
    params.complete_callback = &update_bucket_common_complete_callback;
    params.callback_data = version_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "versioning";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket version configuration finish!");
}