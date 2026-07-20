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

#include <openssl/md5.h>

typedef struct set_compress_policy_data_ex
{
    put_doc_data base;  /* MUST be first member */
    char doc_buf[MAX_COMPRESS_POLICY_DOC_LEN];
} set_compress_policy_data_ex;

static void build_events_json_array(cJSON *rule_obj, obs_compress_policy_rule *rule)
{
    cJSON *events_array = NULL;
    unsigned int j = 0;

    if (!rule->events || rule->events_number == 0) {
        return;
    }

    events_array = cJSON_CreateArray();
    if (!events_array) {
        return;
    }

    for (j = 0; j < rule->events_number; j++) {
        if (rule->events[j]) {
            cJSON_AddItemToArray(events_array,
                cJSON_CreateString(rule->events[j]));
        }
    }
    cJSON_AddItemToObject(rule_obj, "events", events_array);
}

static obs_status build_compress_policy_rule_json(cJSON *rules_array, obs_compress_policy_rule *rule)
{
    cJSON *rule_obj = NULL;

    rule_obj = cJSON_CreateObject();
    if (!rule_obj) {
        COMMLOG(OBS_LOGERROR, "Failed to create cJSON rule object");
        return OBS_STATUS_OutOfMemory;
    }

    if (rule->id) {
        cJSON_AddStringToObject(rule_obj, "id", rule->id);
    }
    if (rule->project) {
        cJSON_AddStringToObject(rule_obj, "project", rule->project);
    }
    if (rule->agency) {
        cJSON_AddStringToObject(rule_obj, "agency", rule->agency);
    }

    build_events_json_array(rule_obj, rule);

    if (rule->prefix) {
        cJSON_AddStringToObject(rule_obj, "prefix", rule->prefix);
    }
    if (rule->suffix) {
        cJSON_AddStringToObject(rule_obj, "suffix", rule->suffix);
    }

    cJSON_AddNumberToObject(rule_obj, "overwrite", rule->overwrite);

    if (rule->decompress_path) {
        cJSON_AddStringToObject(rule_obj, "decompresspath", rule->decompress_path);
    }
    if (rule->policy_type) {
        cJSON_AddStringToObject(rule_obj, "policytype", rule->policy_type);
    }

    cJSON_AddItemToArray(rules_array, rule_obj);
    return OBS_STATUS_OK;
}

static obs_status build_compress_policy_json(obs_compress_policy_rule *rules,
    unsigned int rule_number, char *json_str, int json_len)
{
    cJSON *root = NULL;
    cJSON *rules_array = NULL;
    unsigned int i = 0;
    obs_status status = OBS_STATUS_OK;

    if (!rules || rule_number == 0) {
        COMMLOG(OBS_LOGERROR, "Invalid parameters for build_compress_policy_json");
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
        status = build_compress_policy_rule_json(rules_array, &rules[i]);
        if (OBS_STATUS_OK != status) {
            cJSON_Delete(rules_array);
            cJSON_Delete(root);
            return status;
        }
    }

    return json_rules_finalize(root, rules_array, json_str, json_len);
}

static set_compress_policy_data_ex* init_compress_policy_data(obs_compress_policy_rule *compress_policy_rules,
    unsigned int rule_number, obs_response_handler *handler, void *callback_data)
{
    obs_status ret_status = OBS_STATUS_OK;
    set_compress_policy_data_ex *data = (set_compress_policy_data_ex *)malloc(sizeof(set_compress_policy_data_ex));
    if (!data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc set_compress_policy_data failed.");
        return NULL;
    }
    memset_s(data, sizeof(set_compress_policy_data_ex), 0, sizeof(set_compress_policy_data_ex));

    data->base.doc = data->doc_buf;
    data->base.doc_bytes_written = 0;

    ret_status = build_compress_policy_json(compress_policy_rules, rule_number,
                                              data->base.doc, sizeof(data->doc_buf));
    if (OBS_STATUS_OK != ret_status)
    {
        COMMLOG(OBS_LOGERROR, "build_compress_policy_json failed with status %d", ret_status);
        free(data);
        return NULL;
    }
    data->base.doc_len = (int)strlen(data->base.doc);
    if (data->base.doc_len <= 0)
    {
        COMMLOG(OBS_LOGERROR, "build_compress_policy_json produced empty JSON");
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

void set_bucket_compress_policy(const obs_options *options, obs_compress_policy_rule *compress_policy_rules,
    unsigned int compress_rule_count, obs_response_handler *handler, void *callback_data)
{
    request_params     params;
    obs_put_properties put_properties;
    set_compress_policy_data_ex *data = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "set_bucket_compress_policy start !");

    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    if (compress_rule_count <= 0 || compress_rule_count > OBS_MAX_COMPRESS_POLICY_RULES)
    {
        COMMLOG(OBS_LOGERROR, "set_bucket_compress_policy failed, compress_rule_count(%d) is invalid.", compress_rule_count);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    data = init_compress_policy_data(compress_policy_rules, compress_rule_count, handler, callback_data);
    if (!data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc set_compress_policy_data failed.");
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
    params.subResource = "obscompresspolicy";
    params.put_properties = &put_properties;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_compress_policy finish.");
}
