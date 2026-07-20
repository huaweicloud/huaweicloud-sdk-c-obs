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

typedef struct set_dis_policy_data_ex
{
    put_doc_data base;  /* MUST be first member */
    char doc_buf[MAX_DIS_POLICY_DOC_LEN];
} set_dis_policy_data_ex;

static void build_dis_rule_json(cJSON *rule_obj, obs_dis_policy_rule *rule)
{
    if (rule->id) {
        cJSON_AddStringToObject(rule_obj, "id", rule->id);
    }
    if (rule->stream) {
        cJSON_AddStringToObject(rule_obj, "stream", rule->stream);
    }
    if (rule->project) {
        cJSON_AddStringToObject(rule_obj, "project", rule->project);
    }
    add_string_array_to_json(rule_obj, "events", rule->events, rule->events_number);
    if (rule->prefix) {
        cJSON_AddStringToObject(rule_obj, "prefix", rule->prefix);
    }
    if (rule->suffix) {
        cJSON_AddStringToObject(rule_obj, "suffix", rule->suffix);
    }
    if (rule->agency) {
        cJSON_AddStringToObject(rule_obj, "agency", rule->agency);
    }
}

static obs_status build_dis_policy_json(obs_dis_policy_rule *rules,
    unsigned int rule_number, char *json_str, int json_len)
{
    cJSON *root = NULL;
    cJSON *rules_array = NULL;
    unsigned int i = 0;

    if (!rules || rule_number == 0) {
        COMMLOG(OBS_LOGERROR, "Invalid parameters for build_dis_policy_json");
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
        cJSON *rule_obj = cJSON_CreateObject();
        if (!rule_obj) {
            COMMLOG(OBS_LOGERROR, "Failed to create cJSON rule object");
            cJSON_Delete(rules_array);
            cJSON_Delete(root);
            return OBS_STATUS_OutOfMemory;
        }
        build_dis_rule_json(rule_obj, &rules[i]);
        cJSON_AddItemToArray(rules_array, rule_obj);
    }

    return json_rules_finalize(root, rules_array, json_str, json_len);
}

static set_dis_policy_data_ex* init_dis_policy_data(obs_dis_policy_rule *dis_policy_rules,
    unsigned int rule_number, obs_response_handler *handler, void *callback_data)
{
    obs_status ret_status = OBS_STATUS_OK;
    set_dis_policy_data_ex *data = (set_dis_policy_data_ex *)malloc(sizeof(set_dis_policy_data_ex));
    if (!data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc set_dis_policy_data failed.");
        return NULL;
    }
    memset_s(data, sizeof(set_dis_policy_data_ex), 0, sizeof(set_dis_policy_data_ex));

    data->base.doc = data->doc_buf;
    data->base.doc_bytes_written = 0;

    ret_status = build_dis_policy_json(dis_policy_rules, rule_number,
                                          data->base.doc, sizeof(data->doc_buf));
    if (OBS_STATUS_OK != ret_status)
    {
        COMMLOG(OBS_LOGERROR, "build_dis_policy_json failed with status %d", ret_status);
        free(data);
        return NULL;
    }
    data->base.doc_len = (int)strlen(data->base.doc);
    if (data->base.doc_len <= 0)
    {
        COMMLOG(OBS_LOGERROR, "build_dis_policy_json produced empty JSON");
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

void set_bucket_dis_policy(const obs_options *options, obs_dis_policy_rule *dis_policy_rules,
    unsigned int dis_rule_count, obs_response_handler *handler, void *callback_data)
{
    request_params     params;
    obs_put_properties put_properties;
    set_dis_policy_data_ex *data = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "set_bucket_dis_policy start !");

    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    if (dis_rule_count == 0 || dis_rule_count > OBS_MAX_DIS_POLICY_RULES)
    {
        COMMLOG(OBS_LOGERROR, "set_bucket_dis_policy failed, dis_rule_count(%d) is invalid.", dis_rule_count);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    data = init_dis_policy_data(dis_policy_rules, dis_rule_count, handler, callback_data);
    if (!data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc set_dis_policy_data failed.");
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
    params.subResource = "disPolicy";
    params.put_properties = &put_properties;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_dis_policy finish.");
}
