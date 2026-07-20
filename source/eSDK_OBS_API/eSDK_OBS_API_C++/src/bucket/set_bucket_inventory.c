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

#define MAX_INVENTORY_ID_LEN 256

typedef struct set_inventory_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    void *callback_data;

    char doc[MAX_XML_LEN];
    int doc_len;
    int doc_bytes_written;
    char doc_md5[64];
} set_inventory_data;

/**
 * 构建Filter的XML
 */
static void build_inventory_filter_xml(set_inventory_data *siData,
    obs_inventory_filter *filter)
{
    (void)add_xml_element(siData->doc, &siData->doc_len, "Filter",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);
    (void)add_xml_element(siData->doc, &siData->doc_len, "Prefix",
        filter->prefix, NEED_FORMALIZE, ADD_NAME_CONTENT);
    (void)add_xml_element(siData->doc, &siData->doc_len, "Filter",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
}

/**
 * 构建Destination的XML
 */
static void build_inventory_destination_xml(set_inventory_data *siData,
    obs_inventory_configuration *inventory_config)
{
    (void)add_xml_element(siData->doc, &siData->doc_len, "Destination",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    // 添加输出格式（在Destination内部）
    if (inventory_config->format)
    {
        (void)add_xml_element(siData->doc, &siData->doc_len, "Format",
            inventory_config->format, NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    if (inventory_config->destination->bucket)
    {
        (void)add_xml_element(siData->doc, &siData->doc_len, "Bucket",
            inventory_config->destination->bucket, NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    if (inventory_config->destination->prefix)
    {
        (void)add_xml_element(siData->doc, &siData->doc_len, "Prefix",
            inventory_config->destination->prefix, NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    (void)add_xml_element(siData->doc, &siData->doc_len, "Destination",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
}

static void build_optional_fields_xml(set_inventory_data *siData,
    obs_inventory_optional_fields *optional_fields)
{
    unsigned int i = 0;

    if (!optional_fields || optional_fields->field_count == 0 || !optional_fields->fields)
    {
        return;
    }

    (void)add_xml_element(siData->doc, &siData->doc_len, "OptionalFields",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    for (i = 0; i < optional_fields->field_count; ++i)
    {
        if (optional_fields->fields[i])
        {
            (void)add_xml_element(siData->doc, &siData->doc_len, "Field",
                optional_fields->fields[i], NEED_FORMALIZE, ADD_NAME_CONTENT);
        }
    }

    (void)add_xml_element(siData->doc, &siData->doc_len, "OptionalFields",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
}

/**
 * 构建桶清单配置的XML请求体
 */
static obs_status build_inventory_xml(set_inventory_data *siData,
    obs_inventory_configuration *inventory_config)
{
    // 开始 InventoryConfiguration 元素
    (void)add_xml_element(siData->doc, &siData->doc_len, "InventoryConfiguration",
        NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);

    // 添加清单ID
    if (inventory_config->id)
    {
        (void)add_xml_element(siData->doc, &siData->doc_len, "Id", inventory_config->id,
            NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    // 添加是否启用
    (void)add_xml_element(siData->doc, &siData->doc_len, "IsEnabled",
        inventory_config->is_enabled ? "true" : "false",
        NEED_FORMALIZE, ADD_NAME_CONTENT);

    // 添加过滤器（可选）
    if (inventory_config->filter && inventory_config->filter->prefix)
    {
        build_inventory_filter_xml(siData, inventory_config->filter);
    }

    // 添加目标桶配置
    if (inventory_config->destination)
    {
        build_inventory_destination_xml(siData, inventory_config);
    }

    // 添加调度频率
    if (inventory_config->schedule)
    {
        (void)add_xml_element(siData->doc, &siData->doc_len, "Schedule",
            NULL, NEED_FORMALIZE, ADD_HEAD_ONLY);
        (void)add_xml_element(siData->doc, &siData->doc_len, "Frequency",
            inventory_config->schedule, NEED_FORMALIZE, ADD_NAME_CONTENT);
        (void)add_xml_element(siData->doc, &siData->doc_len, "Schedule",
            NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
    }

    // 添加包含对象版本
    if (inventory_config->included_object_versions)
    {
        (void)add_xml_element(siData->doc, &siData->doc_len, "IncludedObjectVersions",
            inventory_config->included_object_versions, NEED_FORMALIZE, ADD_NAME_CONTENT);
    }

    // 添加可选字段
    build_optional_fields_xml(siData, inventory_config->optional_fields);

    // 结束 InventoryConfiguration 元素
    (void)add_xml_element(siData->doc, &siData->doc_len, "InventoryConfiguration",
        NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);

    return OBS_STATUS_OK;
}

/**
 * 数据回调函数
 */
static int set_inventory_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    set_inventory_data *siData = (set_inventory_data *)callback_data;

    if (!siData->doc_len)
    {
        return 0;
    }

    int remaining = (siData->doc_len - siData->doc_bytes_written);
    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy)
    {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(siData->doc[siData->doc_bytes_written]), toCopy);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "set_inventory_data_callback: memcpy_s failed!\n");
        return 0;
    }

    siData->doc_bytes_written += toCopy;
    return toCopy;
}

/**
 * 属性回调函数
 */
static obs_status set_inventory_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_inventory_data *siData = (set_inventory_data *)callback_data;
    if (siData->response_properties_callback)
    {
        return (*(siData->response_properties_callback))(response_properties,
            siData->callback_data);
    }
    return OBS_STATUS_OK;
}

/**
 * 完成回调函数
 */
static void set_inventory_complete_callback(obs_status requestStatus,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_inventory_data *siData = (set_inventory_data *)callback_data;

    (void)(*(siData->response_complete_callback))(requestStatus, obs_error_info, siData->callback_data);

    free(siData);
    siData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

static obs_status validate_required_string(const char *value, const char *field_name,
    obs_response_handler *handler, void *callback_data)
{
    if (!value || strlen(value) == 0)
    {
        COMMLOG(OBS_LOGERROR, "%s is NULL or empty.", field_name);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return OBS_STATUS_InvalidParameter;
    }
    return OBS_STATUS_OK;
}

/**
 * 校验inventory配置参数
 */
static obs_status validate_inventory_config(const obs_options *options,
    obs_inventory_configuration *inventory_config,
    obs_response_handler *handler, void *callback_data)
{
    obs_status status;

    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return OBS_STATUS_InvalidBucketName;
    }

    if (NULL == inventory_config)
    {
        COMMLOG(OBS_LOGERROR, "inventory_config is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return OBS_STATUS_InvalidParameter;
    }

    status = validate_required_string(inventory_config->id, "inventory_id", handler, callback_data);
    if (status != OBS_STATUS_OK) return status;

    if (NULL == inventory_config->destination || NULL == inventory_config->destination->bucket ||
        strlen(inventory_config->destination->bucket) == 0)
    {
        COMMLOG(OBS_LOGERROR, "destination or destination.bucket is NULL or empty.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return OBS_STATUS_InvalidParameter;
    }

    status = validate_required_string(inventory_config->schedule, "schedule", handler, callback_data);
    if (status != OBS_STATUS_OK) return status;

    status = validate_required_string(inventory_config->format, "format", handler, callback_data);
    if (status != OBS_STATUS_OK) return status;

    status = validate_required_string(inventory_config->included_object_versions,
        "included_object_versions", handler, callback_data);
    return status;
}

/**
 * 初始化设置桶清单配置的数据结构
 */
static set_inventory_data* init_set_inventory_data(obs_inventory_configuration *inventory_config,
    obs_response_handler *handler, void *callback_data)
{
    unsigned char doc_md5[16];
    set_inventory_data *siData = NULL;

    siData = (set_inventory_data *)malloc(sizeof(set_inventory_data));
    if (!siData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc set_inventory_data failed.");
        return NULL;
    }
    memset_s(siData, sizeof(set_inventory_data), 0, sizeof(set_inventory_data));

    siData->response_complete_callback = handler->complete_callback;
    siData->response_properties_callback = handler->properties_callback;
    siData->callback_data = callback_data;
    siData->doc_len = 0;

    obs_status ret_status = build_inventory_xml(siData, inventory_config);
    if (OBS_STATUS_OK != ret_status || siData->doc_len < 0)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        free(siData);
        siData = NULL;
        return NULL;
    }

    COMMLOG(OBS_LOGERROR, "set_inventory doc: %s.", siData->doc);
    siData->doc_bytes_written = 0;
    /* MD5 is required by OBS API protocol for Content-MD5 header, not used for security purposes */
    MD5((unsigned char *)siData->doc, (size_t)siData->doc_len, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), siData->doc_md5);

    return siData;
}

/**
 * 设置桶清单配置
 */
void set_bucket_inventory(const obs_options *options,
    obs_inventory_configuration *inventory_config,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_put_properties put_properties;
    set_inventory_data *siData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set_bucket_inventory start !");

    // 参数校验
    obs_status validate_status = validate_inventory_config(options, inventory_config, handler, callback_data);
    if (OBS_STATUS_OK != validate_status)
    {
        return;
    }

    // 初始化数据结构
    siData = init_set_inventory_data(inventory_config, handler, callback_data);
    if (!siData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc set_inventory_data failed.");
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
    put_properties.md5 = siData->doc_md5;
    put_properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &set_inventory_properties_callback;
    params.toObsCallback = &set_inventory_data_callback;
    params.complete_callback = &set_inventory_complete_callback;
    params.toObsCallbackTotalSize = siData->doc_len;
    params.callback_data = siData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "inventory";
    params.put_properties = &put_properties;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;

    // 添加inventory_id作为查询参数
    if (inventory_config->id)
    {
        char query_param[512];
        int ret = snprintf_s(query_param, sizeof(query_param), _TRUNCATE,
            "id=%s", inventory_config->id);
        if (ret > 0)
        {
            params.queryParams = strdup(query_param);
        }
    }

    request_perform(&params);

    if (params.queryParams)
    {
        free(params.queryParams);
    }

    COMMLOG(OBS_LOGINFO, "set_bucket_inventory finish.");
}
