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
#include <string.h>
#include <stdlib.h>
#include "eSDKOBS.h"
#include "request.h"
#include "simplexml.h"
#include "securec.h"
#include "util.h"
#include "bucket.h"
#include "request_util.h"
#include <openssl/md5.h> 

static int update_bucket_common_data_callback(int buffer_size, char *buffer,
                                    void *callback_data)
{
    update_bucket_common_data *bucket_data = (update_bucket_common_data *) callback_data;

    if (!bucket_data->docLen) {
        return 0;
    }

    int remaining = (bucket_data->docLen - bucket_data->docBytesWritten);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy) {
        return 0;
    }
    
	errno_t err = EOK;  
	err = memcpy_s(buffer,buffer_size,&(bucket_data->doc[bucket_data->docBytesWritten]),toCopy);
    if (err != EOK)
    {
		COMMLOG(OBS_LOGWARN, "update_bucket_common_data_callback: memcpy_s failed!\n");
		return 0;
    }
	
	bucket_data->docBytesWritten += toCopy;

    return toCopy;
}

static void update_bucket_common_complete_callback(obs_status status,
                                         const obs_error_details *error_details,
                                         void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    update_bucket_common_data *bucket_data = (update_bucket_common_data *) callback_data;
    (void)(*(bucket_data->complete_callback))(status, error_details, bucket_data->callback_data);

    free(bucket_data);
    bucket_data = NULL;
    return;
}

update_bucket_common_data* init_create_bucket_cbdata(const char *location_constraint, obs_use_api use_api)
{
    update_bucket_common_data *bucket_data = (update_bucket_common_data *) malloc(sizeof(update_bucket_common_data));
    if (!bucket_data) 
    {
        return NULL;
    }
    memset_s(bucket_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));

    char*plocationConstraint = 0;
    if (location_constraint) 
    {
        int mark = pcre_replace(location_constraint,&plocationConstraint);
        if (use_api == OBS_USE_API_S3) {
            bucket_data->docLen = snprintf_sec(bucket_data->doc, sizeof(bucket_data->doc), _TRUNCATE,  
                         "<CreateBucketConfiguration><LocationConstraint>"
                         "%s</LocationConstraint></CreateBucketConfiguration>",
                         mark ? plocationConstraint : location_constraint);
        } else {
            bucket_data->docLen = snprintf_sec(bucket_data->doc, sizeof(bucket_data->doc), _TRUNCATE,  
                         "<CreateBucketConfiguration><Location>"
                         "%s</Location></CreateBucketConfiguration>",
                         mark ? plocationConstraint : location_constraint);
        }
        bucket_data->docBytesWritten = 0;
        if(mark)
        {
            free(plocationConstraint);
            plocationConstraint = NULL;
        }
    }
    else
    {
        bucket_data->docLen = 0;
    }
    
    return bucket_data;
}

obs_status update_bucket_common_properties_callback(const obs_response_properties *response_properties, 
    void *callback_data)
{
    update_bucket_common_data *bucket_data = (update_bucket_common_data *) callback_data;

    if (bucket_data->properties_callback)
    {
        return (*(bucket_data->properties_callback))
            (response_properties, bucket_data->callback_data);
    }
    return OBS_STATUS_OK;
}

void create_bucket(const obs_options *options, obs_canned_acl canned_acl,
        const char *location_constraint, obs_response_handler *handler,
        void *callback_data)
{
    request_params      params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    obs_put_properties  properties;
    update_bucket_common_data    *bucket_data = NULL;
    
    COMMLOG(OBS_LOGINFO, "create bucket start!");
    
    bucket_data = init_create_bucket_cbdata(location_constraint, use_api);
    if (!bucket_data) 
    {
        COMMLOG(OBS_LOGERROR, "Malloc update_bucket_common_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    bucket_data->complete_callback = handler->complete_callback;
    bucket_data->callback_data = callback_data;
    bucket_data->properties_callback = handler->properties_callback;
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = canned_acl;

    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.put_properties         = &properties;
    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &update_bucket_common_properties_callback;
    params.toObsCallback          = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = bucket_data->docLen;
    params.complete_callback      = &update_bucket_common_complete_callback;
    params.callback_data          = bucket_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = default_storage_class;
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "create bucket finish!");
}

void create_bucket_with_params(const obs_options *options, const obs_create_bucket_params *param,
        obs_response_handler *handler, void *callback_data)
{
    request_params      params;
    obs_use_api use_api = OBS_USE_API_S3;
    //if create bucket with 3az,must be use obs protocol
    if (OBS_REDUNDANCY_3AZ == param->az_redundancy)
    {   
        use_api = OBS_USE_API_OBS;
    }
    else 
    {
        set_use_api_switch(options, &use_api);
    }
    obs_put_properties  properties;
    update_bucket_common_data    *bucket_data = NULL;
    
    COMMLOG(OBS_LOGINFO, "create bucket start!");
    
    bucket_data = init_create_bucket_cbdata(param->location_constraint, use_api);
    if (!bucket_data) 
    {
        COMMLOG(OBS_LOGERROR, "Malloc update_bucket_common_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    bucket_data->complete_callback = handler->complete_callback;
    bucket_data->callback_data = callback_data;
    bucket_data->properties_callback = handler->properties_callback;
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = param->canned_acl;
    properties.az_redundancy = param->az_redundancy;

    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.put_properties         = &properties;
    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &update_bucket_common_properties_callback;
    params.toObsCallback          = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = bucket_data->docLen;
    params.complete_callback      = &update_bucket_common_complete_callback;
    params.callback_data          = bucket_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = default_storage_class;
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "create bucket finish!");
}



// create pfs bucket.
void create_pfs_bucket(const obs_options *options, obs_canned_acl canned_acl,
        const char *location_constraint, obs_response_handler *handler,
        void *callback_data)
{
    request_params      params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    obs_put_properties  properties;
    update_bucket_common_data    *bucket_data = NULL;
    
    COMMLOG(OBS_LOGINFO, "create pfs bucket start!");
    bucket_data = init_create_bucket_cbdata(location_constraint, use_api);
    if (!bucket_data) 
    {
        COMMLOG(OBS_LOGERROR, "Malloc update_bucket_common_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        return;
    }
    bucket_data->complete_callback = handler->complete_callback;
    bucket_data->callback_data = callback_data;
    bucket_data->properties_callback = handler->properties_callback;
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = canned_acl;

    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    // create pfs bucket
    params.bucketContext.bucket_type = OBS_BUCKET_PFS;

    params.put_properties         = &properties;
    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &update_bucket_common_properties_callback;
    params.toObsCallback          = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = bucket_data->docLen;
    params.complete_callback      = &update_bucket_common_complete_callback;
    params.callback_data          = bucket_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = default_storage_class;
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "create pfs bucket finish!");
	
    return;
}


static obs_status xml_callback(const char *element_path, const char *data,
                            int data_len, void *callback_data)
{
    xml_callback_data *cbData = (xml_callback_data *) callback_data;

    int fit;

    if (data) {
        if (!strcmp(element_path, "ListAllMyBucketsResult/Owner/ID")) {
            string_buffer_append(cbData->owner_id, data, data_len, fit);
        }
        else if (!strcmp(element_path,
                         "ListAllMyBucketsResult/Owner/DisplayName")) {
            string_buffer_append(cbData->owner_display_name, data, data_len, fit);
        }
        else if (!strcmp(element_path,
                         "ListAllMyBucketsResult/Buckets/Bucket/Name")) {
            string_buffer_append(cbData->bucket_name, data, data_len, fit);
        }
        else if (!strcmp
                 (element_path,
                  "ListAllMyBucketsResult/Buckets/Bucket/CreationDate")) {
            string_buffer_append(cbData->creationDate, data, data_len, fit);
        }
    }
    else {
        if (!strcmp(element_path, "ListAllMyBucketsResult/Buckets/Bucket")) {
            time_t creationDate = parseIso8601Time(cbData->creationDate);
            int nTimeZone = getTimeZone();
            creationDate += nTimeZone * SECONDS_TO_AN_HOUR;

            obs_status status = (*(cbData->listServiceCallback))
                (cbData->owner_id, cbData->owner_display_name,
                 cbData->bucket_name, creationDate, cbData->callback_data);

            string_buffer_initialize(cbData->bucket_name);
            string_buffer_initialize(cbData->creationDate);

            return status;
        }
    }
    (void) fit;
    return OBS_STATUS_OK;
}

static obs_status xml_obs_callback(const char *element_path, const char *data,
                                   int data_len, void *callback_data)
{
    xml_obs_callback_data *cbData = (xml_obs_callback_data *) callback_data;

    int fit;

    if (data) {
        if (!strcmp(element_path, "ListAllMyBucketsResult/Owner/ID")) {
            string_buffer_append(cbData->owner_id, data, data_len, fit);
        }
        else if (!strcmp(element_path,
                         "ListAllMyBucketsResult/Buckets/Bucket/Name")) {
            string_buffer_append(cbData->bucket_name, data, data_len, fit);
        }
        else if (!strcmp(element_path,
                  "ListAllMyBucketsResult/Buckets/Bucket/CreationDate")) {
            string_buffer_append(cbData->creationDate, data, data_len, fit);
        }
        else if (!strcmp(element_path,
                  "ListAllMyBucketsResult/Buckets/Bucket/Location")) {
            string_buffer_append(cbData->location, data, data_len, fit);
        }
        else if (!strcmp(element_path,
                  "ListAllMyBucketsResult/Buckets/Bucket/BucketType")) {
            string_buffer_append(cbData->bucketType, data, data_len, fit);
        }
    }
    else {
        if (!strcmp(element_path, "ListAllMyBucketsResult/Buckets/Bucket")) {
            time_t creationDate = parseIso8601Time(cbData->creationDate);
            int nTimeZone = getTimeZone();
            creationDate += nTimeZone * SECONDS_TO_AN_HOUR;

            obs_status status = (*(cbData->listServiceCallback))
                (cbData->owner_id, cbData->bucket_name, 
                 creationDate, cbData->location, cbData->callback_data);

            string_buffer_initialize(cbData->bucket_name);
            string_buffer_initialize(cbData->creationDate);
            string_buffer_initialize(cbData->location);
            string_buffer_initialize(cbData->bucketType);
            return status;
        }
    }
    (void) fit;
    return OBS_STATUS_OK;
}

static obs_status properties_callback
    (const obs_response_properties *responseProperties, void *callback_data)
{
    xml_callback_data *cbData = (xml_callback_data *) callback_data;
    if (cbData->responsePropertiesCallback)
    {
        return (*(cbData->responsePropertiesCallback))
            (responseProperties, cbData->callback_data);
    }
    return OBS_STATUS_OK;
}

static void complete_callback(obs_status requestStatus,const obs_error_details *s3ErrorDetails,void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    xml_callback_data *cbData = (xml_callback_data *) callback_data;

    (void)(*(cbData->responseCompleteCallback))
        (requestStatus, s3ErrorDetails, cbData->callback_data);

    simplexml_deinitialize(&(cbData->simpleXml));

    free(cbData);
    cbData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

static obs_status data_callback(int buffer_size, const char *buffer,
                             void *callback_data)
{
    xml_callback_data *cbData = (xml_callback_data *) callback_data;

    return simplexml_add(&(cbData->simpleXml), buffer, buffer_size);
}

void list_bucket(const obs_options *options, obs_list_service_handler *handler, void *callback_data)
{
	obs_use_api use_api = OBS_USE_API_S3;

	if (options->request_options.auth_switch == OBS_OBS_TYPE)
	{
		use_api = OBS_USE_API_OBS;
	}
	else if (options->request_options.auth_switch == OBS_S3_TYPE)
	{
		use_api = OBS_USE_API_S3;
	}
	
    request_params      params;
    
    COMMLOG(OBS_LOGINFO, "Enter list_bucket successfully !");

    xml_callback_data *data = (xml_callback_data *) malloc(sizeof(xml_callback_data));
    if (!data) 
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 
                                                0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc XmlCallbackData failed !");
        return;
    }
    memset_s(data, sizeof(xml_callback_data), 0, sizeof(xml_callback_data));

    simplexml_initialize(&(data->simpleXml), &xml_callback, data);

    data->responsePropertiesCallback = handler->response_handler.properties_callback;
    data->listServiceCallback = handler->listServiceCallback;
    data->responseCompleteCallback = handler->response_handler.complete_callback;
    data->callback_data = callback_data;

    string_buffer_initialize(data->owner_id);
    string_buffer_initialize(data->owner_display_name);
    string_buffer_initialize(data->bucket_name);
    string_buffer_initialize(data->creationDate);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));

    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.httpRequestType       = http_request_type_get;
    params.properties_callback   = &properties_callback;
    params.fromObsCallback       = &data_callback;
    params.complete_callback     = &complete_callback;
    params.callback_data         = data;
    params.isCheckCA             = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat    = no_need_storage_class;
    params.temp_auth             = options->temp_auth; 
	params.use_api               = use_api;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave list_bucket successfully !");
}

void list_bucket_obs(const obs_options *options, obs_list_service_obs_handler *handler, void *callback_data)
{
    request_params      params;
    obs_use_api use_api = OBS_USE_API_S3;
	
	if (options->request_options.auth_switch == OBS_NEGOTIATION_TYPE)
	{
		if (get_api_version(NULL, options->bucket_options.host_name,options->bucket_options.protocol) == 
			OBS_STATUS_OK)
		{
			use_api = OBS_USE_API_OBS;
		}
		else
		{
			use_api = OBS_USE_API_S3;
		}
	}
	else if (options->request_options.auth_switch == OBS_OBS_TYPE)
	{
		use_api = OBS_USE_API_OBS;
	}
	else if (options->request_options.auth_switch == OBS_S3_TYPE)
	{
		use_api = OBS_USE_API_S3;
	}
	
    COMMLOG(OBS_LOGINFO, "Enter list_bucket obs successfully !");
    xml_obs_callback_data *data = (xml_obs_callback_data *)malloc(sizeof(xml_obs_callback_data));
    if (!data) 
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 
                                                0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc XmlCallbackData failed !");
        return;
    }
    memset_s(data, sizeof(xml_obs_callback_data), 0, sizeof(xml_obs_callback_data));

    simplexml_initialize(&(data->simpleXml), &xml_obs_callback, data);

    data->responsePropertiesCallback = handler->response_handler.properties_callback;
    data->listServiceCallback = handler->listServiceCallback;
    data->responseCompleteCallback = handler->response_handler.complete_callback;
    data->callback_data = callback_data;

    string_buffer_initialize(data->owner_id);
    string_buffer_initialize(data->bucket_name);
    string_buffer_initialize(data->creationDate);
    string_buffer_initialize(data->location);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));

    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.httpRequestType       = http_request_type_get;
    params.properties_callback   = &properties_callback;
    params.fromObsCallback       = &data_callback;
    params.complete_callback     = &complete_callback;
    params.callback_data         = data;
    params.isCheckCA             = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat    = no_need_storage_class;
    params.temp_auth             = options->temp_auth; 
    params.use_api = use_api;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "Leave list_bucket_obs successfully !");
}

/* delete bucket */
void delete_bucket(const obs_options *options, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    COMMLOG(OBS_LOGINFO, "delete_bucket start!");
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    params.httpRequestType = http_request_type_delete;
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.properties_callback = handler->properties_callback;
    params.complete_callback   = handler->complete_callback;
    params.isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat = no_need_storage_class;
    params.temp_auth          = options->temp_auth; 
    params.callback_data      = callback_data;
    params.use_api =use_api;
    request_perform(&params);
    
    COMMLOG(OBS_LOGINFO, "delete_bucket finish!");
}


