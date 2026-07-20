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
#include "get_json_policy_common.h"
#include "request_util.h"

get_json_policy_data* init_get_json_policy_data(
    obs_response_properties_callback *properties_cb,
    obs_response_complete_callback *complete_cb,
    void *user_callback,
    void *callback_data,
    int json_buf_size,
    parse_json_rules_fn parse_rules,
    invoke_user_callback_fn invoke_callback,
    free_rules_fn free_rules)
{
    get_json_policy_data *data = (get_json_policy_data*)malloc(sizeof(get_json_policy_data));
    if (!data) {
        COMMLOG(OBS_LOGERROR, "malloc get_json_policy_data failed.");
        return NULL;
    }
    memset_s(data, sizeof(get_json_policy_data), 0, sizeof(get_json_policy_data));

    data->json_buf_size = json_buf_size;
    data->json_buf = (char*)malloc(data->json_buf_size);
    if (!data->json_buf) {
        free(data);
        return NULL;
    }
    memset_s(data->json_buf, data->json_buf_size, 0, data->json_buf_size);
    data->json_buf_len = 0;

    data->response_properties_callback = properties_cb;
    data->response_complete_callback = complete_cb;
    data->user_callback = user_callback;
    data->callback_data = callback_data;
    data->parse_rules = parse_rules;
    data->invoke_callback = invoke_callback;
    data->free_rules = free_rules;

    return data;
}

obs_status get_json_policy_properties_callback(
    const obs_response_properties *response_properties, void *callback_data)
{
    get_json_policy_data *data = (get_json_policy_data*)callback_data;
    if (data->response_properties_callback) {
        return (*(data->response_properties_callback))(response_properties, data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status get_json_policy_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_json_policy_data *data = (get_json_policy_data*)callback_data;
    return json_buffer_append(&data->json_buf, &data->json_buf_len, &data->json_buf_size,
        buffer, buffer_size);
}

void get_json_policy_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    get_json_policy_data *data = (get_json_policy_data*)callback_data;

    if (OBS_STATUS_OK == request_status && data->json_buf_len > 0) {
        void *rules = NULL;
        unsigned int rule_count = 0;

        COMMLOG(OBS_LOGINFO, "Parsing JSON response (len=%d): %s", data->json_buf_len, data->json_buf);
        obs_status parse_status = data->parse_rules(data->json_buf, &rules, &rule_count);
        if (OBS_STATUS_OK == parse_status && rules && rule_count > 0) {
            request_status = data->invoke_callback(data->user_callback, rules, rule_count, data->callback_data);
            data->free_rules(rules, rule_count);
        } else if (OBS_STATUS_OK != parse_status) {
            request_status = parse_status;
        }
    }

    (*(data->response_complete_callback))(request_status, obs_error_info, data->callback_data);

    CHECK_NULL_FREE(data->json_buf);
    CHECK_NULL_FREE(data);

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}
