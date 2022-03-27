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

static obs_status CopyPartXmlCallback(const char *elementPath,
    const char *data, int dataLen,
    void *callback_data)
{
    copy_object_data *cpData = (copy_object_data *)callback_data;

    int fit = 1;
    if (!data)
    {
        return OBS_STATUS_OK;
    }

    if (!strcmp(elementPath, "CopyPartResult/LastModified")) {
        string_buffer_append(cpData->last_modified, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "CopyPartResult/ETag")) {
        if (cpData->etag_return_size && cpData->etag_return) {
            cpData->eTagReturnLen +=
                snprintf_s(&(cpData->etag_return[cpData->eTagReturnLen]),
                    cpData->etag_return_size - cpData->eTagReturnLen,
                    cpData->etag_return_size - cpData->eTagReturnLen - 1,
                    "%.*s", dataLen, data);
            if (cpData->eTagReturnLen >= cpData->etag_return_size) {
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


int copy_part_optionsSet(const obs_options *options,
    obs_copy_destination_object_info *object_info,
    obs_response_handler *handler, void *callback_data)
{
    int ret = 0;
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        ret = 1;
        return ret;
    }
    if (object_info->etag_return_size < 0 || NULL == object_info->destination_bucket || NULL == object_info->destination_key) {
        COMMLOG(OBS_LOGERROR, "etag_return_size < 0 or destination_bucket or destination_key is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        ret = 1;
        return ret;
    }
    return ret;
}

static obs_status CopyPartPropertiesCallback(
    const obs_response_properties *responseProperties, void *callback_data)
{
    copy_object_data *cpData = (copy_object_data *)callback_data;
    if (cpData->responsePropertiesCallback)
    {
        return (*(cpData->responsePropertiesCallback))(responseProperties,
            cpData->callback_data);
    }

    return OBS_STATUS_OK;
}

static void CopyPartCompleteCallback(obs_status requestStatus,
    const obs_error_details *s3ErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    copy_object_data *cpData = (copy_object_data *)callback_data;

    if (cpData->last_modified_return) {
        time_t last_modified = -1;
        if (cpData->last_modifiedLen) {
            last_modified = parseIso8601Time(cpData->last_modified);
            int nTimeZone = getTimeZone();
            last_modified += nTimeZone * SECONDS_TO_AN_HOUR;
        }

        *(cpData->last_modified_return) = last_modified;
    }

    (void)(*(cpData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, cpData->callback_data);

    simplexml_deinitialize(&(cpData->simpleXml));

    free(cpData);
    cpData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}

void copy_part(const obs_options *options, char *key, obs_copy_destination_object_info *object_info,
    obs_upload_part_info *copypart, obs_put_properties *put_properties,
    server_side_encryption_params *encryption_params, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter copy_part successfully !");
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);

    int amp = 0;
    if (copypart->part_number > 0) {
        char part_number_string[64] = { 0 };
        int ret = snprintf_s(part_number_string, sizeof(part_number_string), _TRUNCATE, "%u", copypart->part_number);
        CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        safe_append_with_interface_log("partNumber",
            part_number_string, handler->complete_callback);
    }
    if (copypart->upload_id) {
        safe_append_with_interface_log("uploadId",
            copypart->upload_id, handler->complete_callback);
    }
    if (copy_part_optionsSet(options, object_info, handler, callback_data))
    {
        return;
    }
    copy_object_data *data = (copy_object_data *)malloc(sizeof(copy_object_data));
    if (!data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc CopyObjectData failed !");
        return;
    }
    memset_s(data, sizeof(copy_object_data), 0, sizeof(copy_object_data));

    simplexml_initialize(&(data->simpleXml), &CopyPartXmlCallback, data);
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


    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_copy;
    params.key = object_info->destination_key ? object_info->destination_key : key;
    params.copySourceKey = key;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.copySourceBucketName = options->bucket_options.bucket_name;
    params.bucketContext.bucket_name = object_info->destination_bucket;
    params.put_properties = put_properties;
    params.encryption_params = encryption_params;
    params.properties_callback = &CopyPartPropertiesCallback;
    params.complete_callback = &CopyPartCompleteCallback;
    params.fromObsCallback = &copyObjectDataCallback;
    params.callback_data = data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave copy_part successfully !");
}