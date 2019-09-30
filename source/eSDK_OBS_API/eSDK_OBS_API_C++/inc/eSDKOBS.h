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
#ifndef ESDKOBS_H
#define ESDKOBS_H

#include <stdint.h>
#if defined __GNUC__ || defined LINUX
#include <sys/select.h>
#else 
#include <winsock2.h>
#endif 

#ifdef WIN32
#ifdef OBS_EXPORTS
#define eSDK_OBS_API __declspec(dllexport)
#else
#define eSDK_OBS_API __declspec(dllimport)
#endif
#else
#define eSDK_OBS_API __attribute__((__visibility__("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define OBS_INIT_WINSOCK         1
#define OBS_INIT_ALL                        (OBS_INIT_WINSOCK)
#define OBS_MAX_DELETE_OBJECT_NUMBER  1000
#define OBS_MAX_DELETE_OBJECT_DOC 1024000


typedef enum
{
    OBS_STATUS_OK = 0,
    OBS_STATUS_InitCurlFailed,
    OBS_STATUS_InternalError,
    OBS_STATUS_OutOfMemory,
    OBS_STATUS_Interrupted,
    OBS_STATUS_QueryParamsTooLong,
    OBS_STATUS_FailedToIInitializeRequest,
    OBS_STATUS_MetadataHeadersTooLong,
    OBS_STATUS_BadContentType,
    OBS_STATUS_ContentTypeTooLong,
    OBS_STATUS_BadMd5,   
    OBS_STATUS_Md5TooLong,
    OBS_STATUS_BadCacheControl,
    OBS_STATUS_CacheControlTooLong,
    OBS_STATUS_BadContentDispositionFilename,
    OBS_STATUS_ContentDispositionFilenameTooLong,
    OBS_STATUS_BadContentEncoding,
    OBS_STATUS_ContentEncodingTooLong,
    OBS_STATUS_BadIfMatchEtag,
    OBS_STATUS_IfMatchEtagTooLong,
    OBS_STATUS_BadIfNotMatchEtag,
    OBS_STATUS_IfNotMatchEtagTooLong,
    OBS_STATUS_UriTooLong,
    OBS_STATUS_XmlParseFailure,
    OBS_STATUS_UserIdTooLong,
    OBS_STATUS_UserDisplayNameTooLong,
    OBS_STATUS_EmailAddressTooLong,
    OBS_STATUS_GroupUriTooLong,
    OBS_STATUS_PermissionTooLong,
    OBS_STATUS_TooManyGrants,
    OBS_STATUS_BadGrantee,
    OBS_STATUS_BadPermission,
    OBS_STATUS_XmlDocumentTooLarge,
    OBS_STATUS_NameLookupError,
    OBS_STATUS_FailedToConnect,
    OBS_STATUS_ServerFailedVerification,
    OBS_STATUS_ConnectionFailed,
    OBS_STATUS_AbortedByCallback,
    OBS_STATUS_PartialFile,
    OBS_STATUS_InvalidParameter,
    OBS_STATUS_NoToken,
    OBS_STATUS_OpenFileFailed,
    OBS_STATUS_EmptyFile,

    /**
    * Errors from the obs service
    **/
    OBS_STATUS_AccessDenied,
    OBS_STATUS_AccountProblem,
    OBS_STATUS_AmbiguousGrantByEmailAddress,
    OBS_STATUS_BadDigest,
    OBS_STATUS_BucketAlreadyExists,
    OBS_STATUS_BucketAlreadyOwnedByYou,
    OBS_STATUS_BucketNotEmpty,
    OBS_STATUS_CredentialsNotSupported,
    OBS_STATUS_CrossLocationLoggingProhibited,
    OBS_STATUS_EntityTooSmall,
    OBS_STATUS_EntityTooLarge,
    OBS_STATUS_ExpiredToken,
    OBS_STATUS_IllegalVersioningConfigurationException,
    OBS_STATUS_IncompleteBody,
    OBS_STATUS_IncorrectNumberOfFilesInPostRequest,
    OBS_STATUS_InlineDataTooLarge,
    OBS_STATUS_InvalidAccessKeyId,
    OBS_STATUS_InvalidAddressingHeader,
    OBS_STATUS_InvalidArgument,
    OBS_STATUS_InvalidBucketName,
    OBS_STATUS_InvalidKey,
    OBS_STATUS_InvalidBucketState,
    OBS_STATUS_InvalidDigest,
    OBS_STATUS_InvalidLocationConstraint,
    OBS_STATUS_InvalidObjectState,
    OBS_STATUS_InvalidPart,
    OBS_STATUS_InvalidPartOrder,
    OBS_STATUS_InvalidPayer,
    OBS_STATUS_InvalidPolicyDocument,
    OBS_STATUS_InvalidRange,
    OBS_STATUS_InvalidRedirectLocation,
    OBS_STATUS_InvalidRequest,
    OBS_STATUS_InvalidSecurity,
    OBS_STATUS_InvalidSOAPRequest,
    OBS_STATUS_InvalidStorageClass,
    OBS_STATUS_InvalidTargetBucketForLogging,
    OBS_STATUS_InvalidToken,
    OBS_STATUS_InvalidURI,
    OBS_STATUS_MalformedACLError,
    OBS_STATUS_MalformedPolicy,
    OBS_STATUS_MalformedPOSTRequest,
    OBS_STATUS_MalformedXML,
    OBS_STATUS_MaxMessageLengthExceeded,
    OBS_STATUS_MaxPostPreDataLengthExceededError,
    OBS_STATUS_MetadataTooLarge,
    OBS_STATUS_MethodNotAllowed,
    OBS_STATUS_MissingAttachment,
    OBS_STATUS_MissingContentLength,
    OBS_STATUS_MissingRequestBodyError,
    OBS_STATUS_MissingSecurityElement,
    OBS_STATUS_MissingSecurityHeader,
    OBS_STATUS_NoLoggingStatusForKey,
    OBS_STATUS_NoSuchBucket,
    OBS_STATUS_NoSuchKey,
    OBS_STATUS_NoSuchLifecycleConfiguration,
    OBS_STATUS_NoSuchUpload,
    OBS_STATUS_NoSuchVersion,
    OBS_STATUS_NotImplemented,
    OBS_STATUS_NotSignedUp,
    OBS_STATUS_NotSuchBucketPolicy,
    OBS_STATUS_OperationAborted,
    OBS_STATUS_PermanentRedirect,
    OBS_STATUS_PreconditionFailed,
    OBS_STATUS_Redirect,
    OBS_STATUS_RestoreAlreadyInProgress,
    OBS_STATUS_RequestIsNotMultiPartContent,
    OBS_STATUS_RequestTimeout,
    OBS_STATUS_RequestTimeTooSkewed,
    OBS_STATUS_RequestTorrentOfBucketError,
    OBS_STATUS_SignatureDoesNotMatch,
    OBS_STATUS_ServiceUnavailable,
    OBS_STATUS_SlowDown,
    OBS_STATUS_TemporaryRedirect,
    OBS_STATUS_TokenRefreshRequired,
    OBS_STATUS_TooManyBuckets,
    OBS_STATUS_UnexpectedContent,
    OBS_STATUS_UnresolvableGrantByEmailAddress,
    OBS_STATUS_UserKeyMustBeSpecified,
    OBS_STATUS_InsufficientStorageSpace,
    OBS_STATUS_NoSuchWebsiteConfiguration,
    OBS_STATUS_NoSuchBucketPolicy,
    OBS_STATUS_NoSuchCORSConfiguration,
    OBS_STATUS_InArrearOrInsufficientBalance,
    OBS_STATUS_NoSuchTagSet,
    OBS_STATUS_ErrorUnknown,
    /*
    * The following are HTTP errors returned by obs without enough detail to
    * distinguish any of the above OBS_STATUS_error conditions
    */
    OBS_STATUS_HttpErrorMovedTemporarily,
    OBS_STATUS_HttpErrorBadRequest,
    OBS_STATUS_HttpErrorForbidden,
    OBS_STATUS_HttpErrorNotFound,
    OBS_STATUS_HttpErrorConflict,
    OBS_STATUS_HttpErrorUnknown,

    /*
    * posix new add errors
    */
     OBS_STATUS_QuotaTooSmall,
    OBS_STATUS_BUTT
} obs_status;


