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
#include "inventory_common.h"
#include "inventory_request_common.h"
#include "request_util.h"
#include <openssl/md5.h>

/* 公共XML路径前缀 */
#define INV_PATH_PREFIX "InventoryConfiguration/"
#define INV_PATH_PREFIX_LEN (sizeof(INV_PATH_PREFIX) - 1)

typedef struct get_inventory_data
{
    inventory_request_base base;  /* MUST be first member */

    obs_get_bucket_inventory_callback *get_inventory_callback;
    inventory_configuration_data *inv_data;
} get_inventory_data;

/**
 * 确保inventory_configuration_data存在（懒分配）
 */
static inventory_configuration_data* ensure_inventory_data(get_inventory_data *giData)
{
    if (!giData->inv_data)
    {
        inventory_configuration_data *inv_data = (inventory_configuration_data*)malloc(sizeof(inventory_configuration_data));
        if (inv_data)
        {
            memset_s(inv_data, sizeof(inventory_configuration_data), 0, sizeof(inventory_configuration_data));
            giData->inv_data = inv_data;
        }
    }
    return giData->inv_data;
}

/**
 * 解析XML响应数据
 */
static obs_status parse_inventory_xml(get_inventory_data *giData,
    const char *element_path, const char *data, int data_len)
{
    // 检查是否是InventoryConfiguration子元素
    if (strncmp(element_path, INV_PATH_PREFIX, INV_PATH_PREFIX_LEN) != 0)
    {
        return OBS_STATUS_OK;
    }

    inventory_configuration_data *inv_data = ensure_inventory_data(giData);
    if (!inv_data)
    {
        return OBS_STATUS_OK;
    }

    // 跳过公共前缀，使用后缀匹配
    const char *suffix = element_path + INV_PATH_PREFIX_LEN;
    int fit = 1;
    parse_inventory_field(inv_data, suffix, data, data_len, fit);

    (void)fit;
    return OBS_STATUS_OK;
}

/**
 * XML回调函数
 */
static obs_status get_inventory_xml_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    get_inventory_data *giData = (get_inventory_data *)callback_data;

    if (data)
    {
        return parse_inventory_xml(giData, element_path, data, data_len);
    }

    // 检查是否是InventoryConfiguration子元素
    if (strncmp(element_path, INV_PATH_PREFIX, INV_PATH_PREFIX_LEN) != 0)
    {
        return OBS_STATUS_OK;
    }

    inventory_configuration_data *inv_data = ensure_inventory_data(giData);
    if (!inv_data)
    {
        return OBS_STATUS_OK;
    }

    // 跳过公共前缀，使用后缀匹配
    const char *suffix = element_path + INV_PATH_PREFIX_LEN;

    // 检测到Filter元素
    if (!strcmp(suffix, "Filter"))
    {
        ensure_filter_data(inv_data);
    }
    // 检测到Destination元素
    else if (!strcmp(suffix, "Destination"))
    {
        ensure_destination_data(inv_data);
    }
    // 检测到OptionalFields元素
    else if (!strcmp(suffix, "OptionalFields"))
    {
        ensure_optional_fields_data(inv_data);
    }

    return OBS_STATUS_OK;
}

/**
 * 初始化获取桶清单配置的数据结构
 */
static get_inventory_data* init_get_inventory_data(obs_get_bucket_inventory_handler *handler,
    void *callback_data)
{
    get_inventory_data *giData = NULL;

    giData = (get_inventory_data *)malloc(sizeof(get_inventory_data));
    if (!giData)
    {
        COMMLOG(OBS_LOGERROR, "malloc get_inventory_data failed.");
        return NULL;
    }
    memset_s(giData, sizeof(get_inventory_data), 0, sizeof(get_inventory_data));

    simplexml_initialize(&(giData->base.simpleXml), &get_inventory_xml_callback, giData);

    giData->base.response_properties_callback = handler->response_handler.properties_callback;
    giData->base.response_complete_callback = handler->response_handler.complete_callback;
    giData->base.callback_data = callback_data;
    giData->get_inventory_callback = handler->get_bucket_inventory_callback;

    return giData;
}

/**
 * 构建返回给用户的配置结构体
 */
static obs_status make_get_inventory_callback(get_inventory_data *giData)
{
    obs_status retStatus = OBS_STATUS_OK;

    inventory_configuration_data *inv_data = giData->inv_data;
    if (!inv_data)
    {
        return OBS_STATUS_OK;
    }

    // 创建配置结构体
    obs_inventory_configuration config;
    memset_s(&config, sizeof(obs_inventory_configuration), 0, sizeof(obs_inventory_configuration));

    fill_inventory_config_common(&config, inv_data);

    // 调用用户回调
    retStatus = (*(giData->get_inventory_callback))(&config, giData->base.callback_data);

    // 释放分配的内存
    free_inventory_config(&config);

    return retStatus;
}

/**
 * 完成回调函数
 */
static void get_inventory_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_inventory_data *giData = (get_inventory_data *)callback_data;

    // 如果请求成功且有数据，则调用用户回调
    if (giData->inv_data && giData->inv_data->id[0] != '\0' && OBS_STATUS_OK == request_status)
    {
        request_status = make_get_inventory_callback(giData);
    }

    // 调用完整回调
    (*(giData->base.response_complete_callback))(request_status, obs_error_info,
        giData->base.callback_data);

    // 释放数据
    if (giData->inv_data)
    {
        free_inventory_data_internal(giData->inv_data);
        CHECK_NULL_FREE(giData->inv_data);
    }

    simplexml_deinitialize(&(giData->base.simpleXml));

    free(giData);
    giData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

/**
 * 获取桶清单配置
 */
void get_bucket_inventory(const obs_options *options, const char *inventory_id,
    obs_get_bucket_inventory_handler *handler, void *callback_data)
{
    request_params params;
    get_inventory_data *giData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get_bucket_inventory start !");

    // 参数校验
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    if (!inventory_id || strlen(inventory_id) == 0)
    {
        COMMLOG(OBS_LOGERROR, "inventory_id is NULL or empty.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    // 初始化数据结构
    giData = init_get_inventory_data(handler, callback_data);
    if (NULL == giData)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        return;
    }
    giData->base.use_api = use_api;

    // 初始化请求参数
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    setup_inventory_request_params(options, use_api, &params);
    params.complete_callback = &get_inventory_complete_callback;
    params.callback_data = giData;

    // 添加inventory_id作为查询参数
    char query_param[512];
    int ret = snprintf_s(query_param, sizeof(query_param), _TRUNCATE,
        "id=%s", inventory_id);
    if (ret > 0)
    {
        params.queryParams = strdup(query_param);
    }

    request_perform(&params);

    if (params.queryParams)
    {
        free(params.queryParams);
    }

    COMMLOG(OBS_LOGINFO, "get_bucket_inventory finish.");
}
