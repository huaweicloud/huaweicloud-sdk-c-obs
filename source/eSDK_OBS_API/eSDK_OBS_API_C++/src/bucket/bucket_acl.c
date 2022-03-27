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
#include "simplexml.h"
#include "securec.h"
#include "util.h"
#include "bucket.h"
#include "request_util.h"
#include <openssl/md5.h> 

void set_bucket_acl(const obs_options * options, manager_acl_info * aclinfo,
    obs_response_handler * handler, void *callback_data)
{
    obs_type_acl type = TYPE_BUCKET_ACL;
    set_common_acl(options, aclinfo, type, handler, callback_data);
    COMMLOG(OBS_LOGINFO, "set bucket acl finish!");
}

void set_bucket_acl_by_head(const obs_options * options, obs_canned_acl canned_acl,
    obs_response_handler * handler, void *callback_data)
{
    set_object_acl_by_head(options, NULL, canned_acl, handler, callback_data);
    COMMLOG(OBS_LOGINFO, "set bucket acl by head finish!");
}

void get_bucket_acl(const obs_options * options, manager_acl_info * aclinfo,
    obs_response_handler * handler, void *callback_data)
{
    get_object_acl(options, aclinfo, handler, callback_data);
    COMMLOG(OBS_LOGINFO, "get bucket acl finish!");
}