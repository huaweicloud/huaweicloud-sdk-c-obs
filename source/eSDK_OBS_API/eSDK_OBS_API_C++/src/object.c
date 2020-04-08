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
#include <stdlib.h>
#include <string.h>
#include "eSDKOBS.h"
#include "request.h"
#include "securec.h"
#include "object.h"
#include <openssl/md5.h> 

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined __GNUC__ || defined LINUX
#include <unistd.h>
#include <pthread.h>
pthread_mutex_t g_mutexThreadCheckpoint;
pthread_mutex_t g_mutexThreadCheckpoint_download;
#endif

#if defined WIN32
#include <io.h>
#include <share.h>
#define read _read
#define close _close
#define write _write
#include <process.h>
CRITICAL_SECTION  g_csThreadCheckpoint; 
CRITICAL_SECTION  g_csThreadCheckpoint_download;
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>

#define D_CMU_DATA_DEFAULT_LEN 2048

void initialize_break_point_lock()
{
#if defined __GNUC__ || defined LINUX
	pthread_mutex_init(&g_mutexThreadCheckpoint_download,NULL); 
	pthread_mutex_init(&g_mutexThreadCheckpoint,NULL);
#else
	InitializeCriticalSection(&g_csThreadCheckpoint_download);  
	InitializeCriticalSection(&g_csThreadCheckpoint); 
#endif
}

void deinitialize_break_point_lock()
{
#if defined __GNUC__ || defined LINUX
        pthread_mutex_destroy(&g_mutexThreadCheckpoint_download); 
        pthread_mutex_destroy(&g_mutexThreadCheckpoint); 
#else
        DeleteCriticalSection(&g_csThreadCheckpoint_download); 
        DeleteCriticalSection(&g_csThreadCheckpoint); 
#endif

}

void obs_head_object(const obs_options *options, char *key, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter obs_head_object Successfully!");
    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_head;
    params.key = key;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data     = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.callback_data = callback_data;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave obs_head_object Successfully!");
}

void get_object_metadata(const obs_options *options, obs_object_info *object_info, 
                         server_side_encryption_params *encryption_params,
                         obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter get_object_metadata successfully !");
    if(NULL == object_info->key || !strlen(object_info->key)){
        COMMLOG(OBS_LOGERROR, "key is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidKey, 0, 0);
        return;
    }
    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);

    int amp = 0;
    if (object_info->version_id) {
        safe_append_with_interface_log("versionId", object_info->version_id,
            handler->complete_callback);
    }
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_head;
    params.key = object_info->key;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.encryption_params = encryption_params;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api = use_api;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave get_object_metadata successfully !");
}

void set_object_metadata(const obs_options *options, obs_object_info *object_info, 
						 obs_put_properties *put_properties,
						 server_side_encryption_params *encryption_params,
						 obs_response_handler *handler, void *callback_data)
{
	request_params params;
	obs_use_api use_api = OBS_USE_API_S3;
	set_use_api_switch(options, &use_api);
	COMMLOG(OBS_LOGINFO, "Enter set_object_metadata successfully !");

	if(NULL == object_info->key || !strlen(object_info->key)){
		COMMLOG(OBS_LOGERROR, "key is NULL!");
		(void)(*(handler->complete_callback))(OBS_STATUS_InvalidKey, 0, 0);
		return;
	}
	if(!options->bucket_options.bucket_name){
		COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
		(void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
		return;
	}
	if(put_properties->metadata_action == OBS_NO_METADATA_ACTION){
		COMMLOG(OBS_LOGERROR, "put_properties.metadata_action is OBS_NO_METADATA_ACTION!");
		(void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, 0);
		return;
	}


	string_buffer(queryParams, QUERY_STRING_LEN);
	string_buffer_initialize(queryParams);

	int amp = 0;
	if (object_info->version_id) {
		safe_append_with_interface_log("versionId", object_info->version_id,
			handler->complete_callback);
	}

	memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
	memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
		sizeof(obs_bucket_context));
	memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
		sizeof(obs_http_request_option));

	params.temp_auth = options->temp_auth;
	params.httpRequestType = http_request_type_put;
	params.key = object_info->key;
	params.put_properties = put_properties;
	params.queryParams = queryParams[0] ? queryParams : 0;
	params.encryption_params = encryption_params;
	params.properties_callback = handler->properties_callback;
	params.complete_callback = handler->complete_callback;
	params.callback_data = callback_data;
	params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
	params.storageClassFormat   = no_need_storage_class;
	params.subResource = "metadata";
	params.use_api = use_api;

	request_perform(&params);
	COMMLOG(OBS_LOGINFO, "Leave set_object_metadata successfully !");
}

void put_object(const obs_options *options, char *key, uint64_t content_length,
                obs_put_properties *put_properties,
                server_side_encryption_params *encryption_params,
                obs_put_object_handler *handler, void *callback_data)
{   
    
    request_params params;
    COMMLOG(OBS_LOGINFO, "Enter put_object successfully !");
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_put;
    params.key = key;
    params.put_properties = put_properties;
    params.encryption_params = encryption_params;
    params.toObsCallback = handler->put_object_data_callback;
    params.toObsCallbackTotalSize = content_length;
    params.properties_callback = handler->response_handler.properties_callback;
    params.complete_callback = handler->response_handler.complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = storage_class;
    params.use_api = use_api;
        
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave put_object successfully !");
}

void append_object(const obs_options *options, char *key, uint64_t content_length, const char *position,
                   obs_put_properties *put_properties,server_side_encryption_params *encryption_params,
                   obs_append_object_handler *handler, void *callback_data)
{   
    
    request_params params;

    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;
    COMMLOG(OBS_LOGINFO, "Enter append_object successfully !");
    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if (position) {
        safe_append("position", position, handler->response_handler.complete_callback);
    }
    else {
        safe_append("position", "0", handler->response_handler.complete_callback);
    }
        
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_post;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.subResource = "append";
    params.key = key;
    params.put_properties = put_properties;
    params.encryption_params = encryption_params;
    params.toObsCallback = handler->append_object_data_callback;
    params.toObsCallbackTotalSize = content_length;
    params.properties_callback = handler->response_handler.properties_callback;
    params.complete_callback = handler->response_handler.complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave append_object successfully !");
}

// only posix bucke use
void modify_object(const obs_options *options, char *key, uint64_t content_length, uint64_t position,
                   obs_put_properties *put_properties,server_side_encryption_params *encryption_params,
                   obs_modify_object_handler *handler, void *callback_data)
{   
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;
    char strToAppend[128] = {0};
    COMMLOG(OBS_LOGINFO, "Enter modify_object successfully !");
    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    snprintf_sec(strToAppend, sizeof(strToAppend),_TRUNCATE, "%lu", position);   
    safe_append("position", strToAppend, handler->response_handler.complete_callback);
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_put;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.subResource = "modify";
    params.key = key;
    params.put_properties = put_properties;
    params.encryption_params = encryption_params;
    params.toObsCallback = handler->modify_object_data_callback;
    params.toObsCallbackTotalSize = content_length;
    params.properties_callback = handler->response_handler.properties_callback;
    params.complete_callback = handler->response_handler.complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave modify_object successfully !");
}

// only posix bucke use
void truncate_object(const obs_options *options, char *key, uint64_t object_length,
                   obs_response_handler *handler, void *callback_data)
{   
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;
    char strToAppend[128] = {0};

    COMMLOG(OBS_LOGINFO, "Enter truncate_object successfully !");

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }

    snprintf_sec(strToAppend,sizeof(strToAppend),_TRUNCATE,"%lu",object_length);   
    safe_append("length", strToAppend, handler->complete_callback);
        
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_put;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.subResource = "truncate";
    params.key = key;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api = use_api;
    
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave truncate_object successfully !");
}

// only posix bucke use
void rename_object(const obs_options *options, char *key, char *new_object_name,
                   obs_response_handler *handler, void *callback_data)
{   
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;
    
    COMMLOG(OBS_LOGINFO, "Enter truncate_object successfully !");

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
  
    safe_append("name", new_object_name, handler->complete_callback);
        
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_post;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.subResource = "rename";
    params.key = key;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave truncate_object successfully !");
}


void get_object(const obs_options *options, obs_object_info *object_info,
                obs_get_conditions *get_conditions, 
                server_side_encryption_params *encryption_params,
                obs_get_object_handler *handler, void *callback_data)
{

    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    
    COMMLOG(OBS_LOGINFO, "Enter get_object successfully!");

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);

    int amp = 0;
    if (object_info->version_id) {
        safe_append("versionId",object_info->version_id, handler->response_handler.complete_callback);
    }

    if(get_conditions && get_conditions->image_process_config)
    {
         char strToAppend[1024] = {0};
         int isImageProcModeValid = 0;
                    
            if(get_conditions->image_process_config->image_process_mode == obs_image_process_cmd)
            {
                snprintf_sec(strToAppend,sizeof(strToAppend),_TRUNCATE,"image%s","/");      
                isImageProcModeValid = 1;
            }
            else if(get_conditions->image_process_config->image_process_mode == obs_image_process_style)
            {
                snprintf_sec(strToAppend,sizeof(strToAppend),_TRUNCATE,"style%s","/");
                isImageProcModeValid = 1;
            }
            else
            {
                COMMLOG(OBS_LOGWARN, "Image Process Mode is Not valid !");
                 isImageProcModeValid = 0;
            }

            if(isImageProcModeValid == 1)
            {
                snprintf_sec(strToAppend,sizeof(strToAppend),_TRUNCATE,
                    "%s%s",strToAppend,get_conditions->image_process_config->cmds_stylename);
                
                safe_append("x-image-process",strToAppend,handler->response_handler.complete_callback);
            }
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_get;
    params.key = object_info->key;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.get_conditions = get_conditions;
    params.encryption_params = encryption_params;
    params.fromObsCallback = handler->get_object_data_callback;
    params.properties_callback = handler->response_handler.properties_callback;
    params.complete_callback = handler->response_handler.complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave get_object successfully!");
}

void delete_object(const obs_options *options, obs_object_info *object_info,
                    obs_response_handler *handler, void *callback_data)
{

    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter delete_object successfully !");
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;
    if (object_info->version_id)
    {
        safe_append("versionId", object_info->version_id, handler->complete_callback);
    }

    if(!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_delete;
    params.key = object_info->key;
    params.queryParams = queryParams[0] ? queryParams : 0;
    params.properties_callback =  handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api= use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave delete_object successfully !");
}

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
    if(doData->contents_count<1)
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
    memset_s(contents,sizeof(obs_delete_objects) * doData->contents_count, 0, sizeof(obs_delete_objects) * doData->contents_count);

    int contents_count = doData->contents_count;
    for (i = 0; i < contents_count; i++) {
        obs_delete_objects *contentDest = &(contents[i]);
        delete_object_contents *contentSrc = &(doData->contents[i]);
        contentDest->key = contentSrc->key;
        contentDest->code =contentSrc->code;
        contentDest->message = contentSrc->message;
        contentDest->delete_marker = contentSrc->delete_marker;
        contentDest->delete_marker_version_id =contentSrc->delete_marker_version_id;
    }
    iRet = (*(doData->delete_object_data_callback))
           (contents_count, contents, doData->callback_data);
    CHECK_NULL_FREE(contents);
    return iRet;
}


static obs_status deleteObjectXmlCallback(const char *elementPath, const char *data,
                            int dataLen, void *callback_data)
{
    delete_object_data *doData = (delete_object_data *) callback_data;

    int fit;
    if (data) {
        if (!strcmp(elementPath, "DeleteResult/Deleted/Key")) {
            delete_object_contents *contents =
                &(doData->contents[doData->contents_count]);
            string_buffer_append(contents->key, data, dataLen, fit);
            string_buffer_append(contents->code, "0", 1, fit);
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
    }
    else if (!strcmp(elementPath, "DeleteResult/Deleted") || !strcmp(elementPath, "DeleteResult/Error")) 
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
    }
    (void) fit;
    return OBS_STATUS_OK;
}

static int deleteObjectDataToObsCallback(int buffer_size, char *buffer,
                                        void *callback_data)
{
    delete_object_data *doData = (delete_object_data *) callback_data;
    if (!doData->docLen) {
        return 0;
    }
    int remaining = (doData->docLen - doData->docBytesWritten);
    int toCopy = buffer_size > remaining ? remaining : buffer_size;
    if (!toCopy) {
        return 0;
    }
    
	
	errno_t err = EOK;  
	err = memcpy_s(buffer, buffer_size,  &(doData->doc[doData->docBytesWritten]), toCopy);
	
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
    delete_object_data *doData = (delete_object_data *) callback_data;
    return simplexml_add(&(doData->simpleXml), buffer, buffer_size);
}

