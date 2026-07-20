/*********************************************************************************
* Copyright 2022 Huawei Technologies Co.,Ltd.
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

#ifndef JSON_ERROR_ALIAS_H
#define JSON_ERROR_ALIAS_H

#include "eSDKOBS.h"

/* Table-driven mapping: OBS JSON error code alias -> obs_status
 *
 * These codes are returned by OBS OEF proxy / DIS service in JSON format,
 * where the "code" field does not match the standard S3 error code naming
 * handled by the HANDLE_CODE macros in error_parser.c.
 */
typedef struct json_error_alias_mapping {
    const char *code;
    obs_status status;
} json_error_alias_mapping;

static const json_error_alias_mapping g_json_alias_table[] = {
    /* OBS跨区域复制 */
    {"CRR.00120005", OBS_STATUS_HttpErrorConflict},               /* 跨区域复制资源冲突 */
    {"CRR.00120006", OBS_STATUS_NoSuchReplicationConfiguration},  /* 跨区域复制配置不存在 */
    /* DIS notification policy */
    {"Notification.1011", OBS_STATUS_NoSuchDisConfiguration},     /* DIS policy rules not found */
    {"Notification.0001", OBS_STATUS_InvalidArgument},            /* invalid DIS policy parameter */
    /* Compress policy (online decompression) */
    {"OEFTrigger.1002", OBS_STATUS_NoSuchCompressConfiguration},  /* Compress policy rules not found */
    /* OEF proxy common */
    {"SYS.0014", OBS_STATUS_NoSuchBucket},                        /* Bucket not exist */
};
static const int g_json_alias_table_size = sizeof(g_json_alias_table) / sizeof(g_json_alias_table[0]);

#endif /* JSON_ERROR_ALIAS_H */