static obs_status make_list_bucket_callback(list_objects_data *lbData)
{

    obs_status iRet = OBS_STATUS_OK;

    int is_truncated = (!strcmp(lbData->is_truncated, "true") ||
                       !strcmp(lbData->is_truncated, "1")) ? 1 : 0;

    obs_list_objects_content *contents = NULL;
    int contents_count = 0;
    int i = 0;
    if (lbData->contents_count > 0)
    {
        contents = (obs_list_objects_content*)malloc(sizeof(obs_list_objects_content) * lbData->contents_count);
        if (NULL == contents)
        {
            COMMLOG(OBS_LOGERROR, "Malloc obs_list_objects_content failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(contents, sizeof(obs_list_objects_content) * lbData->contents_count, 0, sizeof(obs_list_objects_content) * lbData->contents_count);  //secure function

        contents_count = lbData->contents_count;
        for (i = 0; i < contents_count; i++) 
        {
            obs_list_objects_content *contentDest = &(contents[i]);
            one_object_content *contentSrc = &(lbData->contents[i]);
            contentDest->key = contentSrc->key;
            contentDest->last_modified = parseIso8601Time(contentSrc->last_modified);
            int nTimeZone = getTimeZone();
            contentDest->last_modified += nTimeZone * SECONDS_TO_AN_HOUR;
            contentDest->etag = contentSrc->etag;
            contentDest->size = parseUnsignedInt(contentSrc->size);
            contentDest->owner_id = contentSrc->owner_id[0] ? contentSrc->owner_id : 0;
            contentDest->owner_display_name = (contentSrc->owner_display_name[0] ? 
                            contentSrc->owner_display_name : 0);
            contentDest->storage_class = (contentSrc->storage_class[0] ? 
                            contentSrc->storage_class:0);
            contentDest->type = (contentSrc->type[0] ? contentSrc->type:0);
        }
    }

    // Make the common prefixes array
    int common_prefixes_count = lbData->common_prefixes_count;
    char **common_prefixes = NULL;
    if(common_prefixes_count > 0)
    {
        common_prefixes = (char**)malloc(sizeof(char *) * common_prefixes_count);
        if (NULL == common_prefixes)
        {
            COMMLOG(OBS_LOGERROR, "Malloc common_prefixes failed!");
            CHECK_NULL_FREE(contents);
            return OBS_STATUS_InternalError;
        }
        memset_s(common_prefixes, sizeof(char *) * common_prefixes_count, 0, sizeof(char *) * common_prefixes_count);

        for (i = 0; i < common_prefixes_count; i++) 
        {
            common_prefixes[i] = lbData->common_prefixes[i];
        }
    }

    iRet = (*(lbData->listObjectCallback))(is_truncated, lbData->next_marker,contents_count, 
                contents, common_prefixes_count,(const char **) common_prefixes, lbData->callback_data);

    CHECK_NULL_FREE(contents);
    CHECK_NULL_FREE(common_prefixes);

    return iRet;
}

static void initialize_list_objects_contents(one_object_content *contents)
{
    string_buffer_initialize(contents->key);
    string_buffer_initialize(contents->last_modified);
    string_buffer_initialize(contents->etag);
    string_buffer_initialize(contents->size);
    string_buffer_initialize(contents->owner_id);
    string_buffer_initialize(contents->owner_display_name);
    string_buffer_initialize(contents->storage_class);
}

static void initialize_list_objects_data(list_objects_data *lbData)
{
    lbData->contents_count = 0;
    initialize_list_objects_contents(lbData->contents);
    lbData->common_prefixes_count = 0;
    lbData->common_prefixes[0][0] = 0;
    lbData->commonPrefixLens[0] = 0;
}


obs_status parse_xml_list_objects(list_objects_data *lbData, const char *element_path,
                                      const char *data, int data_len)
{
    int fit;
    
    if (!strcmp(element_path, "ListBucketResult/IsTruncated")) {
        string_buffer_append(lbData->is_truncated, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/NextMarker")) {
        string_buffer_append(lbData->next_marker, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/Key")) 
    {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);

#ifdef WIN32
        char* strTmpSource = (char*)malloc(sizeof(char) * (data_len + 1));
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(strTmpSource, data_len + 1,  0, data_len + 1);
        strncpy_sec(strTmpSource, data_len+1, data, data_len);
        char* strTmpOut = UTF8_To_String(strTmpSource);
        string_buffer_append(contents->key, strTmpOut, strlen(strTmpOut), fit);
        CHECK_NULL_FREE(strTmpSource);
        CHECK_NULL_FREE(strTmpOut);
#else
        string_buffer_append(contents->key, data, data_len, fit);
#endif
    }
    else if (!strcmp(element_path,"ListBucketResult/Contents/LastModified")) 
    {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->last_modified, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/ETag")) 
    {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->etag, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/Size")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->size, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/Type")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->type, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/Owner/ID")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->owner_id, data, data_len, fit);
    }
    else if (!strcmp(element_path,"ListBucketResult/Contents/Owner/DisplayName")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->owner_display_name, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListBucketResult/Contents/StorageClass")) {
        one_object_content *contents = &(lbData->contents[lbData->contents_count]);
        string_buffer_append(contents->storage_class, data, data_len, fit);
    }
    else if (!strcmp(element_path,"ListBucketResult/CommonPrefixes/Prefix")) {
        int which = lbData->common_prefixes_count;
        lbData->commonPrefixLens[which] +=
            snprintf_sec(lbData->common_prefixes[which], sizeof(lbData->common_prefixes[which]),
                     sizeof(lbData->common_prefixes[which]) - lbData->commonPrefixLens[which] - 1,
                     "%.*s", data_len, data);
        if (lbData->commonPrefixLens[which] >= (int)sizeof(lbData->common_prefixes[which]))
        {
            COMMLOG(OBS_LOGERROR, "prefix length more than 1024.");
            return OBS_STATUS_XmlParseFailure;
        }
    }

    /* Avoid compiler error about variable set but not used */
    (void) fit;
    return OBS_STATUS_OK;
}

static obs_status list_objects_xml_callback(const char *element_path,
                                      const char *data, int data_len,
                                      void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    list_objects_data *lbData = (list_objects_data *) callback_data;

    if (data) {
        return parse_xml_list_objects(lbData, element_path, data, data_len);
    }
    
    if (!strcmp(element_path, "ListBucketResult/Contents")) {
        // Finished a Contents
        lbData->contents_count++;
        if (lbData->contents_count == MAX_CONTENTS) {
            // Make the callback
            obs_status status = make_list_bucket_callback(lbData);
            if (status != OBS_STATUS_OK) {
                return status;
            }
            initialize_list_objects_data(lbData);
        }
        else {
            // Initialize the next one
            initialize_list_objects_contents(&(lbData->contents[lbData->contents_count]));
        }
    }
    else if (!strcmp(element_path, "ListBucketResult/CommonPrefixes/Prefix")) {
        // Finished a Prefix
        lbData->common_prefixes_count++;
        if (lbData->common_prefixes_count == MAX_COMMON_PREFIXES) {
            // Make the callback
            obs_status status = make_list_bucket_callback(lbData);
            if (status != OBS_STATUS_OK) {
                return status;
            }
            initialize_list_objects_data(lbData);
        }
        else {
            // Initialize the next one
            lbData->common_prefixes[lbData->common_prefixes_count][0] = 0;
            lbData->commonPrefixLens[lbData->common_prefixes_count] = 0;
        }
    }

    return OBS_STATUS_OK;
}


static obs_status list_objects_properties_callback(const obs_response_properties *responseProperties,
                                void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    list_objects_data *lbData = (list_objects_data *) callback_data;
    if (lbData->responsePropertiesCallback)
    {
       return (*(lbData->responsePropertiesCallback))(responseProperties, lbData->callback_data); 
    }

    return OBS_STATUS_OK;
}

static obs_status list_objects_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_objects_data *lbData = (list_objects_data *) callback_data;

    return simplexml_add(&(lbData->simpleXml), buffer, buffer_size);
}


static void list_objects_complete_callback(obs_status requestStatus,
                                       const obs_error_details *obsErrorDetails,
                                       void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_objects_data *lbData = (list_objects_data*) callback_data;
    if (0 == lbData->contents_count)
    {
        COMMLOG(OBS_LOGWARN, "listObjects contents_count = %d !", lbData->contents_count);
    }
    
    // Make the callback if there is anything
    if ((lbData->contents_count || lbData->common_prefixes_count) && OBS_STATUS_OK == requestStatus) 
    {
        requestStatus = make_list_bucket_callback(lbData);
    }

    (*(lbData->responseCompleteCallback))(requestStatus, obsErrorDetails, lbData->callback_data);

    simplexml_deinitialize(&(lbData->simpleXml));

    free(lbData);
    lbData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

    return ;
}

static obs_status set_objects_query_params(const char *prefix, const char *marker, const char *delimiter, int maxkeys, 
            char* query_params)
{  
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    
    int amp = 0;
    if (delimiter) 
    {
        safe_append_status("delimiter", delimiter);
    }
       
    if (marker) 
    {
        safe_append_status("marker", marker);
    }

    if (maxkeys) 
    {
        maxkeys = maxkeys > 1000 ? 1000 : maxkeys;
        char maxKeysString[64] = {0};
        snprintf_sec(maxKeysString, sizeof(maxKeysString), _TRUNCATE,  "%d", maxkeys);
        safe_append_status("max-keys", maxKeysString);
    }

    if (prefix) 
    {
        safe_append_status("prefix", prefix);
    }

    memcpy_s(query_params, QUERY_STRING_LEN, queryParams, QUERY_STRING_LEN);

    return OBS_STATUS_OK;
}


static obs_status make_list_versions_callback(list_versions_data *lvData)
{
    int i;
    obs_status iRet = OBS_STATUS_OK;

    int is_truncated = (!strcmp(lvData->is_truncated, "true") ||
            !strcmp(lvData->is_truncated, "1")) ? 1 : 0;

    obs_list_versions *list_versions_info = (obs_list_versions*)malloc(sizeof(obs_list_versions));
    if (NULL == list_versions_info)
    {
        COMMLOG(OBS_LOGERROR, "Malloc obs_list_versions failed!");
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(list_versions_info, sizeof(obs_list_versions), 0, sizeof(obs_list_versions));

    list_versions_info->versions = (obs_version*)malloc(sizeof(obs_version) * lvData->versions_count);
    if (NULL == list_versions_info->versions)
    {
        COMMLOG(OBS_LOGERROR, "Malloc obs_version failed!");
        CHECK_NULL_FREE(list_versions_info);
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(list_versions_info->versions, sizeof(obs_version) * lvData->versions_count, 0, 
        sizeof(obs_version) * lvData->versions_count);

    list_versions_info->common_prefixes = (const char**)malloc(sizeof(char*) * lvData->common_prefixes_count);
    if (NULL == list_versions_info->common_prefixes)
    {
        COMMLOG(OBS_LOGERROR, "Malloc common_prefixes failed!");
        CHECK_NULL_FREE(list_versions_info->versions);
        CHECK_NULL_FREE(list_versions_info);
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(list_versions_info->common_prefixes, sizeof(char*) * lvData->common_prefixes_count, 0, 
        sizeof(char*) * lvData->common_prefixes_count);

    list_versions_info->bucket_name    = lvData->bucket_name;
    list_versions_info->prefix        = lvData->prefix;
    list_versions_info->key_marker     = lvData->key_marker;
    list_versions_info->delimiter     = lvData->delimiter;
    list_versions_info->max_keys       = lvData->max_keys;

    list_versions_info->versions_count = lvData->versions_count;
    for (i=0; i<list_versions_info->versions_count; i++) 
    {
        list_bucket_versions *versionSrc      = &(lvData->versions[i]);
        list_versions_info->versions[i].key       = versionSrc->key;
        list_versions_info->versions[i].version_id = versionSrc->version_id;
        list_versions_info->versions[i].is_latest  = versionSrc->is_latest;

        list_versions_info->versions[i].last_modified = parseIso8601Time(versionSrc->last_modified);
        int nTimeZone = getTimeZone();
        list_versions_info->versions[i].last_modified += nTimeZone * SECONDS_TO_AN_HOUR;

        list_versions_info->versions[i].etag = versionSrc->etag;
        list_versions_info->versions[i].size = parseUnsignedInt(versionSrc->size);
        list_versions_info->versions[i].owner_id = versionSrc->owner_id[0] ? versionSrc->owner_id : 0;
        list_versions_info->versions[i].owner_display_name = versionSrc->owner_display_name[0] ?
            versionSrc->owner_display_name : 0;
        list_versions_info->versions[i].storage_class = versionSrc->storage_class_value[0] ? 
            versionSrc->storage_class_value : 0;
        list_versions_info->versions[i].is_delete = versionSrc->is_delete;
    }

    list_versions_info->common_prefixes_count = lvData->common_prefixes_count;
    for (i=0; i< list_versions_info->common_prefixes_count; i++)
    {
        list_versions_info->common_prefixes[i] = lvData->common_prefixes[i].prefix;
    }

    iRet = (*(lvData->listVersionsCallback))(is_truncated, lvData->next_key_marker, 
            lvData->next_versionId_marker, list_versions_info, lvData->callback_data);

    CHECK_NULL_FREE(list_versions_info->common_prefixes);
    CHECK_NULL_FREE(list_versions_info->versions);
    CHECK_NULL_FREE(list_versions_info);

    return iRet;
}

static void initialize_list_versions(list_bucket_versions *versions)
{
    string_buffer_initialize(versions->key);
    string_buffer_initialize(versions->version_id);
    string_buffer_initialize(versions->is_latest);
    string_buffer_initialize(versions->last_modified);
    string_buffer_initialize(versions->etag);
    string_buffer_initialize(versions->size);
    string_buffer_initialize(versions->owner_id);
    string_buffer_initialize(versions->owner_display_name);
    string_buffer_initialize(versions->storage_class_value);
    string_buffer_initialize(versions->is_delete);
}

static void initialize_list_common_prefixes(list_common_prefixes* common_prefixes)
{
    string_buffer_initialize(common_prefixes->prefix);
}

static void initialize_list_versions_data(list_versions_data *lvData)
{
    lvData->versions_count = 0;
    initialize_list_versions(lvData->versions);

    lvData->common_prefixes_count     = 0;
    initialize_list_common_prefixes(lvData->common_prefixes);
}

obs_status parse_xml_list_versions(list_versions_data *version_data, const char *element_path,
                                      const char *data, int data_len)
{
    int fit;

    list_bucket_versions *versions = &(version_data->versions[version_data->versions_count]);
    if (!strcmp(element_path, "ListVersionsResult/NextKeyMarker")){
        string_buffer_append(version_data->next_key_marker, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/NextVersionIdMarker")){
        string_buffer_append(version_data->next_versionId_marker, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/IsTruncated")) {
        string_buffer_append(version_data->is_truncated, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Name")) {
        string_buffer_append(version_data->bucket_name, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Prefix")) {
        string_buffer_append(version_data->prefix, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/KeyMarker")) {
        string_buffer_append(version_data->key_marker, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Delimiter")) {
        string_buffer_append(version_data->delimiter, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/MaxKeys")) {
        string_buffer_append(version_data->max_keys, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Version/Key") || 
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/Key")) 
    {
#ifdef WIN32
        char* strTmpSource = (char*)malloc(sizeof(char) * (data_len + 1));
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(strTmpSource, sizeof(char) * (data_len + 1), 0, data_len + 1);
        strncpy_sec(strTmpSource, data_len+1, data, data_len);
        char* strTmpOut = UTF8_To_String(strTmpSource);
        string_buffer_append(versions->key, strTmpOut, strlen(strTmpOut), fit);
        CHECK_NULL_FREE(strTmpSource);
        CHECK_NULL_FREE(strTmpOut);
#else
        string_buffer_append(versions->key, data, data_len, fit);
#endif
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Version/VersionId")) 
    {
        string_buffer_append(versions->version_id, data, data_len, fit);
        string_buffer_append(versions->is_delete, "false", 5, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/DeleteMarker/VersionId")) 
    {
        string_buffer_append(versions->version_id, data, data_len, fit);
        string_buffer_append(versions->is_delete, "true", 4, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Version/IsLatest") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/IsLatest")) {
        string_buffer_append(versions->is_latest, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Version/LastModified") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/LastModified")) {
        string_buffer_append(versions->last_modified, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Version/ETag")) {
        string_buffer_append(versions->etag, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Version/Size")) {
        string_buffer_append(versions->size, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Version/Owner/ID") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/Owner/ID")) {
        string_buffer_append(versions->owner_id, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Version/Owner/DisplayName") ||
        !strcmp(element_path, "ListVersionsResult/DeleteMarker/Owner/DisplayName")) {
        string_buffer_append (versions->owner_display_name, data, data_len, fit);
    }
    else_if (!strcmp(element_path, "ListVersionsResult/Version/StorageClass")) {
        string_buffer_append(versions->storage_class_value, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "ListVersionsResult/CommonPrefixes/Prefix")) {
        string_buffer_append(version_data->common_prefixes[version_data->common_prefixes_count].prefix, 
            data, data_len, fit);
    }

    (void) fit;

    return OBS_STATUS_OK;
}


static obs_status list_versions_xml_callback(const char *element_path,
                                      const char *data, int data_len,
                                      void *callback_data)
{
    list_versions_data *version_data = (list_versions_data *) callback_data;

    if (data) 
    {  
        return parse_xml_list_versions(version_data, element_path, data, data_len);

    }

    if (!strcmp(element_path, "ListVersionsResult/Version") || 
        !strcmp(element_path, "ListVersionsResult/DeleteMarker")) 
    {
        // Finished a Version
        version_data->versions_count++;
        if (version_data->versions_count == MAX_VERSIONS) {
            // Make the callback
            obs_status status = make_list_versions_callback(version_data);
            if (OBS_STATUS_OK != status) {
                return status;
            }
            initialize_list_versions_data(version_data);
        }
        else {
            // Initialize the next one
            initialize_list_versions(&(version_data->versions[version_data->versions_count]));
        }
    }
    
    if (!strcmp(element_path, "ListVersionsResult/CommonPrefixes")) {
        // Finished a commonPrefix
        version_data->common_prefixes_count++;
        if (version_data->common_prefixes_count == MAX_VERSION_COMMON_PREFIXES)
        {
            // Make the callback
            obs_status status = make_list_versions_callback(version_data);
            if (OBS_STATUS_OK != status)
            {
                return status;
            }
            initialize_list_versions_data(version_data);
        }
        else
        {
            initialize_list_common_prefixes(&version_data->common_prefixes[version_data->common_prefixes_count]);
        }
    }
    
    return OBS_STATUS_OK;
}


static obs_status list_versions_properties_callback(
    const obs_response_properties *responseProperties, void *callback_data)
{
    list_versions_data *version_data = (list_versions_data *) callback_data;
    if (version_data->responsePropertiesCallback)
    {
        return (*(version_data->responsePropertiesCallback))(responseProperties, 
            version_data->callback_data);
    }

    return OBS_STATUS_OK;
}

static obs_status list_versions_data_callback(int buffer_size, const char *buffer,
                    void *callback_data)
{
    list_versions_data *version_data = (list_versions_data *) callback_data;

    return simplexml_add(&(version_data->simpleXml), buffer, buffer_size);
}


static void list_versions_complete_callback(obs_status requestStatus, 
        const obs_error_details *obsErrorDetails, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_versions_data *version_data = (list_versions_data *) callback_data;
    if (version_data->versions_count) 
    {
        obs_status callback_result = make_list_versions_callback(version_data);
        if (OBS_STATUS_OK != callback_result)
        {
            COMMLOG(OBS_LOGERROR, "make_list_versions_callback failed (%d)!", callback_result);
        }
    }

    (*(version_data->responseCompleteCallback))(requestStatus, obsErrorDetails, version_data->callback_data);

    simplexml_deinitialize(&(version_data->simpleXml));

    free(version_data);
    version_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


void list_bucket_objects(const obs_options *options, const char *prefix, const char *marker, const char *delimiter, 
            int maxkeys, obs_list_objects_handler *handler, void *callback_data)
{
    request_params params;
    char queryParams[QUERY_STRING_LEN + 1] = {0};
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "list bucket objects start!");

    obs_status ret_status = set_objects_query_params(prefix, marker, delimiter, maxkeys, queryParams);
    if (OBS_STATUS_OK != ret_status)
    {
        (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "set_query_params return %d !", ret_status);
        return;
    }
    
    list_objects_data *data = (list_objects_data *) malloc(sizeof(list_objects_data));
    if (!data) 
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc ListBucketData failed !");
        return;
    }
    memset_s(data, sizeof(list_objects_data), 0, sizeof(list_objects_data));

    simplexml_initialize(&(data->simpleXml), &list_objects_xml_callback, data);

    data->responsePropertiesCallback = handler->response_handler.properties_callback;
    data->listObjectCallback         = handler->list_Objects_callback;
    data->responseCompleteCallback   = handler->response_handler.complete_callback;
    data->callback_data               = callback_data;

    string_buffer_initialize(data->is_truncated);
    string_buffer_initialize(data->next_marker);
    initialize_list_objects_data(data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_get;
    params.properties_callback     = &list_objects_properties_callback;
    params.fromObsCallback        = &list_objects_data_callback;
    params.complete_callback       = &list_objects_complete_callback;
    params.callback_data           = data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.queryParams            = queryParams[0] ? queryParams : 0;
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "list bucket objects finish !");
}

/************************************list_versions**********************************************/
static obs_status set_versions_query_params(const char *prefix, const char *key_marker, const char *delimiter, 
            int maxkeys, const char *version_id_marker, char* query_params)
{  
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    
    int amp = 0;
    if (delimiter) 
    {
        safe_append_status("delimiter", delimiter);
    }
       
    if (key_marker) 
    {
        safe_append_status("key-marker", key_marker);
    }

    if (maxkeys) 
    {
        maxkeys = maxkeys > 1000 ? 1000 : maxkeys;
        char maxKeysString[64] = {0};
        snprintf_sec(maxKeysString, sizeof(maxKeysString), _TRUNCATE,  "%d", maxkeys);
        safe_append_status("max-keys", maxKeysString);
    }

    if (prefix) 
    {
        safe_append_status("prefix", prefix);
    }

    if (version_id_marker)
    {
        safe_append_status("version-id-marker",version_id_marker);
    }

    memcpy_s(query_params, QUERY_STRING_LEN, queryParams, QUERY_STRING_LEN);

    return OBS_STATUS_OK;
}


void list_versions(const obs_options *options, const char *prefix, const char *key_marker, const char *delimiter, 
           int maxkeys, const char *version_id_marker, obs_list_versions_handler *handler, void *callback_data)
{
    request_params params;
    char queryParams[QUERY_STRING_LEN + 1] = {0};
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "list_bucket_versions start !");
    if(version_id_marker && !strlen(version_id_marker))
    {
        COMMLOG(OBS_LOGERROR, "version_id_marker is \"\"!");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidParameter, 0, callback_data);
        return;
    }

    obs_status ret_status = set_versions_query_params(prefix, key_marker, delimiter, maxkeys,
                                        version_id_marker, queryParams);
    if (OBS_STATUS_OK != ret_status)
    {
        (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "set_query_params return %d !", ret_status);
        return;
    }

    list_versions_data *lvData = (list_versions_data *) malloc(sizeof(list_versions_data));
    if (!lvData) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc list_versions_data failed !");
        return;
    }
    memset_s(lvData, sizeof(list_versions_data), 0, sizeof(list_versions_data));
    
    simplexml_initialize(&(lvData->simpleXml), &list_versions_xml_callback, lvData);
    
    lvData->responsePropertiesCallback  = handler->response_handler.properties_callback;
    lvData->listVersionsCallback        = handler->list_versions_callback;
    lvData->responseCompleteCallback    = handler->response_handler.complete_callback;
    lvData->callback_data               = callback_data;

    string_buffer_initialize(lvData->is_truncated);
    string_buffer_initialize(lvData->next_key_marker);
    string_buffer_initialize(lvData->next_versionId_marker);
    initialize_list_versions_data(lvData);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
             sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
             sizeof(obs_http_request_option));

    params.httpRequestType         = http_request_type_get;
    params.properties_callback     = &list_versions_properties_callback;
    params.fromObsCallback         = &list_versions_data_callback;
    params.complete_callback       = &list_versions_complete_callback;
    params.callback_data           = lvData;
    params.isCheckCA               = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat      = no_need_storage_class;
    params.queryParams             = queryParams[0] ? queryParams : 0;  
    params.temp_auth               = options->temp_auth;
    params.subResource             = "versions";
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "list_bucket_versions finish!");
}

/*****************************************get_bucket_storageinfo********************************/

static obs_status get_bucket_storageInfo_xml_callback(const char *element_path, const char *data, 
                int data_len, void *callback_data)
{
    get_bucket_storageInfo_data *gbsiData = (get_bucket_storageInfo_data *) callback_data;

    int fit;
    if (data)
    {
        if(!strcmp(element_path, "GetBucketStorageInfoResult/Size")) {
            string_buffer_append(gbsiData->size, data, data_len, fit);
        }
        else if(!strcmp(element_path, "GetBucketStorageInfoResult/ObjectNumber")){
            string_buffer_append(gbsiData->objectnumber, data, data_len, fit);
        }
    }

    (void) fit;
    return OBS_STATUS_OK;
}

static obs_status get_bucket_storageInfo_data_callback(int buffer_size, const char *buffer,
                                       void *callback_data)
{
    get_bucket_storageInfo_data *gbsiData = (get_bucket_storageInfo_data *) callback_data;
    return simplexml_add(&(gbsiData->simpleXml), buffer, buffer_size);
}

static void get_bucket_storageInfo_complete_callback(obs_status requestStatus,
                                       const obs_error_details *obsErrorDetails,
                                       void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_storageInfo_data *gbsiData = (get_bucket_storageInfo_data *) callback_data;

    snprintf_sec(gbsiData->capacity_return, sizeof(gbsiData->size), gbsiData->capacity_length, "%s",
                gbsiData->size);
    snprintf_sec(gbsiData->object_number_return, sizeof(gbsiData->objectnumber), 
                gbsiData->object_number_length, "%s", gbsiData->objectnumber);

    (void)(*(gbsiData->complete_callback))(requestStatus, obsErrorDetails, gbsiData->callback_data);

    simplexml_deinitialize(&(gbsiData->simpleXml));
    free(gbsiData);
    gbsiData = NULL;
    
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
    return;
}

static obs_status get_bucket_storageInfo_properties_callback(
        const obs_response_properties *response_properties, void *callback_data)
{
    get_bucket_storageInfo_data *gbsiData = (get_bucket_storageInfo_data *) callback_data;
    if (gbsiData->properties_callback)
    {
        return (*(gbsiData->properties_callback))(response_properties, gbsiData->callback_data);
    }
    return OBS_STATUS_OK;
}


void get_bucket_storage_info(const obs_options *options, int capacity_length, char *capacity,
                    int object_number_length, char *object_number,
                    obs_response_handler *handler, void *callback_data)
{
    request_params              params;
    get_bucket_storageInfo_data *gbsiData;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get_bucket_storage_info start.");
    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    if(NULL == capacity || NULL == object_number)
    {
        COMMLOG(OBS_LOGERROR, "capacity(%p) or object_number(%p) is invalid.", capacity, object_number);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }
    
    gbsiData = (get_bucket_storageInfo_data *) malloc(sizeof(get_bucket_storageInfo_data));
    if (!gbsiData) {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "Malloc GetBucketStorageInfoData failed.");
        return;
    }
    memset_s(gbsiData, sizeof(get_bucket_storageInfo_data), 0, sizeof(get_bucket_storageInfo_data));

    simplexml_initialize(&(gbsiData->simpleXml), &get_bucket_storageInfo_xml_callback, gbsiData);
    gbsiData->complete_callback             = handler->complete_callback;
    gbsiData->capacity_return               = capacity;
    gbsiData->capacity_length               = capacity_length;
    gbsiData->object_number_return          = object_number;
    gbsiData->object_number_length          = object_number_length;
    gbsiData->callback_data                 = callback_data;
    gbsiData->properties_callback           = handler->properties_callback;
    string_buffer_initialize(gbsiData->size);
    string_buffer_initialize(gbsiData->objectnumber);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
             sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
             sizeof(obs_http_request_option));

    params.httpRequestType         = http_request_type_get;
    params.properties_callback     = &get_bucket_storageInfo_properties_callback;
    params.fromObsCallback         = &get_bucket_storageInfo_data_callback;
    params.complete_callback       = &get_bucket_storageInfo_complete_callback;
    params.callback_data           = gbsiData;
    params.isCheckCA               = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat      = no_need_storage_class;
    params.subResource             = "storageinfo";
    params.temp_auth               = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_storage_info finish.");
}


/**************************list_multipart_uploads************************************************/
static obs_status set_multipart_query_params(const char *prefix, const char *marker, const char *delimiter,
        const char* uploadid_marke, int max_uploads, char* query_params)
{  
    string_buffer(queryParams, QUERY_STRING_LEN);
    string_buffer_initialize(queryParams);
    
    int amp = 0;
    if (delimiter) 
    {
        safe_append_status("delimiter", delimiter);
    }
       
    if (marker) 
    {
        safe_append_status("key-marker", marker);
    }

    if (max_uploads) 
    {
        max_uploads = max_uploads > 1000 ? 1000 : max_uploads;
        char max_upload_string[64] = {0};
        snprintf_sec(max_upload_string, sizeof(max_upload_string), _TRUNCATE,  "%d", max_uploads); 
        safe_append_status("max-uploads", max_upload_string);
    }

    if (prefix) 
    {
        safe_append_status("prefix", prefix);
    }

    if (uploadid_marke)
    {
        safe_append_status("upload-id-marke", uploadid_marke);
    }

    memcpy_s(query_params, QUERY_STRING_LEN, queryParams, QUERY_STRING_LEN);

    return OBS_STATUS_OK;
}


static void initialize_list_multipart_uploads(multipart_upload_info *uploads)
{
    string_buffer_initialize(uploads->key);
    string_buffer_initialize(uploads->upload_id);
    string_buffer_initialize(uploads->initiator_id);
    string_buffer_initialize(uploads->initiator_display_name);
    string_buffer_initialize(uploads->owner_id);
    string_buffer_initialize(uploads->owner_display_name); 
    string_buffer_initialize(uploads->storage_class);
    string_buffer_initialize(uploads->initiated);
}

static void initialize_list_multipart_uploads_data(list_multipart_uploads_data *lmu_data)
{
    string_buffer_initialize(lmu_data->is_truncated);
    string_buffer_initialize(lmu_data->next_marker);
    string_buffer_initialize(lmu_data->next_uploadId_marker);
    lmu_data->uploads_count= 0;
    initialize_list_multipart_uploads(lmu_data->uploads);
    initialize_list_common_prefixes(lmu_data->common_prefixes);
}


static obs_status make_list_multipart_uploads_callback(list_multipart_uploads_data *lmu_data)
{
    int             i = 0;
    int             uploads_count = 0;
    obs_status      iRet = OBS_STATUS_OK;
    obs_list_multipart_upload *uploads = NULL;
    int is_truncated = (!strcmp(lmu_data->is_truncated, "true") ||
                       !strcmp(lmu_data->is_truncated, "1")) ? 1 : 0;
    
    if(lmu_data->uploads_count > 0)
    {
        uploads = (obs_list_multipart_upload*)malloc(sizeof(obs_list_multipart_upload) 
                    * lmu_data->uploads_count);
        if (NULL == uploads)
        {
            COMMLOG(OBS_LOGERROR, "malloc obs_list_multipart_upload failed!");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(uploads, sizeof(obs_list_multipart_upload) * lmu_data->uploads_count, 0, 
                sizeof(obs_list_multipart_upload) * lmu_data->uploads_count);

        uploads_count = lmu_data->uploads_count;
        for (i = 0; i < uploads_count; i++) {
            obs_list_multipart_upload   *upload_dest = &(uploads[i]);
            multipart_upload_info       *uploadSrc   = &(lmu_data->uploads[i]);
            upload_dest->key        = uploadSrc->key;
            upload_dest->upload_id   = uploadSrc->upload_id;
            upload_dest->owner_id    = uploadSrc->owner_id[0] ? uploadSrc->owner_id : 0;
            upload_dest->owner_display_name = (uploadSrc->owner_display_name[0] ?
                            uploadSrc->owner_display_name : 0);
            upload_dest->initiator_id = uploadSrc->initiator_id[0] ?uploadSrc->initiator_id : 0;
            upload_dest->initiator_display_name = (uploadSrc->initiator_display_name[0] ?
                            uploadSrc->initiator_display_name : 0);
            upload_dest->storage_class = uploadSrc->storage_class;
            upload_dest->initiated    = parseIso8601Time(uploadSrc->initiated);
            int nTimeZone = getTimeZone();
            upload_dest->initiated += nTimeZone * SECONDS_TO_AN_HOUR;
        }
    }

    int common_prefixesCount = lmu_data->common_prefixes_count;
    char **common_prefixes = NULL;
    if(common_prefixesCount>0)
    {
        common_prefixes = (char**)malloc(sizeof(char *) * common_prefixesCount);
        if (NULL == common_prefixes)
        {
            COMMLOG(OBS_LOGERROR, "Malloc common_prefixes failed!");
            CHECK_NULL_FREE(uploads);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(common_prefixes, sizeof(char *) * common_prefixesCount, 0, 
                    sizeof(char *) * common_prefixesCount);

        for (i = 0; i < common_prefixesCount; i++) {
            common_prefixes[i] = lmu_data->common_prefixes[i].prefix;
        }
    }

    iRet = (*(lmu_data->list_mulpu_callback))(is_truncated, lmu_data->next_marker, 
        lmu_data->next_uploadId_marker, uploads_count, uploads, common_prefixesCount, 
        (const char **)common_prefixes, lmu_data->callback_data);

    CHECK_NULL_FREE(uploads);
    CHECK_NULL_FREE(common_prefixes);
    return iRet;
}

obs_status parse_xml_list_multipart_uploads(list_multipart_uploads_data *lmu_data, 
            const char *element_path, const char *data, int data_len)
{
    int fit;
    multipart_upload_info  *uploads  = &(lmu_data->uploads[lmu_data->uploads_count]);
    
    if (!strcmp(element_path, "ListMultipartUploadsResult/IsTruncated")) {
        string_buffer_append(lmu_data->is_truncated, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/NextKeyMarker")) {
        string_buffer_append(lmu_data->next_marker, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/NextUploadIdMarker")) {
        string_buffer_append(lmu_data->next_uploadId_marker, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Key")) {
#ifdef WIN32
        char* strTmpSource = (char*)malloc(sizeof(char) * (data_len + 1));
        if (NULL == strTmpSource)
        {
            COMMLOG(OBS_LOGERROR, "Malloc strTmpSource failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(strTmpSource, sizeof(char) * (data_len + 1), 0, data_len + 1);
        strncpy_sec(strTmpSource, data_len+1, data, data_len);
        char* strTmpOut = UTF8_To_String(strTmpSource);
        string_buffer_append(uploads->key, strTmpOut, strlen(strTmpOut), fit);
        CHECK_NULL_FREE(strTmpSource);
        CHECK_NULL_FREE(strTmpOut);
#else
        string_buffer_append(uploads->key, data, data_len, fit);
#endif
    }
    else if (!strcmp(element_path,"ListMultipartUploadsResult/Upload/UploadId")) {
        string_buffer_append(uploads->upload_id, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Initiator/ID")) {
        string_buffer_append(uploads->initiator_id, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Initiator/DisplayName")) {
        string_buffer_append(uploads->initiator_display_name, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/StorageClass")) {
        string_buffer_append(uploads->storage_class, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Initiated")) {
        string_buffer_append(uploads->initiated, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Owner/ID")) {
        string_buffer_append(uploads->owner_id, data, data_len, fit);
    }
    else if (!strcmp(element_path, "ListMultipartUploadsResult/Upload/Owner/DisplayName")) {
        string_buffer_append(uploads->owner_display_name, data, data_len, fit);
    }
    else if (!strcmp(element_path,"ListMultipartUploadsResult/CommonPrefixes/Prefix")) {
        string_buffer_append(lmu_data->common_prefixes[lmu_data->common_prefixes_count].prefix, 
                    data, data_len, fit);
    }

    (void) fit;
    return OBS_STATUS_OK;
}

static obs_status list_multipart_uploads_xml_callback(const char *element_path,
                                    const char *data, int data_len, void *callback_data)
{
    list_multipart_uploads_data *lmu_data = (list_multipart_uploads_data *)callback_data;
    
    if (data) {
        return parse_xml_list_multipart_uploads(lmu_data, element_path, data, data_len);
    }

    if (!strcmp(element_path, "ListMultipartUploadsResult/Upload")) {
        // Finished a Contents
        lmu_data->uploads_count++;
        if (lmu_data->uploads_count == MAX_UPLOADS) {
            // Make the callback
            obs_status status = make_list_multipart_uploads_callback(lmu_data);
            if (OBS_STATUS_OK != status) {
                return status;
            }
            initialize_list_multipart_uploads_data(lmu_data);
        }
        else {
            // Initialize the next one
            initialize_list_multipart_uploads(&(lmu_data->uploads[lmu_data->uploads_count]));
        }
    }
    else if (!strcmp(element_path,"ListMultipartUploadsResult/CommonPrefixes/Prefix")) {
        // Finished a Prefix
        lmu_data->common_prefixes_count++;
        if (lmu_data->common_prefixes_count == MAX_COMMON_PREFIXES) {
            // Make the callback
            obs_status status = make_list_multipart_uploads_callback(lmu_data);
            if (OBS_STATUS_OK != status) {
                return status;
            }
            initialize_list_multipart_uploads_data(lmu_data);
        }
        else {
            // Initialize the next one
            initialize_list_common_prefixes(
                    &lmu_data->common_prefixes[lmu_data->common_prefixes_count]);
        }
    }

    return OBS_STATUS_OK;
}

static obs_status list_multipart_uploads_properties_callback
    (const obs_response_properties *response_properties, void *callback_data)
{
    list_multipart_uploads_data *lmu_data = (list_multipart_uploads_data *) callback_data;
    if (lmu_data->response_properties_callback)
    {
        return (*(lmu_data->response_properties_callback))(response_properties, 
                    lmu_data->callback_data);
    }
    
    return OBS_STATUS_OK;
}

static obs_status list_multipart_uploads_data_callback(int buffer_size, const char *buffer, 
        void *callback_data)
{
    list_multipart_uploads_data *lmu_data = (list_multipart_uploads_data *) callback_data;

    return simplexml_add(&(lmu_data->simpleXml), buffer, buffer_size);
}

static void list_multipart_uploads_complete_callback(obs_status requestStatus,
                                       const obs_error_details *obsErrorDetails,
                                       void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    list_multipart_uploads_data *lmu_data = (list_multipart_uploads_data *) callback_data;

    if (lmu_data->uploads_count || lmu_data->common_prefixes_count) {

        obs_status callbackResult = make_list_multipart_uploads_callback(lmu_data);
        if (OBS_STATUS_OK != callbackResult)
        {
            COMMLOG(OBS_LOGERROR, "make_list_multipart_uploads_callback failed!");
        }
    }

    (*(lmu_data->response_complete_callback))(requestStatus, obsErrorDetails, 
                        lmu_data->callback_data);

    simplexml_deinitialize(&(lmu_data->simpleXml));

    free(lmu_data);
    lmu_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
    return;
}


void list_multipart_uploads(const obs_options *options, const char *prefix, const char *marker, const char *delimiter,
        const char* uploadid_marke, int max_uploads, obs_list_multipart_uploads_handler *handler, 
        void *callback_data)
{
    request_params params;
    char queryParams[QUERY_STRING_LEN + 1] = {0};
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "list_multipart_uploads start !");

    obs_status ret_status = set_multipart_query_params(prefix, marker, delimiter, uploadid_marke,
                                        max_uploads, queryParams);
    if (OBS_STATUS_OK != ret_status)
    {
        (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "set_multipart_query_params return %d !", ret_status);
        return;
    }

    list_multipart_uploads_data *lmu_data = 
        (list_multipart_uploads_data *)malloc(sizeof(list_multipart_uploads_data));
    if (!lmu_data) {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "Malloc ListMultipartUploadsData failed !");
        return;
    }
    memset_s(lmu_data, sizeof(list_multipart_uploads_data), 0, sizeof(list_multipart_uploads_data));
    
    simplexml_initialize(&(lmu_data->simpleXml), &list_multipart_uploads_xml_callback, lmu_data);
    lmu_data->response_properties_callback  = handler->response_handler.properties_callback;
    lmu_data->list_mulpu_callback           = handler->list_mulpu_callback;
    lmu_data->response_complete_callback    = handler->response_handler.complete_callback;
    lmu_data->callback_data                 = callback_data;    
    initialize_list_multipart_uploads_data(lmu_data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
             sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
             sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_get;
    params.properties_callback     = &list_multipart_uploads_properties_callback;
    params.fromObsCallback        = &list_multipart_uploads_data_callback;
    params.complete_callback       = &list_multipart_uploads_complete_callback;
    params.callback_data           = lmu_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.queryParams            = queryParams[0] ? queryParams : 0;
    params.subResource            = "uploads";
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "list_multipart_uploads finish!");
}

obs_status init_set_bucket_quota_cbdata(uint64_t storage_quota, update_bucket_common_data **data)
{
    update_bucket_common_data *quota_data = (update_bucket_common_data *) malloc(sizeof(update_bucket_common_data));
    if (!quota_data) 
    {
        *data = NULL;
        return OBS_STATUS_OutOfMemory;
    }   
    memset_s(quota_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));


    quota_data->docLen = snprintf_sec(quota_data->doc, sizeof(quota_data->doc), _TRUNCATE,
                            "<Quota><StorageQuota>%lu</StorageQuota></Quota>", storage_quota);
    if(quota_data->docLen < 0)
    {
        *data = NULL;
        return OBS_STATUS_InternalError;
    }
    
    quota_data->docBytesWritten = 0;
    *data = quota_data;
    
    return OBS_STATUS_OK;
}

void set_bucket_quota(const obs_options *options, uint64_t storage_quota, 
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    obs_put_properties  properties;
    update_bucket_common_data  *quota_data = NULL;
    obs_status status = OBS_STATUS_OK;
    
    COMMLOG(OBS_LOGINFO, "set bucket quota start!");

    status = init_set_bucket_quota_cbdata(storage_quota, &quota_data);
    if (status != OBS_STATUS_OK) 
    {
        COMMLOG(OBS_LOGERROR, "Malloc update_bucket_common_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    quota_data->complete_callback = handler->complete_callback;
    quota_data->callback_data = callback_data;
    quota_data->properties_callback = handler->properties_callback;
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.put_properties         = &properties;
    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &update_bucket_common_properties_callback;
    params.toObsCallback          = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = quota_data->docLen;
    params.complete_callback      = &update_bucket_common_complete_callback;
    params.callback_data          = quota_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "quota";
    params.temp_auth              = options->temp_auth; 
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket quota finish!");
}

static obs_status get_bucket_quotaxml_callback(const char *element_path,const char *data,
                                            int data_len, void *callback_data)
{
    get_bucket_common_data *bucket_quota_data = (get_bucket_common_data *) callback_data;
    int fit;
    if (data && !strcmp(element_path, "Quota/StorageQuota")) {
        string_buffer_append(bucket_quota_data->common_data, data, data_len, fit);
    }

    (void) fit;

    return OBS_STATUS_OK;
}


static obs_status get_bucket_quota_properties_callback(const obs_response_properties *response_properties,
                                                     void *callback_data)
{
    get_bucket_common_data *bucket_quota_data = (get_bucket_common_data *) callback_data;
    if (bucket_quota_data->responsePropertiesCallback)
    {
        return (*(bucket_quota_data->responsePropertiesCallback))
            (response_properties, bucket_quota_data->callback_data);
    }
    return OBS_STATUS_OK;
}


static obs_status get_bucket_quotadata_callback(int buffer_size, const char *buffer,
                                       void *callback_data)
{
    get_bucket_common_data *gbqData = (get_bucket_common_data *) callback_data;
    return simplexml_add(&(gbqData->simpleXml), buffer, buffer_size);
}

static void get_bucket_quota_complete_callback(obs_status status,
                                         const obs_error_details *error_details,
                                         void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_common_data *bucket_quota_data = (get_bucket_common_data *) callback_data;
    if (OBS_STATUS_OK == status)
    {
        *(bucket_quota_data->ul_return) = atol(bucket_quota_data->common_data);
    }

    (void)(*(bucket_quota_data->responseCompleteCallback))(status, error_details, 
                            bucket_quota_data->callback_data);

    simplexml_deinitialize(&(bucket_quota_data->simpleXml));

    free(bucket_quota_data);
    bucket_quota_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


/*****************************************************************************
*   Prototype    : get_bucket_quota
*   Description  : 
*   Input        : obs_options *options           
*                  int storagequota_return_size   
*                  char *storagequota_return      
*                  obs_response_handler *handler  
*                  void *callback_data            
*   Output       : None
*   Return Value : void
*   Calls        : 
*   Called By    : 
*
*   History:
* 
*       1.  Date         : 2018/5/31
*           Author       : clw
*           Modification : Created function
*
*****************************************************************************/
void get_bucket_quota(const obs_options *options, uint64_t *storagequota_return,
                    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get bucket quota start!");
    
    get_bucket_common_data *bucket_quota_data = (get_bucket_common_data *) malloc(sizeof(get_bucket_common_data));
    if (!bucket_quota_data) 
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc get_bucket_quota_data failed !");
        return;
    }
    memset_s(bucket_quota_data, sizeof(get_bucket_common_data), 0, sizeof(get_bucket_common_data));

    simplexml_initialize(&(bucket_quota_data->simpleXml), &get_bucket_quotaxml_callback, bucket_quota_data);
    
    bucket_quota_data->responsePropertiesCallback = handler->properties_callback;
    bucket_quota_data->responseCompleteCallback = handler->complete_callback;
    bucket_quota_data->callback_data = callback_data;
    bucket_quota_data->ul_return     = storagequota_return;

    string_buffer_initialize(bucket_quota_data->common_data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.httpRequestType         = http_request_type_get;
    params.properties_callback     = &get_bucket_quota_properties_callback;
    params.fromObsCallback         = &get_bucket_quotadata_callback;
    params.complete_callback       = &get_bucket_quota_complete_callback;
    params.callback_data           = bucket_quota_data;
    params.isCheckCA               = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat      = no_need_storage_class;
    params.subResource             = "quota";
    params.temp_auth               = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket quota finish!");
}

void set_bucket_policy(const obs_options *options, const char *policy, obs_response_handler *handler, 
            void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set bucket policy start!");
    update_bucket_common_data *policy_data = (update_bucket_common_data*) malloc(sizeof(update_bucket_common_data));
    if (!policy_data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc set_policy_data failed !");
        return;
    }
    memset_s(policy_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));
    policy_data->properties_callback    = handler->properties_callback;
    policy_data->complete_callback      = handler->complete_callback;
    policy_data->callback_data          = callback_data;
    policy_data->docBytesWritten        = 0;
    policy_data->docLen                 = 
            snprintf_sec(policy_data->doc, sizeof(policy_data->doc), _TRUNCATE, "%s",policy);
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &update_bucket_common_properties_callback;
    params.toObsCallback          = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = policy_data->docLen;
    params.complete_callback      = &update_bucket_common_complete_callback;
    params.callback_data          = policy_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "policy";
    params.temp_auth              = options->temp_auth; 
    params.use_api=use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "set bucket policy finish!");    
}

obs_status get_bucket_policy_properties_callback(const obs_response_properties *response_properties,
                                                void *callback_data)
{
    get_bucket_policy_data *policy_data = (get_bucket_policy_data *) callback_data;

    if (policy_data->responsePropertiesCallback)
    {
        return (*(policy_data->responsePropertiesCallback))
            (response_properties, policy_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_policy_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    get_bucket_policy_data *policy_data = (get_bucket_policy_data *) callback_data;
    snprintf_sec(policy_data->policy, sizeof(policy_data->policy),buffer_size+1, "%s",buffer);

    return OBS_STATUS_OK;
}

void get_bucket_policy_complete_callback(obs_status status,
                                       const obs_error_details *error_details,
                                       void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_policy_data *policy_data = (get_bucket_policy_data *)callback_data;
    snprintf_sec(policy_data->policyReturn, sizeof(policy_data->policy),
             policy_data->policyReturnSize, "%s",
             policy_data->policy);

    (void)(*(policy_data->responseCompleteCallback))(status, error_details, policy_data->callback_data);

    free(policy_data);
    policy_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void get_bucket_policy(const obs_options *options, int policy_return_size, 
                      char *policy_return, obs_response_handler *handler, 
                      void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get bucket policy start!");
    get_bucket_policy_data *policy_data = (get_bucket_policy_data*) malloc(sizeof(get_bucket_policy_data));
    if (!policy_data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc get_policy_data failed !");
        return;
    }
    memset_s(policy_data, sizeof(get_bucket_policy_data), 0, sizeof(get_bucket_policy_data));
    policy_data->responsePropertiesCallback = handler->properties_callback;
    policy_data->responseCompleteCallback = handler->complete_callback;
    policy_data->callback_data = callback_data;
    policy_data->policyReturn = policy_return;
    policy_data->policyReturnSize = policy_return_size;
    string_buffer_initialize(policy_data->policy);

    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.httpRequestType      = http_request_type_get;
    params.properties_callback   = &get_bucket_policy_properties_callback;
    params.fromObsCallback      = &get_bucket_policy_data_callback;
    params.complete_callback     = &get_bucket_policy_complete_callback;
    params.callback_data         = policy_data;
    params.isCheckCA            = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.subResource          = "policy";
    params.temp_auth            = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket policy finish!");
}

void delete_bucket_policy(const obs_options *options, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "delete bucket policy start!");
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.httpRequestType      = http_request_type_delete;
    params.properties_callback   = handler->properties_callback;
    params.complete_callback     = handler->complete_callback;
    params.isCheckCA            = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.subResource          = "policy";
    params.temp_auth            = options->temp_auth; 
    params.callback_data        = callback_data;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "delete bucket policy finish!");
}

obs_status init_set_bucket_version_data(const char *version_status, update_bucket_common_data **out_data)
{
    int tmplen = 0;
    int mark = 0;
    char *replace_status = 0;

    update_bucket_common_data *version_data = (update_bucket_common_data *) malloc(sizeof(update_bucket_common_data));
    if (!version_data) 
    {
        return OBS_STATUS_OutOfMemory;
    }   
    memset_s(version_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));
    
    version_data->docLen = snprintf_sec(version_data->doc, sizeof(version_data->doc), _TRUNCATE, "<VersioningConfiguration>");
    if (version_data->docLen < 0)
    {
        return OBS_STATUS_InternalError;
    }
    mark = pcre_replace(version_status,&replace_status);  
    if(mark)
    {
        free(replace_status);
        replace_status = NULL;
    }
    
    tmplen = snprintf_sec((version_data->doc) + (version_data->docLen), sizeof((version_data->doc)) - (version_data->docLen), 
                    _TRUNCATE, "<Status>%s</Status></VersioningConfiguration>",mark ? replace_status : version_status);
    
    version_data->docLen += tmplen;
    if (tmplen < 0)
    {
        return OBS_STATUS_InternalError;
    }

    version_data->docBytesWritten = 0;
    *out_data = version_data;

    return OBS_STATUS_OK;
}

void set_bucket_version_configuration(const obs_options *options, const char *version_status, 
                                    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    update_bucket_common_data  *version_data = NULL;
    obs_status status = OBS_STATUS_OK;
    
    COMMLOG(OBS_LOGINFO, "set bucket version configuration start!");
    status = init_set_bucket_version_data(version_status, &version_data);
    if (status != OBS_STATUS_OK) 
    {
        COMMLOG(OBS_LOGERROR, "init set version data failed!");
        (void)(*(handler->complete_callback))(status, 0, 0);
        return;
    }
    version_data->complete_callback = handler->complete_callback;
    version_data->properties_callback = handler->properties_callback;
    version_data->callback_data = callback_data;
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &update_bucket_common_properties_callback;
    params.toObsCallback          = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = version_data->docLen;
    params.complete_callback      = &update_bucket_common_complete_callback;
    params.callback_data          = version_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "versioning";
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket version configuration finish!");
}

obs_status get_bucket_version_xml_callback(const char *element_path,const char *data, 
                                                    int data_len, void *callback_data)
{
    int fit;
    
    get_bucket_common_data *version_data = (get_bucket_common_data *) callback_data;
    if (data)
    {
        if(!strcmp(element_path, "VersioningConfiguration/Status")) {
            string_buffer_append(version_data->common_data, data, data_len, fit);
        }
    }
    
    (void) fit;
    return OBS_STATUS_OK;
}

obs_status get_bucket_version_properties_callback(const obs_response_properties *response_properties,
                                            void *callback_data)
{
    get_bucket_common_data *version_data = (get_bucket_common_data *) callback_data;
    if (version_data->responsePropertiesCallback)
    {
        return (*(version_data->responsePropertiesCallback))
            (response_properties, version_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_version_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    get_bucket_common_data *version_data = (get_bucket_common_data *) callback_data;

    return simplexml_add(&(version_data->simpleXml), buffer, buffer_size);
}

void get_bucket_version_complete_callback(obs_status status,
                                       const obs_error_details *error_details,
                                       void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_common_data *version_data = (get_bucket_common_data *) callback_data;

    snprintf_sec(version_data->common_data_return, sizeof(version_data->common_data),
             version_data->common_data_return_size, "%s", version_data->common_data);
    
    (void)(*(version_data->responseCompleteCallback))
        (status, error_details, version_data->callback_data);

    simplexml_deinitialize(&(version_data->simpleXml));

    free(version_data);
    version_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void get_bucket_version_configuration(const obs_options *options, int status_return_size, 
                      char *status_return, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get bucket version start!");
    get_bucket_common_data *version_data = (get_bucket_common_data*) malloc(sizeof(get_bucket_common_data));
    if (!version_data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc version data failed !");
        return;
    }
    memset_s(version_data, sizeof(get_bucket_common_data), 0, sizeof(get_bucket_common_data));
    
    simplexml_initialize(&(version_data->simpleXml), &get_bucket_version_xml_callback, version_data);
    
    version_data->responsePropertiesCallback = handler->properties_callback;
    version_data->responseCompleteCallback = handler->complete_callback;
    version_data->callback_data = callback_data;
    version_data->common_data_return = status_return;
    version_data->common_data_return_size = status_return_size;
    string_buffer_initialize(version_data->common_data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.httpRequestType      = http_request_type_get;
    params.properties_callback   = &get_bucket_version_properties_callback;
    params.fromObsCallback      = &get_bucket_version_data_callback;
    params.complete_callback     = &get_bucket_version_complete_callback;
    params.callback_data         = version_data;
    params.isCheckCA            = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.subResource          = "versioning";
    params.temp_auth            = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket version finish!");
}
obs_status append_xml_document(int *xml_document_len_return, char *xml_document,
                           int xml_document_buffer_size,char *fmt, ...)
{                                          
    va_list args;
    va_start(args, fmt);

    int size = vsnprintf_sec(&(xml_document[*xml_document_len_return]), 
                    xml_document_buffer_size - *xml_document_len_return,
                    xml_document_buffer_size - *xml_document_len_return - 1,
                    fmt, args); 
    va_end(args);

    if(size < 0)
    {
        return OBS_STATUS_InternalError;
    }
    
    *xml_document_len_return += size;
    if (*xml_document_len_return >= xml_document_buffer_size) {
        return OBS_STATUS_XmlDocumentTooLarge;              
    }
    return OBS_STATUS_OK;
} 

obs_status generate_storage_class_xml_document(obs_storage_class storage_class_policy,
                                       int *xml_document_len_return, char *xml_document,
                                       int xml_document_buffer_size,  obs_use_api use_api)
{
    *xml_document_len_return = 0;

    if (use_api == OBS_USE_API_S3) {
        char *storage_class_list[] = {"STANDARD","STANDARD_IA","GLACIER",""};

        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
                        "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
                        "%s", "<StoragePolicy xmlns=\"http://s3.amazonaws.com/doc/2015-06-30/\"><DefaultStorageClass>");
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size, 
                        "%s",storage_class_list[storage_class_policy]);
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size, 
                        "%s", "</DefaultStorageClass></StoragePolicy>");
    } else {
        char *storage_class_list[] = {"STANDARD","WARM","COLD",""};

        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
                        "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
                        "%s", "<StorageClass xmlns=\"http://obs.myhwclouds.com/doc/2015-06-30/\">");
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size, 
                        "%s",storage_class_list[storage_class_policy]);
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size, 
                        "%s", "</StorageClass>");
    }

    return OBS_STATUS_OK;
}

obs_status generate_tagging_xml_document(obs_name_value * tagging_list, unsigned int tag_number,
                int *xml_document_len_return, char *xml_document, int xml_document_buffer_size)
{
    *xml_document_len_return = 0;
    unsigned int i = 0;
    char *key_utf8 = NULL;
    char *value_utf8 = NULL;

    append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
    append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
        "%s", "<Tagging xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\"><TagSet>");

    for (i = 0; i < tag_number; i++)
    {
        key_utf8 = string_To_UTF8(tagging_list[i].name);
        value_utf8 = string_To_UTF8(tagging_list[i].value);
        append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size,
            "<Tag><Key>%s</Key><Value>%s</Value></Tag>", key_utf8, value_utf8);
        CHECK_NULL_FREE(key_utf8);
        CHECK_NULL_FREE(value_utf8);
    }

    append_xml_document(xml_document_len_return, xml_document, xml_document_buffer_size, 
        "%s", "</TagSet></Tagging>");

    return OBS_STATUS_OK;
}
obs_status set_common_properties_callback(const obs_response_properties *response_properties, 
    void *callback_data)
{
     set_common_data *common_data = (set_common_data *) callback_data;

    if (common_data->response_properties_callback)
    {
        return (*(common_data->response_properties_callback))
            (response_properties, common_data->callback_data);
    }
    return OBS_STATUS_OK;
}

void set_common_complete_callback(obs_status status, 
                const obs_error_details *error_details, void *callback_data)
{
    set_common_data *common_data = NULL;

    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    common_data = (set_common_data *) callback_data;
    (void)(*(common_data->response_complete_callback))
        (status, error_details, common_data->callback_data);
    free(common_data);
    common_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

int set_common_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    int remaining      = 0;
    int ret            = 0;
    set_common_data *common_data = NULL;

    common_data = (set_common_data *) callback_data;
    remaining = (common_data->xml_document_len - 
        common_data->xml_document_bytes_written);

    ret = buffer_size > remaining ? remaining : buffer_size;
    if (!ret)
    {
        return 0;
    }

	errno_t err = EOK; 
    err = memcpy_s(buffer, buffer_size, &(common_data->xml_document
        [common_data->xml_document_bytes_written]), ret);
	if (err != EOK)
	{
		COMMLOG(OBS_LOGWARN, "set_common_data_callback: memcpy_s failed!\n");
		return 0;
	}
		
    common_data->xml_document_bytes_written += ret;

    return ret;
}

void set_bucket_storage_class_policy(const obs_options *options,
                            obs_storage_class storage_class_policy, 
                            obs_response_handler *handler,
                            void *callback_data)
{
    request_params params;
    obs_put_properties  properties;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set bucket storage policy start!");
    if(storage_class_policy >= OBS_STORAGE_CLASS_BUTT)
    {
        COMMLOG(OBS_LOGERROR, "storage_class_policy invalid!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }

    set_common_data *common_data = (set_common_data*)malloc(sizeof(set_common_data));
    if (!common_data) 
    {
        COMMLOG(OBS_LOGERROR, "Malloc set_stoarge_policy_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    memset_s(common_data, sizeof(set_common_data), 0, sizeof(set_common_data));
    
    obs_status status = generate_storage_class_xml_document(storage_class_policy,
                                &(common_data->xml_document_len), 
                                common_data->xml_document,
                                sizeof(common_data->xml_document), use_api);
    if (status != OBS_STATUS_OK)
    {
        free(common_data);
        common_data = NULL;
        (void)(*(handler->complete_callback))(status, 0, 0);
        return;
    }
    common_data->response_properties_callback = handler->properties_callback;
    common_data->response_complete_callback = handler->complete_callback;
    common_data->xml_document_bytes_written = 0;
    common_data->callback_data = callback_data;
   
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = OBS_CANNED_ACL_PRIVATE;
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.put_properties         = &properties;
    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &set_common_properties_callback;
    params.toObsCallback          = &set_common_data_callback;
    params.toObsCallbackTotalSize = common_data->xml_document_len;
    params.complete_callback      = &set_common_complete_callback;
    params.callback_data          = common_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    if (use_api == OBS_USE_API_S3) {
        params.subResource            = "storagePolicy";
    } else {
        params.subResource            = "storageClass";
    }
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket storage policy finish!");
}

obs_status get_bucket_storage_policy_xml_callback(const char *element_path, 
                    const char *data, int data_len, void *callback_data)
{
    get_bucket_storage_class_policy_data *storage_class_data = 
        (get_bucket_storage_class_policy_data *) callback_data;

    int fit = 0;

    if (data)
    {
        if (storage_class_data->use_api == OBS_USE_API_S3) {
            if (!strcmp(element_path, "StoragePolicy/DefaultStorageClass"))
            {  
                string_buffer_append(storage_class_data->storage_class_policy, data, data_len, fit);       
            }
        } else {
            if (!strcmp(element_path, "StorageClass"))
            {  
                string_buffer_append(storage_class_data->storage_class_policy, data, data_len, fit);       
            }
        }
    }
    
    (void) fit;
    return OBS_STATUS_OK;
}

obs_status get_bucket_storage_class_properties_callback(
        const obs_response_properties *response_properties, void *callback_data)
{
    get_bucket_storage_class_policy_data *storage_class_data = 
        (get_bucket_storage_class_policy_data *) callback_data;
    if (storage_class_data->response_properties_callback)
    {
        return (*(storage_class_data->response_properties_callback))
            (response_properties, storage_class_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_storage_class_data_callback(int buffer_size, const char *buffer,
                                       void *callback_data)
{
    get_bucket_storage_class_policy_data *storage_class_data = 
        (get_bucket_storage_class_policy_data *) callback_data;

    return simplexml_add(&(storage_class_data->simpleXml), buffer, buffer_size);
}

void get_bucket_storage_class_complete_callback(obs_status status,
    const obs_error_details *error_details, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_storage_class_policy_data *storage_class_data = 
        (get_bucket_storage_class_policy_data *) callback_data;

    obs_status callback_result = (*(storage_class_data->response_bucket_sorage_policy_callback))
        (storage_class_data->storage_class_policy,storage_class_data->callback_data);
    if (callback_result != OBS_STATUS_OK)
    {
        COMMLOG(OBS_LOGERROR, "make_storage_policy_callback failed!");
    }

    (void)(*(storage_class_data->response_complete_callback))(status, error_details,
        storage_class_data->callback_data);
    simplexml_deinitialize(&(storage_class_data->simpleXml));

    if(storage_class_data)
    {
        free(storage_class_data);
        storage_class_data = NULL;
    }
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void get_bucket_storage_class_policy(const obs_options *options, 
                    obs_get_bucket_storage_class_handler *handler, 
                    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get bucket storage class policy start!");
    get_bucket_storage_class_policy_data *storage_class_data = 
        (get_bucket_storage_class_policy_data*) malloc(sizeof(get_bucket_storage_class_policy_data));
    if (!storage_class_data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc get_storage_class_data failed !");
        return;
    }
    memset_s(storage_class_data, sizeof(get_bucket_storage_class_policy_data), 
        0, sizeof(get_bucket_storage_class_policy_data));
    storage_class_data->use_api = use_api;
    simplexml_initialize(&(storage_class_data->simpleXml), &get_bucket_storage_policy_xml_callback, storage_class_data);
    storage_class_data->response_properties_callback = handler->response_handler.properties_callback;
    storage_class_data->response_complete_callback = handler->response_handler.complete_callback;
    storage_class_data->response_bucket_sorage_policy_callback = handler->get_bucket_sorage_class_callback;
    storage_class_data->callback_data = callback_data;
    memset(storage_class_data->storage_class_policy,0,sizeof(storage_class_data->storage_class_policy));

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.httpRequestType       = http_request_type_get;
    params.properties_callback   = &get_bucket_storage_class_properties_callback;
    params.fromObsCallback       = &get_bucket_storage_class_data_callback;
    params.complete_callback     = &get_bucket_storage_class_complete_callback;
    params.callback_data         = storage_class_data;
    params.isCheckCA             = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat    = no_need_storage_class;
    if (use_api == OBS_USE_API_S3) {
        params.subResource           = "storagePolicy";
    } else {
        params.subResource           = "storageClass";
    }
    params.temp_auth             = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket storage class policy finish!");
}

void set_bucket_tagging(const obs_options *options, obs_name_value * tagging_list, 
        unsigned int number, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_put_properties  properties;  
    unsigned char doc_md5[MD5_LEN] = {0};
    char base64_md5[BASE64_MD5_LEN] = {0};
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set bucket tagging start!");
    
    set_common_data *tagging_data = (set_common_data*)malloc(sizeof(set_common_data));
    if (!tagging_data) 
    {
        COMMLOG(OBS_LOGERROR, "Malloc set bucket tagging_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    memset_s(tagging_data, sizeof(set_common_data), 0, sizeof(set_common_data));

    obs_status status = generate_tagging_xml_document(tagging_list, number,
                                &(tagging_data->xml_document_len), tagging_data->xml_document,
                                sizeof(tagging_data->xml_document));
    if (status != OBS_STATUS_OK)
    {
        free(tagging_data);
        tagging_data = NULL;
        (void)(*(handler->complete_callback))(status, 0, 0);
        COMMLOG(OBS_LOGERROR, "tagging: generate storage_class_xml_document failed !");
        return;
    }

    tagging_data->response_properties_callback = handler->properties_callback;
    tagging_data->response_complete_callback = handler->complete_callback;
    tagging_data->xml_document_bytes_written = 0;
    tagging_data->callback_data = callback_data;
   
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    MD5((unsigned char*)tagging_data->xml_document, (size_t)tagging_data->xml_document_len, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), base64_md5);
    properties.md5 = base64_md5;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.put_properties         = &properties;
    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &set_common_properties_callback;
    params.toObsCallback          = &set_common_data_callback;
    params.toObsCallbackTotalSize = tagging_data->xml_document_len;
    params.complete_callback      = &set_common_complete_callback;
    params.callback_data          = tagging_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "tagging";
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket tagging finish!");
}

obs_status get_bucket_tagging_xml_callback(const char *element_path, 
                const char *data, int data_len, void *callback_data)
{
    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *) callback_data;
    int fit = 0;

    if (data)
    {
        if(tagging_data->tagging_count < MAX_NUM_TAGGING)
        {
            if (!strcmp(element_path, "Tagging/TagSet/Tag/Key"))
            {
                string_buffer_append(tagging_data->tagging_list[tagging_data->tagging_count].key, 
                    data, data_len, fit);       
            }
            else if (!strcmp(element_path, "Tagging/TagSet/Tag/Value"))
            {
                string_buffer_append(tagging_data->tagging_list[tagging_data->tagging_count].value, 
                    data, data_len, fit);
            }
        }
    }
    else
    {
         if(!strcmp(element_path, "Tagging/TagSet/Tag"))
         {
              tagging_data->tagging_count++;
              if(tagging_data->tagging_count > MAX_NUM_TAGGING)
              {
                  COMMLOG(OBS_LOGERROR, "etag number exceed the max[10]!");
                  return OBS_STATUS_InternalError;
              }
              else if(tagging_data->tagging_count == MAX_NUM_TAGGING)
              {
                  COMMLOG(OBS_LOGINFO, "already get the max[10] tags!");
              }
              else
              {
                  memset(&tagging_data->tagging_list[tagging_data->tagging_count],0,sizeof(tagging_kv));
              }
         }
    }

    (void) fit;

    return OBS_STATUS_OK;
}

obs_status get_bucket_tagging_properties_callback(
                    const obs_response_properties *response_properties,
                    void *callback_data)
{
    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *) callback_data;
    if (tagging_data->response_properties_callback)
    {
        return (*(tagging_data->response_properties_callback))
            (response_properties, tagging_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status make_list_tagging_callback(get_bucket_tagging_data *tagging_data)
{
    obs_status status = OBS_STATUS_OK;
    obs_name_value *tagging_list = NULL;
    int tagging_count = 0;
    int i = 0;
    if (tagging_data->tagging_count > 0)
    {
        tagging_list = (obs_name_value*)malloc(sizeof(obs_name_value) * tagging_data->tagging_count);
        if (NULL == tagging_list)
        {
            COMMLOG(OBS_LOGERROR, "malloc tagging_list failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(tagging_list, sizeof(obs_name_value) * tagging_data->tagging_count,
                0, sizeof(obs_name_value) * tagging_data->tagging_count);

        tagging_count = tagging_data->tagging_count;
        for (i = 0; i < tagging_count; i++) 
        {
            obs_name_value *content_dest = &(tagging_list[i]);
            tagging_kv *content_src = &(tagging_data->tagging_list[i]);
            content_dest->name = content_src->key;
            content_dest->value = content_src->value;
        }
    }

    if (tagging_data->response_tagging_list_callback)
    {
        status = (*(tagging_data->response_tagging_list_callback))
            (tagging_count, tagging_list,tagging_data->callback_data);
    }
    CHECK_NULL_FREE(tagging_list);

    return status;
}

void get_bucket_tagging_complete_callback(obs_status status,
                                       const obs_error_details *error_details,
                                       void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *) callback_data;

    if (tagging_data->tagging_count) 
    {
        obs_status result = make_list_tagging_callback(tagging_data);
        if (result != OBS_STATUS_OK)
        {
            COMMLOG(OBS_LOGERROR, "make_list_tagging_callback failed!");
        }
    }
    else if (tagging_data->response_tagging_list_callback)
    {
        (*(tagging_data->response_tagging_list_callback))(0, NULL,tagging_data->callback_data);
    }

    (void)(*(tagging_data->response_complete_callback))(status, error_details, 
    tagging_data->callback_data);
    simplexml_deinitialize(&(tagging_data->simpleXml));

    free(tagging_data);
    tagging_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

obs_status get_bucket_tagging_data_callback(int buffer_size, const char *buffer,
                                       void *callback_data)
{
    get_bucket_tagging_data *tagging_data = (get_bucket_tagging_data *) callback_data;
    return simplexml_add(&(tagging_data->simpleXml), buffer, buffer_size);
}


void get_bucket_tagging(const obs_options *options, 
                    obs_get_bucket_tagging_handler *handler, 
                    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get bucket tagging start!");
    get_bucket_tagging_data *tagging_data = 
        (get_bucket_tagging_data*) malloc(sizeof(get_bucket_tagging_data));
    if (!tagging_data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc get_storage_class_data failed !");
        return;
    }
    memset_s(tagging_data, sizeof(get_bucket_tagging_data), 
        0, sizeof(get_bucket_tagging_data));
    
    simplexml_initialize(&(tagging_data->simpleXml), &get_bucket_tagging_xml_callback, tagging_data);
    tagging_data->response_properties_callback = handler->response_handler.properties_callback;
    tagging_data->response_complete_callback = handler->response_handler.complete_callback;
    tagging_data->response_tagging_list_callback = handler->get_bucket_tagging_callback;
    tagging_data->callback_data = callback_data; 

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.httpRequestType      = http_request_type_get;
    params.properties_callback   = &get_bucket_tagging_properties_callback;
    params.fromObsCallback      = &get_bucket_tagging_data_callback;
    params.complete_callback     = &get_bucket_tagging_complete_callback;
    params.callback_data         = tagging_data;
    params.isCheckCA            = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.subResource          = "tagging";
    params.temp_auth            = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket tagging finish!");
}

void delete_bucket_tagging(const obs_options *options, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "delete bucket tagging start!");
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.httpRequestType       = http_request_type_delete;
    params.properties_callback   = handler->properties_callback;
    params.complete_callback     = handler->complete_callback;
    params.isCheckCA             = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat    = no_need_storage_class;
    params.subResource           = "tagging";
    params.temp_auth             = options->temp_auth;
    params.callback_data         = callback_data;
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "delete bucket tagging finish!");
}

/****************************set_bucket_lifecycle_configuration*******************************/
obs_status set_lifecycle_request_xml(set_lifecycle_data *sblcData, 
    obs_lifecycle_conf* bucket_lifecycle_conf, unsigned int blcc_number,obs_use_api use_api)
{
    
    unsigned int i = 0;
    unsigned int j = 0;
    int is_true = 0;
    char **pp_storage_class = NULL;

    pp_storage_class = use_api == OBS_USE_API_S3 ? g_storage_class_s3 : g_storage_class_obs;

    (void)add_xml_element(sblcData->doc,&sblcData->docLen,"LifecycleConfiguration",
            NULL,NOT_NEED_FORMALIZE,ADD_HEAD_ONLY);
    for (i = 0; i < blcc_number; ++i)
    {   
        (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Rule",NULL,NOT_NEED_FORMALIZE,ADD_HEAD_ONLY);

        //add id, prefix, status
        (void)add_xml_element(sblcData->doc,&sblcData->docLen,"ID",bucket_lifecycle_conf[i].id,NEED_FORMALIZE,ADD_NAME_CONTENT);
        (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Prefix",bucket_lifecycle_conf[i].prefix,NEED_FORMALIZE,ADD_NAME_CONTENT);
        (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Status",bucket_lifecycle_conf[i].status,NEED_FORMALIZE,ADD_NAME_CONTENT);        

        is_true = (NULL != bucket_lifecycle_conf[i].days || NULL != bucket_lifecycle_conf[i].date);
        if (is_true)
        {
            //add expiration
            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Expiration",NULL,NOT_NEED_FORMALIZE,ADD_HEAD_ONLY);

            if (bucket_lifecycle_conf[i].days) {          
                (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Days",bucket_lifecycle_conf[i].days,
                    NEED_FORMALIZE,ADD_NAME_CONTENT);    
            }
            if(bucket_lifecycle_conf[i].date)
            {
                char date_Iso8601[50] = {0};
                changeTimeFormat(bucket_lifecycle_conf[i].date, date_Iso8601);
                (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Date",date_Iso8601,NEED_FORMALIZE,ADD_NAME_CONTENT);
            }
            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Expiration",NULL,NOT_NEED_FORMALIZE,ADD_TAIL_ONLY);
        }

        //add transition
        for(j=0; j<bucket_lifecycle_conf[i].transition_num; j++)
        {
            obs_storage_class tempStorageClass = bucket_lifecycle_conf[i].transition[j].storage_class;
            is_true = ((bucket_lifecycle_conf[i].transition[j].days == NULL) && 
                                    (bucket_lifecycle_conf[i].transition[j].date == NULL));
            if(is_true)
            {
                COMMLOG(OBS_LOGERROR, "date and days are both NULL for transition No %d!",j);
                break;
            }

            is_true = ((tempStorageClass != OBS_STORAGE_CLASS_STANDARD_IA) && 
                            (tempStorageClass != OBS_STORAGE_CLASS_GLACIER));
            if(is_true)
            {
                COMMLOG(OBS_LOGERROR, "storage_class[%d] for transition No %d,only glacier and standard-1a are valid !",
                                tempStorageClass,j);
                break;
            }

            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Transition",NULL,NOT_NEED_FORMALIZE,ADD_HEAD_ONLY);

            if (bucket_lifecycle_conf[i].transition[j].days)
            {           
                (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Days", bucket_lifecycle_conf[i].transition[j].days, NEED_FORMALIZE,ADD_NAME_CONTENT);  
            }
            if(bucket_lifecycle_conf[i].transition[j].date)
            {
                char date_Iso8601[50] = {0};
                changeTimeFormat(bucket_lifecycle_conf[i].transition[j].date, date_Iso8601);
                (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Date",date_Iso8601,NEED_FORMALIZE,ADD_NAME_CONTENT);
            }  
            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"StorageClass",pp_storage_class[tempStorageClass],NEED_FORMALIZE,ADD_NAME_CONTENT);

            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Transition",NULL,NOT_NEED_FORMALIZE,ADD_TAIL_ONLY);
        }

        //add non version transition
        for(j=0; j< bucket_lifecycle_conf[i].noncurrent_version_transition_num; j++)
        {
            obs_storage_class tempStorageClass = bucket_lifecycle_conf[i].noncurrent_version_transition[j].storage_class;
            if(bucket_lifecycle_conf[i].noncurrent_version_transition[j].noncurrent_version_days == NULL) 
            {
                COMMLOG(OBS_LOGERROR, "days is NULL for nonCurrentVersionTranstion No %d!", j);
                break;
            }

            is_true = ((tempStorageClass != OBS_STORAGE_CLASS_STANDARD_IA) && 
                                        (tempStorageClass != OBS_STORAGE_CLASS_GLACIER));
            if(is_true)
            {
                COMMLOG(OBS_LOGERROR, "storage_class[%d] for transition No %d,only glacier and standard-1a are valid !",
                                tempStorageClass,j);
                break;
            }
                                 

            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"NoncurrentVersionTransition",NULL,NOT_NEED_FORMALIZE,ADD_HEAD_ONLY);

            if (bucket_lifecycle_conf[i].noncurrent_version_transition[j].noncurrent_version_days)
            {           
                (void)add_xml_element(sblcData->doc,&sblcData->docLen,"NoncurrentDays",
                bucket_lifecycle_conf[i].noncurrent_version_transition[j].noncurrent_version_days,NEED_FORMALIZE,ADD_NAME_CONTENT);   
            }

            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"StorageClass",pp_storage_class[tempStorageClass],NEED_FORMALIZE,ADD_NAME_CONTENT);

            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"NoncurrentVersionTransition",NULL,NOT_NEED_FORMALIZE,ADD_TAIL_ONLY);
        }
        
        //add non current version expiration
        if (bucket_lifecycle_conf[i].noncurrent_version_days) 
        {
            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"NoncurrentVersionExpiration",NULL,NOT_NEED_FORMALIZE,ADD_HEAD_ONLY);
            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"NoncurrentDays",
            bucket_lifecycle_conf[i].noncurrent_version_days,NEED_FORMALIZE,ADD_NAME_CONTENT);
            (void)add_xml_element(sblcData->doc,&sblcData->docLen,"NoncurrentVersionExpiration",NULL,NOT_NEED_FORMALIZE,ADD_TAIL_ONLY);
        }

        (void)add_xml_element(sblcData->doc,&sblcData->docLen,"Rule",NULL,NOT_NEED_FORMALIZE,ADD_TAIL_ONLY);
    }
    
    (void)add_xml_element(sblcData->doc,&sblcData->docLen,"LifecycleConfiguration",
            NULL,NOT_NEED_FORMALIZE,ADD_TAIL_ONLY);
    return OBS_STATUS_OK;
}

static int set_lifecycle_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    set_lifecycle_data *sblcData = (set_lifecycle_data *)callback_data;

    if (!sblcData->docLen) {
        return 0;
    }

    int remaining = (sblcData->docLen - sblcData->docBytesWritten);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;
    if (!toCopy) {
        return 0;
    }

	errno_t err = EOK;  
	err = memcpy_s(buffer, buffer_size, &(sblcData->doc[sblcData->docBytesWritten]), toCopy); 
    if (err != EOK)
    {
		COMMLOG(OBS_LOGWARN, "set_lifecycle_data_callback: memcpy_s failed!\n");
		return 0;
    }
	
    sblcData->docBytesWritten += toCopy;
    return toCopy;
}

static obs_status set_lifecycle_properties_callback(const obs_response_properties *response_properties, 
        void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_lifecycle_data *sblcData = (set_lifecycle_data *)callback_data;
    if (sblcData->response_properties_callback)
    {
        return (*(sblcData->response_properties_callback))(response_properties, 
                sblcData->callback_data);
    }
    return OBS_STATUS_OK;
}


static void set_lifecycle_complete_callback(obs_status requestStatus,
                                         const obs_error_details *obs_error_info,
                                         void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_lifecycle_data *sblcData = (set_lifecycle_data *)callback_data;

    (void)(*(sblcData->response_complete_callback))(requestStatus, obs_error_info, sblcData->callback_data);

    free(sblcData);
    sblcData = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


static set_lifecycle_data* init_set_lifecycle_data( obs_lifecycle_conf* bucket_lifecycle_conf, 
    unsigned int blcc_number, obs_response_handler *handler, void *callback_data,obs_use_api use_api)
{
    unsigned char doc_md5[16];
    set_lifecycle_data *sblcData = NULL;
    
    sblcData = (set_lifecycle_data *) malloc(sizeof(set_lifecycle_data));
    if (!sblcData) 
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "Malloc set_lifecycle_data failed.");
        return NULL;
    }
    memset_s(sblcData, sizeof(set_lifecycle_data), 0, sizeof(set_lifecycle_data));

    sblcData->response_complete_callback     = handler->complete_callback;
    sblcData->response_properties_callback   = handler->properties_callback;
    sblcData->callback_data                  = callback_data;
    sblcData->docLen                         = 0;
     

    obs_status ret_status = set_lifecycle_request_xml(sblcData, bucket_lifecycle_conf, blcc_number,use_api);
    if (OBS_STATUS_OK != ret_status || sblcData->docLen < 0)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        free(sblcData);
        sblcData = NULL;
        return NULL;
    }
    COMMLOG(OBS_LOGERROR, "sblcData-doc: %s.", sblcData->doc);
    sblcData->docBytesWritten = 0;
    MD5((unsigned char *)sblcData->doc, (size_t)sblcData->docLen, doc_md5); 
    base64Encode(doc_md5, sizeof(doc_md5), sblcData->doc_md5);

    return sblcData;
}

void set_bucket_lifecycle_configuration(const obs_options *options, 
           obs_lifecycle_conf* bucket_lifecycle_conf, unsigned int blcc_number, 
           obs_response_handler *handler, void *callback_data)
{
    request_params     params;
    obs_put_properties put_properties;
    set_lifecycle_data *sblcData     = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set_bucket_lifecycle_configuration start !");
    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    
    if(NULL == bucket_lifecycle_conf || 0 == blcc_number)
    {
        COMMLOG(OBS_LOGERROR, "bucket_lifecycle_conf(%p) or blcc_number(%d) is invalid.", 
                    bucket_lifecycle_conf, blcc_number);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }

    sblcData = init_set_lifecycle_data(bucket_lifecycle_conf, blcc_number, handler, callback_data,use_api);
    if (!sblcData) 
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc set_lifecycle_data failed.");
        return;
    }
    
   
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
             sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
             sizeof(obs_http_request_option));

    memset_s(&put_properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    put_properties.md5 = sblcData->doc_md5;
    put_properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &set_lifecycle_properties_callback;
    params.toObsCallback          = &set_lifecycle_data_callback;
    params.complete_callback      = &set_lifecycle_complete_callback;
    params.toObsCallbackTotalSize = sblcData->docLen;
    params.callback_data          = sblcData;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "lifecycle";
    params.put_properties         = &put_properties;
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_lifecycle_configuration finish.");
}


/***********************get_bucket_lifecycle_configuration****************************************/
obs_status parse_xml_get_lifecycle(get_lifecycle_config_data *gblcDataEx, 
                const char *element_path,const char *data, int data_len)
{
    int fit;
    int nIndex = gblcDataEx->blcc_number - 1;
    int transitionIndex = gblcDataEx->blcc_data[nIndex]->transition_num;
    int nonCurrentVersionTransitionIndex = gblcDataEx->blcc_data[nIndex]->noncurrent_version_transition_num;
    
    if(!strcmp(element_path, "LifecycleConfiguration/Rule/ID")) {
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->id, data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/Prefix")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->prefix, data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/Status")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->status, data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/Expiration/Date")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->date, data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/Expiration/Days")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->days, data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/NoncurrentVersionExpiration/NoncurrentDays")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->nonCurrentVerionDays, data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/Transition/Days")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrTransitionData[transitionIndex].days, data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/Transition/Date")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrTransitionData[transitionIndex].date, data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/Transition/StorageClass")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrTransitionData[transitionIndex].storage_class, data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/NoncurrentVersionTransition/NoncurrentDays")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrNonCurrentVersionTransitionData[nonCurrentVersionTransitionIndex].days, 
                    data, data_len, fit);
    }
    else if(!strcmp(element_path, "LifecycleConfiguration/Rule/NoncurrentVersionTransition/StorageClass")){
        string_buffer_append(gblcDataEx->blcc_data[nIndex]->arrNonCurrentVersionTransitionData[nonCurrentVersionTransitionIndex].storage_class, 
                    data, data_len, fit);
    }
    
    (void)fit;
    return OBS_STATUS_OK;
}

static obs_status get_lifecycle_config_xml_callback(const char *element_path,const char *data, 
                    int data_len, void *callback_data)
{
    get_lifecycle_config_data *gblcDataEx = (get_lifecycle_config_data *) callback_data;
    int nIndex = gblcDataEx->blcc_number - 1;

    if (data)
    {
        return parse_xml_get_lifecycle( gblcDataEx, element_path, data, data_len);
    }
    
    if(!strcmp(element_path, "LifecycleConfiguration/Rule"))
    {
        lifecycle_conf_data* blcc_data = (lifecycle_conf_data*)malloc(sizeof(lifecycle_conf_data));
        if (!blcc_data)
        {
            COMMLOG(OBS_LOGERROR, "malloc lifecycle_conf_data failed !");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(blcc_data, sizeof(lifecycle_conf_data), 0, sizeof(lifecycle_conf_data)); 
        gblcDataEx->blcc_data[gblcDataEx->blcc_number] = blcc_data;
        gblcDataEx->blcc_number++;
    }

    if(!strcmp(element_path, "LifecycleConfiguration/Rule/Transition"))
    {
        gblcDataEx->blcc_data[nIndex]->transition_num ++;
    }

    if(!strcmp(element_path, "LifecycleConfiguration/Rule/NoncurrentVersionTransition"))
    {
        gblcDataEx->blcc_data[nIndex]->noncurrent_version_transition_num ++;
    }

    return OBS_STATUS_OK;
}


static get_lifecycle_config_data* init_get_lifecycle_data(obs_lifecycle_handler *handler, 
            void *callback_data)
{
    get_lifecycle_config_data *gblcDataEx = NULL;
    lifecycle_conf_data* blcc_data        = NULL;
    
    gblcDataEx = (get_lifecycle_config_data *) malloc(sizeof(get_lifecycle_config_data));
    if (!gblcDataEx) {
        COMMLOG(OBS_LOGERROR, "malloc lifecycle config data failed.");
        return NULL;
    }
    memset_s(gblcDataEx, sizeof(get_lifecycle_config_data), 0, sizeof(get_lifecycle_config_data));

    simplexml_initialize(&(gblcDataEx->simple_xml_info), &get_lifecycle_config_xml_callback, gblcDataEx);

    gblcDataEx->response_properties_callback  = handler->response_handler.properties_callback;
    gblcDataEx->response_complete_callback    = handler->response_handler.complete_callback;
    gblcDataEx->get_lifecycle_callback        = handler->get_lifecycle_callback;
    gblcDataEx->callback_data                 = callback_data;

    blcc_data = (lifecycle_conf_data*)malloc(sizeof(lifecycle_conf_data));
    if (!blcc_data)
    {
        COMMLOG(OBS_LOGERROR, "malloc lifecycle_conf_data failed.");
        free(gblcDataEx);
        return NULL;
    }
    memset_s(blcc_data, sizeof(lifecycle_conf_data), 0, sizeof(lifecycle_conf_data));
    gblcDataEx->blcc_data[0] = blcc_data;
    gblcDataEx->blcc_number  = 1;

    return gblcDataEx;
}

static obs_status make_get_lifecycle_callback(get_lifecycle_config_data *gblcDataEx)
{
    obs_status iRet = OBS_STATUS_OK;

    int nCount = gblcDataEx->blcc_number - 1;
    if(nCount < 1)
    {
        COMMLOG(OBS_LOGERROR, "Invalid Malloc Parameter.");
        return OBS_STATUS_OutOfMemory;
    }

    obs_lifecycle_conf* buckLifeCycleConf = (obs_lifecycle_conf*)malloc(sizeof(obs_lifecycle_conf) * nCount);
    if (NULL == buckLifeCycleConf)
    {
        COMMLOG(OBS_LOGERROR, "malloc obs_lifecycle_conf failed.");
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(buckLifeCycleConf, sizeof(obs_lifecycle_conf) * nCount, 0, sizeof(obs_lifecycle_conf) * nCount);

    int i = 0;
    int j = 0;
    for (; i<nCount; ++i)
    {
        buckLifeCycleConf[i].date = gblcDataEx->blcc_data[i]->date;
        buckLifeCycleConf[i].days = gblcDataEx->blcc_data[i]->days;
        buckLifeCycleConf[i].id= gblcDataEx->blcc_data[i]->id;
        buckLifeCycleConf[i].prefix= gblcDataEx->blcc_data[i]->prefix;
        buckLifeCycleConf[i].status= gblcDataEx->blcc_data[i]->status;
        buckLifeCycleConf[i].noncurrent_version_days = gblcDataEx->blcc_data[i]->nonCurrentVerionDays;
        buckLifeCycleConf[i].transition_num = gblcDataEx->blcc_data[i]->transition_num;
        buckLifeCycleConf[i].transition = (obs_lifecycle_transtion*)malloc(
                sizeof(obs_lifecycle_transtion) * (gblcDataEx->blcc_data[i]->transition_num));
        if (NULL == buckLifeCycleConf[i].transition)
        {
            COMMLOG(OBS_LOGERROR, "malloc obs_lifecycle_conf failed.");
            return OBS_STATUS_OutOfMemory;
        }
        for(j = 0; j < gblcDataEx->blcc_data[i]->transition_num; j++)
        {
            buckLifeCycleConf[i].transition[j].date = gblcDataEx->blcc_data[i]->arrTransitionData[j].date;
            buckLifeCycleConf[i].transition[j].days = gblcDataEx->blcc_data[i]->arrTransitionData[j].days;
            buckLifeCycleConf[i].transition[j].storage_class = get_storage_class_enum(
                        gblcDataEx->blcc_data[i]->arrTransitionData[j].storage_class, gblcDataEx->use_api);
        }

        buckLifeCycleConf[i].noncurrent_version_transition_num = gblcDataEx->blcc_data[i]->noncurrent_version_transition_num;
        buckLifeCycleConf[i].noncurrent_version_transition = (obs_lifecycle_noncurrent_transtion*)malloc(
            sizeof(obs_lifecycle_noncurrent_transtion) * gblcDataEx->blcc_data[i]->noncurrent_version_transition_num);
        if (NULL == buckLifeCycleConf[i].noncurrent_version_transition )
        {
            COMMLOG(OBS_LOGERROR, "malloc noncurrent_version_transition failed.");
            return OBS_STATUS_OutOfMemory;
        }
         
        for(j = 0; j < gblcDataEx->blcc_data[i]->noncurrent_version_transition_num; j++)
        {
            buckLifeCycleConf[i].noncurrent_version_transition[j].noncurrent_version_days = 
                        gblcDataEx->blcc_data[i]->arrNonCurrentVersionTransitionData[j].days;
            buckLifeCycleConf[i].noncurrent_version_transition[j].storage_class = get_storage_class_enum(
                        gblcDataEx->blcc_data[i]->arrNonCurrentVersionTransitionData[j].storage_class, gblcDataEx->use_api);
        }

    }

    iRet = (*(gblcDataEx->get_lifecycle_callback))(buckLifeCycleConf, nCount, 
                            gblcDataEx->callback_data);

    for(i = 0; i < nCount; i++)
    {
        CHECK_NULL_FREE(buckLifeCycleConf[i].noncurrent_version_transition);
        CHECK_NULL_FREE(buckLifeCycleConf[i].transition);
    }

    CHECK_NULL_FREE(buckLifeCycleConf);

    return iRet;
}


static obs_status get_lifecycle_properties_callback(const obs_response_properties *response_properties, 
        void *callback_data)
{
    get_lifecycle_config_data *gblcDataEx = (get_lifecycle_config_data *)callback_data;
    if (gblcDataEx->response_properties_callback)
    {
        return (*(gblcDataEx->response_properties_callback))(response_properties, 
                            gblcDataEx->callback_data);
    }
    return OBS_STATUS_OK;
}

static obs_status get_lifecycle_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    get_lifecycle_config_data *gblcDataEx = (get_lifecycle_config_data *)callback_data;

    return simplexml_add(&(gblcDataEx->simple_xml_info), buffer, buffer_size);
}


static void get_lifecycle_complete_callback(obs_status request_status, 
            const obs_error_details *obs_error_info, void *callback_data)
{
    unsigned int i = 0;
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_lifecycle_config_data *gblcDataEx = (get_lifecycle_config_data *)callback_data;

    // Make the callback if there is anything
    if (gblcDataEx->blcc_number && OBS_STATUS_OK == request_status) {
        request_status = make_get_lifecycle_callback(gblcDataEx);
    }

    (*(gblcDataEx->response_complete_callback))(request_status, obs_error_info, 
                        gblcDataEx->callback_data);

     for(i = 0; i< gblcDataEx->blcc_number; i++)
     {
         CHECK_NULL_FREE(gblcDataEx->blcc_data[i]);
     }
     
    simplexml_deinitialize(&(gblcDataEx->simple_xml_info));

    free(gblcDataEx);
    gblcDataEx = NULL;

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


void get_bucket_lifecycle_configuration(const obs_options *options,
                       obs_lifecycle_handler *handler, void *callback_data)
{
    request_params params;
    get_lifecycle_config_data *gblcDataEx = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set_bucket_lifecycle_configuration start !");
    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    
    gblcDataEx = init_get_lifecycle_data(handler, callback_data);
    if (NULL == gblcDataEx) 
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    gblcDataEx->use_api = use_api;
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
             sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
             sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_get;
    params.properties_callback     = &get_lifecycle_properties_callback;
    params.fromObsCallback        = &get_lifecycle_data_callback;
    params.complete_callback       = &get_lifecycle_complete_callback;
    params.callback_data           = gblcDataEx;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "lifecycle";
    params.temp_auth              = options->temp_auth; 
    params.use_api=use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_lifecycle_configuration finish.");
    
}


/*************************delete_bucket_lifecycle_configuration *******************************/
void delete_bucket_lifecycle_configuration(const obs_options *options, obs_response_handler *handler, 
            void *callback_data)
{
    request_params params;
    COMMLOG(OBS_LOGINFO, "delete_bucket_lifecycle_configuration start!");
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
                sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
                sizeof(obs_http_request_option));

    params.httpRequestType      = http_request_type_delete;
    params.properties_callback   = handler->properties_callback;
    params.complete_callback     = handler->complete_callback;
    params.isCheckCA            = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.subResource          = "lifecycle";
    params.temp_auth            = options->temp_auth; 
    params.callback_data        = callback_data;
    params.use_api=use_api;
    request_perform(&params);
    
    COMMLOG(OBS_LOGINFO, "delete_bucket_lifecycle_configuration finish!");
}


/*************************set_bucket_cors*******************************************************/
static obs_status set_cors_xml_elements(const char **elements, unsigned int elements_num, 
                char *element_name, set_cors_config_data *sbccData)
{
    unsigned int uiIdx = 0;
    for(uiIdx=0; uiIdx < elements_num; uiIdx++)
    {
        if(NULL != elements[uiIdx])
        {
            (void)add_xml_element(sbccData->doc, &sbccData->doc_len, element_name, 
                        elements[uiIdx], NEED_FORMALIZE, ADD_NAME_CONTENT);   
        }

        if((sbccData->doc_len >= 1024*10) && (uiIdx != (elements_num -1)))
        {
            COMMLOG(OBS_LOGERROR, "set cors fail,element_name(%s) Number(%u) too much.",
                        element_name, elements_num);
            return OBS_STATUS_InvalidParameter;
        }
    }
    return OBS_STATUS_OK;
}

static obs_status set_cors_quest_xml(obs_bucket_cors_conf *obs_cors_conf_info, 
            unsigned int conf_num, set_cors_config_data *sbccData)
{
    unsigned int i = 0;
    obs_status ret_status = OBS_STATUS_OK;
    
    (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "CORSConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
    for (i = 0; i< conf_num; ++i)
    {
        if(NULL == obs_cors_conf_info[i].allowed_method || NULL == obs_cors_conf_info[i].allowed_origin)
        {
            COMMLOG(OBS_LOGERROR, "allowed_method(%p) or allowed_origin(%p) is NULL",
                        obs_cors_conf_info[i].allowed_method,  obs_cors_conf_info[i].allowed_origin);
            return OBS_STATUS_InvalidParameter;
        }

        (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "CORSRule", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
        if(obs_cors_conf_info[i].id)
        {
            (void)add_xml_element(sbccData->doc, &sbccData->doc_len,"ID", 
                            obs_cors_conf_info[i].id, NEED_FORMALIZE, ADD_NAME_CONTENT);    
        }

        ret_status = set_cors_xml_elements(obs_cors_conf_info[i].allowed_method, 
                obs_cors_conf_info[i].allowed_method_number, "AllowedMethod", sbccData);
        if (OBS_STATUS_OK != ret_status)
        {
            return OBS_STATUS_InvalidParameter;
        }

        ret_status = set_cors_xml_elements(obs_cors_conf_info[i].allowed_origin, 
                obs_cors_conf_info[i].allowed_origin_number, "AllowedOrigin", sbccData);
        if (OBS_STATUS_OK != ret_status)
        {
            return OBS_STATUS_InvalidParameter;
        }

        ret_status = set_cors_xml_elements(obs_cors_conf_info[i].allowed_header, 
                obs_cors_conf_info[i].allowed_header_number, "AllowedHeader", sbccData);
        if (OBS_STATUS_OK != ret_status)
        {
            return OBS_STATUS_InvalidParameter;
        }
        
        if(obs_cors_conf_info[i].max_age_seconds)
        {
            (void)add_xml_element(sbccData->doc, &sbccData->doc_len,"MaxAgeSeconds", 
                            obs_cors_conf_info[i].max_age_seconds, NEED_FORMALIZE, ADD_NAME_CONTENT);   
        }

        ret_status = set_cors_xml_elements(obs_cors_conf_info[i].expose_header, 
                obs_cors_conf_info[i].expose_header_number, "ExposeHeader", sbccData);
        if (OBS_STATUS_OK != ret_status)
        {
            return OBS_STATUS_InvalidParameter;
        }

        (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "CORSRule", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    }
    
    (void)add_xml_element(sbccData->doc, &sbccData->doc_len, "CORSConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    
    return OBS_STATUS_OK;
}

static set_cors_config_data* init_cors_data(obs_bucket_cors_conf *obs_cors_conf_info, 
            unsigned int conf_num, obs_response_handler *handler, void *callback_data)
{
    unsigned char doc_md5[16];
    obs_status ret_status = OBS_STATUS_OK;
    set_cors_config_data *sbccData = (set_cors_config_data *) malloc(sizeof(set_cors_config_data));
    if (!sbccData) 
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc cors_data failed.");
        return NULL;
    }
    memset_s(sbccData, sizeof(set_cors_config_data), 0, sizeof(set_cors_config_data));

    sbccData->response_complete_callback    = handler->complete_callback;
    sbccData->response_properties_callback  = handler->properties_callback;
    sbccData->callback_data                 = callback_data;
    sbccData->doc_len                       = 0;
    sbccData->doc_bytes_written             = 0;
   
    ret_status = set_cors_quest_xml(obs_cors_conf_info, conf_num, sbccData);
    if (OBS_STATUS_OK != ret_status || sbccData->doc_len <= 0)
    {
        free(sbccData);
        sbccData = NULL;
        return NULL;
    }
    COMMLOG(OBS_LOGERROR, "request xml: %s.", sbccData->doc);

    MD5((unsigned char *)sbccData->doc, (size_t)sbccData->doc_len, doc_md5); 
    base64Encode(doc_md5, sizeof(doc_md5), sbccData->doc_md5);
    
    return sbccData;
}

static int set_cors_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    set_cors_config_data *sbccData = (set_cors_config_data *) callback_data;

    if (!sbccData->doc_len) {
        return 0;
    }

    int remaining = (sbccData->doc_len - sbccData->doc_bytes_written);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy) {
        return 0;
    }

    memcpy_s(buffer, buffer_size, &(sbccData->doc[sbccData->doc_bytes_written]), toCopy);

    sbccData->doc_bytes_written += toCopy;

    return toCopy;
}

static obs_status set_cors_properties_callback(const obs_response_properties *response_properties, 
        void *callback_data)
{
    set_cors_config_data *sbccData = (set_cors_config_data *) callback_data;    
    if (sbccData->response_properties_callback)
    {
        return (*(sbccData->response_properties_callback))
                             (response_properties, sbccData->callback_data);
    }
    return OBS_STATUS_OK;
}


static void set_cors_complete_callback(obs_status request_status,
                                         const obs_error_details *obs_error_info,
                                         void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_cors_config_data *sbccData = (set_cors_config_data *) callback_data;

    (void)(*(sbccData->response_complete_callback))(request_status, obs_error_info, 
            sbccData->callback_data);

    free(sbccData);
    sbccData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

    

void set_bucket_cors_configuration(const obs_options *options, obs_bucket_cors_conf *obs_cors_conf_info, 
            unsigned int conf_num, obs_response_handler *handler, void *callback_data)
{
    request_params     params;
    obs_put_properties put_properties;
    set_cors_config_data *sbccData   = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "set_bucket_cors start !");

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    
    if(conf_num <= 0 || conf_num > 100)
    {
        COMMLOG(OBS_LOGERROR, "set_bucket_cors faied, conf_num(%d) is invalid.", conf_num);
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }

    sbccData = init_cors_data(obs_cors_conf_info, conf_num, handler, callback_data);
    if (!sbccData) 
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc sbccData failed.");
        return;
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
             sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
             sizeof(obs_http_request_option));
    
    memset_s(&put_properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    put_properties.md5 = sbccData->doc_md5;
    put_properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    params.httpRequestType        = http_request_type_put;
    params.properties_callback     = &set_cors_properties_callback;
    params.toObsCallback          =  &set_cors_data_callback;
    params.complete_callback       = &set_cors_complete_callback;
    params.toObsCallbackTotalSize = sbccData->doc_len;
    params.callback_data           = sbccData;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "cors";
    params.put_properties          = &put_properties;
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_bucket_cors finish.");
}


/*******************get_bucket_cors_configuration**************************************/
static obs_status add_one_get_cors_data(const char *element_path, 
            get_bucket_cors_data *gbccDataEx)
{
    int nIndex = gbccDataEx->bccd_number - 1;
    
    if (!strcmp(element_path, "CORSConfiguration/CORSRule"))
    {
        bucket_cors_conf_data* bcc_data = (bucket_cors_conf_data*)malloc(sizeof(bucket_cors_conf_data));
        if (!bcc_data) 
        {
            COMMLOG(OBS_LOGERROR, "malloc bucket_cors_conf_data failed.");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(bcc_data, sizeof(bucket_cors_conf_data), 0, sizeof(bucket_cors_conf_data)); 
        gbccDataEx->bcc_data[gbccDataEx->bccd_number] = bcc_data;
        gbccDataEx->bccd_number++;
    }

    if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedMethod")) {
        gbccDataEx->bcc_data[nIndex]->allowedMethodCount++;
        gbccDataEx->bcc_data[nIndex]->allowedMethodes[gbccDataEx->bcc_data[nIndex]->allowedMethodCount][0] = 0;
        gbccDataEx->bcc_data[nIndex]->allowedMethodLens[gbccDataEx->bcc_data[nIndex]->allowedMethodCount] = 0;
    }
    else if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedOrigin")) {
        gbccDataEx->bcc_data[nIndex]->allowedOriginCount++;
        gbccDataEx->bcc_data[nIndex]->allowedOrigines[gbccDataEx->bcc_data[nIndex]->allowedOriginCount][0] = 0;
        gbccDataEx->bcc_data[nIndex]->allowedOriginLens[gbccDataEx->bcc_data[nIndex]->allowedOriginCount] = 0;
    }
    else if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedHeader")) {
        gbccDataEx->bcc_data[nIndex]->allowedHeaderCount++;
        gbccDataEx->bcc_data[nIndex]->allowedHeaderes[gbccDataEx->bcc_data[nIndex]->allowedHeaderCount][0] = 0;
        gbccDataEx->bcc_data[nIndex]->allowedHeaderLens[gbccDataEx->bcc_data[nIndex]->allowedHeaderCount] = 0;
    }
    else if (!strcmp(element_path, "CORSConfiguration/CORSRule/ExposeHeader")) {
        gbccDataEx->bcc_data[nIndex]->exposeHeaderCount++;
        gbccDataEx->bcc_data[nIndex]->exposeHeaderes[gbccDataEx->bcc_data[nIndex]->exposeHeaderCount][0] = 0;
        gbccDataEx->bcc_data[nIndex]->exposeHeaderLens[gbccDataEx->bcc_data[nIndex]->exposeHeaderCount] = 0;
    }

    return OBS_STATUS_OK;
}

static obs_status get_cors_xml_callback(const char *element_path,
                                      const char *data, int data_len,
                                      void *callback_data)
{
    int fit;
    obs_status ret_status = OBS_STATUS_OK;
    get_bucket_cors_data *gbccDataEx = (get_bucket_cors_data *) callback_data;
    int nIndex = gbccDataEx->bccd_number - 1;
    
    if (data) {
        if (!strcmp(element_path, "CORSConfiguration/CORSRule/ID")) {
            string_buffer_append(gbccDataEx->bcc_data[nIndex]->id, data, data_len, fit);
        }
        else if (!strcmp(element_path, "CORSConfiguration/CORSRule/MaxAgeSeconds")) {
            string_buffer_append(gbccDataEx->bcc_data[nIndex]->max_age_seconds, data, data_len, fit);
        }
        else if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedMethod")) {
            int which = gbccDataEx->bcc_data[nIndex]->allowedMethodCount;
            gbccDataEx->bcc_data[nIndex]->allowedMethodLens[which] +=
                snprintf_sec(gbccDataEx->bcc_data[nIndex]->allowedMethodes[which], 
                    sizeof(gbccDataEx->bcc_data[nIndex]->allowedMethodes[which]), 
                    sizeof(gbccDataEx->bcc_data[nIndex]->allowedMethodes[which]) -
                         gbccDataEx->bcc_data[nIndex]->allowedMethodLens[which] - 1,
                    "%.*s", data_len, data);
            if (gbccDataEx->bcc_data[nIndex]->allowedMethodLens[which] >=
                (int) sizeof(gbccDataEx->bcc_data[nIndex]->allowedMethodes[which])) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
        else if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedOrigin")) {
            int which = gbccDataEx->bcc_data[nIndex]->allowedOriginCount;
            gbccDataEx->bcc_data[nIndex]->allowedOriginLens[which] +=
                snprintf_sec(gbccDataEx->bcc_data[nIndex]->allowedOrigines[which],
                       sizeof(gbccDataEx->bcc_data[nIndex]->allowedOrigines[which]), 
                         sizeof(gbccDataEx->bcc_data[nIndex]->allowedOrigines[which]) -
                         gbccDataEx->bcc_data[nIndex]->allowedOriginLens[which] - 1,
                         "%.*s", data_len, data);
            if (gbccDataEx->bcc_data[nIndex]->allowedOriginLens[which] >=
                (int) sizeof(gbccDataEx->bcc_data[nIndex]->allowedOrigines[which])) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
        else if (!strcmp(element_path,
                         "CORSConfiguration/CORSRule/AllowedHeader")) {
            int which = gbccDataEx->bcc_data[nIndex]->allowedHeaderCount;
            gbccDataEx->bcc_data[nIndex]->allowedHeaderLens[which] +=
                snprintf_sec(gbccDataEx->bcc_data[nIndex]->allowedHeaderes[which], 
                  sizeof(gbccDataEx->bcc_data[nIndex]->allowedHeaderes[which]), 
                         sizeof(gbccDataEx->bcc_data[nIndex]->allowedHeaderes[which]) -
                         gbccDataEx->bcc_data[nIndex]->allowedHeaderLens[which] - 1,
                         "%.*s", data_len, data);
            if (gbccDataEx->bcc_data[nIndex]->allowedHeaderLens[which] >=
                (int) sizeof(gbccDataEx->bcc_data[nIndex]->allowedHeaderes[which])) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
        else if (!strcmp(element_path, "CORSConfiguration/CORSRule/ExposeHeader")) {
            int which = gbccDataEx->bcc_data[nIndex]->exposeHeaderCount;
            gbccDataEx->bcc_data[nIndex]->exposeHeaderLens[which] +=
                snprintf_sec(gbccDataEx->bcc_data[nIndex]->exposeHeaderes[which], 
                    sizeof(gbccDataEx->bcc_data[nIndex]->exposeHeaderes[which]),
                         sizeof(gbccDataEx->bcc_data[nIndex]->exposeHeaderes[which]) -
                         gbccDataEx->bcc_data[nIndex]->exposeHeaderLens[which] - 1,
                         "%.*s", data_len, data);
            if (gbccDataEx->bcc_data[nIndex]->exposeHeaderLens[which] >=
                (int) sizeof(gbccDataEx->bcc_data[nIndex]->exposeHeaderes[which])) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
    }
    else 
    {
        ret_status = add_one_get_cors_data(element_path, gbccDataEx);
        if (OBS_STATUS_OK != ret_status)
        {
            return ret_status;
        }
    }

    (void) fit;

    return OBS_STATUS_OK;
}


static get_bucket_cors_data* init_get_cors_data(obs_cors_handler *handler, 
            void *callback_data)
{
    get_bucket_cors_data *gbccDataEx    = NULL;
    bucket_cors_conf_data *bcc_data                = NULL;
    
    gbccDataEx = (get_bucket_cors_data *) malloc(sizeof(get_bucket_cors_data));
    if (!gbccDataEx) {
        COMMLOG(OBS_LOGERROR, "malloc cors config data failed.");
        return NULL;
    }
    memset_s(gbccDataEx, sizeof(get_bucket_cors_data), 0, sizeof(get_bucket_cors_data));

    simplexml_initialize(&(gbccDataEx->simple_xml_info), &get_cors_xml_callback, gbccDataEx);

    gbccDataEx->response_properties_callback  = handler->response_handler.properties_callback;
    gbccDataEx->response_complete_callback    = handler->response_handler.complete_callback;
    gbccDataEx->get_cors_callback             = handler->get_cors_callback;
    gbccDataEx->callback_data                 = callback_data;

    bcc_data = (bucket_cors_conf_data*)malloc(sizeof(bucket_cors_conf_data));
    if (!bcc_data)
    {
        COMMLOG(OBS_LOGERROR, "malloc bucket_cors_conf_data failed.");
        free(gbccDataEx);
        return NULL;
    }
    memset_s(bcc_data, sizeof(bucket_cors_conf_data), 0, sizeof(bucket_cors_conf_data));
    gbccDataEx->bcc_data[0]  = bcc_data;
    gbccDataEx->bccd_number  = 1;

    return gbccDataEx;
}

static void free_obs_bucket_cors_conf(obs_bucket_cors_conf* bucketCorsConf, int conf_num)
{
    int i =0;
    for (i =0; i < conf_num; i++)
    {
        if (bucketCorsConf[i].allowed_method)
        {
            free(bucketCorsConf[i].allowed_method);
            bucketCorsConf[i].allowed_method= NULL;
        }

        if (bucketCorsConf[i].allowed_origin)
        {
            free(bucketCorsConf[i].allowed_origin);
            bucketCorsConf[i].allowed_origin= NULL;
        }

        if (bucketCorsConf[i].allowed_header)
        {
            free(bucketCorsConf[i].allowed_header);
            bucketCorsConf[i].allowed_header= NULL;
        }
        
        if (bucketCorsConf[i].expose_header)
        {
            free(bucketCorsConf[i].expose_header);
            bucketCorsConf[i].expose_header= NULL;
        }
    }
   
    free(bucketCorsConf);
    return;
}

static const char** set_return_cors_value(char in_char[][1024], int char_count)
{
    const char **out_char = NULL;
    int i = 0;
    out_char = (const char**)malloc(sizeof(char *) * char_count);
    if (NULL == out_char)
    {
        return NULL;
    }
    memset_s(out_char, sizeof(char *) * char_count, 0, sizeof(char *) * char_count);
    for (i = 0; i < char_count; i++)
    {
        out_char[i] = in_char[i];
    }
    
    return out_char;
}


static obs_status make_get_cors_callbackEx(get_bucket_cors_data *gbccDataEx)
{
    obs_status iRet = OBS_STATUS_OK;
    int i = 0;
    int is_get_data_err = 0;
    int nCount = gbccDataEx->bccd_number - 1;
    if(nCount <= 0)
    {
        COMMLOG(OBS_LOGERROR, "bccd is empty,bccd_num(%u).", gbccDataEx->bccd_number);
        return OBS_STATUS_InternalError;
    }
    
    obs_bucket_cors_conf* bucketCorsConf = (obs_bucket_cors_conf*)malloc(sizeof(obs_bucket_cors_conf) * nCount);
    if (NULL == bucketCorsConf)
    {
        COMMLOG(OBS_LOGERROR, "malloc obs_bucket_cors_conf failed.");
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(bucketCorsConf, sizeof(obs_bucket_cors_conf) * nCount, 0, sizeof(obs_bucket_cors_conf) * nCount);

    for (; i<nCount; ++i)
    {
        COMMLOG(OBS_LOGINFO, "get cors config err,Method(%d),Origin(%d),Header(%d),expose(%d).",
                gbccDataEx->bcc_data[i]->allowedMethodCount, gbccDataEx->bcc_data[i]->allowedOriginCount,
                gbccDataEx->bcc_data[i]->allowedHeaderCount, gbccDataEx->bcc_data[i]->exposeHeaderCount);
        is_get_data_err = (gbccDataEx->bcc_data[i]->allowedMethodCount < 1) 
              || (gbccDataEx->bcc_data[i]->allowedOriginCount < 1);
        if (is_get_data_err)
        {
            free_obs_bucket_cors_conf(bucketCorsConf, i);
            return OBS_STATUS_OutOfMemory;
        }
        
        bucketCorsConf[i].id = gbccDataEx->bcc_data[i]->id;

        bucketCorsConf[i].allowed_method = set_return_cors_value( 
            gbccDataEx->bcc_data[i]->allowedMethodes, gbccDataEx->bcc_data[i]->allowedMethodCount);
        bucketCorsConf[i].allowed_method_number = gbccDataEx->bcc_data[i]->allowedMethodCount;

        bucketCorsConf[i].allowed_origin = set_return_cors_value(
            gbccDataEx->bcc_data[i]->allowedOrigines, gbccDataEx->bcc_data[i]->allowedOriginCount);
        bucketCorsConf[i].allowed_origin_number= gbccDataEx->bcc_data[i]->allowedOriginCount;

        if (gbccDataEx->bcc_data[i]->allowedHeaderCount > 0)
        {
            bucketCorsConf[i].allowed_header = set_return_cors_value(  
                gbccDataEx->bcc_data[i]->allowedHeaderes, gbccDataEx->bcc_data[i]->allowedHeaderCount);
            bucketCorsConf[i].allowed_header_number = gbccDataEx->bcc_data[i]->allowedHeaderCount;
        }

        bucketCorsConf[i].max_age_seconds= gbccDataEx->bcc_data[i]->max_age_seconds;

        if (gbccDataEx->bcc_data[i]->exposeHeaderCount > 0)
        {
            bucketCorsConf[i].expose_header = set_return_cors_value( 
                gbccDataEx->bcc_data[i]->exposeHeaderes, gbccDataEx->bcc_data[i]->exposeHeaderCount);
            bucketCorsConf[i].expose_header_number = gbccDataEx->bcc_data[i]->exposeHeaderCount;
        }

        COMMLOG(OBS_LOGINFO, "get cors config err,Method(%p),Origin(%p),Header(%p),expose(%p).",
                bucketCorsConf[i].allowed_method, bucketCorsConf[i].allowed_origin,
                bucketCorsConf[i].allowed_header,  bucketCorsConf[i].expose_header);
        is_get_data_err = (NULL == bucketCorsConf[i].allowed_method)
                        || (NULL == bucketCorsConf[i].allowed_origin)
                        || (gbccDataEx->bcc_data[i]->allowedHeaderCount > 0 
                            && NULL == bucketCorsConf[i].allowed_header)
                        || (gbccDataEx->bcc_data[i]->exposeHeaderCount > 0
                            && NULL == bucketCorsConf[i].expose_header);
        if(is_get_data_err)
        {
            free_obs_bucket_cors_conf(bucketCorsConf, i+1);
            return OBS_STATUS_OutOfMemory;
        }
    }

    iRet = (*(gbccDataEx->get_cors_callback))(bucketCorsConf, nCount, gbccDataEx->callback_data);

    free_obs_bucket_cors_conf(bucketCorsConf, nCount);

    return iRet;
}


static obs_status get_cors_properties_callback(const obs_response_properties *response_properties, 
        void *callback_data)
{
    get_bucket_cors_data *gbccDataEx = (get_bucket_cors_data *) callback_data;
    if (gbccDataEx->response_properties_callback)
    {
        return (*(gbccDataEx->response_properties_callback))
                                (response_properties, gbccDataEx->callback_data);
    }
    return OBS_STATUS_OK;
}

static obs_status get_cors_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    get_bucket_cors_data *gbccDataEx = (get_bucket_cors_data *)callback_data;

    return simplexml_add(&(gbccDataEx->simple_xml_info), buffer, buffer_size);
}


static void get_cors_complete_callback(obs_status request_status,
                                         const obs_error_details *obs_error_info,
                                         void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    get_bucket_cors_data *gbccDataEx = (get_bucket_cors_data *)callback_data;

    if (gbccDataEx->bccd_number && OBS_STATUS_OK == request_status) {
        request_status = make_get_cors_callbackEx(gbccDataEx);
    }

    (*(gbccDataEx->response_complete_callback))(request_status, obs_error_info, 
                gbccDataEx->callback_data);

    simplexml_deinitialize(&(gbccDataEx->simple_xml_info));

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void get_bucket_cors_configuration(const obs_options *options, obs_cors_handler *handler,
            void *callback_data)
{
    request_params params;
    get_bucket_cors_data *gbccDataEx = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get_bucket_cors_configuration start !");

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    
    gbccDataEx = init_get_cors_data(handler, callback_data);
    if (NULL == gbccDataEx) 
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
             sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
             sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_get;
    params.properties_callback     = &get_cors_properties_callback;
    params.fromObsCallback        = &get_cors_data_callback;
    params.complete_callback       = &get_cors_complete_callback;
    params.callback_data           = gbccDataEx;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "cors";
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_lifecycle_configuration finish.");
}

/******************************delete_bucket_cors_configuration **********************************/
void delete_bucket_cors_configuration(const obs_options *options, obs_response_handler *handler, 
                        void *callback_data)
{
    request_params params;
    COMMLOG(OBS_LOGINFO, "delete_bucket_cors_configuration start!");
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
                sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
                sizeof(obs_http_request_option));
    params.httpRequestType      = http_request_type_delete;
    params.properties_callback  = handler->properties_callback;
    params.complete_callback    = handler->complete_callback;
    params.isCheckCA            = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.subResource          = "cors";
    params.temp_auth            = options->temp_auth;
    params.callback_data        = callback_data;
    params.use_api =use_api;
    request_perform(&params);
    
    COMMLOG(OBS_LOGINFO, "delete_bucket_cors_configuration finish!");
}

obs_status generate_logging_xml_document_s3(char *target_bucket, char *target_prefix, 
            obs_acl_group *acl_group, int *xml_doc_len_return, 
            char *xml_document, int xml_doc_buffer_size)
{
    if (!target_bucket) {
        append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
           "%s", "<BucketLoggingStatus xmlns=\"http://doc.s3.amazonaws.com/2006-03-01\" />");
        return OBS_STATUS_OK;
    }

    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
       "%s", "<BucketLoggingStatus xmlns=\"http://doc.s3.amazonaws.com/2006-03-01\">");
    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
       "<LoggingEnabled><TargetBucket>%s</TargetBucket>", target_bucket);
    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
       "<TargetPrefix>%s</TargetPrefix>", target_prefix ? target_prefix : "");
    if (acl_group) 
    {
        append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
            "%s", "<TargetGrants>");
        int i;
        for (i = 0; i < acl_group->acl_grant_count; i++) 
        {
            append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, "%s", 
                "<Grant><Grantee xmlns:xsi=\"http://www.w3.org/2001/"
                "XMLSchema-instance\" xsi:type=\"");
            obs_acl_grant *grant = &(acl_group->acl_grants[i]);
            switch (grant->grantee_type) 
            {
                case OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL:
                    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
                    "AmazonCustomerByEmail\"><EmailAddress>%s</EmailAddress>",
                    grant->grantee.huawei_customer_by_email.email_address);
                    break;
                case OBS_GRANTEE_TYPE_CANONICAL_USER:
                    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
                    "CanonicalUser\"><ID>%s</ID><DisplayName>%s</DisplayName>",
                    grant->grantee.canonical_user.id, grant->grantee.canonical_user.display_name);
                    break;
                default:
                    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
                    "Group\"><URI>%s</URI>", (grant->grantee_type == OBS_GRANTEE_TYPE_ALL_OBS_USERS) ?
                    ACS_GROUP_AWS_USERS : ACS_GROUP_ALL_USERS);
                    break;
            }
            append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
                    "</Grantee><Permission>%s</Permission></Grant>",
                   ((grant->permission == OBS_PERMISSION_READ) ? "READ" :
                    (grant->permission == OBS_PERMISSION_WRITE) ? "WRITE" :
                    (grant->permission == OBS_PERMISSION_READ_ACP) ? "READ_ACP" :
                    (grant->permission == OBS_PERMISSION_WRITE_ACP) ? "WRITE_ACP" :
                    (grant->permission == OBS_PERMISSION_FULL_CONTROL) ? "FULL_CONTROL" : "READ"));
        }
        append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
                    "%s", "</TargetGrants>");
    }
    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
        "%s", "</LoggingEnabled>");
    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
        "%s", "</BucketLoggingStatus>");

    return OBS_STATUS_OK;
}

obs_status generate_logging_xml_document_obs(char *target_bucket, char *target_prefix, char *agency,
            obs_acl_group *acl_group, int *xml_doc_len_return, 
            char *xml_document, int xml_doc_buffer_size)
{

    if (!target_bucket)
    {
        append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
           "%s", "<BucketLoggingStatus xmlns=\"http://obs.myhwclouds.com/doc/2015-06-30/\" />");
        return OBS_STATUS_OK;
    }
    
    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
       "%s", "<BucketLoggingStatus>");
    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
       "<Agency>%s</Agency><LoggingEnabled><TargetBucket>%s</TargetBucket>", agency, target_bucket);
    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
       "<TargetPrefix>%s</TargetPrefix>", target_prefix ? target_prefix : "");
    if (acl_group) 
    {
        append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size,
            "%s", "<TargetGrants>");
        int i;
        for (i = 0; i < acl_group->acl_grant_count; i++) 
        {
            append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, "%s", 
                "<Grant><Grantee>");
            obs_acl_grant *grant = &(acl_group->acl_grants[i]);
            switch (grant->grantee_type) 
            {
                case OBS_GRANTEE_TYPE_CANONICAL_USER:
                    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
                    "<ID>%s</ID>",
                    grant->grantee.canonical_user.id);
                    break;
                default:
                    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
                    "%s", "<Canned>Everyone</Canned>");
                    break;
            }
            append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
                    "</Grantee><Permission>%s</Permission><Delivered>%s</Delivered></Grant>",
                   ((grant->permission == OBS_PERMISSION_READ) ? "READ" :
                    (grant->permission == OBS_PERMISSION_WRITE) ? "WRITE" :
                    (grant->permission == OBS_PERMISSION_READ_ACP) ? "READ_ACP" :
                    (grant->permission == OBS_PERMISSION_WRITE_ACP) ? "WRITE_ACP" :
                    (grant->permission == OBS_PERMISSION_FULL_CONTROL) ? "FULL_CONTROL" : "READ"),
                    (grant->bucket_delivered) ? "true" : "false");

        }
        append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
                    "%s", "</TargetGrants>");
    } 
    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
        "%s", "</LoggingEnabled>");
    append_xml_document(xml_doc_len_return, xml_document, xml_doc_buffer_size, 
        "%s", "</BucketLoggingStatus>");

    return OBS_STATUS_OK;
}

