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
#include "bucket.h"
#include "request_util.h"
#include <openssl/md5.h> 

int update_bucket_common_data_callback(int buffer_size, char *buffer,
    void *callback_data)
{
    update_bucket_common_data *bucket_data = (update_bucket_common_data *)callback_data;

    if (!bucket_data->docLen) {
        return 0;
    }

    int remaining = (bucket_data->docLen - bucket_data->docBytesWritten);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy) {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(bucket_data->doc[bucket_data->docBytesWritten]), toCopy);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "update_bucket_common_data_callback: memcpy_s failed!\n");
        return 0;
    }

    bucket_data->docBytesWritten += toCopy;

    return toCopy;
}

obs_status update_bucket_common_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    update_bucket_common_data *bucket_data = (update_bucket_common_data *)callback_data;

    if (bucket_data->properties_callback)
    {
        return (*(bucket_data->properties_callback))
            (response_properties, bucket_data->callback_data);
    }
    return OBS_STATUS_OK;
}

void update_bucket_common_complete_callback(obs_status status,
    const obs_error_details *error_details,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    update_bucket_common_data *bucket_data = (update_bucket_common_data *)callback_data;
    check_before_complete(bucket_data->complete_callback,
        status, error_details, bucket_data->callback_data, __FUNCTION__, __LINE__);

    free(bucket_data);
    bucket_data = NULL;
    return;
}

void update_bucket_common_complete_callback_no_free(obs_status status,
    const obs_error_details *error_details,
    void *callback_data)
{
    COMMLOG(OBS_LOGDEBUG, "Enter %s successfully !", __FUNCTION__);
    update_bucket_common_data *bucket_data = (update_bucket_common_data *)callback_data;
	check_before_complete(bucket_data->complete_callback,
		status, error_details, bucket_data->callback_data, __FUNCTION__, __LINE__);
    return;
}

obs_status append_xml_document(int *xml_document_len_return, char *xml_document,
    int xml_document_buffer_size, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int size = vsnprintf_s(&(xml_document[*xml_document_len_return]),
        xml_document_buffer_size - *xml_document_len_return,
        xml_document_buffer_size - *xml_document_len_return - 1,
        fmt, args);
    va_end(args);

    if (size < 0)
    {
        return OBS_STATUS_InternalError;
    }

    *xml_document_len_return += size;
    if (*xml_document_len_return >= xml_document_buffer_size) {
        return OBS_STATUS_XmlDocumentTooLarge;
    }
    return OBS_STATUS_OK;
}

obs_status set_common_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    set_common_data *common_data = (set_common_data *)callback_data;

    if (common_data->response_properties_callback)
    {
        return (*(common_data->response_properties_callback))
            (response_properties, common_data->callback_data);
    }
    return OBS_STATUS_OK;
}

void set_common_complete_callback(obs_status status,
    const obs_error_details *error_details, void *callback_data)
{
    set_common_data *common_data = NULL;

    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    common_data = (set_common_data *)callback_data;
    (void)(*(common_data->response_complete_callback))
        (status, error_details, common_data->callback_data);
    free(common_data);
    common_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

int set_common_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    int remaining = 0;
    int ret = 0;
    set_common_data *common_data = NULL;

    common_data = (set_common_data *)callback_data;
    remaining = (common_data->xml_document_len -
        common_data->xml_document_bytes_written);

    ret = buffer_size > remaining ? remaining : buffer_size;
    if (!ret)
    {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(common_data->xml_document
        [common_data->xml_document_bytes_written]), ret);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "set_common_data_callback: memcpy_s failed!\n");
        return 0;
    }

    common_data->xml_document_bytes_written += ret;

    return ret;
}

