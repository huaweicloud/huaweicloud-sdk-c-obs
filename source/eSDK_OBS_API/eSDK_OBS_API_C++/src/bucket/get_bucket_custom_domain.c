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
#include "object.h"
#include "bucket.h"
#include "request_util.h"
#include <openssl/md5.h> 


obs_status custom_domain_xml_callback_data(get_bucket_custom_domain_data* domain_data,
    const char* element_path, const char* data, int data_len)
{
    int fit = 1;
    (void)fit; // 去除set but not used编译告警

    if (domain_data->domains_count < MAX_CUSTOM_DOMAINS) {
        list_bucket_custom_domains* domains = &(domain_data->domains[domain_data->domains_count]);
        if (!strcmp(element_path, "ListBucketCustomDomainsResult/Domains/DomainName")) {
            string_buffer_append(domains->domain_name, data, data_len, fit);
        }
        else if (!strcmp(element_path,
            "ListBucketCustomDomainsResult/Domains/CreateTime")) {
            string_buffer_append(domains->create_time, data, data_len, fit);
        }
        else if (!strcmp(element_path,
            "ListBucketCustomDomainsResult/Domains/CertificateId")) {
            string_buffer_append(domains->certificate_id, data, data_len, fit);
        }
    }
    return OBS_STATUS_OK;
}

