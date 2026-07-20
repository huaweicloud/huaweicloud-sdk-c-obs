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
#ifndef PUT_DOC_COMMON_H
#define PUT_DOC_COMMON_H

#include "bucket.h"

#define BASE64_MD5_LEN 64

/* Generic data struct for PUT-requests that send a pre-built document (XML or JSON).
 * Users of this module MUST place this struct as their first member, or embed it
 * and pass its address to the shared callbacks. */
typedef struct put_doc_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    void *callback_data;

    char *doc;              /* pointer to document buffer (can be inline or heap-allocated) */
    int doc_len;
    int doc_bytes_written;
    char doc_md5[BASE64_MD5_LEN];
} put_doc_data;

/* Shared data callback: streams doc[] to the HTTP request body */
int put_doc_data_callback(int buffer_size, char *buffer, void *callback_data);

/* Shared properties callback: forwards to user callback */
obs_status put_doc_properties_callback(const obs_response_properties *response_properties,
    void *callback_data);

/* Shared complete callback: calls user complete_callback, then frees data */
void put_doc_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info, void *callback_data);

/* Compute MD5 of doc and base64-encode into doc_md5 field */
void put_doc_compute_md5(put_doc_data *data);

#endif
