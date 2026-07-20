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
#include "request_util.h"

/**
 * 懒分配destination结构体
 */
void ensure_destination_data(inventory_configuration_data *inv_data)
{
    if (!inv_data->destination)
    {
        inventory_destination_data *dest_data = (inventory_destination_data*)malloc(sizeof(inventory_destination_data));
        if (dest_data)
        {
            memset_s(dest_data, sizeof(inventory_destination_data), 0, sizeof(inventory_destination_data));
            inv_data->destination = dest_data;
        }
    }
}

/**
 * 懒分配filter结构体
 */
void ensure_filter_data(inventory_configuration_data *inv_data)
{
    if (!inv_data->filter)
    {
        inventory_filter_data *filter_data = (inventory_filter_data*)malloc(sizeof(inventory_filter_data));
        if (filter_data)
        {
            memset_s(filter_data, sizeof(inventory_filter_data), 0, sizeof(inventory_filter_data));
            inv_data->filter = filter_data;
        }
    }
}

/**
 * 懒分配optional_fields结构体
 */
void ensure_optional_fields_data(inventory_configuration_data *inv_data)
{
    if (!inv_data->optional_fields)
    {
        inventory_optional_fields_data *fields_data = (inventory_optional_fields_data*)malloc(sizeof(inventory_optional_fields_data));
        if (fields_data)
        {
            memset_s(fields_data, sizeof(inventory_optional_fields_data), 0, sizeof(inventory_optional_fields_data));
            inv_data->optional_fields = fields_data;
        }
    }
}

/**
 * 解析Filter/Prefix子元素
 */
void parse_filter_prefix(inventory_configuration_data *inv_data,
    const char *data, int data_len, int fit)
{
    ensure_filter_data(inv_data);
    if (inv_data->filter)
    {
        string_buffer_append(inv_data->filter->prefix, data, data_len, fit);
    }
}

/**
 * 解析Destination子元素（Bucket/Prefix）
 */
