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
#define OBS_MAX_STR_TMP_SIZE 65536

static obs_status complete_multi_part_upload_xml_callback(const char *elementPath, const char *data,
    int dataLen, void *callback_data)
{
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *)callback_data;

    int fit = 1;
    if (!data)
    {
        return OBS_STATUS_OK;
    }

    if (!strcmp(elementPath, "CompleteMultipartUploadResult/Location")) {
#ifdef WIN32
        int strTmpSourceLen = dataLen + 1;
		int ret = 0;
        if (strTmpSourceLen <= 0 || strTmpSourceLen > OBS_MAX_STR_TMP_SIZE) {
            COMMLOG(OBS_LOGERROR, "parameter of malloc is out of range in function: %s,line %d", __FUNCTION__, __LINE__);
            return OBS_STATUS_OutOfMemory;
        }
        char *strTmpSource = (char *)malloc(sizeof(char) * strTmpSourceLen);
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(strTmpSource, sizeof(char) * strTmpSourceLen, 0, strTmpSourceLen);
        if (ret = strncpy_s(strTmpSource, strTmpSourceLen, data, dataLen)) 
        {
            COMMLOG(OBS_LOGERROR, "in %s line %d strncpy_s error, code is %d.", __FUNCTION__, __LINE__, ret);
			CHECK_NULL_FREE(strTmpSource);
            return OBS_STATUS_InternalError;
        }
        char* strTmpOut = UTF8_To_String(strTmpSource);
        string_buffer_append(cmuData->location, strTmpOut, strlen(strTmpOut), fit);
        CHECK_NULL_FREE(strTmpSource);
        CHECK_NULL_FREE(strTmpOut);
#else
        string_buffer_append(cmuData->location, data, dataLen, fit);
#endif
    }
    else if (!strcmp(elementPath, "CompleteMultipartUploadResult/Bucket")) {
        string_buffer_append(cmuData->bucket, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "CompleteMultipartUploadResult/Key")) {
#ifdef WIN32
        int strTmpSourceLen = dataLen + 1;
		int ret = 0;
        if (strTmpSourceLen <= 0 || strTmpSourceLen > OBS_MAX_STR_TMP_SIZE) {
            COMMLOG(OBS_LOGERROR, "parameter of malloc is out of range in function: %s,line %d", __FUNCTION__, __LINE__);
            return OBS_STATUS_OutOfMemory;
        }
        char* strTmpSource = (char*)malloc(sizeof(char) * strTmpSourceLen);
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(strTmpSource, sizeof(char) * strTmpSourceLen, 0, strTmpSourceLen);
        if (ret = strncpy_s(strTmpSource, strTmpSourceLen, data, dataLen))
        {
            COMMLOG(OBS_LOGERROR, "in %s line %d strncpy_s error,code is %d.", __FUNCTION__, __LINE__, ret);
			CHECK_NULL_FREE(strTmpSource);
            return OBS_STATUS_InternalError;
        }
        char* strTmpOut = UTF8_To_String(strTmpSource);
        string_buffer_append(cmuData->key, strTmpOut, strlen(strTmpOut), fit);
        CHECK_NULL_FREE(strTmpSource);
        CHECK_NULL_FREE(strTmpOut);
#else
        string_buffer_append(cmuData->key, data, dataLen, fit);
#endif
    }
    else if (!strcmp(elementPath, "CompleteMultipartUploadResult/ETag")) {
        string_buffer_append(cmuData->etag, data, dataLen, fit);
    }

    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}

static void compose_complete_multi_part_upload_data(complete_multi_part_upload_data* cmuData,
    unsigned int part_number, obs_complete_upload_Info *complete_upload_Info,
    int buffer_len)
{
    unsigned int uiIdx = 0;
    (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "CompleteMultipartUpload",
        NULL, NOT_NEED_FORMALIZE, ADD_HEAD_ONLY, buffer_len);
    for (; uiIdx < part_number; uiIdx++)
    {
        if (NULL == complete_upload_Info[uiIdx].etag)
        {
            continue;
        }
        (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "Part",
            NULL, NOT_NEED_FORMALIZE, ADD_HEAD_ONLY, buffer_len);
        cmuData->docLen += snprintf_s(cmuData->doc + cmuData->docLen,
            256 * part_number - cmuData->docLen, _TRUNCATE,
            "<PartNumber>%u</PartNumber>", complete_upload_Info[uiIdx].part_number);
        (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "ETag", complete_upload_Info[uiIdx].etag,
            NEED_FORMALIZE, ADD_NAME_CONTENT, buffer_len);
        (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "Part",
            NULL, NOT_NEED_FORMALIZE, ADD_TAIL_ONLY, buffer_len);
    }
    (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "CompleteMultipartUpload",
        NULL, NOT_NEED_FORMALIZE, ADD_TAIL_ONLY, buffer_len);
    return;
}

static int complete_multi_part_upload_data_to_obs_callback(int buffer_size, char *buffer,
    void *callback_data)
{
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *)callback_data;

    if (!cmuData->docLen) {
        return 0;
    }
    int remaining = (cmuData->docLen - cmuData->docBytesWritten);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy) {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(cmuData->doc[cmuData->docBytesWritten]), toCopy);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "complete_multi_part_upload_data_to_obs_callback: memcpy_s failed!\n");
        return 0;
    }

    cmuData->docBytesWritten += toCopy;
    return toCopy;
}