typedef enum
{
    OBS_URI_STYLE_VIRTUALHOST               = 0,
    OBS_URI_STYLE_PATH                      = 1
} obs_uri_style;

typedef enum
{
    OBS_PROTOCOL_HTTPS                     = 0,
    OBS_PROTOCOL_HTTP                      = 1
} obs_protocol;

typedef enum
{
    OBS_STORAGE_CLASS_STANDARD              = 0, /* STANDARD */
    OBS_STORAGE_CLASS_STANDARD_IA           = 1, /* STANDARD_IA */
    OBS_STORAGE_CLASS_GLACIER               = 2, /* GLACIER */
    OBS_STORAGE_CLASS_BUTT
} obs_storage_class;

typedef enum image_process_mode
{
    obs_image_process_invalid_mode,
    obs_image_process_cmd,
    obs_image_process_style
}image_process_mode;

typedef enum
{
    OBS_CANNED_ACL_PRIVATE                     = 0,  //used by s3 and obs api
    OBS_CANNED_ACL_PUBLIC_READ                 = 1,  //used by s3 and obs api
    OBS_CANNED_ACL_PUBLIC_READ_WRITE           = 2,  //used by s3 and obs api
    OBS_CANNED_ACL_AUTHENTICATED_READ          = 3,  //only used by s3 api
    OBS_CANNED_ACL_BUCKET_OWNER_READ           = 4,  //only used by s3 api
    OBS_CANNED_ACL_BUCKET_OWNER_FULL_CONTROL   = 5,  //only used by s3 api
    OBS_CANNED_ACL_LOG_DELIVERY_WRITE          = 6,  //only used by s3 api
    OBS_CANNED_ACL_PUBLIC_READ_DELIVERED       = 7,  //only used by obs api
    OBS_CANNED_ACL_PUBLIC_READ_WRITE_DELIVERED = 8,  //only used by obs api
    OBS_CANNED_ACL_BUTT
} obs_canned_acl;

typedef enum
{
    OBS_GRANT_READ                           = 0,
    OBS_GRANT_WRITE                          = 1, 
    OBS_GRANT_READ_ACP                       = 2,
    OBS_GRANT_WRITE_ACP                      = 3,
    OBS_GRANT_FULL_CONTROL                   = 4,
    OBS_GRANT_READ_DELIVERED                 = 5,
    OBS_GRANT_FULL_CONTROL_DELIVERED         = 6,
    OBS_GRANT_BUTT
} obs_grant_domain;


typedef enum
{
    OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL  = 0, // only used by s3 api
    OBS_GRANTEE_TYPE_CANONICAL_USER           = 1, // used by both of s3 and obs api
    OBS_GRANTEE_TYPE_ALL_OBS_USERS            = 2, // only used by s3 api
    OBS_GRANTEE_TYPE_ALL_USERS                = 3, // used by both of s3 and obs api
    OBS_GRANTEE_TYPE_LOG_DELIVERY             = 4, // only used by s3 api
    OBS_GRANTEE_TYPE_BUTT
} obs_grantee_type;

typedef enum
{
    OBS_PERMISSION_READ                     = 0,
    OBS_PERMISSION_WRITE                    = 1,
    OBS_PERMISSION_READ_ACP                 = 2,
    OBS_PERMISSION_WRITE_ACP                = 3,
    OBS_PERMISSION_FULL_CONTROL             = 4,
    OBS_PERMISSION_BUTT
} obs_permission;

typedef enum
{
    OBS_TIER_NULL = 0,
    OBS_TIER_STANDARD, 
    OBS_TIER_EXPEDITED,
    OBS_TIER_BULK, 
} obs_tier;

typedef enum
{
    UPLOAD_NOTSTART,
    UPLOADING,
    UPLOAD_FAILED,
    UPLOAD_SUCCESS,
    STATUS_BUTT
}part_upload_status;

