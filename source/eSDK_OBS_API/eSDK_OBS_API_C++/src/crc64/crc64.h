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
**********************************************************************************
*/
#ifndef CRC64_H
#define CRC64_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CRC64 多项式 (ECMA-182) */
#define CRC64_POLY 0xC96C5795D7870F42ULL
#define CRC64_INIT 0xFFFFFFFFFFFFFFFFULL

/* 内部 CRC64 上下文结构（不对外暴露） */
typedef struct obs_crc64_internal {
    uint64_t value;
} obs_crc64_internal;

/* 内部函数，用于 SDK 内部流式计算 CRC64 */
void obs_crc64_internal_init(obs_crc64_internal *ctx);
void obs_crc64_internal_update(obs_crc64_internal *ctx, const unsigned char *buffer, size_t length);
uint64_t obs_crc64_internal_finalize(obs_crc64_internal *ctx);
void obs_crc64_internal_reset(obs_crc64_internal *ctx);

/* 内部函数，用于 SDK 内部一次性计算 CRC64 */
uint64_t obs_crc64_internal_compute(const unsigned char *buffer, size_t length);

/* 内部函数，用于 CRC64 值与字符串转换（处理 HTTP 头域） */
void obs_crc64_to_string_internal(uint64_t crc64_value, char *buffer, size_t buffer_size);
uint64_t obs_crc64_from_string_internal(const char *string);

/* 内部函数，用于分段上传合并 CRC64 */
uint64_t obs_crc64_internal_combine(uint64_t crc64_first, uint64_t crc64_second, size_t len2);

/* 内部函数，用于计算文件指定偏移量和大小的 CRC64 */
uint64_t obs_crc64_compute_file_range(const char *file_path, uint64_t offset, uint64_t size);

#ifdef __cplusplus
}
#endif

#endif /* CRC64_H */