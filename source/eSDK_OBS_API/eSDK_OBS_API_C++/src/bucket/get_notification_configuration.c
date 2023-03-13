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

static obs_status malloc_smn_data_s3(const char *element_path, get_smn_data *smn_data)
{
    int nTopicConfIdx = smn_data->notification_conf.topic_conf_num;

    if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/S3Key/FilterRule"))
    {
        smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num++;
        int alloc_size = (sizeof(obs_smn_filter_rule)) *
            (smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num + 1);
        obs_smn_filter_rule* tmpFilter = (obs_smn_filter_rule*)malloc(alloc_size);
        if (NULL == tmpFilter)
        {
            COMMLOG(OBS_LOGERROR, "malloc obs_smn_filter_rule failed in malloc_smn_data_s3.");
            return OBS_STATUS_OutOfMemory;
        }

        memset_s(tmpFilter, alloc_size, 0, alloc_size);
        errno_t err = EOK;
        err = memcpy_s(tmpFilter, alloc_size, smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule,
            sizeof(obs_smn_filter_rule) *
            smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num);
        free(smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule);
        smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule = tmpFilter;

        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "malloc_smn_data_s3: memcpy_s failed!\n");
            return OBS_STATUS_OutOfMemory;
        }
    }
    else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration"))
    {
        smn_data->notification_conf.topic_conf_num++;
        int alloc_topic_size = sizeof(obs_smn_topic_configuration) *
            (smn_data->notification_conf.topic_conf_num + 1);
        obs_smn_topic_configuration* tmpTopicConf =
            (obs_smn_topic_configuration*)malloc(alloc_topic_size);
        if (NULL == tmpTopicConf)
        {
            COMMLOG(OBS_LOGERROR, "malloc smn topic failed in malloc_smn_data_s3.");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(tmpTopicConf, alloc_topic_size, 0, alloc_topic_size);

        int nTmpIdx = smn_data->notification_conf.topic_conf_num;
        (tmpTopicConf + nTmpIdx)->filter_rule = (obs_smn_filter_rule*)malloc(sizeof(obs_smn_filter_rule));
        if (!(tmpTopicConf + nTmpIdx)->filter_rule)
        {
            COMMLOG(OBS_LOGERROR, "malloc filter_rule failed in malloc_smn_data_s3.");
            CHECK_NULL_FREE(tmpTopicConf);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s((tmpTopicConf + nTmpIdx)->filter_rule, sizeof(obs_smn_filter_rule), 0,
            sizeof(obs_smn_filter_rule));

        (tmpTopicConf + nTmpIdx)->event = (obs_smn_event_enum*)malloc(sizeof(obs_smn_event_enum));
        if (!(tmpTopicConf + nTmpIdx)->event)
        {
            COMMLOG(OBS_LOGERROR, "malloc notify topic event failed in malloc_smn_data_s3.");
            CHECK_NULL_FREE((tmpTopicConf + nTmpIdx)->filter_rule);
            CHECK_NULL_FREE(tmpTopicConf);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s((tmpTopicConf + nTmpIdx)->event, sizeof(obs_smn_event_enum), 0, sizeof(obs_smn_event_enum));

        errno_t err = EOK;
        err = memcpy_s(tmpTopicConf, alloc_topic_size, smn_data->notification_conf.topic_conf,
            sizeof(obs_smn_topic_configuration) * nTmpIdx);
        free(smn_data->notification_conf.topic_conf);
        smn_data->notification_conf.topic_conf = tmpTopicConf;

        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "malloc_smn_data_s3: memcpy_s failed!\n");
            return OBS_STATUS_OutOfMemory;
        }
    }
    else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Event"))
    {
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num++;
        int alloc_event_size = sizeof(obs_smn_event_enum) *
            (smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num + 1);
        obs_smn_event_enum* tmpEvent = (obs_smn_event_enum*)malloc(alloc_event_size);
        if (NULL == tmpEvent)
        {
            COMMLOG(OBS_LOGERROR, "malloc notify conf failed in malloc_smn_data_s3!");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(tmpEvent, alloc_event_size, 0, alloc_event_size);

        errno_t err = EOK;
        err = memcpy_s(tmpEvent, alloc_event_size,
            smn_data->notification_conf.topic_conf[nTopicConfIdx].event, sizeof(obs_smn_event_enum) *
            (smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num));
        free(smn_data->notification_conf.topic_conf[nTopicConfIdx].event);
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event = tmpEvent;

        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "malloc_smn_data_s3: memcpy_s failed!\n");
            return OBS_STATUS_OutOfMemory;
        }
    }

    return OBS_STATUS_OK;
}

