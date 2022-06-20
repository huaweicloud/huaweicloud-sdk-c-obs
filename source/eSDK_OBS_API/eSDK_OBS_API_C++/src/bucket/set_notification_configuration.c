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

static char* get_event_string_s3(obs_smn_event_enum event)
{
    switch (event)
    {
    case SMN_EVENT_OBJECT_CREATED_ALL:
        return "s3:ObjectCreated:*";
    case SMN_EVENT_OBJECT_CREATED_PUT:
        return "s3:ObjectCreated:Put";
    case SMN_EVENT_OBJECT_CREATED_POST:
        return "s3:ObjectCreated:Post";
    case SMN_EVENT_OBJECT_CREATED_COPY:
        return "s3:ObjectCreated:Copy";
    case SMN_EVENT_OBJECT_CREATED_COMPLETE_MULTIPART_UPLOAD:
        return "s3:ObjectCreated:CompleteMultipartUpload";
    case SMN_EVENT_OBJECT_REMOVED_ALL:
        return "s3:ObjectRemoved:*";
    case SMN_EVENT_OBJECT_REMOVED_DELETE:
        return "s3:ObjectRemoved:Delete";
    case SMN_EVENT_OBJECT_REMOVED_DELETE_MARKER_CREATED:
        return "s3:ObjectRemoved:DeleteMarkerCreated";
    case SMN_EVENT_REDUCED_REDUNDANCY_LOST_OBJECT:
        return "s3:ReducedRedundancyLostObject";
    default:
        return NULL;
    }
}

static char* get_event_string_obs(obs_smn_event_enum event)
{
    switch (event)
    {
    case SMN_EVENT_OBJECT_CREATED_ALL:
        return "ObjectCreated:*";
    case SMN_EVENT_OBJECT_CREATED_PUT:
        return "ObjectCreated:Put";
    case SMN_EVENT_OBJECT_CREATED_POST:
        return "ObjectCreated:Post";
    case SMN_EVENT_OBJECT_CREATED_COPY:
        return "ObjectCreated:Copy";
    case SMN_EVENT_OBJECT_CREATED_COMPLETE_MULTIPART_UPLOAD:
        return "ObjectCreated:CompleteMultipartUpload";
    case SMN_EVENT_OBJECT_REMOVED_ALL:
        return "ObjectRemoved:*";
    case SMN_EVENT_OBJECT_REMOVED_DELETE:
        return "ObjectRemoved:Delete";
    case SMN_EVENT_OBJECT_REMOVED_DELETE_MARKER_CREATED:
        return "ObjectRemoved:DeleteMarkerCreated";
    case SMN_EVENT_REDUCED_REDUNDANCY_LOST_OBJECT:
        return "ReducedRedundancyLostObject";
    default:
        return NULL;
    }
}

static char* get_filter_rule_string(obs_smn_filter_rule_enum rule_name)
{
    switch (rule_name)
    {
    case OBS_SMN_FILTER_PREFIX:
        return "prefix";
    case OBS_SMN_FILTER_SUFFIX:
        return "suffix";
    default:
        return NULL;
    }
}

static obs_status set_notification_quest_xml_s3(obs_smn_notification_configuration* notification_conf,
    set_notification_data *sncData)
{
    unsigned int uiIdx = 0;
    (void)add_xml_element(sncData->doc, &sncData->doc_len, "NotificationConfiguration", NULL,
        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
    for (uiIdx = 0; uiIdx < notification_conf->topic_conf_num; uiIdx++)
    {
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "TopicConfiguration", NULL,
            NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "Topic",
            notification_conf->topic_conf[uiIdx].topic, NEED_FORMALIZE, ADD_NAME_CONTENT);
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "Id",
            notification_conf->topic_conf[uiIdx].id, NEED_FORMALIZE, ADD_NAME_CONTENT);
        unsigned int uiIdxEvent = 0;
        for (; uiIdxEvent < notification_conf->topic_conf[uiIdx].event_num; uiIdxEvent++)
        {
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Event",
                get_event_string_s3(notification_conf->topic_conf[uiIdx].event[uiIdxEvent]),
                NEED_FORMALIZE, ADD_NAME_CONTENT);
        }

        if (0 < notification_conf->topic_conf[uiIdx].filter_rule_num)
        {
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Filter", NULL,
                NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "S3Key", NULL,
                NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
            unsigned int uiIdxFilter = 0;
            for (; uiIdxFilter < notification_conf->topic_conf[uiIdx].filter_rule_num; uiIdxFilter++)
            {
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "FilterRule", NULL,
                    NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "Name", get_filter_rule_string(
                    notification_conf->topic_conf[uiIdx].filter_rule[uiIdxFilter].name),
                    NEED_FORMALIZE, ADD_NAME_CONTENT);
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "Value",
                    notification_conf->topic_conf[uiIdx].filter_rule[uiIdxFilter].value,
                    NEED_FORMALIZE, ADD_NAME_CONTENT);
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "FilterRule", NULL,
                    NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);

            }
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "S3Key", NULL,
                NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Filter", NULL,
                NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
        }


        (void)add_xml_element(sncData->doc, &sncData->doc_len, "TopicConfiguration", NULL,
            NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    }

    (void)add_xml_element(sncData->doc, &sncData->doc_len, "NotificationConfiguration", NULL,
        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    return OBS_STATUS_OK;
}

