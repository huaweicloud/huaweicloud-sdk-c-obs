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
#include "policy_json_common.h"
#include "request_util.h"

const char* cjson_get_string(cJSON *obj)
{
    return (obj && cJSON_IsString(obj) && obj->valuestring) ? obj->valuestring : NULL;
}

obs_status parse_string_array(cJSON *array_obj, char ***out_array, unsigned int *out_count)
{
    int count = 0;
    int i = 0;

    if (!array_obj || !cJSON_IsArray(array_obj)) {
        *out_array = NULL;
        *out_count = 0;
        return OBS_STATUS_OK;
    }

    count = cJSON_GetArraySize(array_obj);
    if (count <= 0) {
        *out_array = NULL;
        *out_count = 0;
        return OBS_STATUS_OK;
    }

    *out_array = (char**)malloc(sizeof(char*) * count);
    if (!*out_array) {
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(*out_array, sizeof(char*) * count, 0, sizeof(char*) * count);

    for (i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(array_obj, i);
        const char *val = cjson_get_string(item);
        if (val) {
            (*out_array)[i] = strdup(val);
        }
    }
    *out_count = (unsigned int)count;
    return OBS_STATUS_OK;
}

cJSON* find_rules_container(cJSON *root)
{
    cJSON *rules_container = root;
    if (cJSON_IsArray(root) && cJSON_GetArraySize(root) > 0) {
        rules_container = cJSON_GetArrayItem(root, 0);
    }
    return rules_container;
}

obs_status parse_json_rules_preamble(const char *json_str,
    cJSON **out_root, cJSON **out_rules_array, int *out_rule_count)
{
    if (!json_str || !out_root || !out_rules_array || !out_rule_count) {
        return OBS_STATUS_InvalidParameter;
    }

    *out_root = cJSON_Parse(json_str);
    if (!*out_root) {
        COMMLOG(OBS_LOGERROR, "Failed to parse JSON response");
        return OBS_STATUS_XmlParseFailure;
    }

    *out_rules_array = cJSON_GetObjectItemCaseSensitive(find_rules_container(*out_root), "rules");
    if (!*out_rules_array || !cJSON_IsArray(*out_rules_array)) {
        COMMLOG(OBS_LOGERROR, "No 'rules' array in JSON response");
        cJSON_Delete(*out_root);
        *out_root = NULL;
        return OBS_STATUS_XmlParseFailure;
    }

    *out_rule_count = cJSON_GetArraySize(*out_rules_array);
    if (*out_rule_count <= 0) {
        cJSON_Delete(*out_root);
        *out_root = NULL;
        return OBS_STATUS_InternalError;
    }

    return OBS_STATUS_OK;
}

obs_status json_buffer_append(char **buf, int *buf_len, int *buf_size,
    const char *data, int data_size)
{
    if (*buf_len + data_size >= *buf_size) {
        int new_size = *buf_size + data_size + 1;
        char *new_buf = (char*)malloc(new_size);
        if (!new_buf) {
            return OBS_STATUS_OutOfMemory;
        }
        if (memcpy_s(new_buf, new_size, *buf, *buf_len) != EOK) {
            free(new_buf);
            return OBS_STATUS_Security_Function_Failed;
        }
        free(*buf);
        *buf = new_buf;
        *buf_size = new_size;
    }

    if (memcpy_s(*buf + *buf_len, *buf_size - *buf_len,
             data, data_size) != EOK) {
        return OBS_STATUS_Security_Function_Failed;
    }
    *buf_len += data_size;
    (*buf)[*buf_len] = '\0';

    return OBS_STATUS_OK;
}

obs_status json_rules_finalize(cJSON *root, cJSON *rules_array,
    char *json_str, int json_len)
{
    char *json_output = NULL;

    cJSON_AddItemToObject(root, "rules", rules_array);

    json_output = cJSON_PrintUnformatted(root);
    if (!json_output) {
        COMMLOG(OBS_LOGERROR, "Failed to print JSON");
        cJSON_Delete(root);
        return OBS_STATUS_OutOfMemory;
    }

    if (strlen(json_output) >= (size_t)json_len) {
        COMMLOG(OBS_LOGERROR, "JSON output too long");
        free(json_output);
        cJSON_Delete(root);
        return OBS_STATUS_InvalidParameter;
    }

    if (strcpy_s(json_str, json_len, json_output) != EOK) {
        free(json_output);
        cJSON_Delete(root);
        return OBS_STATUS_Security_Function_Failed;
    }
    free(json_output);
    cJSON_Delete(root);

    return OBS_STATUS_OK;
}

void add_string_array_to_json(cJSON *parent, const char *name,
    char **arr, unsigned int count)
{
    unsigned int j;
    cJSON *array_obj = NULL;

    if (!arr || count == 0) {
        return;
    }

    array_obj = cJSON_CreateArray();
    if (!array_obj) {
        return;
    }

    for (j = 0; j < count; j++) {
        if (arr[j]) {
            cJSON_AddItemToArray(array_obj, cJSON_CreateString(arr[j]));
        }
    }
    cJSON_AddItemToObject(parent, name, array_obj);
}
