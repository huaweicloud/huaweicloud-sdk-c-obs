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
#include <openssl/md5.h>

#define MAX_REPLICATION_RULES 100

typedef struct set_replication_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    void *callback_data;

    char doc[MAX_XML_LEN];
    int doc_len;
    int doc_bytes_written;
    char doc_md5[64];
} set_replication_data;

/**
 * 构建Filter过滤条件的XML
 */
static void build_replication_filter_xml(set_replication_data *srData, obs_replication_filter *filter)
{
    unsigned int j;
    int need_and = (filter->tag_count + filter->not_tag_count > 1) ? 1 : 0;

    (void)add_xml_element(srData->doc, &srData->doc_len, "Filter",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    if (need_and)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "And",
            NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);
    }

    // 添加正向标签
    for (j = 0; j < filter->tag_count; ++j)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "Tag",
            NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);
        if (filter->tags[j].key)
        {
            (void)add_xml_element(srData->doc, &srData->doc_len, "Key",
                filter->tags[j].key, NEED_FORMALIZE, ADD_NAME_CONTENT);
        }
        if (filter->tags[j].value)
        {
            (void)add_xml_element(srData->doc, &srData->doc_len, "Value",
                filter->tags[j].value, NEED_FORMALIZE, ADD_NAME_CONTENT);
        }
        (void)add_xml_element(srData->doc, &srData->doc_len, "Tag",
            NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
    }

    // 添加Not条件标签
    for (j = 0; j < filter->not_tag_count; ++j)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "Not",
            NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);
        (void)add_xml_element(srData->doc, &srData->doc_len, "Tag",
            NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);
        if (filter->not_tags[j].key)
        {
            (void)add_xml_element(srData->doc, &srData->doc_len, "Key",
                filter->not_tags[j].key, NEED_FORMALIZE, ADD_NAME_CONTENT);
        }
        if (filter->not_tags[j].value)
        {
            (void)add_xml_element(srData->doc, &srData->doc_len, "Value",
                filter->not_tags[j].value, NEED_FORMALIZE, ADD_NAME_CONTENT);
        }
        (void)add_xml_element(srData->doc, &srData->doc_len, "Tag",
            NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
        (void)add_xml_element(srData->doc, &srData->doc_len, "Not",
            NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
    }

    if (need_and)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "And",
            NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
    }

    (void)add_xml_element(srData->doc, &srData->doc_len, "Filter",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
}

/**
 * 构建Destination的XML
 */
static void build_replication_destination_xml(set_replication_data *srData, obs_replication_rule *rule)
{
    (void)add_xml_element(srData->doc, &srData->doc_len, "Destination",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    if (rule->destination_bucket)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "Bucket", rule->destination_bucket,
            NEED_FORMALIZE, ADD_NAME_CONTENT);
    }
    if (rule->storage_class)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "StorageClass", rule->storage_class,
            NEED_FORMALIZE, ADD_NAME_CONTENT);
    }
    if (rule->delete_data)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "DeleteData", rule->delete_data,
            NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    (void)add_xml_element(srData->doc, &srData->doc_len, "Destination",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
}

/**
 * 构建单条复制规则的XML
 */
static void build_replication_rule_xml(set_replication_data *srData, obs_replication_rule *rule)
{
    (void)add_xml_element(srData->doc, &srData->doc_len, "Rule",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    if (rule->id)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "ID", rule->id,
            NEED_FORMALIZE, ADD_NAME_CONTENT);
    }
    if (rule->status)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "Status", rule->status,
            NEED_FORMALIZE, ADD_NAME_CONTENT);
    }
    if (rule->prefix)
    {
        int tmplen = snprintf_s(srData->doc + srData->doc_len,
            MAX_XML_LEN - srData->doc_len, _TRUNCATE,
            "<Prefix>%s</Prefix>", rule->prefix);
        if (tmplen > 0)
        {
            srData->doc_len += tmplen;
        }
    }

    // 添加Filter
    if (rule->filter.not_tag_count > 0 || rule->filter.tag_count > 0)
    {
        build_replication_filter_xml(srData, &rule->filter);
    }

    // 添加Destination
    build_replication_destination_xml(srData, rule);

    if (rule->historical_object_replication)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "HistoricalObjectReplication",
            rule->historical_object_replication,
            NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    (void)add_xml_element(srData->doc, &srData->doc_len, "Rule",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
}

/**
 * 构建跨区域复制配置的XML请求体
 */