static obs_status deleteObjectPropertiesCallback(
        const obs_response_properties *responseProperties, void *callback_data)
{
      delete_object_data *doData = (delete_object_data *) callback_data;
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
    delete_object_data *doData = (delete_object_data *) callback_data;
    if (doData->contents_count) {
        make_del_Object_callback(doData);
    }
    (*(doData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, doData->callback_data);
    simplexml_deinitialize(&(doData->simpleXml));
    free(doData);
    doData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}

obs_status compose_del_xml(obs_object_info *object_info,obs_delete_object_info *delobj,
                     obs_delete_object_handler *handler, delete_object_data* doData ,void *callback_data)
{
    unsigned int uiIdx = 0;
    (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Delete", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY, OBS_MAX_DELETE_OBJECT_DOC);
    if(delobj->quiet)
    {
        (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Quiet", "true", 
                        NOT_NEED_FORMALIZE, ADD_NAME_CONTENT, OBS_MAX_DELETE_OBJECT_DOC);
    }
    
    for(; uiIdx < delobj->keys_number; uiIdx++)
    {
        (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Object", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY, OBS_MAX_DELETE_OBJECT_DOC);
        (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Key", object_info[uiIdx].key, 
                        NEED_FORMALIZE, ADD_NAME_CONTENT, OBS_MAX_DELETE_OBJECT_DOC);
        if(NULL != object_info[uiIdx].version_id)
        {
            (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "VersionId", 
                    object_info[uiIdx].version_id, NOT_NEED_FORMALIZE, ADD_NAME_CONTENT, OBS_MAX_DELETE_OBJECT_DOC);
        }
        
        (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Object", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY, OBS_MAX_DELETE_OBJECT_DOC); 

        if((doData->docLen >= OBS_MAX_DELETE_OBJECT_DOC) && (uiIdx != (delobj->keys_number -1)))
        {
            (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
            return OBS_STATUS_OutOfMemory;
        }
    }
    (void)add_xml_element_in_bufflen(doData->doc, &doData->docLen, "Delete", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY, OBS_MAX_DELETE_OBJECT_DOC);
    return OBS_STATUS_OK;
}

void batch_delete_objects(const obs_options *options, obs_object_info *object_info,obs_delete_object_info *delobj,     
                          obs_put_properties *put_properties, obs_delete_object_handler *handler, void *callback_data)
{

    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    obs_put_properties  properties;  
    unsigned char doc_md5[16] = {0};
    char base64_md5[64] = {0};
    
    COMMLOG(OBS_LOGINFO, "Enter batch_delete_objects successfully !");
    if(put_properties == NULL)
    {
        memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    }
    else
    {
        memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
        properties = *put_properties;
    }
    if(delobj->keys_number > OBS_MAX_DELETE_OBJECT_NUMBER || NULL == object_info)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Input param keys_number is greater than 100  or obs_object_info is NULL!");
        return;
    }
    if(!options->bucket_options.bucket_name)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    delete_object_data* doData = (delete_object_data *) malloc(sizeof(delete_object_data));
    if (NULL == doData) {
            (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
            COMMLOG(OBS_LOGERROR, "Malloc DeleteObjectData failed!");
            return;
    }
    memset_s(doData,sizeof(delete_object_data), 0, sizeof(delete_object_data));
    simplexml_initialize(&(doData->simpleXml), &deleteObjectXmlCallback, doData);
    doData->responsePropertiesCallback = handler->response_handler.properties_callback;
    doData->responseCompleteCallback = handler->response_handler.complete_callback;
    doData->delete_object_data_callback = handler->delete_object_data_callback;
    doData->callback_data = callback_data;
    doData->docLen = 0;
    doData->docBytesWritten = 0;
    if(OBS_STATUS_OK != compose_del_xml(object_info, delobj,handler, doData ,callback_data))
    {
        return;
    }
    
    COMMLOG(OBS_LOGDEBUG, "batch_delete_objects doc = %s!",doData->doc);
    
    MD5((unsigned char*)doData->doc, (size_t)doData->docLen, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), base64_md5);
    properties.md5 = base64_md5;
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_post;
    params.subResource = "delete";
    params.put_properties = &properties;
    params.properties_callback = &deleteObjectPropertiesCallback;
    params.complete_callback =  &deleteObjectCompleteCallback;
    params.toObsCallback = &deleteObjectDataToObsCallback;
    params.toObsCallbackTotalSize = doData->docLen;
    params.fromObsCallback = &deleteObjectDataFromObsCallback;
    params.callback_data = doData;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave batch_delete_objects successfully !");
}


void upload_part(const obs_options *options, char *key, obs_upload_part_info *upload_part_info, 
                 uint64_t content_length, obs_put_properties *put_properties,
                 server_side_encryption_params *encryption_params,
                 obs_upload_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    COMMLOG(OBS_LOGINFO, "Enter upload_part successfully !");
    if ( !options->bucket_options.bucket_name )
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if( 0 == upload_part_info->part_number || NULL == upload_part_info->upload_id )
    {
        COMMLOG(OBS_LOGERROR, "part_number or upload_id  is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }
    int amp = 0;
    char part_number_string[64] = {0};
    snprintf_sec(part_number_string, sizeof(part_number_string), _TRUNCATE, "%u",
                upload_part_info->part_number);
    safe_append_with_interface_log("partNumber", part_number_string,
                handler->response_handler.complete_callback);
    
    if (upload_part_info->upload_id) {
        safe_append_with_interface_log("uploadId", upload_part_info->upload_id,
            handler->response_handler.complete_callback);
    }
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_put;
    params.key = key;
    params.queryParams= queryParams[0] ? queryParams : 0;
    params.put_properties = put_properties;
    params.encryption_params = encryption_params;
    params.toObsCallback = handler->upload_data_callback;
    params.toObsCallbackTotalSize = content_length;
    params.properties_callback = handler->response_handler.properties_callback;
    params.complete_callback = handler->response_handler.complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave upload_part successfully !");
}

static obs_status initiate_multi_part_upload_xml_callback(const char *elementPath,
                                                          const char *data, int dataLen,
                                                          void *callback_data)
{
    initiate_multi_part_upload_data *imuData = (initiate_multi_part_upload_data *) callback_data;

    int fit;
    if (data && !strcmp(elementPath, "InitiateMultipartUploadResult/UploadId")) {
        string_buffer_append(imuData->uploadID, data, dataLen, fit);
    }
    (void) fit;

    return OBS_STATUS_OK;
}

static obs_status initiate_multi_part_upload_properties_callback
    (const obs_response_properties *responseProperties, void *callback_data)
{
    initiate_multi_part_upload_data *imuData = (initiate_multi_part_upload_data *) callback_data;
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
    initiate_multi_part_upload_data *imuData = (initiate_multi_part_upload_data *) callback_data;

    return simplexml_add(&(imuData->simpleXml), buffer, buffer_size);
}


static void initiate_multi_part_upload_complete_callback(obs_status requestStatus,
                                       const obs_error_details *s3ErrorDetails,
                                       void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    initiate_multi_part_upload_data *imuData = (initiate_multi_part_upload_data *) callback_data;

    snprintf_sec(imuData->upload_id_return, sizeof(imuData->uploadID),
             imuData->upload_id_return_size, "%s",
             imuData->uploadID);

    (void)(*(imuData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, imuData->callback_data);

    simplexml_deinitialize(&(imuData->simpleXml));

    free(imuData);
    imuData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}


void initiate_multi_part_upload(const obs_options *options, char *key,int upload_id_return_size,
                                char *upload_id_return, obs_put_properties *put_properties,
                                server_side_encryption_params *encryption_params,
                                obs_response_handler *handler, void *callback_data)
{
    request_params params;
    COMMLOG(OBS_LOGINFO, "Enter initiate_multi_part_upload successfully !");
    if ( !options->bucket_options.bucket_name )
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    initiate_multi_part_upload_data *imuData =
        (initiate_multi_part_upload_data *) malloc(sizeof(initiate_multi_part_upload_data));
    
    if (!imuData) 
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc InitiateMultipartUploadData failed !");
        return;
    }
    if(upload_id_return_size < 0 )
    {
        COMMLOG(OBS_LOGERROR, "upload_id_return_size is invalid!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        free(imuData);
        imuData=NULL;
        return;
    }
    memset_s(imuData,sizeof(initiate_multi_part_upload_data), 0, sizeof(initiate_multi_part_upload_data));
    simplexml_initialize(&(imuData->simpleXml), &initiate_multi_part_upload_xml_callback, imuData);
    imuData->responsePropertiesCallback = handler->properties_callback;
    imuData->responseCompleteCallback = handler->complete_callback;
    imuData->callback_data = callback_data;

    imuData->upload_id_return_size = upload_id_return_size;
    imuData->upload_id_return = upload_id_return;
    string_buffer_initialize(imuData->uploadID);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_post;
    params.key = key;
    params.subResource= "uploads";
    params.put_properties = put_properties;
    params.encryption_params = encryption_params;
    params.properties_callback = &initiate_multi_part_upload_properties_callback;
    params.complete_callback = &initiate_multi_part_upload_complete_callback;
    params.fromObsCallback = &initiate_multi_part_upload_data_callback;
    params.callback_data = imuData;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = storage_class;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave initiate_multi_part_upload successfully !");
}

static obs_status complete_multi_part_upload_xml_callback(const char *elementPath, const char *data,
                                                          int dataLen, void *callback_data)
{
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *) callback_data;

    int fit;
    if (!data) 
    {
        return OBS_STATUS_OK;
    }

    if (!strcmp(elementPath, "CompleteMultipartUploadResult/Location")) {
#ifdef WIN32
        char* strTmpSource = (char*)malloc(sizeof(char) * (dataLen + 1));
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(strTmpSource, sizeof(char) * (dataLen + 1), 0, dataLen + 1);
        strncpy_sec(strTmpSource, dataLen+1, data, dataLen);
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
        char* strTmpSource = (char*)malloc(sizeof(char) * (dataLen + 1));
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(strTmpSource, sizeof(char) * (dataLen + 1), 0, dataLen + 1);
        strncpy_sec(strTmpSource, dataLen+1, data, dataLen);
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
    
    (void) fit;
    return OBS_STATUS_OK;
}

static void compose_complete_multi_part_upload_data(complete_multi_part_upload_data* cmuData,
                       unsigned int part_number, obs_complete_upload_Info *complete_upload_Info,
                       int buffer_len)
{
    unsigned int uiIdx = 0;
    (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "CompleteMultipartUpload",
            NULL,NOT_NEED_FORMALIZE,ADD_HEAD_ONLY, buffer_len);
    for(; uiIdx < part_number; uiIdx++)
    {
        if (NULL == complete_upload_Info[uiIdx].etag)
        {
            continue;
        }
        (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "Part",
            NULL,NOT_NEED_FORMALIZE,ADD_HEAD_ONLY, buffer_len);
        cmuData->docLen += snprintf_sec(cmuData->doc + cmuData->docLen, 
                256 * part_number - cmuData->docLen, _TRUNCATE,
                "<PartNumber>%u</PartNumber>", complete_upload_Info[uiIdx].part_number);
        (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "ETag", complete_upload_Info[uiIdx].etag,
                NEED_FORMALIZE, ADD_NAME_CONTENT, buffer_len);  
        (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "Part",
            NULL,NOT_NEED_FORMALIZE,ADD_TAIL_ONLY, buffer_len);
    }
    (void)add_xml_element_in_bufflen(cmuData->doc, &cmuData->docLen, "CompleteMultipartUpload",
            NULL,NOT_NEED_FORMALIZE,ADD_TAIL_ONLY, buffer_len);
    return ;
}

static int complete_multi_part_upload_data_to_obs_callback(int buffer_size, char *buffer,
                                                           void *callback_data)
{
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *) callback_data;

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
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *) callback_data;
    return simplexml_add(&(cmuData->simpleXml), buffer, buffer_size);
}

static void complete_multi_part_upload_complete_callback(obs_status requestStatus,
                                                         const obs_error_details *s3ErrorDetails,
                                                         void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *) callback_data;
    (*(cmuData->complete_multipart_upload_callback))
        (cmuData->location,cmuData->bucket,cmuData->key,cmuData->etag,cmuData->callback_data);
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
    complete_multi_part_upload_data *cmuData = (complete_multi_part_upload_data *) callback_data;

    if(cmuData->responsePropertiesCallback)
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
    if ( !options->bucket_options.bucket_name || part_number > 10000)
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL or part_number is:%u.", part_number);
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    int amp = 0;
    if ( upload_id ) {
        safe_append_with_interface_log("uploadId", upload_id,
            handler->response_handler.complete_callback);
    }
    else
    {
        COMMLOG(OBS_LOGERROR, "upload_id is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }
    complete_multi_part_upload_data* cmuData =
        (complete_multi_part_upload_data *) malloc(sizeof(complete_multi_part_upload_data));
    if (NULL == cmuData) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc cmuData failed !");
        return;
    }
    memset_s(cmuData,sizeof(complete_multi_part_upload_data), 0, sizeof(complete_multi_part_upload_data));
    cmuData->doc = (char*)malloc(ONE_PART_REQUEST_XML_LEN * part_number);
    if (NULL == cmuData->doc) {
        COMMLOG(OBS_LOGERROR, "Malloc cmuData->doc failed !");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        free(cmuData);   
        cmuData = NULL;
        return;
    }
    
    memset_s(cmuData->doc,ONE_PART_REQUEST_XML_LEN * part_number, 0, ONE_PART_REQUEST_XML_LEN * part_number);
    
    simplexml_initialize(&(cmuData->simpleXml), &complete_multi_part_upload_xml_callback, cmuData);
    cmuData->responsePropertiesCallback = handler->response_handler.properties_callback;
    cmuData->responseCompleteCallback = handler->response_handler.complete_callback;
    cmuData->complete_multipart_upload_callback = handler->complete_multipart_upload_callback;
    cmuData->callback_data = callback_data;
    
    cmuData->docLen = 0;
    cmuData->docBytesWritten = 0;
    compose_complete_multi_part_upload_data(cmuData, part_number, complete_upload_Info, 
                256 * part_number);
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

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
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave complete_multi_part_upload successfully !");
}

static obs_status make_list_parts_callback(list_parts_data *lpData)
{
    int i;
    obs_status iRet =  OBS_STATUS_OK;
    int is_truncated = (!strcmp(lpData->is_truncated, "true") ||
                       !strcmp(lpData->is_truncated, "1")) ? 1 : 0;
    char *initiator_id = lpData->initiator_id;
    char *initiator_display_name = lpData->initiator_display_name;
    char *owner_id = lpData->owner_id;
    char *owner_display_name = lpData->owner_display_name;

    if(lpData->parts_count<1)
    {
        COMMLOG(OBS_LOGERROR, "Invalid Malloc Parameter!");
        return OBS_STATUS_InternalError;
    }
    obs_list_parts *parts = (obs_list_parts*)malloc(sizeof(obs_list_parts) * lpData->parts_count);
    if (NULL == parts)
    {
        COMMLOG(OBS_LOGERROR, "Malloc S3ListParts failed!");
        return OBS_STATUS_InternalError;
    }
    memset_s(parts,sizeof(obs_list_parts) * lpData->parts_count, 0, sizeof(obs_list_parts) * lpData->parts_count);

    int parts_count = lpData->parts_count;
    for (i = 0; i < parts_count; i++) {
        obs_list_parts *partsDest = &(parts[i]);
        parts_info *partsSrc = &(lpData->parts[i]);
        partsDest->part_number = partsSrc->part_number;

        partsDest->last_modified =
            parseIso8601Time(partsSrc->last_modified);
        int nTimeZone = getTimeZone();
        partsDest->last_modified += nTimeZone * SECONDS_TO_AN_HOUR;
        partsDest->etag = partsSrc->etag;
        partsDest->size = parseUnsignedInt(partsSrc->size);

    }

    if(lpData->list_parts_callback_ex)
    {
        obs_uploaded_parts_total_info uploadedPartsInfo;
        uploadedPartsInfo.is_truncated = is_truncated;
        uploadedPartsInfo.initiator_id = initiator_id;
        uploadedPartsInfo.nextpart_number_marker = lpData->nextpart_number_marker;
        uploadedPartsInfo.initiator_display_name = initiator_display_name;
        uploadedPartsInfo.owner_id = owner_id;
        uploadedPartsInfo.owner_display_name = owner_display_name;
        uploadedPartsInfo.sorage_class = lpData->storage_class;
        uploadedPartsInfo.parts_count = parts_count;
        
        iRet = (*(lpData->list_parts_callback_ex))(&uploadedPartsInfo, parts,  lpData->callback_data);
    }

    CHECK_NULL_FREE(parts);
    return iRet;
}

static void initialize_parts(parts_info *parts)
{
    parts->part_number = 0;
    string_buffer_initialize(parts->last_modified);
    string_buffer_initialize(parts->etag);
    string_buffer_initialize(parts->size);
}


static void initialize_list_parts_data(list_parts_data *lpData)
{
    lpData->parts_count = 0;
    initialize_parts(lpData->parts);
}


void parse_xmlnode_list_parts(list_parts_data *lpData, const char *elementPath, 
                const char *data, int dataLen)
{
    int fit;
    if(!strcmp(elementPath, "ListPartsResult/Initiator/ID")) {
        string_buffer_append(lpData->initiator_id, data, dataLen, fit);
    }
    else if(!strcmp(elementPath, "ListPartsResult/Initiator/DisplayName")) {
        string_buffer_append(lpData->initiator_display_name, data, dataLen, fit);
    }
    else if(!strcmp(elementPath, "ListPartsResult/Owner/ID")) {
        string_buffer_append(lpData->owner_id, data, dataLen, fit);
    }
    else if(!strcmp(elementPath, "ListPartsResult/Owner/DisplayName")) {
        string_buffer_append(lpData->owner_display_name, data, dataLen, fit);
    }
    else if(!strcmp(elementPath, "ListPartsResult/StorageClass")) {
        string_buffer_append(lpData->storage_class, data, dataLen, fit);
    }
    else if(!strcmp(elementPath, "ListPartsResult/NextPartNumberMarker")) {
        lpData->nextpart_number_marker = atoi(data);
    }
    else if(!strcmp(elementPath, "ListPartsResult/IsTruncated")) {
        string_buffer_append(lpData->is_truncated, data, dataLen, fit);
    }
    else if(!strcmp(elementPath, "ListPartsResult/Part/PartNumber"))
    {
        parts_info *parts = &(lpData->parts[lpData->parts_count]);
        parts->part_number = atoi(data);
    }
    else if(!strcmp(elementPath, "ListPartsResult/Part/LastModified"))
    {
        parts_info *parts = &(lpData->parts[lpData->parts_count]);
        string_buffer_append(parts->last_modified, data, dataLen, fit);
    }
    else if(!strcmp(elementPath, "ListPartsResult/Part/ETag"))
    {
        parts_info *parts = &(lpData->parts[lpData->parts_count]);
        string_buffer_append(parts->etag, data, dataLen, fit);
    }
    else if(!strcmp(elementPath, "ListPartsResult/Part/Size"))
    {
        parts_info *parts = &(lpData->parts[lpData->parts_count]);
        string_buffer_append(parts->size, data, dataLen, fit);
    }

    (void) fit;
    return ;
}


static obs_status ListPartsXmlCallback(const char *elementPath,
                                      const char *data, int dataLen,
                                      void *callback_data)
{
    list_parts_data *lpData = (list_parts_data *) callback_data;
    if (data)
    {
         parse_xmlnode_list_parts(lpData, elementPath, data, dataLen);
    }
    else 
    {
        if (strcmp(elementPath, "ListPartsResult/Part")) 
        {
            return OBS_STATUS_OK;
        }

        lpData->parts_count++;
        if (lpData->parts_count == MAX_PARTS) {
            obs_status status = make_list_parts_callback(lpData);
            if (status != OBS_STATUS_OK) {
                return status;
            }
            initialize_list_parts_data(lpData);
        }
        else {
            initialize_parts(&(lpData->parts[lpData->parts_count]));
        }
    }

    return OBS_STATUS_OK;
}

static obs_status ListPartsPropertiesCallback(
        const obs_response_properties *responseProperties, void *callback_data)
{
     list_parts_data *lpData = (list_parts_data *) callback_data;
      if (lpData->responsePropertiesCallback)
      {
          return (*(lpData->responsePropertiesCallback))(responseProperties, 
              lpData->callback_data);
      }
    
      return OBS_STATUS_OK;
}


static void ListPartsCompleteCallback(obs_status requestStatus,
                                       const obs_error_details *s3ErrorDetails,
                                       void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_parts_data *lpData = (list_parts_data *) callback_data;
    if (lpData->parts_count) {
        make_list_parts_callback(lpData);
    }

    (*(lpData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, lpData->callback_data);

    simplexml_deinitialize(&(lpData->simpleXml));
    free(lpData);
    lpData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

static obs_status ListPartsDataCallback(int buffer_size, const char *buffer,
                                       void *callback_data)
{
    list_parts_data *lpData = (list_parts_data *) callback_data;
    return simplexml_add(&(lpData->simpleXml), buffer, buffer_size);
}

void list_parts (const obs_options *options, char *key, list_part_info *listpart,
                 obs_list_parts_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    COMMLOG(OBS_LOGINFO, "Enter list_parts successfully !");
    if ( !options->bucket_options.bucket_name )
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    int amp = 0;
    if (listpart->upload_id) {
        safe_append_with_interface_log("uploadId", listpart->upload_id,
            handler->response_handler.complete_callback);
    }
    else
    {
        COMMLOG(OBS_LOGERROR, "upload_id is NULL!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }
    
    char max_parts_string[64] = {0};
    snprintf_sec(max_parts_string, sizeof(max_parts_string), _TRUNCATE, "%u",
                listpart->max_parts);
    safe_append_with_interface_log("max-parts",max_parts_string,
            handler->response_handler.complete_callback);
    
    char part_number_string[64] = {0};
    snprintf_sec(part_number_string, sizeof(part_number_string), _TRUNCATE, "%u",
                listpart->part_number_marker);
    safe_append_with_interface_log("part-number-marker", part_number_string,
            handler->response_handler.complete_callback);

    list_parts_data *lpData = (list_parts_data *) malloc(sizeof(list_parts_data));
    if (!lpData) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGINFO, "Malloc ListPartsData failed !");
        return;
    }
    memset_s(lpData,sizeof(list_parts_data), 0, sizeof(list_parts_data));
    simplexml_initialize(&(lpData->simpleXml), &ListPartsXmlCallback, lpData);

    lpData->responsePropertiesCallback =
        handler->response_handler.properties_callback;
    lpData->list_parts_callback_ex = handler->list_parts_callback_ex;
    lpData->responseCompleteCallback =
        handler->response_handler.complete_callback;
    lpData->callback_data = callback_data;

    string_buffer_initialize(lpData->initiator_id);
    string_buffer_initialize(lpData->initiator_display_name);
    string_buffer_initialize(lpData->owner_id);
    string_buffer_initialize(lpData->owner_display_name);
    string_buffer_initialize(lpData->storage_class);
    string_buffer_initialize(lpData->is_truncated);
    initialize_list_parts_data(lpData);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_get;
    params.key = key;
    params.queryParams= queryParams[0] ? queryParams : 0;
    params.fromObsCallback = &ListPartsDataCallback;
    params.properties_callback = &ListPartsPropertiesCallback;
    params.complete_callback = &ListPartsCompleteCallback;
    params.callback_data = lpData;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave list_parts successfully !");
}

void abort_multi_part_upload(const obs_options *options, char *key, const char *upload_id,
                             obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    int amp = 0;
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    COMMLOG(OBS_LOGINFO, "Enter abort_multi_part_upload successfully !");
    if ( !options->bucket_options.bucket_name )
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if (upload_id) {
        safe_append_with_interface_log("uploadId",
            upload_id, handler->complete_callback);
    }
    else
    {
        COMMLOG(OBS_LOGERROR, "upload_id is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_delete;
    params.key = key;
    params.queryParams= queryParams[0] ? queryParams : 0;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave abort_multi_part_upload successfully !");

}

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
    copy_object_data *coData = (copy_object_data *) callback_data;
    int fit;
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
                snprintf_sec(&(coData->etag_return[coData->eTagReturnLen]), 
                         coData->etag_return_size - coData->eTagReturnLen,
                         coData->etag_return_size -
                         coData->eTagReturnLen - 1,
                         "%.*s", dataLen, data);
            if (coData->eTagReturnLen >= coData->etag_return_size) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
    }
    (void) fit;
    
    return OBS_STATUS_OK;
}

static obs_status copyObjectPropertiesCallback(
        const obs_response_properties *responseProperties, void *callback_data)
{
     copy_object_data *coData = (copy_object_data *) callback_data;
      if (coData->responsePropertiesCallback)
      {
          return (*(coData->responsePropertiesCallback))(responseProperties, 
              coData->callback_data);
      }
    
      return OBS_STATUS_OK;
}

static void copyObjectCompleteCallback(obs_status requestStatus, 
                                       const obs_error_details *s3ErrorDetails,void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    copy_object_data *coData = (copy_object_data *) callback_data;

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
static obs_status copyObjectDataCallback(int buffer_size, const char *buffer,
                                       void *callback_data)
{

    copy_object_data *coData = (copy_object_data *) callback_data;

    return simplexml_add(&(coData->simpleXml), buffer, buffer_size);
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
    set_obs_put_properties( put_properties, is_copy);
    if ( !options->bucket_options.bucket_name )
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if(object_info->etag_return_size < 0 
        || NULL == object_info->destination_bucket 
        || NULL == object_info->destination_key){
        COMMLOG(OBS_LOGERROR, "etag_return_size < 0 or destination_bucket or destination_key is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }
    copy_object_data *data =
        (copy_object_data *) malloc(sizeof(copy_object_data));
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

    char versionkey[1024] = {0};
    if(object_info->version_id)
    {
        snprintf_sec(versionkey,sizeof(versionkey), _TRUNCATE,
            "%s?version_id=%s",key, version_id);
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
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
    params.storageClassFormat   = storage_class;
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave copy_object successfully !");

}


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
        tmp_permission= OBS_PERMISSION_WRITE_ACP;
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
    else if (!strcmp(group_uri_str , ACS_GROUP_ALL_USERS)) {
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

obs_status parse_xml_convert_acl(convert_acl_data_info *caData, const char *elementPath,
                                              const char *data, int dataLen)
{
    int fit = 1;
    if (!strcmp(elementPath, "AccessControlPolicy/Owner/ID")) {
         caData->ownerIdLen +=
                snprintf_sec(&(caData->owner_id[caData->ownerIdLen]),
                OBS_MAX_GRANTEE_USER_ID_SIZE + 1 - caData->ownerIdLen,
                OBS_MAX_GRANTEE_USER_ID_SIZE - caData->ownerIdLen - 1,
                "%.*s", dataLen, data);
        if (caData->ownerIdLen >= OBS_MAX_GRANTEE_USER_ID_SIZE) {
            return OBS_STATUS_UserIdTooLong;
        }
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/Owner/DisplayName")) {
        caData->ownerDisplayNameLen +=
                snprintf_sec(&(caData->owner_display_name[caData->ownerDisplayNameLen]),
                OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE + 1 - caData->ownerDisplayNameLen,
                OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE - caData->ownerDisplayNameLen - 1,
                    "%.*s", dataLen, data);
        if (caData->ownerDisplayNameLen >= OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE) {
            return OBS_STATUS_UserDisplayNameTooLong;
        }
    }
    else if (!strcmp(elementPath,
            "AccessControlPolicy/AccessControlList/Grant/Grantee/EmailAddress")) {
        string_buffer_append(caData->email_address, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/ID")) {
        string_buffer_append(caData->userId, data, dataLen, fit);
    }
    else if (!strcmp(elementPath,
            "AccessControlPolicy/AccessControlList/Grant/Grantee/DisplayName")) {
        string_buffer_append(caData->userDisplayName, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/URI")) {
        string_buffer_append(caData->groupUri, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Permission")) {
        string_buffer_append(caData->permission, data, dataLen, fit);
    }
    
    if (!fit) 
    {
        return OBS_STATUS_EmailAddressTooLong;
    }
    return OBS_STATUS_OK;
}

static obs_status convert_acl_xml_callback_s3(const char *elementPath,
                                              const char *data, int dataLen,
                                              void *callback_data)
{
    convert_acl_data_info *caData = (convert_acl_data_info *) callback_data;

    obs_status ret_status;

    if (data) {
        ret_status =  parse_xml_convert_acl(caData, elementPath, data, dataLen);
        if (OBS_STATUS_OK != ret_status)
        {
            COMMLOG(OBS_LOGERROR, "parse_xml_convert_acl failed.");
            return ret_status;
        }
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant"))
    {
        if (*(caData->acl_grant_count_return) == OBS_MAX_ACL_GRANT_COUNT) {
            return OBS_STATUS_TooManyGrants;
        }

        obs_acl_grant *grant = &(caData->acl_grants[*(caData->acl_grant_count_return)]);

        if (caData->email_address[0]) {
            grant->grantee_type = OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL;
            strcpy_s(grant->grantee.huawei_customer_by_email.email_address,
                    sizeof(grant->grantee.huawei_customer_by_email.email_address),
                    caData->email_address);
        }
        else if (caData->userId[0] && caData->userDisplayName[0]) {
            grant->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
            strcpy_s(grant->grantee.canonical_user.id, sizeof(grant->grantee.canonical_user.id), caData->userId);
            strcpy_s(grant->grantee.canonical_user.display_name,
                    sizeof(grant->grantee.canonical_user.display_name),
                    caData->userDisplayName);
        }   
        else if (caData->groupUri[0]) {
            grant->grantee_type  = convert_group_uri_str(caData->groupUri);
        }
        else {
            return OBS_STATUS_BadGrantee;
        }

        grant->permission = convert_obs_permission_str(caData->permission);
        (*(caData->acl_grant_count_return))++;

        string_buffer_initialize(caData->email_address);
        string_buffer_initialize(caData->userId);
        string_buffer_initialize(caData->userDisplayName);
        string_buffer_initialize(caData->groupUri);
        string_buffer_initialize(caData->permission);
    }

    return OBS_STATUS_OK;
}

//
static obs_status convert_acl_xml_callback_obs(const char *elementPath,
                                              const char *data, int dataLen,
                                              void *callback_data)
{
    convert_acl_data_info *caData = (convert_acl_data_info *) callback_data;
    string_buffer(object_delivered_string, 32);	
    string_buffer_initialize(object_delivered_string);

    int fit = 1;

    if (data) {
        if (!strcmp(elementPath, "AccessControlPolicy/Owner/ID")) {
            caData->ownerIdLen +=
                snprintf_sec(&(caData->owner_id[caData->ownerIdLen]),
                OBS_MAX_GRANTEE_USER_ID_SIZE + 1 - caData->ownerIdLen,
                OBS_MAX_GRANTEE_USER_ID_SIZE - caData->ownerIdLen - 1,
                "%.*s", dataLen, data);
            if (caData->ownerIdLen >= OBS_MAX_GRANTEE_USER_ID_SIZE) {
                return OBS_STATUS_UserIdTooLong;
            }
        }
        else if (!strcmp(elementPath, "AccessControlPolicy/Delivered")) {
            string_buffer_append(object_delivered_string, data, dataLen, fit);
            *(caData->object_delivered) = convert_obs_object_delivered_str(object_delivered_string);
        }
        else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/ID")) {
            string_buffer_append(caData->userId, data, dataLen, fit);
        }
        else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Grantee/Canned")) {
            string_buffer_append(caData->groupUri, data, dataLen, fit);
        }
        else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Permission")) {
            string_buffer_append(caData->permission, data, dataLen, fit);
        }
        else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant/Delivered")) {
            string_buffer_append(caData->bucket_delivered, data, dataLen, fit);
        }

        if (!fit) {
            return OBS_STATUS_UserIdTooLong;
        }
    }
    else if (!strcmp(elementPath, "AccessControlPolicy/AccessControlList/Grant")) {
        if (*(caData->acl_grant_count_return) == OBS_MAX_ACL_GRANT_COUNT) {
            return OBS_STATUS_TooManyGrants;
        }

        obs_acl_grant *grant = &(caData->acl_grants[*(caData->acl_grant_count_return)]);

        if (caData->userId[0]) {
            grant->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
            strcpy_s(grant->grantee.canonical_user.id, sizeof(grant->grantee.canonical_user.id), caData->userId);
        }   
        else if (caData->groupUri[0]) {
            grant->grantee_type  = OBS_GRANTEE_TYPE_ALL_USERS;
        }
        else {
            return OBS_STATUS_BadGrantee;
        }

        grant->permission = convert_obs_permission_str(caData->permission);
        grant->bucket_delivered = convert_obs_bucket_delivered_str(caData->bucket_delivered);
        (*(caData->acl_grant_count_return))++;

        string_buffer_initialize(caData->userId);
        string_buffer_initialize(caData->groupUri);
        string_buffer_initialize(caData->permission);
        string_buffer_initialize(caData->bucket_delivered);
    }

    return OBS_STATUS_OK;
}

static obs_status convert_acl_xml_callback(const char *elementPath,
                                           const char *data, int dataLen,
                                           void *callback_data)
{
    
    convert_acl_data_info *caData = (convert_acl_data_info *) callback_data;
    if (caData->use_api == OBS_USE_API_S3) {
        return convert_acl_xml_callback_s3(elementPath, data, dataLen, callback_data);
    } else {
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
    memset_s(&simpleXml,sizeof(simpleXml), 0, sizeof(simple_xml));
    simplexml_initialize(&simpleXml, &convert_acl_xml_callback, &data);

    obs_status status = simplexml_add(&simpleXml, aclXml, strlen(aclXml));

    simplexml_deinitialize(&simpleXml);

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
    return status;
}


static obs_status getAclDataCallback(int buffer_size, const char *buffer,
                                   void *callback_data)
{
    get_acl_data *gaData = (get_acl_data *) callback_data;

    int fit;

    string_buffer_append(gaData->aclXmlDocument, buffer, buffer_size, fit);

    return fit ? OBS_STATUS_OK : OBS_STATUS_XmlDocumentTooLarge;
}

static obs_status getAclPropertiesCallback(
        const obs_response_properties *responseProperties, void *callback_data)
{
      get_acl_data *gaData = (get_acl_data *) callback_data;
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

    get_acl_data *gaData = (get_acl_data *) callback_data;

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
        safe_append("versionId", aclinfo->object_info.version_id, handler->complete_callback);
    }
    get_acl_data *gaData = (get_acl_data *) malloc(sizeof(get_acl_data));
    if (!gaData) {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc GetAclData failed!");
        return;
    }
    memset_s(gaData, sizeof(get_acl_data), 0 , sizeof(get_acl_data));
    if(!options->bucket_options.bucket_name ){
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
    gaData->use_api= use_api;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_get;
    params.key = aclinfo->object_info.key;
    params.queryParams= queryParams[0] ? queryParams : 0;
    params.subResource= "acl";
    params.fromObsCallback = &getAclDataCallback;
    params.properties_callback = &getAclPropertiesCallback;
    params.complete_callback = &getAclCompleteCallback;
    params.callback_data = gaData;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave get_object_acl successfully !");
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
        switch (grant->grantee_type) {
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
#define append(fmt, ...)                                                  \
    do {                                                                 \
        *xmlDocumentLenReturn += snprintf_sec                              \
            (&(xmlDocument[*xmlDocumentLenReturn]), xmlDocumentBufferSize - *xmlDocumentLenReturn , \
             xmlDocumentBufferSize - *xmlDocumentLenReturn - 1,           \
             fmt, __VA_ARGS__);                                           \
        if (*xmlDocumentLenReturn >= xmlDocumentBufferSize) {             \
            return OBS_STATUS_XmlDocumentTooLarge;                           \
        } \
    } while (0)

    if (type == TYPE_OBJECT_ACL)
    {
        append("<AccessControlPolicy><Owner><ID>%s</ID></Owner>",owner_id);
        append("<Delivered>%s</Delivered><AccessControlList>", ((object_delivered == OBJECT_DELIVERED_TRUE)? "true" : "false"));     // For object, true is the default value for delivered.
    	}
    else
    {
        append("<AccessControlPolicy><Owner><ID>%s</ID></Owner><AccessControlList>", owner_id);
    }

    int i;
    for (i = 0; i < aclGrantCount; i++) {
        append("%s", "<Grant><Grantee>");
        const obs_acl_grant *grant = &(acl_grants[i]);
        switch (grant->grantee_type) {
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
        append("</Grantee><Permission>%s</Permission>",
                ((grant->permission == OBS_PERMISSION_READ) ? "READ" :
                (grant->permission == OBS_PERMISSION_WRITE) ? "WRITE" :
                (grant->permission == OBS_PERMISSION_READ_ACP) ? "READ_ACP" :
                (grant->permission == OBS_PERMISSION_WRITE_ACP) ? "WRITE_ACP" :
                (grant->permission == OBS_PERMISSION_FULL_CONTROL) ? "FULL_CONTROL" : "READ"));
        if (type != TYPE_OBJECT_ACL)
        {
            append("<Delivered>%s</Delivered>", ((grant->bucket_delivered == BUCKET_DELIVERED_FALSE)? "false" : "true"));  								// For bucket, false is the default value for delivered.
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
    } else {
        return generate_acl_xml_document_obs(owner_id, object_delivered, aclGrantCount, acl_grants, xmlDocumentLenReturn, xmlDocument, xmlDocumentBufferSize, type);
    }
}

static int setAclDataCallback(int buffer_size, char *buffer, void *callback_data)
{
    set_acl_data *paData = (set_acl_data *) callback_data;

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

    set_acl_data *paData = (set_acl_data *) callback_data;

    (void)(*(paData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, paData->callback_data); 
    free(paData); 
    paData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}

void set_object_acl(const obs_options *options, manager_acl_info *aclinfo, obs_response_handler *handler, void *callback_data)
{
    obs_type_acl type = TYPE_OBJECT_ACL;
    set_common_acl(options, aclinfo, type, handler, callback_data);
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
    if(!options->bucket_options.bucket_name ){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if (*(aclinfo->acl_grant_count_return)> OBS_MAX_ACL_GRANT_COUNT) {
        (void)(*(handler->complete_callback))
            (OBS_STATUS_TooManyGrants, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Input param aclGrantCount is greater than S3_MAX_ACL_GRANT_COUNT(100) !");
        return;
    }
    set_acl_data *data = (set_acl_data *) malloc(sizeof(set_acl_data));
    if (!data) {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc SetAclData failed !");
        return;
    }
    memset_s(data, sizeof(set_acl_data ), 0 , sizeof(set_acl_data ));
    if (aclinfo->object_info.version_id) {
        safe_append("versionId", aclinfo->object_info.version_id, handler->complete_callback);
    }
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
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_put;
    params.key = aclinfo->object_info.key;
    params.queryParams= queryParams[0] ? queryParams : 0;
    params.subResource= "acl";
    params.toObsCallback = &setAclDataCallback;
    params.toObsCallbackTotalSize = data->aclXmlDocumentLen;
    params.properties_callback = handler->properties_callback;
    params.complete_callback =  &setAclCompleteCallback;
    params.callback_data = data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave set_object_acl successfully !");
}

void set_object_acl_by_head(const obs_options *options, obs_object_info *object_info, 
                     obs_canned_acl canned_acl, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    obs_put_properties properties;
    COMMLOG(OBS_LOGINFO, "Enter set_object_acl_by_head successfully !");
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;
    if(!options->bucket_options.bucket_name ){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if (object_info && (object_info->version_id)) {
        safe_append("versionId", object_info->version_id, handler->complete_callback);
    }

    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = canned_acl;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth = options->temp_auth;
    params.httpRequestType = http_request_type_put;
    params.key = object_info ? object_info->key : NULL;
    params.queryParams= queryParams[0] ? queryParams : 0;
    params.subResource= "acl";
    params.put_properties = &properties;
    params.properties_callback = handler->properties_callback;
    params.complete_callback =  handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = default_storage_class;
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave set_object_acl_by_head successfully !");
}

static obs_status CopyPartXmlCallback(const char *elementPath,
                                    const char *data, int dataLen,
                                      void *callback_data)
{
    copy_object_data *cpData = (copy_object_data *) callback_data;

    int fit;
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
                snprintf_sec(&(cpData->etag_return[cpData->eTagReturnLen]), 
                           cpData->etag_return_size - cpData->eTagReturnLen,
                           cpData->etag_return_size - cpData->eTagReturnLen - 1,
                           "%.*s", dataLen, data);
            if (cpData->eTagReturnLen >= cpData->etag_return_size) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
    }

    (void) fit;

    return OBS_STATUS_OK;
}

static obs_status CopyPartPropertiesCallback(
        const obs_response_properties *responseProperties, void *callback_data)
{
      copy_object_data *cpData = (copy_object_data *) callback_data;
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

    copy_object_data *cpData = (copy_object_data *) callback_data;

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
               server_side_encryption_params *encryption_params,obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter copy_part successfully !");
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    
    int amp = 0;
    if (copypart->part_number > 0) {
        char part_number_string[64] = {0};
        snprintf_sec(part_number_string, sizeof(part_number_string), _TRUNCATE, "%u", copypart->part_number);
        safe_append_with_interface_log("partNumber",
            part_number_string, handler->complete_callback);
    }
    if (copypart->upload_id) {
        safe_append_with_interface_log("uploadId",
            copypart->upload_id, handler->complete_callback);
    }
    if ( !options->bucket_options.bucket_name )
    {
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if(object_info->etag_return_size < 0 || NULL == object_info->destination_bucket || NULL == object_info->destination_key){
        COMMLOG(OBS_LOGERROR, "etag_return_size < 0 or destination_bucket or destination_key is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, callback_data);
        return;
    }
    copy_object_data *data = (copy_object_data *) malloc(sizeof(copy_object_data));
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
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth =options->temp_auth;
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
    params.storageClassFormat   = no_need_storage_class;
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave copy_part successfully !");
}

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
    int iRemaining      = 0;
    int iRet            = 0;
    set_sal_data *pstData = NULL;

    pstData = (set_sal_data *) callback_data;
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
      set_sal_data *pstData = (set_sal_data *) callback_data;
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
    pstData = (set_sal_data *) callback_data;
    (void)(*(pstData->responseCompleteCallback))
        (enRequestStatus, pstS3ErrorDetails, pstData->callback_data);
    free(pstData);
    pstData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


void restore_object(const obs_options *options, obs_object_info *object_info, const char *days, 
                obs_tier tier,const obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter restore_object successfully !");
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    int amp = 0;

    if(!options->bucket_options.bucket_name ){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, callback_data);
        return;
    }
    if(NULL == object_info->key || !strlen(object_info->key))
    {
        COMMLOG(OBS_LOGERROR, "key is NULL!");
        return;
    }
    if(NULL == days || !strlen(days))
    {
        COMMLOG(OBS_LOGERROR, "days is NULL!");
        return;
    }
    if (object_info->version_id)
    {
        safe_append("versionId", object_info->version_id, handler->complete_callback);
    }
    set_sal_data *data = (set_sal_data *) malloc(sizeof(set_sal_data));
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
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.temp_auth =options->temp_auth;
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
    params.storageClassFormat   = no_need_storage_class;
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave restore_object successfully !");
}

void obs_options_obj_or_bucket(const obs_options *options, int is_bucket, char* key, char* origin,
                    char (*request_method)[OBS_COMMON_LEN_256], unsigned int method_number, 
                    char (*request_header)[OBS_COMMON_LEN_256], unsigned int header_number, 
                    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    if(NULL == request_method || NULL == origin){
        COMMLOG(OBS_LOGERROR, "requestMethod or origin is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, 0);
        return;
    }
    if (0 == is_bucket && (NULL == key || !strlen(key)))
    {
        COMMLOG(OBS_LOGERROR, "Key is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, 0);
        return;
    }

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, 0);
        return;
    }
    unsigned int i = 0;
    obs_cors_conf corsConf;
    corsConf.origin = origin;
    corsConf.rmNumber = method_number;
    corsConf.rhNumber = header_number;
    for(i = 0; i < method_number; i ++)
    {
        corsConf.requestMethod[i] = request_method[i];
    }

    for(i = 0; i < header_number; i ++)
    {
        corsConf.requestHeader[i] = request_header[i];
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.temp_auth =options->temp_auth;
    params.httpRequestType = http_request_type_options;
    params.key = key;
    params.corsConf = &corsConf;
    params.properties_callback = handler->properties_callback;
    params.complete_callback = handler->complete_callback;
    params.callback_data = callback_data;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void obs_options_object(const obs_options *options, char* key, char* origin,
                    char (*request_method)[OBS_COMMON_LEN_256], unsigned int method_number,
                    char (*request_header)[OBS_COMMON_LEN_256], unsigned int header_number,
                    obs_response_handler *handler, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    obs_options_obj_or_bucket(options, 0, key, origin, request_method, method_number,
        request_header, header_number, handler, callback_data);

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}

int getUploadFileSummary(upload_file_summary *pstUploadFileSummay, const char * file_name)
{
    if(file_name == NULL)
    {
        return -1;
    }
    else
    {
        int ret_stat = -1;

#if defined __GNUC__ || defined LINUX  
        struct stat statbuf;
        ret_stat = stat(file_name, &statbuf);
#else
        struct _stati64 statbuf;
        ret_stat = _stati64(file_name,&statbuf);
#endif
         if(ret_stat==-1) 
         {
             return -1;
         }
         else
         {
              pstUploadFileSummay->fileSize = statbuf.st_size;
              pstUploadFileSummay->lastModify = statbuf.st_mtime;
         }
    }

    return 0;    
}

int isXmlFileValid(const char * file_name,exml_root xmlRootIn)
{
    xmlNodePtr curNode;  
    int retVal = 0;
    xmlDocPtr doc;           //the doc pointer to parse the file
    doc = xmlReadFile(file_name,"utf-8",XML_PARSE_RECOVER); //parse the xml file

    if (NULL == doc)    
    {        
        COMMLOG(OBS_LOGERROR, "Document not parsed successfully. ");
        xmlFreeDoc(doc); 
        return 0;    
    }

    curNode = xmlDocGetRootElement(doc); //get the root node
    if (NULL == curNode) 
    {       
        COMMLOG(OBS_LOGERROR, "empty document");
        xmlFreeDoc(doc);       
        return 0;    
    }

    if ((!xmlStrcmp(curNode->name, BAD_CAST "uploadinfo")) && (xmlRootIn == UPLOAD_FILE_INFO) )  
    {       
        retVal = 1;  
    }

     if ((!xmlStrcmp(curNode->name, BAD_CAST "downloadinfo")) && (xmlRootIn == DOWNLOAD_FILE_INFO) )  
    {       
        retVal = 1;  
    }
    xmlFreeDoc(doc); 
    return retVal;

}

int open_file(const char * file_name, int *ret_stat, int *file_size)
{
    int fd = 0;
    
#if defined __GNUC__ || defined LINUX  
    struct stat statbuf;
    *ret_stat = stat(file_name, &statbuf);
#else
    struct _stati64 statbuf;
    *ret_stat = _stati64(file_name,&statbuf);
#endif
    if (*ret_stat==-1) 
    { 
#if defined __GNUC__ || defined LINUX
        fd = open(file_name,O_RDWR | O_CREAT, S_IRUSR|S_IWUSR);
#else
        (void)_sopen_s( &fd, file_name, _O_RDWR |_O_CREAT |_O_BINARY, _SH_DENYNO, 
                  _S_IREAD | _S_IWRITE );
#endif
        if (fd != -1)
        {
            close(fd);
        }
    }
    else
    {
        *file_size = statbuf.st_size;
    }

    return fd;
}


int set_check_pointFile_with_name(const char * checkPointFileName, 
                             int * isFirstTimeUpload, exml_root xmlRootIn)
{
    int retVal = 0;
    int fd = -1;
    int ret_stat = -1;
    int file_size =0;
    
    fd = open_file(checkPointFileName, &ret_stat, &file_size);
    if (fd == -1)
    {
        COMMLOG(OBS_LOGERROR, "%s create checkpoint file failed !","setCheckPointFile");
        return -1;
    }
    
    if (ret_stat==-1) 
    { 
        *isFirstTimeUpload = 1;    
        retVal = 0;
    }
    else
    {
        retVal = 0;
        if(file_size != 0)
        {
            retVal = isXmlFileValid(checkPointFileName, xmlRootIn);
        }
        
        if(retVal == 1)
        {
            *isFirstTimeUpload = 0;                    
        }
        else
        {
           retVal = -1;
           COMMLOG(OBS_LOGERROR, "%s check point file exist but is not valid !","setCheckPointFile");
        }
    } 
    
    return retVal;
}

int set_check_pointFile_with_null(const char * uploadFileName, char * checkPointFileName, 
                             int * isFirstTimeUpload, exml_root xmlRootIn)
{
    int retVal = 0;
    int fd = -1;
    int ret_stat = 0;
    int isxmlValid = -1;
    int file_size =0;
    
    sprintf_sec(checkPointFileName,1024,"%s%s",uploadFileName,".xmltmp");
    while (ret_stat == 0)
    {
        fd = open_file(checkPointFileName, &ret_stat, &file_size);
        if (fd == -1)
        {
            COMMLOG(OBS_LOGERROR, "%s create checkpoint file failed !","setCheckPointFile");
            return -1;
        }
        
        if(ret_stat == -1)
        {           
            *isFirstTimeUpload = 1;    
            retVal = 0;
            break; 
        }

        if (file_size != 0)
        {
            isxmlValid = isXmlFileValid(checkPointFileName,xmlRootIn);
        }
        
        if(isxmlValid == 1)
        {               
            *isFirstTimeUpload = 0;
            retVal = 0;
            break;
        }           

        retVal = strcat_s(checkPointFileName,1024,".xmltmp");
        if(retVal != 0)
        {
            retVal =  -1;
            break;
        }
    }

    return retVal;
}


static int setCheckPointFile(const char * uploadFileName, char * checkPointFileName, 
                             int * isFirstTimeUpload, exml_root xmlRootIn)
{    
    *isFirstTimeUpload = 1;
   
    if (uploadFileName == NULL)
    {
        return -1;
    }

    if ((checkPointFileName != NULL) && (checkPointFileName[0] != '\0'))
    {
        return set_check_pointFile_with_name(checkPointFileName, isFirstTimeUpload, xmlRootIn);
    }
    else 
    {
        return set_check_pointFile_with_null(uploadFileName, checkPointFileName, isFirstTimeUpload,
                    xmlRootIn);
    }
}

void cleanUploadList(upload_file_part_info * uploadPartList)
{
    upload_file_part_info * ptrUploadPart = uploadPartList;
    upload_file_part_info * ptrUploadPartNext = uploadPartList;
    while(ptrUploadPart)
    {
        ptrUploadPartNext = ptrUploadPart->next;

        free(ptrUploadPart);
        ptrUploadPart = NULL;

        ptrUploadPart = ptrUploadPartNext;
    }
}

part_upload_status GetUploadStatusEnum(const char * strStatus)
{
    if(!strcmp(strStatus,"UPLOAD_NOTSTART"))
    {
        return UPLOAD_NOTSTART;
    }
    else if(!strcmp(strStatus,"UPLOADING"))
    {
        return UPLOADING;
    }
    else if(!strcmp(strStatus,"UPLOAD_FAILED"))
    {
        return UPLOAD_FAILED;
    }
    else if(!strcmp(strStatus,"UPLOAD_SUCCESS"))
    {
        return UPLOAD_SUCCESS;
    }
    else
    {
        return UPLOAD_NOTSTART;
    }
}

int check_file_is_valid(char *file_name)
{
    int ret_stat = -1;
#if defined __GNUC__ || defined LINUX  
    struct stat statbuf;
    ret_stat = stat(file_name, &statbuf);
#else
    struct _stati64 statbuf;
    ret_stat = _stati64(file_name,&statbuf);
#endif
    if(ret_stat == -1)
    {
        COMMLOG(OBS_LOGERROR, "%s file[%s] is not exist","readCheckpointFile",file_name);
        return -1;
    }
   
    if(statbuf.st_size == 0)
    {
        COMMLOG(OBS_LOGERROR, "%s checkpoint file[%s] size is 0 !","readCheckpointFile",file_name);
        return -1;
    }
    
    return 0;
}

xmlNodePtr get_xmlnode_from_file(const char * file_name,  xmlDocPtr *doc)
{
    xmlNodePtr curNode;  
    *doc = xmlReadFile(file_name, "utf-8", XML_PARSE_RECOVER);     
    if (NULL == *doc)    
    {        
        COMMLOG(OBS_LOGERROR, "Document not parsed successfully.");
        return NULL;    
    }

    curNode = xmlDocGetRootElement(*doc);
    if (NULL == curNode) 
    {       
        COMMLOG(OBS_LOGERROR, "empty document");
        xmlFreeDoc(*doc);       
        return NULL;    
    }

    return curNode;
}

void parse_xmlnode_fileinfo(upload_file_summary * pstUploadFileSummary, xmlNodePtr fileinfoNode)
{
    while(fileinfoNode != NULL)          
    {             
         xmlChar *nodeContent = xmlNodeGetContent(fileinfoNode);  
		 errno_t err = EOK;

         if(!xmlStrcmp(fileinfoNode->name,(xmlChar *)"filesize"))
         {
             pstUploadFileSummary->fileSize = parseUnsignedInt((char*)nodeContent);
         }
         else if(!xmlStrcmp((xmlChar *)fileinfoNode->name,(xmlChar *)"lastmodify"))
         {
             pstUploadFileSummary->lastModify = parseUnsignedInt((char*)nodeContent);
         }
         else if(!xmlStrcmp(fileinfoNode->name,(xmlChar *)"md5"))
         {
         }
         else if(!xmlStrcmp(fileinfoNode->name,(xmlChar *)"checksum"))
         {
             pstUploadFileSummary->fileCheckSum = (int)parseUnsignedInt((char*)nodeContent);
         }
         else if(!xmlStrcmp(fileinfoNode->name,(xmlChar *)"uploadid"))
         {
             err = memcpy_s(pstUploadFileSummary->upload_id,MAX_SIZE_UPLOADID,nodeContent,strlen((char*)nodeContent)+1);
         }
         else if(!xmlStrcmp(fileinfoNode->name,(xmlChar *)"bucketname"))
         {
             err = memcpy_s(pstUploadFileSummary->bucket_name,MAX_BKTNAME_SIZE,nodeContent,strlen((char*)nodeContent)+1);
         }
         else if(!xmlStrcmp(fileinfoNode->name,(xmlChar *)"key"))
         {
             err = memcpy_s(pstUploadFileSummary->key,MAX_KEY_SIZE,nodeContent,strlen((char*)nodeContent)+1);
         }
		 
		 if (err != EOK)
		 {
			 COMMLOG(OBS_LOGWARN, "parse_xmlnode_fileinfo: memcpy_s failed !");
		 }
		 
         xmlFree(nodeContent);             
         fileinfoNode = fileinfoNode->next;          
    }  
    
    return;
}

int parse_xmlnode_partsinfo(upload_file_part_info ** uploadPartList, xmlNodePtr partNode, int *partCount)
{
    upload_file_part_info * pstUploadPart = NULL;
    upload_file_part_info * uploadPartNode;
    int partCountTmp = 0;
    xmlNodePtr partinfoNode = NULL;
    *uploadPartList = NULL;
    
    while(partNode)
    {
        if (strncmp((char*)partNode->name,"part",strlen("part")))
        {
            partNode = partNode->next;
            continue;
        }
 
        uploadPartNode = (upload_file_part_info *)malloc(sizeof(upload_file_part_info));
        if(uploadPartNode == NULL)
        {
           COMMLOG(OBS_LOGERROR, "int readCheckpointFile, malloc for uploadPartNode failed");
           cleanUploadList(*uploadPartList);
           partCountTmp = 0;
           *partCount = 0;
           return -1;
        }
        uploadPartNode->next = NULL;
        partCountTmp ++;

        partinfoNode = partNode->xmlChildrenNode;
        while(partinfoNode != NULL)          
        {             
           xmlChar *nodeContent = xmlNodeGetContent(partinfoNode);             
           COMMLOG(OBS_LOGINFO, "name:%s content %s\n",partinfoNode->name,nodeContent);
           if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"partNum"))
           {
               uploadPartNode->part_num = (int)parseUnsignedInt((char*)nodeContent) - 1;
           }
           else if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"etag"))
           {
                memset(uploadPartNode->etag,0,MAX_SIZE_ETAG);
			    errno_t err = EOK;  
				err = memcpy_s(uploadPartNode->etag, MAX_SIZE_ETAG, nodeContent, strlen((char*)nodeContent));
				if (err != EOK)
				{
					COMMLOG(OBS_LOGWARN, "parse_xmlnode_partsinfo: memcpy_s failed!\n");
					return -1;
				}
           }
           else if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"startByte"))
           {
               uploadPartNode->start_byte = parseUnsignedInt((char*)nodeContent);
           }
           else if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"partSize"))
           {
               uploadPartNode->part_size = parseUnsignedInt((char*)nodeContent);
           }
           else if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"uploadStatus"))
           {
               uploadPartNode->uploadStatus = GetUploadStatusEnum((char*)nodeContent);
           }                       
           
           xmlFree(nodeContent);  
           partinfoNode = partinfoNode->next;                       
        } 

        uploadPartNode->prev = pstUploadPart;
        if(pstUploadPart == NULL)
        {
           pstUploadPart = uploadPartNode;
           *uploadPartList  = uploadPartNode;
        }
        else
        {
           pstUploadPart->next = uploadPartNode;
           pstUploadPart = pstUploadPart->next;
        } 

        partNode = partNode->next;
    }
    
    *partCount = partCountTmp;   
    return 0;
}



int readCheckpointFile(upload_file_summary * pstUploadFileSummary,
                       upload_file_part_info ** uploadPartList, 
                       int *partCount,char * file_name)
{
    xmlNodePtr curNode;  
    xmlDocPtr doc;           
    
    if(check_file_is_valid(file_name) == -1)
    {
        return -1;
    }
    
    curNode = get_xmlnode_from_file(file_name, &doc);
    if (NULL == curNode) 
    {           
        return -1;    
    }
   
    *uploadPartList = NULL;
    *partCount = 0;
    
    if (xmlStrcmp((xmlChar *)curNode->name, BAD_CAST "uploadinfo"))    
    {       
        COMMLOG(OBS_LOGERROR, "document of the wrong type, root node != uploadinfo");
        xmlFreeDoc(doc);       
        return -1;    
    }
    curNode = curNode->xmlChildrenNode;
    while(curNode != NULL)    
    {                   
        if(!xmlStrcmp(curNode->name,(xmlChar *)"fileinfo"))       
        {          
             xmlNodePtr fileinfoNode = curNode->xmlChildrenNode; 
             parse_xmlnode_fileinfo(pstUploadFileSummary, fileinfoNode);
        }  
          
        if(!xmlStrcmp(curNode->name,(xmlChar *)"partsinfo"))       
        { 
            xmlNodePtr partNode = curNode->xmlChildrenNode; 
            if (-1 == parse_xmlnode_partsinfo(uploadPartList, partNode, partCount))
            {
               xmlFreeDoc(doc); 
               return -1;
            }
        }
        
        curNode = curNode->next;    
    }

    xmlFreeDoc(doc);  
    return 0;
}

int isUploadFileChanged(upload_file_summary *pstNewSummary, upload_file_summary * pstOldSummary)
{
    if ((pstNewSummary->fileSize == pstOldSummary->fileSize)
            &&(pstNewSummary->lastModify == pstOldSummary->lastModify))
    {
       return 0;
    }
    else
    {
       return 1;
    }
}

static obs_status listPartsCallback_Ex_Intern(obs_uploaded_parts_total_info* uploaded_parts,
                                              obs_list_parts *parts,
                                              void *callback_data)
 {
     if(uploaded_parts || parts || callback_data)
     {
         return OBS_STATUS_OK;
     }
     return OBS_STATUS_OK;
 }


 static obs_status ListPartsPropertiesCallback_Intern(const obs_response_properties *properties, 
                                                      void *callback_data)
{
    if(properties||callback_data)
    {
        return OBS_STATUS_OK;
    }
    return OBS_STATUS_OK;
}

typedef struct
{
    obs_status retStatus;
}lisPartResult;

static void ListPartsCompleteCallback_Intern(obs_status status,
                                             const obs_error_details *error, 
                                             void *callback_data)
{
    lisPartResult * pstResult =  (lisPartResult *)callback_data;

    pstResult->retStatus = status;
    //the following 4 lines is just to avoid warning
    if(error)
    {
        return;
    }
    return;
}

int checkUploadFileInfo(upload_file_summary * pstUploadInfo, const obs_options *options, const char * keyIn)
{
    obs_list_parts_handler listPartsHandler = 
    { 
        { &ListPartsPropertiesCallback_Intern, &ListPartsCompleteCallback_Intern }, &listPartsCallback_Ex_Intern 
    };

    lisPartResult stListPartResult;
    memset_s(&stListPartResult,sizeof(lisPartResult),0,sizeof(lisPartResult));

    if((strlen((const char *)pstUploadInfo->bucket_name)==0) 
        || strcmp((const char *)pstUploadInfo->bucket_name,options->bucket_options.bucket_name))
    {
        return 0;
    }
    if((strlen((const char *)pstUploadInfo->key)==0) || strcmp((const char *)pstUploadInfo->key,keyIn))
    {
        return 0;
    }

    if(strlen((const char *)pstUploadInfo->upload_id)==0) 
    {
        return 0;
    }
    list_part_info listpart;
    memset_s(&listpart,sizeof(list_part_info),0,sizeof(list_part_info));
    listpart.max_parts = 100;
    listpart.upload_id = pstUploadInfo->upload_id;
    //call list parts here
    list_parts(options,(char *)pstUploadInfo->key,&listpart,&listPartsHandler, &stListPartResult);
    if(stListPartResult.retStatus == OBS_STATUS_OK)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


void abortMultipartUploadAndFree(const obs_options *options,  char *key,
                                 const char * upload_id, const char *checkpointFilename, EN_FILE_ACTION enAction)
{
    int fdTemp = -1;
    if(strlen(upload_id)!=0)
    {
        obs_response_handler response_handler =
        {
            &ListPartsPropertiesCallback_Intern, &ListPartsCompleteCallback_Intern 
        };
        lisPartResult stListPartResult;
        memset_s(&stListPartResult,sizeof(lisPartResult),0,sizeof(lisPartResult));
        //abort the upload task here;
        abort_multi_part_upload(options, key, upload_id, &response_handler, &stListPartResult);
    }

    if((checkpointFilename == NULL)||(enAction == DO_NOTHING))
    {
        return;
    }

    //clean up the checkpoint file here.
    if(enAction == CLEAN_FILE)
    {
#if defined __GNUC__ || defined LINUX
        fdTemp = open(checkpointFilename, O_CREAT | O_TRUNC, S_IRUSR|S_IWUSR);

#else
        (void)_sopen_s( &fdTemp, checkpointFilename, _O_CREAT | _O_TRUNC, _SH_DENYNO, 
        _S_IREAD | _S_IWRITE );
#endif 
     }
     else if(enAction  == DELETE_FILE)
     {
         (void)remove(checkpointFilename);
     }
     
    if(fdTemp != -1)
    {
        close(fdTemp); 
        fdTemp = -1;
    } 
}

int setPartList(upload_file_summary *pstUploadFileSummaryNew, uint64_t uploadPartsize,
                upload_file_part_info ** uploadPartList, int *partCount,int isFirstTime)
{
    int partCountTemp = 0;
    upload_file_part_info * uploadPartListTemp = NULL;
    upload_file_part_info * pstuploadPartListTemp = NULL;
    upload_file_part_info * pstpUloadPartPrev = NULL;
    uint64_t lastPartSize  = 0;
    int i = 0;

    upload_file_summary stUploadFileSummaryOld;
    memset(&stUploadFileSummaryOld,0,sizeof(upload_file_summary));
    // read the content from the check point file, and get the  parts list to upload this time
    if(!isFirstTime)
    {
        return 0;
    }
    // this means the client did not want to do checkpoint, or this is the first time to upload this file
    // or the upload file has been changed.
    //we should start to upload the whole file

    //calculate the total count of the parts and the last part size
    partCountTemp = (int)(pstUploadFileSummaryNew->fileSize / uploadPartsize);
    lastPartSize = pstUploadFileSummaryNew->fileSize % uploadPartsize;

    *partCount = partCountTemp;
    //malloc  and set for part list,set the parts info to the part list
    for(i=0;i<partCountTemp;i++)
    {
        pstuploadPartListTemp = (upload_file_part_info*)malloc(sizeof(upload_file_part_info));
        if(pstuploadPartListTemp == NULL)
        {
            COMMLOG(OBS_LOGERROR, "in %s failed to malloc for uploadPartListTemp !", __FUNCTION__);
            cleanUploadList(uploadPartListTemp);
            uploadPartListTemp = NULL;
            return -1; 
        }
        pstuploadPartListTemp->next = NULL;
        pstuploadPartListTemp->part_num = i;
        pstuploadPartListTemp->start_byte = uploadPartsize*i;
        pstuploadPartListTemp->part_size = uploadPartsize;
        memset(pstuploadPartListTemp->etag,0,sizeof(pstuploadPartListTemp->etag));
        pstuploadPartListTemp->uploadStatus = UPLOAD_NOTSTART;
        if(i==0)
        {
            pstuploadPartListTemp->prev = NULL;
            pstpUloadPartPrev = NULL;
            uploadPartListTemp = pstuploadPartListTemp;
        }
        else
        {
            pstuploadPartListTemp->prev = pstpUloadPartPrev;
            pstpUloadPartPrev->next = pstuploadPartListTemp;
        }

        pstpUloadPartPrev = pstuploadPartListTemp;
    }

    if(lastPartSize != 0)
    {
        pstuploadPartListTemp = (upload_file_part_info*)malloc(sizeof(upload_file_part_info));
        if(pstuploadPartListTemp == NULL)
        {
            COMMLOG(OBS_LOGERROR, "in %s failed to malloc for uploadPartListTemp !", __FUNCTION__);
            cleanUploadList(uploadPartListTemp);
            uploadPartListTemp = NULL;
            return -1; 
        }
        pstuploadPartListTemp->prev = pstpUloadPartPrev;

        if(pstpUloadPartPrev)
        {
            pstpUloadPartPrev->next = pstuploadPartListTemp;
        }

        pstuploadPartListTemp->part_num = i;
        pstuploadPartListTemp->start_byte = uploadPartsize*i;
        pstuploadPartListTemp->part_size = lastPartSize;
        memset(pstuploadPartListTemp->etag,0,sizeof(pstuploadPartListTemp->etag));
        pstuploadPartListTemp->uploadStatus = UPLOAD_NOTSTART;
        pstuploadPartListTemp->next = NULL;
        *partCount = partCountTemp + 1;
    }
    else
    {
        pstuploadPartListTemp = NULL;
    }
    *uploadPartList = uploadPartListTemp;
    return 0;
}

int writeCheckpointFile(upload_file_summary * pstUploadFileSummary,
                        upload_file_part_info * uploadPartList, 
                        int partCount, const char * file_name)
{
    char str_content[512] = {0};    
    xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");	
    char contentBuff[64];
    int i=0;  
    int nRel = -1;
    upload_file_part_info * ptrUploadPart;

    //create the root node    
    xmlNodePtr root_node = xmlNewNode(NULL,BAD_CAST"uploadinfo");   

    xmlNodePtr node_fileinfo = xmlNewNode(NULL,BAD_CAST"fileinfo");    
    xmlNodePtr node_partsinfo = xmlNewNode(NULL,BAD_CAST"partsinfo"); 
    //set the root node <uploadinfo>    
    xmlDocSetRootElement(doc,root_node);    
    //add <fileinfo> and <partsinfo> under <uploadinfo>    
               
    sprintf_sec(str_content,512,"%llu",(long long unsigned int)pstUploadFileSummary->fileSize);           
    
    //add <fileinfo> node under <uploadinfo>    
    xmlAddChild(root_node,node_fileinfo);  
    //add <filesize> under <fileinfo>    
    sprintf_sec(str_content,512,"%llu",(long long unsigned int)pstUploadFileSummary->fileSize);    
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "filesize", BAD_CAST str_content); 
    
    //add <lastmodify> under <fileinfo>    
    sprintf_sec(str_content,512,"%llu",(long long unsigned int)pstUploadFileSummary->lastModify);    
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "lastmodify", BAD_CAST str_content);   
    
    //add <fileMd5> under <fileinfo>    
    //snprintf(str_content,512,"%d",pstUploadFileSummary->fileMd5);    
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "md5",BAD_CAST "");  
    
    //add <fileMd5> under <fileinfo>    
    sprintf_sec(str_content,512,"%d",pstUploadFileSummary->fileCheckSum);    
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "checksum",BAD_CAST str_content);   
    
    //add <uploadid> under <fileinfo>    
    sprintf_sec(str_content,512,"%s",pstUploadFileSummary->upload_id);    
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "uploadid",BAD_CAST str_content); 

    // add <bucketname> under <fileinfo>   
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "bucketname",BAD_CAST pstUploadFileSummary->bucket_name); 

    //add <key> under <fileinfo>  
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "key",BAD_CAST pstUploadFileSummary->key); 

    //add <partsinfo> under <uploadinfo>
    xmlAddChild(root_node,node_partsinfo);  


    if((partCount)&&(uploadPartList==NULL))
    {
        xmlFreeDoc(doc);    
        return -1;
    }
    
    ptrUploadPart = uploadPartList;
    for(i=0;i<partCount;i++)         
    {
        xmlNodePtr partNode = NULL;
        sprintf_sec(contentBuff,16,"part%d",i+1);            
        partNode =  xmlNewNode(NULL,BAD_CAST contentBuff);//here  contentBuff indicat the name of the xml node
        
        sprintf_sec(contentBuff,16,"%d",i+1);            
        xmlNewChild(partNode,NULL,BAD_CAST "partNum",BAD_CAST contentBuff);

        if(ptrUploadPart!=NULL)
        {
            sprintf_sec(contentBuff,64,"%s",ptrUploadPart->etag);
            xmlNewChild(partNode,NULL,BAD_CAST "etag",BAD_CAST contentBuff); 

            sprintf_sec(contentBuff,64,"%llu",(long long unsigned int)ptrUploadPart->start_byte);
            xmlNewChild(partNode,NULL,BAD_CAST "startByte",BAD_CAST contentBuff);    

            sprintf_sec(contentBuff,64,"%llu",(long long unsigned int)ptrUploadPart->part_size);
            xmlNewChild(partNode,NULL,BAD_CAST "partSize",BAD_CAST contentBuff); 

            sprintf_sec(contentBuff,64,"%s",g_uploadStatus[ptrUploadPart->uploadStatus]);
            xmlNewChild(partNode,NULL,BAD_CAST "uploadStatus",BAD_CAST contentBuff);  
        }
        xmlAddChild(node_partsinfo,partNode);         
        ptrUploadPart = ptrUploadPart->next;
    }

    
    //store the xml file    
    nRel = xmlSaveFile(file_name,doc);    
    if (nRel != -1)    {         
        COMMLOG(OBS_LOGERROR, "one xml doc is written in %d bytes\n",nRel);
    }
    
    //free all the node int the doc    
    xmlFreeDoc(doc);    
    return 0;
}

int DividUploadPartList(upload_file_part_info * listSrc, upload_file_part_info ** listDone, upload_file_part_info ** listNotDone)
{
    //we assume that the listSrc is sorted in ascending order
    upload_file_part_info *pstSrcListNode = listSrc;
    upload_file_part_info *pstTmpDoneList = NULL;
    upload_file_part_info *pstTmpDoneNode = NULL;
    upload_file_part_info *pstTmpNotDoneList = NULL;
    upload_file_part_info *pstTmpNotDoneNode = NULL;
    upload_file_part_info *pstTmpList = NULL;
    upload_file_part_info *pstTmpNode = NULL;

    while(pstSrcListNode)
    {
        //1. set the node and list to process, store in pstTmpNode and pstTmpList
        if(pstSrcListNode->uploadStatus == UPLOAD_SUCCESS)
        {
            pstTmpList = pstTmpDoneList;
            pstTmpNode = pstTmpDoneNode;
        }
        else
        {
            pstTmpList = pstTmpNotDoneList;
            pstTmpNode = pstTmpNotDoneNode;
        }

        //2, add the node from listSrc after pstTmpNode.
        if(pstTmpList == NULL)
        {
            pstTmpNode = pstSrcListNode;
            pstTmpList = pstSrcListNode;
            pstTmpNode->prev = NULL;
        }
        else
        {
            pstTmpNode->next = pstSrcListNode;
            pstSrcListNode->prev = pstTmpNode;            
            pstTmpNode = pstTmpNode->next;
        }
        //3, set the pstTmpNode and pstTmpList, back to done list or not done list
        if(pstSrcListNode->uploadStatus == UPLOAD_SUCCESS)
        {
            pstTmpDoneList = pstTmpList;
            pstTmpDoneNode = pstTmpNode;
        }
        else
        {
            pstTmpNotDoneList = pstTmpList;
            pstTmpNotDoneNode = pstTmpNode;
        }
        pstSrcListNode = pstSrcListNode->next;
        pstTmpNode->next = NULL;
    }
    *listDone = pstTmpDoneList;
    *listNotDone = pstTmpNotDoneList;
    return 0;
    
}

void addUploadPartNodeToList(upload_file_part_info  **listToAdd, upload_file_part_info *partNode)
{
    upload_file_part_info * pstTempNode = *listToAdd;
    partNode->next = NULL;
    partNode->prev = NULL;

    //need to add before the first node.
    if(pstTempNode == NULL)
    {
        *listToAdd = partNode;
        return;
    }
    if(pstTempNode->part_num > partNode->part_num)
    {
        partNode->next = pstTempNode;
        pstTempNode->prev = partNode;

        *listToAdd = partNode;
        return;
    }
    //need to add at the middle of the list.
    while(pstTempNode)
    {
        if(pstTempNode->part_num > partNode->part_num)
        {
            partNode->next = pstTempNode;

            partNode->prev = pstTempNode->prev;

            pstTempNode->prev->next = partNode;

            pstTempNode->prev = partNode;
            return;
        }
        else
        {
            if(pstTempNode->next != NULL)//avoid moving to the next of the last node
            {
                pstTempNode = pstTempNode->next;
            }
            else
            {
                break;
            }
        }
    }
    // add the nod after the last node
    if((pstTempNode != NULL)&&(pstTempNode->next == NULL))
    {
       pstTempNode->next = partNode;
       partNode->prev = pstTempNode;
    }
}


int updateCheckPoint(char * elementPath, const char * content, const char * file_name)
{
    xmlNodePtr curNode;  
    xmlDocPtr doc;           //the doc pointer to parse the file
    unsigned int i = 0;
    char * strToParse = elementPath;
    char strArry[MAX_XML_DEPTH][32] = {{0}}; 
    unsigned int strNum = 0; 

    char* p=strtok(strToParse, "/");
    while (p != NULL && strNum < MAX_XML_DEPTH){
        strncpy_sec(strArry[strNum], 32, p, strlen(p)+1);
        p = strtok(NULL,"/");
        strNum++;
    }

    curNode = get_xmlnode_from_file(file_name, &doc);
    if (NULL == curNode) 
    {       
        COMMLOG(OBS_LOGERROR, "empty document");       
        return -1;    
    }

    if (xmlStrcmp(curNode->name, BAD_CAST strArry[0]))    
    {   
        COMMLOG(OBS_LOGERROR, "document of the wrong type, root node != strArry[0]");
        xmlFreeDoc(doc);       
        return -1;    
    }

    curNode = curNode->xmlChildrenNode;
    for (i=1; i<strNum; i++)
    {
        while (curNode != NULL)
        {
            if(!xmlStrcmp(curNode->name, BAD_CAST strArry[i]))
            {
                break;
            }
            curNode = curNode->next;
        }

        if(curNode==NULL)
        {
            break;
        }

        if((strNum - 1) > i)
        {
            curNode = curNode->children;
        }
    }

    if((i==strNum) && (curNode!=NULL))
    {
        xmlNodeSetContent(curNode,(const xmlChar *)content);
        xmlSaveFile(file_name,doc); 
    }
    else
    {
        xmlFreeDoc(doc);  
        return -1;
    }

    xmlFreeDoc(doc);  
    return 0;  
}

int GetUploadPartListToProcess(upload_file_part_info **listDone, upload_file_part_info ** listNotDones, 
                               int partCountIn, int * partCountOut,int task_num)
{
    int i = 0;
    int nodeCoutNotDone = 0;
    upload_file_part_info * pstTempNodeNotDone = * listNotDones;
    upload_file_part_info * pstTempNodeNotDoneNext =  * listNotDones;    
    
    for(i=0;i<partCountIn;i++)
    {
        if(pstTempNodeNotDone)
        {
            pstTempNodeNotDoneNext = pstTempNodeNotDone->next;
            addUploadPartNodeToList(listDone, pstTempNodeNotDone);

            pstTempNodeNotDone = pstTempNodeNotDoneNext;
            if(pstTempNodeNotDone)
            {
                pstTempNodeNotDoneNext = pstTempNodeNotDone->next;
            }
        }
    }

    *listNotDones =  pstTempNodeNotDone;
    pstTempNodeNotDone = * listNotDones;

    while(pstTempNodeNotDone)
    {        
        nodeCoutNotDone++;
        pstTempNodeNotDone = pstTempNodeNotDone->next;
    }

    if(nodeCoutNotDone > MAX_THREAD_NUM)
    {
        *partCountOut = MAX_THREAD_NUM;
    }
    else
    {
        *partCountOut = nodeCoutNotDone;
    }

    if(task_num < *partCountOut)
    {
        *partCountOut = task_num;
    }

    return 0;
    
}

static obs_status uploadPartCompletePropertiesCallback
    (const obs_response_properties *properties, void *callback_data)
{
    upload_file_callback_data * cbd =  (upload_file_callback_data *)callback_data;

    if(properties->etag)
    {
        strcpy_s(cbd->stUploadFilePartInfo->etag,MAX_SIZE_ETAG,properties->etag);
    }


    if(cbd->enableCheckPoint)
    {
        char pathToUpdate[1024];

        if(properties->etag)
        {
            sprintf_sec(pathToUpdate,1024,"%s%d/%s","uploadinfo/partsinfo/part",cbd->part_num + 1,"etag");
#if defined(WIN32)
            EnterCriticalSection(&g_csThreadCheckpoint);
#endif

#if defined __GNUC__ || defined LINUX
                     pthread_mutex_lock(&g_mutexThreadCheckpoint);
#endif
            updateCheckPoint(pathToUpdate, properties->etag, cbd->checkpointFilename);
#if defined(WIN32)
            LeaveCriticalSection(&g_csThreadCheckpoint);
#endif

#if defined __GNUC__ || defined LINUX
                     pthread_mutex_unlock(&g_mutexThreadCheckpoint);
#endif
        }
    }


    if(cbd->respHandler->complete_callback)
    {
        (cbd->respHandler->properties_callback)(properties,
                                              cbd->callbackDataIn);
    }
    return OBS_STATUS_OK;
}

static void  uploadPartCompleteCallback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    upload_file_callback_data * cbd =  (upload_file_callback_data *)callback_data;

    if(status == OBS_STATUS_OK)
    {
        cbd->stUploadFilePartInfo->uploadStatus = UPLOAD_SUCCESS;
    }
    else
    {
        cbd->stUploadFilePartInfo->uploadStatus = UPLOAD_FAILED;
    }
    if(cbd->enableCheckPoint)
    {
        char pathToUpdate[1024];
        char contentToSet[32];

        sprintf_sec(pathToUpdate,1024,"%s%d/%s","uploadinfo/partsinfo/part",cbd->part_num + 1,"uploadStatus");
        if(status == OBS_STATUS_OK)
        {
            sprintf_sec(contentToSet,32,"%s","UPLOAD_SUCCESS");
        }
        else
        {
            sprintf_sec(contentToSet,32,"%s","UPLOAD_FAILED");
        }
#if defined(WIN32)
        EnterCriticalSection(&g_csThreadCheckpoint);
#endif

#if defined __GNUC__ || defined LINUX
                     pthread_mutex_lock(&g_mutexThreadCheckpoint);
#endif

        updateCheckPoint(pathToUpdate, contentToSet, cbd->checkpointFilename);
#if defined(WIN32)
        LeaveCriticalSection(&g_csThreadCheckpoint);
#endif

#if defined __GNUC__ || defined LINUX
                     pthread_mutex_unlock(&g_mutexThreadCheckpoint);
#endif
        
    }
    
    if(cbd->respHandler->complete_callback)
    {
        (cbd->respHandler->complete_callback)(status,
                                     error, cbd->callbackDataIn);
    }
    return;
}

static int  uploadPartCallback(int buffer_size, char * buffer, void *callback_data)
{
    upload_file_callback_data * cbd =  (upload_file_callback_data *)callback_data;
    int fdUpload = cbd->fdUploadFile;
    int bytesRead = 0;
    if(fdUpload == -1)
    {
        return -1;
    }
    else
    {        
        if (cbd->bytesRemaining) 
        {
            int toRead = (int)((cbd->bytesRemaining > (unsigned) buffer_size) ?
                          (unsigned) buffer_size : cbd->bytesRemaining);
                          
            bytesRead = read(fdUpload,buffer,toRead);
            cbd->bytesRemaining -= bytesRead;
        }    
    }
    return bytesRead;
    
}

#if defined (WIN32)
unsigned __stdcall UploadThreadProc_win32(void* param)
{   
    upload_file_proc_data * pstPara = (upload_file_proc_data *)param;
    char * uploadFileName = pstPara->stUploadParams->fileNameUpload;
    uint64_t start_byte = pstPara->stUploadFilePartInfo->start_byte;
    uint64_t part_size = pstPara->stUploadFilePartInfo->part_size;
    int part_num = pstPara->stUploadFilePartInfo->part_num;
    server_side_encryption_params * pstEncrypParam;
    char *szUpload = NULL;
    
    int fd = -1;
    uint64_t curPos;  

    (void)_sopen_s( &fd, uploadFileName, _O_RDONLY |_O_BINARY, _SH_DENYWR, 
                    _S_IREAD);

    if(fd == -1)
    {
        COMMLOG(OBS_LOGINFO, "open upload file failed, partnum[%d]\n",part_num);
    }
    else
    {
        obs_upload_handler uploadResponseHandler = 
        { 
            {&uploadPartCompletePropertiesCallback,
            &uploadPartCompleteCallback}, 
            &uploadPartCallback 
        }; 
        upload_file_callback_data  data;
        obs_put_properties stPutProperties;
        obs_name_value metaProperties[OBS_MAX_METADATA_COUNT] = {0}; 

        curPos =  _lseeki64(fd,start_byte,SEEK_SET);
        
        szUpload =   pstPara->stUploadParams->upload_id;
        
        memset(&data,0,sizeof(upload_file_callback_data));

        data.bytesRemaining = pstPara->stUploadFilePartInfo->part_size;
        data.totalBytes = pstPara->stUploadFilePartInfo->part_size;
        data.callbackDataIn = pstPara->callBackData;
        data.checkpointFilename = pstPara->stUploadParams->fileNameCheckpoint;
        data.enableCheckPoint = pstPara->stUploadParams->enable_check_point;
        data.fdUploadFile = fd;
        data.part_num = part_num;
        data.respHandler = pstPara->stUploadParams->response_handler;
        data.taskHandler = 0;
        data.stUploadFilePartInfo = pstPara->stUploadFilePartInfo;

        pstEncrypParam = pstPara->stUploadParams->pstServerSideEncryptionParams;
        if(data.enableCheckPoint == 1)
        {
            char pathToUpdate[2014];
            char contentToSet[64];

            sprintf_sec(pathToUpdate,1024,"%s%u/%s","uploadinfo/partsinfo/part",part_num+1,"uploadStatus");
            sprintf_sec(contentToSet,32,"%s","UPLOADING");
            EnterCriticalSection(&g_csThreadCheckpoint);
            updateCheckPoint(pathToUpdate, contentToSet, pstPara->stUploadParams->fileNameCheckpoint);
            LeaveCriticalSection(&g_csThreadCheckpoint);
        }
        memset(&stPutProperties,0,sizeof(obs_put_properties));

        stPutProperties.expires = -1;
        stPutProperties.canned_acl = OBS_CANNED_ACL_PUBLIC_READ_WRITE;
        stPutProperties.meta_data = metaProperties;
        pstPara->stUploadFilePartInfo->uploadStatus = UPLOADING;
        if((pstEncrypParam)&&(pstEncrypParam->encryption_type == OBS_ENCRYPTION_KMS))
        {
            pstEncrypParam = NULL;
        }
        obs_upload_part_info upload_part_info;
        memset(&upload_part_info,0,sizeof(obs_upload_part_info));
        upload_part_info.part_number = part_num + 1;
        upload_part_info.upload_id = szUpload;
        upload_part(pstPara->stUploadParams->options, pstPara->stUploadParams->objectName, 
                    &upload_part_info, part_size, &stPutProperties, pstEncrypParam,&uploadResponseHandler, &data);
     }
     if(fd != -1)
     {
       _close(fd);
        fd = -1;
     }
     return 1;
}
#endif

#if defined __GNUC__ || defined LINUX
void *UploadThreadProc_linux(void* param)
{
    upload_file_proc_data * pstPara = (upload_file_proc_data *)param;
    char * uploadFileName = pstPara->stUploadParams->fileNameUpload;
    uint64_t start_byte = pstPara->stUploadFilePartInfo->start_byte;
    uint64_t part_size = pstPara->stUploadFilePartInfo->part_size;
    int part_num = pstPara->stUploadFilePartInfo->part_num;
    server_side_encryption_params * pstEncrypParam;
    char *szUpload = NULL;
    
    int fd = -1;
    fd = open(uploadFileName,O_RDONLY);
    if(fd == -1)
    {
        COMMLOG(OBS_LOGINFO, "open upload file failed, partnum[%d]\n",part_num);
        return NULL;
    }
    else
    {
        obs_upload_handler uploadResponseHandler = 
        { 
            {&uploadPartCompletePropertiesCallback,
            &uploadPartCompleteCallback}, 
            &uploadPartCallback 
        }; 
        upload_file_callback_data  data;
        obs_put_properties stPutProperties;
        obs_name_value metaProperties[OBS_MAX_METADATA_COUNT];
        memset_s(metaProperties,sizeof(obs_name_value)*OBS_MAX_METADATA_COUNT,0,sizeof(obs_name_value)*OBS_MAX_METADATA_COUNT); 

        lseek(fd, (long long int)start_byte, SEEK_SET);
        szUpload =   pstPara->stUploadParams->upload_id;
   
        memset_s(&data,sizeof(upload_file_callback_data),0,sizeof(upload_file_callback_data));
        data.bytesRemaining = pstPara->stUploadFilePartInfo->part_size;
        data.totalBytes = pstPara->stUploadFilePartInfo->part_size;
        data.callbackDataIn = pstPara->callBackData;
        data.checkpointFilename = pstPara->stUploadParams->fileNameCheckpoint;
        data.enableCheckPoint = pstPara->stUploadParams->enable_check_point;
        data.fdUploadFile = fd;
        data.part_num = part_num;
        data.respHandler = pstPara->stUploadParams->response_handler;
        data.taskHandler = 0;
        data.stUploadFilePartInfo = pstPara->stUploadFilePartInfo;

        pstEncrypParam = pstPara->stUploadParams->pstServerSideEncryptionParams;
        
        if(data.enableCheckPoint == 1)
        {
            char pathToUpdate[1024];
            char contentToSet[32];

            sprintf_sec(pathToUpdate,1024,"%s%d/%s","uploadinfo/partsinfo/part",part_num,"uploadStatus");
            sprintf_sec(contentToSet,32,"%s","UPLOADING");            
            pthread_mutex_lock(&g_mutexThreadCheckpoint);
            updateCheckPoint(pathToUpdate, contentToSet, pstPara->stUploadParams->fileNameCheckpoint);
            pthread_mutex_unlock(&g_mutexThreadCheckpoint);
            
        }
        memset(&stPutProperties,0,sizeof(obs_put_properties));
    
        stPutProperties.expires = -1;
        stPutProperties.canned_acl = OBS_CANNED_ACL_PUBLIC_READ_WRITE;
        stPutProperties.meta_data = metaProperties;
        pstPara->stUploadFilePartInfo->uploadStatus = UPLOADING;
        if((pstEncrypParam)&&(pstEncrypParam->encryption_type == OBS_ENCRYPTION_KMS))
        {
            pstEncrypParam = NULL;
        }
        obs_upload_part_info upload_part_info;
        memset(&upload_part_info,0,sizeof(obs_upload_part_info));
        upload_part_info.part_number = part_num + 1;
        upload_part_info.upload_id = szUpload;
        upload_part(pstPara->stUploadParams->options, pstPara->stUploadParams->objectName, 
                    &upload_part_info, part_size, &stPutProperties, pstEncrypParam,&uploadResponseHandler, &data);
     }

     if(fd != -1)
     {
          close(fd);
          fd = -1;
     }
     return NULL;
}
#endif

void startUploadThreads(upload_params * pstUploadParams, 
                        upload_file_part_info * uploadFilePartInfoList,  
                        int partCount, void* callback_data)
{

    int i=0;
    upload_file_proc_data * uploadFileProcDataList = (upload_file_proc_data *)malloc(sizeof(upload_file_proc_data)*partCount);
    if (uploadFileProcDataList == NULL)
    {
		COMMLOG(OBS_LOGWARN, "startUploadThreads: uploadFileProcDataList malloc failed!\n");
    }
	
	
	upload_file_proc_data * pstUploadFileProcData = uploadFileProcDataList;

    upload_file_part_info *pstOnePartInfo = uploadFilePartInfoList;
#ifdef WIN32
    HANDLE * arrHandle = (HANDLE *)malloc(sizeof(HANDLE)*partCount);
    unsigned  uiThread2ID;
    DWORD   dwExitCode;
#endif

#if defined __GNUC__ || defined LINUX
    pthread_t * arrThread = (pthread_t *)malloc(sizeof(pthread_t)*partCount);
	if (arrThread == NULL)
	{
		COMMLOG(OBS_LOGWARN, "startUploadThreads: pthread_t malloc failed!\n");
	}
    int err;
#endif
    memset(uploadFileProcDataList,0,sizeof(upload_file_proc_data)*partCount);

    for(i=0;i<partCount;i++)
    {
        pstUploadFileProcData->stUploadParams = pstUploadParams;
        pstUploadFileProcData->stUploadFilePartInfo = pstOnePartInfo;
        pstUploadFileProcData->callBackData = callback_data;

        pstOnePartInfo = pstOnePartInfo->next;
        pstUploadFileProcData ++;
    }
    pstOnePartInfo = uploadFilePartInfoList;    
#ifdef WIN32    
    for(i=0;i<partCount;i++)
    {
        arrHandle[i] = (HANDLE)_beginthreadex(NULL,0,UploadThreadProc_win32, 
                                              &uploadFileProcDataList[i],CREATE_SUSPENDED,&uiThread2ID);
        if(arrHandle[i] ==0){
            GetExitCodeThread( arrHandle[i], &dwExitCode );
            COMMLOG(OBS_LOGERROR, "create thread i[%d] failed exit code = %u \n",i,dwExitCode);
        }
        pstOnePartInfo->threadHandler = arrHandle[i];
        pstOnePartInfo = pstOnePartInfo->next;
    } 
    for(i=0;i<partCount;i++)
    {
        ResumeThread(arrHandle[i]);
    }
    for(i=0;i<partCount;i++)
    {
        WaitForSingleObject(arrHandle[i],INFINITE);
    }

    for(i=0;i<partCount;i++)
    {
        CloseHandle(arrHandle[i]);
    }

    if(arrHandle)
    {
        free(arrHandle);
        arrHandle = NULL;
    }
#endif

#if defined __GNUC__ || defined LINUX
    for(i=0;i<partCount;i++)
    {
        err = pthread_create(&arrThread[i], NULL,UploadThreadProc_linux,(void *)&uploadFileProcDataList[i]);
        if(err != 0)
        {
            COMMLOG(OBS_LOGINFO, "create thread failed i[%d]\n",i);
        }
        
        err = pthread_join(arrThread[i], NULL);
        if(err != 0)
        {
            COMMLOG(OBS_LOGINFO, "join thread failed i[%d]\n",i);
        }
    } 

    if(arrThread)
    {
        free(arrThread);
        arrThread = NULL;
    }
#endif
    if(uploadFileProcDataList)
    {
        free(uploadFileProcDataList);
        uploadFileProcDataList = NULL;
    }
}

int isAllPartsComplete(upload_file_part_info * uploadPartNode,int * isAllSuccess)
{
    upload_file_part_info * ptrUploadPartPrev = uploadPartNode;
    upload_file_part_info * ptrUploadPartNext = uploadPartNode;
    *isAllSuccess = 1;

    if(uploadPartNode == NULL)
    {
        *isAllSuccess = 0;
        return 0;
    }
    while(ptrUploadPartPrev)
    {
        if(ptrUploadPartPrev->uploadStatus != UPLOAD_SUCCESS)
        {
            *isAllSuccess = 0;
        }

        if (   (ptrUploadPartPrev->uploadStatus != UPLOAD_SUCCESS)
           &&(ptrUploadPartPrev->uploadStatus != UPLOAD_FAILED))
           {
               return 0;
           }
        ptrUploadPartPrev = ptrUploadPartPrev->prev;
    }

    while(ptrUploadPartNext)
    {
        if(ptrUploadPartNext->uploadStatus != UPLOAD_SUCCESS)
        {
            *isAllSuccess = 0;
        }

         if (   (ptrUploadPartNext->uploadStatus != UPLOAD_SUCCESS)
           &&(ptrUploadPartNext->uploadStatus != UPLOAD_FAILED))
           {
               return 0;
           }
        ptrUploadPartNext = ptrUploadPartNext->next;
    }
    return 1;
}

obs_status CompleteMultipartUploadCallback_Intern(const char *location,  
                                         const char *bucket, 
                                         const char *key, 
                                         const char *etag, 
                                         void *callback_data) 
{ 
    (void)callback_data; 	
    COMMLOG(OBS_LOGINFO, "location = %s \n bucket = %s \n key = %s \n etag = %s \n",location,bucket,key,etag);
    return OBS_STATUS_OK; 
}

int completeUploadFileParts(upload_file_part_info * pstUploadInfoList,int partCount,
                            const obs_options *options,  char * key, 
                            const char * upload_id, obs_response_handler *handler)
{
    obs_complete_upload_Info * pstUploadInfo;
    obs_complete_upload_Info * upInfoList =  NULL;
    int i = 0;
    
    upload_file_part_info * pstSrcUploadInfo = pstUploadInfoList;
    obs_complete_multi_part_upload_handler response_handler = 
    { 
        {handler->properties_callback, handler->complete_callback}, 
        &CompleteMultipartUploadCallback_Intern 
    };

    obs_put_properties stPutProperties;
    memset(&stPutProperties,0,sizeof(obs_put_properties));
    stPutProperties.expires = -1;
    stPutProperties.canned_acl = OBS_CANNED_ACL_PUBLIC_READ_WRITE;

    upInfoList = (obs_complete_upload_Info *)malloc(sizeof(obs_complete_upload_Info) * partCount);
    if (upInfoList == NULL)
    {
        COMMLOG(OBS_LOGERROR, "in completeUploadFileParts, malloc for upInfoList failed");
        return -1;
    }


    pstUploadInfo = upInfoList;
    for (i=0; i<partCount; i++)
    {
        pstUploadInfo->etag = pstSrcUploadInfo->etag;
        pstUploadInfo->part_number = pstSrcUploadInfo->part_num + 1;
        pstUploadInfo++;
        pstSrcUploadInfo = pstSrcUploadInfo->next;
    }
    //call complete part here
    complete_multi_part_upload(options, key, upload_id, partCount, upInfoList, 
                        &stPutProperties, &response_handler, 0);
    //release the data
    if(upInfoList)
    {
        free(upInfoList);
        upInfoList = NULL;
    }
    
    return 0;
}

int set_isFirstTime(const obs_options *options, char *key, obs_upload_file_configuration *upload_file_config, 
            upload_file_part_info **pstUploadPartList, int *partCount,
            upload_file_summary *pstUploadFileSum)
{
    int isFirtTime = 1;
    int retVal = -1;
    int readCheckPointResult = 0;
    int uploadfileChanged = 0;
    char checkpointFilename[1024] = {0};
    upload_file_summary stUploadFileSummaryOld;
    
    if (!upload_file_config->enable_check_point)
    {
        return isFirtTime;
    }

    if(upload_file_config->check_point_file)
    {
		errno_t err = EOK;
        err = memcpy_s(checkpointFilename,1024,upload_file_config->check_point_file,
                strlen(upload_file_config->check_point_file)+1);
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "set_isFirstTime: memcpy_s failed!");
		}
    }
    
    retVal = setCheckPointFile(upload_file_config->upload_file,checkpointFilename,&isFirtTime,UPLOAD_FILE_INFO);
    if (!upload_file_config->check_point_file)
    {
        upload_file_config->check_point_file = checkpointFilename;
    }
    if(retVal == -1)
    {
        //no need to return here, we can continue but treat enable_check_point as false
        upload_file_config->enable_check_point = 0;  
        return isFirtTime;
    }

    readCheckPointResult = readCheckpointFile(&stUploadFileSummaryOld,pstUploadPartList,partCount,checkpointFilename);             
    if(readCheckPointResult == 0)//read success
    {
        uploadfileChanged=isUploadFileChanged(pstUploadFileSum,&stUploadFileSummaryOld);
        if(uploadfileChanged||checkUploadFileInfo(&stUploadFileSummaryOld,options,key) == 0)
        {
            //here the upload info is not available
            isFirtTime = 1;
            abortMultipartUploadAndFree(options,key,stUploadFileSummaryOld.upload_id,checkpointFilename,CLEAN_FILE);
            if(pstUploadPartList)
            {
                 cleanUploadList(*pstUploadPartList);
                 pstUploadPartList = NULL;
                 *partCount = 0;
            } 
        }
        else
        {
            isFirtTime = 0;
            memcpy_s(pstUploadFileSum,sizeof(upload_file_summary),&stUploadFileSummaryOld,sizeof(upload_file_summary));
        }
    }      

    return isFirtTime;
}

int get_uploadId_for_uploadFile(const obs_options *options, char *key, 
            obs_upload_file_configuration *upload_file_config, 
            char *upload_id, upload_params *pstUploadParams, upload_file_part_info * pstUploadPartList,
            int set_partlist_retVal,
            obs_response_handler *commonHandler,  char *checkpointFilename, int isFirtTime)
{
    if(set_partlist_retVal  == -1)
    {
        COMMLOG(OBS_LOGINFO, "set_partlist_retVal = %d", set_partlist_retVal);
        if(upload_file_config->enable_check_point)
        {
            (void)remove(checkpointFilename);
        }

        if(pstUploadPartList)
        {
            cleanUploadList(pstUploadPartList);
            pstUploadPartList = NULL;
        } 
        return -1;
    }

    if(!(upload_file_config->check_point_file) || !strcmp(upload_file_config->check_point_file, checkpointFilename)){
        upload_file_config->check_point_file = checkpointFilename;
    }

     // init upload task   
    if((isFirtTime == 1)||(NULL == pstUploadParams->upload_id)||(strlen(pstUploadParams->upload_id)==0))
    {
        initiate_multi_part_upload(options,key,MAX_SIZE_UPLOADID, upload_id, 0, 0,commonHandler, 0);
        if(strlen(upload_id)==0) //failed to init multiPart task
        {
            if(upload_file_config->enable_check_point)
            {
                (void)remove(checkpointFilename);
            }

            if(pstUploadPartList)
            {
                cleanUploadList(pstUploadPartList);
                pstUploadPartList = NULL;
            } 
            return -1;
        }
    }
    else
    {
        if(strlen(pstUploadParams->upload_id)!=0)
        {
            strcpy_s(upload_id,MAX_SIZE_UPLOADID,pstUploadParams->upload_id);
        }
    }

    return 0;
}


void upload_complete_handle(const obs_options *options, char *key, obs_upload_file_response_handler *handler,
    upload_file_part_info * pstUploadPartList, int partCount, const char *upload_id, 
    obs_upload_file_configuration *upload_file_config, const char *checkpointFilename,
    void *callback_data)
{
    int isAllSuccess = 0;
    obs_upload_file_part_info * resultInfo;
    int is_true = 0;
    if(isAllPartsComplete(pstUploadPartList,&isAllSuccess) == 1)
    {        
        int retComplete = -1;
        if(isAllSuccess == 1)
        {
            retComplete = completeUploadFileParts(pstUploadPartList,partCount,options, key, upload_id, 
                    &handler->response_handler);
            is_true = ((retComplete == 0) && upload_file_config->enable_check_point);
            if(is_true)
            {
                (void)remove(checkpointFilename);  
            }
            
            if(retComplete == 0)
            {
                if(handler->upload_file_callback)
                {
                    handler->upload_file_callback(OBS_STATUS_OK,"upload file success!\n",0,NULL,callback_data);
                }
            }
            else if(handler->upload_file_callback)
            {
                handler->upload_file_callback(OBS_STATUS_InternalError,
                        "upload part all success, but complete multi part failed!\n",0,NULL,callback_data);
            }
        }
        else
        {
            upload_file_part_info * printNode = pstUploadPartList;
            obs_upload_file_part_info * pstPartInfoRet;
            resultInfo = (obs_upload_file_part_info*)malloc(sizeof(obs_upload_file_part_info)*partCount);
            if(resultInfo==NULL)
            {
                COMMLOG(OBS_LOGERROR, "malloc resultInfo failed\n");
                return ;
            }
            memset_s(resultInfo, sizeof(obs_upload_file_part_info)*partCount, 0 , 
                sizeof(obs_upload_file_part_info)*partCount);
            pstPartInfoRet = resultInfo;
            while(printNode)
            {
                pstPartInfoRet->part_num = printNode->part_num + 1;
                pstPartInfoRet->part_size = printNode->part_size;
                pstPartInfoRet->start_byte = printNode->start_byte;
                pstPartInfoRet->status_return = printNode->uploadStatus;
                printNode = printNode->next;
                pstPartInfoRet ++;
            }

            if(handler->upload_file_callback)
            {
                handler->upload_file_callback(OBS_STATUS_InternalError,
                    "some part success, some parts failed!\n",partCount,resultInfo,callback_data);
            }
                  
            free(resultInfo);
            resultInfo = NULL;       
        }
        
        is_true = (((isAllSuccess == 0) || (retComplete != 0)) 
                    &&(upload_file_config->enable_check_point == 0));
        if (is_true)
        {
            abortMultipartUploadAndFree(options,key, upload_id,NULL,DO_NOTHING);
        }

        return;
    }
    
    upload_file_part_info * printNode = pstUploadPartList;
    if(!upload_file_config->enable_check_point)
    {
        abortMultipartUploadAndFree(options,key, upload_id, NULL, DO_NOTHING);
    }

    obs_upload_file_part_info * pstPartInfoRet;
    resultInfo = (obs_upload_file_part_info*)malloc(sizeof(obs_upload_file_part_info)*partCount);
    if(resultInfo==NULL)
    {
        COMMLOG(OBS_LOGERROR, "malloc resultInfo failed\n");
        return ;
    }
    memset_s(resultInfo, sizeof(obs_upload_file_part_info)*partCount, 0 , 
        sizeof(obs_upload_file_part_info)*partCount);
    pstPartInfoRet = resultInfo;

    while(printNode)
    {
        COMMLOG(OBS_LOGERROR,"part_num[%d], status[%s]\n",printNode->part_num,
                g_uploadStatus[printNode->uploadStatus]);

        pstPartInfoRet->part_num = printNode->part_num + 1;
        pstPartInfoRet->part_size = printNode->part_size;
        pstPartInfoRet->start_byte = printNode->start_byte;
        pstPartInfoRet->status_return = printNode->uploadStatus;
        printNode = printNode->next;
        pstPartInfoRet ++;
    }

    if(handler->upload_file_callback)
    {
        handler->upload_file_callback(OBS_STATUS_InternalError,
            "some part success, some parts failed!\n",partCount,resultInfo,callback_data);
    }
    free(resultInfo);
    resultInfo = NULL;    

    return ;
}

void upload_file(const obs_options *options, char *key, server_side_encryption_params *encryption_params, 
                 obs_upload_file_configuration *upload_file_config, obs_upload_file_response_handler *handler, void *callback_data)
{
    int isFirtTime = 1;
    int retVal = -1;
    upload_file_part_info * pstUploadPartList = NULL;
    upload_file_summary stUploadFileSum;
    upload_file_part_info * pstUploadPartListDone = NULL;
    upload_file_part_info * pstUploadPartListNotDone = NULL;
    int partCount = 0;
    int partCountToProc = 0;
    char checkpointFilename[1024] = {0};
    char upload_id[MAX_SIZE_UPLOADID];
    upload_params stUploadParams;
    int is_ture = 0;
    uint64_t uploadPartSize = 0;

    memset_s(&stUploadParams,sizeof(upload_params),0,sizeof(upload_params));
    memset_s(&stUploadFileSum,sizeof(upload_file_summary),0,sizeof(upload_file_summary));

    if(upload_file_config->check_point_file)
    {
        memcpy_s(checkpointFilename,1024,upload_file_config->check_point_file,
                    strlen(upload_file_config->check_point_file)+1);
    }
    //get the summary of the upload file 
    retVal = getUploadFileSummary(&stUploadFileSum,upload_file_config->upload_file);
    if((retVal == -1) || (stUploadFileSum.fileSize == 0))
    {
        COMMLOG(OBS_LOGERROR, "the upload file is not exist or it's size is 0\n");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidPart, 0, callback_data);
        return;
    }
    
    //set the check point file
    isFirtTime = set_isFirstTime(options, key, upload_file_config, 
                                   &pstUploadPartList, &partCount, &stUploadFileSum);

    if(upload_file_config->check_point_file)
    {
        memcpy_s(checkpointFilename,1024,upload_file_config->check_point_file,
            strlen(upload_file_config->check_point_file)+1);
    }
    is_ture = ((upload_file_config->part_size == 0)
                            ||(upload_file_config->part_size > MAX_PART_SIZE));
    uploadPartSize = is_ture ? MAX_PART_SIZE : upload_file_config->part_size;  
    uploadPartSize = (uploadPartSize> stUploadFileSum.fileSize) ? stUploadFileSum.fileSize : uploadPartSize;

    //set the part list to upload
    retVal = setPartList(&stUploadFileSum, uploadPartSize,&pstUploadPartList,&partCount,isFirtTime);
    stUploadParams.upload_id = stUploadFileSum.upload_id;

    retVal = get_uploadId_for_uploadFile(options,key, upload_file_config, upload_id, &stUploadParams, 
                 pstUploadPartList, retVal, &(handler->response_handler), checkpointFilename, isFirtTime);
    if(-1 == retVal)
    {
        return;
    }
    memcpy_s(stUploadFileSum.bucket_name,MAX_BKTNAME_SIZE,options->bucket_options.bucket_name,strlen(options->bucket_options.bucket_name)+1);
    memcpy_s(stUploadFileSum.key,MAX_KEY_SIZE,key,strlen(key)+1);
    memcpy_s(stUploadFileSum.upload_id,MAX_SIZE_UPLOADID,upload_id,strlen(upload_id)+1);
    is_ture = ((upload_file_config->enable_check_point == 1) && (isFirtTime == 1));
    if(is_ture)
    {
        writeCheckpointFile(&stUploadFileSum,pstUploadPartList,partCount,checkpointFilename);
    }
    stUploadParams.fileNameCheckpoint = checkpointFilename;
    stUploadParams.enable_check_point = upload_file_config->enable_check_point;
    stUploadParams.callBackData = callback_data;
    stUploadParams.fileNameUpload = upload_file_config->upload_file;
    stUploadParams.objectName = key;
    stUploadParams.options = options;
    stUploadParams.pstServerSideEncryptionParams = encryption_params;
    stUploadParams.response_handler = &(handler->response_handler);
    stUploadParams.upload_id = upload_id;

    (void)DividUploadPartList(pstUploadPartList,&pstUploadPartListDone,&pstUploadPartListNotDone);

    //start upload part threads now
    partCountToProc = 0;
    upload_file_config->task_num = (upload_file_config->task_num == 0)?MAX_THREAD_NUM:upload_file_config->task_num;
    while(pstUploadPartListNotDone)
    {
#if defined (WIN32)
        Sleep(1000);
#else
        sleep(1);
#endif
         (void)GetUploadPartListToProcess(&pstUploadPartListDone,&pstUploadPartListNotDone,
            partCountToProc,&partCountToProc,upload_file_config->task_num);

         startUploadThreads(&stUploadParams,pstUploadPartListNotDone,partCountToProc,callback_data);
    }
    pstUploadPartList = pstUploadPartListDone;
    upload_complete_handle(options, key, handler, pstUploadPartList, partCount, upload_id, 
                    upload_file_config, checkpointFilename, callback_data);
    
    if(pstUploadPartList)
    {
        cleanUploadList(pstUploadPartList);
        pstUploadPartList = NULL;
    } 
    
    return;    
}

obs_storage_class getStorageClassEnum(const char * storage_class_value)
{
    if(!strcmp(storage_class_value,"STANDARD"))
    {
        return OBS_STORAGE_CLASS_STANDARD;
    }
    else if(!strcmp(storage_class_value,"STANDARD_IA"))
    {
        return OBS_STORAGE_CLASS_STANDARD_IA;
    }
    else if(!strcmp(storage_class_value,"GLACIER"))
    {
        return OBS_STORAGE_CLASS_GLACIER;
    }
    else
    {
       return OBS_STORAGE_CLASS_STANDARD;
    }
}

static obs_status GetObjectMetadataPropertiesCallback_Intern
                  (const obs_response_properties *properties, void *callback_data)
{
    get_object_metadata_callback_data * cb = (get_object_metadata_callback_data *)callback_data;
    download_file_summary * pstFileInfo = cb->pstFileInfo;

    pstFileInfo->objectLength = properties->content_length;
    pstFileInfo->lastModify = properties->last_modified;
    if(properties->etag)
    {
		errno_t err = EOK;  
		err = memcpy_s(pstFileInfo->etag,MAX_SIZE_ETAG,properties->etag,strlen(properties->etag));
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "GetObjectMetadataPropertiesCallback_Intern: memcpy_s failed!\n");
		}
    }

    if(properties->storage_class)
    {
        pstFileInfo->storage_class = getStorageClassEnum(properties->storage_class);
    }

    return OBS_STATUS_OK;    
}

