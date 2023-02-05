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

obs_permission convert_obs_permission_str(const char *permission)
{
    obs_permission tmp_permission = OBS_PERMISSION_BUTT;
    if (!strcmp(permission, "READ")) {
        tmp_permission = OBS_PERMISSION_READ;
    }
    else if (!strcmp(permission, "WRITE")) {
        tmp_permission = OBS_PERMISSION_WRITE;
    }
    else if (!strcmp(permission, "READ_ACP")) {
        tmp_permission = OBS_PERMISSION_READ_ACP;
    }
    else if (!strcmp(permission, "WRITE_ACP")) {
        tmp_permission = OBS_PERMISSION_WRITE_ACP;
    }
    else if (!strcmp(permission, "FULL_CONTROL")) {
        tmp_permission = OBS_PERMISSION_FULL_CONTROL;
    }

    return tmp_permission;
}

obs_grantee_type convert_group_uri_str(const char *group_uri_str)
{
    obs_grantee_type tmp_grantee_type = OBS_GRANTEE_TYPE_BUTT;
    if (!strcmp(group_uri_str, ACS_GROUP_AWS_USERS)) {
        tmp_grantee_type = OBS_GRANTEE_TYPE_ALL_OBS_USERS;
    }
    else if (!strcmp(group_uri_str, ACS_GROUP_ALL_USERS)) {
        tmp_grantee_type = OBS_GRANTEE_TYPE_ALL_USERS;
    }
    else if (!strcmp(group_uri_str, ACS_GROUP_LOG_DELIVERY)) {
        tmp_grantee_type = OBS_GRANTEE_TYPE_LOG_DELIVERY;
    }
    return tmp_grantee_type;
}

obs_bucket_delivered convert_obs_bucket_delivered_str(const char *delivered)
{
    obs_bucket_delivered tmp_delivered = BUCKET_DELIVERED_FALSE;

    if (!strcmp(delivered, "true") || !strcmp(delivered, "TRUE")) {
        tmp_delivered = BUCKET_DELIVERED_TRUE;
    }

    return tmp_delivered;
}

obs_object_delivered convert_obs_object_delivered_str(const char *delivered)
{
    obs_object_delivered tmp_delivered = OBJECT_DELIVERED_TRUE;

    if (!strcmp(delivered, "false") || !strcmp(delivered, "FALSE")) {
        tmp_delivered = OBJECT_DELIVERED_FALSE;
    }

    return tmp_delivered;
}


