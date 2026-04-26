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


#define NAME_LEN_MIN                (3)
#define NAME_LEN_MAX                (63)
#define CERT_ID_LEN_MIN             (16)
#define CERT_ID_LEN_MAX             (16)
#define SET_CUSTOM_DOMAIN_XML_SIZE  (40 * 1024)

static void generate_set_custom_domain_xml_document(obs_custom_domain* custom_domain,
    int* xml_document_len_return, char* xml_document, int xml_document_buffer_size)
{
    *xml_document_len_return = 0;

    custom_domain_certificate_config* custom_domain_cert_config = custom_domain->custom_domain_certificate_config;

    append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

    append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "<CustomDomainConfiguration>");

    append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "<Name>%s</Name>", custom_domain_cert_config->name);

    if (custom_domain_cert_config->certificate_id) {
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "<CertificateId>%s</CertificateId>", custom_domain_cert_config->certificate_id);
    }


    if (custom_domain_cert_config->certificate) {
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "<Certificate>%s</Certificate>", custom_domain_cert_config->certificate);
    }

    if (custom_domain_cert_config->certificate_chain) {
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "<CertificateChain>%s</CertificateChain>", custom_domain_cert_config->certificate_chain);
    }

    if (custom_domain_cert_config->private_key) {
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "<PrivateKey>%s</PrivateKey>", custom_domain_cert_config->private_key);
    }

    append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "</CustomDomainConfiguration>");
}

obs_status check_custom_certificate_config(set_common_data* domain_data,
    custom_domain_certificate_config* domain_cert_config, obs_response_handler* handler)
{
    obs_status status = OBS_STATUS_OK;
    if (!domain_cert_config->name) {
        COMMLOG(OBS_LOGERROR, "check_custom_certificate_config No Certificate Name.");
        return OBS_STATUS_NoCertName;
    }

    if (!domain_cert_config->certificate) {
        COMMLOG(OBS_LOGERROR, "check_custom_certificate_config No Certificate.");
        return OBS_STATUS_CertNotExist;
    }

    if (!domain_cert_config->private_key) {
        COMMLOG(OBS_LOGERROR, "check_custom_certificate_config No Certificate Private Key.");
        return OBS_STATUS_CertPKNotExist;
    }

    if (domain_cert_config->name) {
        size_t name_len = strlen(domain_cert_config->name);
        if (name_len < NAME_LEN_MIN || name_len > NAME_LEN_MAX) {
            COMMLOG(OBS_LOGERROR, "check_custom_certificate_config Invalid Cert Name Length.");
            return OBS_STATUS_InvalidCertNameLen;
        }
    }

    if (domain_cert_config->certificate_id) {
        size_t cert_id_len = strlen(domain_cert_config->certificate_id);
        if (cert_id_len < CERT_ID_LEN_MIN || cert_id_len > CERT_ID_LEN_MAX) {
            COMMLOG(OBS_LOGERROR, "check_custom_certificate_config Invalid Cert Id Length.");
            return OBS_STATUS_InvalidCertIdLen;
        }
    }

    return status;
}

static void handle_error_with_logging(char* errMsg, obs_status obsErrNo, obs_response_handler* handler, void* callback_data)
{
    COMMLOG(OBS_LOGERROR, errMsg);
    (void)(*(handler->complete_callback))(obsErrNo, 0, callback_data);
}

static void safe_memcpy_and_logging(void* dest, size_t dest_size, const void* src, size_t count, const char* func, int line) {
    errno_t err = memcpy_s(dest, dest_size, src, count);
    CheckAndLogNoneZero(err, "memcpy_s", func, line);
}