static void GetObjectMetadataCompleteCallback_Intern(obs_status status,
                                                     const obs_error_details *error, 
                                                     void *callback_data)
{
    get_object_metadata_callback_data * cb = (get_object_metadata_callback_data *)callback_data;
    (void)error;
    cb->retStatus = status;
}

obs_status getObjectInfo(download_file_summary * downloadFileInfo,
                  const obs_options *options, char *key, char* version_id,
                  server_side_encryption_params *encryption_params)
{
    get_object_metadata_callback_data stGetObjectMetadataCallBackData;

    obs_response_handler getObjMetadataHandler  = { 
       &GetObjectMetadataPropertiesCallback_Intern, &GetObjectMetadataCompleteCallback_Intern 
    }; 

    memset_s(&stGetObjectMetadataCallBackData,sizeof(get_object_metadata_callback_data),
        0,sizeof(get_object_metadata_callback_data));

    stGetObjectMetadataCallBackData.pstFileInfo = downloadFileInfo;   

    obs_object_info object_info;
    memset_s(&object_info,sizeof(obs_object_info),0,sizeof(obs_object_info));
    object_info.key = key;
    object_info.version_id = version_id;
    get_object_metadata(options, &object_info, encryption_params,
        &getObjMetadataHandler,&stGetObjectMetadataCallBackData);

    return stGetObjectMetadataCallBackData.retStatus;   
}