obs_status generate_logging_xml_document(char *target_bucket, char *target_prefix, char *agency,
            obs_acl_group *acl_group, int *xml_doc_len_return, 
            char *xml_document, int xml_doc_buffer_size, obs_use_api use_api)
{
    if (use_api == OBS_USE_API_S3) {
        return generate_logging_xml_document_s3(target_bucket, target_prefix, acl_group, xml_doc_len_return, xml_document, xml_doc_buffer_size);
    } else {
        return generate_logging_xml_document_obs(target_bucket, target_prefix, agency, acl_group, xml_doc_len_return, xml_document, xml_doc_buffer_size);
    }
}

void set_bucket_logging_configuration_common(const obs_options *options, char *target_bucket, char *target_prefix, char *agency,
            obs_acl_group *acl_group, obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "set bucket logging configuration start!");

    set_common_data *bucket_logging_data = (set_common_data*)malloc(sizeof(set_common_data));
    if (!bucket_logging_data) 
    {
        COMMLOG(OBS_LOGERROR, "malloc set bucket_logging_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    memset_s(bucket_logging_data, sizeof(set_common_data), 0, sizeof(set_common_data));
   
    obs_status status = generate_logging_xml_document(target_bucket, target_prefix, agency, acl_group,
                                &(bucket_logging_data->xml_document_len), 
                                bucket_logging_data->xml_document,
                                sizeof(bucket_logging_data->xml_document),use_api);
    if (status != OBS_STATUS_OK)
    {
        free(bucket_logging_data);
        bucket_logging_data = NULL;
        (void)(*(handler->complete_callback))(status, 0, 0);
        COMMLOG(OBS_LOGERROR, "generate_storage_class_xml_document failed !");
        return;
    }
    bucket_logging_data->response_properties_callback = handler->properties_callback;
    bucket_logging_data->response_complete_callback   = handler->complete_callback;
    bucket_logging_data->callback_data                = callback_data;
       
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &set_common_properties_callback;
    params.toObsCallback          = &set_common_data_callback;
    params.toObsCallbackTotalSize = bucket_logging_data->xml_document_len;
    params.complete_callback      = &set_common_complete_callback;
    params.callback_data          = bucket_logging_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "logging";
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket logging configuration finish!");
}

void set_bucket_logging_configuration(const obs_options *options, char *target_bucket, char *target_prefix, 
            obs_acl_group *acl_group, obs_response_handler *handler, void *callback_data)
{
    set_bucket_logging_configuration_common(options, target_bucket, target_prefix, NULL,
                                            acl_group, handler, callback_data);
}

void set_bucket_logging_configuration_obs(const obs_options *options, char *target_bucket, char *target_prefix, char *agency,
            obs_acl_group *acl_group, obs_response_handler *handler, void *callback_data)
{
    set_bucket_logging_configuration_common(options, target_bucket, target_prefix, agency,
                                            acl_group, handler, callback_data);
}

obs_status get_bucket_logging_properties_callback(const obs_response_properties *response_properties, 
                    void *callback_data)
{
    get_bucket_logging_data *logging_data = (get_bucket_logging_data *) callback_data;
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
    get_bucket_logging_data *logging_data = (get_bucket_logging_data *) callback_data;
    int fit;
    string_buffer_append(logging_data->xml_document, buffer, buffer_size, fit);

    return fit ? OBS_STATUS_OK : OBS_STATUS_XmlDocumentTooLarge;
}



obs_status conver_bucket_logging_grant(const char *element_path, convert_bucket_logging_data *convert_data)
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

    if (convert_data->use_api == OBS_USE_API_S3) {
        if (convert_data->email_address[0]) {
            grant->grantee_type = OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL;
            strcpy_s(grant->grantee.huawei_customer_by_email.email_address,
                sizeof(grant->grantee.huawei_customer_by_email.email_address),
                   convert_data->email_address);
        }
        else_if (convert_data->userId[0] && convert_data->userDisplayName[0]) {
            grant->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
            strcpy_s(grant->grantee.canonical_user.id, sizeof(grant->grantee.canonical_user.id), 
                convert_data->userId);
            strcpy_s(grant->grantee.canonical_user.display_name,
            sizeof(grant->grantee.canonical_user.display_name),
                   convert_data->userDisplayName);
        }
        else_if (convert_data->groupUri[0]) {
            if (!strcmp(convert_data->groupUri, ACS_GROUP_AWS_USERS)) {
                grant->grantee_type = OBS_GRANTEE_TYPE_ALL_OBS_USERS;
            }
            else_if (!strcmp(convert_data->groupUri, ACS_GROUP_ALL_USERS)) {
                grant->grantee_type = OBS_GRANTEE_TYPE_ALL_USERS;
            }
            else {
                return OBS_STATUS_BadGrantee;
            }
        }
        else {
            return OBS_STATUS_BadGrantee;
        }
    }else {
        grant->grantee_type = OBS_GRANTEE_TYPE_ALL_USERS;
        if (convert_data->userId[0]) {
            grant->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
            strcpy_s(grant->grantee.canonical_user.id, sizeof(grant->grantee.canonical_user.id), 
                convert_data->userId);
        }
    }

    if (!strcmp(convert_data->permission, "READ")) {
        grant->permission = OBS_PERMISSION_READ;
    }
    else_if (!strcmp(convert_data->permission, "WRITE")) {
        grant->permission = OBS_PERMISSION_WRITE;
    }
    else_if (!strcmp(convert_data->permission, "READ_ACP")) {
        grant->permission = OBS_PERMISSION_READ_ACP;
    }
    else_if (!strcmp(convert_data->permission, "WRITE_ACP")) {
        grant->permission = OBS_PERMISSION_WRITE_ACP;
    }
    else_if (!strcmp(convert_data->permission, "FULL_CONTROL")) {
        grant->permission = OBS_PERMISSION_FULL_CONTROL;
    }
    else {
        return OBS_STATUS_BadPermission;
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
    convert_bucket_logging_data *convert_data = (convert_bucket_logging_data *) callback_data;
    obs_status status = OBS_STATUS_OK;
    int fit;

    if (data)
    {
        if (!strcmp(element_path, "BucketLoggingStatus/Agency"))
        {
            convert_data->agencyReturnLen +=
                snprintf_sec(&(convert_data->agencyReturn
                           [convert_data->agencyReturnLen]),
                            convert_data->agencyReturnSize,
                         convert_data->agencyReturnSize - convert_data->agencyReturnLen - 1,
                         "%.*s", data_len, data);
            if (convert_data->agencyReturnLen >= convert_data->agencyReturnSize) {
                return OBS_STATUS_InvalidParameter;
            }
        }    
        else_if (!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
                    "TargetBucket")) 
        {
            convert_data->targetBucketReturnLen +=
                snprintf_sec(&(convert_data->targetBucketReturn
                           [convert_data->targetBucketReturnLen]),
                            convert_data->targetBucketReturnSize,
                         convert_data->targetBucketReturnSize - convert_data->targetBucketReturnLen - 1,
                         "%.*s", data_len, data);
            if (convert_data->targetBucketReturnLen >= convert_data->targetBucketReturnSize) {
                return OBS_STATUS_InvalidParameter;
            }
        }
        else_if (!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
                    "TargetPrefix")) 
        {
            convert_data->targetPrefixReturnLen +=
                snprintf_sec(&(convert_data->targetPrefixReturn
                           [convert_data->targetPrefixReturnLen]),
                            convert_data->targetPrefixReturnSize,
                         convert_data->targetPrefixReturnSize - convert_data->targetPrefixReturnLen - 1,
                         "%.*s", data_len, data);
            if (convert_data->targetPrefixReturnLen >= convert_data->targetPrefixReturnSize) {
                return OBS_STATUS_InvalidParameter;
            }
        }
        else_if (!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
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
        else_if (!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
                         "TargetGrants/Grant/Grantee/DisplayName")) 
        {
            string_buffer_append(convert_data->userDisplayName, data, data_len, fit);
            if (!fit) {
                return OBS_STATUS_UserDisplayNameTooLong;
            }
        }
        else_if (!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
                         "TargetGrants/Grant/Grantee/URI")) 
        {
            string_buffer_append(convert_data->groupUri, data, data_len, fit);
            if (!fit) {
                return OBS_STATUS_GroupUriTooLong;
            }
        }
        else_if (!strcmp(element_path, "BucketLoggingStatus/LoggingEnabled/"
                         "TargetGrants/Grant/Permission")) 
        {
            string_buffer_append(convert_data->permission, data, data_len, fit);
            if (!fit) {
                return OBS_STATUS_PermissionTooLong;
            }
        }
    }

    status = conver_bucket_logging_grant(element_path, convert_data);
    return status;
}