typedef enum
{
    OBS_SMN_FILTER_NULL = 0,
    OBS_SMN_FILTER_PREFIX,
    OBS_SMN_FILTER_SUFFIX
}obs_smn_filter_rule_enum;

typedef enum
{
    SMN_EVENT_NULL = 0,
    SMN_EVENT_OBJECT_CREATED_ALL,
    SMN_EVENT_OBJECT_CREATED_PUT,
    SMN_EVENT_OBJECT_CREATED_POST,
    SMN_EVENT_OBJECT_CREATED_COPY,
    SMN_EVENT_OBJECT_CREATED_COMPLETE_MULTIPART_UPLOAD,
    SMN_EVENT_OBJECT_REMOVED_ALL,
    SMN_EVENT_OBJECT_REMOVED_DELETE,
    SMN_EVENT_OBJECT_REMOVED_DELETE_MARKER_CREATED,
    SMN_EVENT_REDUCED_REDUNDANCY_LOST_OBJECT
}obs_smn_event_enum;

typedef enum
{
    DOWNLOAD_NOTSTART,
    DOWNLOADING,
    DOWNLOAD_FAILED,
    DOWNLOAD_SUCCESS,
    COMBINE_SUCCESS,
    DOWN_STATUS_BUTT
}download_status;

typedef enum
{
    OBS_USE_API_S3              = 0,
    OBS_USE_API_OBS             = 1
}obs_use_api;

typedef enum
{
    OBS_NO_CERTIFICATE,
    OBS_DEFAULT_CERTIFICATE,
    OBS_DEFINED_CERTIFICATE
}obs_certificate_conf;

typedef enum
{
    OBS_ENCRYPTION_KMS,
    OBS_ENCRYPTION_SSEC   
}obs_encryption_type;

typedef enum
{
    OBJECT_DELIVERED_TRUE              = 0,				// Default value is true.
    OBJECT_DELIVERED_FALSE             = 1
}obs_object_delivered;

typedef enum
{
    BUCKET_DELIVERED_FALSE             = 0,				// Default value is false.
    BUCKET_DELIVERED_TRUE              = 1
}obs_bucket_delivered;


typedef enum
{
    OBS_BUCKET_OBJECT          = 0,   //object bucket
    OBS_BUCKET_POSIX           = 1    //posix bucket
}obs_bucket_type;

typedef enum
{
    OBS_BUCKET_LIST_ALL             = 0,   //list all type bucket
    OBS_BUCKET_LIST_OBJECT          = 1,   //list object bucket
    OBS_BUCKET_LIST_POSIX           = 2    //list posix bucket
}obs_bucket_list_type;

#define OBS_COMMON_LEN_256 256

#define OBS_MAX_ACL_GRANT_COUNT             100

#define OBS_MAX_GRANTEE_EMAIL_ADDRESS_SIZE  128

#define OBS_MAX_GRANTEE_USER_ID_SIZE        128

#define OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE   128
    
#define OBS_MAX_HOSTNAME_SIZE               255

#define OBS_MAX_KEY_SIZE                    1024

#define OBS_MAX_METADATA_SIZE               4096

#define OBS_METADATA_HEADER_NAME_PREFIX     "x-amz-meta-"

#define OBS_VERSION_STATUS_ENABLED           "Enabled"

#define OBS_VERSION_STATUS_SUSPENDED        "Suspended"

#define OBS_MAX_METADATA_COUNT \
    (OBS_MAX_METADATA_SIZE / (sizeof(OBS_METADATA_HEADER_NAME_PREFIX "nv") - 1))


typedef struct obs_request_context obs_request_context;

typedef struct obs_acl_grant
{
    obs_grantee_type grantee_type;
    union
    {
        struct
        {
            char email_address[OBS_MAX_GRANTEE_EMAIL_ADDRESS_SIZE];
        } huawei_customer_by_email; // only used by s3 api
        struct
        {
            char id[OBS_MAX_GRANTEE_USER_ID_SIZE];
            char display_name[OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE]; // only used by s3 api
        } canonical_user;
    } grantee;
    obs_permission permission;
    obs_bucket_delivered bucket_delivered; // only used by obs api
}obs_acl_grant;

typedef struct obs_acl_group
{
  int acl_grant_count;
  obs_acl_grant *acl_grants; 
}obs_acl_group;

typedef struct obs_object_info
{
    char* key;
    char* version_id;
}obs_object_info;

typedef struct obs_delete_object_info
{
    unsigned int keys_number;
    int quiet;
}obs_delete_object_info;

typedef struct manager_acl_info
{
    obs_object_info object_info;
    char *owner_id;
    char *owner_display_name;
    int *acl_grant_count_return; 
    obs_object_delivered object_delivered; // only used by obs api
    obs_acl_grant *acl_grants;
}manager_acl_info;


typedef struct obs_upload_part_info
{
    unsigned int part_number; 
    char *upload_id;
}obs_upload_part_info;

typedef struct obs_complete_upload_Info
{
    unsigned int part_number;
    char *etag;
}obs_complete_upload_Info;

typedef struct list_part_info
{
    char *upload_id;
    unsigned int max_parts;
    unsigned int part_number_marker;
}list_part_info;


typedef struct obs_name_value
{
    char *name;
    char *value;
} obs_name_value;

typedef struct obs_error_details
{
    const char *message;

    const char *resource;

    const char *further_details;

    int extra_details_count;

    obs_name_value *extra_details;
} obs_error_details;

typedef struct obs_response_properties
{
    const char *request_id;

    const char *request_id2;

    const char *content_type;

    uint64_t content_length;

    const char *server;

    const char *etag;

    const char *expiration;

    const char *website_redirect_location;

    const char *version_id;

    int64_t last_modified;

    int meta_data_count;

    const obs_name_value *meta_data;

    char use_server_side_encryption;

    const char *allow_origin;

    const char *allow_headers;

    const char *max_age;

    const char *allow_methods;

    const char *expose_headers;

    const char *storage_class;

    const char *server_side_encryption;

    const char *kms_key_id;

    const char *customer_algorithm;

    const char *customer_key_md5;

    const char *bucket_location;

    const char *obs_version;

    const char *restore;

    const char *obs_object_type;

    const char *obs_next_append_position;
    
    const char *obs_head_epid;

	const char *reserved_indicator;
} obs_response_properties;