obs_status restoreGlacierObject(const obs_options *options, char * key, char * version_id)
{
    
    obs_tier tier = OBS_TIER_EXPEDITED;
    char *days = "1";
    lisPartResult retResult;
    
    const obs_response_handler handler =
    {
        &ListPartsPropertiesCallback_Intern,&ListPartsCompleteCallback_Intern,
    };
    retResult.retStatus = OBS_STATUS_OK;
    obs_object_info  object_info;
    memset_s(&object_info,sizeof(obs_object_info),0,sizeof(obs_object_info));
    object_info.key = key;
    object_info.version_id = version_id;
    restore_object(options,&object_info,days, tier,&handler, &retResult);
    return retResult.retStatus;
}

download_status GetDownloadStatusEnum(const char * strStatus)
{
    if(!strcmp(strStatus,"DOWNLOAD_NOTSTART"))
    {
        return DOWNLOAD_NOTSTART;
    }
    else if(!strcmp(strStatus,"DOWNLOADING"))
    {
        return DOWNLOADING;
    }
    else if(!strcmp(strStatus,"DOWNLOAD_FAILED"))
    {
        return DOWNLOAD_FAILED;
    }
    else if(!strcmp(strStatus,"DOWNLOAD_SUCCESS"))
    {
        return DOWNLOAD_SUCCESS;
    }
    else if(!strcmp(strStatus,"COMBINE_SUCCESS"))
    {
        return COMBINE_SUCCESS;
    }
    else
    {
        return DOWNLOAD_NOTSTART;
    }
}

