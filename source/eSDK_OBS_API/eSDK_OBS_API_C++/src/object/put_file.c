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
**********************************************************************************/
#include "object.h"
#include "request_util.h"
#include "../crc64/crc64.h"
#include <string.h>
#include <stdlib.h>
#include "securec.h"

#if defined __GNUC__ || defined LINUX
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#else
#include <io.h>
#include <share.h>
#endif

/**
 * @brief 内部数据结构,用于保存文件上传的上下文
 */
typedef struct put_file_data {
    int fd;                          // 文件描述符
    uint64_t file_size;              // 文件总大小
    uint64_t bytes_sent;             // 已发送的字节数
    void *user_callback_data;        // 用户原始回调数据
    obs_put_object_handler original_handler;  // 用户原始handler(副本)
    int handler_allocated;           // 标记handler是否已分配
    char *crc64_str;                 // CRC64字符串(需在回调中释放)
} put_file_data;

/**
 * @brief 包装属性回调,将回调数据转换回用户的回调数据
 */
static obs_status wrapped_properties_callback(const obs_response_properties *properties,
                                               void *callback_data)
{
    put_file_data *data = (put_file_data *)callback_data;

    // 调用用户原始的属性回调
    if (data && data->original_handler.response_handler.properties_callback) {
        return data->original_handler.response_handler.properties_callback(
            properties, data->user_callback_data);
    }
    return OBS_STATUS_OK;
}

/**
 * @brief 包装完成回调,在回调执行后释放内部数据和关闭文件
 */