void parse_destination_element(inventory_configuration_data *inv_data,
    const char *suffix, const char *data, int data_len, int fit)
{
    ensure_destination_data(inv_data);
    if (!inv_data->destination)
    {
        return;
    }
    if (!strcmp(suffix, "Bucket"))
    {
        string_buffer_append(inv_data->destination->bucket, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Prefix"))
    {
        string_buffer_append(inv_data->destination->prefix, data, data_len, fit);
    }
}

/**
 * 解析OptionalFields/Field子元素
 */
void parse_optional_field(inventory_configuration_data *inv_data,
    const char *data, int data_len)
{
    ensure_optional_fields_data(inv_data);
    if (inv_data->optional_fields && inv_data->optional_fields->field_count < MAX_INVENTORY_FIELDS)
    {
        char *field = (char *)malloc(data_len + 1);
        if (field)
        {
            if (memcpy_s(field, data_len + 1, data, data_len) != EOK) {
                free(field);
                return;
            }
            field[data_len] = '\0';
            inv_data->optional_fields->fields[inv_data->optional_fields->field_count] = field;
            inv_data->optional_fields->field_count++;
        }
    }
}

/**
 * 根据后缀分派字段解析
 */
void parse_inventory_field(inventory_configuration_data *inv_data,
    const char *suffix, const char *data, int data_len, int fit)
{
    if (!strcmp(suffix, "Id"))
    {
        string_buffer_append(inv_data->id, data, data_len, fit);
    }
    else if (!strcmp(suffix, "IsEnabled"))
    {
        string_buffer_append(inv_data->is_enabled, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Schedule/Frequency"))
    {
        string_buffer_append(inv_data->schedule, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Destination/Format"))
    {
        string_buffer_append(inv_data->format, data, data_len, fit);
    }
    else if (!strcmp(suffix, "IncludedObjectVersions"))
    {
        string_buffer_append(inv_data->included_object_versions, data, data_len, fit);
    }
    else if (!strcmp(suffix, "Filter/Prefix"))
    {
        parse_filter_prefix(inv_data, data, data_len, fit);
    }
    else if (!strncmp(suffix, "Destination/", 12))
    {
        parse_destination_element(inv_data, suffix + 12, data, data_len, fit);
    }
    else if (!strcmp(suffix, "OptionalFields/Field"))
    {
        parse_optional_field(inv_data, data, data_len);
    }
}

/**
 * 填充filter到公共结构体
 */
void fill_inventory_filter_common(obs_inventory_configuration *config,
    inventory_configuration_data *inv_data)
{
    if (!inv_data->filter || inv_data->filter->prefix[0] == '\0')
    {
        return;
    }
    config->filter = (obs_inventory_filter*)malloc(sizeof(obs_inventory_filter));
    if (config->filter)
    {
        config->filter->prefix = strdup(inv_data->filter->prefix);
    }
}

/**
 * 填充destination到公共结构体
 */
void fill_inventory_destination_common(obs_inventory_configuration *config,
    inventory_configuration_data *inv_data)
{
    if (!inv_data->destination)
    {
        return;
    }
    config->destination = (obs_inventory_destination*)malloc(sizeof(obs_inventory_destination));
    if (!config->destination)
    {
        return;
    }
    memset_s(config->destination, sizeof(obs_inventory_destination), 0, sizeof(obs_inventory_destination));
    if (inv_data->destination->bucket[0] != '\0')
    {
        config->destination->bucket = strdup(inv_data->destination->bucket);
    }
    if (inv_data->destination->prefix[0] != '\0')
    {
        config->destination->prefix = strdup(inv_data->destination->prefix);
    }
}

/**
 * 填充optional_fields到公共结构体
 */
void fill_inventory_optional_fields_common(obs_inventory_configuration *config,
    inventory_configuration_data *inv_data)
{
    unsigned int j;
    if (!inv_data->optional_fields || inv_data->optional_fields->field_count == 0)
    {
        return;
    }
    config->optional_fields = (obs_inventory_optional_fields*)malloc(sizeof(obs_inventory_optional_fields));
    if (!config->optional_fields)
    {
        return;
    }
    config->optional_fields->field_count = inv_data->optional_fields->field_count;
    config->optional_fields->fields = (char**)malloc(sizeof(char*) * config->optional_fields->field_count);
    if (config->optional_fields->fields)
    {
        for (j = 0; j < config->optional_fields->field_count; ++j)
        {
            if (inv_data->optional_fields->fields[j])
            {
                config->optional_fields->fields[j] = strdup(inv_data->optional_fields->fields[j]);
            }
        }
    }
}

/**
 * 填充完整的清单配置到公共结构体
 */
void fill_inventory_config_common(obs_inventory_configuration *config,
    inventory_configuration_data *inv_data)
{
    if (inv_data->id[0] != '\0')
    {
        config->id = strdup(inv_data->id);
    }
    config->is_enabled = (strcmp(inv_data->is_enabled, "true") == 0);
    if (inv_data->schedule[0] != '\0')
    {
        config->schedule = strdup(inv_data->schedule);
    }
    if (inv_data->format[0] != '\0')
    {
        config->format = strdup(inv_data->format);
    }
    if (inv_data->included_object_versions[0] != '\0')
    {
        config->included_object_versions = strdup(inv_data->included_object_versions);
    }

    fill_inventory_filter_common(config, inv_data);
    fill_inventory_destination_common(config, inv_data);
    fill_inventory_optional_fields_common(config, inv_data);
}

/**
 * 释放obs_inventory_configuration结构体中动态分配的内存
 */
void free_inventory_config(obs_inventory_configuration *config)
{
    unsigned int j;
    if (!config) return;

    CHECK_NULL_FREE(config->id);
    CHECK_NULL_FREE(config->schedule);
    CHECK_NULL_FREE(config->format);
    CHECK_NULL_FREE(config->included_object_versions);

    if (config->filter)
    {
        CHECK_NULL_FREE(config->filter->prefix);
        CHECK_NULL_FREE(config->filter);
    }
    if (config->destination)
    {
        CHECK_NULL_FREE(config->destination->bucket);
        CHECK_NULL_FREE(config->destination->prefix);
        CHECK_NULL_FREE(config->destination);
    }
    if (config->optional_fields)
    {
        if (config->optional_fields->fields)
        {
            for (j = 0; j < config->optional_fields->field_count; ++j)
            {
                CHECK_NULL_FREE(config->optional_fields->fields[j]);
            }
            CHECK_NULL_FREE(config->optional_fields->fields);
        }
        CHECK_NULL_FREE(config->optional_fields);
    }
}

/**
 * 释放inventory_configuration_data内部子结构体
 */
void free_inventory_data_internal(inventory_configuration_data *inv_data)
{
    if (!inv_data)
    {
        return;
    }

    CHECK_NULL_FREE(inv_data->filter);

    if (inv_data->destination)
    {
        CHECK_NULL_FREE(inv_data->destination);
    }

    if (inv_data->optional_fields)
    {
        unsigned int j;
        for (j = 0; j < inv_data->optional_fields->field_count; j++)
        {
            CHECK_NULL_FREE(inv_data->optional_fields->fields[j]);
        }
        CHECK_NULL_FREE(inv_data->optional_fields);
    }
}