obs_smn_event_enum get_event_enum_s3(const char* event_string)
{
    if (!strcmp(event_string, "s3:ObjectCreated:*"))
    {
        return SMN_EVENT_OBJECT_CREATED_ALL;
    }
    else if (!strcmp(event_string, "s3:ObjectCreated:Put"))
    {
        return SMN_EVENT_OBJECT_CREATED_PUT;
    }
    else if (!strcmp(event_string, "s3:ObjectCreated:Post"))
    {
        return SMN_EVENT_OBJECT_CREATED_POST;
    }
    else if (!strcmp(event_string, "s3:ObjectCreated:Copy"))
    {
        return SMN_EVENT_OBJECT_CREATED_COPY;
    }
    else if (!strcmp(event_string, "s3:ObjectCreated:CompleteMultipartUpload"))
    {
        return SMN_EVENT_OBJECT_CREATED_COMPLETE_MULTIPART_UPLOAD;
    }
    else if (!strcmp(event_string, "s3:ObjectRemoved:*"))
    {
        return SMN_EVENT_OBJECT_REMOVED_ALL;
    }
    else if (!strcmp(event_string, "s3:ObjectRemoved:Delete"))
    {
        return SMN_EVENT_OBJECT_REMOVED_DELETE;
    }
    else if (!strcmp(event_string, "s3:ObjectRemoved:DeleteMarkerCreated"))
    {
        return SMN_EVENT_OBJECT_REMOVED_DELETE_MARKER_CREATED;
    }
    else if (!strcmp(event_string, "s3:ReducedRedundancyLostObject"))
    {
        return SMN_EVENT_REDUCED_REDUNDANCY_LOST_OBJECT;
    }

    return SMN_EVENT_NULL;
}

obs_smn_event_enum get_event_enum_obs(const char* event_string)
{
    if (!strcmp(event_string, "ObjectCreated:*"))
    {
        return SMN_EVENT_OBJECT_CREATED_ALL;
    }
    else if (!strcmp(event_string, "ObjectCreated:Put"))
    {
        return SMN_EVENT_OBJECT_CREATED_PUT;
    }
    else if (!strcmp(event_string, "ObjectCreated:Post"))
    {
        return SMN_EVENT_OBJECT_CREATED_POST;
    }
    else if (!strcmp(event_string, "ObjectCreated:Copy"))
    {
        return SMN_EVENT_OBJECT_CREATED_COPY;
    }
    else if (!strcmp(event_string, "ObjectCreated:CompleteMultipartUpload"))
    {
        return SMN_EVENT_OBJECT_CREATED_COMPLETE_MULTIPART_UPLOAD;
    }
    else if (!strcmp(event_string, "ObjectRemoved:*"))
    {
        return SMN_EVENT_OBJECT_REMOVED_ALL;
    }
    else if (!strcmp(event_string, "ObjectRemoved:Delete"))
    {
        return SMN_EVENT_OBJECT_REMOVED_DELETE;
    }
    else if (!strcmp(event_string, "ObjectRemoved:DeleteMarkerCreated"))
    {
        return SMN_EVENT_OBJECT_REMOVED_DELETE_MARKER_CREATED;
    }
    else if (!strcmp(event_string, "ReducedRedundancyLostObject"))
    {
        return SMN_EVENT_REDUCED_REDUNDANCY_LOST_OBJECT;
    }

    return SMN_EVENT_NULL;
}

obs_smn_filter_rule_enum get_filter_rule_enum(const char* rule_string)
{
    if (!strcmp(rule_string, "prefix"))
    {
        return OBS_SMN_FILTER_PREFIX;
    }
    else if (!strcmp(rule_string, "suffix"))
    {
        return OBS_SMN_FILTER_SUFFIX;
    }

    return OBS_SMN_FILTER_NULL;
}

void initialize_list_common_prefixes(list_common_prefixes* common_prefixes)
{
    string_buffer_initialize(common_prefixes->prefix);
}

void get_tagging_xml_callback_existdata(get_bucket_tagging_data* tagging_data,
    const char* element_path, const char* data, int data_len)
{
    int fit = 1;
    if (tagging_data->tagging_count < MAX_NUM_TAGGING)
    {
        if (!strcmp(element_path, "Tagging/TagSet/Tag/Key"))
        {
            string_buffer_append(tagging_data->tagging_list[tagging_data->tagging_count].key,
                data, data_len, fit);
        }
        else if (!strcmp(element_path, "Tagging/TagSet/Tag/Value"))
        {
            string_buffer_append(tagging_data->tagging_list[tagging_data->tagging_count].value,
                data, data_len, fit);
        }
    }

    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
}

