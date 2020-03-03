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
#ifndef COMMON_H
#define COMMON_H

#include "eSDKOBS.h"

typedef enum
{
    TYPE_OBJECT_ACL              = 0,
    TYPE_BUCKET_ACL              = 1
}obs_type_acl;


void set_common_acl(const obs_options *options, manager_acl_info *aclinfo, obs_type_acl type, obs_response_handler *handler, void *callback_data);

obs_status get_api_version(char *bucket_name,char *host_name,obs_protocol protocol);


#endif /* COMMON_H */

