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

obs_status parse_xml_get_lifecycle(get_lifecycle_config_data *gblcDataEx,
    const char *element_path, const char *data, int data_len)
{
    int fit = 1;
    int nIndex = gblcDataEx->blcc_number - 1;
    int transitionIndex = gblcDataEx->blcc_data[nIndex]->transition_num;
    int nonCurrentVersionTransitionIndex = gblcDataEx->blcc_data[nIndex]->noncurrent_version_transition_num;

    if (!strcmp(element_path, "LifecycleConfiguration/Rule/ID")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->id, data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/Prefix")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->prefix, data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/Status")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->status, data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/Expiration/Date")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->date, data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/Expiration/Days")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->days, data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/NoncurrentVersionExpiration/NoncurrentDays")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->nonCurrentVerionDays, data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/Transition/Days")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrTransitionData[transitionIndex].days, data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/Transition/Date")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrTransitionData[transitionIndex].date, data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/Transition/StorageClass")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrTransitionData[transitionIndex].storage_class, data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/NoncurrentVersionTransition/NoncurrentDays")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrNonCurrentVersionTransitionData[nonCurrentVersionTransitionIndex].days,
            data, data_len, fit);
    }
    else if (!strcmp(element_path, "LifecycleConfiguration/Rule/NoncurrentVersionTransition/StorageClass")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrNonCurrentVersionTransitionData[nonCurrentVersionTransitionIndex].storage_class,
            data, data_len, fit);
    }

    //(void)fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}

static obs_status get_lifecycle_config_xml_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    get_lifecycle_config_data *gblcDataEx = (get_lifecycle_config_data *)callback_data;
    int nIndex = gblcDataEx->blcc_number - 1;

    if (data)
    {
        return parse_xml_get_lifecycle(gblcDataEx, element_path, data, data_len);
    }

    if (!strcmp(element_path, "LifecycleConfiguration/Rule"))
    {
        lifecycle_conf_data* blcc_data = (lifecycle_conf_data*)malloc(sizeof(lifecycle_conf_data));
        if (!blcc_data)
        {
            COMMLOG(OBS_LOGERROR, "malloc lifecycle_conf_data failed !");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(blcc_data, sizeof(lifecycle_conf_data), 0, sizeof(lifecycle_conf_data));
        gblcDataEx->blcc_data[gblcDataEx->blcc_number] = blcc_data;
        gblcDataEx->blcc_number++;
    }

    if (!strcmp(element_path, "LifecycleConfiguration/Rule/Transition"))
    {
        gblcDataEx->blcc_data[nIndex]->transition_num++;
    }

    if (!strcmp(element_path, "LifecycleConfiguration/Rule/NoncurrentVersionTransition"))
    {
        gblcDataEx->blcc_data[nIndex]->noncurrent_version_transition_num++;
    }

    return OBS_STATUS_OK;
}


static get_lifecycle_config_data* init_get_lifecycle_data(obs_lifecycle_handler *handler,
    void *callback_data)
{
    get_lifecycle_config_data *gblcDataEx = NULL;
    lifecycle_conf_data* blcc_data = NULL;

    gblcDataEx = (get_lifecycle_config_data *)malloc(sizeof(get_lifecycle_config_data));
    if (!gblcDataEx) {
        COMMLOG(OBS_LOGERROR, "malloc lifecycle config data failed.");
        return NULL;
    }
    memset_s(gblcDataEx, sizeof(get_lifecycle_config_data), 0, sizeof(get_lifecycle_config_data));

    simplexml_initialize(&(gblcDataEx->simple_xml_info), &get_lifecycle_config_xml_callback, gblcDataEx);

    gblcDataEx->response_properties_callback = handler->response_handler.properties_callback;
    gblcDataEx->response_complete_callback = handler->response_handler.complete_callback;
    gblcDataEx->get_lifecycle_callback = handler->get_lifecycle_callback;
    gblcDataEx->callback_data = callback_data;

    blcc_data = (lifecycle_conf_data*)malloc(sizeof(lifecycle_conf_data));
    if (!blcc_data)
    {
        COMMLOG(OBS_LOGERROR, "malloc lifecycle_conf_data failed.");
        free(gblcDataEx);
        return NULL;
    }
    memset_s(blcc_data, sizeof(lifecycle_conf_data), 0, sizeof(lifecycle_conf_data));
    gblcDataEx->blcc_data[0] = blcc_data;
    gblcDataEx->blcc_number = 1;

    return gblcDataEx;
}

