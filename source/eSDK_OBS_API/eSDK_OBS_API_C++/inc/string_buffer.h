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
#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <stdio.h>
#include "securec.h"
#include "log.h"

#ifdef _MSC_VER
#define snprintf_s _snprintf_s
#endif

#define QUERY_STRING_LEN 4096		
#define HEAD_NAME_LEN 128		
#define SIGNED_HEADERS_LEN 1024	

#define string_buffer(name, len)                                        \
    char name[len + 1];                                                 \
    int name##Len


#define string_buffer_initialize(sb)                                    \
    do {                                                                \
        sb[0] = 0;                                                      \
        sb##Len = 0;                                                    \
    } while (0)

#define string_buffer_append(sb, str, len, all_fit)                     \
    do {                                                                \
        if(snprintf_s(&(sb[sb##Len]), sizeof(sb) - sb##Len , sizeof(sb) - sb##Len -1, "%.*s", (int) (len), str) > 0) \
        {                                                                \
            sb##Len += snprintf_s(&(sb[sb##Len]), sizeof(sb) - sb##Len ,  \
            sizeof(sb) - sb##Len -1 ,                                     \
            "%.*s", (int) (len), str);                                    \
        }                                                               \
        if (sb##Len > (int) (sizeof(sb) - 1)) {                         \
            sb##Len = sizeof(sb) - 1;                                   \
            all_fit = 0;                                                \
        }                                                               \
        else {                                                          \
            all_fit = 1;                                                \
        }                                                               \
    } while (0)

#define string_multibuffer(name, size)                                  \
    char name[size];                                                    \
    int name##Size

#define string_multibuffer_initialize(smb)                              \
    do {                                                                \
        smb##Size = 0;                                                  \
    } while (0)

#define string_multibuffer_current(smb)                                  \
    &(smb[smb##Size])

#define string_multibuffer_add(smb, str, len, all_fit)                  \
    do {                                                                \
        smb##Size += (snprintf_s(&(smb[smb##Size]), sizeof(smb) - smb##Size,    \
                               sizeof(smb) - smb##Size,                 \
                               "%.*s", (int) (len), str) + 1);          \
        if (smb##Size > (int) sizeof(smb)) {                            \
            smb##Size = sizeof(smb);                                    \
            all_fit = 0;                                                \
        }                                                               \
        else {                                                          \
            all_fit = 1;                                                \
        }                                                               \
    } while (0)

#define string_multibuffer_append(smb, str, len, all_fit)               \
    do {                                                                \
        smb##Size--;                                                    \
        string_multibuffer_add(smb, str, len, all_fit);                 \
    } while (0)

#define malloc_buffer_append(sb, str, len)                              \
    do                                                                  \
    {                                                                   \
        sb = (char*) malloc(len + 1);                                   \
        if (NULL == sb)                                                 \
        {                                                               \
            COMMLOG(OBS_LOGERROR, "malloc sb failed.");                \
            return OBS_STATUS_OutOfMemory;                            \
        }                                                               \
        memset_s(sb, len+1, 0, len+1);                                  \
        snprintf_s(sb, len+1, len, "%.*s", (int)len, str);              \
    }while (0)

#endif /* STRING_BUFFER_H */

#define safe_append_with_interface_log(name, value, complete_callback)\
    do {\
        int fit;\
        if (amp)\
        {\
            string_buffer_append(queryParams, "&", 1, fit);\
            if (!fit)\
            {\
                (void)(*(complete_callback))(OBS_STATUS_QueryParamsTooLong, 0, callback_data);\
                return;\
            }\
        }\
        string_buffer_append(queryParams, name "=", sizeof(name "=") - 1, fit);\
        if (!fit)\
        {\
            (void)(*(complete_callback))(OBS_STATUS_QueryParamsTooLong, 0, callback_data);\
            return;\
        }\
        amp = 1;\
        char encoded[3 * 1024];\
        int isFlag = 0;\
        if (isFlag == urlEncode(encoded, value, 1024, 0))\
        {\
            (void)(*(complete_callback))(OBS_STATUS_QueryParamsTooLong, 0, callback_data);\
            return;\
        }\
        string_buffer_append(queryParams, encoded, strlen(encoded), fit);\
        if (!fit)\
        {\
            (void)(*(complete_callback))(OBS_STATUS_QueryParamsTooLong, 0, callback_data);\
            return;\
        }\
    } while (0)

#define safe_append(name, value, complete_callback)\
    do {\
        int fit;\
        if (amp)\
        {\
        string_buffer_append(queryParams, "&", 1, fit);\
        if (!fit)\
        {\
            (void)(*(complete_callback))(OBS_STATUS_QueryParamsTooLong, 0, callback_data);\
            return;\
        }\
        }\
        string_buffer_append(queryParams, name "=", sizeof(name "=") - 1, fit);\
        if (!fit)\
        {\
            (void)(*(complete_callback))(OBS_STATUS_QueryParamsTooLong, 0, callback_data);\
            return;\
        }\
        amp = 1;\
        char encoded[3 * 1024];\
        int isFlag = 0;\
        if (isFlag == urlEncode(encoded, value, 1024, 0))\
        {\
            (void)(*(complete_callback))(OBS_STATUS_QueryParamsTooLong, 0, callback_data);\
            return;\
        }\
        string_buffer_append(queryParams, encoded, strlen(encoded), fit);\
        if (!fit)\
        {\
            (void)(*(complete_callback))(OBS_STATUS_QueryParamsTooLong, 0, callback_data);\
            return;\
        }\
    } while (0)

#define safe_append_status(name, value)\
    do {\
        int fit;\
        if (amp)\
        {\
            string_buffer_append(queryParams, "&", 1, fit);\
            if (!fit)\
            {\
                return OBS_STATUS_QueryParamsTooLong;\
            }\
        }\
        string_buffer_append(queryParams, name "=", sizeof(name "=") - 1, fit);\
        if (!fit)\
        {\
            return OBS_STATUS_QueryParamsTooLong;\
        }\
        amp = 1;\
        char encoded[3 * 1024];\
        int isFlag = 0;\
        if (isFlag == urlEncode(encoded, value, 1024, 0))\
        {\
            return OBS_STATUS_QueryParamsTooLong;\
        }\
        string_buffer_append(queryParams, encoded, strlen(encoded), fit);\
        if (!fit)\
        {\
            return OBS_STATUS_QueryParamsTooLong;\
        }\
    } while (0)