static obs_status malloc_smn_data_obs(const char *element_path, get_smn_data *smn_data)
{
    int nTopicConfIdx = smn_data->notification_conf.topic_conf_num;
    errno_t err = EOK;

    if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/Object/FilterRule"))
    {
        smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num++;
        int alloc_size = (sizeof(obs_smn_filter_rule)) *
            (smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num + 1);
        obs_smn_filter_rule* tmpFilter = (obs_smn_filter_rule*)malloc(alloc_size);
        if (NULL == tmpFilter)
        {
            COMMLOG(OBS_LOGERROR, "malloc obs_smn_filter_rule failed in malloc_smn_data_obs.");
            return OBS_STATUS_OutOfMemory;
        }

        memset_s(tmpFilter, alloc_size, 0, alloc_size);
        err = memcpy_s(tmpFilter, alloc_size, smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule,
            sizeof(obs_smn_filter_rule) *
            smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num);

        free(smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule);
        smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule = tmpFilter;

        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "Malloc_smn_data_obs: memcpy_s failed!\n");
            return OBS_STATUS_OutOfMemory;
        }
    }
    else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration"))
    {
        smn_data->notification_conf.topic_conf_num++;
        int alloc_topic_size = sizeof(obs_smn_topic_configuration) *
            (smn_data->notification_conf.topic_conf_num + 1);
        obs_smn_topic_configuration* tmpTopicConf =
            (obs_smn_topic_configuration*)malloc(alloc_topic_size);
        if (NULL == tmpTopicConf)
        {
            COMMLOG(OBS_LOGERROR, "malloc smn topic failed in malloc_smn_data_obs.");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(tmpTopicConf, alloc_topic_size, 0, alloc_topic_size);

        int nTmpIdx = smn_data->notification_conf.topic_conf_num;
        (tmpTopicConf + nTmpIdx)->filter_rule = (obs_smn_filter_rule*)malloc(sizeof(obs_smn_filter_rule));
        if (!(tmpTopicConf + nTmpIdx)->filter_rule)
        {
            COMMLOG(OBS_LOGERROR, "malloc filter_rule failed in malloc_smn_data_obs.");
            CHECK_NULL_FREE(tmpTopicConf);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s((tmpTopicConf + nTmpIdx)->filter_rule, sizeof(obs_smn_filter_rule), 0,
            sizeof(obs_smn_filter_rule));

        (tmpTopicConf + nTmpIdx)->event = (obs_smn_event_enum*)malloc(sizeof(obs_smn_event_enum));
        if (!(tmpTopicConf + nTmpIdx)->event)
        {
            COMMLOG(OBS_LOGERROR, "malloc notify topic event failed in malloc_smn_data_obs.");
            CHECK_NULL_FREE((tmpTopicConf + nTmpIdx)->filter_rule);
            CHECK_NULL_FREE(tmpTopicConf);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s((tmpTopicConf + nTmpIdx)->event, sizeof(obs_smn_event_enum), 0, sizeof(obs_smn_event_enum));

        err = memcpy_s(tmpTopicConf, alloc_topic_size, smn_data->notification_conf.topic_conf,
            sizeof(obs_smn_topic_configuration) * nTmpIdx);

        free(smn_data->notification_conf.topic_conf);
        smn_data->notification_conf.topic_conf = tmpTopicConf;

        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "Malloc_smn_data_obs: memcpy_s failed!\n");
            return OBS_STATUS_OutOfMemory;
        }
    }
    else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Event"))
    {
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num++;
        int alloc_event_size = sizeof(obs_smn_event_enum) *
            (smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num + 1);
        obs_smn_event_enum* tmpEvent = (obs_smn_event_enum*)malloc(alloc_event_size);
        if (NULL == tmpEvent)
        {
            COMMLOG(OBS_LOGERROR, "malloc notify conf failed in malloc_smn_data_obs!");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(tmpEvent, alloc_event_size, 0, alloc_event_size);

        err = memcpy_s(tmpEvent, alloc_event_size,
            smn_data->notification_conf.topic_conf[nTopicConfIdx].event, sizeof(obs_smn_event_enum) *
            (smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num));

        free(smn_data->notification_conf.topic_conf[nTopicConfIdx].event);
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event = tmpEvent;

        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "Malloc_smn_data_obs: memcpy_s failed!\n");
            return OBS_STATUS_OutOfMemory;
        }
    }

    return OBS_STATUS_OK;
}