typedef struct obs_list_objects_content
{
    const char *key;
    int64_t last_modified;
    const char *etag;
    uint64_t size;
    const char *owner_id;
    const char *owner_display_name;
    const char *storage_class;
    const char *type;
} obs_list_objects_content;

typedef struct obs_version
{

    const char *key;
    const char *version_id;
    const char *is_latest;
    int64_t last_modified;
    const char *etag;
    uint64_t size;
    const char *owner_id;
    const char *owner_display_name;
    const char *storage_class;
    const char *is_delete;
} obs_version;

typedef struct obs_list_versions
{
    const char* bucket_name;
    const char* prefix;
    const char* key_marker;
    const char* delimiter;
    const char* max_keys;
    obs_version* versions;
    int versions_count;
    const char** common_prefixes;
    int common_prefixes_count;
} obs_list_versions;


typedef struct obs_list_parts
{
    unsigned int part_number;
    int64_t last_modified;
    const char *etag;
    uint64_t size;
    const char *storage_class;
}obs_list_parts;

typedef struct obs_list_multipart_upload
{
    const char *key;
    const char *upload_id;
    const char *initiator_id;
    const char *initiator_display_name;
    const char *owner_id;
    const char *owner_display_name;
    const char *storage_class;
    int64_t    initiated;
} obs_list_multipart_upload;

typedef struct obs_lifecycle_transtion
{
    const char *date;
    const char *days;
    obs_storage_class storage_class;
}obs_lifecycle_transtion;

typedef struct obs_lifecycle_noncurrent_transtion
{
    const char *noncurrent_version_days;
    obs_storage_class storage_class;
}obs_lifecycle_noncurrent_transtion;

typedef struct obs_lifecycle_conf
{
    const char *date;
    const char *days;
    const char *id;
    const char *prefix;
    const char *status;
    const char *noncurrent_version_days;
    obs_lifecycle_transtion * transition;
    unsigned int transition_num;
    obs_lifecycle_noncurrent_transtion * noncurrent_version_transition;
    unsigned int noncurrent_version_transition_num;

}obs_lifecycle_conf;

typedef struct obs_bucket_cors_conf
{
    const char *id;
    const char **allowed_method;
    unsigned int allowed_method_number;
    const char **allowed_origin;
    unsigned int allowed_origin_number;
    const char **allowed_header;
    unsigned int allowed_header_number;
    const char *max_age_seconds;
    const char **expose_header;
    unsigned int expose_header_number;
}obs_bucket_cors_conf;

typedef struct obs_uploaded_parts_total_info
{
    int  is_truncated;
    unsigned int nextpart_number_marker;
    char *initiator_id;
    char *initiator_display_name;
    char *owner_id;
    char *owner_display_name;
    char *sorage_class;
    int  parts_count; 
}obs_uploaded_parts_total_info;

typedef struct obs_copy_destination_object_info
{
    char *destination_bucket;
    char *destination_key;
    char *version_id;
    int64_t *last_modified_return;
    int etag_return_size;
    char *etag_return;
}obs_copy_destination_object_info;

typedef struct _obs_upload_file_configuration
{
    char *upload_file;
    uint64_t part_size;
    char * check_point_file;
    int enable_check_point;
    int task_num;
}obs_upload_file_configuration;

typedef struct _obs_download_file_configuration
{
    char * downLoad_file;
    uint64_t part_size;
    char * check_point_file;
    int enable_check_point;
    int task_num;
}obs_download_file_configuration;

typedef struct _obs_upload_file_part_info
{
    int part_num;
    uint64_t start_byte;
    uint64_t part_size;
    part_upload_status status_return;
}obs_upload_file_part_info;

typedef struct _obs_download_file_part_info
{
    int part_num;
    uint64_t start_byte;
    uint64_t part_size;
    download_status status_return;
}obs_download_file_part_info;
typedef struct obs_set_bucket_redirect_all_conf
{
    const char *host_name;
    const char *protocol;
}obs_set_bucket_redirect_all_conf;

typedef struct obs_delete_objects
{
    const char *key;
    const char *code;
    const char *message;
    const char *delete_marker;
    const char *delete_marker_version_id;
} obs_delete_objects;

typedef struct bucket_website_routingrule
{
    const char *key_prefix_equals;
    const char *http_errorcode_returned_equals;
    const char *protocol;
    const char *host_name;
    const char *replace_key_prefix_with;
    const char *replace_key_with;
    const char *http_redirect_code;
}bucket_website_routingrule;

typedef struct obs_set_bucket_website_conf
{
    const char *suffix;
    const char *key;
    bucket_website_routingrule *routingrule_info;
    int routingrule_count;
}obs_set_bucket_website_conf;

typedef struct obs_smn_filter_rule
{
    obs_smn_filter_rule_enum name;
    char* value;
}obs_smn_filter_rule;


typedef struct obs_smn_topic_configuration
{
    char* topic;
    char* id;
    obs_smn_filter_rule* filter_rule;
    unsigned int filter_rule_num;
    obs_smn_event_enum* event;
    unsigned int event_num;
}obs_smn_topic_configuration;


typedef struct obs_smn_notification_configuration
{
    obs_smn_topic_configuration* topic_conf;
    unsigned int topic_conf_num;
}obs_smn_notification_configuration;


/***************************response handle function*******************************************/
typedef obs_status (obs_response_properties_callback)(const obs_response_properties *properties, 
                            void *callback_data);

typedef void (obs_response_complete_callback)(obs_status status, 
                            const obs_error_details *error_details, void *callback_data);

typedef int (obs_put_object_data_callback)(int buffer_size, char *buffer,
                                           void *callback_data);
    
typedef int (obs_append_object_data_callback)(int buffer_size, char *buffer,
                                              void *callback_data);
typedef int (obs_modify_object_data_callback)(int buffer_size, char *buffer,
                                              void *callback_data);

typedef obs_status (obs_get_object_data_callback)(int buffer_size, const char *buffer,
                                           void *callback_data);

