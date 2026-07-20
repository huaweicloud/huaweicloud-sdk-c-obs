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
#ifndef INVENTORY_REQUEST_COMMON_H
#define INVENTORY_REQUEST_COMMON_H

#include "bucket.h"
#include "simplexml.h"

/* Common base for GET and LIST inventory request data.
 * Must be the FIRST member of any struct that uses the shared callbacks. */
typedef struct inventory_request_base
{
    simple_xml simpleXml;

    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    void *callback_data;

    obs_use_api use_api;
} inventory_request_base;

/* Shared properties callback -- works for any struct whose first member is inventory_request_base */
obs_status inventory_request_properties_callback(
    const obs_response_properties *response_properties, void *callback_data);

/* Shared data callback -- feeds data to simpleXml parser */
obs_status inventory_request_data_callback(int buffer_size, const char *buffer,
    void *callback_data);

/* Set up request_params for inventory GET/LIST: copy bucketContext + request_option, set common fields.
 * Caller must still set params.subResource, params.queryParams, etc. */
void setup_inventory_request_params(const obs_options *options,
    obs_use_api use_api, request_params *params);

#endif