void cleanDownloadList(download_file_part_info * downloadPartListinfo)
{
    download_file_part_info * ptrDownloadPart = downloadPartListinfo;
    download_file_part_info * ptrDownloadPartNext = downloadPartListinfo;
    while(ptrDownloadPart)
    {
        ptrDownloadPartNext = ptrDownloadPart->next;

        free(ptrDownloadPart);
        ptrDownloadPart = NULL;

        ptrDownloadPart = ptrDownloadPartNext;
    }
}


void parse_download_xmlnode_objectinfo(xmlNodePtr curNode, 
                download_file_summary * pstDownLoadSummary)
{        
    xmlNodePtr objectinfoNode = curNode->xmlChildrenNode; 
    while(objectinfoNode != NULL)          
    {
        xmlChar *nodeContent = xmlNodeGetContent(objectinfoNode);  
		errno_t err = EOK; 
		
		if(!xmlStrcmp(objectinfoNode->name,(xmlChar *)"ContentLength"))
        {
            pstDownLoadSummary->objectLength = parseUnsignedInt((char*)nodeContent);
        }
        else if(!xmlStrcmp((xmlChar *)objectinfoNode->name,(xmlChar *)"lastmodify"))
        {
            pstDownLoadSummary->lastModify = parseUnsignedInt((char*)nodeContent);
        }
        else if(!xmlStrcmp(objectinfoNode->name,(xmlChar *)"etag"))
        {
            err = memcpy_s(pstDownLoadSummary->etag,MAX_SIZE_ETAG,(char*)nodeContent,strlen((char*)nodeContent)+1);
        }
        else if(!xmlStrcmp(objectinfoNode->name,(xmlChar *)"storageclass"))
        {
            pstDownLoadSummary->storage_class = getStorageClassEnum((char*)nodeContent);
        }
        else if(!xmlStrcmp(objectinfoNode->name,(xmlChar *)"bucketname"))
        {
            err = memcpy_s(pstDownLoadSummary->bucket_name,MAX_BKTNAME_SIZE,nodeContent,strlen((char*)nodeContent)+1);
        }
        else if(!xmlStrcmp(objectinfoNode->name,(xmlChar *)"key"))
        {
            err = memcpy_s(pstDownLoadSummary->key,MAX_KEY_SIZE,nodeContent,strlen((char*)nodeContent)+1);
        }

		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "parse_download_xmlnode_objectinfo: memcpy_s failed!\n");
		}
		
        xmlFree(nodeContent);             
        objectinfoNode = objectinfoNode->next;          
    }       
    
    return;
}