static obs_status get_notification_xml_s3_callback(const char *element_path,
    const char *data, int data_len, void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *)callback_data;
    int nTopicConfIdx = smn_data->notification_conf.topic_conf_num;
    int nFilterRuleIdx = smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num;
    int nEventIdx = smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num;
    obs_status ret_status = OBS_STATUS_OK;

    if (data)
    {
        if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Topic"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].topic, data, data_len);
        }
        else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Id"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].id,
                data, data_len);
        }
        else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Event"))
        {
            char* str_event = NULL;
            malloc_buffer_append(str_event, data, data_len);
            smn_data->notification_conf.topic_conf[nTopicConfIdx].event[nEventIdx] =
                get_event_enum_s3(str_event);
            CHECK_NULL_FREE(str_event);
        }
        else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/S3Key/FilterRule/Name"))
        {
            char* str_name = NULL;
            malloc_buffer_append(str_name, data, data_len);
            smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule[nFilterRuleIdx].name =
                get_filter_rule_enum(str_name);
            CHECK_NULL_FREE(str_name);
        }
        else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/S3Key/FilterRule/Value"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule[nFilterRuleIdx].value,
                data, data_len);
        }
    }
    else
    {
        ret_status = malloc_smn_data_s3(element_path, smn_data);
        if (OBS_STATUS_OK != ret_status)
        {
            return ret_status;
        }
    }
    return OBS_STATUS_OK;
}

static obs_status get_notification_xml_obs_callback(const char *element_path,
    const char *data, int data_len, void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *)callback_data;
    int nTopicConfIdx = smn_data->notification_conf.topic_conf_num;
    int nFilterRuleIdx = smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num;
    int nEventIdx = smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num;
    obs_status ret_status = OBS_STATUS_OK;

    if (data)
    {
        if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Topic"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].topic, data, data_len);
        }
        else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Id"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].id,
                data, data_len);
        }
        else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Event"))
        {
            char* str_event = NULL;
            malloc_buffer_append(str_event, data, data_len);
            smn_data->notification_conf.topic_conf[nTopicConfIdx].event[nEventIdx] =
                get_event_enum_obs(str_event);
            CHECK_NULL_FREE(str_event);
        }
        else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/Object/FilterRule/Name"))
        {
            char* str_name = NULL;
            malloc_buffer_append(str_name, data, data_len);
            smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule[nFilterRuleIdx].name =
                get_filter_rule_enum(str_name);
            CHECK_NULL_FREE(str_name);
        }
        else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/Object/FilterRule/Value"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule[nFilterRuleIdx].value,
                data, data_len);
        }
    }
    else
    {
        ret_status = malloc_smn_data_obs(element_path, smn_data);
        if (OBS_STATUS_OK != ret_status)
        {
            return ret_status;
        }
    }
    return OBS_STATUS_OK;
}

static obs_status get_notification_xml_callback(const char *element_path,
    const char *data, int data_len, void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *)callback_data;
    if (smn_data->use_api == OBS_USE_API_S3) {
        return get_notification_xml_s3_callback(element_path, data, data_len, callback_data);
    }
    else {
        return get_notification_xml_obs_callback(element_path, data, data_len, callback_data);
    }
}

