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
#include "tagging_common.h"

void get_bucket_tagging(const obs_options *options,
    obs_get_bucket_tagging_handler *handler,
    void *callback_data)
{
    get_tagging_impl(options, NULL, 0, &handler->response_handler,
        handler->get_bucket_tagging_callback, callback_data);
}
