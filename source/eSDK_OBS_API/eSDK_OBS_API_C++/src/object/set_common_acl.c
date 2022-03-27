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
#include "request_util.h"
#include <openssl/md5.h> 

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
obs_status generate_acl_xml_document_s3_switchGranteeType(const obs_acl_grant *grant,
    char *xmlDocument, int *xmlDocumentLenReturn, int xmlDocumentBufferSize)
{
    switch (grant->grantee_type)
    {
    case OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL:
        append("AmazonCustomerByEmail\"><EmailAddress>%s</EmailAddress>",
            grant->grantee.huawei_customer_by_email.email_address);
        break;
    case OBS_GRANTEE_TYPE_CANONICAL_USER:
        append("CanonicalUser\"><ID>%s</ID><DisplayName>%s</DisplayName>",
            grant->grantee.canonical_user.id,
            grant->grantee.canonical_user.display_name);
        break;
    case OBS_GRANTEE_TYPE_ALL_OBS_USERS:
        append("Group\"><URI>%s</URI>", ACS_GROUP_AWS_USERS);
        break;
    case OBS_GRANTEE_TYPE_ALL_USERS:
        append("Group\"><URI>%s</URI>", ACS_GROUP_ALL_USERS);
        break;
    default:
        append("Group\"><URI>%s</URI>", ACS_GROUP_LOG_DELIVERY);
        break;
    }
    return OBS_STATUS_OK;
}

static obs_status generate_acl_xml_document_s3(const char *owner_id,
    const char *owner_display_name,
    int aclGrantCount,
    const obs_acl_grant *acl_grants,
    int *xmlDocumentLenReturn,
    char *xmlDocument,
    int xmlDocumentBufferSize)
{
    *xmlDocumentLenReturn = 0;
    append("<AccessControlPolicy><Owner><ID>%s</ID><DisplayName>%s"
        "</DisplayName></Owner><AccessControlList>", owner_id,
        owner_display_name);

    int i;
    for (i = 0; i < aclGrantCount; i++) {
        append("%s", "<Grant><Grantee xmlns:xsi=\"http://www.w3.org/2001/"
            "XMLSchema-instance\" xsi:type=\"");
        const obs_acl_grant *grant = &(acl_grants[i]);
        obs_status ret = generate_acl_xml_document_s3_switchGranteeType(
            grant, xmlDocument, xmlDocumentLenReturn, xmlDocumentBufferSize);
        if (ret != OBS_STATUS_OK)
            return ret;
        append("</Grantee><Permission>%s</Permission></Grant>",
            ((grant->permission == OBS_PERMISSION_READ) ? "READ" :
            (grant->permission == OBS_PERMISSION_WRITE) ? "WRITE" :
                (grant->permission == OBS_PERMISSION_READ_ACP) ? "READ_ACP" :
                (grant->permission == OBS_PERMISSION_WRITE_ACP) ? "WRITE_ACP" :
                (grant->permission == OBS_PERMISSION_FULL_CONTROL) ? "FULL_CONTROL" : "READ"));
    }

    append("%s", "</AccessControlList></AccessControlPolicy>");

    return OBS_STATUS_OK;
}

obs_status generate_acl_xml_document_obs_switchGranteeType(const obs_acl_grant *grant,
    char *xmlDocument, int *xmlDocumentLenReturn, int xmlDocumentBufferSize)
{
    switch (grant->grantee_type)
    {
    case OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL:
        append("<EmailAddress>%s</EmailAddress>",
            grant->grantee.huawei_customer_by_email.email_address);
        break;
    case OBS_GRANTEE_TYPE_CANONICAL_USER:
        append("<ID>%s</ID>",
            grant->grantee.canonical_user.id);
        break;
    default:
        append("%s", "<Canned>Everyone</Canned>");
        break;
    }
    return OBS_STATUS_OK;
}

