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
#include "put_doc_common.h"

typedef struct set_mirror_back_to_source_data_ex
{
    put_doc_data base;  /* MUST be first member */
    char doc_buf[MAX_MIRROR_BACK_TO_SOURCE_DOC_LEN];
} set_mirror_back_to_source_data_ex;

// Build the "condition" sub-object for a mirror rule
static void build_mirror_condition_json(cJSON *rule_obj,
    obs_mirror_back_to_source_condition *condition)
{
    cJSON *condition_obj = NULL;

    if (!condition->http_error_code_returned_equals &&
        !condition->key_prefix_equals) {
        return;
    }

    condition_obj = cJSON_CreateObject();
    if (!condition_obj) {
        return;
    }

    if (condition->http_error_code_returned_equals) {
        cJSON_AddStringToObject(condition_obj, "httpErrorCodeReturnedEquals",
            condition->http_error_code_returned_equals);
    }
    if (condition->key_prefix_equals) {
        cJSON_AddStringToObject(condition_obj, "objectKeyPrefixEquals",
            condition->key_prefix_equals);
    }
    cJSON_AddItemToObject(rule_obj, "condition", condition_obj);
}

// Build the "sourceEndpoint" sub-object (master/slave arrays)
static void build_mirror_source_endpoint_json(cJSON *redirect_obj,
    obs_mirror_back_to_source_source_endpoint *endpoint)
{
    cJSON *public_source_obj = NULL;
    cJSON *source_endpoint_obj = NULL;

    if (!endpoint->master && !endpoint->slave) {
        return;
    }

    public_source_obj = cJSON_CreateObject();
    if (!public_source_obj) {
        return;
    }

    source_endpoint_obj = cJSON_CreateObject();
    if (!source_endpoint_obj) {
        cJSON_Delete(public_source_obj);
        return;
    }

    if (endpoint->master && endpoint->master_number > 0) {
        add_string_array_to_json(source_endpoint_obj, "master",
            endpoint->master, endpoint->master_number);
    }

    if (endpoint->slave && endpoint->slave_number > 0) {
        add_string_array_to_json(source_endpoint_obj, "slave",
            endpoint->slave, endpoint->slave_number);
    }

    cJSON_AddItemToObject(public_source_obj, "sourceEndpoint", source_endpoint_obj);
    cJSON_AddItemToObject(redirect_obj, "publicSource", public_source_obj);
}

static void build_set_header_json_array(cJSON *mirror_http_header_obj,
    obs_mirror_back_to_source_set_http_header *set, unsigned int set_number)
{
    cJSON *set_array = NULL;
    unsigned int j;

    if (!set || set_number == 0) {
        return;
    }

    set_array = cJSON_CreateArray();
    if (!set_array) {
        return;
    }

    for (j = 0; j < set_number; j++) {
        if (set[j].key) {
            cJSON *set_obj = cJSON_CreateObject();
            if (set_obj) {
                cJSON_AddStringToObject(set_obj, "key", set[j].key);
                if (set[j].value) {
                    cJSON_AddStringToObject(set_obj, "value", set[j].value);
                }
                cJSON_AddItemToArray(set_array, set_obj);
            }
        }
    }
    cJSON_AddItemToObject(mirror_http_header_obj, "set", set_array);
}

// Build the "mirrorHttpHeader" sub-object (passAll, pass, remove, set arrays)
static void build_mirror_http_header_json(cJSON *redirect_obj,
    obs_mirror_back_to_source_http_header *header)
{
    cJSON *mirror_http_header_obj = NULL;

    if (!header->pass_all &&
        (!header->pass || header->pass_number == 0) &&
        (!header->remove || header->remove_number == 0) &&
        (!header->set || header->set_number == 0)) {
        return;
    }

    mirror_http_header_obj = cJSON_CreateObject();
    if (!mirror_http_header_obj) {
        return;
    }

    // passAll
    cJSON_AddBoolToObject(mirror_http_header_obj, "passAll", header->pass_all);

    // pass
    add_string_array_to_json(mirror_http_header_obj, "pass",
        header->pass, header->pass_number);

    // remove
    add_string_array_to_json(mirror_http_header_obj, "remove",
        header->remove, header->remove_number);

    // set (array of key-value objects)
    build_set_header_json_array(mirror_http_header_obj, header->set, header->set_number);

    cJSON_AddItemToObject(redirect_obj, "mirrorHttpHeader", mirror_http_header_obj);
}