int parse_download_xmlnode_partsinfo(xmlNodePtr curNode, 
                download_file_part_info **downloadPartList,
                int *partCount)
{        
    download_file_part_info * downloadPartNode;
    download_file_part_info * pstDownloadPart = NULL;
    int partCountTmp = 0;
    
    xmlNodePtr partNode = curNode->xmlChildrenNode; 
    xmlNodePtr partinfoNode = NULL;
    while(partNode)
    {
        if(strncmp((char*)partNode->name,"part",strlen("part")))
        {
            partNode = partNode->next;
            continue;
        }

        downloadPartNode = (download_file_part_info *)malloc(sizeof(download_file_part_info));
        if(downloadPartNode == NULL)
        {
            COMMLOG(OBS_LOGERROR, "int readCheckpointFile_Download, malloc for uploadPartNode failed");
            cleanDownloadList(*downloadPartList);
            partCountTmp = 0;
            *partCount = 0;
            return -1;
        }
        downloadPartNode->next = NULL;
        partCountTmp ++;
   
        partinfoNode = partNode->xmlChildrenNode;
        while(partinfoNode != NULL)          
        {
            xmlChar *nodeContent = xmlNodeGetContent(partinfoNode);             
            COMMLOG(OBS_LOGINFO,"name:%s content %s\n",partinfoNode->name,nodeContent);
            //get parts info and store in uploadPartList
            if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"partNum"))
            {
                downloadPartNode->part_num = (int)parseUnsignedInt((char*)nodeContent) - 1;
            }
            else if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"etag"))
            {
                memset(downloadPartNode->etag,0,MAX_SIZE_ETAG);
                
				errno_t err = EOK;  
				err = memcpy_s(downloadPartNode->etag, MAX_SIZE_ETAG, nodeContent, strlen((char*)nodeContent)+1);
				if (err != EOK)
				{
					COMMLOG(OBS_LOGWARN, "parse_download_xmlnode_partsinfo: memcpy_s failed!\n");
					return -1;
				}
            }
            else if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"startByte"))
            {
                downloadPartNode->start_byte = parseUnsignedInt((char*)nodeContent);
            }
            else if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"partSize"))
            {
                downloadPartNode->part_size = parseUnsignedInt((char*)nodeContent);
            }
            else if(!xmlStrcmp(partinfoNode->name,(xmlChar *)"downloadStatus"))
            {
                downloadPartNode->downloadStatus = GetDownloadStatusEnum((char*)nodeContent);
            }                       

            xmlFree(nodeContent); 
            partinfoNode = partinfoNode->next;                       
        } 
        
        downloadPartNode->prev = pstDownloadPart;
        if(pstDownloadPart == NULL)
        {
            pstDownloadPart = downloadPartNode;
            *downloadPartList  = downloadPartNode;
        }
        else
        {
            pstDownloadPart->next = downloadPartNode;
            pstDownloadPart = pstDownloadPart->next;
        }              

        partNode = partNode->next;
    }
    
    *partCount = partCountTmp;   
    return 0;
}


int readCheckpointFile_Download(download_file_summary * pstDownLoadSummary,
                                download_file_part_info **downloadPartList, 
                                int *partCount, char *file_name)
{
    xmlNodePtr curNode;  
    xmlDocPtr doc;           //the doc pointer to parse the file
    int ret_stat = 0;
       
    if(check_file_is_valid(file_name) == -1)
    {
       return -1;
    }

    curNode = get_xmlnode_from_file(file_name, &doc);
    if (NULL == curNode) 
    {           
       return -1;    
    }

    if (xmlStrcmp((xmlChar *)curNode->name, BAD_CAST "downloadinfo"))    
    {   
        COMMLOG(OBS_LOGERROR, "document of the wrong type, root node != downloadinfo");
        xmlFreeDoc(doc);       
        return -1;    
    }
    curNode = curNode->xmlChildrenNode;
    while(curNode != NULL)    
    {                   
        if(!xmlStrcmp(curNode->name,(xmlChar *)"objectinfo"))       
        {
            parse_download_xmlnode_objectinfo(curNode, pstDownLoadSummary);
        }  
          
        if(!xmlStrcmp(curNode->name,(xmlChar *)"partsinfo"))       
        { 
            ret_stat = parse_download_xmlnode_partsinfo(curNode, downloadPartList,
                        partCount);
            if(-1 == ret_stat)
            {
                break;
            }
        }
        
        curNode = curNode->next;    
    }

    xmlFreeDoc(doc);  
    return ret_stat;
}

void removeTempFiles(const char * fileName, download_file_part_info * downloadPartList, int removeAll)
{
    download_file_part_info * partNode = downloadPartList;
    char fileNameTemp[1024] = {0};
    if((fileName == NULL) || (downloadPartList == NULL))
    {
        return;
    } 

     if(removeAll)
    {
        remove(fileName);
    }

    while(partNode)
    {
#if defined WIN32
        Sleep(0);
#endif

#if defined __GNUC__ || defined LINUX
        sleep(0);
#endif
            if(partNode->downloadStatus != COMBINE_SUCCESS)
            {
                sprintf_sec(fileNameTemp,1024,"%s.%d",fileName,partNode->part_num);
                remove(fileNameTemp);
            }
        partNode = partNode->next;
    } 
}

int setDownloadpartList(download_file_summary *pstDownLoadFileSummaryNew, uint64_t downloadPartsize,
                        download_file_part_info ** downloadPartList, int *partCount)
{   
    int partCountTemp = 0;
    download_file_part_info * downloadPartListTemp = NULL;
    download_file_part_info * pstdownloadPartListTemp =NULL;
    download_file_part_info * pstDownloadPartPrev = NULL;
    uint64_t lastPartSize  = 0;
    int i = 0;
    COMMLOG(OBS_LOGERROR, "download  pstDownLoadFileSummaryNew->objectLength = %d  ;downloadPartsize = %d",
        pstDownLoadFileSummaryNew->objectLength ,downloadPartsize);
    partCountTemp = (int)(pstDownLoadFileSummaryNew->objectLength / downloadPartsize);
    lastPartSize = pstDownLoadFileSummaryNew->objectLength % downloadPartsize;
    *partCount = partCountTemp;
    for(i=0;i<partCountTemp;i++)
    {
        pstdownloadPartListTemp = (download_file_part_info*)malloc(sizeof(download_file_part_info));
        if(pstdownloadPartListTemp == NULL)
        {
            COMMLOG(OBS_LOGERROR, "in %s failed to malloc for uploadPartListTemp !", __FUNCTION__);
            cleanDownloadList(pstdownloadPartListTemp);
            pstdownloadPartListTemp = NULL;
            return -1; 
        }
        pstdownloadPartListTemp->next = NULL;
        pstdownloadPartListTemp->part_num = i;
        pstdownloadPartListTemp->start_byte = downloadPartsize*i;
        pstdownloadPartListTemp->part_size = downloadPartsize;
        pstdownloadPartListTemp->downloadStatus = DOWNLOAD_NOTSTART;
        memset_s(pstdownloadPartListTemp->etag,MAX_SIZE_ETAG,0,MAX_SIZE_ETAG);
        if(i==0)
        {
            pstdownloadPartListTemp->prev = NULL;
            pstDownloadPartPrev = NULL;
            downloadPartListTemp = pstdownloadPartListTemp;
        }
        else
        {
            pstdownloadPartListTemp->prev = pstDownloadPartPrev;
            pstDownloadPartPrev->next = pstdownloadPartListTemp;
        }

        pstDownloadPartPrev = pstdownloadPartListTemp;
    }
    if(lastPartSize != 0)
    {
        pstdownloadPartListTemp = (download_file_part_info*)malloc(sizeof(download_file_part_info));
        if(pstdownloadPartListTemp == NULL)
        {
            COMMLOG(OBS_LOGERROR, "in %s failed to malloc for uploadPartListTemp !", __FUNCTION__);
            cleanDownloadList(downloadPartListTemp);
            downloadPartListTemp = NULL;
            return -1; 
        }
        pstdownloadPartListTemp->prev = pstDownloadPartPrev;

        if(pstDownloadPartPrev)
        {
            pstDownloadPartPrev->next = pstdownloadPartListTemp;
        }
        COMMLOG(OBS_LOGERROR, "download 4");

        pstdownloadPartListTemp->part_num = i;
        pstdownloadPartListTemp->start_byte = downloadPartsize*i;
        pstdownloadPartListTemp->part_size = lastPartSize;
        pstdownloadPartListTemp->downloadStatus = DOWNLOAD_NOTSTART;
        pstdownloadPartListTemp->next = NULL;
        memset_s(pstdownloadPartListTemp->etag,MAX_SIZE_ETAG,0,MAX_SIZE_ETAG);
        *partCount = partCountTemp + 1;
    }
    else
    {
        pstdownloadPartListTemp = NULL;
    }
    COMMLOG(OBS_LOGERROR, "download 5");
    *downloadPartList = downloadPartListTemp;
    return 0;
}

