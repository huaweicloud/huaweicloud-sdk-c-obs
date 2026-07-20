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
#ifndef INVENTORY_COMMON_H
#define INVENTORY_COMMON_H

#include "bucket.h"
#include "string_buffer.h"

#define MAX_INVENTORY_ID_LEN 256
#define MAX_INVENTORY_FIELDS 20

typedef struct inventory_destination_data
{
    string_buffer(bucket, 256);
    string_buffer(prefix, 1024);
} inventory_destination_data;

typedef struct inventory_filter_data
{
    string_buffer(prefix, 1024);
} inventory_filter_data;

typedef struct inventory_optional_fields_data
{
    unsigned int field_count;
    char *fields[MAX_INVENTORY_FIELDS];
} inventory_optional_fields_data;

typedef struct inventory_configuration_data
{
    string_buffer(id, MAX_INVENTORY_ID_LEN);
    string_buffer(is_enabled, 64);
    inventory_destination_data *destination;
    inventory_filter_data *filter;
    inventory_optional_fields_data *optional_fields;
    string_buffer(schedule, 64);
    string_buffer(format, 64);
    string_buffer(included_object_versions, 64);
} inventory_configuration_data;

/* Lazy allocation helpers for sub-structures */
void ensure_destination_data(inventory_configuration_data *inv_data);
void ensure_filter_data(inventory_configuration_data *inv_data);
void ensure_optional_fields_data(inventory_configuration_data *inv_data);

/* XML parsing helpers */
void parse_filter_prefix(inventory_configuration_data *inv_data,
    const char *data, int data_len, int fit);
void parse_destination_element(inventory_configuration_data *inv_data,
    const char *suffix, const char *data, int data_len, int fit);
void parse_optional_field(inventory_configuration_data *inv_data,
    const char *data, int data_len);
void parse_inventory_field(inventory_configuration_data *inv_data,
    const char *suffix, const char *data, int data_len, int fit);

/* Fill public config from internal data */
void fill_inventory_filter_common(obs_inventory_configuration *config,
    inventory_configuration_data *inv_data);
void fill_inventory_destination_common(obs_inventory_configuration *config,
    inventory_configuration_data *inv_data);
void fill_inventory_optional_fields_common(obs_inventory_configuration *config,
    inventory_configuration_data *inv_data);
void fill_inventory_config_common(obs_inventory_configuration *config,
    inventory_configuration_data *inv_data);

/* Free public config dynamically allocated memory */
void free_inventory_config(obs_inventory_configuration *config);

/* Free internal inventory_configuration_data sub-structures */
void free_inventory_data_internal(inventory_configuration_data *inv_data);

#endif
