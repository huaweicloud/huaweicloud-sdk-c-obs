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

obs_status generate_storage_class_xml_document(obs_storage_class storage_class_policy,
    int *xml_document_len_return, char *xml_document,
    int xml_document_buffer_size, obs_use_api use_api)
{
    *xml_document_len_return = 0;
    obs_status ret = OBS_STATUS_OK;

    if (use_api == OBS_USE_API_S3) {
        char *storage_class_list[] = { "STANDARD","STANDARD_IA","GLACIER","" };

        ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
        ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "%s", "<StoragePolicy xmlns=\"http://s3.amazonaws.com/doc/2015-06-30/\"><DefaultStorageClass>");
        ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "%s", storage_class_list[storage_class_policy]);
        ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "%s", "</DefaultStorageClass></StoragePolicy>");
    }
    else {
        char *storage_class_list[] = { "STANDARD","WARM","COLD","" };

        ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
        ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "%s", "<StorageClass xmlns=\"http://obs.myhwclouds.com/doc/2015-06-30/\">");
        ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "%s", storage_class_list[storage_class_policy]);
        ret = append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "%s", "</StorageClass>");
    }

    return ret;
}

void set_bucket_storage_class_policy(const obs_options *options,
    obs_storage_class storage_class_policy,
    obs_response_handler *handler,
    void *callback_data)
{
    request_params params;
    obs_put_properties  properties;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set bucket storage policy start!");
    if (storage_class_policy >= OBS_STORAGE_CLASS_BUTT)
    {
        COMMLOG(OBS_LOGERROR, "storage_class_policy invalid!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }

    set_common_data *common_data = (set_common_data*)malloc(sizeof(set_common_data));
    if (!common_data)
    {
        COMMLOG(OBS_LOGERROR, "Malloc set_stoarge_policy_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    memset_s(common_data, sizeof(set_common_data), 0, sizeof(set_common_data));

    obs_status status = generate_storage_class_xml_document(storage_class_policy,
        &(common_data->xml_document_len),
        common_data->xml_document,
        sizeof(common_data->xml_document), use_api);
    if (status != OBS_STATUS_OK)
    {
        free(common_data);
        common_data = NULL;
        (void)(*(handler->complete_callback))(status, 0, 0);
        return;
    }
    common_data->response_properties_callback = handler->properties_callback;
    common_data->response_complete_callback = handler->complete_callback;
    common_data->xml_document_bytes_written = 0;
    common_data->callback_data = callback_data;

    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

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
    params.toObsCallbackTotalSize = common_data->xml_document_len;
    params.complete_callback = &set_common_complete_callback;
    params.callback_data = common_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    if (use_api == OBS_USE_API_S3) {
        params.subResource = "storagePolicy";
    }
    else {
        params.subResource = "storageClass";
    }
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket storage policy finish!");
}