static obs_status build_replication_xml(set_replication_data *srData,
    obs_replication_configuration *replication_config)
{
    unsigned int i = 0;

    // 开始 ReplicationConfiguration 元素
    (void)add_xml_element(srData->doc, &srData->doc_len, "ReplicationConfiguration",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    // 添加Agency（委托名称）
    if (replication_config->agency)
    {
        (void)add_xml_element(srData->doc, &srData->doc_len, "Agency", replication_config->agency,
            NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    // 添加每个复制规则
    for (i = 0; i < replication_config->rule_count; ++i)
    {
        build_replication_rule_xml(srData, &replication_config->rules[i]);
    }

    // 结束 ReplicationConfiguration 元素
    (void)add_xml_element(srData->doc, &srData->doc_len, "ReplicationConfiguration",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);

    return OBS_STATUS_OK;
}

/**
 * 数据回调函数
 */
static int set_replication_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    set_replication_data *srData = (set_replication_data *)callback_data;

    if (!srData->doc_len)
    {
        return 0;
    }

    int remaining = (srData->doc_len - srData->doc_bytes_written);
    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy)
    {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(srData->doc[srData->doc_bytes_written]), toCopy);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "set_replication_data_callback: memcpy_s failed!\n");
        return 0;
    }

    srData->doc_bytes_written += toCopy;
    return toCopy;
}

/**
 * 属性回调函数
 */
static obs_status set_replication_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_replication_data *srData = (set_replication_data *)callback_data;
    if (srData->response_properties_callback)
    {
        return (*(srData->response_properties_callback))(response_properties,
            srData->callback_data);
    }
    return OBS_STATUS_OK;
}

/**
 * 完成回调函数
 */
static void set_replication_complete_callback(obs_status requestStatus,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_replication_data *srData = (set_replication_data *)callback_data;

    (void)(*(srData->response_complete_callback))(requestStatus, obs_error_info, srData->callback_data);

    free(srData);
    srData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

/**
 * 初始化设置跨区域复制配置的数据结构
 */
static set_replication_data* init_set_replication_data(obs_replication_configuration *replication_config,
    obs_response_handler *handler, void *callback_data)
{
    unsigned char doc_md5[16];
    set_replication_data *srData = NULL;

    srData = (set_replication_data *)malloc(sizeof(set_replication_data));
    if (!srData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc set_replication_data failed.");
        return NULL;
    }
    memset_s(srData, sizeof(set_replication_data), 0, sizeof(set_replication_data));

    srData->response_complete_callback = handler->complete_callback;
    srData->response_properties_callback = handler->properties_callback;
    srData->callback_data = callback_data;
    srData->doc_len = 0;

    obs_status ret_status = build_replication_xml(srData, replication_config);
    if (OBS_STATUS_OK != ret_status || srData->doc_len < 0)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        free(srData);
        srData = NULL;
        return NULL;
    }

    COMMLOG(OBS_LOGINFO, "set_replication doc: %s.", srData->doc);
    srData->doc_bytes_written = 0;
    /* MD5 is required by OBS API protocol for Content-MD5 header, not used for security purposes */
    MD5((unsigned char *)srData->doc, (size_t)srData->doc_len, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), srData->doc_md5);

    return srData;
}

/**
 * 设置桶跨区域复制配置
 */
void set_bucket_replication(const obs_options *options,
    obs_replication_configuration *replication_config,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_put_properties put_properties;
    set_replication_data *srData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set_bucket_replication start !");

    // 参数校验
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    if (NULL == replication_config || 0 == replication_config->rule_count)
    {
        COMMLOG(OBS_LOGERROR, "replication_config or rule_count(%d) is invalid.",
            replication_config ? replication_config->rule_count : 0);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    if (replication_config->rule_count > MAX_REPLICATION_RULES)
    {
        COMMLOG(OBS_LOGERROR, "rule_count(%d) exceeds maximum(%d).",
            replication_config->rule_count, MAX_REPLICATION_RULES);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    // 初始化数据结构
    srData = init_set_replication_data(replication_config, handler, callback_data);
    if (!srData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc set_replication_data failed.");
        return;
    }

    // 初始化请求参数
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    memset_s(&put_properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    put_properties.md5 = srData->doc_md5;
    put_properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &set_replication_properties_callback;
    params.toObsCallback = &set_replication_data_callback;
    params.complete_callback = &set_replication_complete_callback;
    params.toObsCallbackTotalSize = srData->doc_len;
    params.callback_data = srData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "replication";
    params.put_properties = &put_properties;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;

    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_replication finish.");
}