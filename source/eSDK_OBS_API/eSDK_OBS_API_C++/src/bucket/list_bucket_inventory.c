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

#define MAX_INVENTORY_LIST_COUNT 100

/* 公共XML路径前缀 */
#define INV_PATH_PREFIX "ListInventoryConfiguration/InventoryConfiguration/"
#define INV_PATH_PREFIX_LEN (sizeof(INV_PATH_PREFIX) - 1)

typedef struct list_inventory_data
{
    inventory_request_base base;  /* MUST be first member */

    obs_list_bucket_inventory_callback *list_inventory_callback;
    inventory_configuration_data *inventories[MAX_INVENTORY_LIST_COUNT];
    unsigned int inventory_count;
    int current_inventory_complete;
} list_inventory_data;

/**
 * 确保当前inventory_configuration_data存在（懒分配）
 */
static inventory_configuration_data* ensure_inventory_data(list_inventory_data *liData)
{
    // 如果当前inventory未完成，返回现有的
    if (liData->inventory_count > 0 && !liData->current_inventory_complete
        && liData->inventories[liData->inventory_count - 1])
    {
        return liData->inventories[liData->inventory_count - 1];
    }

    // 需要创建新的inventory数据
    if (liData->inventory_count >= MAX_INVENTORY_LIST_COUNT)
    {
        COMMLOG(OBS_LOGWARN, "Maximum number of inventory configurations reached.");
        return NULL;
    }

    inventory_configuration_data *inv_data = (inventory_configuration_data*)malloc(sizeof(inventory_configuration_data));
    if (!inv_data)
    {
        COMMLOG(OBS_LOGERROR, "malloc inventory_configuration_data failed !");
        return NULL;
    }
    memset_s(inv_data, sizeof(inventory_configuration_data), 0, sizeof(inventory_configuration_data));
    liData->inventories[liData->inventory_count] = inv_data;
    liData->inventory_count++;
    liData->current_inventory_complete = 0;
    return inv_data;
}

/**
 * 解析XML响应数据
 */
static obs_status parse_list_inventory_xml(list_inventory_data *liData,
    const char *element_path, const char *data, int data_len)
{
    int fit = 1;

    // 检查是否是InventoryConfiguration子元素
    if (strncmp(element_path, INV_PATH_PREFIX, INV_PATH_PREFIX_LEN) != 0)
    {
        (void)fit;
        return OBS_STATUS_OK;
    }

    inventory_configuration_data *inv_data = ensure_inventory_data(liData);
    if (!inv_data)
    {
        return OBS_STATUS_OK;
    }

    // 跳过公共前缀，使用后缀匹配
    const char *suffix = element_path + INV_PATH_PREFIX_LEN;

    parse_inventory_field(inv_data, suffix, data, data_len, fit);

    (void)fit;
    return OBS_STATUS_OK;
}

/**
 * XML回调函数
 */
static obs_status list_inventory_xml_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    list_inventory_data *liData = (list_inventory_data *)callback_data;

    if (data)
    {
        return parse_list_inventory_xml(liData, element_path, data, data_len);
    }

    // endElement回调 - 检测InventoryConfiguration结束
    if (!strcmp(element_path, "ListInventoryConfiguration/InventoryConfiguration"))
    {
        liData->current_inventory_complete = 1;
    }

    return OBS_STATUS_OK;
}

/**
 * 初始化列举桶清单配置的数据结构
 */
static list_inventory_data* init_list_inventory_data(obs_list_bucket_inventory_handler *handler,
    void *callback_data)
{
    list_inventory_data *liData = NULL;

    liData = (list_inventory_data *)malloc(sizeof(list_inventory_data));
    if (!liData)
    {
        COMMLOG(OBS_LOGERROR, "malloc list_inventory_data failed.");
        return NULL;
    }
    memset_s(liData, sizeof(list_inventory_data), 0, sizeof(list_inventory_data));

    simplexml_initialize(&(liData->base.simpleXml), &list_inventory_xml_callback, liData);

    liData->base.response_properties_callback = handler->response_handler.properties_callback;
    liData->base.response_complete_callback = handler->response_handler.complete_callback;
    liData->base.callback_data = callback_data;
    liData->list_inventory_callback = handler->list_bucket_inventory_callback;

    return liData;
}

/**
 * 构建返回给用户的配置结构体数组
 */
static obs_status make_list_inventory_callback(list_inventory_data *liData)
{
    obs_status retStatus = OBS_STATUS_OK;
    unsigned int i = 0;

    // 分配配置数组
    obs_inventory_configuration **inventories = (obs_inventory_configuration**)malloc(
        sizeof(obs_inventory_configuration*) * liData->inventory_count);
    if (NULL == inventories)
    {
        COMMLOG(OBS_LOGERROR, "malloc obs_inventory_configuration array failed.");
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(inventories, sizeof(obs_inventory_configuration*) * liData->inventory_count, 0,
        sizeof(obs_inventory_configuration*) * liData->inventory_count);

    // 填充配置数据
    for (i = 0; i < liData->inventory_count; ++i)
    {
        obs_inventory_configuration *config = (obs_inventory_configuration*)malloc(sizeof(obs_inventory_configuration));
        if (!config)
        {
            COMMLOG(OBS_LOGERROR, "malloc obs_inventory_configuration failed.");
            retStatus = OBS_STATUS_OutOfMemory;
            goto cleanup;
        }
        memset_s(config, sizeof(obs_inventory_configuration), 0, sizeof(obs_inventory_configuration));
        inventories[i] = config;
        fill_inventory_config_common(config, liData->inventories[i]);
    }

    // 调用用户回调
    retStatus = (*(liData->list_inventory_callback))(inventories, liData->inventory_count,
        liData->base.callback_data);

cleanup:
    // 释放分配的内存
    for (i = 0; i < liData->inventory_count; ++i)
    {
        if (inventories[i])
        {
            free_inventory_config(inventories[i]);
            free(inventories[i]);
        }
    }
    CHECK_NULL_FREE(inventories);

    return retStatus;
}

/**
 * 完成回调函数
 */
static void list_inventory_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info, void *callback_data)
{
    unsigned int i = 0;
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_inventory_data *liData = (list_inventory_data *)callback_data;

    // 如果请求成功且有数据，则调用用户回调
    if (liData->inventory_count > 0 && OBS_STATUS_OK == request_status)
    {
        request_status = make_list_inventory_callback(liData);
    }

    // 调用完整回调
    (*(liData->base.response_complete_callback))(request_status, obs_error_info,
        liData->base.callback_data);

    // 释放数据
    for (i = 0; i < liData->inventory_count; i++)
    {
        if (liData->inventories[i])
        {
            free_inventory_data_internal(liData->inventories[i]);
            CHECK_NULL_FREE(liData->inventories[i]);
        }
    }

    simplexml_deinitialize(&(liData->base.simpleXml));

    free(liData);
    liData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

/**
 * 列举桶清单配置
 */
void list_bucket_inventory(const obs_options *options,
    obs_list_bucket_inventory_handler *handler, void *callback_data)
{
    request_params params;
    list_inventory_data *liData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "list_bucket_inventory start !");

    // 参数校验
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    // 初始化数据结构
    liData = init_list_inventory_data(handler, callback_data);
    if (NULL == liData)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        return;
    }
    liData->base.use_api = use_api;

    // 初始化请求参数
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    setup_inventory_request_params(options, use_api, &params);
    params.complete_callback = &list_inventory_complete_callback;
    params.callback_data = liData;

    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "list_bucket_inventory finish.");
}