obs_status custom_domain_xml_callback_nodata(get_bucket_custom_domain_data* domain_data, const char* element_path)
{
    if (!strcmp(element_path, "ListBucketCustomDomainsResult/Domains"))
    {
        domain_data->domains_count++;
        if (domain_data->domains_count == MAX_CUSTOM_DOMAINS)
        {
            COMMLOG(OBS_LOGINFO, "already get the max[%d] domains!", MAX_CUSTOM_DOMAINS);
            return OBS_STATUS_OK;
        }
        memset_s(&domain_data->domains[domain_data->domains_count], sizeof(list_bucket_custom_domains), 0, sizeof(list_bucket_custom_domains));
        return OBS_STATUS_OK;
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_custom_domain_data_callback(int buffer_size, const char* buffer,
    void* callback_data)
{
    get_bucket_custom_domain_data* domain_data = (get_bucket_custom_domain_data*)callback_data;
    return simplexml_add(&(domain_data->simpleXml), buffer, buffer_size);
}

obs_status get_bucket_custom_domain_properties_callback(
    const obs_response_properties* response_properties,
    void* callback_data)
{
    get_bucket_custom_domain_data* domain_data = (get_bucket_custom_domain_data*)callback_data;
    if (domain_data->response_properties_callback)
    {
        return (*(domain_data->response_properties_callback))
            (response_properties, domain_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status make_list_custom_domain_callback(get_bucket_custom_domain_data* domain_data)
{
    obs_status status = OBS_STATUS_OK;
    obs_domain_response* domain_list = NULL;
    int domain_count = 0;
    int i = 0;
    if (domain_data->domains_count > 0)
    {
        domain_list = (obs_domain_response*)malloc(sizeof(obs_domain_response) * domain_data->domains_count);
        if (NULL == domain_list)
        {
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(domain_list, sizeof(obs_domain_response) * domain_data->domains_count,
            0, sizeof(obs_domain_response) * domain_data->domains_count);

        domain_count = domain_data->domains_count;
        for (i = 0; i < domain_count; i++)
        {
            obs_domain_response* content_dest = &(domain_list[i]);
            list_bucket_custom_domains* content_src = &(domain_data->domains[i]);
            content_dest->domain_name = content_src->domain_name;
            content_dest->create_time = content_src->create_time;
            content_dest->certificate_id = content_src->certificate_id;
        }
    }

    if (domain_data->response_custom_domain_callback)
    {
        status = (*(domain_data->response_custom_domain_callback))
            (domain_count, domain_list, domain_data->callback_data);
    }
    CHECK_NULL_FREE(domain_list);

    return status;
}

void get_bucket_custom_domain_complete_callback(obs_status status,
    const obs_error_details* error_details,
    void* callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_custom_domain_data* domain_data = (get_bucket_custom_domain_data*)callback_data;

    if (domain_data == NULL) {
        return;
    }

    if (domain_data->domains_count) {
        (void)make_list_custom_domain_callback(domain_data);
    }
    else if (domain_data->response_custom_domain_callback) {
        (*(domain_data->response_custom_domain_callback))(0, NULL, domain_data->callback_data);
    }

    if (domain_data->response_complete_callback != NULL) {
        (void)(*(domain_data->response_complete_callback))(status, error_details,
            domain_data->callback_data);
        simplexml_deinitialize(&(domain_data->simpleXml));
    }
    free(domain_data);
    domain_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

obs_status custom_domain_xml_callback(const char* element_path,
    const char* data, int data_len, void* callback_data)
{
    get_bucket_custom_domain_data* domain_data = (get_bucket_custom_domain_data*)callback_data;

    if (data)
    {
        custom_domain_xml_callback_data(domain_data, element_path, data, data_len);
    }
    else
    {
        return custom_domain_xml_callback_nodata(domain_data, element_path);
    }

    return OBS_STATUS_OK;
}

static void handle_error_with_logging(char* errMsg, obs_status obsErrNo, obs_get_bucket_custom_domain_handler* handler, void* callback_data)
{
    COMMLOG(OBS_LOGERROR, errMsg);
    (void)(*(handler->response_handler.complete_callback))(obsErrNo, 0, callback_data);
}

static errno_t safe_memcpy_and_logging(void* dest, size_t dest_size, const void* src, size_t count, const char* func, int line) {
    errno_t err = memcpy_s(dest, dest_size, src, count);
    CheckAndLogNoneZero(err, "memcpy_s", func, line);
    return err;
}

void get_bucket_custom_domain(const obs_options* options,
    obs_get_bucket_custom_domain_handler* handler, void* callback_data)
{
    if (handler == NULL || handler->response_handler.complete_callback == NULL) {
        COMMLOG(OBS_LOGERROR, "get_bucket_custom_domain handler is NULL!");
        return;
    }
    request_params     params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get_bucket_custom_domain start!");

    if (!options->bucket_options.bucket_name) {
        handle_error_with_logging("get_bucket_custom_domain bucket_name is NULL.",
            OBS_STATUS_InvalidBucketName, handler, callback_data);
        return;
    }

    get_bucket_custom_domain_data* custom_domain_data =
        (get_bucket_custom_domain_data*)malloc(sizeof(get_bucket_custom_domain_data));

    if (!custom_domain_data) {
        handle_error_with_logging("get_bucket_custom_domain custom_domain_data is NULL.",
            OBS_STATUS_OutOfMemory, handler, callback_data);
        return;
    }

    memset_s(custom_domain_data, sizeof(get_bucket_custom_domain_data),
        0, sizeof(get_bucket_custom_domain_data));

    simplexml_initialize(&(custom_domain_data->simpleXml), &custom_domain_xml_callback, custom_domain_data);

    custom_domain_data->response_properties_callback = handler->response_handler.properties_callback;
    custom_domain_data->response_custom_domain_callback = handler->get_bucket_custom_domain_callback;
    custom_domain_data->response_complete_callback = handler->response_handler.complete_callback;
    custom_domain_data->callback_data = callback_data;


    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = safe_memcpy_and_logging(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context), __FUNCTION__, __LINE__);

    if (err != 0) {
        handle_error_with_logging("get_bucket_custom_domain copying the bucket context failed.",
            OBS_STATUS_Security_Function_Failed, handler, callback_data);
        return;
    }

    err = safe_memcpy_and_logging(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option), __FUNCTION__, __LINE__);
    if (err != 0) {
        handle_error_with_logging("get_bucket_custom_domain copying the request option failed.",
            OBS_STATUS_Security_Function_Failed, handler, callback_data);
        return;
    }

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_custom_domain_properties_callback;
    params.fromObsCallback = &get_bucket_custom_domain_data_callback;
    params.complete_callback = &get_bucket_custom_domain_complete_callback;
    params.callback_data = custom_domain_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.temp_auth = options->temp_auth;
    params.subResource = "customdomain";
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_custom_domain finish.");
}