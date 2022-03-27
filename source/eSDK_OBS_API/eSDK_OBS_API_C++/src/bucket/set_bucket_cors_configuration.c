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

static obs_status set_cors_xml_elements(const char **elements, unsigned int elements_num,
    char *element_name, set_cors_config_data *sbccData)
{
    unsigned int uiIdx = 0;
    for (uiIdx = 0; uiIdx < elements_num; uiIdx++)
    {
        if (NULL != elements[uiIdx])
        {
            (void)add_xml_element(sbccData->doc, &sbccData->doc_len, element_name,
                elements[uiIdx], NEED_FORMALIZE, ADD_NAME_CONTENT);
        }

        if ((sbccData->doc_len >= 1024 * 10) && (uiIdx != (elements_num - 1)))
        {
            COMMLOG(OBS_LOGERROR, "set cors fail,element_name(%s) Number(%u) too much.",
                element_name, elements_num);
            return OBS_STATUS_InvalidParameter;
        }
    }
    return OBS_STATUS_OK;
}

static obs_status set_cors_quest_xml(obs_bucket_cors_conf *obs_cors_conf_info,
    unsigned int conf_num, set_cors_config_data *sbccData)
{
    unsigned int i = 0;
    obs_status ret_status = OBS_STATUS_OK;

    (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "CORSConfiguration", NULL,
        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
    for (i = 0; i < conf_num; ++i)
    {
        if (NULL == obs_cors_conf_info[i].allowed_method || NULL == obs_cors_conf_info[i].allowed_origin)
        {
            COMMLOG(OBS_LOGERROR, "allowed_method(%p) or allowed_origin(%p) is NULL",
                obs_cors_conf_info[i].allowed_method, obs_cors_conf_info[i].allowed_origin);
            return OBS_STATUS_InvalidParameter;
        }

        (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "CORSRule", NULL,
            NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
        if (obs_cors_conf_info[i].id)
        {
            (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "ID",
                obs_cors_conf_info[i].id, NEED_FORMALIZE, ADD_NAME_CONTENT);
        }

        ret_status = set_cors_xml_elements(obs_cors_conf_info[i].allowed_method,
            obs_cors_conf_info[i].allowed_method_number, "AllowedMethod", sbccData);
        if (OBS_STATUS_OK != ret_status)
        {
            return OBS_STATUS_InvalidParameter;
        }

        ret_status = set_cors_xml_elements(obs_cors_conf_info[i].allowed_origin,
            obs_cors_conf_info[i].allowed_origin_number, "AllowedOrigin", sbccData);
        if (OBS_STATUS_OK != ret_status)
        {
            return OBS_STATUS_InvalidParameter;
        }

        ret_status = set_cors_xml_elements(obs_cors_conf_info[i].allowed_header,
            obs_cors_conf_info[i].allowed_header_number, "AllowedHeader", sbccData);
        if (OBS_STATUS_OK != ret_status)
        {
            return OBS_STATUS_InvalidParameter;
        }

        if (obs_cors_conf_info[i].max_age_seconds)
        {
            (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "MaxAgeSeconds",
                obs_cors_conf_info[i].max_age_seconds, NEED_FORMALIZE, ADD_NAME_CONTENT);
        }

        ret_status = set_cors_xml_elements(obs_cors_conf_info[i].expose_header,
            obs_cors_conf_info[i].expose_header_number, "ExposeHeader", sbccData);
        if (OBS_STATUS_OK != ret_status)
        {
            return OBS_STATUS_InvalidParameter;
        }

        (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "CORSRule", NULL,
            NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    }

    (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "CORSConfiguration", NULL,
        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);

    return OBS_STATUS_OK;
}

static set_cors_config_data* init_cors_data(obs_bucket_cors_conf *obs_cors_conf_info,
    unsigned int conf_num, obs_response_handler *handler, void *callback_data)
{
    unsigned char doc_md5[16];
    obs_status ret_status = OBS_STATUS_OK;
    set_cors_config_data *sbccData = (set_cors_config_data *)malloc(sizeof(set_cors_config_data));
    if (!sbccData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc cors_data failed.");
        return NULL;
    }
    memset_s(sbccData, sizeof(set_cors_config_data), 0, sizeof(set_cors_config_data));

    sbccData->response_complete_callback = handler->complete_callback;
    sbccData->response_properties_callback = handler->properties_callback;
    sbccData->callback_data = callback_data;
    sbccData->doc_len = 0;
    sbccData->doc_bytes_written = 0;

    ret_status = set_cors_quest_xml(obs_cors_conf_info, conf_num, sbccData);
    if (OBS_STATUS_OK != ret_status || sbccData->doc_len <= 0)
    {
        free(sbccData);
        sbccData = NULL;
        return NULL;
    }
    COMMLOG(OBS_LOGERROR, "request xml: %s.", sbccData->doc);

    MD5((unsigned char *)sbccData->doc, (size_t)sbccData->doc_len, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), sbccData->doc_md5);

    return sbccData;
}

static int set_cors_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    set_cors_config_data *sbccData = (set_cors_config_data *)callback_data;

    if (!sbccData->doc_len) {
        return 0;
    }

    int remaining = (sbccData->doc_len - sbccData->doc_bytes_written);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy) {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(sbccData->doc[sbccData->doc_bytes_written]), toCopy);
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    sbccData->doc_bytes_written += toCopy;

    return toCopy;
}

static obs_status set_cors_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    set_cors_config_data *sbccData = (set_cors_config_data *)callback_data;
    if (sbccData->response_properties_callback)
    {
        return (*(sbccData->response_properties_callback))
            (response_properties, sbccData->callback_data);
    }
    return OBS_STATUS_OK;
}


static void set_cors_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_cors_config_data *sbccData = (set_cors_config_data *)callback_data;

    (void)(*(sbccData->response_complete_callback))(request_status, obs_error_info,
        sbccData->callback_data);

    free(sbccData);
    sbccData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}



void set_bucket_cors_configuration(const obs_options *options, obs_bucket_cors_conf *obs_cors_conf_info,
    unsigned int conf_num, obs_response_handler *handler, void *callback_data)
{
    request_params     params;
    obs_put_properties put_properties;
    set_cors_config_data *sbccData = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "set_bucket_cors start !");

    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }

    if (conf_num <= 0 || conf_num > 100)
    {
        COMMLOG(OBS_LOGERROR, "set_bucket_cors faied, conf_num(%d) is invalid.", conf_num);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }

    sbccData = init_cors_data(obs_cors_conf_info, conf_num, handler, callback_data);
    if (!sbccData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc sbccData failed.");
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
    put_properties.md5 = sbccData->doc_md5;
    put_properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &set_cors_properties_callback;
    params.toObsCallback = &set_cors_data_callback;
    params.complete_callback = &set_cors_complete_callback;
    params.toObsCallbackTotalSize = sbccData->doc_len;
    params.callback_data = sbccData;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "cors";
    params.put_properties = &put_properties;
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_cors finish.");
}