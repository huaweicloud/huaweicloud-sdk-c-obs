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
#include "request_util.h"
#include "util.h"
#include "policy_json_common.h"
#include "get_json_policy_common.h"


static void free_obs_compress_policy_rule(obs_compress_policy_rule *rule)
{
    unsigned int i = 0;

    if (!rule) {
        return;
    }

    // Free string fields
    CHECK_NULL_FREE(rule->id);
    CHECK_NULL_FREE(rule->project);
    CHECK_NULL_FREE(rule->agency);
    CHECK_NULL_FREE(rule->prefix);
    CHECK_NULL_FREE(rule->suffix);
    CHECK_NULL_FREE(rule->decompress_path);
    CHECK_NULL_FREE(rule->policy_type);

    // Free events string array
    if (rule->events) {
        for (i = 0; i < rule->events_number; i++) {
            CHECK_NULL_FREE(rule->events[i]);
        }
        CHECK_NULL_FREE(rule->events);
    }
}

static void parse_cjson_string_field(cJSON *parent, const char *key, char **out)
{
    const char *val = cjson_get_string(cJSON_GetObjectItemCaseSensitive(parent, key));
    if (val) {
        *out = strdup(val);
    }
}

static void parse_compress_policy_rule(cJSON *rule_obj, obs_compress_policy_rule *rule)
{
    parse_cjson_string_field(rule_obj, "id", &rule->id);
    parse_cjson_string_field(rule_obj, "project", &rule->project);
    parse_cjson_string_field(rule_obj, "agency", &rule->agency);

    // events
    cJSON *events = cJSON_GetObjectItemCaseSensitive(rule_obj, "events");
    if (events && cJSON_IsArray(events)) {
        parse_string_array(events, &rule->events, &rule->events_number);
    }

    parse_cjson_string_field(rule_obj, "prefix", &rule->prefix);
    parse_cjson_string_field(rule_obj, "suffix", &rule->suffix);

    // overwrite
    cJSON *overwrite = cJSON_GetObjectItemCaseSensitive(rule_obj, "overwrite");
    if (overwrite && cJSON_IsNumber(overwrite)) {
        rule->overwrite = overwrite->valueint;
    }

    parse_cjson_string_field(rule_obj, "decompresspath", &rule->decompress_path);
    parse_cjson_string_field(rule_obj, "policytype", &rule->policy_type);
}

static obs_status parse_json_to_compress_policy_rules(const char *json_str,
    obs_compress_policy_rule **out_rules, unsigned int *out_count)
{
    cJSON *root = NULL;
    cJSON *rules_array = NULL;
    int rule_count = 0;
    int i = 0;
    obs_compress_policy_rule *rules = NULL;

    obs_status status = parse_json_rules_preamble(json_str, &root, &rules_array, &rule_count);
    if (status != OBS_STATUS_OK) {
        return status;
    }

    rules = (obs_compress_policy_rule*)malloc(sizeof(obs_compress_policy_rule) * rule_count);
    if (!rules) {
        cJSON_Delete(root);
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(rules, sizeof(obs_compress_policy_rule) * rule_count, 0, sizeof(obs_compress_policy_rule) * rule_count);

    for (i = 0; i < rule_count; i++) {
        cJSON *rule_obj = cJSON_GetArrayItem(rules_array, i);
        if (!rule_obj) continue;

        parse_compress_policy_rule(rule_obj, &rules[i]);
    }

    cJSON_Delete(root);
    *out_rules = rules;
    *out_count = (unsigned int)rule_count;
    return OBS_STATUS_OK;
}

/* Adapter functions for get_json_policy_common */
static obs_status parse_compress_rules_adapter(const char *json_str, void **out_rules, unsigned int *out_count)
{
    return parse_json_to_compress_policy_rules(json_str, (obs_compress_policy_rule**)out_rules, out_count);
}

static obs_status invoke_compress_callback_adapter(void *user_callback, void *rules, unsigned int rule_count, void *callback_data)
{
    return (*(obs_get_bucket_compress_policy_callback*)user_callback)((obs_compress_policy_rule*)rules, rule_count, callback_data);
}

static void free_compress_rules_adapter(void *rules, unsigned int rule_count)
{
    obs_compress_policy_rule *r = (obs_compress_policy_rule*)rules;
    for (unsigned int i = 0; i < rule_count; i++) {
        free_obs_compress_policy_rule(&r[i]);
    }
    free(r);
}

void get_bucket_compress_policy(const obs_options *options, obs_get_bucket_compress_policy_handler *handler,
    void *callback_data)
{
    request_params params;
    get_json_policy_data *data = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    COMMLOG(OBS_LOGINFO, "get_bucket_compress_policy start !");

    data = init_get_json_policy_data(
        handler->response_handler.properties_callback,
        handler->response_handler.complete_callback,
        handler->get_bucket_compress_policy_callback,
        callback_data,
        MAX_COMPRESS_POLICY_DOC_LEN,
        parse_compress_rules_adapter,
        invoke_compress_callback_adapter,
        free_compress_rules_adapter);
    if (NULL == data) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }

    obs_status status = copy_options_and_init_params(options, &params, &use_api, &handler->response_handler, callback_data);
    if (OBS_STATUS_OK != status) {
        CHECK_NULL_FREE(data->json_buf);
        CHECK_NULL_FREE(data);
        return;
    }

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_json_policy_properties_callback;
    params.fromObsCallback = &get_json_policy_data_callback;
    params.complete_callback = &get_json_policy_complete_callback;
    params.callback_data = data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "obscompresspolicy";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_compress_policy finish.");
}