static obs_status generate_acl_xml_document_obs(const char *owner_id,
    obs_object_delivered object_delivered,
    int aclGrantCount,
    const obs_acl_grant *acl_grants,
    int *xmlDocumentLenReturn,
    char *xmlDocument,
    int xmlDocumentBufferSize,
    obs_type_acl type)
{
    *xmlDocumentLenReturn = 0;

    if (type == TYPE_OBJECT_ACL)
    {
        append("<AccessControlPolicy><Owner><ID>%s</ID></Owner>", owner_id);
        append("<Delivered>%s</Delivered><AccessControlList>", ((object_delivered == OBJECT_DELIVERED_TRUE) ? "true" : "false"));     // For object, true is the default value for delivered.
    }
    else
    {
        append("<AccessControlPolicy><Owner><ID>%s</ID></Owner><AccessControlList>", owner_id);
    }

    int i;
    for (i = 0; i < aclGrantCount; i++) {
        append("%s", "<Grant><Grantee>");
        const obs_acl_grant *grant = &(acl_grants[i]);
        obs_status ret = generate_acl_xml_document_obs_switchGranteeType(
            grant, xmlDocument, xmlDocumentLenReturn, xmlDocumentBufferSize);
        if (ret != OBS_STATUS_OK)
            return ret;
        append("</Grantee><Permission>%s</Permission>",
            ((grant->permission == OBS_PERMISSION_READ) ? "READ" :
            (grant->permission == OBS_PERMISSION_WRITE) ? "WRITE" :
                (grant->permission == OBS_PERMISSION_READ_ACP) ? "READ_ACP" :
                (grant->permission == OBS_PERMISSION_WRITE_ACP) ? "WRITE_ACP" :
                (grant->permission == OBS_PERMISSION_FULL_CONTROL) ? "FULL_CONTROL" : "READ"));
        if (type != TYPE_OBJECT_ACL)
        {
            append("<Delivered>%s</Delivered>", ((grant->bucket_delivered == BUCKET_DELIVERED_FALSE) ? "false" : "true"));  								// For bucket, false is the default value for delivered.
        }
        append("%s", "</Grant>");
    }

    append("%s", "</AccessControlList></AccessControlPolicy>");

    return OBS_STATUS_OK;
}

static obs_status generate_acl_xml_document(const char *owner_id,
    const char *owner_display_name,
    obs_object_delivered object_delivered,
    int aclGrantCount,
    const obs_acl_grant *acl_grants,
    int *xmlDocumentLenReturn,
    char *xmlDocument,
    int xmlDocumentBufferSize,
    obs_type_acl type,
    obs_use_api use_api)
{
    if (use_api == OBS_USE_API_S3) {
        return generate_acl_xml_document_s3(owner_id, owner_display_name, aclGrantCount, acl_grants, xmlDocumentLenReturn, xmlDocument, xmlDocumentBufferSize);
    }
    else {
        return generate_acl_xml_document_obs(owner_id, object_delivered, aclGrantCount, acl_grants, xmlDocumentLenReturn, xmlDocument, xmlDocumentBufferSize, type);
    }
}

static int setAclDataCallback(int buffer_size, char *buffer, void *callback_data)
{
    set_acl_data *paData = (set_acl_data *)callback_data;

    int remaining = (paData->aclXmlDocumentLen -
        paData->aclXmlDocumentBytesWritten);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy) {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, toCopy, &(paData->aclXmlDocument
        [paData->aclXmlDocumentBytesWritten]), toCopy);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "setAclDataCallback: memcpy_s failed!\n");
        return 0;
    }

    paData->aclXmlDocumentBytesWritten += toCopy;

    return toCopy;
}

static void setAclCompleteCallback(obs_status requestStatus,
    const obs_error_details *s3ErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_acl_data *paData = (set_acl_data *)callback_data;

    (void)(*(paData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, paData->callback_data);
    free(paData);
    paData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}

void set_common_acl(const obs_options *options, manager_acl_info *aclinfo, obs_type_acl type, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter set_object_acl successfully !");
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;
    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if (*(aclinfo->acl_grant_count_return) > OBS_MAX_ACL_GRANT_COUNT) {
        (void)(*(handler->complete_callback))
            (OBS_STATUS_TooManyGrants, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Input param aclGrantCount is greater than S3_MAX_ACL_GRANT_COUNT(100) !");
        return;
    }
    if (aclinfo->object_info.version_id) {
        safe_append("versionId", aclinfo->object_info.version_id, handler->complete_callback);
    }
    set_acl_data *data = (set_acl_data *)malloc(sizeof(set_acl_data));
    if (!data) {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc SetAclData failed !");
        return;
    }
    memset_s(data, sizeof(set_acl_data), 0, sizeof(set_acl_data));
    obs_status status = generate_acl_xml_document(aclinfo->owner_id, aclinfo->owner_display_name, aclinfo->object_delivered,
        *(aclinfo->acl_grant_count_return), aclinfo->acl_grants,
        &(data->aclXmlDocumentLen), data->aclXmlDocument,
        sizeof(data->aclXmlDocument), type, use_api);
    if (status != OBS_STATUS_OK) {
        free(data);
        data = NULL;
        (void)(*(handler->complete_callback))(status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "generateAclXmlDocument failed");
        return;
    }
    data->responsePropertiesCallback = handler->properties_callback;
    data->responseCompleteCallback = handler->complete_callback;
    data->callback_data = callback_data;
    data->aclXmlDocumentBytesWritten = 0;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_put;
    params.key = aclinfo->object_info.key;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.subResource = "acl";
    params.toObsCallback = &setAclDataCallback;
    params.toObsCallbackTotalSize = data->aclXmlDocumentLen;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = &setAclCompleteCallback;
    params.callback_data = data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave set_object_acl successfully !");
}