// Build the "redirect" sub-object for a mirror rule
static void build_mirror_redirect_json(cJSON *rule_obj,
    obs_mirror_back_to_source_redirect *redirect)
{
    cJSON *redirect_obj = NULL;

    redirect_obj = cJSON_CreateObject();
    if (!redirect_obj) {
        return;
    }

    // agency
    if (redirect->agency) {
        cJSON_AddStringToObject(redirect_obj, "agency", redirect->agency);
    }

    // publicSource -> sourceEndpoint -> master/slave
    build_mirror_source_endpoint_json(redirect_obj,
        &redirect->public_source.source_endpoint);

    // retryConditions
    add_string_array_to_json(redirect_obj, "retryConditions",
        redirect->retry_conditions, redirect->retry_conditions_number);

    // passQueryString
    cJSON_AddBoolToObject(redirect_obj, "passQueryString", redirect->pass_query_string);

    // mirrorFollowRedirect
    cJSON_AddBoolToObject(redirect_obj, "mirrorFollowRedirect", redirect->mirror_follow_redirect);

    // mirrorHttpHeader
    build_mirror_http_header_json(redirect_obj, &redirect->mirror_http_header);

    // replaceKeyWith
    if (redirect->replace_key_with) {
        cJSON_AddStringToObject(redirect_obj, "replaceKeyWith", redirect->replace_key_with);
    }

    // replaceKeyPrefixWith
    if (redirect->replace_key_prefix_with) {
        cJSON_AddStringToObject(redirect_obj, "replaceKeyPrefixWith",
            redirect->replace_key_prefix_with);
    }

    // vpcEndpointURN
    if (redirect->vpc_endpoint_urn) {
        cJSON_AddStringToObject(redirect_obj, "vpcEndpointURN", redirect->vpc_endpoint_urn);
    }

    // redirectWithoutReferer
    cJSON_AddBoolToObject(redirect_obj, "redirectWithoutReferer",
        redirect->redirect_without_referer);

    // mirrorAllowHttpMethod
    add_string_array_to_json(redirect_obj, "mirrorAllowHttpMethod",
        redirect->mirror_allow_http_method, redirect->mirror_allow_http_method_number);

    cJSON_AddItemToObject(rule_obj, "redirect", redirect_obj);
}

static obs_status build_mirror_back_to_source_json(obs_mirror_back_to_source_rule *rules,
    unsigned int rule_number, char *json_str, int json_len)
{
    cJSON *root = NULL;
    cJSON *rules_array = NULL;
    cJSON *rule_obj = NULL;
    unsigned int i;

    if (!rules || rule_number == 0) {
        COMMLOG(OBS_LOGERROR, "Invalid parameters for build_mirror_back_to_source_json");
        return OBS_STATUS_InvalidParameter;
    }

    root = cJSON_CreateObject();
    if (!root) {
        COMMLOG(OBS_LOGERROR, "Failed to create cJSON object");
        return OBS_STATUS_OutOfMemory;
    }

    rules_array = cJSON_CreateArray();
    if (!rules_array) {
        COMMLOG(OBS_LOGERROR, "Failed to create cJSON array");
        cJSON_Delete(root);
        return OBS_STATUS_OutOfMemory;
    }

    for (i = 0; i < rule_number; i++) {
        rule_obj = cJSON_CreateObject();
        if (!rule_obj) {
            COMMLOG(OBS_LOGERROR, "Failed to create cJSON rule object");
            cJSON_Delete(rules_array);
            cJSON_Delete(root);
            return OBS_STATUS_OutOfMemory;
        }

        // id
        if (rules[i].id) {
            cJSON_AddStringToObject(rule_obj, "id", rules[i].id);
        }

        // condition
        build_mirror_condition_json(rule_obj, &rules[i].condition);

        // redirect
        build_mirror_redirect_json(rule_obj, &rules[i].redirect);

        cJSON_AddItemToArray(rules_array, rule_obj);
    }

    return json_rules_finalize(root, rules_array, json_str, json_len);
}