obs_status convert_bls(const char *blsXml, bucket_logging_message *logging_message,obs_use_api use_api)
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
    data.use_api =use_api;

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

    get_bucket_logging_data *logging_data = (get_bucket_logging_data *) callback_data;

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

void get_bucket_logging_configuration(const obs_options *options, obs_response_handler *handler, 
                    bucket_logging_message *logging_message_data, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get bucket logging conf start!");
    get_bucket_logging_data *logging_data = 
        (get_bucket_logging_data*) malloc(sizeof(get_bucket_logging_data));
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
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.httpRequestType      = http_request_type_get;
    params.properties_callback   = &get_bucket_logging_properties_callback;
    params.fromObsCallback      = &get_bucket_logging_data_callback;
    params.complete_callback     = &get_bucket_logging_complete_callback;
    params.callback_data         = logging_data;
    params.isCheckCA            = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.subResource          = "logging";
    params.temp_auth            = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket logging conf finish!");
}

void generate_redirect(update_bucket_common_data *website_data,
                obs_set_bucket_website_conf *set_bucket_website_conf, int i)
{
    
    int tmplen = 0;
    int mark = 0;
    
    if (set_bucket_website_conf->routingrule_info[i].protocol)
    {
        char* pprotocol = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].protocol,&pprotocol);
        tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<Protocol>%s</Protocol>", 
            mark ? pprotocol : set_bucket_website_conf->routingrule_info[i].protocol);
        website_data->docLen += tmplen;
        free(pprotocol);
    }
    if (set_bucket_website_conf->routingrule_info[i].host_name)
    {
        char* phostName = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].host_name,&phostName);
        tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,"<HostName>%s</HostName>", 
            mark ? phostName : set_bucket_website_conf->routingrule_info[i].host_name);
        website_data->docLen += tmplen;
        free(phostName);
    }
    if (set_bucket_website_conf->routingrule_info[i].replace_key_prefix_with)
    {
        char* preplaceKeyPrefixWith = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].replace_key_prefix_with,&preplaceKeyPrefixWith);
        tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<ReplaceKeyPrefixWith>%s</ReplaceKeyPrefixWith>", 
            mark ? preplaceKeyPrefixWith : set_bucket_website_conf->routingrule_info[i].replace_key_prefix_with);
        website_data->docLen += tmplen;
        free(preplaceKeyPrefixWith);
    }
    if (set_bucket_website_conf->routingrule_info[i].replace_key_with)
    {
        char* preplaceKeyWith = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].replace_key_with,&preplaceKeyWith);
        tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<ReplaceKeyWith>%s</ReplaceKeyWith>", 
            mark ? preplaceKeyWith : set_bucket_website_conf->routingrule_info[i].replace_key_with);
        website_data->docLen += tmplen;
        free(preplaceKeyWith);
    }
    if (set_bucket_website_conf->routingrule_info[i].http_redirect_code)
    {
        char*phttpRedirectCode = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].http_redirect_code,&phttpRedirectCode);
        tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<HttpRedirectCode>%s</HttpRedirectCode>", 
            mark ? phttpRedirectCode : set_bucket_website_conf->routingrule_info[i].http_redirect_code);
        website_data->docLen += tmplen;
        free(phttpRedirectCode);
    }
}

