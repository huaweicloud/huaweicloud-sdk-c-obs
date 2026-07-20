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
#ifndef POLICY_JSON_COMMON_H
#define POLICY_JSON_COMMON_H

#include "bucket.h"

/* Get string value from a cJSON object, return NULL if not a valid string */
const char* cjson_get_string(cJSON *obj);

/* Parse a cJSON array of strings into a char** array */
obs_status parse_string_array(cJSON *array_obj, char ***out_array, unsigned int *out_count);

/* Find the rules container object in a JSON response */
cJSON* find_rules_container(cJSON *root);

/* Common preamble for parsing JSON rules: validate params, parse JSON, find "rules" array */
obs_status parse_json_rules_preamble(const char *json_str,
    cJSON **out_root, cJSON **out_rules_array, int *out_rule_count);

/* Append data to a dynamically growing JSON buffer */
obs_status json_buffer_append(char **buf, int *buf_len, int *buf_size,
    const char *data, int data_size);

/* Finalize JSON serialization: attach rules array, print, validate length, copy */
obs_status json_rules_finalize(cJSON *root, cJSON *rules_array,
    char *json_str, int json_len);

/* Add a char** array as a cJSON array to a parent object */
void add_string_array_to_json(cJSON *parent, const char *name,
    char **arr, unsigned int count);

#endif
