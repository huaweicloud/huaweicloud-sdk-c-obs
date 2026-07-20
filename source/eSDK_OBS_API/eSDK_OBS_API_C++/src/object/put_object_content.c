/*********************************************************************************
* Copyright 2019 Huawei Technologies Co.,Ltd.
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use
* this file except in compliance with the License.  You may obtain a copy of the
* License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software distributed
* under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations under the License.
**********************************************************************************
*/
#include "object.h"
#include "request_util.h"
#include "../crc64/crc64.h"
#include <string.h>
#include <stdlib.h>
#include "securec.h"

/**
 * @brief 内部数据结构,用于保存文本内容上传的上下文
 */
typedef struct put_content_data {
    const char *content;          // 文本内容指针
    uint64_t content_length;      // 内容总长度
    uint64_t bytes_sent;          // 已发送的字节数
    void *user_callback_data;     // 用户原始回调数据
    obs_put_object_handler original_handler;    // 用户原始handler(副本)
    int handler_allocated;        // 标记handler是否已分配
    char *crc64_str;              // CRC64字符串(需在回调中释放)
} put_content_data;

/**
 * @brief 包装属性回调,将回调数据转换回用户的回调数据
 */
static obs_status wrapped_properties_callback(const obs_response_properties *properties,
                                               void *callback_data)
{
    put_content_data *data = (put_content_data *)callback_data;

    // 调用用户原始的属性回调
    if (data && data->original_handler.response_handler.properties_callback) {
        return data->original_handler.response_handler.properties_callback(
            properties, data->user_callback_data);
    }
    return OBS_STATUS_OK;
}

/**
 * @brief 包装完成回调,在回调执行后释放内部数据
 */
static void wrapped_complete_callback(obs_status status,
                                       const obs_error_details *error_details,
                                       void *callback_data)
{
    put_content_data *data = (put_content_data *)callback_data;

    // 调用用户原始的完成回调
    if (data && data->original_handler.response_handler.complete_callback) {
        data->original_handler.response_handler.complete_callback(
            status, error_details, data->user_callback_data);
    }

    // 释放CRC64字符串
    if (data && data->crc64_str) {
        free(data->crc64_str);
    }

    // 释放内部数据结构
    if (data) {
        free(data);
    }
}

/**
 * @brief 包装进度回调,将回调数据转换回用户的回调数据
 */
static void wrapped_progress_callback(uint64_t now, uint64_t total,
                                       void *callback_data)
{
    put_content_data *data = (put_content_data *)callback_data;

    // 调用用户原始的进度回调
    if (data && data->original_handler.progress_callback) {
        data->original_handler.progress_callback(
            now, total, data->user_callback_data);
    }
}

/**
 * @brief 内部回调函数,用于提供文本数据
 *
 * @param buffer_size 缓冲区大小
 * @param buffer 输出缓冲区
 * @param callback_data 回调数据(put_content_data指针)
 * @return int 实际写入的字节数,0表示结束
 */
static int put_object_content_data_callback(int buffer_size, char *buffer,
                                            void *callback_data)
{
    put_content_data *data = (put_content_data *)callback_data;

    // 参数校验
    if (!data || !buffer || buffer_size <= 0) {
        COMMLOG(OBS_LOGERROR, "put_object_content_data_callback: invalid parameters");
        return 0;
    }

    // 计算剩余待发送的字节数
    uint64_t remaining_bytes = data->content_length - data->bytes_sent;

    // 如果已发送完毕,返回0
    if (remaining_bytes == 0) {
        return 0;
    }

    // 计算本次要发送的字节数
    int bytes_to_send = (int)(remaining_bytes < (uint64_t)buffer_size ?
                              remaining_bytes : buffer_size);

    // 拷贝数据到缓冲区
    memcpy_s(buffer, buffer_size,
             data->content + data->bytes_sent, bytes_to_send);

    // 更新已发送字节数
    data->bytes_sent += (uint64_t)bytes_to_send;

    return bytes_to_send;
}

/**
 * @brief 报告错误并调用完成回调
 */
static void put_content_report_error(obs_put_object_handler *handler,
                                     obs_status status, const char *message,
                                     void *callback_data)
{
    if (handler && handler->response_handler.complete_callback) {
        obs_error_details error;
        error.message = message;
        handler->response_handler.complete_callback(status, &error, callback_data);
    }
}

/**
 * @brief 校验put_object_content的参数,如果校验失败则调用完成回调并返回0
 * @return 1 表示校验通过, 0 表示校验失败(已调用回调)
 */