typedef obs_status (obs_list_service_callback)(const char *owner_id,
                                               const char *owner_display_name,
                                               const char *bucket_name,
                                               int64_t creation_date_seconds,
                                               void *callback_data);

typedef obs_status (obs_list_service_obs_callback)(const char *owner_id,
                                                   const char *bucket_name,
                                                   int64_t creation_date_seconds,
                                                   const char *location,
                                                   void *callback_data);

typedef obs_status (obs_get_bucket_storage_policy)(const char * storage_class_policy,
                                               void *callback_data);

typedef obs_status (obs_get_bucket_websiteconf_callback) (const char *hostname, const char *protocol,
                    const char *suffix,const char *key, const bucket_website_routingrule *routingrule,
                    int webdatacount, void *callback_data);


typedef int (obs_upload_data_callback)(int buffer_size, char *buffer, void *callback_data);

typedef obs_status (obs_complete_multi_part_upload_callback)(const char *location, 
                                                             const char *bucket,
                                                             const char *key,
                                                             const char* etag,
                                                             void *callback_data);

typedef obs_status (obs_list_parts_callback_ex)(obs_uploaded_parts_total_info* uploaded_parts,
            obs_list_parts *parts, void *callback_data);
typedef void (obs_upload_file_callback)(obs_status status, char *result_message, int part_count_return,
            obs_upload_file_part_info * upload_info_list, void *callback_data);

typedef obs_status (obs_list_objects_callback)(int is_truncated, const char *next_marker,
            int contents_count,  const obs_list_objects_content *contents,
            int common_prefixes_count, const char **common_prefixes,
            void *callback_data);


typedef obs_status (obs_list_multipart_uploads_callback)(int is_truncated, const char *next_marker,
            const char *next_uploadId_marker, int uploads_count, 
            const obs_list_multipart_upload *uploads, int common_prefixes_count, 
            const char **common_prefixes, void *callback_data);

typedef obs_status (obs_list_versions_callback)(int is_truncated, const char *next_key_marker,
            const char *next_versionid_marker, const obs_list_versions *versions,
            void *callback_data);

typedef obs_status (get_lifecycle_configuration_callback) (obs_lifecycle_conf* bucket_lifecycle_conf,
            unsigned int blcc_number, void *callback_data);

typedef void (obs_download_file_callback)(obs_status status, char *result_message, 
            int part_count_return, obs_download_file_part_info * download_info_list,
            void *callback_data);

typedef obs_status (get_cors_configuration_callback)(obs_bucket_cors_conf* bucket_cors_conf,
            unsigned int bcc_number, void *callback_data);

typedef obs_status (obs_delete_object_data_callback)(int contents_count, 
            obs_delete_objects *contents, void *callback_data);

typedef obs_status (obs_smn_callback)(obs_smn_notification_configuration* notification_conf,
            void *callback_data);


/**************************response handler struct**********************************************/

typedef struct obs_response_handler
{
    obs_response_properties_callback *properties_callback;
    obs_response_complete_callback   *complete_callback;
} obs_response_handler;

typedef struct obs_list_objects_handler
{
    obs_response_handler response_handler;
    obs_list_objects_callback *list_Objects_callback;
} obs_list_objects_handler;

typedef struct obs_list_versions_handler
{
    obs_response_handler response_handler;
    obs_list_versions_callback *list_versions_callback;
} obs_list_versions_handler;

typedef struct obs_list_multipart_uploads_handler
{
    obs_response_handler response_handler;
    obs_list_multipart_uploads_callback *list_mulpu_callback;
} obs_list_multipart_uploads_handler;
 
typedef struct obs_put_object_handler
{
    obs_response_handler response_handler;
    obs_put_object_data_callback *put_object_data_callback;
} obs_put_object_handler;
typedef struct obs_append_object_handler
{
    obs_response_handler response_handler;
    obs_append_object_data_callback *append_object_data_callback;
} obs_append_object_handler;

typedef struct obs_modify_object_handler
{
    obs_response_handler response_handler;
    obs_modify_object_data_callback *modify_object_data_callback;
} obs_modify_object_handler;


typedef struct obs_get_object_handler
{
    obs_response_handler response_handler;
    obs_get_object_data_callback *get_object_data_callback;
} obs_get_object_handler;

typedef struct obs_lifecycle_handler
{
    obs_response_handler response_handler;
    get_lifecycle_configuration_callback *get_lifecycle_callback;
} obs_lifecycle_handler;

typedef struct obs_cors_handler
{
    obs_response_handler response_handler;
    get_cors_configuration_callback *get_cors_callback;
} obs_cors_handler;


typedef struct obs_upload_handler
{
    obs_response_handler response_handler;
    obs_upload_data_callback *upload_data_callback;
} obs_upload_handler;

typedef struct obs_complete_multi_part_upload_handler
{
    obs_response_handler response_handler;
    obs_complete_multi_part_upload_callback *complete_multipart_upload_callback;
} obs_complete_multi_part_upload_handler;

typedef struct obs_list_parts_handler
{
    obs_response_handler response_handler;
    obs_list_parts_callback_ex *list_parts_callback_ex;
} obs_list_parts_handler;

typedef struct obs_upload_file_response_handler
{
    obs_response_handler response_handler;
    obs_upload_file_callback *upload_file_callback;
} obs_upload_file_response_handler;
typedef struct __obs_download_file_response_handler
{
    obs_response_handler response_handler;
    obs_download_file_callback *download_file_callback;
}obs_download_file_response_handler;

typedef struct obs_delete_object_handler
{
    obs_response_handler response_handler;
    obs_delete_object_data_callback *delete_object_data_callback;
} obs_delete_object_handler;

typedef struct obs_get_bucket_websiteconf_handler
{
    obs_response_handler response_handler;
    obs_get_bucket_websiteconf_callback *get_bucket_website_conf_callback;
} obs_get_bucket_websiteconf_handler;

typedef struct obs_smn_handler
{
    obs_response_handler response_handler;
    obs_smn_callback *get_smn_callback_func;
}obs_smn_handler;