static get_smn_data* init_get_smn_data(obs_smn_handler *handler, void *callback_data, obs_use_api use_api)
{
    get_smn_data *smn_data = (get_smn_data *)malloc(sizeof(get_smn_data));
    if (!smn_data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc get_smn_data failed.");
        return NULL;
    }
    memset_s(smn_data, sizeof(get_smn_data), 0, sizeof(get_smn_data));

    smn_data->notification_conf.topic_conf =
        (obs_smn_topic_configuration*)malloc(sizeof(obs_smn_topic_configuration));
    if (!smn_data->notification_conf.topic_conf)
    {
        free(smn_data);
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc obs_smn_topic_configuration failed.");
        return NULL;
    }
    memset_s(smn_data->notification_conf.topic_conf, sizeof(obs_smn_topic_configuration), 0,
        sizeof(obs_smn_topic_configuration));
    smn_data->notification_conf.topic_conf_num = 0;

    smn_data->notification_conf.topic_conf->filter_rule =
        (obs_smn_filter_rule*)malloc(sizeof(obs_smn_filter_rule));
    if (!smn_data->notification_conf.topic_conf->filter_rule)
    {
        free(smn_data->notification_conf.topic_conf);
        free(smn_data);
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc obs_smn_filter_rule failed.");
        return NULL;
    }
    memset_s(smn_data->notification_conf.topic_conf->filter_rule, sizeof(obs_smn_filter_rule), 0,
        sizeof(obs_smn_filter_rule));
    smn_data->notification_conf.topic_conf->filter_rule_num = 0;


    smn_data->notification_conf.topic_conf->event =
        (obs_smn_event_enum*)malloc(sizeof(obs_smn_event_enum));
    if (!smn_data->notification_conf.topic_conf->event)
    {
        free(smn_data->notification_conf.topic_conf->filter_rule);
        free(smn_data->notification_conf.topic_conf);
        free(smn_data);
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc obs_smn_event_enum failed.");
        return NULL;
    }
    memset_s(smn_data->notification_conf.topic_conf->event, sizeof(obs_smn_event_enum), 0,
        sizeof(obs_smn_event_enum));
    smn_data->notification_conf.topic_conf->event_num = 0;
    smn_data->notification_conf.topic_conf->event[0] = SMN_EVENT_NULL;

    smn_data->use_api = use_api;
    simplexml_initialize(&(smn_data->simple_xml_info), &get_notification_xml_callback, smn_data);
    smn_data->response_properties_callback = handler->response_handler.properties_callback;
    smn_data->response_complete_callback = handler->response_handler.complete_callback;
    smn_data->get_smn_callback_func = handler->get_smn_callback_func;
    smn_data->callback_data = callback_data;

    return smn_data;
}


static obs_status get_notification_properties_callback(
    const obs_response_properties *response_properties, void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *)callback_data;
    if (smn_data->response_properties_callback)
    {
        return (*(smn_data->response_properties_callback))(response_properties,
            smn_data->callback_data);
    }

    return OBS_STATUS_OK;
}

static obs_status get_notification_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *)callback_data;

    return simplexml_add(&(smn_data->simple_xml_info), buffer, buffer_size);
}


static void get_notification_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    unsigned int topic_loop = 0;
    unsigned int filter_loop = 0;
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_smn_data *smn_data = (get_smn_data *)callback_data;
    if (OBS_STATUS_OK == request_status && smn_data->get_smn_callback_func)
    {
        request_status = (*(smn_data->get_smn_callback_func))(&smn_data->notification_conf,
            smn_data->callback_data);
    }

    (*(smn_data->response_complete_callback))(request_status, obs_error_info,
        smn_data->callback_data);

    for (topic_loop = 0; topic_loop < smn_data->notification_conf.topic_conf_num; topic_loop++)
    {
        CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].id);
        CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].topic);
        for (filter_loop = 0; filter_loop < smn_data->notification_conf.topic_conf[topic_loop].filter_rule_num; filter_loop++)
        {
            if(smn_data->notification_conf.topic_conf[topic_loop].filter_rule == NULL){
                COMMLOG(OBS_LOGWARN,"filter_rule is NULL in function:%s, line%d", __FUNCTION__, __LINE__);
                break;
            }
            CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].filter_rule[filter_loop].value);
        }
        CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].filter_rule);
        CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].event);
    }
    CHECK_NULL_FREE(smn_data->notification_conf.topic_conf);

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


void get_notification_configuration(const obs_options *options,
    obs_smn_handler *handler, void *callback_data)
{
    request_params  params;
    get_smn_data    *smn_data = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get_bucket_cors_configuration start !");

    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }

    smn_data = init_get_smn_data(handler, callback_data, use_api);
    if (NULL == smn_data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
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

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_notification_properties_callback;
    params.fromObsCallback = &get_notification_data_callback;
    params.complete_callback = &get_notification_complete_callback;
    params.callback_data = smn_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "notification";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_lifecycle_configuration finish.");
}