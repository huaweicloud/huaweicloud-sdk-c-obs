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


obs_status generate_logging_xml_document_s3(char *target_bucket, char *target_prefix,
    obs_acl_group *acl_group, int *xml_doc_len_return,
    char *xml_document, int xml_doc_buffer_size)
{
    obs_status ret = OBS_STATUS_OK;
    if (!target_bucket) {
        ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
            "%s", "<BucketLoggingStatus xmlns=\"http://doc.s3.amazonaws.com/2006-03-01\" />");
        return ret;
    }

    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "%s", "<BucketLoggingStatus xmlns=\"http://doc.s3.amazonaws.com/2006-03-01\">");
    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "<LoggingEnabled><TargetBucket>%s</TargetBucket>", target_bucket);
    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "<TargetPrefix>%s</TargetPrefix>", target_prefix ? target_prefix : "");
    if (acl_group)
    {
        ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
            "%s", "<TargetGrants>");
        int i;
        for (i = 0; i < acl_group->acl_grant_count; i++)
        {
            ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, "%s",
                "<Grant><Grantee xmlns:xsi=\"http://www.w3.org/2001/"
                "XMLSchema-instance\" xsi:type=\"");
            obs_acl_grant *grant = &(acl_group->acl_grants[i]);
            switch (grant->grantee_type)
            {
            case OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL:
                ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
                    "AmazonCustomerByEmail\"><EmailAddress>%s</EmailAddress>",
                    grant->grantee.huawei_customer_by_email.email_address);
                break;
            case OBS_GRANTEE_TYPE_CANONICAL_USER:
                ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
                    "CanonicalUser\"><ID>%s</ID><DisplayName>%s</DisplayName>",
                    grant->grantee.canonical_user.id, grant->grantee.canonical_user.display_name);
                break;
            default:
                ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
                    "Group\"><URI>%s</URI>", (grant->grantee_type == OBS_GRANTEE_TYPE_ALL_OBS_USERS) ?
                    ACS_GROUP_AWS_USERS : ACS_GROUP_ALL_USERS);
                break;
            }
            ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
                "</Grantee><Permission>%s</Permission></Grant>",
                ((grant->permission == OBS_PERMISSION_READ) ? "READ" :
                (grant->permission == OBS_PERMISSION_WRITE) ? "WRITE" :
                    (grant->permission == OBS_PERMISSION_READ_ACP) ? "READ_ACP" :
                    (grant->permission == OBS_PERMISSION_WRITE_ACP) ? "WRITE_ACP" :
                    (grant->permission == OBS_PERMISSION_FULL_CONTROL) ? "FULL_CONTROL" : "READ"));
        }
        ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
            "%s", "</TargetGrants>");
    }
    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "%s", "</LoggingEnabled>");
    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "%s", "</BucketLoggingStatus>");

    return ret;
}

obs_status generate_logging_xml_document_obs(char *target_bucket, char *target_prefix, char *agency,
    obs_acl_group *acl_group, int *xml_doc_len_return,
    char *xml_document, int xml_doc_buffer_size)
{
    obs_status ret = OBS_STATUS_OK;
    if (!target_bucket)
    {
        ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
            "%s", "<BucketLoggingStatus xmlns=\"http://obs.myhwclouds.com/doc/2015-06-30/\" />");
        return ret;
    }

    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "%s", "<BucketLoggingStatus>");
    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "<Agency>%s</Agency><LoggingEnabled><TargetBucket>%s</TargetBucket>", agency, target_bucket);
    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "<TargetPrefix>%s</TargetPrefix>", target_prefix ? target_prefix : "");
    if (acl_group)
    {
        ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
            "%s", "<TargetGrants>");
        int i;
        for (i = 0; i < acl_group->acl_grant_count; i++)
        {
            ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, "%s",
                "<Grant><Grantee>");
            obs_acl_grant *grant = &(acl_group->acl_grants[i]);
            switch (grant->grantee_type)
            {
            case OBS_GRANTEE_TYPE_CANONICAL_USER:
                ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
                    "<ID>%s</ID>",
                    grant->grantee.canonical_user.id);
                break;
            default:
                ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
                    "%s", "<Canned>Everyone</Canned>");
                break;
            }
            ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
                "</Grantee><Permission>%s</Permission><Delivered>%s</Delivered></Grant>",
                ((grant->permission == OBS_PERMISSION_READ) ? "READ" :
                (grant->permission == OBS_PERMISSION_WRITE) ? "WRITE" :
                    (grant->permission == OBS_PERMISSION_READ_ACP) ? "READ_ACP" :
                    (grant->permission == OBS_PERMISSION_WRITE_ACP) ? "WRITE_ACP" :
                    (grant->permission == OBS_PERMISSION_FULL_CONTROL) ? "FULL_CONTROL" : "READ"),
                    (grant->bucket_delivered) ? "true" : "false");

        }
        ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
            "%s", "</TargetGrants>");
    }
    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "%s", "</LoggingEnabled>");
    ret = append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
        "%s", "</BucketLoggingStatus>");

    return ret;
}