static obs_status complete_multi_part_upload_data_from_obs_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *)callback_data;
    if (cmuData->server_callback)
    {
        obs_sever_callback_data server_callback_data;
        int server_callback_buf_size = buffer_size + 1;
        if (server_callback_buf_size <= 0 || server_callback_buf_size > OBS_MAX_STR_TMP_SIZE) {
            COMMLOG(OBS_LOGERROR, "parameter of malloc is out of range in function: %s,line %d", __FUNCTION__, __LINE__);
            return OBS_STATUS_OutOfMemory;
        }
        char * server_callback_buf = (char *)malloc(sizeof(char) * server_callback_buf_size);
        if(server_callback_buf == NULL){
            COMMLOG(OBS_LOGERROR, "malloc failed in function: %s,line %d", __FUNCTION__, __LINE__);
            return OBS_STATUS_OutOfMemory;
        }
        errno_t ret = memset_s(server_callback_buf, server_callback_buf_size, 0, server_callback_buf_size);
        if(ret != EOK){
            if(ret == ERANGE){
                COMMLOG(OBS_LOGWARN, "memset_s failed in function: %s, return_value: ERANGE", __FUNCTION__);
            }else if(ret == EINVAL){
                COMMLOG(OBS_LOGWARN, "memset_s failed in function: %s, return_value: EINVAL", __FUNCTION__);
            }else if(ret == ERANGE_AND_RESET){
                COMMLOG(OBS_LOGWARN, "memset_s failed in function: %s, return_value: ERANGE_AND_RESET", __FUNCTION__);
            }else {
                COMMLOG(OBS_LOGWARN, "memset_s failed in function: %s, return_value: %d", __FUNCTION__, ret);
            }
            CHECK_NULL_FREE(server_callback_buf);
            return OBS_STATUS_InternalError;
        }
		ret = memcpy_s(server_callback_buf, server_callback_buf_size, buffer, buffer_size);
		if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memcpy_s), __FUNCTION__, __LINE__, ret)) {
			CHECK_NULL_FREE(server_callback_buf);
			return OBS_STATUS_Security_Function_Failed;
		}
        server_callback_data.buffer = server_callback_buf;
        server_callback_data.buffer_len = buffer_size;
        cmuData->callback_data = (void *)(&server_callback_data);
        (*(cmuData->responsePropertiesCallback))
            (NULL, cmuData->callback_data);
        free(server_callback_buf);
        return OBS_STATUS_OK;
    }
    return simplexml_add(&(cmuData->simpleXml), buffer, buffer_size);
}

static void complete_multi_part_upload_complete_callback(obs_status requestStatus,
    const obs_error_details *s3ErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *)callback_data;
    (*(cmuData->complete_multipart_upload_callback))
        (cmuData->location, cmuData->bucket, cmuData->key, cmuData->etag, cmuData->callback_data);
    (*(cmuData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, cmuData->callback_data);
    simplexml_deinitialize(&(cmuData->simpleXml));
    if (NULL != cmuData->doc)
    {
        free(cmuData->doc);
        cmuData->doc = NULL;
    }
    free(cmuData);
    cmuData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}

static obs_status complete_multi_part_upload_properties_callback
(const obs_response_properties *responseProperties, void *callback_data)
{
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *)callback_data;

    if (cmuData->responsePropertiesCallback)
    {
        return (*(cmuData->responsePropertiesCallback))
            (responseProperties, cmuData->callback_data);
    }

    return OBS_STATUS_OK;
}

void complete_multi_part_upload(const obs_options *options, char *key, const char *upload_id, unsigned int part_number,
    obs_complete_upload_Info *complete_upload_Info, obs_put_properties *put_properties,
    obs_complete_multi_part_upload_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    COMMLOG(OBS_LOGINFO, "Enter complete_multi_part_upload successfully !");
    if (!options->bucket_options.bucket_name || part_number > 10000)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL or part_number is:%u.", part_number);
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    int amp = 0;
    if (upload_id) {
        safe_append_with_interface_log("uploadId", upload_id,
            strlen(upload_id), handler->response_handler.complete_callback);
    }
    else
    {
        COMMLOG(OBS_LOGERROR, "upload_id is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }
    complete_multi_part_upload_data* cmuData =
        (complete_multi_part_upload_data *)malloc(sizeof(complete_multi_part_upload_data));
    if (NULL == cmuData) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc cmuData failed !");
        return;
    }
    memset_s(cmuData, sizeof(complete_multi_part_upload_data), 0, sizeof(complete_multi_part_upload_data));
    cmuData->doc = (char*)malloc(ONE_PART_REQUEST_XML_LEN * part_number);
    if (NULL == cmuData->doc) {
        COMMLOG(OBS_LOGERROR, "Malloc cmuData->doc failed !");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        free(cmuData);
        cmuData = NULL;
        return;
    }

    memset_s(cmuData->doc, ONE_PART_REQUEST_XML_LEN * part_number, 0, ONE_PART_REQUEST_XML_LEN * part_number);

    simplexml_initialize(&(cmuData->simpleXml), &complete_multi_part_upload_xml_callback, cmuData);
    cmuData->responsePropertiesCallback = handler->response_handler.properties_callback;
    cmuData->responseCompleteCallback = handler->response_handler.complete_callback;
    cmuData->complete_multipart_upload_callback = handler->complete_multipart_upload_callback;
    cmuData->callback_data = callback_data;
    cmuData->server_callback = false;

    cmuData->docLen = 0;
    cmuData->docBytesWritten = 0;
    compose_complete_multi_part_upload_data(cmuData, part_number, complete_upload_Info,
        256 * part_number);
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
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.put_properties = put_properties;
    params.properties_callback = &complete_multi_part_upload_properties_callback;
    params.complete_callback = &complete_multi_part_upload_complete_callback;
    params.toObsCallback = &complete_multi_part_upload_data_to_obs_callback;
    params.toObsCallbackTotalSize = cmuData->docLen;
    params.fromObsCallback = &complete_multi_part_upload_data_from_obs_callback;
    params.callback_data = cmuData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave complete_multi_part_upload successfully !");
}
