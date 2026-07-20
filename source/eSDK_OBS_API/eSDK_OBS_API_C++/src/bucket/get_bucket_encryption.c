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

#define MAX_ENCRYPTION_ALGORITHM_LEN 64
#define MAX_KMS_KEY_ID_LEN 256
#define MAX_PROJECT_ID_LEN 256
#define MAX_KMS_DATA_ENCRYPTION_LEN 64

typedef struct encryption_rule_data
{
    string_buffer(sse_algorithm, MAX_ENCRYPTION_ALGORITHM_LEN);
    string_buffer(kms_data_encryption, MAX_KMS_DATA_ENCRYPTION_LEN);
    string_buffer(kms_master_key_id, MAX_KMS_KEY_ID_LEN);
    string_buffer(project_id, MAX_PROJECT_ID_LEN);
} encryption_rule_data;

typedef struct get_encryption_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    obs_get_bucket_encryption_callback *get_encryption_callback;
    void *callback_data;

    encryption_rule_data *rule;
    obs_use_api use_api;
} get_encryption_data;

/**
 * 解析XML响应数据
 */
static obs_status parse_encryption_xml(get_encryption_data *geData,
    const char *element_path, const char *data, int data_len)
{
    int fit = 1;

    // 解析SSEAlgorithm
    if (!strcmp(element_path, "ServerSideEncryptionConfiguration/Rule/ApplyServerSideEncryptionByDefault/SSEAlgorithm"))
    {
        if (geData->rule)
        {
            string_buffer_append(geData->rule->sse_algorithm, data, data_len, fit);
        }
    }
    // 解析KMSDataEncryption
    else if (!strcmp(element_path, "ServerSideEncryptionConfiguration/Rule/ApplyServerSideEncryptionByDefault/KMSDataEncryption"))
    {
        if (geData->rule)
        {
            string_buffer_append(geData->rule->kms_data_encryption, data, data_len, fit);
        }
    }
    // 解析KMSMasterKeyID
    else if (!strcmp(element_path, "ServerSideEncryptionConfiguration/Rule/ApplyServerSideEncryptionByDefault/KMSMasterKeyID"))
    {
        if (geData->rule)
        {
            string_buffer_append(geData->rule->kms_master_key_id, data, data_len, fit);
        }
    }
    // 解析ProjectID
    else if (!strcmp(element_path, "ServerSideEncryptionConfiguration/Rule/ApplyServerSideEncryptionByDefault/ProjectID"))
    {
        if (geData->rule)
        {
            string_buffer_append(geData->rule->project_id, data, data_len, fit);
        }
    }

    (void)fit;
    return OBS_STATUS_OK;
}

/**
 * XML回调函数
 */
static obs_status get_encryption_xml_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    get_encryption_data *geData = (get_encryption_data *)callback_data;

    // 检测到Rule元素的子元素时，先创建rule（如果尚未创建）
    // 这样可以确保在解析SSEAlgorithm等子元素时rule已经存在
    if (data && !geData->rule)
    {
        // Check if we're parsing a child element of Rule
        if (strstr(element_path, "ServerSideEncryptionConfiguration/Rule/") == element_path)
        {
            encryption_rule_data *rule_data = (encryption_rule_data*)malloc(sizeof(encryption_rule_data));
            if (rule_data)
            {
                memset_s(rule_data, sizeof(encryption_rule_data), 0, sizeof(encryption_rule_data));
                geData->rule = rule_data;
            }
        }
    }

    if (data)
    {
        obs_status status = parse_encryption_xml(geData, element_path, data, data_len);
        return status;
    }

    return OBS_STATUS_OK;
}

/**
 * 初始化获取桶加密配置的数据结构
 */
static get_encryption_data* init_get_encryption_data(obs_get_bucket_encryption_handler *handler,
    void *callback_data)
{
    get_encryption_data *geData = NULL;

    geData = (get_encryption_data *)malloc(sizeof(get_encryption_data));
    if (!geData)
    {
        COMMLOG(OBS_LOGERROR, "malloc get_encryption_data failed.");
        return NULL;
    }
    memset_s(geData, sizeof(get_encryption_data), 0, sizeof(get_encryption_data));

    simplexml_initialize(&(geData->simpleXml), &get_encryption_xml_callback, geData);

    geData->response_properties_callback = handler->response_handler.properties_callback;
    geData->response_complete_callback = handler->response_handler.complete_callback;
    geData->get_encryption_callback = handler->get_bucket_encryption_callback;
    geData->callback_data = callback_data;

    return geData;
}

/**
 * 构建返回给用户的配置结构体
 */
