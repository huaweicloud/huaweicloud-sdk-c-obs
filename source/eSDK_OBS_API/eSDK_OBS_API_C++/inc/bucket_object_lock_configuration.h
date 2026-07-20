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
#ifndef BUCKET_OBJECT_LOCK_CONFIGURATION
#define BUCKET_OBJECT_LOCK_CONFIGURATION
#include "eSDKOBS.h"

char* get_bucket_object_lock_sub_resource(obs_use_api use_api);
obs_status check_bucket_object_lock_config_params(const char* function, const obs_options *options,
	obs_bucket_object_lock_configuration *object_lock_config,
	obs_response_handler *handler, void *callback_data);
obs_status init_bucket_object_lock_xml(obs_bucket_object_lock_configuration *object_lock_config,
	char *xml_doc, int xml_doc_size, int *xml_doc_len);
obs_status parse_bucket_object_lock_xml(const char *xml_doc, int xml_doc_len,
	obs_bucket_object_lock_configuration *object_lock_config_return);

#endif
