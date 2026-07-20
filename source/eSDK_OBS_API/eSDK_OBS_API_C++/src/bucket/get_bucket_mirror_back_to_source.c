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
#include "policy_json_common.h"
#include "get_json_policy_common.h"

// Helper: free a char** array with count elements
static void free_string_array(char **arr, unsigned int count)
{
    unsigned int i;
    if (!arr) return;
    for (i = 0; i < count; i++) {
        if (arr[i]) { free(arr[i]); }
    }
    free(arr);
}

// Helper: free set_http_header array
static void free_set_headers(obs_mirror_back_to_source_set_http_header *arr, unsigned int count)
{
    unsigned int i;
    if (!arr) return;
    for (i = 0; i < count; i++) {
        if (arr[i].key) free(arr[i].key);
        if (arr[i].value) free(arr[i].value);
    }
    free(arr);
}

static void free_obs_mirror_back_to_source_rule(obs_mirror_back_to_source_rule *rule)
{
    if (!rule) {
        return;
    }

    // Free simple string fields
    CHECK_NULL_FREE(rule->id);
    CHECK_NULL_FREE(rule->condition.http_error_code_returned_equals);
    CHECK_NULL_FREE(rule->condition.key_prefix_equals);
    CHECK_NULL_FREE(rule->redirect.agency);
    CHECK_NULL_FREE(rule->redirect.replace_key_prefix_with);
    CHECK_NULL_FREE(rule->redirect.replace_key_with);
    CHECK_NULL_FREE(rule->redirect.vpc_endpoint_urn);

    // Free arrays
    free_string_array(rule->redirect.public_source.source_endpoint.master,
        rule->redirect.public_source.source_endpoint.master_number);
    rule->redirect.public_source.source_endpoint.master = NULL;

    free_string_array(rule->redirect.public_source.source_endpoint.slave,
        rule->redirect.public_source.source_endpoint.slave_number);
    rule->redirect.public_source.source_endpoint.slave = NULL;

    free_string_array(rule->redirect.retry_conditions,
        rule->redirect.retry_conditions_number);
    rule->redirect.retry_conditions = NULL;

    free_string_array(rule->redirect.mirror_http_header.pass,
        rule->redirect.mirror_http_header.pass_number);
    rule->redirect.mirror_http_header.pass = NULL;

    free_string_array(rule->redirect.mirror_http_header.remove,
        rule->redirect.mirror_http_header.remove_number);
    rule->redirect.mirror_http_header.remove = NULL;

    free_set_headers(rule->redirect.mirror_http_header.set,
        rule->redirect.mirror_http_header.set_number);
    rule->redirect.mirror_http_header.set = NULL;

    free_string_array(rule->redirect.mirror_allow_http_method,
        rule->redirect.mirror_allow_http_method_number);
    rule->redirect.mirror_allow_http_method = NULL;
}

// Parse the condition sub-object of a mirror rule
static void parse_mirror_condition(cJSON *condition, obs_mirror_back_to_source_rule *rule)
{
    const char *prefix;
    cJSON *http_code;

    if (!condition || !cJSON_IsObject(condition)) return;

    prefix = cjson_get_string(cJSON_GetObjectItemCaseSensitive(condition, "objectKeyPrefixEquals"));
    if (prefix) {
        rule->condition.key_prefix_equals = strdup(prefix);
    }

    http_code = cJSON_GetObjectItemCaseSensitive(condition, "httpErrorCodeReturnedEquals");
    if (http_code) {
        if (cJSON_IsNumber(http_code)) {
            char code_str[16];
            snprintf_s(code_str, sizeof(code_str), _TRUNCATE, "%d", http_code->valueint);
            rule->condition.http_error_code_returned_equals = strdup(code_str);
        } else {
            const char *code_str = cjson_get_string(http_code);
            if (code_str) {
                rule->condition.http_error_code_returned_equals = strdup(code_str);
            }
        }
    }
}

// Parse the sourceEndpoint sub-object
static void parse_mirror_source_endpoint(cJSON *source_endpoint,
    obs_mirror_back_to_source_source_endpoint *endpoint)
{
    cJSON *master;
    cJSON *slave;

    if (!source_endpoint || !cJSON_IsObject(source_endpoint)) return;

    master = cJSON_GetObjectItemCaseSensitive(source_endpoint, "master");
    if (master && cJSON_IsArray(master)) {
        parse_string_array(master, &endpoint->master, &endpoint->master_number);
    }

    slave = cJSON_GetObjectItemCaseSensitive(source_endpoint, "slave");
    if (slave && cJSON_IsArray(slave)) {
        parse_string_array(slave, &endpoint->slave, &endpoint->slave_number);
    }
}

