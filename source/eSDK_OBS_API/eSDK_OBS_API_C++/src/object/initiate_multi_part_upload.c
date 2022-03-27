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

static obs_status initiate_multi_part_upload_xml_callback(const char *elementPath,
    const char *data, int dataLen,
    void *callback_data)
{
    initiate_multi_part_upload_data *imuData = (initiate_multi_part_upload_data *)callback_data;

    int fit = 1;
    if (data && !strcmp(elementPath, "InitiateMultipartUploadResult/UploadId")) {
        string_buffer_append(imuData->uploadID, data, dataLen, fit);
    }
    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}

static obs_status initiate_multi_part_upload_properties_callback
(const obs_response_properties *responseProperties, void *callback_data)
{
    initiate_multi_part_upload_data *imuData = (initiate_multi_part_upload_data *)callback_data;
    if (imuData->responsePropertiesCallback)
    {
        return (*(imuData->responsePropertiesCallback))
            (responseProperties, imuData->callback_data);
    }

    return OBS_STATUS_OK;
}


static obs_status initiate_multi_part_upload_data_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    initiate_multi_part_upload_data *imuData = (initiate_multi_part_upload_data *)callback_data;

    return simplexml_add(&(imuData->simpleXml), buffer, buffer_size);
}


static void initiate_multi_part_upload_complete_callback(obs_status requestStatus,
    const obs_error_details *s3ErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    initiate_multi_part_upload_data *imuData = (initiate_multi_part_upload_data *)callback_data;

    int ret = snprintf_s(imuData->upload_id_return, imuData->upload_id_return_size,
        imuData->upload_id_return_size - 1, "%s",
        imuData->uploadID);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);

    (void)(*(imuData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, imuData->callback_data);

    simplexml_deinitialize(&(imuData->simpleXml));

    free(imuData);
    imuData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}


void initiate_multi_part_upload(const obs_options *options, char *key, int upload_id_return_size,
    char *upload_id_return, obs_put_properties *put_properties,
    server_side_encryption_params *encryption_params,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter initiate_multi_part_upload successfully !");
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    initiate_multi_part_upload_data *imuData =
        (initiate_multi_part_upload_data *)malloc(sizeof(initiate_multi_part_upload_data));

    if (!imuData)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc InitiateMultipartUploadData failed !");
        return;
    }
    if (upload_id_return_size < 0)
    {
        COMMLOG(OBS_LOGERROR, "upload_id_return_size is invalid!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        free(imuData);
        imuData = NULL;
        return;
    }
    memset_s(imuData, sizeof(initiate_multi_part_upload_data), 0, sizeof(initiate_multi_part_upload_data));
    simplexml_initialize(&(imuData->simpleXml), &initiate_multi_part_upload_xml_callback, imuData);
    imuData->responsePropertiesCallback = handler->properties_callback;
    imuData->responseCompleteCallback = handler->complete_callback;
    imuData->callback_data = callback_data;

    imuData->upload_id_return_size = upload_id_return_size;
    imuData->upload_id_return = upload_id_return;
    string_buffer_initialize(imuData->uploadID);

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
    params.key = key;
    params.subResource = "uploads";
    params.put_properties = put_properties;
    params.encryption_params = encryption_params;
    params.properties_callback = &initiate_multi_part_upload_properties_callback;
    params.complete_callback = &initiate_multi_part_upload_complete_callback;
    params.fromObsCallback = &initiate_multi_part_upload_data_callback;
    params.callback_data = imuData;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = storage_class;
    params.use_api = use_api;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave initiate_multi_part_upload successfully !");
}