int writeCheckpointFile_Download(download_file_summary * pstDownloadFileSummary,
                                 download_file_part_info * downloadPartList, int partCount,char * file_name)
{
    //new doc    
    char str_content[512] = {0};    
    xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");	
    char contentBuff[64];
    int i=0;  
    int nRel = -1;

    download_file_part_info * ptrDownloadPart;

    xmlNodePtr root_node = xmlNewNode(NULL,BAD_CAST"downloadinfo");   

    xmlNodePtr node_objectinfo = xmlNewNode(NULL,BAD_CAST"objectinfo");    
    xmlNodePtr node_partsinfo = xmlNewNode(NULL,BAD_CAST"partsinfo"); 
    //set the root node <uploadinfo>    
    xmlDocSetRootElement(doc,root_node);    
    //add <object_info> and <partsinfo> under <downloadinfo>        
    
    //add <object_info> node under <downloadinfo>    
    xmlAddChild(root_node,node_objectinfo);  

    
    //add <filesize> under <object_info>    
    sprintf_sec(str_content,512,"%llu",(long long unsigned int)pstDownloadFileSummary->objectLength);    
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "ContentLength", BAD_CAST str_content); 
    
    //add <lastmodify> under <object_info>    
    sprintf_sec(str_content,512,"%llu",(long long unsigned int)pstDownloadFileSummary->lastModify);    
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "lastmodify", BAD_CAST str_content);   
    
    //add <etag> under <object_info>    
    //snprintf(str_content,512,"%d",pstUploadFileSummary->fileMd5);  
    //sprintf_sec(str_content,512,"%s",pstDownloadFileSummary->etag); 
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "etag",BAD_CAST pstDownloadFileSummary->etag);  
    
    //add <storageclass> under <object_info>    
    sprintf_sec(str_content,512,"%s",g_storageClass[pstDownloadFileSummary->storage_class]); 
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "storageclass",BAD_CAST str_content);   

    // add <bucketname> under <object_info>   
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "bucketname",BAD_CAST pstDownloadFileSummary->bucket_name); 

    //add <key> under <object_info>  
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "key",BAD_CAST pstDownloadFileSummary->key); 

    //add <partsinfo> under <downloadinfo>
    xmlAddChild(root_node,node_partsinfo);  

    if((partCount)&&(downloadPartList==NULL))
    {
        xmlFreeDoc(doc);    
        return -1;
    }
    ptrDownloadPart = downloadPartList;
    for(i=0;i<partCount;i++)         
    {
        xmlNodePtr partNode = NULL;
        sprintf_sec(contentBuff,16,"part%d",i+1);            
        partNode =  xmlNewNode(NULL,BAD_CAST contentBuff);//here  contentBuff indicat the name of the xml node
        
        sprintf_sec(contentBuff,16,"%d",i+1);            
        xmlNewChild(partNode,NULL,BAD_CAST "partNum",BAD_CAST contentBuff);

        if(ptrDownloadPart!=NULL)
        {
            sprintf_sec(contentBuff,64,"%s",ptrDownloadPart->etag);
            xmlNewChild(partNode,NULL,BAD_CAST "etag",BAD_CAST contentBuff); 

            sprintf_sec(contentBuff,64,"%llu",(long long unsigned int)ptrDownloadPart->start_byte);
            xmlNewChild(partNode,NULL,BAD_CAST "startByte",BAD_CAST contentBuff);    

            sprintf_sec(contentBuff,64,"%llu",(long long unsigned int)ptrDownloadPart->part_size);
            xmlNewChild(partNode,NULL,BAD_CAST "partSize",BAD_CAST contentBuff); 

            sprintf_sec(contentBuff,64,"%s",g_downloadStatus[ptrDownloadPart->downloadStatus]);
            xmlNewChild(partNode,NULL,BAD_CAST "downloadStatus",BAD_CAST contentBuff);  
        }
        xmlAddChild(node_partsinfo,partNode);         
        ptrDownloadPart = ptrDownloadPart->next;
    }

    
    //store the xml file    
    nRel = xmlSaveFile(file_name,doc);    
    if (nRel != -1)    {         
    COMMLOG(OBS_LOGINFO, "%s file[%s] is not exist","readCheckpointFile",file_name);
    }    
    //free all the node int the doc    
    xmlFreeDoc(doc);    
    return 0;
}

int DividDownloadPartList(download_file_part_info * listSrc, download_file_part_info ** listDone, 
                          download_file_part_info ** listNotDone)
{
    //we assume that the listSrc is sorted in ascending order
    download_file_part_info *pstSrcListNode = listSrc;
    download_file_part_info *pstTmpDoneList = NULL;
    download_file_part_info *pstTmpDoneNode = NULL;

    download_file_part_info *pstTmpNotDoneList = NULL;
    download_file_part_info *pstTmpNotDoneNode = NULL;

    download_file_part_info *pstTmpList = NULL;
    download_file_part_info *pstTmpNode = NULL;

    while(pstSrcListNode)
    {
        //1. set the node and list to process, store in pstTmpNode and pstTmpList
        if((pstSrcListNode->downloadStatus == DOWNLOAD_SUCCESS) || (pstSrcListNode->downloadStatus == COMBINE_SUCCESS))
        {
            pstTmpList = pstTmpDoneList;
            pstTmpNode = pstTmpDoneNode;
        }
        else
        {
            pstTmpList = pstTmpNotDoneList;
            pstTmpNode = pstTmpNotDoneNode;
        }
        //2, add the node from listSrc after pstTmpNode.
        if(pstTmpList == NULL)
        {
            pstTmpNode = pstSrcListNode;
            pstTmpList = pstSrcListNode;
            pstTmpNode->prev = NULL;
        }
        else
        {
            pstTmpNode->next = pstSrcListNode;
            pstSrcListNode->prev = pstTmpNode;            
            pstTmpNode = pstTmpNode->next;
        }

        //3, set the pstTmpNode and pstTmpList, back to done list or not done list
        if((pstSrcListNode->downloadStatus == DOWNLOAD_SUCCESS) 
            || (pstSrcListNode->downloadStatus == COMBINE_SUCCESS))
        {
            pstTmpDoneList = pstTmpList;
            pstTmpDoneNode = pstTmpNode;
        }
        else
        {
            pstTmpNotDoneList = pstTmpList;
            pstTmpNotDoneNode = pstTmpNode;
        }
        
        
        pstSrcListNode = pstSrcListNode->next;
        pstTmpNode->next = NULL;
    }


    *listDone = pstTmpDoneList;
    *listNotDone = pstTmpNotDoneList;
    return 0;
    
}

void addDownloadPartNodeToList(download_file_part_info **listToAdd, download_file_part_info *partNode)
{
    download_file_part_info * pstTempNode = *listToAdd;
    partNode->next = NULL;
    partNode->prev = NULL;

    //need to add before the first node.
    if(pstTempNode == NULL)
    {
        *listToAdd = partNode;
        return;
    }
    if(pstTempNode->part_num > partNode->part_num)
    {
        partNode->next = pstTempNode;
        pstTempNode->prev = partNode;

        *listToAdd = partNode;
        return;
        
    }

    //need to add at the middle of the list.
    while(pstTempNode)
    {
        if(pstTempNode->part_num > partNode->part_num)
        {
            partNode->next = pstTempNode;
            partNode->prev = pstTempNode->prev;
            pstTempNode->prev->next = partNode;
            pstTempNode->prev = partNode;
            return;
        }
        else
        {
            if(pstTempNode->next != NULL)
            {
                pstTempNode = pstTempNode->next;
            }
            else
            {
                break;
            }
        }
    }
    if((pstTempNode != NULL)&&(pstTempNode->next == NULL))
    {
       pstTempNode->next = partNode;
       partNode->prev = pstTempNode;
    }
}


int GetDownloadPartListToProcess(download_file_part_info **listDone, download_file_part_info ** listNotDones, 
                                 int partCountIn, int * partCountOut,int task_num)
{
    int i = 0;
    int nodeCoutNotDone = 0;
    download_file_part_info * pstTempNodeNotDone = * listNotDones;
    download_file_part_info * pstTempNodeNotDoneNext =  * listNotDones;    
    
    for(i=0;i<partCountIn;i++)
    {
        if(pstTempNodeNotDone)
        {
            pstTempNodeNotDoneNext = pstTempNodeNotDone->next;
            addDownloadPartNodeToList(listDone, pstTempNodeNotDone);

            pstTempNodeNotDone = pstTempNodeNotDoneNext;
            if(pstTempNodeNotDone)
            {
                pstTempNodeNotDoneNext = pstTempNodeNotDone->next;
            }
        }
    }
    *listNotDones =  pstTempNodeNotDone;
    pstTempNodeNotDone = * listNotDones;
    while(pstTempNodeNotDone)
    {        
        nodeCoutNotDone++;
        pstTempNodeNotDone = pstTempNodeNotDone->next;
    }
    if(nodeCoutNotDone > MAX_THREAD_NUM)
    {
        *partCountOut = MAX_THREAD_NUM;
    }
    else
    {
        *partCountOut = nodeCoutNotDone;
    }
    if(task_num < *partCountOut)
    {
        *partCountOut = task_num;
    }
    return 0;    
}


static obs_status DownloadPartCompletePropertiesCallback
    (const obs_response_properties *properties, void *callback_data)
{
    download_file_callback_data * cbd =  (download_file_callback_data *)callback_data;

    if(properties->etag)
    {
        strcpy_s(cbd->pstDownloadFilePartInfo->etag,MAX_SIZE_ETAG,properties->etag);
    }
    if(cbd->enableCheckPoint)
    {
        char pathToUpdate[1024];
        if(properties->etag)
        {
            sprintf_sec(pathToUpdate,1024,"%s%d/%s","downloadinfo/partsinfo/part",
                cbd->pstDownloadFilePartInfo->part_num + 1,"etag");
#if defined(WIN32)
            EnterCriticalSection((CRITICAL_SECTION *)cbd->xmlWriteMutex);
#endif

#if defined __GNUC__ || defined LINUX
            pthread_mutex_lock((pthread_mutex_t *)cbd->xmlWriteMutex);
#endif
            updateCheckPoint(pathToUpdate, properties->etag, cbd->checkpointFilename);
#if defined(WIN32)
            LeaveCriticalSection((CRITICAL_SECTION *)cbd->xmlWriteMutex);
#endif

#if defined __GNUC__ || defined LINUX
            pthread_mutex_unlock((pthread_mutex_t *)cbd->xmlWriteMutex);
#endif
        }
    }
    if(cbd->respHandler->complete_callback)
    {
        (cbd->respHandler->properties_callback)(properties,
                                    cbd->callbackDataIn);
    }
    return OBS_STATUS_OK;
}

static void  downloadPartCompleteCallback(obs_status status,
                                          const obs_error_details *error, 
                                          void *callback_data)
{
    download_file_callback_data * cbd =  (download_file_callback_data *)callback_data;

    if(status == OBS_STATUS_OK)
    {        
        cbd->pstDownloadFilePartInfo->downloadStatus = DOWNLOAD_SUCCESS;
    }
    else
    {        
        cbd->pstDownloadFilePartInfo->downloadStatus = DOWNLOAD_FAILED;
    }

    if(cbd->enableCheckPoint)
    {
        char pathToUpdate[1024];
        char contentToSet[32];

        sprintf_sec(pathToUpdate,1024,"%s%d/%s","downloadinfo/partsinfo/part",
            cbd->pstDownloadFilePartInfo->part_num + 1,"downloadStatus");
        if(status == OBS_STATUS_OK)
        {
            sprintf_sec(contentToSet,32,"%s","DOWNLOAD_SUCCESS");
        }
        else
        {
            sprintf_sec(contentToSet,32,"%s","DOWNLOAD_FAILED");
        }

        //must ensure do close tempfile before updateCheckPoint
        if (cbd->fdStorefile != -1)
        {
            close(cbd->fdStorefile);
            cbd->fdStorefile = -1;
        }
        
#if defined(WIN32)
        EnterCriticalSection((CRITICAL_SECTION *)cbd->xmlWriteMutex);
#endif

#if defined __GNUC__ || defined LINUX
        pthread_mutex_lock((pthread_mutex_t *)cbd->xmlWriteMutex);
#endif

        updateCheckPoint(pathToUpdate, contentToSet, cbd->checkpointFilename);
#if defined(WIN32)
        LeaveCriticalSection((CRITICAL_SECTION *)cbd->xmlWriteMutex);
#endif

#if defined __GNUC__ || defined LINUX
        pthread_mutex_unlock((pthread_mutex_t *)cbd->xmlWriteMutex);
#endif
        
    }
    
    if(cbd->respHandler->complete_callback)
    {
        (cbd->respHandler->complete_callback)(status,
                                     error, cbd->callbackDataIn);
    }
    return;    
}

static obs_status getObjectPartDataCallback(int buffer_size, const char *buffer,
                                            void *callback_data)
{
    download_file_callback_data * cbd = (download_file_callback_data *)callback_data;

    int fd = cbd->fdStorefile;

    size_t wrote = write(fd,buffer,buffer_size);
    
    return ((wrote < (size_t) buffer_size) ? 
            OBS_STATUS_AbortedByCallback : OBS_STATUS_OK);
}

#if defined (WIN32)
unsigned __stdcall DownloadThreadProc_win32(void* param)
{
    download_file_proc_data * pstPara = (download_file_proc_data *)param;
    char * storeFileName = pstPara->pstDownloadParams->fileNameStore;
    uint64_t part_size = pstPara->pstDownloadFilePartInfo->part_size;
    int part_num = pstPara->pstDownloadFilePartInfo->part_num;
    server_side_encryption_params * pstEncrypParam;
    char strPartNum[16] = {0};
    download_file_callback_data  data;
    int fd = -1;
    char * fileNameTemp = (char*)malloc(1024);
	
	if (fileNameTemp == NULL)
	{
		COMMLOG(OBS_LOGWARN, "DownloadThreadProc_win32: malloc failed!\n");
	}
	
    sprintf_sec(fileNameTemp,1024,"%s.%d",storeFileName,part_num);

    (void)_sopen_s( &fd, fileNameTemp, _O_BINARY |  _O_RDWR | _O_CREAT,
                  _SH_DENYNO, _S_IREAD | _S_IWRITE );

    free(fileNameTemp);
    fileNameTemp  = NULL;

    if(fd == -1)
    {
        COMMLOG(OBS_LOGWARN, "DownloadThreadProc_win32 open upload file failed, partnum[%d]\n",part_num);
    }
    else
    {
        obs_get_object_handler getObjectHandler = 
        { 
            {&DownloadPartCompletePropertiesCallback,
            &downloadPartCompleteCallback}, 
            &getObjectPartDataCallback 
        };
        
        sprintf_sec(strPartNum,16,"%d",part_num+1);
        memset(&data,0,sizeof(download_file_callback_data));
        data.bytesRemaining = part_size;
        data.totalBytes = part_size;
        data.callbackDataIn = pstPara->callBackData;
        data.checkpointFilename = pstPara->pstDownloadParams->fileNameCheckpoint;
        data.enableCheckPoint = pstPara->pstDownloadParams->enable_check_point;
        data.fdStorefile = fd;
        data.respHandler = pstPara->pstDownloadParams->response_handler;
        data.taskHandler = 0;
        data.pstDownloadFilePartInfo = pstPara->pstDownloadFilePartInfo;

     pstEncrypParam = pstPara->pstDownloadParams->pstServerSideEncryptionParams;
        
        if(data.enableCheckPoint == 1)
        {
            char pathToUpdate[1024];
            char contentToSet[32];

            sprintf_sec(pathToUpdate,1024,"%s%s/%s","downloadinfo/partsinfo/part",strPartNum,"downloadStatus");
            sprintf_sec(contentToSet,32,"%s","DOWNLOADING");            
            EnterCriticalSection((CRITICAL_SECTION *)pstPara->xmlWriteMutex);
            updateCheckPoint(pathToUpdate, contentToSet, pstPara->pstDownloadParams->fileNameCheckpoint);
            LeaveCriticalSection((CRITICAL_SECTION *)pstPara->xmlWriteMutex);
            
        }
        obs_object_info object_info;
        memset(&object_info,0,sizeof(obs_object_info));
        object_info.key = pstPara->pstDownloadParams->objectName;
        object_info.version_id = pstPara->pstDownloadParams->version_id;
        pstPara->pstDownloadFilePartInfo->downloadStatus = DOWNLOADING;

        obs_get_conditions get_conditions = *(pstPara->pstDownloadParams->get_conditions);
        get_conditions.start_byte = pstPara->pstDownloadFilePartInfo->start_byte;
        get_conditions.byte_count = part_size;
        COMMLOG(OBS_LOGINFO, "get_object partnum[%d] start:%ld size:%ld",part_num, get_conditions.start_byte, get_conditions.byte_count);
        get_object(pstPara->pstDownloadParams->options, &object_info, &get_conditions,
                   pstPara->pstDownloadParams->pstServerSideEncryptionParams, &getObjectHandler,&data );
    }
    
    if (data.fdStorefile != -1)
    {
        close(data.fdStorefile);
        data.fdStorefile = -1;
    }
    
    return 1;
}
#endif

#if defined __GNUC__ || defined LINUX
void * DownloadThreadProc_linux(void* param)
{
    download_file_proc_data * pstPara = (download_file_proc_data *)param;
    char * storeFileName = pstPara->pstDownloadParams->fileNameStore;
    uint64_t part_size = pstPara->pstDownloadFilePartInfo->part_size;
    int part_num = pstPara->pstDownloadFilePartInfo->part_num;
    char strPartNum[16] = {0};
    download_file_callback_data  data;
    int fd = -1;
    char * fileNameTemp = (char*)malloc(1024);
	
	if (fileNameTemp == NULL)
	{
		COMMLOG(OBS_LOGWARN, "DownloadThreadProc_linux: malloc failed!\n");
	}
	
    sprintf_sec(fileNameTemp,1024,"%s.%d",storeFileName,part_num);
                  
    fd = open(fileNameTemp,O_WRONLY |O_CREAT |O_TRUNC, S_IRUSR|S_IWUSR);
    free(fileNameTemp);
    fileNameTemp  = NULL;

    if(fd == -1)
    {
        COMMLOG(OBS_LOGERROR, "open store file failed, partnum[%d]\n",part_num);
        return NULL;
    }
    else
    {
        obs_get_object_handler getObjectHandler = 
        { 
            {&DownloadPartCompletePropertiesCallback,
            &downloadPartCompleteCallback}, 
            &getObjectPartDataCallback 
        };
        
        sprintf_sec(strPartNum,16,"%d",part_num+1);
        
        memset(&data,0,sizeof(download_file_callback_data));

        data.bytesRemaining = part_size;
        data.totalBytes = part_size;
        data.callbackDataIn = pstPara->callBackData;
        data.checkpointFilename = pstPara->pstDownloadParams->fileNameCheckpoint;
        data.enableCheckPoint = pstPara->pstDownloadParams->enable_check_point;
        data.fdStorefile = fd;
        data.respHandler = pstPara->pstDownloadParams->response_handler;
        data.taskHandler = 0;
        data.pstDownloadFilePartInfo = pstPara->pstDownloadFilePartInfo;
        data.xmlWriteMutex = pstPara->xmlWriteMutex;

        
        if(data.enableCheckPoint == 1)
        {
            char pathToUpdate[1024];
            char contentToSet[32];

            sprintf_sec(pathToUpdate,1024,"%s%s/%s","downloadinfo/partsinfo/part",strPartNum,"downloadStatus");
            sprintf_sec(contentToSet,32,"%s","DOWNLOADING");            
            pthread_mutex_lock((pthread_mutex_t *)pstPara->xmlWriteMutex);
            updateCheckPoint(pathToUpdate, contentToSet, pstPara->pstDownloadParams->fileNameCheckpoint);
            pthread_mutex_unlock((pthread_mutex_t *)pstPara->xmlWriteMutex);
            
        }

        obs_object_info object_info;
        memset(&object_info,0,sizeof(obs_object_info));
        object_info.key = pstPara->pstDownloadParams->objectName;
        object_info.version_id = pstPara->pstDownloadParams->version_id;
        pstPara->pstDownloadFilePartInfo->downloadStatus = DOWNLOADING;

        obs_get_conditions get_conditions = *(pstPara->pstDownloadParams->get_conditions);
        get_conditions.start_byte = pstPara->pstDownloadFilePartInfo->start_byte;
        get_conditions.byte_count = part_size;
        COMMLOG(OBS_LOGINFO, "get_object partnum[%d] start:%ld size:%ld",part_num, get_conditions.start_byte, get_conditions.byte_count);
        get_object(pstPara->pstDownloadParams->options, &object_info, &get_conditions,
                   pstPara->pstDownloadParams->pstServerSideEncryptionParams, &getObjectHandler,&data );
     }

     if (data.fdStorefile != -1)
     {
         close(data.fdStorefile);
         data.fdStorefile = -1;
     }
     
     return NULL;
}
#endif


