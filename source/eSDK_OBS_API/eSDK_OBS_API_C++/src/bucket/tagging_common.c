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
#include "tagging_common.h"
#include "request_util.h"
#include <openssl/md5.h>

void get_tagging_impl(const obs_options *options, const char *key,
    int require_key,
    obs_response_handler *response_handler,
    obs_get_bucket_tagging_callback *tagging_callback,
    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get tagging start!");

    if (require_key && !key) {
        COMMLOG(OBS_LOGERROR, "key is NULL!");
        (void)(*(response_handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    get_bucket_tagging_data *tagging_data =
        (get_bucket_tagging_data*)malloc(sizeof(get_bucket_tagging_data));
    if (!tagging_data)
    {
        (void)(*(response_handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc get_tagging_data failed !");
        return;
    }
    memset_s(tagging_data, sizeof(get_bucket_tagging_data),
        0, sizeof(get_bucket_tagging_data));

    simplexml_initialize(&(tagging_data->simpleXml), &get_tagging_xml_callback, tagging_data);
    tagging_data->response_properties_callback = response_handler->properties_callback;
    tagging_data->response_complete_callback = response_handler->complete_callback;
    tagging_data->response_tagging_list_callback = tagging_callback;
    tagging_data->callback_data = callback_data;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_tagging_properties_callback;
    params.fromObsCallback = &get_tagging_data_callback;
    params.complete_callback = &get_tagging_complete_callback;
    params.callback_data = tagging_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.key = key;
    params.subResource = "tagging";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get tagging finish!");
}

void set_tagging_impl(const obs_options *options, const char *key,
    int require_key,
    obs_name_value *tagging_list, unsigned int number,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_put_properties properties;
    unsigned char doc_md5[MD5_LEN] = { 0 };
    char base64_md5[BASE64_MD5_LEN] = { 0 };
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set tagging start!");

    if (require_key && !key) {
        COMMLOG(OBS_LOGERROR, "key is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    if (number > 0 && !tagging_list) {
        COMMLOG(OBS_LOGERROR, "tagging_list is NULL when number > 0!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    set_common_data *tagging_data = (set_common_data*)malloc(sizeof(set_common_data));
    if (!tagging_data)
    {
        COMMLOG(OBS_LOGERROR, "Malloc tagging_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        return;
    }
    memset_s(tagging_data, sizeof(set_common_data), 0, sizeof(set_common_data));

    obs_status status = generate_tagging_xml_document(tagging_list, number,
        &(tagging_data->xml_document_len), tagging_data->xml_document,
        sizeof(tagging_data->xml_document));
    if (status != OBS_STATUS_OK)
    {
        free(tagging_data);
        tagging_data = NULL;
        (void)(*(handler->complete_callback))(status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "tagging: generate_tagging_xml_document failed !");
        return;
    }

    tagging_data->response_properties_callback = handler->properties_callback;
    tagging_data->response_complete_callback = handler->complete_callback;
    tagging_data->xml_document_bytes_written = 0;
    tagging_data->callback_data = callback_data;

    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    /* MD5 is required by OBS API protocol for Content-MD5 header, not used for security purposes */
    MD5((unsigned char*)tagging_data->xml_document, (size_t)tagging_data->xml_document_len, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), base64_md5);
    properties.md5 = base64_md5;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.put_properties = &properties;
    params.httpRequestType = http_request_type_put;
    params.properties_callback = &set_common_properties_callback;
    params.toObsCallback = &set_common_data_callback;
    params.toObsCallbackTotalSize = tagging_data->xml_document_len;
    params.complete_callback = &set_common_complete_callback;
    params.callback_data = tagging_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.key = key;
    params.subResource = "tagging";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set tagging finish!");
}
