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

void set_obs_put_properties(obs_put_properties *put_properties, unsigned int is_copy)
{
    if (NULL == put_properties)
    {
        if (0 == is_copy)
        {
            COMMLOG(OBS_LOGWARN, "set_obs_put_properties: put_properties is NULL!");
        }
    }
    else
    {
        if (0 < is_copy)
        {
            put_properties->meta_data_count = 0;
        }
        else
        {
            if (0 == put_properties->meta_data_count)
            {
                put_properties->meta_data_count = -1;
            }
        }
    }
}

static obs_status copyObjectXmlCallback(const char *elementPath,
    const char *data, int dataLen,
    void *callback_data)
{
    copy_object_data *coData = (copy_object_data *)callback_data;
    int fit = 1;
    if (!data)
    {
        return OBS_STATUS_OK;
    }

    if (!strcmp(elementPath, "CopyObjectResult/LastModified")) {
        string_buffer_append(coData->last_modified, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "CopyObjectResult/ETag")) {
        if (coData->etag_return_size && coData->etag_return) {
            coData->eTagReturnLen +=
                snprintf_s(&(coData->etag_return[coData->eTagReturnLen]),
                    coData->etag_return_size - coData->eTagReturnLen,
                    coData->etag_return_size -
                    coData->eTagReturnLen - 1,
                    "%.*s", dataLen, data);
            if (coData->eTagReturnLen >= coData->etag_return_size) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
    }
    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}

static obs_status copyObjectPropertiesCallback(
    const obs_response_properties *responseProperties, void *callback_data)
{
    copy_object_data *coData = (copy_object_data *)callback_data;
    if (coData->responsePropertiesCallback)
    {
        return (*(coData->responsePropertiesCallback))(responseProperties,
            coData->callback_data);
    }

    return OBS_STATUS_OK;
}

static void copyObjectCompleteCallback(obs_status requestStatus,
    const obs_error_details *s3ErrorDetails, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    copy_object_data *coData = (copy_object_data *)callback_data;

    if (coData->last_modified_return) {
        time_t last_modified = -1;
        if (coData->last_modifiedLen) {
            last_modified = parseIso8601Time(coData->last_modified);
            int nTimeZone = getTimeZone();
            last_modified += nTimeZone * SECONDS_TO_AN_HOUR;
        }

        *(coData->last_modified_return) = last_modified;
    }

    (void)(*(coData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, coData->callback_data);

    simplexml_deinitialize(&(coData->simpleXml));

    free(coData);
    coData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}


void copy_object(const obs_options *options, char *key, const char *version_id,
    obs_copy_destination_object_info *object_info,
    unsigned int is_copy, obs_put_properties *put_properties,
    server_side_encryption_params *encryption_params,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter copy_object successfully !");
    set_obs_put_properties(put_properties, is_copy);
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if (object_info->etag_return_size < 0
        || NULL == object_info->destination_bucket
        || NULL == object_info->destination_key) {
        COMMLOG(OBS_LOGERROR, "etag_return_size < 0 or destination_bucket or destination_key is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }
    copy_object_data *data =
        (copy_object_data *)malloc(sizeof(copy_object_data));
    if (!data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc CopyObjectData failed !");
        return;
    }
    memset_s(data, sizeof(copy_object_data), 0, sizeof(copy_object_data));

    simplexml_initialize(&(data->simpleXml), &copyObjectXmlCallback, data);
    data->responsePropertiesCallback = handler->properties_callback;
    data->responseCompleteCallback = handler->complete_callback;
    data->callback_data = callback_data;
    data->last_modified_return = object_info->last_modified_return;
    data->etag_return_size = object_info->etag_return_size;
    data->etag_return = object_info->etag_return;
    if (data->etag_return_size && data->etag_return) {
        data->etag_return[0] = 0;
    }
    data->eTagReturnLen = 0;
    string_buffer_initialize(data->last_modified);

    char versionkey[1024] = { 0 };
    if (object_info->version_id)
    {
        int ret = snprintf_s(versionkey, sizeof(versionkey), _TRUNCATE,
            "%s?version_id=%s", key, version_id);
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_copy;
    params.key = object_info->destination_key;
    params.copySourceKey = versionkey[0] ? versionkey : key;
    params.copySourceBucketName = options->bucket_options.bucket_name;
    params.bucketContext.bucket_name = object_info->destination_bucket;
    params.put_properties = put_properties;
    params.encryption_params = encryption_params;
    params.properties_callback = &copyObjectPropertiesCallback;
    params.complete_callback = &copyObjectCompleteCallback;
    params.fromObsCallback = &copyObjectDataCallback;
    params.callback_data = data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave copy_object successfully !");

}