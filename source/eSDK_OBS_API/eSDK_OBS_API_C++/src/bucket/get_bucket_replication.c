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
#include "string_buffer.h"
#include <openssl/md5.h>

#define MAX_REPLICATION_RULES 100
#define MAX_REPLICATION_TAGS 10

typedef struct replication_tag_data
{
    string_buffer(key, 256);
    string_buffer(value, 256);
} replication_tag_data;

typedef struct replication_rule_data
{
    string_buffer(id, 256);
    string_buffer(destination_bucket, 256);
    string_buffer(storage_class, 64);
    string_buffer(prefix, 1024);
    string_buffer(status, 64);
    string_buffer(delete_data, 64);
    string_buffer(historical_object_replication, 64);
    // Filter相关
    unsigned int tag_count;
    replication_tag_data tags[MAX_REPLICATION_TAGS];
    unsigned int not_tag_count;
    replication_tag_data not_tags[MAX_REPLICATION_TAGS];
    // XML解析状态
    int completed;  // Rule元素是否已结束
} replication_rule_data;

typedef struct get_replication_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    obs_get_bucket_replication_callback *get_replication_callback;
    void *callback_data;

    string_buffer(agency, 256);
    replication_rule_data *rules_data[MAX_REPLICATION_RULES];
    unsigned int rules_count;
    obs_use_api use_api;
} get_replication_data;

/**
 * 确保当前rule_data存在（懒分配）
 * 当遇到Rule的子元素但还没有rule_data，或当前rule_data已完成（多规则场景），
 * 自动创建一条新的rule_data
 */
static int ensure_replication_rule(get_replication_data *grData)
{
    int ruleIndex = (int)(grData->rules_count) - 1;
    if (ruleIndex >= 0 && !grData->rules_data[ruleIndex]->completed) {
        return ruleIndex;
    }
    if (grData->rules_count >= MAX_REPLICATION_RULES) {
        return -1;
    }
    replication_rule_data *rule_data = (replication_rule_data*)malloc(sizeof(replication_rule_data));
    if (!rule_data) {
        COMMLOG(OBS_LOGERROR, "malloc replication_rule_data failed !");
        return -1;
    }
    memset_s(rule_data, sizeof(replication_rule_data), 0, sizeof(replication_rule_data));
    grData->rules_data[grData->rules_count] = rule_data;
    grData->rules_count++;
    return (int)(grData->rules_count) - 1;
}

#define REPL_PREFIX "ReplicationConfiguration/Rule/"

