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

void set_object_metadata(const obs_options *options, obs_object_info *object_info,
    obs_put_properties *put_properties,
    server_side_encryption_params *encryption_params,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter set_object_metadata successfully !");

    if (NULL == object_info->key || !strlen(object_info->key)) {
        COMMLOG(OBS_LOGERROR, "key is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidKey, 0, callback_data);
        return;
    }
    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if (put_properties->metadata_action == OBS_NO_METADATA_ACTION) {
        COMMLOG(OBS_LOGERROR, "put_properties.metadata_action is OBS_NO_METADATA_ACTION!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }


    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);

    int amp = 0;
    if (object_info->version_id) {
        safe_append_with_interface_log("versionId", object_info->version_id,
            strlen(object_info->version_id), handler->complete_callback);
    }
    errno_t err = EOK;
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_put;
    params.key = object_info->key;
    params.put_properties = put_properties;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.encryption_params = encryption_params;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "metadata";
    params.use_api = use_api;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave set_object_metadata successfully !");
}