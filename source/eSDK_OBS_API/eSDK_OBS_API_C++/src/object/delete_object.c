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

void delete_object(const obs_options *options, obs_object_info *object_info,
    obs_response_handler *handler, void *callback_data)
{

    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    COMMLOG(OBS_LOGINFO, "Enter delete_object successfully !");
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;
    if (object_info->version_id)
    {
        safe_append("versionId", object_info->version_id, strlen(object_info->version_id), handler->complete_callback);
    }

	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, handler, callback_data)) {
		return;
	}

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_delete;
    params.key = object_info->key;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave delete_object successfully !");
}