void generate_routingrules(update_bucket_common_data *website_data,
                obs_set_bucket_website_conf *set_bucket_website_conf)
{
    int tmplen = 0;
    int mark = 0;
    int i = 0;
    
    tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
    sizeof(website_data->doc) - website_data->docLen, 
    _TRUNCATE, "<RoutingRules>");
    website_data->docLen += tmplen;
    
    for(i = 0; i < set_bucket_website_conf->routingrule_count; i++)
    {
        tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen,
            _TRUNCATE, "<RoutingRule><Condition>");
        website_data->docLen += tmplen;
        if (set_bucket_website_conf->routingrule_info[i].key_prefix_equals)
        {
            char* pkeyPrefixEquals = 0;
            mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].key_prefix_equals,&pkeyPrefixEquals);
            tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
                sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
                "<KeyPrefixEquals>%s</KeyPrefixEquals>", 
                mark ? pkeyPrefixEquals : set_bucket_website_conf->routingrule_info[i].key_prefix_equals);
            website_data->docLen += tmplen;
            free(pkeyPrefixEquals);
        }
        if(set_bucket_website_conf->routingrule_info[i].http_errorcode_returned_equals) 
        {
            char* phttpErrorCodeReturnedEquals = 0;
            mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].http_errorcode_returned_equals,&phttpErrorCodeReturnedEquals);
            tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
                sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
                "<HttpErrorCodeReturnedEquals>%s</HttpErrorCodeReturnedEquals>",
                mark ? phttpErrorCodeReturnedEquals : set_bucket_website_conf->routingrule_info[i].http_errorcode_returned_equals);
            website_data->docLen += tmplen;
            free(phttpErrorCodeReturnedEquals);
        }

        tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen,
            _TRUNCATE,"</Condition><Redirect>");
            website_data->docLen += tmplen;

        generate_redirect(website_data, set_bucket_website_conf, i);
        
        tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen, 
            _TRUNCATE,"</Redirect></RoutingRule>");
        website_data->docLen += tmplen;
    }
    
    tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
        sizeof(website_data->doc) - website_data->docLen, 
        _TRUNCATE,"</RoutingRules>");
    website_data->docLen += tmplen;
    
}

