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

static obs_status generateRestoreXmlDocument(const char *pcDays,
    obs_tier enTier,
    int *xmlDocumentLenReturn,
    char *xmlDocument,
    int xmlDocumentBufferSize)
{
    char *pcTierString = NULL;
    *xmlDocumentLenReturn = 0;

    appendXmlDocument("%s", "<RestoreRequest>");

    appendXmlDocument("<Days>%s</Days>", pcDays);

    switch (enTier)
    {
    case OBS_TIER_EXPEDITED:
        pcTierString = "Expedited";
        break;
    case OBS_TIER_BULK:
        pcTierString = "Bulk";
        break;
    case OBS_TIER_STANDARD:
        pcTierString = "Standard";
        break;
    default:
        pcTierString = "NULL";
        break;
    }

    if (enTier == OBS_TIER_EXPEDITED || enTier == OBS_TIER_STANDARD || enTier == OBS_TIER_BULK)
    {
        appendXmlDocument("<GlacierJobParameters><Tier>%s</Tier></GlacierJobParameters>",
            pcTierString);
    }

    appendXmlDocument("%s", "</RestoreRequest>");

    return OBS_STATUS_OK;
}


static obs_status generateRestoreXmlDocument_obs(const char *pcDays,
    obs_tier enTier,
    int *xmlDocumentLenReturn,
    char *xmlDocument,
    int xmlDocumentBufferSize)
{
    char *pcTierString = NULL;
    *xmlDocumentLenReturn = 0;

    appendXmlDocument("%s", "<RestoreRequest>");

    appendXmlDocument("<Days>%s</Days>", pcDays);

    switch (enTier)
    {
    case OBS_TIER_EXPEDITED:
        pcTierString = "Expedited";
        break;
    case OBS_TIER_STANDARD:
        pcTierString = "Standard";
        break;
    default:
        pcTierString = "NULL";
        break;
    }

    if (enTier == OBS_TIER_EXPEDITED || enTier == OBS_TIER_STANDARD)
    {
        appendXmlDocument("<RestoreJob><Tier>%s</Tier></RestoreJob>", pcTierString);
    }

    appendXmlDocument("%s", "</RestoreRequest>");

    return OBS_STATUS_OK;
}

int set_data_callback(int iBufferSize, char *pcBuffer, void *callback_data)
{
    int iRemaining = 0;
    int iRet = 0;
    set_sal_data *pstData = NULL;

    pstData = (set_sal_data *)callback_data;
    iRemaining = (pstData->salXmlDocumentLen -
        pstData->salXmlDocumentBytesWritten);
    iRet = iBufferSize > iRemaining ? iRemaining : iBufferSize;
    if (!iRet)
    {
        return 0;
    }
    errno_t err = EOK;
    err = memcpy_s(pcBuffer, iBufferSize, &(pstData->salXmlDocument
        [pstData->salXmlDocumentBytesWritten]), iRet);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "set_data_callback: memcpy_s failed!");
        return 0;
    }
    pstData->salXmlDocumentBytesWritten += iRet;
    return iRet;
}

static obs_status restoreObjectPropertiesCallback(
    const obs_response_properties *responseProperties, void *callback_data)
{
    set_sal_data *pstData = (set_sal_data *)callback_data;
    if (pstData->responsePropertiesCallback)
    {
        return (*(pstData->responsePropertiesCallback))(responseProperties,
            pstData->callback_data);
    }

    return OBS_STATUS_OK;
}

void setCompleteCallback(obs_status enRequestStatus,
    const obs_error_details *pstS3ErrorDetails,
    void *callback_data)
{
    set_sal_data *pstData = NULL;

    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    pstData = (set_sal_data *)callback_data;
    (void)(*(pstData->responseCompleteCallback))
        (enRequestStatus, pstS3ErrorDetails, pstData->callback_data);
    free(pstData);
    pstData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

int restore_object_optionsSet(const obs_options *options, obs_object_info *object_info,
    const char *days, const obs_response_handler *handler, void *callback_data)
{
    int ret = 0;
    if (!options->bucket_options.bucket_name) {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        ret = 1;
        return ret;
    }
    if (NULL == object_info->key || !strlen(object_info->key))
    {
        COMMLOG(OBS_LOGERROR, "key is NULL!");
        ret = 1;
        return ret;
    }
    if (NULL == days || !strlen(days))
    {
        COMMLOG(OBS_LOGERROR, "days is NULL!");
        ret = 1;
        return ret;
    }
    return ret;
}


void restore_object(const obs_options *options, obs_object_info *object_info, const char *days,
    obs_tier tier, const obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter restore_object successfully !");
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;

    if (restore_object_optionsSet(options, object_info, days, handler, callback_data))
        return;
    if (object_info->version_id)
    {
        safe_append("versionId", object_info->version_id, handler->complete_callback);
    }
    set_sal_data *data = (set_sal_data *)malloc(sizeof(set_sal_data));
    if (!data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc SetSalData failed !");
        return;
    }

    obs_status status;
    if (options->request_options.auth_switch == OBS_OBS_TYPE)
    {
        status = generateRestoreXmlDocument_obs(days, tier,
            &(data->salXmlDocumentLen), data->salXmlDocument,
            sizeof(data->salXmlDocument));
    }
    else
    {
        status = generateRestoreXmlDocument(days, tier,
            &(data->salXmlDocumentLen), data->salXmlDocument,
            sizeof(data->salXmlDocument));
    }

    if (status != OBS_STATUS_OK)
    {
        free(data);
        data = NULL;
        (void)(*(handler->complete_callback))(status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "generateRestoreXmlDocument failed !");
        return;
    }

    data->responsePropertiesCallback = handler->properties_callback;
    data->responseCompleteCallback = handler->complete_callback;
    data->callback_data = callback_data;
    data->salXmlDocumentBytesWritten = 0;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_post;
    params.key = object_info->key;
    params.subResource = "restore";
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.toObsCallback = &set_data_callback;
    params.toObsCallbackTotalSize = data->salXmlDocumentLen;
    params.properties_callback = &restoreObjectPropertiesCallback;
    params.complete_callback = &setCompleteCallback;
    params.callback_data = data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave restore_object successfully !");
}