void set_bucket_custom_domain(const obs_options* options, obs_custom_domain* custom_domain,
    obs_response_handler* handler, void* callback_data)
{
    request_params     params;
    obs_put_properties properties;
    obs_use_api use_api = OBS_USE_API_S3;
    char base64_md5[BASE64_MD5_LEN] = { 0 };
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    (void)queryParamsLen; // 去除set but not used编译告警
    
    if (handler == NULL || handler->complete_callback == NULL) {
        COMMLOG(OBS_LOGERROR, "set_bucket_custom_domain handler is NULL.");
        return;
    }

    if (options == NULL) {
        handle_error_with_logging("set_bucket_custom_domain options is NULL.",
            OBS_STATUS_InvalidParameter, handler, callback_data);
        return;
    }

    if (options->bucket_options.bucket_name == NULL) {
        handle_error_with_logging("set_bucket_custom_domain options is NULL.",
            OBS_STATUS_InvalidBucketName, handler, callback_data);
        return;
    }

    if (custom_domain == NULL) {
        handle_error_with_logging("set_bucket_custom_domain custom_domain is NULL!",
            OBS_STATUS_InvalidParameter, handler, callback_data);
        return;
    }

    if (custom_domain->domain_name == NULL) {
        handle_error_with_logging("set_bucket_custom_domain domain_name is invalid!",
            OBS_STATUS_InvalidDomainName, handler, callback_data);
        return;
    }

    set_common_data* domain_data = (set_common_data*)malloc(sizeof(set_common_data));
    if (!domain_data) {
        handle_error_with_logging("set_bucket_custom_domain Malloc failed!",
            OBS_STATUS_OutOfMemory, handler, callback_data);
        return;
    }

    memset_s(domain_data, sizeof(set_common_data), 0, sizeof(set_common_data));
    domain_data->response_properties_callback = handler->properties_callback;
    domain_data->response_complete_callback = handler->complete_callback;
    domain_data->xml_document_bytes_written = 0;
    domain_data->callback_data = callback_data;

    int ret = snprintf_s(queryParams, sizeof(queryParams),
        _TRUNCATE, "customdomain=%s", custom_domain->domain_name);
        COMMLOG(OBS_LOGERROR, "domain_name processing failed!");
    if (ret < 0) {
        handle_error_with_logging("set_bucket_custom_domain snprintf_s error!",
            (ret == -1 ? OBS_STATUS_OutOfMemory : OBS_STATUS_ErrorUnknown), handler, callback_data);
        free(domain_data);
        domain_data = NULL;
        return;
    }

    obs_status status = OBS_STATUS_OK;
    custom_domain_certificate_config* domain_cert_config = custom_domain->custom_domain_certificate_config;
    if (domain_cert_config) {
        status = check_custom_certificate_config(domain_data, domain_cert_config, handler);
        if (status != OBS_STATUS_OK) {
            free(domain_data);
            domain_data = NULL;
            handle_error_with_logging("Invalid Parameter!", status, handler, callback_data);
            return;
        } else if (options->bucket_options.protocol != OBS_PROTOCOL_HTTPS) {
            free(domain_data);
            domain_data = NULL;
            handle_error_with_logging("Only HTTPS option is allowed when certificates are used!",
                OBS_STATUS_SecureConnectionRequiredForCustomDomainCertificate, handler, callback_data);
            return;
        }
    }

    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set_bucket_custom_domain start!");

    if (domain_cert_config) {
        generate_set_custom_domain_xml_document(custom_domain,
            &(domain_data->xml_document_len),
            domain_data->xml_document,
            sizeof(domain_data->xml_document));

        if (domain_data->xml_document_len > SET_CUSTOM_DOMAIN_XML_SIZE) {
            free(domain_data);
            domain_data = NULL;
            handle_error_with_logging("set_bucket_custom_domain request XML size is greater than 40kB!",
                OBS_STATUS_MalformedXML, handler, callback_data);
            return;
        }
    }

    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    compute_md5(domain_data->xml_document, (size_t)domain_data->xml_document_len, (char*)base64_md5, BASE64_MD5_LEN);
    properties.md5 = base64_md5;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    safe_memcpy_and_logging(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context), __FUNCTION__, __LINE__);
    safe_memcpy_and_logging(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option), __FUNCTION__, __LINE__);

    params.put_properties = &properties;
    params.httpRequestType = http_request_type_put;
    params.properties_callback = &set_common_properties_callback;
    params.complete_callback = &set_common_complete_callback;
    params.toObsCallback = &set_common_data_callback;
    params.toObsCallbackTotalSize = domain_data->xml_document_len;
    params.callback_data = domain_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.temp_auth = options->temp_auth;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "set_bucket_custom_domain finish.");
}