/**************************return struct*******************************************/
typedef struct obs_bucket_context
{
    char *host_name;
    char *bucket_name;
    obs_protocol protocol;
    obs_uri_style uri_style;
    char *access_key;
    char *secret_access_key;
    char *certificate_info;
    obs_storage_class storage_class;
    char * token; 
    char * epid;
    obs_bucket_type bucket_type;
    obs_bucket_list_type bucket_list_type;
} obs_bucket_context;

typedef enum
{
    OBS_HTTP2_OPEN                      = 0,
    OBS_HTTP2_CLOSE                      = 1
} obs_http2_switch;

typedef enum
{
    OBS_BBR_OPEN                      = 0,
    OBS_BBR_CLOSE                      = 1
} obs_bbr_switch;

typedef enum
{
    OBS_OPENSSL_CLOSE                    =0,
    OBS_OPENSSL_OPEN                     = 1
} obs_openssl_switch;

typedef enum
{
	OBS_NEGOTIATION_TYPE	                   =0,
	OBS_OBS_TYPE                               =1,
	OBS_S3_TYPE                                =2
}obs_auth_switch;

typedef enum
{
	OBS_NO_METADATA_ACTION	                   =0,
	OBS_REPLACE                                =1,
	OBS_REPLACE_NEW                            =2
}metadata_action_indicator;

typedef struct obs_http_request_option
{
    int speed_limit;
    int speed_time;
    int connect_time;
    int max_connected_time;
    char *proxy_host;
    char *proxy_auth;
    char *ssl_cipher_list;
    obs_http2_switch http2_switch;
    obs_bbr_switch   bbr_switch;
	obs_auth_switch  auth_switch;
} obs_http_request_option;

typedef struct temp_auth_configure
{
    long long int expires;
    void (* temp_auth_callback)(char * temp_auth_url,char * temp_auth_headers,void *callback_data);
    void * callback_data;
}temp_auth_configure;

typedef struct obs_options
{
    obs_bucket_context bucket_options;
    obs_http_request_option request_options;
    temp_auth_configure *temp_auth; 
} obs_options;

typedef struct image_process_configure
{
    image_process_mode image_process_mode;
    char * cmds_stylename;
}image_process_configure;

typedef struct obs_get_conditions
{
    uint64_t start_byte;
    uint64_t byte_count;
    int64_t if_modified_since;
    int64_t if_not_modified_since;
    char *if_match_etag;
    char *if_not_match_etag;
    image_process_configure * image_process_config;    
} obs_get_conditions;

typedef struct file_object_config
{
    int auto_split;
    char * file_name;
    void (*print_process_callback)(uint64_t remain_bytes, int progress_rate);
}file_object_config;

typedef struct grant_domain_config
{
    char *domain;
    obs_grant_domain grant_domain;
}grant_domain_config;



typedef struct obs_put_properties
{
    char *content_type;
    char *md5;
    char *cache_control;
    char *content_disposition_filename;
    char *content_encoding;
    char *website_redirect_location;
    obs_get_conditions *get_conditions;
    uint64_t start_byte;
    uint64_t byte_count;
    int64_t expires;
    obs_canned_acl canned_acl;
    grant_domain_config *domain_config;
    int meta_data_count;
    obs_name_value *meta_data;
    file_object_config * file_object_config;
	metadata_action_indicator metadata_action;
} obs_put_properties;

typedef struct server_side_encryption_params
{
    obs_encryption_type encryption_type;
    char *kms_server_side_encryption;
    char *kms_key_id;
    char *ssec_customer_algorithm;
    char *ssec_customer_key;
    char *des_ssec_customer_algorithm;
    char *des_ssec_customer_key;
}server_side_encryption_params;

typedef obs_status (obs_get_bucket_storage_policy_callback)(const char * storage_class_policy,
                            void *callback_data);
 
typedef struct obs_get_bucket_storage_class_handler
{
    obs_response_handler response_handler;
    obs_get_bucket_storage_policy_callback *get_bucket_sorage_class_callback;
}obs_get_bucket_storage_class_handler;
 
typedef obs_status (obs_get_bucket_tagging_callback)(int tagging_count, 
            obs_name_value *tagging_list, void *callback_data);
 
typedef struct obs_get_bucket_tagging_handler
{
    obs_response_handler response_handler;
    obs_get_bucket_tagging_callback *get_bucket_tagging_callback;
}obs_get_bucket_tagging_handler;

typedef struct obs_list_service_handler
{
    obs_response_handler response_handler;
    obs_list_service_callback *listServiceCallback;
} obs_list_service_handler;

typedef struct obs_list_service_obs_handler
{
    obs_response_handler response_handler;
    obs_list_service_obs_callback *listServiceCallback;
} obs_list_service_obs_handler;

typedef struct bucket_logging_message
{
     char *target_bucket;
     int  target_bucket_size;
     char *target_prefix;
     int  target_prefix_size;
     obs_acl_grant *acl_grants;
     int *acl_grant_count;
     char *agency;
     int  agency_size;
}bucket_logging_message;

/****************************init handle *****************************************************/
eSDK_OBS_API obs_status obs_initialize(int win32_flags);

eSDK_OBS_API void obs_deinitialize();

eSDK_OBS_API void init_obs_options(obs_options * options);

eSDK_OBS_API int obs_status_is_retryable(obs_status status);

eSDK_OBS_API obs_status set_online_request_max_count(uint32_t online_request_max);

eSDK_OBS_API obs_status init_certificate_by_path(obs_protocol protocol, 
                            obs_certificate_conf ca_conf, const char *path, int path_length);

eSDK_OBS_API obs_status init_certificate_by_buffer(const char *buffer, int buffer_length);

/*************************************bucket handle**************************************/

