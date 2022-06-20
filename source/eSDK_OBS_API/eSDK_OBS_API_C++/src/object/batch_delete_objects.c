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

static void initialize_del_Object_contents(delete_object_contents *contents)
{
    string_buffer_initialize(contents->key);
    string_buffer_initialize(contents->code);
    string_buffer_initialize(contents->message);
    string_buffer_initialize(contents->delete_marker);
    string_buffer_initialize(contents->delete_marker_version_id);
}

static void initialize_del_Object_data(delete_object_data *doData)
{
    doData->contents_count = 0;
    initialize_del_Object_contents(doData->contents);
}

static obs_status make_del_Object_callback(delete_object_data *doData)
{
    int i;
    obs_status iRet = OBS_STATUS_OK;
    if (doData->contents_count < 1)
    {
        COMMLOG(OBS_LOGERROR, "Invalid Malloc Parameter!");
        return OBS_STATUS_InternalError;
    }
    obs_delete_objects *contents = (obs_delete_objects*)malloc(sizeof(obs_delete_objects) * doData->contents_count);
    if (NULL == contents)
    {
        COMMLOG(OBS_LOGERROR, "Malloc obs_delete_objects failed!");
        return OBS_STATUS_InternalError;
    }
    memset_s(contents, sizeof(obs_delete_objects) * doData->contents_count, 0, sizeof(obs_delete_objects) * doData->contents_count);

    int contents_count = doData->contents_count;
    for (i = 0; i < contents_count; i++) {
        obs_delete_objects *contentDest = &(contents[i]);
        delete_object_contents *contentSrc = &(doData->contents[i]);
        contentDest->key = contentSrc->key;
        contentDest->code = contentSrc->code;
        contentDest->message = contentSrc->message;
        contentDest->delete_marker = contentSrc->delete_marker;
        contentDest->delete_marker_version_id = contentSrc->delete_marker_version_id;
    }
    iRet = (*(doData->delete_object_data_callback))
        (contents_count, contents, doData->callback_data);
    CHECK_NULL_FREE(contents);
    return iRet;
}


int  dataExistDeleteObjectXmlCallback(const char *elementPath, delete_object_data *doData,
    const char *data, int dataLen, int fit)
{
    if (!strcmp(elementPath, "DeleteResult/Deleted/Key")) {
        delete_object_contents *contests = &(doData->contents[doData->contents_count]);
        string_buffer_append(contests->key, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "DeleteResult/Deleted/DeleteMarker")) {
        delete_object_contents *contents =
            &(doData->contents[doData->contents_count]);
        string_buffer_append(contents->delete_marker, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "DeleteResult/Deleted/DeleteMarkerVersionId")) {
        delete_object_contents *contents =
            &(doData->contents[doData->contents_count]);
        string_buffer_append(contents->delete_marker_version_id, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "DeleteResult/Error/Key")) {
        delete_object_contents *contents =
            &(doData->contents[doData->contents_count]);
        string_buffer_append(contents->key, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "DeleteResult/Error/Code")) {
        delete_object_contents *contents =
            &(doData->contents[doData->contents_count]);
        string_buffer_append(contents->code, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "DeleteResult/Error/Message")) {
        delete_object_contents *contents =
            &(doData->contents[doData->contents_count]);
        string_buffer_append(contents->message, data, dataLen, fit);
    }
    return fit;
}

obs_status dataNotExistDeleteObjectXmlCallback(delete_object_data *doData)
{
    doData->contents_count++;
    if (doData->contents_count == OBS_MAX_DELETE_OBJECT_NUMBER)
    {
        obs_status status = make_del_Object_callback(doData);
        if (status != OBS_STATUS_OK) {
            return status;
        }
        initialize_del_Object_data(doData);
    }
    else {
        initialize_del_Object_contents
        (&(doData->contents[doData->contents_count]));
    }
    return OBS_STATUS_OK;
}

static obs_status deleteObjectXmlCallback(const char *elementPath, const char *data,
    int dataLen, void *callback_data)
{
    delete_object_data *doData = (delete_object_data *)callback_data;

    int fit = 1;
    if (data) {
        fit = dataExistDeleteObjectXmlCallback(elementPath, doData, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "DeleteResult/Deleted") || !strcmp(elementPath, "DeleteResult/Error"))
    {
        return dataNotExistDeleteObjectXmlCallback(doData);
    }
    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}

static int deleteObjectDataToObsCallback(int buffer_size, char *buffer,
    void *callback_data)
{
    delete_object_data *doData = (delete_object_data *)callback_data;
    if (!doData->docLen) {
        return 0;
    }
    int remaining = (doData->docLen - doData->docBytesWritten);
    int toCopy = buffer_size > remaining ? remaining : buffer_size;
    if (!toCopy) {
        return 0;
    }


    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(doData->doc[doData->docBytesWritten]), toCopy);

    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "deleteObjectDataToObsCallback: memcpy_s failed!\n");
        return 0;
    }

    doData->docBytesWritten += toCopy;
    return toCopy;
}

static obs_status deleteObjectDataFromObsCallback(int buffer_size, const char *buffer,
    void *callback_data)
{
    delete_object_data *doData = (delete_object_data *)callback_data;
    return simplexml_add(&(doData->simpleXml), buffer, buffer_size);
}

