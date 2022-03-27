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

obs_status convert_bucket_logging_grant_s3orobs(convert_bucket_logging_data *convert_data, obs_acl_grant *grant)
{
    errno_t err = EOK;
    if (convert_data->use_api == OBS_USE_API_S3) {
        if (convert_data->email_address[0]) {
            grant->grantee_type = OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL;
            err = strcpy_s(grant->grantee.huawei_customer_by_email.email_address,
                sizeof(grant->grantee.huawei_customer_by_email.email_address),
                convert_data->email_address);
        }
        else_if(convert_data->userId[0] && convert_data->userDisplayName[0]) {
            grant->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
            err = strcpy_s(grant->grantee.canonical_user.id, sizeof(grant->grantee.canonical_user.id),
                convert_data->userId);
            err = strcpy_s(grant->grantee.canonical_user.display_name,
                sizeof(grant->grantee.canonical_user.display_name),
                convert_data->userDisplayName);
        }
        else_if(convert_data->groupUri[0]) {
            if (!strcmp(convert_data->groupUri, ACS_GROUP_AWS_USERS)) {
                grant->grantee_type = OBS_GRANTEE_TYPE_ALL_OBS_USERS;
            }
            else_if(!strcmp(convert_data->groupUri, ACS_GROUP_ALL_USERS)) {
                grant->grantee_type = OBS_GRANTEE_TYPE_ALL_USERS;
            }
            else
            {
                return OBS_STATUS_BadGrantee;
            }
        }
        else {
            return OBS_STATUS_BadGrantee;
        }
    }
    else {
        grant->grantee_type = OBS_GRANTEE_TYPE_ALL_USERS;
        if (convert_data->userId[0]) {
            grant->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
            err = strcpy_s(grant->grantee.canonical_user.id, sizeof(grant->grantee.canonical_user.id),
                convert_data->userId);
        }
    }
    CheckAndLogNoneZero(err, "strcpy_s", __FUNCTION__, __LINE__);
    return OBS_STATUS_OK;
}

obs_status convert_bucket_logging_grant_permission(convert_bucket_logging_data *convert_data, obs_acl_grant *grant) {
    if (!strcmp(convert_data->permission, "READ")) {
        grant->permission = OBS_PERMISSION_READ;
    }
    else_if(!strcmp(convert_data->permission, "WRITE")) {
        grant->permission = OBS_PERMISSION_WRITE;
    }
    else_if(!strcmp(convert_data->permission, "READ_ACP")) {
        grant->permission = OBS_PERMISSION_READ_ACP;
    }
    else_if(!strcmp(convert_data->permission, "WRITE_ACP")) {
        grant->permission = OBS_PERMISSION_WRITE_ACP;
    }
    else_if(!strcmp(convert_data->permission, "FULL_CONTROL")) {
        grant->permission = OBS_PERMISSION_FULL_CONTROL;
    }
    else
    {
        return OBS_STATUS_BadPermission;
    }

    return OBS_STATUS_OK;
}

obs_status convert_bucket_logging_grant(const char *element_path, convert_bucket_logging_data *convert_data)
{
    if (strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
        "TargetGrants/Grant"))
    {
        COMMLOG(OBS_LOGINFO, "Logging without grant!");
        return OBS_STATUS_OK;
    }

    if (*(convert_data->acl_grant_count_return) == OBS_MAX_ACL_GRANT_COUNT) {
        return OBS_STATUS_TooManyGrants;
    }

    obs_acl_grant *grant = &(convert_data->acl_grants[*(convert_data->acl_grant_count_return)]);

    obs_status status = convert_bucket_logging_grant_s3orobs(convert_data, grant);
    if (status != OBS_STATUS_OK) {
        return status;
    }

    status = convert_bucket_logging_grant_permission(convert_data, grant);
    if (status != OBS_STATUS_OK) {
        return status;
    }
    (*(convert_data->acl_grant_count_return))++;
    string_buffer_initialize(convert_data->email_address);
    string_buffer_initialize(convert_data->userId);
    string_buffer_initialize(convert_data->userDisplayName);
    string_buffer_initialize(convert_data->groupUri);
    string_buffer_initialize(convert_data->permission);
    return OBS_STATUS_OK;
}