obs_status generate_websiteconf_doc (update_bucket_common_data **data,
            obs_set_bucket_website_conf *set_bucket_website_conf,
            obs_response_handler *handler)
{
    update_bucket_common_data *website_data = *data;
    int tmplen = 0;
    int mark = 0;
        
    if(NULL == set_bucket_website_conf->suffix)
    {
        COMMLOG(OBS_LOGERROR, "set_bucket_website_conf suffix is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return OBS_STATUS_InvalidParameter;
    }

    website_data->docLen = snprintf_sec(website_data->doc, sizeof(website_data->doc), 
        _TRUNCATE, "<WebsiteConfiguration>");

    char* psuffix = 0;
    mark = pcre_replace(set_bucket_website_conf->suffix,&psuffix);
    tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
        sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
        "<IndexDocument><Suffix>%s</Suffix></IndexDocument>", 
        mark ? psuffix : set_bucket_website_conf->suffix);
    website_data->docLen += tmplen;
    free(psuffix);
    
    if (set_bucket_website_conf->key)
    {
        char*pkey = 0;
        mark = pcre_replace(set_bucket_website_conf->key,&pkey);

        tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE, 
            "<ErrorDocument><Key>%s</Key></ErrorDocument>", 
            mark ? pkey : set_bucket_website_conf->key);
        website_data->docLen += tmplen;
        free(pkey);
    }
    
    generate_routingrules(website_data, set_bucket_website_conf);
    
    tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
    sizeof(website_data->doc) - website_data->docLen, 
    _TRUNCATE,"</WebsiteConfiguration>");
    website_data->docLen += tmplen;
    website_data->docBytesWritten = 0;
    return OBS_STATUS_OK;
}