eSDK_OBS_API void create_bucket(obs_options *options, obs_canned_acl canned_acl,
            const char *location_constraint, obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void create_posix_bucket(obs_options *options, obs_canned_acl canned_acl,
                const char *location_constraint, uint64_t storage_quota, obs_response_handler *handler,
                void *callback_data);

eSDK_OBS_API void list_bucket(obs_options *options, obs_list_service_handler *handler, 
                   void *callback_data);

eSDK_OBS_API void list_bucket_obs(obs_options *options, obs_list_service_obs_handler *handler, 
                   void *callback_data);

eSDK_OBS_API void delete_bucket(obs_options *options, obs_response_handler *handler, void *callback_data);


eSDK_OBS_API void list_bucket_objects(obs_options *options, const char *prefix, const char *marker, const char *delimiter, 
            int maxkeys, obs_list_objects_handler *handler, void *callback_data);

eSDK_OBS_API void list_versions(obs_options *options, const char *prefix, const char *key_marker, const char *delimiter, 
           int maxkeys, const char *version_id_marker, obs_list_versions_handler *handler, void *callback_data);

eSDK_OBS_API void set_bucket_quota(obs_options *options, uint64_t storage_quota, 
                               obs_response_handler *handler, void *callback_data);
 
eSDK_OBS_API void get_bucket_quota(obs_options *options, uint64_t *storagequota_return,
            obs_response_handler *handler,  void *callback_data);
 
eSDK_OBS_API void set_bucket_policy(obs_options *options, const char *policy, 
            obs_response_handler *handler,  void *callback_data);
 
eSDK_OBS_API void get_bucket_policy(obs_options *options, int policy_return_size, 
                      char *policy_return, obs_response_handler *handler, 
                      void *callback_data);
 
eSDK_OBS_API void delete_bucket_policy(obs_options *options, obs_response_handler *handler,
                      void *callback_data);
 
eSDK_OBS_API void set_bucket_version_configuration(obs_options *options, const char *version_status, 
           obs_response_handler *handler, void *callback_data);
 
eSDK_OBS_API void get_bucket_version_configuration(obs_options *options, int status_return_size, 
                      char *status_return, obs_response_handler *handler, void *callback_data);
 
eSDK_OBS_API void set_bucket_storage_class_policy(obs_options *options, 
          obs_storage_class storage_class_policy, obs_response_handler *handler, void *callback_data);
 
eSDK_OBS_API void get_bucket_storage_class_policy(obs_options *options, 
                        obs_get_bucket_storage_class_handler *handler, void *callback_data);
 
eSDK_OBS_API void set_bucket_tagging(obs_options *options,obs_name_value * tagging_list, 
        unsigned int number, obs_response_handler *handler, void *callback_data);
 
eSDK_OBS_API void get_bucket_tagging(obs_options *options, obs_get_bucket_tagging_handler *handler, 
                    void *callback_data);
 
eSDK_OBS_API void delete_bucket_tagging(obs_options *options, obs_response_handler *handler,
        void *callback_data);


eSDK_OBS_API void set_bucket_logging_configuration(obs_options *options, char *target_bucket, char *target_prefix, 
                obs_acl_group *acl_group, obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void set_bucket_logging_configuration_obs(obs_options *options, char *target_bucket, char *target_prefix, char *agency,
                obs_acl_group *acl_group, obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void get_bucket_logging_configuration(obs_options *options, obs_response_handler *handler, 
                    bucket_logging_message *logging_message_data, void *callback_data);


eSDK_OBS_API void set_bucket_website_configuration(obs_options *options, 
                    obs_set_bucket_redirect_all_conf *set_bucket_redirect_all,
                    obs_set_bucket_website_conf *set_bucket_website_conf,
                    obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void get_bucket_website_configuration(obs_options *options, 
                    obs_get_bucket_websiteconf_handler *handler, 
                    void *callback_data);

eSDK_OBS_API void delete_bucket_website_configuration(obs_options *options, 
                    obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void get_bucket_storage_info(obs_options *options, int capacity_length, char *capacity,
                    int object_number_length, char *object_number,
                    obs_response_handler *handler, void *callback_data);
 
 eSDK_OBS_API void list_multipart_uploads(obs_options *options, const char *prefix, const char *marker, const char *delimiter,
                    const char* uploadid_marke, int max_uploads, obs_list_multipart_uploads_handler *handler, 
                    void *callback_data);

eSDK_OBS_API void set_bucket_lifecycle_configuration(obs_options *options, 
           obs_lifecycle_conf* bucket_lifecycle_conf, unsigned int blcc_number, 
           obs_response_handler *handler, void *callback_data);
 
eSDK_OBS_API void get_bucket_lifecycle_configuration(obs_options *options,
                obs_lifecycle_handler *handler, void *callback_data);

eSDK_OBS_API void delete_bucket_lifecycle_configuration(obs_options *options, 
            obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void set_bucket_cors_configuration(obs_options *options, obs_bucket_cors_conf *obs_cors_conf_info, 
            unsigned int conf_num, obs_response_handler *handler, void *callback_data);


eSDK_OBS_API void get_bucket_cors_configuration(obs_options *options, obs_cors_handler *handler,
            void *callback_data);

eSDK_OBS_API void delete_bucket_cors_configuration(obs_options *options, 
            obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void set_notification_configuration(obs_options *options, 
    obs_smn_notification_configuration* notification_conf, obs_response_handler *handler,
    void *callback_data);


eSDK_OBS_API void get_notification_configuration(obs_options *options,
                            obs_smn_handler *handler,  void *callback_data);

eSDK_OBS_API void set_bucket_acl(obs_options * options, manager_acl_info * aclinfo, 
            obs_response_handler * handler, void *callback_data);


eSDK_OBS_API void set_bucket_acl_by_head(obs_options * options, obs_canned_acl canned_acl, 
            obs_response_handler * handler, void *callback_data);

eSDK_OBS_API void get_bucket_acl(obs_options * options, manager_acl_info * aclinfo, 
           obs_response_handler * handler, void *callback_data);

eSDK_OBS_API void obs_options_bucket(obs_options *options, char* origin,
                    char (*request_method)[OBS_COMMON_LEN_256], unsigned int method_number,
                    char (*request_header)[OBS_COMMON_LEN_256], unsigned int header_number,
                    obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void get_bucket_metadata_with_corsconf(obs_options *options, char *origin,
                    char (*requestHeader)[OBS_COMMON_LEN_256], unsigned int number, 
                    obs_response_handler *handler);

eSDK_OBS_API void obs_head_bucket(obs_options *options, obs_response_handler *handler, 
                    void *callback_data);


/*************************************object handle*************************************/

eSDK_OBS_API void get_object_metadata(obs_options *options, obs_object_info *object_info, 
                                  server_side_encryption_params *encryption_params,
                                  obs_response_handler *handler, void *callback_data);
								  
eSDK_OBS_API void set_object_metadata(obs_options *options, obs_object_info *object_info, 
								  obs_put_properties *put_properties,
								  server_side_encryption_params *encryption_params,
								  obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void put_object(obs_options *options, char *key, uint64_t content_length,
                         obs_put_properties *put_properties,
                         server_side_encryption_params *encryption_params,
                         obs_put_object_handler *handler, void *callback_data);

eSDK_OBS_API void init_get_properties(obs_get_conditions *get_conditions);

eSDK_OBS_API void get_object(obs_options *options, obs_object_info *object_info,
                         obs_get_conditions *get_conditions, 
                         server_side_encryption_params *encryption_params,
                         obs_get_object_handler *handler, void *callback_data);

eSDK_OBS_API void delete_object(obs_options *options, obs_object_info *object_info,
                             obs_response_handler *handler, void *callback_data);

eSDK_OBS_API const char *obs_get_status_name(obs_status status);

eSDK_OBS_API obs_status obs_create_request_context(obs_request_context **request_context_return);

eSDK_OBS_API void obs_destroy_request_context(obs_request_context *request_context);

eSDK_OBS_API obs_status obs_runall_request_context(obs_request_context *request_context);

eSDK_OBS_API void obs_head_object(obs_options *options, char *key,
                                  obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void init_put_properties(obs_put_properties *put_properties);

eSDK_OBS_API void upload_part(obs_options *options, char *key, obs_upload_part_info *upload_part_info, 
                              uint64_t content_length, obs_put_properties *put_properties,
                              server_side_encryption_params *encryption_params,
                              obs_upload_handler *handler, void *callback_data);

eSDK_OBS_API void initiate_multi_part_upload(obs_options *options, char *key,int upload_id_return_size,
                                         char *upload_id_return, obs_put_properties *put_properties,
                                         server_side_encryption_params *encryption_params,
                                         obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void complete_multi_part_upload(obs_options *options, char *key, const char *upload_id, unsigned int part_number, 
                                         obs_complete_upload_Info *complete_upload_Info,obs_put_properties *put_properties,
                                         obs_complete_multi_part_upload_handler *handler, void *callback_data);

eSDK_OBS_API void list_parts (obs_options *options, char *key, list_part_info *listpart,
                          obs_list_parts_handler *handler, void *callback_data);

eSDK_OBS_API void abort_multi_part_upload(obs_options *options, char *key, const char *upload_id,
                                      obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void copy_object(obs_options *options, char *key, const char *version_id, obs_copy_destination_object_info *object_info,
                          unsigned int is_copy, obs_put_properties *put_properties, server_side_encryption_params *encryption_params,
                          obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void copy_part(obs_options *options, char *key, obs_copy_destination_object_info *object_info,
                        obs_upload_part_info *copypart, obs_put_properties *put_properties, 
                        server_side_encryption_params *encryption_params,obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void restore_object(obs_options *options, obs_object_info *object_info, const char *days, 
                             obs_tier tier,const obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void obs_options_object(obs_options *options, char* key, char* origin,
                    char (*request_method)[OBS_COMMON_LEN_256], unsigned int method_number,
                    char (*request_header)[OBS_COMMON_LEN_256], unsigned int header_number,
                    obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void initialize_break_point_lock();

eSDK_OBS_API void deinitialize_break_point_lock();

eSDK_OBS_API void upload_file(obs_options *options, char *key, server_side_encryption_params *encryption_params, 
                          obs_upload_file_configuration *upload_file_config, obs_upload_file_response_handler *handler,
                          void *callback_data);

eSDK_OBS_API void download_file(obs_options *options, char *key, char* version_id, obs_get_conditions *get_conditions,
                        server_side_encryption_params *encryption_params,
                        obs_download_file_configuration * download_file_config,
                        obs_download_file_response_handler *handler, void *callback_data);

eSDK_OBS_API void batch_delete_objects(obs_options *options, obs_object_info *object_info,obs_delete_object_info *delobj,     
                                  obs_put_properties *put_properties, obs_delete_object_handler *handler, void *callback_data);

eSDK_OBS_API void get_object_acl(obs_options *options, manager_acl_info *aclinfo, 
                                 obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void set_object_acl(obs_options *options, manager_acl_info *aclinfo, 
                                 obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void set_object_acl_by_head(obs_options *options, obs_object_info *object_info, obs_canned_acl canned_acl, 
                                         obs_response_handler *handler, void *callback_data);
// only object bucket can use Modify_object
eSDK_OBS_API void append_object(obs_options *options, char *key, uint64_t content_length, const char * position,
                   obs_put_properties *put_properties,server_side_encryption_params *encryption_params,
                   obs_append_object_handler *handler, void *callback_data);
// only posix bucket can use Modify_object
eSDK_OBS_API void modify_object(obs_options *options, char *key, uint64_t content_length, uint64_t position,
                   obs_put_properties *put_properties,server_side_encryption_params *encryption_params,
                   obs_modify_object_handler *handler, void *callback_data);
// only posix bucket can use truncate_object
eSDK_OBS_API void truncate_object(obs_options *options, char *key, uint64_t object_length,
                   obs_response_handler *handler, void *callback_data);
// only posix bucket can use rename_object
eSDK_OBS_API void rename_object(obs_options *options, char *key, char *new_object_name,
                   obs_response_handler *handler, void *callback_data);

eSDK_OBS_API void compute_md5(const char *buffer, int64_t buffer_size, char *outbuffer);

eSDK_OBS_API int set_obs_log_path(const char *log_path);

eSDK_OBS_API void set_openssl_callback(obs_openssl_switch switch_flag);


#ifdef __cplusplus
}
#endif

#endif /* LIBOBS_H */