static int validate_put_content_params(const obs_options *options, const char *key,
                                       const char *content, uint64_t content_length,
                                       obs_put_object_handler *handler,
                                       void *callback_data)
{
    if (!options) {
        COMMLOG(OBS_LOGERROR, "put_object_content: options is NULL");
        put_content_report_error(handler, OBS_STATUS_InvalidArgument, "options is NULL", callback_data);
        return 0;
    }

    if (!key) {
        COMMLOG(OBS_LOGERROR, "put_object_content: key is NULL");
        put_content_report_error(handler, OBS_STATUS_InvalidArgument, "key is NULL", callback_data);
        return 0;
    }

    if (!handler) {
        COMMLOG(OBS_LOGERROR, "put_object_content: handler is NULL");
        return 0;
    }

    // 检查content_length是否在有效范围内
    // 单次上传对象大小范围是[0, 5GB]
    if (content_length > MAX_SINGLE_UPLOAD_SIZE) {
        COMMLOG(OBS_LOGERROR, "put_object_content: content_length exceeds 5GB limit");
        put_content_report_error(handler, OBS_STATUS_InvalidArgument,
            "content_length exceeds 5GB limit, use multipart upload instead", callback_data);
        return 0;
    }

    // 如果content为NULL但content_length大于0,这是错误情况
    if (!content && content_length > 0) {
        COMMLOG(OBS_LOGERROR, "put_object_content: content is NULL but content_length > 0");
        put_content_report_error(handler, OBS_STATUS_InvalidArgument,
            "content is NULL but content_length > 0", callback_data);
        return 0;
    }

    return 1;
}

/**
 * @brief 上传文本内容到对象存储
 *
 * @param options OBS配置选项
 * @param key 对象名
 * @param content 待上传的文本内容
 * @param content_length 文本内容长度
 * @param put_properties 上传属性
 * @param encryption_params 服务端加密参数
 * @param handler 响应处理器
 * @param callback_data 用户自定义回调数据
 */
void put_object_content(const obs_options *options, const char *key,
                        const char *content, uint64_t content_length,
                        obs_put_properties *put_properties,
                        server_side_encryption_params *encryption_params,
                        obs_put_object_handler *handler, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter put_object_content successfully !");

    // 参数校验
    if (!validate_put_content_params(options, key, content, content_length, handler, callback_data)) {
        return;
    }

    // 创建内部数据结构
    put_content_data *content_data = (put_content_data *)malloc(sizeof(put_content_data));
    if (!content_data) {
        COMMLOG(OBS_LOGERROR, "put_object_content: malloc failed for content_data");
        put_content_report_error(handler, OBS_STATUS_OutOfMemory, "malloc failed", callback_data);
        return;
    }

    // 初始化内部数据结构
    memset_s(content_data, sizeof(put_content_data), 0, sizeof(put_content_data));
    content_data->content = content;
    content_data->content_length = content_length;
    content_data->bytes_sent = 0;
    content_data->user_callback_data = callback_data;
    (void)memcpy_s(&content_data->original_handler, sizeof(obs_put_object_handler), handler, sizeof(obs_put_object_handler));
    content_data->handler_allocated = 1;

    // 创建新的handler,使用包装的回调和内部数据回调
    obs_put_object_handler content_handler;
    memset_s(&content_handler, sizeof(obs_put_object_handler), 0, sizeof(obs_put_object_handler));
    content_handler.response_handler.properties_callback = wrapped_properties_callback;
    content_handler.response_handler.complete_callback = wrapped_complete_callback;
    content_handler.put_object_data_callback = put_object_content_data_callback;
    content_handler.progress_callback = handler->progress_callback ? wrapped_progress_callback : NULL;

    // 如果启用了CRC64且未手动指定，计算CRC64值
    if (put_properties && put_properties->enable_crc64 &&
        (!put_properties->crc64 || !put_properties->crc64[0])) {
        uint64_t content_crc64 = obs_crc64_internal_compute((const unsigned char *)content, content_length);
        if (content_crc64 != 0) {
            char *crc64_str = (char *)malloc(32);
            if (crc64_str) {
                obs_crc64_to_string_internal(content_crc64, crc64_str, 32);
                put_properties->crc64 = crc64_str;
                content_data->crc64_str = crc64_str;
            }
        }
    }

    // 调用原始的put_object函数
    put_object(options, (char *)key, content_length,
               put_properties, encryption_params,
               &content_handler, content_data);

    // 注意:不在这里释放content_data,它会在wrapped_complete_callback中被释放

    COMMLOG(OBS_LOGINFO, "Leave put_object_content successfully !");
}