obs_status generate_logging_xml_document(char *target_bucket, char *target_prefix, char *agency,
    obs_acl_group *acl_group, int *xml_doc_len_return,
    char *xml_document, int xml_doc_buffer_size, obs_use_api use_api)
{
    if (use_api == OBS_USE_API_S3) {
        return generate_logging_xml_document_s3(target_bucket, target_prefix, acl_group, xml_doc_len_return, xml_document, xml_doc_buffer_size);
    }
    else {
        return generate_logging_xml_document_obs(target_bucket, target_prefix, agency, acl_group, xml_doc_len_return, xml_document, xml_doc_buffer_size);
    }
}

void set_bucket_logging_configuration_common(const obs_options *options, char *target_bucket, char *target_prefix, char *agency,
    obs_acl_group *acl_group, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set bucket logging configuration start!");

    set_common_data *bucket_logging_data = (set_common_data*)malloc(sizeof(set_common_data));
    if (!bucket_logging_data)
    {
        COMMLOG(OBS_LOGERROR, "malloc set bucket_logging_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    memset_s(bucket_logging_data, sizeof(set_common_data), 0, sizeof(set_common_data));

    obs_status status = generate_logging_xml_document(target_bucket, target_prefix, agency, acl_group,
        &(bucket_logging_data->xml_document_len),
        bucket_logging_data->xml_document,
        sizeof(bucket_logging_data->xml_document), use_api);
    if (status != OBS_STATUS_OK)
    {
        free(bucket_logging_data);
        bucket_logging_data = NULL;
        (void)(*(handler->complete_callback))(status, 0, 0);
        COMMLOG(OBS_LOGERROR, "generate_storage_class_xml_document failed !");
        return;
    }
    bucket_logging_data->response_properties_callback = handler->properties_callback;
    bucket_logging_data->response_complete_callback = handler->complete_callback;
    bucket_logging_data->callback_data = callback_data;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &set_common_properties_callback;
    params.toObsCallback = &set_common_data_callback;
    params.toObsCallbackTotalSize = bucket_logging_data->xml_document_len;
    params.complete_callback = &set_common_complete_callback;
    params.callback_data = bucket_logging_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "logging";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket logging configuration finish!");
}

void set_bucket_logging_configuration(const obs_options *options, char *target_bucket, char *target_prefix,
    obs_acl_group *acl_group, obs_response_handler *handler, void *callback_data)
{
    set_bucket_logging_configuration_common(options, target_bucket, target_prefix, NULL,
        acl_group, handler, callback_data);
}

void set_bucket_logging_configuration_obs(const obs_options *options, char *target_bucket, char *target_prefix, char *agency,
    obs_acl_group *acl_group, obs_response_handler *handler, void *callback_data)
{
    set_bucket_logging_configuration_common(options, target_bucket, target_prefix, agency,
        acl_group, handler, callback_data);
}