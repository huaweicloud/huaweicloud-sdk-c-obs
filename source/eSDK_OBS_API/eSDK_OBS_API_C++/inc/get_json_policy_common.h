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
#ifndef GET_JSON_POLICY_COMMON_H
#define GET_JSON_POLICY_COMMON_H

#include "bucket.h"
#include "policy_json_common.h"

/* Function pointer types for policy-specific operations */
typedef obs_status (*parse_json_rules_fn)(const char *json_str, void **out_rules, unsigned int *out_count);
typedef obs_status (*invoke_user_callback_fn)(void *user_callback, void *rules, unsigned int rule_count, void *callback_data);
typedef void (*free_rules_fn)(void *rules, unsigned int rule_count);

/* Common data struct for GET-JSON-policy requests.
 * Must be the FIRST member of any struct that uses the shared callbacks. */
typedef struct get_json_policy_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    void *user_callback;
    void *callback_data;

    char *json_buf;
    int json_buf_len;
    int json_buf_size;

    /* Policy-specific function pointers */
    parse_json_rules_fn     parse_rules;
    invoke_user_callback_fn invoke_callback;
    free_rules_fn           free_rules;
} get_json_policy_data;

/* Initialize the common data struct.
 * Returns NULL on OOM. Caller must free. */
get_json_policy_data* init_get_json_policy_data(
    obs_response_properties_callback *properties_cb,
    obs_response_complete_callback *complete_cb,
    void *user_callback,
    void *callback_data,
    int json_buf_size,
    parse_json_rules_fn parse_rules,
    invoke_user_callback_fn invoke_callback,
    free_rules_fn free_rules);

/* Shared properties callback */
obs_status get_json_policy_properties_callback(
    const obs_response_properties *response_properties, void *callback_data);

/* Shared data callback -- appends to json_buf via json_buffer_append */
obs_status get_json_policy_data_callback(int buffer_size, const char *buffer,
    void *callback_data);

/* Shared complete callback -- parse JSON, invoke user callback, free rules, free buffers */
void get_json_policy_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info, void *callback_data);

#endif