static obs_status parse_set_header_array(cJSON *set_arr,
    obs_mirror_back_to_source_http_header *header)
{
    int set_count = cJSON_GetArraySize(set_arr);
    int k;

    if (set_count <= 0) {
        return OBS_STATUS_OK;
    }

    header->set = (obs_mirror_back_to_source_set_http_header*)malloc(
        sizeof(obs_mirror_back_to_source_set_http_header) * set_count);
    if (!header->set) {
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(header->set, sizeof(obs_mirror_back_to_source_set_http_header) * set_count, 0,
        sizeof(obs_mirror_back_to_source_set_http_header) * set_count);
    header->set_number = (unsigned int)set_count;

    for (k = 0; k < set_count; k++) {
        cJSON *set_obj = cJSON_GetArrayItem(set_arr, k);
        if (set_obj && cJSON_IsObject(set_obj)) {
            const char *key = cjson_get_string(cJSON_GetObjectItemCaseSensitive(set_obj, "key"));
            if (key) {
                header->set[k].key = strdup(key);
            }
            const char *val = cjson_get_string(cJSON_GetObjectItemCaseSensitive(set_obj, "value"));
            if (val) {
                header->set[k].value = strdup(val);
            }
        }
    }
    return OBS_STATUS_OK;
}

// Parse the mirrorHttpHeader sub-object
static obs_status parse_mirror_http_header(cJSON *mirror_http_header,
    obs_mirror_back_to_source_http_header *header)
{
    cJSON *pass_all;
    cJSON *pass_arr;
    cJSON *remove_arr;
    cJSON *set_arr;

    if (!mirror_http_header || !cJSON_IsObject(mirror_http_header)) {
        return OBS_STATUS_OK;
    }

    pass_all = cJSON_GetObjectItemCaseSensitive(mirror_http_header, "passAll");
    if (pass_all && cJSON_IsBool(pass_all)) {
        header->pass_all = cJSON_IsTrue(pass_all) ? 1 : 0;
    }

    pass_arr = cJSON_GetObjectItemCaseSensitive(mirror_http_header, "pass");
    if (pass_arr && cJSON_IsArray(pass_arr)) {
        parse_string_array(pass_arr, &header->pass, &header->pass_number);
    }

    remove_arr = cJSON_GetObjectItemCaseSensitive(mirror_http_header, "remove");
    if (remove_arr && cJSON_IsArray(remove_arr)) {
        parse_string_array(remove_arr, &header->remove, &header->remove_number);
    }

    set_arr = cJSON_GetObjectItemCaseSensitive(mirror_http_header, "set");
    if (set_arr && cJSON_IsArray(set_arr)) {
        return parse_set_header_array(set_arr, header);
    }

    return OBS_STATUS_OK;
}

static void parse_cjson_string_to_field(cJSON *parent, const char *key, char **out)
{
    const char *val = cjson_get_string(cJSON_GetObjectItemCaseSensitive(parent, key));
    if (val) *out = strdup(val);
}

static void parse_cjson_bool_to_field(cJSON *parent, const char *key, int *out)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, key);
    if (item && cJSON_IsBool(item)) {
        *out = cJSON_IsTrue(item) ? 1 : 0;
    }
}

static obs_status parse_redirect_string_fields(cJSON *redirect,
    obs_mirror_back_to_source_rule *rule)
{
    parse_cjson_string_to_field(redirect, "agency", &rule->redirect.agency);
    parse_cjson_string_to_field(redirect, "replaceKeyWith", &rule->redirect.replace_key_with);
    parse_cjson_string_to_field(redirect, "replaceKeyPrefixWith", &rule->redirect.replace_key_prefix_with);
    parse_cjson_string_to_field(redirect, "vpcEndpointURN", &rule->redirect.vpc_endpoint_urn);
    return OBS_STATUS_OK;
}

static obs_status parse_redirect_bool_fields(cJSON *redirect,
    obs_mirror_back_to_source_rule *rule)
{
    parse_cjson_bool_to_field(redirect, "passQueryString", &rule->redirect.pass_query_string);
    parse_cjson_bool_to_field(redirect, "mirrorFollowRedirect", &rule->redirect.mirror_follow_redirect);
    parse_cjson_bool_to_field(redirect, "redirectWithoutReferer", &rule->redirect.redirect_without_referer);
    return OBS_STATUS_OK;
}

