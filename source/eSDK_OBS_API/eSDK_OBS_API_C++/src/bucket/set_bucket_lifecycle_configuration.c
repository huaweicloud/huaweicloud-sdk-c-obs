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

void add_xml_element_expiration(set_lifecycle_data* sblcData, obs_lifecycle_conf* bucket_lifecycle_conf, unsigned int i)
{
    (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Expiration", NULL, NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);

    if (bucket_lifecycle_conf[i].days) {
        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Days", bucket_lifecycle_conf[i].days,
            NEED_FORMALIZE, ADD_NAME_CONTENT);
    }
    if (bucket_lifecycle_conf[i].date) {
        char date_Iso8601[50] = { 0 };
        changeTimeFormat(bucket_lifecycle_conf[i].date, date_Iso8601);
        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Date", date_Iso8601, NEED_FORMALIZE, ADD_NAME_CONTENT);
    }
    (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Expiration", NULL, NEED_FORMALIZE, ADD_TAIL_ONLY);
}

void add_xml_element_transition(set_lifecycle_data* sblcData, obs_lifecycle_conf* bucket_lifecycle_conf, unsigned int i, char** pp_storage_class)
{
    unsigned int j = 0;
    int is_true = 0;
    for (j = 0; j < bucket_lifecycle_conf[i].transition_num; j++)
    {
        obs_storage_class tempStorageClass = bucket_lifecycle_conf[i].transition[j].storage_class;
        is_true = ((bucket_lifecycle_conf[i].transition[j].days == NULL) &&
            (bucket_lifecycle_conf[i].transition[j].date == NULL));

        if (is_true)
        {
            COMMLOG(OBS_LOGERROR, "date and days are both NULL for transition No %d!", j);
            break;
        }

        is_true = ((tempStorageClass != OBS_STORAGE_CLASS_STANDARD_IA) &&
            (tempStorageClass != OBS_STORAGE_CLASS_GLACIER));
        if (is_true)
        {
            COMMLOG(OBS_LOGERROR, "storage_class[%d] for transition No %d, only glacier and standard-la are valid !",
                tempStorageClass, j);
            break;
        }

        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Transition", NULL, NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);

        if (bucket_lifecycle_conf[i].transition[j].days)
        {
            (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Days", bucket_lifecycle_conf[i].transition[j].days, NEED_FORMALIZE, ADD_NAME_CONTENT);
        }
        if (bucket_lifecycle_conf[i].transition[j].date)
        {
            char date_Iso8601[50] = { 0 };
            changeTimeFormat(bucket_lifecycle_conf[i].transition[j].date, date_Iso8601);
            (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Date", date_Iso8601, NEED_FORMALIZE, ADD_NAME_CONTENT);
        }
        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "StorageClass", pp_storage_class[tempStorageClass], NEED_FORMALIZE, ADD_NAME_CONTENT);

        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Transition", NULL, NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    }
}

void add_xml_elemet_noversion_transition(set_lifecycle_data* sblcData, obs_lifecycle_conf* bucket_lifecycle_conf, unsigned int i, char** pp_storage_class)
{
    unsigned int j = 0;
    int is_true = 0;
    for (j = 0; j < bucket_lifecycle_conf[i].noncurrent_version_transition_num; j++)
    {
        obs_storage_class tempStorageClass = bucket_lifecycle_conf[i].noncurrent_version_transition[j].storage_class;
        if (bucket_lifecycle_conf[i].noncurrent_version_transition[j].noncurrent_version_days == NULL)
        {
            COMMLOG(OBS_LOGERROR, "days is NULL for nonCurrentVersionTranstion No %d!", j);
            break;
        }

        is_true = ((tempStorageClass != OBS_STORAGE_CLASS_STANDARD_IA) &&
            (tempStorageClass != OBS_STORAGE_CLASS_GLACIER));
        if (is_true)
        {
            COMMLOG(OBS_LOGERROR, "storage_class[%d] for transition No %d, only glacier and standard-la are valid !",
                tempStorageClass, j);
            break;
        }

        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "NoncurrentVersionTransition", NULL, NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);

        if (bucket_lifecycle_conf[i].noncurrent_version_transition[j].noncurrent_version_days)
        {
            (void)add_xml_element(sblcData->doc, &sblcData->docLen, "NoncurrentDays",
                bucket_lifecycle_conf[i].noncurrent_version_transition[j].noncurrent_version_days, NEED_FORMALIZE, ADD_NAME_CONTENT);
        }

        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "StorageClass", pp_storage_class[tempStorageClass], NEED_FORMALIZE, ADD_NAME_CONTENT);

        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "NoncurrentVersionTransition", NULL, NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    }

}

obs_status set_lifecycle_request_xml(set_lifecycle_data *sblcData,
    obs_lifecycle_conf* bucket_lifecycle_conf, unsigned int blcc_number, obs_use_api use_api)
{

    unsigned int i = 0;
    int is_true = 0;
    char **pp_storage_class = NULL;

    pp_storage_class = use_api == OBS_USE_API_S3 ? g_storage_class_s3 : g_storage_class_obs;

    (void)add_xml_element(sblcData->doc, &sblcData->docLen, "LifecycleConfiguration",
        NULL, NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
    for (i = 0; i < blcc_number; ++i)
    {
        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Rule", NULL, NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);

        //add id, prefix, status
        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "ID", bucket_lifecycle_conf[i].id, NEED_FORMALIZE, ADD_NAME_CONTENT);
        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Prefix", bucket_lifecycle_conf[i].prefix, NEED_FORMALIZE, ADD_NAME_CONTENT);
        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Status", bucket_lifecycle_conf[i].status, NEED_FORMALIZE, ADD_NAME_CONTENT);

        is_true = (NULL != bucket_lifecycle_conf[i].days || NULL != bucket_lifecycle_conf[i].date);
        if (is_true)
        {
            //add expiration
            add_xml_element_expiration(sblcData, bucket_lifecycle_conf, i);
        }

        //add transition
        add_xml_element_transition(sblcData, bucket_lifecycle_conf, i, pp_storage_class);

        //add non version transition
        add_xml_elemet_noversion_transition(sblcData, bucket_lifecycle_conf, i, pp_storage_class);

        //add non current version expiration
        if (bucket_lifecycle_conf[i].noncurrent_version_days)
        {
            (void)add_xml_element(sblcData->doc, &sblcData->docLen, "NoncurrentVersionExpiration", NULL, NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
            (void)add_xml_element(sblcData->doc, &sblcData->docLen, "NoncurrentDays",
                bucket_lifecycle_conf[i].noncurrent_version_days, NEED_FORMALIZE, ADD_NAME_CONTENT);
            (void)add_xml_element(sblcData->doc, &sblcData->docLen, "NoncurrentVersionExpiration", NULL, NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
        }

        (void)add_xml_element(sblcData->doc, &sblcData->docLen, "Rule", NULL, NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    }

    (void)add_xml_element(sblcData->doc, &sblcData->docLen, "LifecycleConfiguration",
        NULL, NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    return OBS_STATUS_OK;
}

static int set_lifecycle_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    set_lifecycle_data *sblcData = (set_lifecycle_data *)callback_data;

    if (!sblcData->docLen) {
        return 0;
    }

    int remaining = (sblcData->docLen - sblcData->docBytesWritten);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;
    if (!toCopy) {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(sblcData->doc[sblcData->docBytesWritten]), toCopy);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "set_lifecycle_data_callback: memcpy_s failed!\n");
        return 0;
    }

    sblcData->docBytesWritten += toCopy;
    return toCopy;
}

static obs_status set_lifecycle_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_lifecycle_data *sblcData = (set_lifecycle_data *)callback_data;
    if (sblcData->response_properties_callback)
    {
        return (*(sblcData->response_properties_callback))(response_properties,
            sblcData->callback_data);
    }
    return OBS_STATUS_OK;
}


static void set_lifecycle_complete_callback(obs_status requestStatus,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_lifecycle_data *sblcData = (set_lifecycle_data *)callback_data;

    (void)(*(sblcData->response_complete_callback))(requestStatus, obs_error_info, sblcData->callback_data);

    free(sblcData);
    sblcData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


static set_lifecycle_data* init_set_lifecycle_data(obs_lifecycle_conf* bucket_lifecycle_conf,
    unsigned int blcc_number, obs_response_handler *handler, void *callback_data, obs_use_api use_api)
{
    unsigned char doc_md5[16];
    set_lifecycle_data *sblcData = NULL;

    sblcData = (set_lifecycle_data *)malloc(sizeof(set_lifecycle_data));
    if (!sblcData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "Malloc set_lifecycle_data failed.");
        return NULL;
    }
    memset_s(sblcData, sizeof(set_lifecycle_data), 0, sizeof(set_lifecycle_data));

    sblcData->response_complete_callback = handler->complete_callback;
    sblcData->response_properties_callback = handler->properties_callback;
    sblcData->callback_data = callback_data;
    sblcData->docLen = 0;


    obs_status ret_status = set_lifecycle_request_xml(sblcData, bucket_lifecycle_conf, blcc_number, use_api);
    if (OBS_STATUS_OK != ret_status || sblcData->docLen < 0)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        free(sblcData);
        sblcData = NULL;
        return NULL;
    }
    COMMLOG(OBS_LOGERROR, "sblcData-doc: %s.", sblcData->doc);
    sblcData->docBytesWritten = 0;
    MD5((unsigned char *)sblcData->doc, (size_t)sblcData->docLen, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), sblcData->doc_md5);

    return sblcData;
}