static obs_status set_notification_quest_xml_obs(obs_smn_notification_configuration* notification_conf,
    set_notification_data *sncData)
{
    unsigned int uiIdx = 0;
    (void)add_xml_element(sncData->doc, &sncData->doc_len, "NotificationConfiguration", NULL,
        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
    for (uiIdx = 0; uiIdx < notification_conf->topic_conf_num; uiIdx++)
    {
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "TopicConfiguration", NULL,
            NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "Id",
            notification_conf->topic_conf[uiIdx].id, NEED_FORMALIZE, ADD_NAME_CONTENT);
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "Topic",
            notification_conf->topic_conf[uiIdx].topic, NEED_FORMALIZE, ADD_NAME_CONTENT);
        unsigned int uiIdxEvent = 0;
        for (; uiIdxEvent < notification_conf->topic_conf[uiIdx].event_num; uiIdxEvent++)
        {
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Event",
                get_event_string_obs(notification_conf->topic_conf[uiIdx].event[uiIdxEvent]),
                NEED_FORMALIZE, ADD_NAME_CONTENT);
        }

        if (0 < notification_conf->topic_conf[uiIdx].filter_rule_num)
        {
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Filter", NULL,
                NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Object", NULL,
                NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
            unsigned int uiIdxFilter = 0;
            for (; uiIdxFilter < notification_conf->topic_conf[uiIdx].filter_rule_num; uiIdxFilter++)
            {
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "FilterRule", NULL,
                    NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "Name", get_filter_rule_string(
                    notification_conf->topic_conf[uiIdx].filter_rule[uiIdxFilter].name),
                    NEED_FORMALIZE, ADD_NAME_CONTENT);
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "Value",
                    notification_conf->topic_conf[uiIdx].filter_rule[uiIdxFilter].value,
                    NEED_FORMALIZE, ADD_NAME_CONTENT);
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "FilterRule", NULL,
                    NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);

            }
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Object", NULL,
                NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Filter", NULL,
                NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
        }


        (void)add_xml_element(sncData->doc, &sncData->doc_len, "TopicConfiguration", NULL,
            NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    }

    (void)add_xml_element(sncData->doc, &sncData->doc_len, "NotificationConfiguration", NULL,
        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    return OBS_STATUS_OK;
}

static obs_status set_notification_quest_xml(obs_smn_notification_configuration* notification_conf,
    set_notification_data *sncData, obs_use_api use_api)
{
    if (use_api == OBS_USE_API_S3) {
        return set_notification_quest_xml_s3(notification_conf, sncData);
    }
    else {
        return set_notification_quest_xml_obs(notification_conf, sncData);
    }
}


static set_notification_data* init_set_notification_data(
    obs_smn_notification_configuration* notification_conf, obs_response_handler *handler,
    void *callback_data, obs_use_api use_api)
{
    obs_status ret_status = OBS_STATUS_OK;
    set_notification_data *sncData = NULL;

    sncData = (set_notification_data *)malloc(sizeof(set_notification_data));
    if (!sncData) {
        COMMLOG(OBS_LOGERROR, "malloc cors config data failed.");
        return NULL;
    }
    memset_s(sncData, sizeof(set_notification_data), 0, sizeof(set_notification_data));

    sncData->response_complete_callback = handler->complete_callback;
    sncData->response_properties_callback = handler->properties_callback;
    sncData->callback_data = callback_data;
    sncData->doc_len = 0;
    sncData->doc_bytes_written = 0;

    ret_status = set_notification_quest_xml(notification_conf, sncData, use_api);
    if (OBS_STATUS_OK != ret_status || sncData->doc_len <= 0)
    {
        free(sncData);
        sncData = NULL;
        return NULL;
    }
    COMMLOG(OBS_LOGERROR, "request xml: %s.", sncData->doc);

    return sncData;
}


static int set_notification_data_callback(int buffer_size, char *buffer,
    void *callback_data)
{
    set_cors_config_data *sncData = (set_cors_config_data *)callback_data;

    if (!sncData->doc_len)
    {
        return 0;
    }

    int remaining = (sncData->doc_len - sncData->doc_bytes_written);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy)
    {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(sncData->doc[sncData->doc_bytes_written]), toCopy);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "set_notification_data_callback: memcpy_s failed!\n");
        return 0;
    }

    sncData->doc_bytes_written += toCopy;

    return toCopy;
}

static obs_status set_notification_properties_callback(
    const obs_response_properties *response_properties, void *callback_data)
{
    set_notification_data *sncData = (set_notification_data *)callback_data;
    if (sncData->response_properties_callback)
    {
        return (*(sncData->response_properties_callback))(response_properties,
            sncData->callback_data);
    }

    return OBS_STATUS_OK;
}


static void set_notification_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_notification_data *sncData = (set_notification_data *)callback_data;

    (*(sncData->response_complete_callback))(request_status, obs_error_info, sncData->callback_data);

    CHECK_NULL_FREE(sncData);
    sncData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


void set_notification_configuration(const obs_options *options,
    obs_smn_notification_configuration* notification_conf, obs_response_handler *handler,
    void *callback_data)
{
    request_params          params;
    set_notification_data   *sncData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "set_notification_configuration start !");

    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }

    if (!notification_conf)
    {
        COMMLOG(OBS_LOGERROR, "set_notification faied, notification_conf is null.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }

    sncData = init_set_notification_data(notification_conf, handler, callback_data, use_api);
    if (!sncData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc sncData failed.");
        return;
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &set_notification_properties_callback;
    params.toObsCallback = &set_notification_data_callback;
    params.complete_callback = &set_notification_complete_callback;
    params.toObsCallbackTotalSize = sncData->doc_len;
    params.callback_data = sncData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "notification";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_notification_configuration finish.");
}