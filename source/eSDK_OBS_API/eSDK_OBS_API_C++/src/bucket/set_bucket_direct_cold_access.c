/*********************************************************************************
* Copyright 2019 Huawei Technologies Co.,Ltd.
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use
* this file except in compliance with the License.  You may obtain a copy of the
* License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software distributed
* under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations under the License.
**********************************************************************************
*/
#include "bucket.h"
#include "request_util.h"
#include <openssl/md5.h>

obs_status generate_direct_cold_access_xml_document(const char *enabled,
    int *xml_document_len_return, char *xml_document, int xml_document_buffer_size)
{
    *xml_document_len_return = 0;
    obs_status ret = OBS_STATUS_OK;

    if (!enabled) {
        COMMLOG(OBS_LOGERROR, "enabled parameter for generate_direct_cold_access_xml_document is NULL");
        return OBS_STATUS_InvalidArgument;
    }

    ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
    ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "<DirectColdAccessConfiguration><Status>%s</Status></DirectColdAccessConfiguration>",
        enabled);

    return ret;
}

void set_bucket_direct_cold_access(const obs_options *options, const char *enabled,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_put_properties  properties;
    unsigned char doc_md5[MD5_LEN] = { 0 };
    char base64_md5[BASE64_MD5_LEN] = { 0 };
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set bucket direct cold access start!");

    set_common_data *direct_cold_access_data = (set_common_data*)malloc(sizeof(set_common_data));
    if (!direct_cold_access_data)
    {
        COMMLOG(OBS_LOGERROR, "Malloc set bucket direct_cold_access_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        return;
    }
    memset_s(direct_cold_access_data, sizeof(set_common_data), 0, sizeof(set_common_data));

    obs_status status = generate_direct_cold_access_xml_document(enabled,
        &(direct_cold_access_data->xml_document_len), direct_cold_access_data->xml_document,
        sizeof(direct_cold_access_data->xml_document));
    if (status != OBS_STATUS_OK)
    {
        free(direct_cold_access_data);
        direct_cold_access_data = NULL;
        (void)(*(handler->complete_callback))(status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "direct_cold_access: generate_direct_cold_access_xml_document failed !");
        return;
    }

    direct_cold_access_data->response_properties_callback = handler->properties_callback;
    direct_cold_access_data->response_complete_callback = handler->complete_callback;
    direct_cold_access_data->xml_document_bytes_written = 0;
    direct_cold_access_data->callback_data = callback_data;

    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    /* MD5 is required by OBS API protocol for Content-MD5 header, not used for security purposes */
    MD5((unsigned char*)direct_cold_access_data->xml_document, (size_t)direct_cold_access_data->xml_document_len, doc_md5);
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
    params.toObsCallbackTotalSize = direct_cold_access_data->xml_document_len;
    params.complete_callback = &set_common_complete_callback;
    params.callback_data = direct_cold_access_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "directcoldaccess";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket direct cold access finish!");
}