void set_bucket_lifecycle_configuration(const obs_options *options,
    obs_lifecycle_conf* bucket_lifecycle_conf, unsigned int blcc_number,
    obs_response_handler *handler, void *callback_data)
{
    request_params     params;
    obs_put_properties put_properties;
    set_lifecycle_data *sblcData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set_bucket_lifecycle_configuration start !");
    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }

    if (NULL == bucket_lifecycle_conf || 0 == blcc_number)
    {
        COMMLOG(OBS_LOGERROR, "bucket_lifecycle_conf(%p) or blcc_number(%d) is invalid.",
            bucket_lifecycle_conf, blcc_number);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }

    sblcData = init_set_lifecycle_data(bucket_lifecycle_conf, blcc_number, handler, callback_data, use_api);
    if (!sblcData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc set_lifecycle_data failed.");
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

    memset_s(&put_properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    put_properties.md5 = sblcData->doc_md5;
    put_properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &set_lifecycle_properties_callback;
    params.toObsCallback = &set_lifecycle_data_callback;
    params.complete_callback = &set_lifecycle_complete_callback;
    params.toObsCallbackTotalSize = sblcData->docLen;
    params.callback_data = sblcData;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "lifecycle";
    params.put_properties = &put_properties;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_lifecycle_configuration finish.");
}