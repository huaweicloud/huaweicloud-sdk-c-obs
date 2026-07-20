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

typedef struct set_encryption_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    void *callback_data;

    char doc[MAX_XML_LEN];
    int doc_len;
    int doc_bytes_written;
    char doc_md5[64];
} set_encryption_data;

/**
 * 构建SSE-KMS特有的可选字段（KMSDataEncryption、KMSMasterKeyID、ProjectID）
 */
static void build_kms_encryption_fields(set_encryption_data *seData,
    obs_server_side_encryption_configuration *encryption_config,
    obs_use_api use_api)
{
    // 添加数据加密算法（可选，仅SSE-KMS）
    if (encryption_config->rule->kms_data_encryption)
    {
        (void)add_xml_element(seData->doc, &seData->doc_len, "KMSDataEncryption",
            encryption_config->rule->kms_data_encryption, NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    // 添加KMS主密钥ID（可选，仅SSE-KMS）
    if (encryption_config->rule->kms_master_key_id)
    {
        (void)add_xml_element(seData->doc, &seData->doc_len, "KMSMasterKeyID",
            encryption_config->rule->kms_master_key_id, NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    // 添加项目ID（可选，仅SSE-KMS，仅S3模式）
    // 华为云OBS不支持ProjectID元素
    if (use_api == OBS_USE_API_S3 && encryption_config->rule->project_id)
    {
        (void)add_xml_element(seData->doc, &seData->doc_len, "ProjectID",
            encryption_config->rule->project_id, NEED_FORMALIZE, ADD_NAME_CONTENT);
    }
}

/**
 * 构建桶加密配置的XML请求体
 */
static obs_status build_encryption_xml(set_encryption_data *seData,
    obs_server_side_encryption_configuration *encryption_config,
    obs_use_api use_api)
{
    // 开始 ServerSideEncryptionConfiguration 元素
    (void)add_xml_element(seData->doc, &seData->doc_len, "ServerSideEncryptionConfiguration",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    // 添加 Rule 元素
    (void)add_xml_element(seData->doc, &seData->doc_len, "Rule",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    // 添加 ApplyServerSideEncryptionByDefault 元素
    (void)add_xml_element(seData->doc, &seData->doc_len, "ApplyServerSideEncryptionByDefault",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    // 添加加密算法（必填）
    if (encryption_config->rule && encryption_config->rule->sse_algorithm)
    {
        (void)add_xml_element(seData->doc, &seData->doc_len, "SSEAlgorithm",
            encryption_config->rule->sse_algorithm, NEED_FORMALIZE, ADD_NAME_CONTENT);

        // 如果是SSE-KMS方式，添加可选参数
        if (strcmp(encryption_config->rule->sse_algorithm, "kms") == 0)
        {
            build_kms_encryption_fields(seData, encryption_config, use_api);
        }
    }

    // 结束 ApplyServerSideEncryptionByDefault 元素
    (void)add_xml_element(seData->doc, &seData->doc_len, "ApplyServerSideEncryptionByDefault",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);

    // 添加 BucketKeyEnabled 元素（华为云OBS需要）
    // 在华为云模式下，如果使用SSE-KMS，需要添加BucketKeyEnabled
    if (encryption_config->rule && encryption_config->rule->sse_algorithm &&
        strcmp(encryption_config->rule->sse_algorithm, "kms") == 0)
    {
        (void)add_xml_element(seData->doc, &seData->doc_len, "BucketKeyEnabled",
            "true", NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    // 结束 Rule 元素
    (void)add_xml_element(seData->doc, &seData->doc_len, "Rule",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);

    // 结束 ServerSideEncryptionConfiguration 元素
    (void)add_xml_element(seData->doc, &seData->doc_len, "ServerSideEncryptionConfiguration",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);

    return OBS_STATUS_OK;
}

/**
 * 数据回调函数
 */
static int set_encryption_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    set_encryption_data *seData = (set_encryption_data *)callback_data;

    if (!seData->doc_len)
    {
        return 0;
    }

    int remaining = (seData->doc_len - seData->doc_bytes_written);
    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy)
    {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(seData->doc[seData->doc_bytes_written]), toCopy);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "set_encryption_data_callback: memcpy_s failed!\n");
        return 0;
    }

    seData->doc_bytes_written += toCopy;
    return toCopy;
}

/**
 * 属性回调函数
 */
static obs_status set_encryption_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_encryption_data *seData = (set_encryption_data *)callback_data;
    if (seData->response_properties_callback)
    {
        return (*(seData->response_properties_callback))(response_properties,
            seData->callback_data);
    }
    return OBS_STATUS_OK;
}

/**
 * 完成回调函数
 */
static void set_encryption_complete_callback(obs_status requestStatus,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_encryption_data *seData = (set_encryption_data *)callback_data;

    (void)(*(seData->response_complete_callback))(requestStatus, obs_error_info, seData->callback_data);

    free(seData);
    seData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

/**
 * 初始化设置桶加密配置的数据结构
 */
static set_encryption_data* init_set_encryption_data(obs_server_side_encryption_configuration *encryption_config,
    obs_response_handler *handler, void *callback_data, obs_use_api use_api)
{
    unsigned char doc_md5[16];
    set_encryption_data *seData = NULL;

    seData = (set_encryption_data *)malloc(sizeof(set_encryption_data));
    if (!seData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc set_encryption_data failed.");
        return NULL;
    }
    memset_s(seData, sizeof(set_encryption_data), 0, sizeof(set_encryption_data));

    seData->response_complete_callback = handler->complete_callback;
    seData->response_properties_callback = handler->properties_callback;
    seData->callback_data = callback_data;
    seData->doc_len = 0;

    obs_status ret_status = build_encryption_xml(seData, encryption_config, use_api);
    if (OBS_STATUS_OK != ret_status || seData->doc_len < 0)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        free(seData);
        seData = NULL;
        return NULL;
    }

    COMMLOG(OBS_LOGERROR, "set_encryption doc: %s.", seData->doc);
    seData->doc_bytes_written = 0;
    /* MD5 is required by OBS API protocol for Content-MD5 header, not used for security purposes */
    MD5((unsigned char *)seData->doc, (size_t)seData->doc_len, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), seData->doc_md5);

    return seData;
}

/**
 * 校验桶加密配置参数
 * @return 1 表示校验通过，0 表示校验失败（已回调错误）
 */
static int validate_encryption_config(const obs_options *options,
    obs_server_side_encryption_configuration *encryption_config,
    obs_response_handler *handler, void *callback_data)
{
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return 0;
    }

    if (NULL == encryption_config)
    {
        COMMLOG(OBS_LOGERROR, "encryption_config is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return 0;
    }

    if (NULL == encryption_config->rule)
    {
        COMMLOG(OBS_LOGERROR, "encryption_config.rule is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return 0;
    }

    if (!encryption_config->rule->sse_algorithm || strlen(encryption_config->rule->sse_algorithm) == 0)
    {
        COMMLOG(OBS_LOGERROR, "sse_algorithm is NULL or empty.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return 0;
    }

    // 校验加密算法值
    if (strcmp(encryption_config->rule->sse_algorithm, "kms") != 0 &&
        strcmp(encryption_config->rule->sse_algorithm, "AES256") != 0)
    {
        COMMLOG(OBS_LOGERROR, "Invalid sse_algorithm: %s. Must be 'kms' or 'AES256'.",
            encryption_config->rule->sse_algorithm);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return 0;
    }

    // 如果是SSE-KMS，校验可选的 KMSDataEncryption 值
    if (strcmp(encryption_config->rule->sse_algorithm, "kms") == 0 &&
        encryption_config->rule->kms_data_encryption != NULL)
    {
        if (strcmp(encryption_config->rule->kms_data_encryption, "SM4") != 0)
        {
            COMMLOG(OBS_LOGERROR, "Invalid kms_data_encryption: %s. Must be 'SM4' or NULL.",
                encryption_config->rule->kms_data_encryption);
            (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
            return 0;
        }
    }

    return 1;
}

/**
 * 设置桶加密配置
 */
void set_bucket_encryption(const obs_options *options,
    obs_server_side_encryption_configuration *encryption_config,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_put_properties put_properties;
    set_encryption_data *seData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set_bucket_encryption start !");

    // 参数校验
    if (!validate_encryption_config(options, encryption_config, handler, callback_data))
    {
        return;
    }

    // 初始化数据结构
    seData = init_set_encryption_data(encryption_config, handler, callback_data, use_api);
    if (!seData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc set_encryption_data failed.");
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
    put_properties.md5 = seData->doc_md5;
    put_properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &set_encryption_properties_callback;
    params.toObsCallback = &set_encryption_data_callback;
    params.complete_callback = &set_encryption_complete_callback;
    params.toObsCallbackTotalSize = seData->doc_len;
    params.callback_data = seData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "encryption";
    params.put_properties = &put_properties;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;

    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_encryption finish.");
}