obs_status generate_website_redirctall_doc(update_bucket_common_data **data,
            obs_set_bucket_redirect_all_conf *set_bucket_redirect_all, 
            obs_response_handler *handler)
{
    
    int tmplen = 0;
    int mark = 0;
    update_bucket_common_data *website_data = *data;

    website_data->docLen = snprintf_sec(website_data->doc, sizeof(website_data->doc), 
        _TRUNCATE, "<WebsiteConfiguration>");
    
    if (website_data->docLen < 0)
    {
        COMMLOG(OBS_LOGERROR, "snprintf_s error!");
        return OBS_STATUS_InternalError;
    }
    
    if(NULL == set_bucket_redirect_all->host_name)
    {
        COMMLOG(OBS_LOGERROR, "set_bucket_redirect_all host_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return OBS_STATUS_InvalidParameter;
    }

    tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<RedirectAllRequestsTo>");
    website_data->docLen += tmplen;

    char* phostName = 0;
    mark = pcre_replace(set_bucket_redirect_all->host_name,&phostName);
    tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
        sizeof(website_data->doc)- website_data->docLen, _TRUNCATE,
        "<HostName>%s</HostName>", mark ? phostName : set_bucket_redirect_all->host_name);
    website_data->docLen += tmplen;
    free(phostName);
    phostName = NULL;

    if (set_bucket_redirect_all->protocol)
    {
        char*pprotocol = 0;
        mark = pcre_replace(set_bucket_redirect_all->protocol,&pprotocol);
        tmplen = snprintf_sec(website_data->doc + website_data->docLen, 
            sizeof(website_data->doc)- website_data->docLen, _TRUNCATE,
            "<Protocol>%s</Protocol>", mark ? pprotocol : set_bucket_redirect_all->protocol);
        website_data->docLen += tmplen;
        free(pprotocol);
        pprotocol = NULL;
    }

    tmplen = snprintf_sec((website_data->doc) + (website_data->docLen), 
        sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
        "</RedirectAllRequestsTo></WebsiteConfiguration>");
    website_data->docLen += tmplen;
    return OBS_STATUS_OK;

}