obs_status parse_xml_convert_acl_noGrant(convert_acl_data_info *caData, const char* elementPath,
    const char *data, int dataLen)
{
    if (!strcmp(elementPath, "AccessControlPolicy/Owner/ID"))
    {
        caData->ownerIdLen +=
            snprintf_s(&(caData->owner_id[caData->ownerIdLen]),
                OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE + 1 - caData->ownerIdLen,
                OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE - caData->ownerIdLen - 1,
                "%.*s", dataLen, data);
        if (caData->ownerIdLen >= OBS_MAX_GRANTEE_USER_ID_SIZE) {
            return OBS_STATUS_UserIdTooLong;
        }
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/Owner/DisplayName"))
    {
        caData->ownerDisplayNameLen +=
            snprintf_s(&(caData->owner_display_name[caData->ownerDisplayNameLen]),
                OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE + 1 - caData->ownerDisplayNameLen,
                OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE - caData->ownerDisplayNameLen - 1,
                "%.*s", dataLen, data);
        if (caData->ownerDisplayNameLen >= OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE) {
            return OBS_STATUS_UserDisplayNameTooLong;
        }
    }
    return OBS_STATUS_OK;
}

int parse_xml_convert_acl_withGrant(convert_acl_data_info *caData, const char* elementPath,
    const char *data, int dataLen, int fit)
{
    if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/EmailADDress")) {
        string_buffer_append(caData->email_address, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/ID"))
    {
        string_buffer_append(caData->userId, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/DisplayName"))
    {
        string_buffer_append(caData->userDisplayName, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/URI"))
    {
        string_buffer_append(caData->groupUri, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/Permission"))
    {
        string_buffer_append(caData->permission, data, dataLen, fit);
    }
    return fit;
}


obs_status parse_xml_convert_acl(convert_acl_data_info *caData, const char *elementPath,
    const char *data, int dataLen)
{
    int fit = 1;
    if (strstr(elementPath, "AccessControlPolicy/AccessControlList/Grant/"))
    {
        parse_xml_convert_acl_withGrant(caData, elementPath, data, dataLen, fit);
    }
    else
    {
        return parse_xml_convert_acl_noGrant(caData, elementPath, data, dataLen);
    }

    if (!fit)
    {
        return OBS_STATUS_EmailAddressTooLong;
    }
    return OBS_STATUS_OK;
}

obs_status convert_acl_xml_callback_s3_nodata(convert_acl_data_info *caData)
{
    if (*(caData->acl_grant_count_return) == OBS_MAX_ACL_GRANT_COUNT) {
        return OBS_STATUS_TooManyGrants;
    }

    obs_acl_grant *grant = &(caData->acl_grants[*(caData->acl_grant_count_return)]);

    errno_t err = EOK;
    if (caData->email_address[0]) {
        grant->grantee_type = OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL;
        err = strcpy_s(grant->grantee.huawei_customer_by_email.email_address,
            sizeof(grant->grantee.huawei_customer_by_email.email_address),
            caData->email_address);
    }
    else if (caData->userId[0] && caData->userDisplayName[0])
    {
        grant->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
        err = strcpy_s(grant->grantee.canonical_user.id, sizeof(grant->grantee.canonical_user.id), caData->userId);
        err = strcpy_s(grant->grantee.canonical_user.display_name,
            sizeof(grant->grantee.canonical_user.display_name),
            caData->userDisplayName);
    }
    else if (caData->groupUri[0])
    {
        grant->grantee_type = convert_group_uri_str(caData->groupUri);
    }
    else
    {
        return OBS_STATUS_BadGrantee;
    }
    CheckAndLogNoneZero(err, "strcpy_s", __FUNCTION__, __LINE__);

    grant->permission = convert_obs_permission_str(caData->permission);
    (*(caData->acl_grant_count_return))++;

    string_buffer_initialize(caData->email_address);
    string_buffer_initialize(caData->userId);
    string_buffer_initialize(caData->userDisplayName);
    string_buffer_initialize(caData->groupUri);
    string_buffer_initialize(caData->permission);
    return OBS_STATUS_OK;
}


static obs_status convert_acl_xml_callback_s3(const char *elementPath,
    const char *data, int dataLen,
    void *callback_data)
{
    convert_acl_data_info *caData = (convert_acl_data_info *)callback_data;

    obs_status ret_status;

    if (data) {
        ret_status = parse_xml_convert_acl(caData, elementPath, data, dataLen);
        if (OBS_STATUS_OK != ret_status)
        {
            COMMLOG(OBS_LOGERROR, "parse_xml_convert_acl failed.");
            return ret_status;
        }
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant"))
    {
        return convert_acl_xml_callback_s3_nodata(caData);
    }

    return OBS_STATUS_OK;
}

obs_status convert_acl_xml_callback_obs_withData(const char *elementPath,
    const char *data, int dataLen,
    convert_acl_data_info *caData,
    char *object_delivered_string,
    int object_delivered_stringLen,
    int fit)
{
    if (!strcmp(elementPath, "AccessControlPolicy/Owner/ID")) {
        caData->ownerIdLen +=
            snprintf_s(&(caData->owner_id[caData->ownerIdLen]),
                OBS_MAX_GRANTEE_USER_ID_SIZE + 1 - caData->ownerIdLen,
                OBS_MAX_GRANTEE_USER_ID_SIZE - caData->ownerIdLen - 1,
                "%.*s", dataLen, data);
        if (caData->ownerIdLen >= OBS_MAX_GRANTEE_USER_ID_SIZE)
        {
            return OBS_STATUS_UserIdTooLong;
        }
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/Delivered"))
    {
        string_buffer_append(object_delivered_string, data, dataLen, fit);
        *(caData->object_delivered) = convert_obs_object_delivered_str(object_delivered_string);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/ID"))
    {
        string_buffer_append(caData->userId, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/Canned"))
    {
        string_buffer_append(caData->groupUri, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Permission"))
    {
        string_buffer_append(caData->permission, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Delivered"))
    {
        string_buffer_append(caData->bucket_delivered, data, dataLen, fit);
    }

    if (!fit) {
        return OBS_STATUS_UserIdTooLong;
    }
    return OBS_STATUS_OK;
}

obs_status convert_acl_xml_callback_obs_noData(convert_acl_data_info *caData)
{
    if (*(caData->acl_grant_count_return) == OBS_MAX_ACL_GRANT_COUNT) {
        return OBS_STATUS_TooManyGrants;
    }
    obs_acl_grant *grant = &(caData->acl_grants[*(caData->acl_grant_count_return)]);

    if (caData->userId[0]) {
        grant->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
        errno_t err = strcpy_s(grant->grantee.canonical_user.id, sizeof(grant->grantee.canonical_user.id), caData->userId);
        CheckAndLogNoneZero(err, "strcpy_s", __FUNCTION__, __LINE__);
    }
    else if (caData->groupUri[0])
    {
        grant->grantee_type = OBS_GRANTEE_TYPE_ALL_USERS;
    }
    else
    {
        return OBS_STATUS_BadGrantee;
    }
    grant->permission = convert_obs_permission_str(caData->permission);
    grant->bucket_delivered = convert_obs_bucket_delivered_str(caData->bucket_delivered);
    (*(caData->acl_grant_count_return))++;

    string_buffer_initialize(caData->userId);
    string_buffer_initialize(caData->groupUri);
    string_buffer_initialize(caData->permission);
    string_buffer_initialize(caData->bucket_delivered);

    return OBS_STATUS_OK;
}
//
static obs_status convert_acl_xml_callback_obs(const char *elementPath,
    const char *data, int dataLen,
    void *callback_data)
{
    convert_acl_data_info *caData = (convert_acl_data_info *)callback_data;
    string_buffer(object_delivered_string, 32);
    string_buffer_initialize(object_delivered_string);

    int fit = 1;

    if (data) {
        return convert_acl_xml_callback_obs_withData(elementPath, data, dataLen,
            caData, object_delivered_string, object_delivered_stringLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant")) {
        return convert_acl_xml_callback_obs_noData(caData);
    }

    return OBS_STATUS_OK;
}


static obs_status convert_acl_xml_callback(const char *elementPath,
    const char *data, int dataLen,
    void *callback_data)
{

    convert_acl_data_info *caData = (convert_acl_data_info *)callback_data;
    if (caData->use_api == OBS_USE_API_S3) {
        return convert_acl_xml_callback_s3(elementPath, data, dataLen, callback_data);
    }
    else {
        return convert_acl_xml_callback_obs(elementPath, data, dataLen, callback_data);
    }
}


obs_status obs_convert_acl(const char *aclXml, char *owner_id, char *owner_display_name, obs_object_delivered *object_delivered,
    int *acl_grant_count_return, obs_acl_grant *acl_grants, obs_use_api use_api)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    convert_acl_data_info data;
    memset_s(&data, sizeof(data), 0, sizeof(convert_acl_data_info));

    data.owner_id = owner_id;
    data.ownerIdLen = 0;
    data.owner_id[0] = 0;
    data.owner_display_name = owner_display_name;
    data.ownerDisplayNameLen = 0;
    data.owner_display_name[0] = 0;
    data.acl_grant_count_return = acl_grant_count_return;
    data.acl_grants = acl_grants;
    *acl_grant_count_return = 0;
    data.object_delivered = object_delivered;
    string_buffer_initialize(data.email_address);
    string_buffer_initialize(data.userId);
    string_buffer_initialize(data.userDisplayName);
    string_buffer_initialize(data.groupUri);
    string_buffer_initialize(data.permission);
    data.use_api = use_api;
    simple_xml simpleXml;
    memset_s(&simpleXml, sizeof(simpleXml), 0, sizeof(simple_xml));
    simplexml_initialize(&simpleXml, &convert_acl_xml_callback, &data);

    obs_status status = simplexml_add(&simpleXml, aclXml, strlen(aclXml));

    simplexml_deinitialize(&simpleXml);

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
    return status;
}


static obs_status getAclDataCallback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_acl_data *gaData = (get_acl_data *)callback_data;

    int fit;

    string_buffer_append(gaData->aclXmlDocument, buffer, buffer_size, fit);

    return fit ? OBS_STATUS_OK : OBS_STATUS_XmlDocumentTooLarge;
}

static obs_status getAclPropertiesCallback(
    const obs_response_properties *responseProperties, void *callback_data)
{
    get_acl_data *gaData = (get_acl_data *)callback_data;
    if (gaData->responsePropertiesCallback)
    {
        return (*(gaData->responsePropertiesCallback))(responseProperties,
            gaData->callback_data);
    }

    return OBS_STATUS_OK;
}

static void getAclCompleteCallback(obs_status requestStatus,
    const obs_error_details *s3ErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_acl_data *gaData = (get_acl_data *)callback_data;

    if (requestStatus == OBS_STATUS_OK) {
        requestStatus = obs_convert_acl
        (gaData->aclXmlDocument, gaData->owner_id, gaData->owner_display_name, gaData->object_delivered,
            gaData->acl_grant_count_return, gaData->acl_grants, gaData->use_api);
    }

    (void)(*(gaData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, gaData->callback_data);

    free(gaData);
    gaData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}

void get_object_acl(const obs_options *options, manager_acl_info *aclinfo, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter get_object_acl successfully !");
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;
    if (aclinfo->object_info.version_id) {
        safe_append("versionId", aclinfo->object_info.version_id, strlen(aclinfo->object_info.version_id), handler->complete_callback);
    }
    get_acl_data *gaData = (get_acl_data *)malloc(sizeof(get_acl_data));
    if (!gaData) {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc GetAclData failed!");
        return;
    }
    memset_s(gaData, sizeof(get_acl_data), 0, sizeof(get_acl_data));
    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        free(gaData);
        gaData = NULL;
        return;
    }

    gaData->responsePropertiesCallback = handler->properties_callback;
    gaData->responseCompleteCallback = handler->complete_callback;
    gaData->callback_data = callback_data;

    gaData->acl_grant_count_return = aclinfo->acl_grant_count_return;
    gaData->acl_grants = aclinfo->acl_grants;
    gaData->owner_id = aclinfo->owner_id;
    gaData->owner_display_name = aclinfo->owner_display_name;
    gaData->object_delivered = &(aclinfo->object_delivered);
    string_buffer_initialize(gaData->aclXmlDocument);                // gaData->aclXmlDocument[0] = 0; gaData->aclXmlDocumentLen = 0;
    *(aclinfo->acl_grant_count_return) = 0;
    gaData->use_api = use_api;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_get;
    params.key = aclinfo->object_info.key;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.subResource = "acl";
    params.fromObsCallback = &getAclDataCallback;
    params.properties_callback = &getAclPropertiesCallback;
    params.complete_callback = &getAclCompleteCallback;
    params.callback_data = gaData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave get_object_acl successfully !");
}