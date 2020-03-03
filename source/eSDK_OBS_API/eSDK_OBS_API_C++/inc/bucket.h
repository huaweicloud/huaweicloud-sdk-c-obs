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
#include "common.h"


#define SECONDS_TO_AN_HOUR 3600

#ifdef WIN32
# pragma warning (disable:4819)
# pragma warning (disable:4127)
#endif

// We read up to 32 Contents at a time
#define MAX_CONTENTS 32
// We read up to 8 CommonPrefixes at a time
#define MAX_COMMON_PREFIXES 8
#define MAX_VERSIONS 32
#define MAX_VERSION_COMMON_PREFIXES 64
#define MAX_UPLOADS 32
#define D_MAX_RULE_NUMBER 100
#define BLS_XML_DOC_MAXSIZE (64 * 1024)
#define MAX_NAME_LEN  256
#define MAX_NUM_TAGGING 10

#define MAX_COUNT_ONE_CORS_RULE 20
#define MAX_CORS_RULE 100
#define BASE64_MD5_LEN 64
#define MD5_LEN 16
#define PFS_BUCKET_MIN_QUOTA 1099511627776

#define else_if else if


char * g_storage_class_s3[OBS_STORAGE_CLASS_BUTT] = 
{
    "STANDARD",
    "STANDARD_IA",
    "GLACIER"
};

char * g_storage_class_obs[OBS_STORAGE_CLASS_BUTT] = 
{
    "STANDARD",
    "WARM",
    "COLD"
};

typedef struct update_bucket_common_data
{
    obs_response_properties_callback *properties_callback;
    obs_response_complete_callback *complete_callback;
    char doc[1024];
    int docLen;
    int docBytesWritten;
    void *callback_data;
} update_bucket_common_data;

typedef struct xml_callback_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_list_service_callback *listServiceCallback;
    obs_response_complete_callback *responseCompleteCallback;
    void *callback_data;

    string_buffer(owner_id, 256);
    string_buffer(owner_display_name, 256);
    string_buffer(bucket_name, 256);
    string_buffer(creationDate, 128);
} xml_callback_data;

typedef struct xml_obs_callback_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_list_service_obs_callback *listServiceCallback;
    obs_response_complete_callback *responseCompleteCallback;
    void *callback_data;

    string_buffer(owner_id, 256);
    string_buffer(bucket_name, 256);
    string_buffer(creationDate, 128);
    string_buffer(location, 256);
    string_buffer(bucketType, 64);
} xml_obs_callback_data;

typedef struct one_object_content
{
    string_buffer(key, 1024);
    string_buffer(last_modified, 256);
    string_buffer(etag, 256);
    string_buffer(size, 24);
    string_buffer(owner_id, 256);
    string_buffer(owner_display_name, 256);
    string_buffer(storage_class, 16);
    string_buffer(type, 16);
} one_object_content;

typedef struct list_common_prefixes
{
    string_buffer(prefix, 1024);
}list_common_prefixes;


typedef struct list_objects_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_list_objects_callback        *listObjectCallback;
    obs_response_complete_callback   *responseCompleteCallback;
    void *callback_data;

    string_buffer(is_truncated, 64);
    string_buffer(next_marker, 1024);

    int contents_count;
    one_object_content contents[MAX_CONTENTS];

    int common_prefixes_count;
    char common_prefixes[MAX_COMMON_PREFIXES][1024];
    int commonPrefixLens[MAX_COMMON_PREFIXES];
} list_objects_data;


typedef struct list_bucket_versions
{
    string_buffer(key, 1024);
    string_buffer(version_id, 256);
    string_buffer(is_latest, 64);
    string_buffer(last_modified, 256);
    string_buffer(etag, 256);
    string_buffer(size, 24);
    string_buffer(owner_id, 256);
    string_buffer(owner_display_name, 256);
    string_buffer(storage_class_value, 64);
    string_buffer(is_delete, 64);
} list_bucket_versions;


typedef struct list_versions_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_list_versions_callback       *listVersionsCallback;
    obs_response_complete_callback   *responseCompleteCallback;
    void *callback_data;


    string_buffer(next_key_marker, 1024);
    string_buffer(next_versionId_marker, 1024);
    string_buffer(is_truncated, 64);

    string_buffer(bucket_name, 1024);
    string_buffer(prefix, 1024);
    string_buffer(key_marker, 64);
    string_buffer(delimiter, 64);
    string_buffer(max_keys, 32);

    int versions_count;
    list_bucket_versions versions[MAX_VERSIONS];

    int common_prefixes_count;
    list_common_prefixes common_prefixes[MAX_VERSION_COMMON_PREFIXES];    
}list_versions_data;