static void parse_replication_simple_fields(replication_rule_data *rule,
    const char *suffix, const char *data, int data_len, int fit)
{
    if (!strcmp(suffix, "ID"))
    {
        string_buffer_append(rule->id, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Prefix"))
    {
        string_buffer_append(rule->prefix, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Status"))
    {
        string_buffer_append(rule->status, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Destination/Bucket"))
    {
        string_buffer_append(rule->destination_bucket, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Destination/StorageClass"))
    {
        string_buffer_append(rule->storage_class, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Destination/DeleteData"))
    {
        string_buffer_append(rule->delete_data, data, data_len, fit);
    }
    else if (!strcmp(suffix, "HistoricalObjectReplication"))
    {
        string_buffer_append(rule->historical_object_replication, data, data_len, fit);
    }
}

static void parse_replication_tag_key(replication_tag_data *tags,
    unsigned int tag_count, const char *data, int data_len, int fit)
{
    if (tag_count < MAX_REPLICATION_TAGS)
    {
        string_buffer_append(tags[tag_count].key, data, data_len, fit);
    }
}

static void parse_replication_tag_value(replication_tag_data *tags,
    unsigned int tag_count, const char *data, int data_len, int fit)
{
    if (tag_count < MAX_REPLICATION_TAGS)
    {
        string_buffer_append(tags[tag_count].value, data, data_len, fit);
    }
}

static void parse_replication_filter_fields(replication_rule_data *rule,
    const char *suffix, const char *data, int data_len, int fit)
{
    if (!strcmp(suffix, "Filter/Not/Tag/Key"))
    {
        parse_replication_tag_key(rule->not_tags, rule->not_tag_count, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Filter/Not/Tag/Value"))
    {
        parse_replication_tag_value(rule->not_tags, rule->not_tag_count, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Filter/Tag/Key"))
    {
        parse_replication_tag_key(rule->tags, rule->tag_count, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Filter/Tag/Value"))
    {
        parse_replication_tag_value(rule->tags, rule->tag_count, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Filter/And/Tag/Key"))
    {
        parse_replication_tag_key(rule->tags, rule->tag_count, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Filter/And/Tag/Value"))
    {
        parse_replication_tag_value(rule->tags, rule->tag_count, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Filter/And/Not/Tag/Key"))
    {
        parse_replication_tag_key(rule->not_tags, rule->not_tag_count, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Filter/And/Not/Tag/Value"))
    {
        parse_replication_tag_value(rule->not_tags, rule->not_tag_count, data, data_len, fit);
    }
}

/**
 * 解析Rule子元素字段（拆分为simple fields和filter fields）
 */
static void parse_replication_rule_field(replication_rule_data *rule,
    const char *element_path, const char *data, int data_len, int fit)
{
    if (strncmp(element_path, REPL_PREFIX, sizeof(REPL_PREFIX) - 1) != 0)
    {
        return;
    }
    const char *suffix = element_path + sizeof(REPL_PREFIX) - 1;
    parse_replication_simple_fields(rule, suffix, data, data_len, fit);
    parse_replication_filter_fields(rule, suffix, data, data_len, fit);
}

/**
 * 解析XML响应数据
 */
static obs_status parse_replication_xml(get_replication_data *grData,
    const char *element_path, const char *data, int data_len)
{
    int fit = 1;

    if (!strncmp(element_path, "ReplicationConfiguration/Rule/", 30))
    {
        int ruleIndex = ensure_replication_rule(grData);
        if (ruleIndex >= 0 && ruleIndex < MAX_REPLICATION_RULES)
        {
            parse_replication_rule_field(grData->rules_data[ruleIndex],
                element_path, data, data_len, fit);
        }
    }
    else if (!strcmp(element_path, "ReplicationConfiguration/Agency"))
    {
        string_buffer_append(grData->agency, data, data_len, fit);
    }

    (void)fit;
    return OBS_STATUS_OK;
}

/**
 * 处理元素结束事件（更新标签计数等）
 */
static void handle_replication_end_element(replication_rule_data *rule, const char *element_path)
{
    if (!strcmp(element_path, "ReplicationConfiguration/Rule"))
    {
        rule->completed = 1;
    }
    else if (!strcmp(element_path, "ReplicationConfiguration/Rule/Filter/Not/Tag"))
    {
        if (rule->not_tag_count < MAX_REPLICATION_TAGS) { rule->not_tag_count++; }
    }
    else if (!strcmp(element_path, "ReplicationConfiguration/Rule/Filter/Tag"))
    {
        if (rule->tag_count < MAX_REPLICATION_TAGS) { rule->tag_count++; }
    }
    else if (!strcmp(element_path, "ReplicationConfiguration/Rule/Filter/And/Tag"))
    {
        if (rule->tag_count < MAX_REPLICATION_TAGS) { rule->tag_count++; }
    }
    else if (!strcmp(element_path, "ReplicationConfiguration/Rule/Filter/And/Not/Tag"))
    {
        if (rule->not_tag_count < MAX_REPLICATION_TAGS) { rule->not_tag_count++; }
    }
}

/**
 * XML回调函数
 * 注意：simplexml只在saxCharacters(data!=NULL)和saxEndElement(data==NULL)时
 * 调用此回调，saxStartElement不调用回调。
 * 因此data==NULL表示元素结束，data!=NULL表示元素文本内容。
 */
static obs_status get_replication_xml_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    get_replication_data *grData = (get_replication_data *)callback_data;

    if (data)
    {
        return parse_replication_xml(grData, element_path, data, data_len);
    }

    // data==NULL: 元素结束事件
    int ruleIndex = (int)(grData->rules_count) - 1;
    if (ruleIndex >= 0 && ruleIndex < MAX_REPLICATION_RULES)
    {
        handle_replication_end_element(grData->rules_data[ruleIndex], element_path);
    }

    return OBS_STATUS_OK;
}

/**
 * 初始化获取跨区域复制配置的数据结构
 */
static get_replication_data* init_get_replication_data(obs_get_bucket_replication_handler *handler,
    void *callback_data)
{
    get_replication_data *grData = NULL;

    grData = (get_replication_data *)malloc(sizeof(get_replication_data));
    if (!grData)
    {
        COMMLOG(OBS_LOGERROR, "malloc get_replication_data failed.");
        return NULL;
    }
    memset_s(grData, sizeof(get_replication_data), 0, sizeof(get_replication_data));

    simplexml_initialize(&(grData->simpleXml), &get_replication_xml_callback, grData);

    grData->response_properties_callback = handler->response_handler.properties_callback;
    grData->response_complete_callback = handler->response_handler.complete_callback;
    grData->get_replication_callback = handler->get_bucket_replication_callback;
    grData->callback_data = callback_data;

    return grData;
}

/**
 * 填充标签数组（从解析数据到输出结构体）
 */
static void fill_replication_tags(obs_replication_tag *out_tags,
    replication_tag_data *in_tags, unsigned int tag_count)
{
    unsigned int j;
    for (j = 0; j < tag_count; ++j)
    {
        if (in_tags[j].key[0] != '\0')
        {
            out_tags[j].key = strdup(in_tags[j].key);
        }
        if (in_tags[j].value[0] != '\0')
        {
            out_tags[j].value = strdup(in_tags[j].value);
        }
    }
}

/**
 * 填充单条复制规则数据
 */
static void fill_replication_rule(obs_replication_rule *out_rule, replication_rule_data *in_rule)
{
    if (in_rule->id[0] != '\0') { out_rule->id = strdup(in_rule->id); }
    if (in_rule->prefix[0] != '\0') { out_rule->prefix = strdup(in_rule->prefix); }
    if (in_rule->status[0] != '\0') { out_rule->status = strdup(in_rule->status); }
    if (in_rule->destination_bucket[0] != '\0') { out_rule->destination_bucket = strdup(in_rule->destination_bucket); }
    if (in_rule->storage_class[0] != '\0') { out_rule->storage_class = strdup(in_rule->storage_class); }
    if (in_rule->delete_data[0] != '\0') { out_rule->delete_data = strdup(in_rule->delete_data); }
    if (in_rule->historical_object_replication[0] != '\0') { out_rule->historical_object_replication = strdup(in_rule->historical_object_replication); }

    // 填充Filter标签数据
    if (in_rule->tag_count > 0)
    {
        out_rule->filter.tag_count = in_rule->tag_count;
        out_rule->filter.tags = (obs_replication_tag*)malloc(sizeof(obs_replication_tag) * in_rule->tag_count);
        if (out_rule->filter.tags)
        {
            memset_s(out_rule->filter.tags, sizeof(obs_replication_tag) * in_rule->tag_count, 0,
                sizeof(obs_replication_tag) * in_rule->tag_count);
            fill_replication_tags(out_rule->filter.tags, in_rule->tags, in_rule->tag_count);
        }
    }

    if (in_rule->not_tag_count > 0)
    {
        out_rule->filter.not_tag_count = in_rule->not_tag_count;
        out_rule->filter.not_tags = (obs_replication_tag*)malloc(sizeof(obs_replication_tag) * in_rule->not_tag_count);
        if (out_rule->filter.not_tags)
        {
            memset_s(out_rule->filter.not_tags, sizeof(obs_replication_tag) * in_rule->not_tag_count, 0,
                sizeof(obs_replication_tag) * in_rule->not_tag_count);
            fill_replication_tags(out_rule->filter.not_tags, in_rule->not_tags, in_rule->not_tag_count);
        }
    }
}

/**
 * 释放规则数组的内存
 */
static void free_replication_rules(obs_replication_rule *rules, unsigned int count)
{
    unsigned int i, j;
    for (i = 0; i < count; ++i)
    {
        CHECK_NULL_FREE(rules[i].id);
        CHECK_NULL_FREE(rules[i].prefix);
        CHECK_NULL_FREE(rules[i].status);
        CHECK_NULL_FREE(rules[i].destination_bucket);
        CHECK_NULL_FREE(rules[i].storage_class);
        CHECK_NULL_FREE(rules[i].delete_data);
        CHECK_NULL_FREE(rules[i].historical_object_replication);
        if (rules[i].filter.tags)
        {
            for (j = 0; j < rules[i].filter.tag_count; ++j)
            {
                CHECK_NULL_FREE(rules[i].filter.tags[j].key);
                CHECK_NULL_FREE(rules[i].filter.tags[j].value);
            }
            CHECK_NULL_FREE(rules[i].filter.tags);
        }
        if (rules[i].filter.not_tags)
        {
            for (j = 0; j < rules[i].filter.not_tag_count; ++j)
            {
                CHECK_NULL_FREE(rules[i].filter.not_tags[j].key);
                CHECK_NULL_FREE(rules[i].filter.not_tags[j].value);
            }
            CHECK_NULL_FREE(rules[i].filter.not_tags);
        }
    }
}

/**
 * 构建返回给用户的配置结构体
 */
static obs_status make_get_replication_callback(get_replication_data *grData)
{
    obs_status retStatus = OBS_STATUS_OK;
    unsigned int i = 0;

    if (grData->rules_count == 0)
    {
        COMMLOG(OBS_LOGERROR, "No replication rules found.");
        return OBS_STATUS_NoSuchReplicationConfiguration;
    }

    // 分配规则数组
    obs_replication_rule *rules = (obs_replication_rule*)malloc(
        sizeof(obs_replication_rule) * grData->rules_count);
    if (NULL == rules)
    {
        COMMLOG(OBS_LOGERROR, "malloc obs_replication_rule failed.");
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(rules, sizeof(obs_replication_rule) * grData->rules_count, 0,
        sizeof(obs_replication_rule) * grData->rules_count);

    // 填充规则数据
    for (i = 0; i < grData->rules_count; ++i)
    {
        fill_replication_rule(&rules[i], grData->rules_data[i]);
    }

    // 创建配置结构体
    obs_replication_configuration config;
    config.agency = NULL;
    if (grData->agency[0] != '\0')
    {
        config.agency = strdup(grData->agency);
    }
    config.rules = rules;
    config.rule_count = grData->rules_count;

    // 调用用户回调
    retStatus = (*(grData->get_replication_callback))(&config, grData->rules_count,
        grData->callback_data);

    // 释放分配的内存
    free_replication_rules(rules, grData->rules_count);
    CHECK_NULL_FREE(rules);
    CHECK_NULL_FREE(config.agency);

    return retStatus;
}

/**
 * 属性回调函数
 */
static obs_status get_replication_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    get_replication_data *grData = (get_replication_data *)callback_data;
    if (grData->response_properties_callback)
    {
        return (*(grData->response_properties_callback))(response_properties,
            grData->callback_data);
    }
    return OBS_STATUS_OK;
}

/**
 * 数据回调函数
 */
static obs_status get_replication_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    get_replication_data *grData = (get_replication_data *)callback_data;

    return simplexml_add(&(grData->simpleXml), buffer, buffer_size);
}

/**
 * 完成回调函数
 */
static void get_replication_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info, void *callback_data)
{
    unsigned int i = 0;
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_replication_data *grData = (get_replication_data *)callback_data;

    // 如果请求成功且有数据，则调用用户回调
    if (grData->rules_count > 0 && OBS_STATUS_OK == request_status)
    {
        request_status = make_get_replication_callback(grData);
    }

    // 调用完整回调
    (*(grData->response_complete_callback))(request_status, obs_error_info,
        grData->callback_data);

    // 释放规则数据
    for (i = 0; i < grData->rules_count; i++)
    {
        CHECK_NULL_FREE(grData->rules_data[i]);
    }

    simplexml_deinitialize(&(grData->simpleXml));

    free(grData);
    grData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

/**
 * 获取桶跨区域复制配置
 */
void get_bucket_replication(const obs_options *options,
    obs_get_bucket_replication_handler *handler, void *callback_data)
{
    request_params params;
    get_replication_data *grData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get_bucket_replication start !");

    // 参数校验
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    // 初始化数据结构
    grData = init_get_replication_data(handler, callback_data);
    if (NULL == grData)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        return;
    }
    grData->use_api = use_api;

    // 初始化请求参数
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_replication_properties_callback;
    params.fromObsCallback = &get_replication_data_callback;
    params.complete_callback = &get_replication_complete_callback;
    params.callback_data = grData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "replication";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;

    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_replication finish.");
}