void set_bucket_website_configuration(const obs_options *options, 
                    obs_set_bucket_redirect_all_conf *set_bucket_redirect_all,
                    obs_set_bucket_website_conf *set_bucket_website_conf,
                    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_status status = OBS_STATUS_OK;
    COMMLOG(OBS_LOGINFO, "set bucket website start!");
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    update_bucket_common_data *website_data = (update_bucket_common_data*)malloc(sizeof(update_bucket_common_data));
    if (!website_data) 
    {
        COMMLOG(OBS_LOGERROR, "set: malloc website_configuration_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    memset_s(website_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));  
    website_data->complete_callback = handler->complete_callback;
    website_data->properties_callback = handler->properties_callback;
    website_data->callback_data = callback_data;
    
    if (set_bucket_redirect_all)
    {
        status = generate_website_redirctall_doc(&website_data, set_bucket_redirect_all, handler);
        if (status != OBS_STATUS_OK)
        {
            free(website_data);
            website_data = NULL;
            return;
        }
    }

    if (set_bucket_website_conf)
    {
        status = generate_websiteconf_doc(&website_data, set_bucket_website_conf, handler);
        if (status != OBS_STATUS_OK)
        {
            free(website_data);
            website_data = NULL;
            return;
        }
    }
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_put;
    params.properties_callback    = &update_bucket_common_properties_callback;
    params.toObsCallback          = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = website_data->docLen;
    params.complete_callback      = &update_bucket_common_complete_callback;
    params.callback_data          = website_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "website";
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket website configuration finish!");
}

void initialize_bucketwebsite(bucket_website *webdata)
{
    string_buffer_initialize(webdata->key_prefix_equals);
    string_buffer_initialize(webdata->http_errorcode_returned_equals);
    string_buffer_initialize(webdata->replace_key_prefix_with);
    string_buffer_initialize(webdata->replace_key_with);
    string_buffer_initialize(webdata->http_redirect_code);
    string_buffer_initialize(webdata->hostname);
    string_buffer_initialize(webdata->protocol);
}

void initialize_bucketwebsitedata(obs_bucket_website_configuration_data *data)
{
    data->webdata_count= 0;
    initialize_bucketwebsite(data->webdata);
}

obs_status make_get_bucket_websitedata_callback(obs_bucket_website_configuration_data *data)
{
    obs_status status = OBS_STATUS_OK;
    bucket_website_routingrule *websitein = NULL;
    if(data->webdata_count > 0)
    {
        websitein = (bucket_website_routingrule*)malloc(sizeof(bucket_website_routingrule) * data->webdata_count);
        if (NULL == websitein)
        {
            COMMLOG(OBS_LOGERROR, "malloc bucket_website_routingrule failed!");
            return OBS_STATUS_InternalError;
        }
        memset_s(websitein, sizeof(bucket_website_routingrule) * data->webdata_count, 
            0, sizeof(bucket_website_routingrule) * data->webdata_count);
    }

    int webdata_count = data->webdata_count;
    int i ;
    for (i = 0; i < webdata_count; i++) 
    {
        bucket_website_routingrule *website_dest = &(websitein[i]);
        bucket_website *website_src = &(data->webdata[i]);
        website_dest->key_prefix_equals = website_src->key_prefix_equals;
        website_dest->http_errorcode_returned_equals = website_src->http_errorcode_returned_equals;
        website_dest->replace_key_prefix_with = website_src->replace_key_prefix_with;
        website_dest->replace_key_with = website_src->replace_key_with;
        website_dest->http_redirect_code = website_src->http_redirect_code;
        website_dest->host_name = website_src->hostname;
        website_dest->protocol = website_src->protocol;
    }

    status = (*(data->get_bucket_websiteconf_callback))
    (data->hostname,data->protocol,data->suffix,data->key,websitein,webdata_count,data->callback_data);

    CHECK_NULL_FREE(websitein);
    return status;
}

obs_status get_bucket_websiteconf_xml_callback(const char *element_path, const char *data, 
                                int data_len, void *callback_data)
{
    obs_bucket_website_configuration_data *website_data = 
        (obs_bucket_website_configuration_data *) callback_data;

    int fit;

    if (!data)
    {
        if (!strcmp(element_path, "WebsiteConfiguration/RoutingRules/RoutingRule")) 
        {
            website_data->webdata_count++;
            if (website_data->webdata_count == MAX_WEBSITE) 
            {
                obs_status status = make_get_bucket_websitedata_callback(website_data);
                if (status != OBS_STATUS_OK) 
                {
                    return status;
                }
                initialize_bucketwebsitedata(website_data);
            }
            else 
            {
                initialize_bucketwebsite(&(website_data->webdata[website_data->webdata_count]));
            }
        }
        return OBS_STATUS_OK;
    }

    if(!strcmp(element_path, "WebsiteConfiguration/RedirectAllRequestsTo/HostName")) 
    {
        string_buffer_append(website_data->hostname, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/RedirectAllRequestsTo/Protocol"))
    {
        string_buffer_append(website_data->protocol, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/IndexDocument/Suffix"))
    {
        string_buffer_append(website_data->suffix, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/ErrorDocument/Key"))
    {
        string_buffer_append(website_data->key, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/RoutingRules/RoutingRule/Condition/KeyPrefixEquals"))
    {
        bucket_website* bws = &(website_data->webdata[website_data->webdata_count]);
        string_buffer_append(bws->key_prefix_equals, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/RoutingRules/RoutingRule/Condition/HttpErrorCodeReturnedEquals"))
    {
        bucket_website* bws = &(website_data->webdata[website_data->webdata_count]);
        string_buffer_append(bws->http_errorcode_returned_equals, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/RoutingRules/RoutingRule/Redirect/ReplaceKeyPrefixWith")){
        bucket_website* bws = &(website_data->webdata[website_data->webdata_count]);
        string_buffer_append(bws->replace_key_prefix_with, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/RoutingRules/RoutingRule/Redirect/HostName"))
    {
        bucket_website* bws = &(website_data->webdata[website_data->webdata_count]);
        string_buffer_append(bws->hostname, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/RoutingRules/RoutingRule/Redirect/Protocol"))
    {
        bucket_website* bws = &(website_data->webdata[website_data->webdata_count]);
        string_buffer_append(bws->protocol, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/RoutingRules/RoutingRule/Redirect/ReplaceKeyWith"))
    {
        bucket_website* bws = &(website_data->webdata[website_data->webdata_count]);
        string_buffer_append(bws->replace_key_with, data, data_len, fit);
    }
    else_if(!strcmp(element_path, "WebsiteConfiguration/RoutingRules/RoutingRule/Redirect/HttpRedirectCode"))
    {
        bucket_website* bws = &(website_data->webdata[website_data->webdata_count]);
        string_buffer_append(bws->http_redirect_code, data, data_len, fit);
    }

    (void) fit;

    return OBS_STATUS_OK;
}

obs_status get_bucket_websiteconf_properties_callback
    (const obs_response_properties *response_properties, void *callback_data)
{
    obs_bucket_website_configuration_data *websiteconf_data = 
        (obs_bucket_website_configuration_data *) callback_data;
    if (websiteconf_data->response_properties_callback)
    {
        return (*(websiteconf_data->response_properties_callback))
            (response_properties, websiteconf_data->callback_data);
    }
    return OBS_STATUS_OK;
}

obs_status get_bucket_websiteconf_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    obs_bucket_website_configuration_data *websiteconf_data = 
        (obs_bucket_website_configuration_data *) callback_data;
    return simplexml_add(&(websiteconf_data->simpleXml), buffer, buffer_size);
}

void get_bucket_websiteconf_complete_callback(obs_status status, const obs_error_details *error_details,
           void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    obs_bucket_website_configuration_data *websiteconf_data = 
        (obs_bucket_website_configuration_data *) callback_data;
    if (OBS_STATUS_OK == status)
    {
        make_get_bucket_websitedata_callback(websiteconf_data);
    }
    (*(websiteconf_data->response_complete_callback))
        (status, error_details, websiteconf_data->callback_data);
    simplexml_deinitialize(&(websiteconf_data->simpleXml));

    free(websiteconf_data);
    websiteconf_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void get_bucket_website_configuration(const obs_options *options, 
                    obs_get_bucket_websiteconf_handler *handler, 
                    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get bucket website configuration start!");
    obs_bucket_website_configuration_data *websiteconf_data = 
        (obs_bucket_website_configuration_data*) malloc(sizeof(obs_bucket_website_configuration_data));
    if (!websiteconf_data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc get websiteconf_datafailed !");
        return;
    }
    memset_s(websiteconf_data, sizeof(obs_bucket_website_configuration_data), 
        0, sizeof(obs_bucket_website_configuration_data));

    simplexml_initialize(&(websiteconf_data->simpleXml), &get_bucket_websiteconf_xml_callback, websiteconf_data);

    websiteconf_data->response_properties_callback = handler->response_handler.properties_callback;
    websiteconf_data->response_complete_callback = handler->response_handler.complete_callback;
    websiteconf_data->get_bucket_websiteconf_callback = handler->get_bucket_website_conf_callback;
    websiteconf_data->callback_data = callback_data;
    string_buffer_initialize(websiteconf_data->hostname);
    string_buffer_initialize(websiteconf_data->protocol);
    string_buffer_initialize(websiteconf_data->suffix);
    string_buffer_initialize(websiteconf_data->key);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.httpRequestType      = http_request_type_get;
    params.properties_callback   = &get_bucket_websiteconf_properties_callback;
    params.fromObsCallback      = &get_bucket_websiteconf_data_callback;
    params.complete_callback     = &get_bucket_websiteconf_complete_callback;
    params.callback_data         = websiteconf_data;
    params.isCheckCA            = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat   = no_need_storage_class;
    params.subResource          = "website";
    params.temp_auth            = options->temp_auth; 
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket website configuration finish!");
}

void delete_bucket_website_configuration(const obs_options *options, obs_response_handler *handler,
        void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "delete bucket website configuration start!");
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));
    
    params.httpRequestType       = http_request_type_delete;
    params.properties_callback   = handler->properties_callback;
    params.complete_callback     = handler->complete_callback;
    params.isCheckCA             = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat    = no_need_storage_class;
    params.subResource           = "website";
    params.temp_auth             = options->temp_auth; 
    params.callback_data         = callback_data;
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "delete bucket website configuration finish!");
}

/*************************set_notification_configuration*******************************/
static char* get_event_string_s3(obs_smn_event_enum event)
{
    switch (event)
    {
    case SMN_EVENT_OBJECT_CREATED_ALL:
        return "s3:ObjectCreated:*";
    case SMN_EVENT_OBJECT_CREATED_PUT:
        return "s3:ObjectCreated:Put";
    case SMN_EVENT_OBJECT_CREATED_POST:
        return "s3:ObjectCreated:Post";
    case SMN_EVENT_OBJECT_CREATED_COPY:
        return "s3:ObjectCreated:Copy";
    case SMN_EVENT_OBJECT_CREATED_COMPLETE_MULTIPART_UPLOAD:
        return "s3:ObjectCreated:CompleteMultipartUpload";
    case SMN_EVENT_OBJECT_REMOVED_ALL:
        return "s3:ObjectRemoved:*";
    case SMN_EVENT_OBJECT_REMOVED_DELETE:
        return "s3:ObjectRemoved:Delete";
    case SMN_EVENT_OBJECT_REMOVED_DELETE_MARKER_CREATED:
        return "s3:ObjectRemoved:DeleteMarkerCreated";
    case SMN_EVENT_REDUCED_REDUNDANCY_LOST_OBJECT:
        return "s3:ReducedRedundancyLostObject";
    default:
        return NULL;
    }
}

static char* get_event_string_obs(obs_smn_event_enum event)
{
    switch (event)
    {
    case SMN_EVENT_OBJECT_CREATED_ALL:
        return "ObjectCreated:*";
    case SMN_EVENT_OBJECT_CREATED_PUT:
        return "ObjectCreated:Put";
    case SMN_EVENT_OBJECT_CREATED_POST:
        return "ObjectCreated:Post";
    case SMN_EVENT_OBJECT_CREATED_COPY:
        return "ObjectCreated:Copy";
    case SMN_EVENT_OBJECT_CREATED_COMPLETE_MULTIPART_UPLOAD:
        return "ObjectCreated:CompleteMultipartUpload";
    case SMN_EVENT_OBJECT_REMOVED_ALL:
        return "ObjectRemoved:*";
    case SMN_EVENT_OBJECT_REMOVED_DELETE:
        return "ObjectRemoved:Delete";
    case SMN_EVENT_OBJECT_REMOVED_DELETE_MARKER_CREATED:
        return "ObjectRemoved:DeleteMarkerCreated";
    case SMN_EVENT_REDUCED_REDUNDANCY_LOST_OBJECT:
        return "ReducedRedundancyLostObject";
    default:
        return NULL;
    }
}


obs_smn_event_enum get_event_enum_s3(const char* event_string)
{
    if (!strcmp(event_string, "s3:ObjectCreated:*"))
    {
        return SMN_EVENT_OBJECT_CREATED_ALL;
    }
    else if (!strcmp(event_string, "s3:ObjectCreated:Put"))
    {
        return SMN_EVENT_OBJECT_CREATED_PUT;
    }
    else if (!strcmp(event_string, "s3:ObjectCreated:Post"))
    {
        return SMN_EVENT_OBJECT_CREATED_POST;
    }
    else if (!strcmp(event_string, "s3:ObjectCreated:Copy"))
    {
        return SMN_EVENT_OBJECT_CREATED_COPY;
    }
    else if (!strcmp(event_string, "s3:ObjectCreated:CompleteMultipartUpload"))
    {
        return SMN_EVENT_OBJECT_CREATED_COMPLETE_MULTIPART_UPLOAD;
    }
    else if (!strcmp(event_string, "s3:ObjectRemoved:*"))
    {
        return SMN_EVENT_OBJECT_REMOVED_ALL;
    }
    else if (!strcmp(event_string, "s3:ObjectRemoved:Delete"))
    {
        return SMN_EVENT_OBJECT_REMOVED_DELETE;
    }
    else if (!strcmp(event_string, "s3:ObjectRemoved:DeleteMarkerCreated"))
    {
        return SMN_EVENT_OBJECT_REMOVED_DELETE_MARKER_CREATED;
    }
    else if (!strcmp(event_string, "s3:ReducedRedundancyLostObject"))
    {
        return SMN_EVENT_REDUCED_REDUNDANCY_LOST_OBJECT;
    }

    return SMN_EVENT_NULL;
}

obs_smn_event_enum get_event_enum_obs(const char* event_string)
{
    if (!strcmp(event_string, "ObjectCreated:*"))
    {
        return SMN_EVENT_OBJECT_CREATED_ALL;
    }
    else if (!strcmp(event_string, "ObjectCreated:Put"))
    {
        return SMN_EVENT_OBJECT_CREATED_PUT;
    }
    else if (!strcmp(event_string, "ObjectCreated:Post"))
    {
        return SMN_EVENT_OBJECT_CREATED_POST;
    }
    else if (!strcmp(event_string, "ObjectCreated:Copy"))
    {
        return SMN_EVENT_OBJECT_CREATED_COPY;
    }
    else if (!strcmp(event_string, "ObjectCreated:CompleteMultipartUpload"))
    {
        return SMN_EVENT_OBJECT_CREATED_COMPLETE_MULTIPART_UPLOAD;
    }
    else if (!strcmp(event_string, "ObjectRemoved:*"))
    {
        return SMN_EVENT_OBJECT_REMOVED_ALL;
    }
    else if (!strcmp(event_string, "ObjectRemoved:Delete"))
    {
        return SMN_EVENT_OBJECT_REMOVED_DELETE;
    }
    else if (!strcmp(event_string, "ObjectRemoved:DeleteMarkerCreated"))
    {
        return SMN_EVENT_OBJECT_REMOVED_DELETE_MARKER_CREATED;
    }
    else if (!strcmp(event_string, "ReducedRedundancyLostObject"))
    {
        return SMN_EVENT_REDUCED_REDUNDANCY_LOST_OBJECT;
    }

    return SMN_EVENT_NULL;
}

static char* get_filter_rule_string(obs_smn_filter_rule_enum rule_name)
{
    switch (rule_name)
    {
    case OBS_SMN_FILTER_PREFIX:
        return "prefix";
    case OBS_SMN_FILTER_SUFFIX:
        return "suffix";
    default:
        return NULL;
    }
}

obs_smn_filter_rule_enum get_filter_rule_enum(const char* rule_string)
{
    if (!strcmp(rule_string, "prefix"))
    {
        return OBS_SMN_FILTER_PREFIX;
    }
    else if (!strcmp(rule_string, "suffix"))
    {
        return OBS_SMN_FILTER_SUFFIX;
    }

    return OBS_SMN_FILTER_NULL;
}

static obs_status set_notification_quest_xml_s3(obs_smn_notification_configuration* notification_conf, 
            set_notification_data *sncData)
{
    unsigned int uiIdx = 0;
    (void)add_xml_element(sncData->doc, &sncData->doc_len, "NotificationConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
    for (uiIdx = 0; uiIdx < notification_conf->topic_conf_num; uiIdx++)
    {
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "TopicConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
        (void)add_xml_element(sncData->doc, &sncData->doc_len,"Topic", 
                      notification_conf->topic_conf[uiIdx].topic, NEED_FORMALIZE, ADD_NAME_CONTENT); 
        (void)add_xml_element(sncData->doc, &sncData->doc_len,"Id", 
                      notification_conf->topic_conf[uiIdx].id, NEED_FORMALIZE, ADD_NAME_CONTENT); 
        unsigned int uiIdxEvent = 0;
        for (; uiIdxEvent< notification_conf->topic_conf[uiIdx].event_num; uiIdxEvent++)
        {
             (void)add_xml_element(sncData->doc, &sncData->doc_len,"Event", 
                      get_event_string_s3(notification_conf->topic_conf[uiIdx].event[uiIdxEvent]), 
                      NEED_FORMALIZE, ADD_NAME_CONTENT);
        }

        if (0 < notification_conf->topic_conf[uiIdx].filter_rule_num)
        {
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Filter", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "S3Key", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
            unsigned int uiIdxFilter = 0;
            for (; uiIdxFilter<notification_conf->topic_conf[uiIdx].filter_rule_num; uiIdxFilter++)
            {
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "FilterRule", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
                (void)add_xml_element(sncData->doc, &sncData->doc_len,"Name", get_filter_rule_string(
                        notification_conf->topic_conf[uiIdx].filter_rule[uiIdxFilter].name), 
                      NEED_FORMALIZE, ADD_NAME_CONTENT);
                (void)add_xml_element(sncData->doc, &sncData->doc_len,"Value", 
                      notification_conf->topic_conf[uiIdx].filter_rule[uiIdxFilter].value,
                      NEED_FORMALIZE, ADD_NAME_CONTENT); 
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "FilterRule", NULL, 
                                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
                
            }
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "S3Key", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Filter", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
        }

   
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "TopicConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    }
    
    (void)add_xml_element(sncData->doc, &sncData->doc_len, "NotificationConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    return OBS_STATUS_OK;
}

static obs_status set_notification_quest_xml_obs(obs_smn_notification_configuration* notification_conf, 
            set_notification_data *sncData)
{
    unsigned int uiIdx = 0;
    (void)add_xml_element(sncData->doc, &sncData->doc_len, "NotificationConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
    for (uiIdx = 0; uiIdx < notification_conf->topic_conf_num; uiIdx++)
    {
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "TopicConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
        (void)add_xml_element(sncData->doc, &sncData->doc_len,"Id", 
                      notification_conf->topic_conf[uiIdx].id, NEED_FORMALIZE, ADD_NAME_CONTENT); 
        (void)add_xml_element(sncData->doc, &sncData->doc_len,"Topic", 
                      notification_conf->topic_conf[uiIdx].topic, NEED_FORMALIZE, ADD_NAME_CONTENT);
        unsigned int uiIdxEvent = 0;
        for (; uiIdxEvent< notification_conf->topic_conf[uiIdx].event_num; uiIdxEvent++)
        {
             (void)add_xml_element(sncData->doc, &sncData->doc_len,"Event", 
                      get_event_string_obs(notification_conf->topic_conf[uiIdx].event[uiIdxEvent]), 
                      NEED_FORMALIZE, ADD_NAME_CONTENT);
        }

        if (0 < notification_conf->topic_conf[uiIdx].filter_rule_num)
        {
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Filter", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Object", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
            unsigned int uiIdxFilter = 0;
            for (; uiIdxFilter<notification_conf->topic_conf[uiIdx].filter_rule_num; uiIdxFilter++)
            {
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "FilterRule", NULL, 
                        NOT_NEED_FORMALIZE, ADD_HEAD_ONLY);
                (void)add_xml_element(sncData->doc, &sncData->doc_len,"Name", get_filter_rule_string(
                        notification_conf->topic_conf[uiIdx].filter_rule[uiIdxFilter].name), 
                      NEED_FORMALIZE, ADD_NAME_CONTENT);
                (void)add_xml_element(sncData->doc, &sncData->doc_len,"Value", 
                      notification_conf->topic_conf[uiIdx].filter_rule[uiIdxFilter].value,
                      NEED_FORMALIZE, ADD_NAME_CONTENT); 
                (void)add_xml_element(sncData->doc, &sncData->doc_len, "FilterRule", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
                
            }
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Object", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
            (void)add_xml_element(sncData->doc, &sncData->doc_len, "Filter", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
        }

   
        (void)add_xml_element(sncData->doc, &sncData->doc_len, "TopicConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    }
    
    (void)add_xml_element(sncData->doc, &sncData->doc_len, "NotificationConfiguration", NULL, 
                        NOT_NEED_FORMALIZE, ADD_TAIL_ONLY);
    return OBS_STATUS_OK;
}

static obs_status set_notification_quest_xml(obs_smn_notification_configuration* notification_conf, 
            set_notification_data *sncData, obs_use_api use_api)
{
    if (use_api == OBS_USE_API_S3) {
        return set_notification_quest_xml_s3(notification_conf, sncData);
    } else {
        return set_notification_quest_xml_obs(notification_conf, sncData);
    }
}

static set_notification_data* init_set_notification_data(
   obs_smn_notification_configuration* notification_conf, obs_response_handler *handler,
   void *callback_data, obs_use_api use_api)
{
    obs_status ret_status = OBS_STATUS_OK;
    set_notification_data *sncData    = NULL;
    
    sncData = (set_notification_data *) malloc(sizeof(set_notification_data));
    if (!sncData) {
        COMMLOG(OBS_LOGERROR, "malloc cors config data failed.");
        return NULL;
    }
    memset_s(sncData, sizeof(set_notification_data), 0, sizeof(set_notification_data));
    
    sncData->response_complete_callback    = handler->complete_callback;
    sncData->response_properties_callback  = handler->properties_callback;
    sncData->callback_data                 = callback_data;
    sncData->doc_len                       = 0;
    sncData->doc_bytes_written             = 0;
   
    ret_status = set_notification_quest_xml(notification_conf, sncData,use_api);
    if (OBS_STATUS_OK != ret_status || sncData->doc_len <= 0)
    {
        free(sncData);
        sncData = NULL;
        return NULL;
    }
    COMMLOG(OBS_LOGERROR, "request xml: %s.", sncData->doc);
    
    return sncData;
}

static int set_notification_data_callback(int buffer_size, char *buffer,
                                    void *callback_data)
{
    set_cors_config_data *sncData  = (set_cors_config_data *) callback_data;

    if (!sncData->doc_len)
    {
        return 0;
    }

    int remaining = (sncData->doc_len - sncData->doc_bytes_written);

    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy)
    {
        return 0;
    }

	errno_t err = EOK;  
	err = memcpy_s(buffer, buffer_size, &(sncData->doc[sncData->doc_bytes_written]), toCopy);
    if (err != EOK)
    {
		COMMLOG(OBS_LOGWARN, "set_notification_data_callback: memcpy_s failed!\n");
		return 0;
    }
	
    sncData->doc_bytes_written += toCopy;

    return toCopy;
}

static obs_status set_notification_properties_callback(
    const obs_response_properties *response_properties, void *callback_data)
{
    set_notification_data *sncData = (set_notification_data *) callback_data;
    if (sncData->response_properties_callback)
    {
        return (*(sncData->response_properties_callback))(response_properties, 
                        sncData->callback_data);
    }

    return OBS_STATUS_OK;
}


static void set_notification_complete_callback(obs_status request_status,
                                         const obs_error_details *obs_error_info,
                                         void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    set_notification_data *sncData = (set_notification_data *) callback_data;

    (*(sncData->response_complete_callback))(request_status, obs_error_info, sncData->callback_data);

    CHECK_NULL_FREE(sncData);
    sncData = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void set_notification_configuration(const obs_options *options, 
    obs_smn_notification_configuration* notification_conf, obs_response_handler *handler, 
    void *callback_data)
{
    request_params          params;
    set_notification_data   *sncData   = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "set_notification_configuration start !");

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }
    
    if(!notification_conf)
    {
        COMMLOG(OBS_LOGERROR, "set_notification faied, notification_conf is null.");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return;
    }

    sncData = init_set_notification_data(notification_conf, handler, callback_data, use_api);
    if (!sncData) 
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc sncData failed.");
        return;
    }

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
             sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
             sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_put;
    params.properties_callback     = &set_notification_properties_callback;
    params.toObsCallback          = &set_notification_data_callback;
    params.complete_callback       = &set_notification_complete_callback;
    params.toObsCallbackTotalSize = sncData->doc_len;
    params.callback_data           = sncData;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "notification";
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set_notification_configuration finish.");
}


/*****************get_notification_configuration*****************************************/
static obs_status malloc_smn_data_s3(const char *element_path, get_smn_data *smn_data)
{
    int nTopicConfIdx = smn_data->notification_conf.topic_conf_num;
    
    if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/S3Key/FilterRule"))
    {
        smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num++;
        int alloc_size = (sizeof(obs_smn_filter_rule)) *
                    (smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num + 1);
        obs_smn_filter_rule* tmpFilter = (obs_smn_filter_rule*)malloc(alloc_size);
        if (NULL == tmpFilter)
        {
            COMMLOG(OBS_LOGERROR, "malloc obs_smn_filter_rule failed.");
            return OBS_STATUS_OutOfMemory;
        }

        memset_s(tmpFilter, alloc_size, 0, alloc_size);
        errno_t err = EOK;
		err = memcpy_s(tmpFilter, alloc_size, smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule,
                sizeof(obs_smn_filter_rule) * 
                smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num);
        free(smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule);
        smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule = tmpFilter;
		
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "malloc_smn_data_s3: memcpy_s failed!\n");
			return OBS_STATUS_OutOfMemory;
		}
    }
    else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration"))
    {
        smn_data->notification_conf.topic_conf_num++;
        int alloc_topic_size = sizeof(obs_smn_topic_configuration) * 
            (smn_data->notification_conf.topic_conf_num + 1);
        obs_smn_topic_configuration* tmpTopicConf = 
                (obs_smn_topic_configuration*)malloc(alloc_topic_size);
        if (NULL == tmpTopicConf)
        {
            COMMLOG(OBS_LOGERROR, "malloc smn topic failed.");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(tmpTopicConf, alloc_topic_size, 0, alloc_topic_size);

        int nTmpIdx = smn_data->notification_conf.topic_conf_num;
        (tmpTopicConf+nTmpIdx)->filter_rule = (obs_smn_filter_rule*) malloc(sizeof(obs_smn_filter_rule));
        if (!(tmpTopicConf+nTmpIdx)->filter_rule)
        {
            COMMLOG(OBS_LOGERROR, "malloc filter_rule failed.");
            CHECK_NULL_FREE(tmpTopicConf);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s((tmpTopicConf+nTmpIdx)->filter_rule, sizeof(obs_smn_filter_rule), 0, 
            sizeof(obs_smn_filter_rule));

        (tmpTopicConf+nTmpIdx)->event = (obs_smn_event_enum*) malloc(sizeof(obs_smn_event_enum));
        if (!(tmpTopicConf+nTmpIdx)->event)
        {
            COMMLOG(OBS_LOGERROR, "malloc notify topic event failed.");
            CHECK_NULL_FREE((tmpTopicConf+nTmpIdx)->filter_rule);
            CHECK_NULL_FREE(tmpTopicConf);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s((tmpTopicConf+nTmpIdx)->event, sizeof(obs_smn_event_enum), 0, sizeof(obs_smn_event_enum));

        errno_t err = EOK;  
		err = memcpy_s(tmpTopicConf, alloc_topic_size, smn_data->notification_conf.topic_conf,
            sizeof(obs_smn_topic_configuration) * nTmpIdx);		
        free(smn_data->notification_conf.topic_conf);
        smn_data->notification_conf.topic_conf = tmpTopicConf;
		
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "malloc_smn_data_s3: memcpy_s failed!\n");
			return OBS_STATUS_OutOfMemory;
		}
    }
    else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Event"))
    {
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num++;
        int alloc_event_size = sizeof(obs_smn_event_enum) * 
            (smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num + 1);
        obs_smn_event_enum* tmpEvent = (obs_smn_event_enum*)malloc(alloc_event_size);
        if (NULL == tmpEvent)
        {
            COMMLOG(OBS_LOGERROR, "malloc notify conf failed !");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(tmpEvent, alloc_event_size, 0, alloc_event_size);

		errno_t err = EOK;  
		err = memcpy_s(tmpEvent, alloc_event_size,
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event, sizeof(obs_smn_event_enum) * 
        (smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num));
        free(smn_data->notification_conf.topic_conf[nTopicConfIdx].event);		
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event = tmpEvent;
		
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "malloc_smn_data_s3: memcpy_s failed!\n");
			return OBS_STATUS_OutOfMemory;
		}
    }

    return OBS_STATUS_OK;
}

static obs_status malloc_smn_data_obs(const char *element_path, get_smn_data *smn_data)
{
    int nTopicConfIdx = smn_data->notification_conf.topic_conf_num;
	errno_t err = EOK;
    
    if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/Object/FilterRule"))
    {
        smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num++;
        int alloc_size = (sizeof(obs_smn_filter_rule)) *
                    (smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num + 1);
        obs_smn_filter_rule* tmpFilter = (obs_smn_filter_rule*)malloc(alloc_size);
        if (NULL == tmpFilter)
        {
            COMMLOG(OBS_LOGERROR, "malloc obs_smn_filter_rule failed.");
            return OBS_STATUS_OutOfMemory;
        }

        memset_s(tmpFilter, alloc_size, 0, alloc_size);
        err = memcpy_s(tmpFilter, alloc_size, smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule,
                sizeof(obs_smn_filter_rule) * 
                smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num);
		
        free(smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule);
        smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule = tmpFilter;
		
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "Malloc_smn_data_obs: memcpy_s failed!\n");
			return OBS_STATUS_OutOfMemory;
		}	
    }
    else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration"))
    {
        smn_data->notification_conf.topic_conf_num++;
        int alloc_topic_size = sizeof(obs_smn_topic_configuration) * 
            (smn_data->notification_conf.topic_conf_num + 1);
        obs_smn_topic_configuration* tmpTopicConf = 
                (obs_smn_topic_configuration*)malloc(alloc_topic_size);
        if (NULL == tmpTopicConf)
        {
            COMMLOG(OBS_LOGERROR, "malloc smn topic failed.");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(tmpTopicConf, alloc_topic_size, 0, alloc_topic_size);

        int nTmpIdx = smn_data->notification_conf.topic_conf_num;
        (tmpTopicConf+nTmpIdx)->filter_rule = (obs_smn_filter_rule*) malloc(sizeof(obs_smn_filter_rule));
        if (!(tmpTopicConf+nTmpIdx)->filter_rule)
        {
            COMMLOG(OBS_LOGERROR, "malloc filter_rule failed.");
            CHECK_NULL_FREE(tmpTopicConf);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s((tmpTopicConf+nTmpIdx)->filter_rule, sizeof(obs_smn_filter_rule), 0, 
            sizeof(obs_smn_filter_rule));

        (tmpTopicConf+nTmpIdx)->event = (obs_smn_event_enum*) malloc(sizeof(obs_smn_event_enum));
        if (!(tmpTopicConf+nTmpIdx)->event)
        {
            COMMLOG(OBS_LOGERROR, "malloc notify topic event failed.");
            CHECK_NULL_FREE((tmpTopicConf+nTmpIdx)->filter_rule);
            CHECK_NULL_FREE(tmpTopicConf);
            return OBS_STATUS_OutOfMemory;
        }
        memset_s((tmpTopicConf+nTmpIdx)->event, sizeof(obs_smn_event_enum), 0, sizeof(obs_smn_event_enum));

        err = memcpy_s(tmpTopicConf, alloc_topic_size, smn_data->notification_conf.topic_conf,
            sizeof(obs_smn_topic_configuration) * nTmpIdx);

		free(smn_data->notification_conf.topic_conf);
        smn_data->notification_conf.topic_conf = tmpTopicConf;
		
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "Malloc_smn_data_obs: memcpy_s failed!\n");
			return OBS_STATUS_OutOfMemory;
		}
    }
    else if (!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Event"))
    {
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num++;
        int alloc_event_size = sizeof(obs_smn_event_enum) * 
            (smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num + 1);
        obs_smn_event_enum* tmpEvent = (obs_smn_event_enum*)malloc(alloc_event_size);
        if (NULL == tmpEvent)
        {
            COMMLOG(OBS_LOGERROR, "malloc notify conf failed !");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(tmpEvent, alloc_event_size, 0, alloc_event_size);
        
		err = memcpy_s(tmpEvent, alloc_event_size,
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event, sizeof(obs_smn_event_enum) * 
        (smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num));
		
		free(smn_data->notification_conf.topic_conf[nTopicConfIdx].event);
        smn_data->notification_conf.topic_conf[nTopicConfIdx].event = tmpEvent;
		
		if (err != EOK)
		{
			COMMLOG(OBS_LOGWARN, "Malloc_smn_data_obs: memcpy_s failed!\n");
			return OBS_STATUS_OutOfMemory;
		}
    }

    return OBS_STATUS_OK;
}

static obs_status get_notification_xml_s3_callback(const char *element_path,
        const char *data, int data_len, void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *)callback_data;
    int nTopicConfIdx = smn_data->notification_conf.topic_conf_num;
    int nFilterRuleIdx = smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num;
    int nEventIdx = smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num;
    obs_status ret_status = OBS_STATUS_OK;

    if (data)
    {
        if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Topic"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].topic, data, data_len);
        }
        else if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Id"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].id, 
                            data, data_len);
        }
        else if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Event"))
        {
            char* str_event = NULL;
            malloc_buffer_append(str_event, data, data_len);
            smn_data->notification_conf.topic_conf[nTopicConfIdx].event[nEventIdx] = 
                        get_event_enum_s3(str_event);
            CHECK_NULL_FREE(str_event);
        }
        else if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/S3Key/FilterRule/Name"))
        {
            char* str_name = NULL;
            malloc_buffer_append(str_name, data, data_len);
            smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule[nFilterRuleIdx].name = 
                        get_filter_rule_enum(str_name);
            CHECK_NULL_FREE(str_name);
        }
        else if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/S3Key/FilterRule/Value"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule[nFilterRuleIdx].value, 
                    data, data_len);
        }
    }
    else
    {
        ret_status = malloc_smn_data_s3(element_path, smn_data);
        if (OBS_STATUS_OK != ret_status)
        {
            return ret_status;
        }
    }
    return OBS_STATUS_OK;
}

static obs_status get_notification_xml_obs_callback(const char *element_path,
        const char *data, int data_len, void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *)callback_data;
    int nTopicConfIdx = smn_data->notification_conf.topic_conf_num;
    int nFilterRuleIdx = smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule_num;
    int nEventIdx = smn_data->notification_conf.topic_conf[nTopicConfIdx].event_num;
    obs_status ret_status = OBS_STATUS_OK;

    if (data)
    {
        if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Topic"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].topic, data, data_len);
        }
        else if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Id"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].id, 
                            data, data_len);
        }
        else if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Event"))
        {
            char* str_event = NULL;
            malloc_buffer_append(str_event, data, data_len);
            smn_data->notification_conf.topic_conf[nTopicConfIdx].event[nEventIdx] = 
                        get_event_enum_obs(str_event);
            CHECK_NULL_FREE(str_event);
        }
        else if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/Object/FilterRule/Name"))
        {
            char* str_name = NULL;
            malloc_buffer_append(str_name, data, data_len);
            smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule[nFilterRuleIdx].name = 
                        get_filter_rule_enum(str_name);
            CHECK_NULL_FREE(str_name);
        }
        else if(!strcmp(element_path, "NotificationConfiguration/TopicConfiguration/Filter/Object/FilterRule/Value"))
        {
            malloc_buffer_append(smn_data->notification_conf.topic_conf[nTopicConfIdx].filter_rule[nFilterRuleIdx].value, 
                    data, data_len);
        }
    }
    else
    {
        ret_status = malloc_smn_data_obs(element_path, smn_data);
        if (OBS_STATUS_OK != ret_status)
        {
            return ret_status;
        }
    }
    return OBS_STATUS_OK;
}

static obs_status get_notification_xml_callback(const char *element_path,
        const char *data, int data_len, void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *)callback_data;
    if (smn_data->use_api == OBS_USE_API_S3) {
        return get_notification_xml_s3_callback(element_path, data, data_len, callback_data);
    } else {
        return get_notification_xml_obs_callback(element_path, data, data_len, callback_data);
    }
}

static get_smn_data* init_get_smn_data(obs_smn_handler *handler,  void *callback_data, obs_use_api use_api)
{
    get_smn_data *smn_data = (get_smn_data *) malloc(sizeof(get_smn_data));
    if (!smn_data) 
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc get_smn_data failed.");
        return NULL;
    }
    memset_s(smn_data, sizeof(get_smn_data), 0, sizeof(get_smn_data));

    smn_data->notification_conf.topic_conf = 
        (obs_smn_topic_configuration*) malloc(sizeof(obs_smn_topic_configuration));
    if (!smn_data->notification_conf.topic_conf) 
    {
        free(smn_data);
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc obs_smn_topic_configuration failed.");
        return NULL;
    }
    memset_s(smn_data->notification_conf.topic_conf, sizeof(obs_smn_topic_configuration), 0,
            sizeof(obs_smn_topic_configuration));
    smn_data->notification_conf.topic_conf_num = 0;

    smn_data->notification_conf.topic_conf->filter_rule = 
                (obs_smn_filter_rule*)malloc(sizeof(obs_smn_filter_rule));
    if (!smn_data->notification_conf.topic_conf->filter_rule)
    {
        free(smn_data->notification_conf.topic_conf);
        free(smn_data);
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc obs_smn_filter_rule failed.");
        return NULL;
    }
    memset_s(smn_data->notification_conf.topic_conf->filter_rule, sizeof(obs_smn_filter_rule), 0,
            sizeof(obs_smn_filter_rule));
    smn_data->notification_conf.topic_conf->filter_rule_num = 0;

    
    smn_data->notification_conf.topic_conf->event = 
                (obs_smn_event_enum*)malloc(sizeof(obs_smn_event_enum));
    if (!smn_data->notification_conf.topic_conf->event)
    {
        free(smn_data->notification_conf.topic_conf->filter_rule);
        free(smn_data->notification_conf.topic_conf);
        free(smn_data);
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        COMMLOG(OBS_LOGERROR, "malloc obs_smn_event_enum failed.");
        return NULL;
    }
    memset_s(smn_data->notification_conf.topic_conf->event, sizeof(obs_smn_event_enum), 0,
            sizeof(obs_smn_event_enum));
    smn_data->notification_conf.topic_conf->event_num = 0;
    smn_data->notification_conf.topic_conf->event[0] = SMN_EVENT_NULL;

    smn_data->use_api = use_api;
    simplexml_initialize(&(smn_data->simple_xml_info), &get_notification_xml_callback, smn_data);
    smn_data->response_properties_callback  = handler->response_handler.properties_callback;
    smn_data->response_complete_callback    = handler->response_handler.complete_callback;
    smn_data->get_smn_callback_func         = handler->get_smn_callback_func;
    smn_data->callback_data                 = callback_data;
    
    return smn_data;
}


static obs_status get_notification_properties_callback(
    const obs_response_properties *response_properties, void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *) callback_data;
    if (smn_data->response_properties_callback)
    {
        return (*(smn_data->response_properties_callback))(response_properties, 
                        smn_data->callback_data);
    }

    return OBS_STATUS_OK;
}

static obs_status get_notification_data_callback(int buffer_size, const char *buffer, 
        void *callback_data)
{
    get_smn_data *smn_data = (get_smn_data *) callback_data;

    return simplexml_add(&(smn_data->simple_xml_info), buffer, buffer_size);
}


static void get_notification_complete_callback(obs_status request_status,
                                         const obs_error_details *obs_error_info,
                                         void *callback_data)
{
    unsigned int topic_loop = 0;
    unsigned int filter_loop = 0;
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    
    get_smn_data *smn_data = (get_smn_data *) callback_data;
    if (OBS_STATUS_OK == request_status && smn_data->get_smn_callback_func)
    {
        request_status = (*(smn_data->get_smn_callback_func))(&smn_data->notification_conf, 
                        smn_data->callback_data);
    }

    (*(smn_data->response_complete_callback))(request_status, obs_error_info, 
                        smn_data->callback_data);
    
    for(topic_loop=0; topic_loop < smn_data->notification_conf.topic_conf_num; topic_loop++)
    {
        CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].id);
        CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].topic);
        for (filter_loop=0; filter_loop<smn_data->notification_conf.topic_conf[topic_loop].filter_rule_num; filter_loop++)
        {  
            CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].filter_rule[filter_loop].value);
        }
        CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].filter_rule);
        CHECK_NULL_FREE(smn_data->notification_conf.topic_conf[topic_loop].event);
    }
    CHECK_NULL_FREE(smn_data->notification_conf.topic_conf); 

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}




void get_notification_configuration(const obs_options *options,
                            obs_smn_handler *handler,  void *callback_data)
{
    request_params  params;
    get_smn_data    *smn_data = NULL;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get_bucket_cors_configuration start !");

    if(!options->bucket_options.bucket_name){
        COMMLOG(OBS_LOGERROR, "bucket_name is NULL.");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidBucketName, 0, 0);
        return;
    }

    smn_data = init_get_smn_data(handler, callback_data,use_api);
    if (NULL == smn_data) 
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
             sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
             sizeof(obs_http_request_option));

    params.httpRequestType        = http_request_type_get;
    params.properties_callback     = &get_notification_properties_callback;
    params.fromObsCallback        = &get_notification_data_callback;
    params.complete_callback       = &get_notification_complete_callback;
    params.callback_data           = smn_data;
    params.isCheckCA              = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat     = no_need_storage_class;
    params.subResource            = "notification";
    params.temp_auth              = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_lifecycle_configuration finish.");
}


void set_bucket_acl(const obs_options * options, manager_acl_info * aclinfo, 
                    obs_response_handler * handler, void *callback_data)
{
    obs_type_acl type = TYPE_BUCKET_ACL;
    set_common_acl(options, aclinfo, type, handler, callback_data);
    COMMLOG(OBS_LOGINFO, "set bucket acl finish!");
}

void set_bucket_acl_by_head(const obs_options * options, obs_canned_acl canned_acl, 
                            obs_response_handler * handler, void *callback_data)
{
    set_object_acl_by_head(options, NULL, canned_acl, handler,callback_data);
    COMMLOG(OBS_LOGINFO, "set bucket acl by head finish!");
}

void get_bucket_acl(const obs_options * options, manager_acl_info * aclinfo, 
                    obs_response_handler * handler, void *callback_data)
{
    get_object_acl(options, aclinfo, handler, callback_data);   
    COMMLOG(OBS_LOGINFO, "get bucket acl finish!");
}

void obs_options_bucket(const obs_options *options, char* origin,
                    char (*request_method)[OBS_COMMON_LEN_256], unsigned int method_number,
                    char (*request_header)[OBS_COMMON_LEN_256], unsigned int header_number,
                    obs_response_handler *handler, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    obs_options_obj_or_bucket(options, 1, NULL, origin, request_method, method_number,
        request_header, header_number, handler, callback_data);

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);

}

void obs_head_bucket(const obs_options *options, obs_response_handler *handler, void *callback_data)
{ 
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    obs_head_object(options, 0, handler, callback_data);
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void get_bucket_metadata_with_corsconf(const obs_options *options, char *origin,
                    char (*requestHeader)[OBS_COMMON_LEN_256], unsigned int number, 
                    obs_response_handler *handler)
{
    request_params params;
    unsigned int i = 0;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    COMMLOG(OBS_LOGINFO, "get bucket metadata with corsconf start!");
    
    obs_cors_conf cors_conf;
    
    memset_s(&cors_conf, sizeof(cors_conf), 0, sizeof(cors_conf));

    cors_conf.origin = origin;
    cors_conf.rhNumber = number;
    for(; i < number; i++)
    {
        cors_conf.requestHeader[i] = requestHeader[i];
    }
    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options, 
        sizeof(obs_bucket_context));
    memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options, 
        sizeof(obs_http_request_option));

    params.httpRequestType         = http_request_type_head;
    params.properties_callback     = handler->properties_callback;
    params.complete_callback       = handler->complete_callback;
    params.isCheckCA               = options->bucket_options.certificate_info ? 1 : 0;
    params.storageClassFormat      = no_need_storage_class;
    params.corsConf                = &cors_conf;
    params.temp_auth               = options->temp_auth; 
    params.use_api =use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket metadata with corsconf finish!");
}