static obs_status make_get_lifecycle_callback(get_lifecycle_config_data *gblcDataEx)
{
    obs_status iRet = OBS_STATUS_OK;

    int nCount = gblcDataEx->blcc_number - 1;
    if (nCount < 1)
    {
        COMMLOG(OBS_LOGERROR, "Invalid Malloc Parameter.");
        return OBS_STATUS_OutOfMemory;
    }

    obs_lifecycle_conf* buckLifeCycleConf = (obs_lifecycle_conf*)malloc(sizeof(obs_lifecycle_conf) * nCount);
    if (NULL == buckLifeCycleConf)
    {
        COMMLOG(OBS_LOGERROR, "malloc obs_lifecycle_conf failed.");
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(buckLifeCycleConf, sizeof(obs_lifecycle_conf) * nCount, 0, sizeof(obs_lifecycle_conf) * nCount);

    int i = 0;
    int j = 0;
    for (; i < nCount; ++i)
    {
        buckLifeCycleConf[i].date = gblcDataEx->blcc_data[i]->date;
        buckLifeCycleConf[i].days = gblcDataEx->blcc_data[i]->days;
        buckLifeCycleConf[i].id = gblcDataEx->blcc_data[i]->id;
        buckLifeCycleConf[i].prefix = gblcDataEx->blcc_data[i]->prefix;
        buckLifeCycleConf[i].status = gblcDataEx->blcc_data[i]->status;
        buckLifeCycleConf[i].noncurrent_version_days = gblcDataEx->blcc_data[i]->nonCurrentVerionDays;
        buckLifeCycleConf[i].transition_num = gblcDataEx->blcc_data[i]->transition_num;
        buckLifeCycleConf[i].transition = (obs_lifecycle_transtion*)malloc(
            sizeof(obs_lifecycle_transtion) * (gblcDataEx->blcc_data[i]->transition_num));
        if (NULL == buckLifeCycleConf[i].transition)
        {
            for (int n = 0; n < i; ++n) {
                CHECK_NULL_FREE(buckLifeCycleConf[n].noncurrent_version_transition);
                CHECK_NULL_FREE(buckLifeCycleConf[n].transition);
            }
            CHECK_NULL_FREE(buckLifeCycleConf);
            COMMLOG(OBS_LOGERROR, "malloc obs_lifecycle_conf failed.");
            return OBS_STATUS_OutOfMemory;
        }
        for (j = 0; j < gblcDataEx->blcc_data[i]->transition_num; j++)
        {
            buckLifeCycleConf[i].transition[j].date = gblcDataEx->blcc_data[i]->arrTransitionData[j].date;
            buckLifeCycleConf[i].transition[j].days = gblcDataEx->blcc_data[i]->arrTransitionData[j].days;
            buckLifeCycleConf[i].transition[j].storage_class = get_storage_class_enum(
                gblcDataEx->blcc_data[i]->arrTransitionData[j].storage_class, gblcDataEx->use_api);
        }

        buckLifeCycleConf[i].noncurrent_version_transition_num = gblcDataEx->blcc_data[i]->noncurrent_version_transition_num;
        buckLifeCycleConf[i].noncurrent_version_transition = (obs_lifecycle_noncurrent_transtion*)malloc(
            sizeof(obs_lifecycle_noncurrent_transtion) * gblcDataEx->blcc_data[i]->noncurrent_version_transition_num);
        if (NULL == buckLifeCycleConf[i].noncurrent_version_transition)
        {
            for (int n = 0; n < i; ++n) {
                CHECK_NULL_FREE(buckLifeCycleConf[n].noncurrent_version_transition);
                CHECK_NULL_FREE(buckLifeCycleConf[n].transition);
            }
            CHECK_NULL_FREE(buckLifeCycleConf[i].transition);
            CHECK_NULL_FREE(buckLifeCycleConf);
            COMMLOG(OBS_LOGERROR, "malloc noncurrent_version_transition failed.");
            return OBS_STATUS_OutOfMemory;
        }

        for (j = 0; j < gblcDataEx->blcc_data[i]->noncurrent_version_transition_num; j++)
        {
            buckLifeCycleConf[i].noncurrent_version_transition[j].noncurrent_version_days =
                gblcDataEx->blcc_data[i]->arrNonCurrentVersionTransitionData[j].days;
            buckLifeCycleConf[i].noncurrent_version_transition[j].storage_class = get_storage_class_enum(
                gblcDataEx->blcc_data[i]->arrNonCurrentVersionTransitionData[j].storage_class, gblcDataEx->use_api);
        }

    }

    iRet = (*(gblcDataEx->get_lifecycle_callback))(buckLifeCycleConf, nCount,
        gblcDataEx->callback_data);

    for (i = 0; i < nCount; i++)
    {
        CHECK_NULL_FREE(buckLifeCycleConf[i].noncurrent_version_transition);
        CHECK_NULL_FREE(buckLifeCycleConf[i].transition);
    }


    return iRet;
}


static obs_status get_lifecycle_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    get_lifecycle_config_data *gblcDataEx = (get_lifecycle_config_data *)callback_data;
    if (gblcDataEx->response_properties_callback)
    {
        return (*(gblcDataEx->response_properties_callback))(response_properties,
            gblcDataEx->callback_data);
    }
    return OBS_STATUS_OK;
}

static obs_status get_lifecycle_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    get_lifecycle_config_data *gblcDataEx = (get_lifecycle_config_data *)callback_data;

    return simplexml_add(&(gblcDataEx->simple_xml_info), buffer, buffer_size);
}


static void get_lifecycle_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info, void *callback_data)
{
    unsigned int i = 0;
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_lifecycle_config_data *gblcDataEx = (get_lifecycle_config_data *)callback_data;

    // Make the callback if there is anything
    if (gblcDataEx->blcc_number && OBS_STATUS_OK == request_status) {
        request_status = make_get_lifecycle_callback(gblcDataEx);
    }

    (*(gblcDataEx->response_complete_callback))(request_status, obs_error_info,
        gblcDataEx->callback_data);

    for (i = 0; i < gblcDataEx->blcc_number; i++)
    {
        CHECK_NULL_FREE(gblcDataEx->blcc_data[i]);
    }

    simplexml_deinitialize(&(gblcDataEx->simple_xml_info));

    free(gblcDataEx);
    gblcDataEx = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


void get_bucket_lifecycle_configuration(const obs_options *options,
    obs_lifecycle_handler *handler, void *callback_data)
{
    request_params params;
    get_lifecycle_config_data *gblcDataEx = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set_bucket_lifecycle_configuration start !");
    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    gblcDataEx = init_get_lifecycle_data(handler, callback_data);
    if (NULL == gblcDataEx)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        return;
    }
    gblcDataEx->use_api = use_api;
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_lifecycle_properties_callback;
    params.fromObsCallback = &get_lifecycle_data_callback;
    params.complete_callback = &get_lifecycle_complete_callback;
    params.callback_data = gblcDataEx;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "lifecycle";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_lifecycle_configuration finish.");

}