static obs_status make_get_encryption_callback(get_encryption_data *geData)
{
    obs_status retStatus = OBS_STATUS_OK;

    // 创建配置结构体
    obs_server_side_encryption_configuration config;
    memset_s(&config, sizeof(obs_server_side_encryption_configuration), 0, sizeof(obs_server_side_encryption_configuration));

    // 分配并复制Rule
    if (geData->rule && geData->rule->sse_algorithm[0] != '\0')
    {
        config.rule = (obs_server_side_encryption_rule*)malloc(sizeof(obs_server_side_encryption_rule));
        if (config.rule)
        {
            memset_s(config.rule, sizeof(obs_server_side_encryption_rule), 0, sizeof(obs_server_side_encryption_rule));

            // 分配并复制SSEAlgorithm
            if (geData->rule->sse_algorithm[0] != '\0')
            {
                config.rule->sse_algorithm = strdup(geData->rule->sse_algorithm);
            }

            // 分配并复制KMSDataEncryption
            if (geData->rule->kms_data_encryption[0] != '\0')
            {
                config.rule->kms_data_encryption = strdup(geData->rule->kms_data_encryption);
            }

            // 分配并复制KMSMasterKeyID
            if (geData->rule->kms_master_key_id[0] != '\0')
            {
                config.rule->kms_master_key_id = strdup(geData->rule->kms_master_key_id);
            }

            // 分配并复制ProjectID
            if (geData->rule->project_id[0] != '\0')
            {
                config.rule->project_id = strdup(geData->rule->project_id);
            }
        }
    }

    // 调用用户回调
    retStatus = (*(geData->get_encryption_callback))(&config, geData->callback_data);

    // 释放分配的内存
    if (config.rule)
    {
        CHECK_NULL_FREE(config.rule->sse_algorithm);
        CHECK_NULL_FREE(config.rule->kms_data_encryption);
        CHECK_NULL_FREE(config.rule->kms_master_key_id);
        CHECK_NULL_FREE(config.rule->project_id);
        CHECK_NULL_FREE(config.rule);
    }

    return retStatus;
}

/**
 * 属性回调函数
 */
static obs_status get_encryption_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    get_encryption_data *geData = (get_encryption_data *)callback_data;
    if (geData->response_properties_callback)
    {
        return (*(geData->response_properties_callback))(response_properties,
            geData->callback_data);
    }
    return OBS_STATUS_OK;
}

/**
 * 数据回调函数
 */
static obs_status get_encryption_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    get_encryption_data *geData = (get_encryption_data *)callback_data;

    COMMLOG(OBS_LOGINFO, "[DATA_CB] buffer_size=%d", buffer_size);

    return simplexml_add(&(geData->simpleXml), buffer, buffer_size);
}

/**
 * 完成回调函数
 */
static void get_encryption_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_encryption_data *geData = (get_encryption_data *)callback_data;

    if (geData->rule)
    {
        COMMLOG(OBS_LOGINFO, "[COMPLETE_CB] sse_algorithm=[%s]", geData->rule->sse_algorithm);
    }

    // 如果请求成功且有规则数据，则调用用户回调
    if (geData->rule && geData->rule->sse_algorithm[0] != '\0' && OBS_STATUS_OK == request_status)
    {
        COMMLOG(OBS_LOGINFO, "[COMPLETE_CB] Calling make_get_encryption_callback");
        request_status = make_get_encryption_callback(geData);
    }
    else
    {
        COMMLOG(OBS_LOGINFO, "[COMPLETE_CB] NOT calling make_get_encryption_callback: rule=%s, status=%d",
                (geData->rule && geData->rule->sse_algorithm[0]) ? "exists" : "null", request_status);
    }

    // 调用完整回调
    (*(geData->response_complete_callback))(request_status, obs_error_info,
        geData->callback_data);

    // 释放数据
    CHECK_NULL_FREE(geData->rule);

    simplexml_deinitialize(&(geData->simpleXml));

    free(geData);
    geData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

/**
 * 获取桶加密配置
 */
void get_bucket_encryption(const obs_options *options,
    obs_get_bucket_encryption_handler *handler, void *callback_data)
{
    request_params params;
    get_encryption_data *geData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get_bucket_encryption start !");

    // 参数校验
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    // 初始化数据结构
    geData = init_get_encryption_data(handler, callback_data);
    if (NULL == geData)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        return;
    }
    geData->use_api = use_api;

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
    params.properties_callback = &get_encryption_properties_callback;
    params.fromObsCallback = &get_encryption_data_callback;
    params.complete_callback = &get_encryption_complete_callback;
    params.callback_data = geData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "encryption";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;

    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_encryption finish.");
}