typedef struct get_bucket_common_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_response_complete_callback *responseCompleteCallback;
    void *callback_data;

    int common_data_return_size;
    char *common_data_return;
    uint64_t *ul_return;
    
    string_buffer(common_data, 256);
} get_bucket_common_data;

typedef struct get_bucket_policy_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_response_complete_callback *responseCompleteCallback;
    void *callback_data;

    int policyReturnSize;
    char *policyReturn;

    string_buffer(policy, 1024);
} get_bucket_policy_data;

typedef struct get_bucket_storageInfo_data
{
    simple_xml simpleXml;
    obs_response_properties_callback *properties_callback;
    obs_response_complete_callback *complete_callback;
    
    char *capacity_return;
    int   capacity_length;
    char *object_number_return;
    int   object_number_length;
    
    string_buffer(size, 256);
    string_buffer(objectnumber, 256);
    void *callback_data;
} get_bucket_storageInfo_data;

typedef struct get_bucket_storage_class_policy_data
{
    simple_xml simpleXml;
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    obs_get_bucket_storage_policy_callback *response_bucket_sorage_policy_callback;
    void *callback_data;
    obs_use_api use_api;

    string_buffer(storage_class_policy,15);     
} get_bucket_storage_class_policy_data;

typedef struct tagging_kv
{
    string_buffer(key, MAX_NAME_LEN);
    string_buffer(value, MAX_NAME_LEN);
} tagging_kv;

typedef struct get_bucket_tagging_data
{
    simple_xml simpleXml;
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    obs_get_bucket_tagging_callback *response_tagging_list_callback;
    void *callback_data;
    int tagging_return_size;
    char *tagging_return;

    int tagging_count;
    tagging_kv tagging_list[MAX_NUM_TAGGING]; 
} get_bucket_tagging_data;

typedef struct multipart_upload_info
{
    string_buffer(key, 1024);
    string_buffer(upload_id, 256);
    string_buffer(initiator_id, 256);
    string_buffer(initiator_display_name, 256);
    string_buffer(owner_id, 256);
    string_buffer(owner_display_name, 256);
    string_buffer(storage_class, 256);
    string_buffer(initiated, 256);
} multipart_upload_info;

typedef struct list_multipart_uploads_data
{
    simple_xml simpleXml;

    obs_response_properties_callback    *response_properties_callback;
    obs_list_multipart_uploads_callback *list_mulpu_callback;
    obs_response_complete_callback      *response_complete_callback;
    void *callback_data;

    string_buffer(is_truncated, 64);
    string_buffer(next_marker, 1024);
    string_buffer(next_uploadId_marker, 1024);

    int uploads_count;
    multipart_upload_info uploads[MAX_UPLOADS];

    int common_prefixes_count;
    list_common_prefixes common_prefixes[MAX_VERSION_COMMON_PREFIXES];    
} list_multipart_uploads_data;


typedef struct set_lifecycle_data
{
    obs_response_properties_callback    *response_properties_callback;
    obs_response_complete_callback      *response_complete_callback;
    void *callback_data;
        
    char doc[MAX_XML_LEN];
    int docLen, docBytesWritten;
    char doc_md5[64];
} set_lifecycle_data;


typedef struct set_common_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    int xml_document_len;
    char xml_document[BLS_XML_DOC_MAXSIZE];
    int xml_document_bytes_written;
    void *callback_data;
} set_common_data;



typedef struct transition_data
{
    string_buffer(date, 63);
    string_buffer(days, 63);
    string_buffer(storage_class, 15);
}transition_data;


typedef struct non_current_version_transition_data
{
    string_buffer(days, 63);
    string_buffer(storage_class, 15);
}non_current_version_transition_data;   

typedef struct lifecycle_conf_data
{
    string_buffer(date, 63);
    string_buffer(days, 63);
    string_buffer(id, 256);
    string_buffer(prefix, 256);
    string_buffer(status, 63);
    string_buffer(nonCurrentVerionDays,63);
    transition_data arrTransitionData[2];
    non_current_version_transition_data arrNonCurrentVersionTransitionData[2];
    int transition_num;
    int noncurrent_version_transition_num;
}lifecycle_conf_data;


typedef struct get_lifecycle_config_data
{
    simple_xml simple_xml_info;
    obs_response_properties_callback    *response_properties_callback;
    obs_response_complete_callback      *response_complete_callback;
    get_lifecycle_configuration_callback *get_lifecycle_callback;
    void *callback_data;

    lifecycle_conf_data* blcc_data[D_MAX_RULE_NUMBER];
    unsigned int blcc_number;
    obs_use_api use_api;
} get_lifecycle_config_data;