static set_mirror_back_to_source_data_ex* init_mirror_back_to_source_data(obs_mirror_back_to_source_rule *mirror_back_to_source_rules,
    unsigned int rule_number, obs_response_handler *handler, void *callback_data)
{
    obs_status ret_status = OBS_STATUS_OK;
    set_mirror_back_to_source_data_ex *data = (set_mirror_back_to_source_data_ex *)malloc(sizeof(set_mirror_back_to_source_data_ex));
    if (!data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc mirror_back_to_source_data failed.");
        return NULL;
    }
    memset_s(data, sizeof(set_mirror_back_to_source_data_ex), 0, sizeof(set_mirror_back_to_source_data_ex));

    data->base.doc = data->doc_buf;
    data->base.doc_bytes_written = 0;

    ret_status = build_mirror_back_to_source_json(mirror_back_to_source_rules, rule_number,
                                                   data->base.doc, sizeof(data->doc_buf));
    if (OBS_STATUS_OK != ret_status)
    {
        COMMLOG(OBS_LOGERROR, "build_mirror_back_to_source_json failed with status %d", ret_status);
        free(data);
        return NULL;
    }
    data->base.doc_len = (int)strlen(data->base.doc);
    if (data->base.doc_len <= 0)
    {
        COMMLOG(OBS_LOGERROR, "build_mirror_back_to_source_json produced empty JSON");
        free(data);
        return NULL;
    }
    COMMLOG(OBS_LOGINFO, "request json: %s.", data->base.doc);

    data->base.response_complete_callback = handler->complete_callback;
    data->base.response_properties_callback = handler->properties_callback;
    data->base.callback_data = callback_data;

    put_doc_compute_md5(&data->base);

    return data;
}

void set_bucket_mirror_back_to_source(const obs_options *options, obs_mirror_back_to_source_rule *mirror_back_to_source_rules,
    unsigned int mirror_rule_count, obs_response_handler *handler, void *callback_data)
{
    request_params     params;
    obs_put_properties put_properties;
    set_mirror_back_to_source_data_ex *data = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "set_bucket_mirror_back_to_source start !");

    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    if (mirror_rule_count <= 0 || mirror_rule_count > OBS_MAX_MIRROR_BACK_TO_SOURCE_RULES)
    {
        COMMLOG(OBS_LOGERROR, "set_bucket_mirror_back_to_source failed, mirror_rule_count(%d) is invalid.", mirror_rule_count);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    data = init_mirror_back_to_source_data(mirror_back_to_source_rules, mirror_rule_count, handler, callback_data);
    if (!data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc mirror_back_to_source_data failed.");
        return;
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    memset_s(&put_properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    put_properties.md5 = data->base.doc_md5;
    put_properties.canned_acl = OBS_CANNED_ACL_PRIVATE;
    put_properties.content_type = "application/json";

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &put_doc_properties_callback;
    params.toObsCallback = &put_doc_data_callback;
    params.complete_callback = &put_doc_complete_callback;
    params.toObsCallbackTotalSize = data->base.doc_len;
    params.callback_data = data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "mirrorBackToSource";
    params.put_properties = &put_properties;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_mirror_back_to_source finish.");
}