obs_status get_tagging_xml_callback_nodata(get_bucket_tagging_data* tagging_data,
    const char* element_path)
{
    if (!strcmp(element_path, "Tagging/TagSet/Tag"))
    {
        tagging_data->tagging_count++;
        if (tagging_data->tagging_count > MAX_NUM_TAGGING)
        {
            COMMLOG(OBS_LOGERROR, "etag number exceed the max[10]!");
            return OBS_STATUS_InternalError;
        }
        else if (tagging_data->tagging_count == MAX_NUM_TAGGING)
        {
            COMMLOG(OBS_LOGINFO, "already get the max[10] tags!");
            return OBS_STATUS_OK;
        }
        else
        {
            memset_s(&tagging_data->tagging_list[tagging_data->tagging_count], sizeof(tagging_kv), 0, sizeof(tagging_kv));
            return OBS_STATUS_OK;
        }
    }
    return OBS_STATUS_OK;
}

obs_status get_tagging_xml_callback(const char *element_path,
    const char *data, int data_len, void *callback_data)
{
    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *)callback_data;

    if (data)
    {
        get_tagging_xml_callback_existdata(tagging_data, element_path, data, data_len);
    }
    else
    {
        return get_tagging_xml_callback_nodata(tagging_data, element_path);
    }

    return OBS_STATUS_OK;
}

obs_status get_tagging_properties_callback(
    const obs_response_properties *response_properties,
    void *callback_data)
{
    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *)callback_data;
    if (tagging_data->response_properties_callback)
    {
        return (*(tagging_data->response_properties_callback))
            (response_properties, tagging_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status make_list_tagging_callback(get_bucket_tagging_data *tagging_data)
{
    obs_status status = OBS_STATUS_OK;
    obs_name_value *tagging_list = NULL;
    unsigned int tagging_count = 0;
    unsigned int i = 0;
    if (tagging_data->tagging_count > 0)
    {
        tagging_list = (obs_name_value*)malloc(sizeof(obs_name_value) * tagging_data->tagging_count);
        if (NULL == tagging_list)
        {
            COMMLOG(OBS_LOGERROR, "malloc tagging_list failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(tagging_list, sizeof(obs_name_value) * tagging_data->tagging_count,
            0, sizeof(obs_name_value) * tagging_data->tagging_count);

        tagging_count = tagging_data->tagging_count;
        for (i = 0; i < tagging_count; i++)
        {
            obs_name_value *content_dest = &(tagging_list[i]);
            tagging_kv *content_src = &(tagging_data->tagging_list[i]);
            content_dest->name = content_src->key;
            content_dest->value = content_src->value;
        }
    }

    if (tagging_data->response_tagging_list_callback)
    {
        status = (*(tagging_data->response_tagging_list_callback))
            (tagging_count, tagging_list, tagging_data->callback_data);
    }
    CHECK_NULL_FREE(tagging_list);

    return status;
}

void get_tagging_complete_callback(obs_status status,
    const obs_error_details *error_details,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *)callback_data;

    if (tagging_data->tagging_count)
    {
        obs_status result = make_list_tagging_callback(tagging_data);
        if (result != OBS_STATUS_OK)
        {
            COMMLOG(OBS_LOGERROR, "make_list_tagging_callback failed!");
        }
    }
    else if (tagging_data->response_tagging_list_callback)
    {
        (*(tagging_data->response_tagging_list_callback))(0, NULL, tagging_data->callback_data);
    }

    (void)(*(tagging_data->response_complete_callback))(status, error_details,
        tagging_data->callback_data);
    simplexml_deinitialize(&(tagging_data->simpleXml));

    free(tagging_data);
    tagging_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

obs_status get_tagging_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *)callback_data;
    return simplexml_add(&(tagging_data->simpleXml), buffer, buffer_size);
}

obs_status generate_tagging_xml_document(obs_name_value * tagging_list, unsigned int tag_number,
    int *xml_document_len_return, char *xml_document, int xml_document_buffer_size)
{
    *xml_document_len_return = 0;
    unsigned int i = 0;
    char *key_utf8 = NULL;
    char *value_utf8 = NULL;
    obs_status ret = OBS_STATUS_OK;

    ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
    ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "%s", "<Tagging xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\"><TagSet>");

    for (i = 0; i < tag_number; i++)
    {
        key_utf8 = string_To_UTF8(tagging_list[i].name);
        value_utf8 = string_To_UTF8(tagging_list[i].value);
        ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "<Tag><Key>%s</Key><Value>%s</Value></Tag>", key_utf8, value_utf8);
        CHECK_NULL_FREE(key_utf8);
        CHECK_NULL_FREE(value_utf8);
    }

    ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "%s", "</TagSet></Tagging>");

    return ret;
}