#define MAX_WEBSITE 10
typedef struct bucket_website
{
    string_buffer(key_prefix_equals, 256);
    string_buffer(http_errorcode_returned_equals, 256);
    string_buffer(replace_key_prefix_with, 256);
    string_buffer(replace_key_with, 256);
    string_buffer(http_redirect_code, 256);
    string_buffer(hostname, 256);
    string_buffer(protocol, 256);
} bucket_website;

typedef struct obs_bucket_website_configuration_data
{
    simple_xml simpleXml;
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    obs_get_bucket_websiteconf_callback *get_bucket_websiteconf_callback;
    void *callback_data;
    string_buffer(hostname, 256);
    string_buffer(protocol, 256);
    string_buffer(suffix, 256);
    string_buffer(key, 256);
    bucket_website webdata[MAX_WEBSITE];
    int webdata_count;

} obs_bucket_website_configuration_data;


typedef struct set_cors_config_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback   *response_complete_callback;
    void *callback_data;

    char doc[1024*100];
    int  doc_len, doc_bytes_written;
    char doc_md5[64];
}set_cors_config_data;


typedef struct bucket_cors_conf_data
{
    string_buffer(id, 256);
    string_buffer(max_age_seconds, 100);

    int allowedMethodCount;
    char allowedMethodes[MAX_COUNT_ONE_CORS_RULE][1024];
    int allowedMethodLens[MAX_COUNT_ONE_CORS_RULE];

    int allowedOriginCount;
    char allowedOrigines[MAX_COUNT_ONE_CORS_RULE][1024];
    int allowedOriginLens[MAX_COUNT_ONE_CORS_RULE];

    int allowedHeaderCount;
    char allowedHeaderes[MAX_COUNT_ONE_CORS_RULE][1024];
    int allowedHeaderLens[MAX_COUNT_ONE_CORS_RULE];

    int exposeHeaderCount;
    char exposeHeaderes[MAX_COUNT_ONE_CORS_RULE][1024];
    int exposeHeaderLens[MAX_COUNT_ONE_CORS_RULE];
}bucket_cors_conf_data;

typedef struct get_bucket_cors_data
{
    simple_xml simple_xml_info;

    obs_response_properties_callback    *response_properties_callback;
    obs_response_complete_callback      *response_complete_callback;
    get_cors_configuration_callback     *get_cors_callback;
    void *callback_data;

    bucket_cors_conf_data* bcc_data[MAX_CORS_RULE];
    unsigned int bccd_number;
}get_bucket_cors_data;

typedef struct get_bucket_logging_data
{
    simple_xml simple_xml_info;
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback *response_complete_callback;
    void *callback_data;
    bucket_logging_message *logging_message;
    string_buffer(xml_document, BLS_XML_DOC_MAXSIZE);
    obs_use_api use_api;
}get_bucket_logging_data;

typedef struct convert_bucket_logging_data
{
    char *targetBucketReturn;
    int targetBucketReturnLen;
    int targetBucketReturnSize;
    char *targetPrefixReturn;
    int targetPrefixReturnLen;
    int targetPrefixReturnSize;
    int *acl_grant_count_return;
    char *agencyReturn;
    int agencyReturnLen;
    int agencyReturnSize;
    obs_acl_grant *acl_grants; 
    string_buffer(email_address, OBS_MAX_GRANTEE_EMAIL_ADDRESS_SIZE);
    string_buffer(userId, OBS_MAX_GRANTEE_USER_ID_SIZE);
    string_buffer(userDisplayName, OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE);
    string_buffer(groupUri, 128);
    string_buffer(permission, 32);
    obs_use_api use_api;
} convert_bucket_logging_data;



typedef struct set_notification_data
{
    obs_response_properties_callback *response_properties_callback;
    obs_response_complete_callback  *response_complete_callback;
    void *callback_data;
    char doc[1024*100];
    int  doc_len, doc_bytes_written;
} set_notification_data;


typedef struct get_smn_data
{
    simple_xml                          simple_xml_info;
    obs_response_properties_callback    *response_properties_callback;
    obs_response_complete_callback      *response_complete_callback;
    obs_smn_callback                    *get_smn_callback_func;
    void                                *callback_data;
    obs_smn_notification_configuration  notification_conf;
    obs_use_api use_api;
} get_smn_data;

void obs_options_obj_or_bucket(const obs_options *options, int isBucket, char* key, char* origin,
                char (*requestMethod)[OBS_COMMON_LEN_256], unsigned int rmNumber,
                char (*requestHeader)[OBS_COMMON_LEN_256], unsigned int rhNumber,
                obs_response_handler *handler, void *callback_data);