static void wrapped_complete_callback(obs_status status,
                                       const obs_error_details *error_details,
                                       void *callback_data)
{
    put_file_data *data = (put_file_data *)callback_data;

    // 关闭文件
    if (data && data->fd != -1) {
#if defined __GNUC__ || defined LINUX
        close(data->fd);
#else
        _close(data->fd);
#endif
    }

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
 * @brief 内部回调函数,用于从文件读取数据并上传
 *
 * @param buffer_size 缓冲区大小
 * @param buffer 输出缓冲区
 * @param callback_data 回调数据(put_file_data指针)
 * @return int 实际读取的字节数,0表示结束
 */
static int put_file_data_callback(int buffer_size, char *buffer,
                                   void *callback_data)
{
    put_file_data *data = (put_file_data *)callback_data;

    // 参数校验
    if (!data || !buffer || buffer_size <= 0) {
        COMMLOG(OBS_LOGERROR, "put_file_data_callback: invalid parameters");
        return 0;
    }

    // 检查文件描述符
    if (data->fd == -1) {
        COMMLOG(OBS_LOGERROR, "put_file_data_callback: file descriptor is invalid");
        return 0;
    }

    // 计算剩余待发送的字节数
    uint64_t remaining_bytes = data->file_size - data->bytes_sent;

    // 如果已发送完毕,返回0
    if (remaining_bytes == 0) {
        return 0;
    }

    // 计算本次要读取的字节数
    int bytes_to_read = (int)(remaining_bytes < (uint64_t)buffer_size ?
                              remaining_bytes : buffer_size);

    // 从文件读取数据
    int bytes_read = 0;
#if defined __GNUC__ || defined LINUX
    bytes_read = (int)read(data->fd, buffer, bytes_to_read);
    if (bytes_read < 0) {
        COMMLOG(OBS_LOGERROR, "put_file_data_callback: read failed");
        return 0;
    }
#else
    bytes_read = _read(data->fd, buffer, bytes_to_read);
    if (bytes_read < 0) {
        COMMLOG(OBS_LOGERROR, "put_file_data_callback: _read failed");
        return 0;
    }
#endif

    // 更新已发送字节数
    data->bytes_sent += (uint64_t)bytes_read;

    return bytes_read;
}

/**
 * @brief 上传本地文件到对象存储
 *
 * @param options OBS配置选项
 * @param key 对象名
 * @param file_path 待上传文件的完整路径
 * @param put_properties 上传属性
 * @param encryption_params 服务端加密参数
 * @param handler 响应处理器
 * @param callback_data 用户自定义回调数据
 */

/**
 * @brief 报告错误并调用完成回调
 */
static void put_file_report_error(obs_put_object_handler *handler,
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
 * @brief 校验put_file的参数,如果校验失败则调用完成回调并返回0
 * @return 1 表示校验通过, 0 表示校验失败(已调用回调)
 */
static int validate_put_file_params(const obs_options *options, const char *key,
                                    const char *file_path,
                                    obs_put_object_handler *handler,
                                    void *callback_data)
{
    if (!options) {
        COMMLOG(OBS_LOGERROR, "put_file: options is NULL");
        put_file_report_error(handler, OBS_STATUS_InvalidArgument, "options is NULL", callback_data);
        return 0;
    }

    if (!key) {
        COMMLOG(OBS_LOGERROR, "put_file: key is NULL");
        put_file_report_error(handler, OBS_STATUS_InvalidArgument, "key is NULL", callback_data);
        return 0;
    }

    if (!file_path) {
        COMMLOG(OBS_LOGERROR, "put_file: file_path is NULL");
        put_file_report_error(handler, OBS_STATUS_InvalidArgument, "file_path is NULL", callback_data);
        return 0;
    }

    if (!handler) {
        COMMLOG(OBS_LOGERROR, "put_file: handler is NULL");
        return 0;
    }

    return 1;
}

/**
 * @brief 获取文件大小
 * @return 1 表示成功(文件大小写入*file_size), 0 表示失败(已调用回调)
 */
static int get_file_size_for_put(const char *file_path,
                                 obs_put_object_handler *handler,
                                 uint64_t *file_size, void *callback_data)
{
#if defined __GNUC__ || defined LINUX
    struct stat statbuf;
    if (stat(file_path, &statbuf) != 0) {
        COMMLOG(OBS_LOGERROR, "put_file: stat failed for file_path: %s", file_path);
        put_file_report_error(handler, OBS_STATUS_OpenFileFailed, "file not accessible", callback_data);
        return 0;
    }
    *file_size = (uint64_t)statbuf.st_size;
#else
    struct _stati64 statbuf;
    if (file_stati64(file_path, &statbuf) != 0) {
        COMMLOG(OBS_LOGERROR, "put_file: file_stati64 failed for file_path: %s", file_path);
        put_file_report_error(handler, OBS_STATUS_OpenFileFailed, "file not accessible", callback_data);
        return 0;
    }
    *file_size = (uint64_t)statbuf.st_size;
#endif
    return 1;
}

/**
 * @brief 打开文件用于put上传
 * @return 文件描述符, -1 表示失败(已调用回调)
 */
static int open_file_for_put(const char *file_path,
                             obs_put_object_handler *handler,
                             void *callback_data)
{
    int fd = -1;
#if defined __GNUC__ || defined LINUX
    char resolved_path[PATH_MAX];
    if (realpath(file_path, resolved_path) == NULL) {
        COMMLOG(OBS_LOGERROR, "put_file: realpath failed for file_path: %s", file_path);
        put_file_report_error(handler, OBS_STATUS_OpenFileFailed, "failed to resolve file path", callback_data);
        return -1;
    }
    fd = open(resolved_path, O_RDONLY);
    if (fd == -1) {
        COMMLOG(OBS_LOGERROR, "put_file: open failed for file_path: %s", file_path);
        put_file_report_error(handler, OBS_STATUS_OpenFileFailed, "failed to open file", callback_data);
    }
#else
    if (_sopen_s(&fd, file_path, _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD) != 0) {
        COMMLOG(OBS_LOGERROR, "put_file: _sopen_s failed for file_path: %s", file_path);
        put_file_report_error(handler, OBS_STATUS_OpenFileFailed, "failed to open file", callback_data);
        fd = -1;
    }
#endif
    return fd;
}

/**
 * @brief 关闭文件描述符(跨平台)
 */
static void close_file_fd(int fd)
{
#if defined __GNUC__ || defined LINUX
    close(fd);
#else
    _close(fd);
#endif
}

void put_file(const obs_options *options, const char *key,
              const char *file_path,
              obs_put_properties *put_properties,
              server_side_encryption_params *encryption_params,
              obs_put_object_handler *handler, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter put_file successfully !");

    // 参数校验
    if (!validate_put_file_params(options, key, file_path, handler, callback_data)) {
        return;
    }

    // 获取文件大小
    uint64_t file_size = 0;
    if (!get_file_size_for_put(file_path, handler, &file_size, callback_data)) {
        return;
    }

    // 检查文件大小是否在有效范围内
    // 单次上传对象大小范围是[0, 5GB]
    if (file_size > MAX_SINGLE_UPLOAD_SIZE) {
        COMMLOG(OBS_LOGERROR, "put_file: file_size exceeds 5GB limit");
        put_file_report_error(handler, OBS_STATUS_InvalidArgument,
            "file_size exceeds 5GB limit, use upload_file for multipart upload", callback_data);
        return;
    }

    // 打开文件
    int fd = open_file_for_put(file_path, handler, callback_data);
    if (fd == -1) {
        return;
    }

    // 创建内部数据结构
    put_file_data *file_data = (put_file_data *)malloc(sizeof(put_file_data));
    if (!file_data) {
        COMMLOG(OBS_LOGERROR, "put_file: malloc failed for file_data");
        close_file_fd(fd);
        put_file_report_error(handler, OBS_STATUS_OutOfMemory, "malloc failed", callback_data);
        return;
    }

    // 初始化内部数据结构
    memset_s(file_data, sizeof(put_file_data), 0, sizeof(put_file_data));
    file_data->fd = fd;
    file_data->file_size = file_size;
    file_data->bytes_sent = 0;
    file_data->user_callback_data = callback_data;
    (void)memcpy_s(&file_data->original_handler, sizeof(obs_put_object_handler), handler, sizeof(obs_put_object_handler));
    file_data->handler_allocated = 1;

    // 创建新的handler,使用包装的回调和内部数据回调
    obs_put_object_handler file_handler;
    memset_s(&file_handler, sizeof(obs_put_object_handler), 0, sizeof(obs_put_object_handler));
    file_handler.response_handler.properties_callback = wrapped_properties_callback;
    file_handler.response_handler.complete_callback = wrapped_complete_callback;
    file_handler.put_object_data_callback = put_file_data_callback;
    file_handler.progress_callback = handler->progress_callback;

    // 如果启用了CRC64且未手动指定，计算CRC64值
    if (put_properties && put_properties->enable_crc64 &&
        (!put_properties->crc64 || !put_properties->crc64[0])) {
        uint64_t file_crc64 = obs_compute_file_crc64(file_path);
        if (file_crc64 != 0) {
            char *crc64_str = (char *)malloc(32);
            if (crc64_str) {
                obs_crc64_to_string_internal(file_crc64, crc64_str, 32);
                put_properties->crc64 = crc64_str;
                file_data->crc64_str = crc64_str;
            }
        }
    }

    // 调用原始的put_object函数
    put_object(options, (char *)key, file_size,
               put_properties, encryption_params,
               &file_handler, file_data);

    // 注意:不在这里释放file_data和关闭文件,它会在wrapped_complete_callback中被释放

    COMMLOG(OBS_LOGINFO, "Leave put_file successfully !");
}