obs_status convert_bucket_logging_xml_callback(const char *element_path,
    const char *data, int data_len,
    void *callback_data)
{
    convert_bucket_logging_data *convert_data = (convert_bucket_logging_data *)callback_data;
    obs_status status = OBS_STATUS_OK;
    int fit;

    if (data)
    {
        if (!strcmp(element_path, "BucketLoggingStatus/Agency"))
        {
            convert_data->agencyReturnLen +=
                snprintf_s(&(convert_data->agencyReturn
                    [convert_data->agencyReturnLen]),
                    convert_data->agencyReturnSize,
                    convert_data->agencyReturnSize - convert_data->agencyReturnLen - 1,
                    "%.*s", data_len, data);
            if (convert_data->agencyReturnLen >= convert_data->agencyReturnSize) {
                return OBS_STATUS_InvalidParameter;
            }
        }
        else_if(!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
            "TargetBucket"))
        {
            convert_data->targetBucketReturnLen +=
                snprintf_s(&(convert_data->targetBucketReturn
                    [convert_data->targetBucketReturnLen]),
                    convert_data->targetBucketReturnSize,
                    convert_data->targetBucketReturnSize - convert_data->targetBucketReturnLen - 1,
                    "%.*s", data_len, data);
            if (convert_data->targetBucketReturnLen >= convert_data->targetBucketReturnSize) {
                return OBS_STATUS_InvalidParameter;
            }
        }
        else_if(!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
            "TargetPrefix"))
        {
            convert_data->targetPrefixReturnLen +=
                snprintf_s(&(convert_data->targetPrefixReturn
                    [convert_data->targetPrefixReturnLen]),
                    convert_data->targetPrefixReturnSize,
                    convert_data->targetPrefixReturnSize - convert_data->targetPrefixReturnLen - 1,
                    "%.*s", data_len, data);
            if (convert_data->targetPrefixReturnLen >= convert_data->targetPrefixReturnSize) {
                return OBS_STATUS_InvalidParameter;
            }
        }
        else_if(!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
            "TargetGrants/Grant/Grantee/EmailAddress"))
        {

            string_buffer_append(convert_data->email_address, data, data_len, fit);
            if (!fit) {
                return OBS_STATUS_EmailAddressTooLong;
            }
        }
        else if (!strcmp(element_path,
            "BucketLoggingStatus/LoggingEnabled/TargetGrants/Grant/"
            "Grantee/ID"))
        {
            string_buffer_append(convert_data->userId, data, data_len, fit);
            if (!fit) {
                return OBS_STATUS_UserIdTooLong;
            }
        }
        else_if(!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
            "TargetGrants/Grant/Grantee/DisplayName"))
        {
            string_buffer_append(convert_data->userDisplayName, data, data_len, fit);
            if (!fit) {
                return OBS_STATUS_UserDisplayNameTooLong;
            }
        }
        else_if(!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
            "TargetGrants/Grant/Grantee/URI"))
        {
            string_buffer_append(convert_data->groupUri, data, data_len, fit);
            if (!fit) {
                return OBS_STATUS_GroupUriTooLong;
            }
        }
        else_if(!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
            "TargetGrants/Grant/Permission"))
        {
            string_buffer_append(convert_data->permission, data, data_len, fit);
            if (!fit) {
                return OBS_STATUS_PermissionTooLong;
            }
        }
    }

    status = convert_bucket_logging_grant(element_path, convert_data);
    return status;
}

obs_status convert_bls(const char *blsXml, bucket_logging_message *logging_message, obs_use_api use_api)
{
    convert_bucket_logging_data data;

    data.targetBucketReturn = logging_message->target_bucket;
    data.targetBucketReturnLen = 0;
    data.targetBucketReturnSize = logging_message->target_bucket_size;
    data.targetPrefixReturn = logging_message->target_prefix;
    data.targetPrefixReturnLen = 0;
    data.targetPrefixReturnSize = logging_message->target_prefix_size;
    data.acl_grant_count_return = logging_message->acl_grant_count;
    data.agencyReturn = logging_message->agency;
    data.agencyReturnLen = 0;
    data.agencyReturnSize = logging_message->agency_size;
    data.acl_grants = logging_message->acl_grants;
    string_buffer_initialize(data.email_address);
    string_buffer_initialize(data.userId);
    string_buffer_initialize(data.userDisplayName);
    string_buffer_initialize(data.groupUri);
    string_buffer_initialize(data.permission);
    data.use_api = use_api;

    simple_xml xml;
    simplexml_initialize(&xml, &convert_bucket_logging_xml_callback, &data);

    obs_status status = simplexml_add(&xml, blsXml, strlen(blsXml));

    simplexml_deinitialize(&xml);

    return status;
}


void get_bucket_logging_complete_callback(obs_status status,
    const obs_error_details *error_details, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_logging_data *logging_data = (get_bucket_logging_data *)callback_data;

    if (status == OBS_STATUS_OK)
    {
        status = convert_bls(logging_data->xml_document, logging_data->logging_message, logging_data->use_api);
    }

    (void)(*(logging_data->response_complete_callback))
        (status, error_details, logging_data->callback_data);

    free(logging_data);
    logging_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}

obs_status get_bucket_logging_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    get_bucket_logging_data *logging_data = (get_bucket_logging_data *)callback_data;
    if (logging_data->response_properties_callback)
    {
        return (*(logging_data->response_properties_callback))
            (response_properties, logging_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_logging_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_bucket_logging_data *logging_data = (get_bucket_logging_data *)callback_data;
    int fit;
    string_buffer_append(logging_data->xml_document, buffer, buffer_size, fit);

    return fit ? OBS_STATUS_OK : OBS_STATUS_XmlDocumentTooLarge;
}


void get_bucket_logging_configuration(const obs_options *options, obs_response_handler *handler,
    bucket_logging_message *logging_message_data, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get bucket logging conf start!");
    get_bucket_logging_data *logging_data =
        (get_bucket_logging_data*)malloc(sizeof(get_bucket_logging_data));
    if (!logging_data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc get logging_datafailed !");
        return;
    }
    memset_s(logging_data, sizeof(get_bucket_logging_data),
        0, sizeof(get_bucket_logging_data));

    logging_data->response_properties_callback = handler->properties_callback;
    logging_data->response_complete_callback = handler->complete_callback;
    logging_data->callback_data = callback_data;

    logging_data->logging_message = logging_message_data;
    string_buffer_initialize(logging_data->xml_document);
    *(logging_message_data->acl_grant_count) = 0;
    logging_data->use_api = use_api;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_logging_properties_callback;
    params.fromObsCallback = &get_bucket_logging_data_callback;
    params.complete_callback = &get_bucket_logging_complete_callback;
    params.callback_data = logging_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "logging";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket logging conf finish!");
}