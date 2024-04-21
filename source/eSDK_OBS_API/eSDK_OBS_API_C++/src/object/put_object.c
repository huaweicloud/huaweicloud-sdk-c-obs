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

void put_object(const obs_options *options, char *key, uint64_t content_length,
    obs_put_properties *put_properties,
    server_side_encryption_params *encryption_params,
    obs_put_object_handler *handler, void *callback_data)
{

    request_params params;
    COMMLOG(OBS_LOGINFO, "Enter put_object successfully !");
    obs_use_api use_api = OBS_USE_API_S3;
	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, &handler->response_handler, callback_data)) {
		return;
	}
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_put;
    params.key = key;
    params.put_properties = put_properties;
    params.encryption_params = encryption_params;
    params.toObsCallback = handler->put_object_data_callback;
    params.toObsCallbackTotalSize = content_length;
    params.properties_callback = handler->response_handler.properties_callback;
    params.complete_callback = handler->response_handler.complete_callback;
    params.progressCallback = handler->progress_callback;
    params.callback_data = callback_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave put_object successfully !");
}