void startDownloadThreads(download_params * pstDownloadParams, 
                          download_file_part_info * downloadFilePartInfoList,  
                          int partCount, void* callback_data, void *xmlwrite_mutex)
{

    int i=0;
    download_file_proc_data * downloadFileProcDataList = 
        (download_file_proc_data *)malloc(sizeof(download_file_proc_data)*partCount);	
	if (downloadFileProcDataList == NULL)
	{
		COMMLOG(OBS_LOGWARN, "startDownloadThreads: downloadFileProcDataList malloc failed\n");
	}
	
    download_file_proc_data * pstDownloadFileProcData = downloadFileProcDataList;
    download_file_part_info *pstOnePartInfo = downloadFilePartInfoList;
#ifdef WIN32
    HANDLE * arrHandle = (HANDLE *)malloc(sizeof(HANDLE)*partCount);
    unsigned  uiThread2ID;
    DWORD   dwExitCode;

#endif

#if defined __GNUC__ || defined LINUX
    pthread_t * arrThread = (pthread_t *)malloc(sizeof(pthread_t)*partCount);
	
	if (arrThread == NULL)
	{
		COMMLOG(OBS_LOGWARN, "startDownloadThreads: arrThread malloc failed\n",i);
	}
	
    int err;
#endif
    memset(downloadFileProcDataList,0,sizeof(download_file_proc_data)*partCount);

    for(i=0;i<partCount;i++)
    {
        pstDownloadFileProcData->pstDownloadParams = pstDownloadParams;
        pstDownloadFileProcData->pstDownloadFilePartInfo = pstOnePartInfo;
        pstDownloadFileProcData->callBackData = callback_data;
        pstDownloadFileProcData->xmlWriteMutex = xmlwrite_mutex;
        pstOnePartInfo = pstOnePartInfo->next;
        pstDownloadFileProcData ++;
    }
    pstOnePartInfo = downloadFilePartInfoList;    
#ifdef WIN32    
    for(i=0;i<partCount;i++)
    {
        arrHandle[i] = (HANDLE)_beginthreadex(NULL,0,DownloadThreadProc_win32, 
                                              &downloadFileProcDataList[i],CREATE_SUSPENDED,&uiThread2ID);
        if(arrHandle[i] ==0){
            GetExitCodeThread( arrHandle[i], &dwExitCode );
            COMMLOG(OBS_LOGERROR, "create thread i[%d] failed exit code = %u \n",i,dwExitCode);
        }
        pstOnePartInfo = pstOnePartInfo->next;
    } 

    for(i=0;i<partCount;i++)
    {
        ResumeThread(arrHandle[i]);
    }
    for(i=0;i<partCount;i++)
    {
        WaitForSingleObject(arrHandle[i],INFINITE);
    }

     for(i=0;i<partCount;i++)
    {
        CloseHandle(arrHandle[i]);
    }

    if(arrHandle)
    {
        free(arrHandle);
        arrHandle = NULL;
    } 
#endif
#if defined __GNUC__ || defined LINUX
    for (i = 0; i < partCount; i++)
    {
        err = pthread_create(&arrThread[i], NULL,DownloadThreadProc_linux,(void *)&downloadFileProcDataList[i]);
        if(err != 0)
        {
            COMMLOG(OBS_LOGWARN, "startDownloadThreads create thread failed i[%d]\n",i);
        }
    } 

    for (i = 0; i < partCount; i++)
    {
        err = pthread_join(arrThread[i], NULL);
        if(err != 0)
        {
            COMMLOG(OBS_LOGWARN, "startDownloadThreads join thread failed i[%d]\n",i);
        }
    } 

    if(arrThread)
    {
        free(arrThread);
        arrThread = NULL;
    }
#endif
    
    if(downloadFileProcDataList)
    {
        free(downloadFileProcDataList);
        downloadFileProcDataList = NULL;
    }    
}


int isAllDownLoadPartsSuccess(download_file_part_info * downloadPartNode)
{
    download_file_part_info * ptrDownloadPartPrev = downloadPartNode;
    download_file_part_info * ptrDownloadPartNext = downloadPartNode;

    if(downloadPartNode == NULL)
    {
        return 0;
    }
    while(ptrDownloadPartPrev)
    {
        if((ptrDownloadPartPrev->downloadStatus!= DOWNLOAD_SUCCESS)&&(ptrDownloadPartPrev->downloadStatus!= COMBINE_SUCCESS))
        {
            return 0;
        }
        ptrDownloadPartPrev = ptrDownloadPartPrev->prev;
    }
    while(ptrDownloadPartNext)
    {
        if((ptrDownloadPartNext->downloadStatus != DOWNLOAD_SUCCESS)&&(ptrDownloadPartNext->downloadStatus != COMBINE_SUCCESS))
        {
            return 0;
        }
        ptrDownloadPartNext = ptrDownloadPartNext->next;
    }
    return 1;
}

int combinePartsFile(const char * fileName, download_file_part_info * downloadPartList, const char * check_point_file, void *xmlwrite_mutex)
{
    download_file_part_info * partNode = downloadPartList;
    char fileNameTemp[1024] = {0};
    int fdDest = -1;
    int fdSrc = -1;
    uint64_t remain_bytes = 0;
    int bytesToRead = 0;
    int bytesReadOut = 0;
    int bytesWritten = 0;
    char * buff = NULL; 
    int writeSuccess = 1;
    int is_true = 0;

    is_true = ((fileName == NULL) || (downloadPartList == NULL));
    if(is_true)
    {
        return -1;
    } 
#if defined WIN32
    _sopen_s(&fdDest, fileName,_O_BINARY |_O_WRONLY | _O_CREAT,
                  _SH_DENYNO, _S_IREAD | _S_IWRITE);
#endif

#if defined __GNUC__ || defined LINUX
    fdDest = open(fileName,O_WRONLY |O_CREAT, S_IRUSR|S_IWUSR);
#endif
    if(fdDest == -1)
    {
        COMMLOG(OBS_LOGERROR, "%s open file[%s] failed\n","combinePartsFile",fileName);
        return -1;
    }   
    
    buff = (char *)malloc(MAX_READ_ONCE);
    if (!buff)
    {
        COMMLOG(OBS_LOGERROR, "malloc failed.");
        close(fdDest);
        fdDest = -1; 
        return -1;
    }
    
    while(partNode)
    {     
        char pathToUpdate[1024];
        char contentToSet[32];
        writeSuccess = 1;

        if(partNode->downloadStatus == COMBINE_SUCCESS)
        {
#ifdef WIN32
           if(_lseeki64(fdDest, (long long int)partNode->part_size, SEEK_CUR)==-1)
           {
               return -1;
           }
#else
           if(lseek(fdDest, (long long int)partNode->part_size, SEEK_CUR)==-1)
           {
               return -1;
           }
#endif
            partNode = partNode->next;
            continue;
        }
        
        sprintf_sec(fileNameTemp,1024,"%s.%d",fileName,partNode->part_num);
        
#if defined WIN32
        Sleep(0);
        _sopen_s(&fdSrc, fileNameTemp,_O_BINARY | _O_RDONLY,
                  _SH_DENYWR, _S_IREAD);
#endif

#if defined __GNUC__ || defined LINUX
        sleep(0);
        fdSrc = open(fileNameTemp,O_RDONLY);
#endif
        if(fdSrc == -1)
        {
            COMMLOG(OBS_LOGERROR, "%s open file[%s] failed\n","combinePartsFile",fileNameTemp);
            close(fdDest);             
            fdDest = -1;
            free(buff);
            buff = NULL;
            return -1;
        }
        
        remain_bytes = partNode->part_size;
        while(remain_bytes)
        {
#if defined WIN32
            Sleep(0);
#endif

#if defined __GNUC__ || defined LINUX
             sleep(0);
#endif
            bytesToRead = (int)((remain_bytes>MAX_READ_ONCE)?MAX_READ_ONCE:remain_bytes);
            
			bytesReadOut = read(fdSrc,buff,bytesToRead);
			if (bytesReadOut < 0)
			{
				COMMLOG(OBS_LOGWARN, "combinePartsFile: bytesReadOut is negative");
				writeSuccess = 0;
				break;
			}
			
            bytesWritten = write(fdDest,buff,bytesReadOut);
            if(bytesWritten < bytesReadOut)
            {
                writeSuccess = 0;
                break;                
            }
            remain_bytes = remain_bytes - bytesWritten;
        }
        close(fdSrc);
        fdSrc = -1;
        if(writeSuccess == 1)
        {
            partNode->downloadStatus = COMBINE_SUCCESS;
            if(check_point_file)
            {
                sprintf_sec(pathToUpdate,1024,"%s%d/%s","downloadinfo/partsinfo/part",partNode->part_num + 1,"downloadStatus");
                sprintf_sec(contentToSet,32,"%s","COMBINE_SUCCESS");

#if defined(WIN32)
                EnterCriticalSection((CRITICAL_SECTION *)xmlwrite_mutex);
#endif

#if defined __GNUC__ || defined LINUX
                pthread_mutex_lock((pthread_mutex_t *)xmlwrite_mutex);
#endif
                updateCheckPoint(pathToUpdate, contentToSet, check_point_file);
#if defined(WIN32)
                LeaveCriticalSection((CRITICAL_SECTION *)xmlwrite_mutex);
#endif

#if defined __GNUC__ || defined LINUX
                pthread_mutex_unlock((pthread_mutex_t *)xmlwrite_mutex);
#endif
            }
            //must resure do remove tempfile after updateCheckPoint(COMBINE_SUCCESS)
            remove(fileNameTemp);
        
       }
       else
       {
            if(check_point_file == NULL)//break-point-continue down, is not enabled
            {
                removeTempFiles(fileName,downloadPartList,0);
            }
            break;
       }
       partNode = partNode->next;
    }


   
    close(fdDest);
    fdDest = -1; 
    free(buff);
    buff = NULL;
    //here, remove the last file, it may contains some parts combined in
    is_true = ((writeSuccess == 0) && (check_point_file == NULL));
    if (is_true)
    {
        (void)remove(fileName);
    }
    
    return 0;    
}

int setDownloadReturnPartList(download_file_part_info * partListIntern,
                              obs_download_file_part_info **partListReturn, int partCount)
{
    int i = 0;
    download_file_part_info * partInfoNode = partListIntern;
    obs_download_file_part_info * partListReturnTemp = 
        (obs_download_file_part_info *)malloc(sizeof(obs_download_file_part_info)*partCount);
    if(partListReturnTemp == NULL)
    {
        return -1;
    }
    (*partListReturn) = partListReturnTemp;
    for(i=0;i<partCount;i++)
    {
        partListReturnTemp->part_num = partInfoNode->part_num + 1;
        partListReturnTemp->part_size =  partInfoNode->part_size;
        partListReturnTemp->start_byte = partInfoNode->start_byte;
        partListReturnTemp->status_return = partInfoNode->downloadStatus;
        partInfoNode = partInfoNode->next;
        partListReturnTemp++;
    }    
    return 0;
}

int isObjectChanged(download_file_summary * infoNew, download_file_summary * infoOld)
{
    if( (infoNew->lastModify != infoOld->lastModify) 
        || (infoNew->objectLength != infoOld->objectLength)
        || (infoNew->storage_class!= infoOld->storage_class))
    {
        return 1;
    }

    if(strcmp(infoNew->etag,infoOld->etag))
    {
        return 1;
    }
    return 0;
}

int checkDownloadPartsInfo(download_file_part_info * downloadPartList)
{
    download_file_part_info * partNode = downloadPartList;
    download_file_part_info * partNodePrev = NULL;
    int isValid = 1;
    while(partNode)
    {
#if defined WIN32
        Sleep(0);
#endif

#if defined __GNUC__ || defined LINUX
        sleep(0);
#endif
        if(partNode->prev)
        {
            partNodePrev = partNode->prev;
            if((partNodePrev->start_byte + partNodePrev->part_size) != partNode->start_byte)
            {
               isValid = 0;
               break;
            }
        }
        partNode = partNode->next;
    } 
    return isValid;
}

static int get_download_isfirst_time(obs_download_file_configuration * download_file_config, char *storeFile,
            const char *key, char *checkpointFile, download_file_summary *pdownLoadFileInfo,
            download_file_part_info** pstDownloadFilePartInfoList, int *partCount)
{
    int isFirstTime = 1;
    int retVal = -1;
    int isObjectModified = 0;
    int isPatsInfoValid= 0;
    int is_true = 0;
    download_file_summary downLoadFileInfoOld;
    
    //2,set the file to store the object, and the checkpoint file
    is_true = ((download_file_config->downLoad_file == NULL)
                    ||(!strlen(download_file_config->downLoad_file)));
    if (is_true)
    {
        memcpy_s(storeFile,1024,key,strlen(key)+1);
    }
    else 
    {
		errno_t err = EOK; 
        err = memcpy_s(storeFile,1024,download_file_config->downLoad_file,strlen(download_file_config->downLoad_file)+1);
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "get_download_isfirst_time: memcpy_s failed!\n");
		}
    }

    is_true = ((download_file_config->check_point_file!=NULL)
                    && (strlen(download_file_config->check_point_file)!=0));
    if (is_true)
    {
        errno_t err = EOK;  
		err = memcpy_s(checkpointFile,1024,download_file_config->check_point_file,
                        strlen(download_file_config->check_point_file)+1);
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "get_download_isfirst_time: memcpy_s failed!\n");
		}
    }
    else
    {
        memset_s(checkpointFile,1024,0,1024);
    }

    if (download_file_config->enable_check_point)
    {
        retVal = setCheckPointFile(storeFile,checkpointFile, &isFirstTime,DOWNLOAD_FILE_INFO);
        if(retVal == -1) 
        {
            download_file_config->enable_check_point = 0;
            isFirstTime = 1;
        }
    }

    //3, read the content of the checkpoint file
    if(!download_file_config->enable_check_point)
    {
         return isFirstTime;
    }
    
    retVal = readCheckpointFile_Download(&downLoadFileInfoOld,
                        pstDownloadFilePartInfoList,partCount,checkpointFile);        
    if(retVal == -1)
    {
        isFirstTime = 1;
        if(*pstDownloadFilePartInfoList != NULL)
        {
            cleanDownloadList(*pstDownloadFilePartInfoList);
            *pstDownloadFilePartInfoList = NULL;
        }            
    }
    else
    {
        isObjectModified =  isObjectChanged(pdownLoadFileInfo,&downLoadFileInfoOld);
        isPatsInfoValid = checkDownloadPartsInfo(*pstDownloadFilePartInfoList);
        is_true = ((isObjectModified)||(!isPatsInfoValid));
        if (is_true)
        {
            removeTempFiles(storeFile,*pstDownloadFilePartInfoList,1);
            isFirstTime = 1;
            if(*pstDownloadFilePartInfoList != NULL)
            {
                cleanDownloadList(*pstDownloadFilePartInfoList);
                pstDownloadFilePartInfoList = NULL;
            }
        }                        
    }

    return isFirstTime;
}

void download_complete_handle(download_file_part_info * pstPartInfoListDone,
          obs_download_file_configuration * download_file_config,
          char *checkpointFile, const char *storeFile,
          obs_download_file_response_handler *handler, void *callback_data,
          int partCount, void *xmlwrite_mutex)
{
    int retVal = -1;
    download_file_part_info * pstDownloadFilePartInfoList = NULL;
    
    pstDownloadFilePartInfoList = pstPartInfoListDone;
    if(isAllDownLoadPartsSuccess(pstDownloadFilePartInfoList))
    {
        char * pstCheckPoint = download_file_config->enable_check_point ? checkpointFile : NULL;
        COMMLOG(OBS_LOGINFO, "%s all parts download success\n","DownloadFile");
        retVal = combinePartsFile(storeFile,pstDownloadFilePartInfoList, pstCheckPoint, xmlwrite_mutex);
        if(retVal == 0)
        {
            char strReturn[1024] = {0};
            sprintf_sec(strReturn,100,"DownloadFile %s success\n",storeFile);
            COMMLOG(OBS_LOGINFO, "DownloadFile combine success\n");
            if(handler->download_file_callback)
            {
                handler->download_file_callback(OBS_STATUS_OK,strReturn,0,NULL,callback_data);
            }
            remove(checkpointFile);
        }
        else
        {
            COMMLOG(OBS_LOGERROR, "DownloadFile combine failed\n");
            if(download_file_config->enable_check_point == 0)
            {
                removeTempFiles(storeFile,pstDownloadFilePartInfoList,1);    
            }
            if(handler->download_file_callback)
            {
                handler->download_file_callback(OBS_STATUS_InternalError,
                    "DownloadFile combine failed\n",0,NULL,callback_data);
            }
        }
    }
    else
    {
        obs_download_file_part_info * partListReturn = NULL;
        COMMLOG(OBS_LOGERROR, "%s Not all parts download success\n","DownloadFile");
        retVal = setDownloadReturnPartList(pstDownloadFilePartInfoList,&partListReturn, partCount);
        if((retVal == 0) && (handler->download_file_callback))
        {
            handler->download_file_callback(OBS_STATUS_InternalError,"DownloadFile Not all parts download success\n",
                partCount,partListReturn,callback_data);
        }

        if(download_file_config->enable_check_point==0)
        {
             removeTempFiles(storeFile,pstDownloadFilePartInfoList,1);    
        }

        if(partListReturn)
        {
            free(partListReturn);
            partListReturn = NULL;
        }
    }  

    if(pstDownloadFilePartInfoList)
    {
        cleanDownloadList(pstDownloadFilePartInfoList);
        pstDownloadFilePartInfoList = NULL;
    }

    return;
}

void download_file(const obs_options *options, char *key, char* version_id,
                   obs_get_conditions *get_conditions,
                   server_side_encryption_params *encryption_params,
                   obs_download_file_configuration * download_file_config,
                   obs_download_file_response_handler *handler, void *callback_data)
{
    download_file_summary downLoadFileInfo;
    int retVal = -1;
    char storeFile[1024] = {0};
    int isFirstTime = 1;
    download_file_part_info * pstDownloadFilePartInfoList = NULL;
    download_file_part_info * pstPartInfoListDone = NULL;
    download_file_part_info * pstPartInfoListNotDone = NULL;
    int partCount = 0;
    char checkpointFile[1024];
    int partCountToProc = 0;
    uint64_t part_size = 0;
    int is_true = 0;
#if defined __GNUC__ || defined LINUX
    pthread_mutex_t mutexThreadCheckpoint;
#endif
#if defined WIN32
    CRITICAL_SECTION  mutexThreadCheckpoint;
#endif


    download_params stDownloadParams;
    COMMLOG(OBS_LOGERROR, "in DownloadFile download_file_config: partsize=%d ",download_file_config->part_size);

    memset_s(&downLoadFileInfo,sizeof(download_file_summary),0,sizeof(download_file_summary));
    //get the info of the object
    obs_status ret_status = getObjectInfo(&downLoadFileInfo, options, key, version_id,encryption_params);
    if(OBS_STATUS_OK != ret_status)
    {
       COMMLOG(OBS_LOGERROR, "in DownloadFile Get object metadata failed(%d),bucket=%s, key=%s,version_id=%s",
                      ret_status, options->bucket_options.bucket_name,
                      key,
                      version_id);
       (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
       return;
    }
    //if the storage is glacier, restore the object firstly
    if(downLoadFileInfo.storage_class == OBS_STORAGE_CLASS_GLACIER)
    {
        ret_status = restoreGlacierObject(options,key,version_id);
        if(OBS_STATUS_OK != ret_status)
        {
            COMMLOG(OBS_LOGERROR, "in DownloadFile restoreGlacierObject failed(%d).", ret_status);
            (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
            return;
        }
    }

    //2,set the file to store the object, and the checkpoint file
    isFirstTime = get_download_isfirst_time(download_file_config, storeFile, key, checkpointFile, 
                        &downLoadFileInfo, &pstDownloadFilePartInfoList, &partCount);
    
    is_true = ((download_file_config->part_size <= 0) 
            || (download_file_config->part_size > MAX_PART_SIZE));
    part_size = is_true ? DEFAULT_PART_SIZE : download_file_config->part_size;
    part_size = part_size > downLoadFileInfo.objectLength ? downLoadFileInfo.objectLength : part_size;

    //set down load part list
    is_true = ((isFirstTime == 1)||(download_file_config->enable_check_point == 0));
    if (is_true)
    {
        retVal = setDownloadpartList(&downLoadFileInfo,part_size,&pstDownloadFilePartInfoList,&partCount);
        if (retVal == -1)
        {
            if (download_file_config->enable_check_point)
            {
                remove(checkpointFile);
            }            
            return;
        }
    }

    is_true = ((isFirstTime == 1) && (download_file_config->enable_check_point == 1));
    if (is_true)
    {
        (void)writeCheckpointFile_Download(&downLoadFileInfo,
                         pstDownloadFilePartInfoList,partCount,checkpointFile);
    }

    //divid the list
    (void)DividDownloadPartList(pstDownloadFilePartInfoList,&pstPartInfoListDone,&pstPartInfoListNotDone);

    //start thread to download
    memset_s(&stDownloadParams,sizeof(download_params),0,sizeof(download_params));
    stDownloadParams.callBackData = callback_data;
    stDownloadParams.enable_check_point = download_file_config->enable_check_point;
    stDownloadParams.fileNameCheckpoint = checkpointFile;
    stDownloadParams.fileNameStore = storeFile;
    stDownloadParams.objectName = key;
    stDownloadParams.options = options;
    stDownloadParams.version_id = version_id;
    stDownloadParams.pstServerSideEncryptionParams = encryption_params;
    stDownloadParams.response_handler = &(handler->response_handler);
    stDownloadParams.get_conditions = get_conditions;
    download_file_config->task_num = download_file_config->task_num == 0 ? MAX_THREAD_NUM :
                            download_file_config->task_num;                          
    partCountToProc = 0;

    if (download_file_config->enable_check_point)
    {
    #if defined __GNUC__ || defined LINUX
        pthread_mutex_init(&mutexThreadCheckpoint,NULL); 
    #endif
    #if defined WIN32
        InitializeCriticalSection(&mutexThreadCheckpoint);
    #endif
    }
    
    while(pstPartInfoListNotDone)
    {
        GetDownloadPartListToProcess(&pstPartInfoListDone,&pstPartInfoListNotDone,
            partCountToProc,&partCountToProc,download_file_config->task_num);
        if(partCountToProc > 0)
        {
            startDownloadThreads(&stDownloadParams,pstPartInfoListNotDone,partCountToProc,callback_data, &mutexThreadCheckpoint); 
        }
    }
    download_complete_handle(pstPartInfoListDone, download_file_config, checkpointFile, storeFile,
          handler, callback_data, partCount, &mutexThreadCheckpoint);

    if (download_file_config->enable_check_point)
    {
    #if defined __GNUC__ || defined LINUX
        pthread_mutex_destroy(&mutexThreadCheckpoint);  
    #endif
    #if defined WIN32
        DeleteCriticalSection(&mutexThreadCheckpoint);
    #endif
    }
}


