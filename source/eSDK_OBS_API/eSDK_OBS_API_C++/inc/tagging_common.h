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
#ifndef TAGGING_COMMON_H
#define TAGGING_COMMON_H

#include "bucket.h"

/* Shared implementation for get_bucket_tagging and get_object_tagging.
 * require_key: 1 for object tagging (key is required), 0 for bucket tagging. */
void get_tagging_impl(const obs_options *options, const char *key,
    int require_key,
    obs_response_handler *response_handler,
    obs_get_bucket_tagging_callback *tagging_callback,
    void *callback_data);

/* Shared implementation for set_bucket_tagging and set_object_tagging.
 * require_key: 1 for object tagging (key is required), 0 for bucket tagging. */
void set_tagging_impl(const obs_options *options, const char *key,
    int require_key,
    obs_name_value *tagging_list, unsigned int number,
    obs_response_handler *handler, void *callback_data);

#endif