static obs_status deleteObjectPropertiesCallback(
    const obs_response_properties *responseProperties, void *callback_data)
{
    delete_object_data *doData = (delete_object_data *)callback_data;
    if (doData->responsePropertiesCallback)
    {
        return (*(doData->responsePropertiesCallback))(responseProperties,
            doData->callback_data);
    }

    return OBS_STATUS_OK;
}


static void deleteObjectCompleteCallback(obs_status requestStatus,
    const obs_error_details *s3ErrorDetails,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    delete_object_data *doData = (delete_object_data *)callback_data;
    if (doData->contents_count) {
        obs_status ret = make_del_Object_callback(doData);
        if (ret != OBS_STATUS_OK) {
            COMMLOG(OBS_LOGDEBUG, "Failed to call make_del_Object_callback, status: %d.", ret);
        }
    }
    (*(doData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, doData->callback_data);
    simplexml_deinitialize(&(doData->simpleXml));
    free(doData);
    doData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}


obs_status compose_del_xml(obs_object_info *object_info, obs_delete_object_info *delobj,
    obs_delete_object_handler *handler, delete_object_data* doData, void *callback_data)
{
    unsigned int uiIdx = 0;
    (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Delete", NULL,
        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY, OBS_MAX_DELETE_OBJECT_DOC);
    if (delobj->quiet)
    {
        (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Quiet", "true",
            NOT_NEED_FORMALIZE, ADD_NAME_CONTENT, OBS_MAX_DELETE_OBJECT_DOC);
    }

    for (; uiIdx < delobj->keys_number; uiIdx++)
    {
        (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Object", NULL,
            NOT_NEED_FORMALIZE, ADD_HEAD_ONLY, OBS_MAX_DELETE_OBJECT_DOC);
        (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Key", object_info[uiIdx].key,
            NEED_FORMALIZE, ADD_NAME_CONTENT, OBS_MAX_DELETE_OBJECT_DOC);
        if (NULL != object_info[uiIdx].version_id)
        {
            (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "VersionId",
                object_info[uiIdx].version_id, NOT_NEED_FORMALIZE, ADD_NAME_CONTENT, OBS_MAX_DELETE_OBJECT_DOC);
        }

        (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Object", NULL,
            NOT_NEED_FORMALIZE, ADD_TAIL_ONLY, OBS_MAX_DELETE_OBJECT_DOC);

        if ((doData->docLen >= OBS_MAX_DELETE_OBJECT_DOC) && (uiIdx != (delobj->keys_number - 1)))
        {
            (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
            return OBS_STATUS_OutOfMemory;
        }
    }
    (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Delete", NULL,
        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY, OBS_MAX_DELETE_OBJECT_DOC);
    return OBS_STATUS_OK;
}


void batch_delete_objects(const obs_options *options, obs_object_info *object_info, obs_delete_object_info *delobj,
    obs_put_properties *put_properties, obs_delete_object_handler *handler, void *callback_data)
{

    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    obs_put_properties  properties;
    unsigned char doc_md5[16] = { 0 };
    char base64_md5[64] = { 0 };

    COMMLOG(OBS_LOGINFO, "Enter batch_delete_objects successfully !");
    if (put_properties == NULL)
    {
        memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    }
    else
    {
        memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
        properties = *put_properties;
    }
    if (delobj->keys_number > OBS_MAX_DELETE_OBJECT_NUMBER || NULL == object_info)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Input param keys_number is greater than 100  or obs_object_info is NULL!");
        return;
    }
    if (!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    delete_object_data* doData = (delete_object_data *)malloc(sizeof(delete_object_data));
    if (NULL == doData) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc DeleteObjectData failed!");
        return;
    }
    memset_s(doData, sizeof(delete_object_data), 0, sizeof(delete_object_data));
    simplexml_initialize(&(doData->simpleXml), &deleteObjectXmlCallback, doData);
    doData->responsePropertiesCallback = handler->response_handler.properties_callback;
    doData->responseCompleteCallback = handler->response_handler.complete_callback;
    doData->delete_object_data_callback = handler->delete_object_data_callback;
    doData->callback_data = callback_data;
    doData->docLen = 0;
    doData->docBytesWritten = 0;
    if (OBS_STATUS_OK != compose_del_xml(object_info, delobj, handler, doData, callback_data))
    {
        free(doData);
        doData = NULL;
        return;
    }

    COMMLOG(OBS_LOGDEBUG, "batch_delete_objects doc = %s!", doData->doc);

    MD5((unsigned char*)doData->doc, (size_t)doData->docLen, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), base64_md5);
    properties.md5 = base64_md5;

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
    params.subResource = "delete";
    params.put_properties = &properties;
    params.properties_callback = &deleteObjectPropertiesCallback;
    params.complete_callback = &deleteObjectCompleteCallback;
    params.toObsCallback = &deleteObjectDataToObsCallback;
    params.toObsCallbackTotalSize = doData->docLen;
    params.fromObsCallback = &deleteObjectDataFromObsCallback;
    params.callback_data = doData;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave batch_delete_objects successfully !");
}