// Parse the redirect sub-object of a mirror rule
static obs_status parse_mirror_redirect(cJSON *redirect, obs_mirror_back_to_source_rule *rule)
{
    obs_status status;

    if (!redirect || !cJSON_IsObject(redirect)) return OBS_STATUS_OK;

    // String fields
    status = parse_redirect_string_fields(redirect, rule);
    if (status != OBS_STATUS_OK) return status;

    // publicSource -> sourceEndpoint
    cJSON *public_source = cJSON_GetObjectItemCaseSensitive(redirect, "publicSource");
    if (public_source && cJSON_IsObject(public_source)) {
        cJSON *source_endpoint = cJSON_GetObjectItemCaseSensitive(public_source, "sourceEndpoint");
        if (source_endpoint) {
            parse_mirror_source_endpoint(source_endpoint,
                &rule->redirect.public_source.source_endpoint);
        }
    }

    // retryConditions
    cJSON *retry_conditions = cJSON_GetObjectItemCaseSensitive(redirect, "retryConditions");
    if (retry_conditions && cJSON_IsArray(retry_conditions)) {
        parse_string_array(retry_conditions,
            &rule->redirect.retry_conditions,
            &rule->redirect.retry_conditions_number);
    }

    // Bool fields
    parse_redirect_bool_fields(redirect, rule);

    // mirrorHttpHeader
    cJSON *mirror_http_header = cJSON_GetObjectItemCaseSensitive(redirect, "mirrorHttpHeader");
    status = parse_mirror_http_header(mirror_http_header, &rule->redirect.mirror_http_header);
    if (status != OBS_STATUS_OK) return status;

    // mirrorAllowHttpMethod
    cJSON *http_methods = cJSON_GetObjectItemCaseSensitive(redirect, "mirrorAllowHttpMethod");
    if (http_methods && cJSON_IsArray(http_methods)) {
        parse_string_array(http_methods,
            &rule->redirect.mirror_allow_http_method,
            &rule->redirect.mirror_allow_http_method_number);
    }

    return OBS_STATUS_OK;
}

// Parse a single rule from JSON
static obs_status parse_single_mirror_rule(cJSON *rule_obj, obs_mirror_back_to_source_rule *rule)
{
    const char *str_val;
    cJSON *condition;
    cJSON *redirect;
    obs_status status;

    // id
    str_val = cjson_get_string(cJSON_GetObjectItemCaseSensitive(rule_obj, "id"));
    if (str_val) rule->id = strdup(str_val);

    // condition
    condition = cJSON_GetObjectItemCaseSensitive(rule_obj, "condition");
    parse_mirror_condition(condition, rule);

    // redirect
    redirect = cJSON_GetObjectItemCaseSensitive(rule_obj, "redirect");
    status = parse_mirror_redirect(redirect, rule);
    return status;
}

static obs_status parse_json_to_mirror_back_to_source_rules(const char *json_str,
    obs_mirror_back_to_source_rule **out_rules, unsigned int *out_count)
{
    cJSON *root = NULL;
    cJSON *rules_array = NULL;
    int rule_count = 0;
    int i = 0;
    obs_mirror_back_to_source_rule *rules = NULL;
    obs_status status;

    status = parse_json_rules_preamble(json_str, &root, &rules_array, &rule_count);
    if (status != OBS_STATUS_OK) {
        return status;
    }

    rules = (obs_mirror_back_to_source_rule*)malloc(sizeof(obs_mirror_back_to_source_rule) * rule_count);
    if (!rules) {
        cJSON_Delete(root);
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(rules, sizeof(obs_mirror_back_to_source_rule) * rule_count, 0, sizeof(obs_mirror_back_to_source_rule) * rule_count);

    for (i = 0; i < rule_count; i++) {
        cJSON *rule_obj = cJSON_GetArrayItem(rules_array, i);
        if (!rule_obj) continue;
        parse_single_mirror_rule(rule_obj, &rules[i]);
    }

    cJSON_Delete(root);
    *out_rules = rules;
    *out_count = (unsigned int)rule_count;
    return OBS_STATUS_OK;
}

/* Adapter functions for get_json_policy_common */
static obs_status parse_mirror_rules_adapter(const char *json_str, void **out_rules, unsigned int *out_count)
{
    return parse_json_to_mirror_back_to_source_rules(json_str, (obs_mirror_back_to_source_rule**)out_rules, out_count);
}

static obs_status invoke_mirror_callback_adapter(void *user_callback, void *rules, unsigned int rule_count, void *callback_data)
{
    return (*(obs_get_bucket_mirror_back_to_source_callback*)user_callback)((obs_mirror_back_to_source_rule*)rules, rule_count, callback_data);
}

static void free_mirror_rules_adapter(void *rules, unsigned int rule_count)
{
    obs_mirror_back_to_source_rule *r = (obs_mirror_back_to_source_rule*)rules;
    for (unsigned int i = 0; i < rule_count; i++) {
        free_obs_mirror_back_to_source_rule(&r[i]);
    }
    free(r);
}

void get_bucket_mirror_back_to_source(const obs_options *options, obs_get_bucket_mirror_back_to_source_handler *handler,
    void *callback_data)
{
    request_params params;
    get_json_policy_data *data = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    COMMLOG(OBS_LOGINFO, "get_bucket_mirror_back_to_source_configuration start !");

    data = init_get_json_policy_data(
        handler->response_handler.properties_callback,
        handler->response_handler.complete_callback,
        handler->get_bucket_mirror_back_to_source_callback,
        callback_data,
        MAX_MIRROR_BACK_TO_SOURCE_DOC_LEN,
        parse_mirror_rules_adapter,
        invoke_mirror_callback_adapter,
        free_mirror_rules_adapter);
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
    params.subResource = "mirrorBackToSource";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_mirror_back_to_source_configuration finish.");
}
