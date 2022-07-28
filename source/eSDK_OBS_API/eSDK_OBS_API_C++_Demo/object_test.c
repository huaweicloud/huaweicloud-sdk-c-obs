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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#if defined __GNUC__ || defined LINUX
#include <string.h>
#include <getopt.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#else
#include "getopt.h"
#endif

#include "eSDKOBS.h"
#include "demo_common.h"
#include "securec.h"
#include "cJSON.h"

extern int64_t parseIso8601Time(const char *str);

FILE **uploadFilePool = NULL;

#ifndef FOPEN_EXTRA_FLAGS
#define FOPEN_EXTRA_FLAGS ""
#endif

// Option prefixes -----------------------------------------------------------

#define LOCATION_PREFIX "location="
#define LOCATION_PREFIX_LEN (sizeof(LOCATION_PREFIX) - 1)
#define CANNED_ACL_PREFIX "canned_acl="
#define CANNED_ACL_PREFIX_LEN (sizeof(CANNED_ACL_PREFIX) - 1)

#define CERTIFICATE_INFO_PREFIX "ca"
#define CERTIFICATE_INFO_PREFIX_LEN (sizeof(CERTIFICATE_INFO_PREFIX) - 1)
#define PROTOCOL_PREFIX "protocol="
#define PROTOCOL_PREFIX_LEN (sizeof(PROTOCOL_PREFIX) - 1)

#define OBJECT_POSITION "position="
#define OBJECT_POSITION_LEN (sizeof(PROTOCOL_PREFIX) - 1)

#define USE_KMS "use_kms="
#define USE_KMS_LEN (sizeof(USE_KMS) - 1)

#define USE_SSEC "use_ssec="
#define USE_SSEC_LEN (sizeof(USE_SSEC) - 1)

#define UPLOAD_SLICE_SIZE "upload_slice_size="
#define UPLOAD_SLICE_SIZE_LEN (sizeof(UPLOAD_SLICE_SIZE) - 1)

#define DESTINATION_BUCKET "destination_bucket="
#define DESTINATION_BUCKET_LEN (sizeof(DESTINATION_BUCKET) - 1)

#define DESTINATION_KEY "destination_key="
#define DESTINATION_KEY_LEN (sizeof(DESTINATION_KEY) - 1)

#define PART_NUM "part_num="
#define PART_NUM_LEN (sizeof(PART_NUM) - 1)

#define PREFIX_PREFIX "prefix="
#define PREFIX_PREFIX_LEN (sizeof(PREFIX_PREFIX) - 1)
#define MARKER_PREFIX "marker="
#define MARKER_PREFIX_LEN (sizeof(MARKER_PREFIX) - 1)
#define DELIMITER_PREFIX "delimiter="
#define DELIMITER_PREFIX_LEN (sizeof(DELIMITER_PREFIX) - 1)
#define UPLOADIDMARKER_PREFIX "uploadidmarker="
#define UPLOADIDMARKER_PREFIX_LEN (sizeof(UPLOADIDMARKER_PREFIX) - 1)
#define VERSIONIDMARKER_PREFIX "versionidmarker="
#define VERSIONIDMARKER_PREFIX_LEN (sizeof(VERSIONIDMARKER_PREFIX) - 1)
#define MAXKEYS_PREFIX "maxkeys="
#define MAXKEYS_PREFIX_LEN (sizeof(MAXKEYS_PREFIX) - 1)
#define MAXUPLOADS_PREFIX "maxuploads="
#define MAXUPLOADS_PREFIX_LEN (sizeof(MAXUPLOADS_PREFIX) - 1)
#define FILENAME_PREFIX "file_name="
#define FILENAME_PREFIX_LEN (sizeof(FILENAME_PREFIX) - 1)
#define CONTENT_LENGTH_PREFIX "content_length="
#define CONTENT_LENGTH_PREFIX_LEN (sizeof(CONTENT_LENGTH_PREFIX) - 1)
#define CACHE_CONTROL_PREFIX "cache_control="
#define CACHE_CONTROL_PREFIX_LEN (sizeof(CACHE_CONTROL_PREFIX) - 1)
#define CONTENT_TYPE_PREFIX "content_type="
#define CONTENT_TYPE_PREFIX_LEN (sizeof(CONTENT_TYPE_PREFIX) - 1)
#define MD5_PREFIX "md5="
#define MD5_PREFIX_LEN (sizeof(MD5_PREFIX) - 1)
#define CONTENT_DISPOSITION_FILENAME_PREFIX "content_disposition_filename="
#define CONTENT_DISPOSITION_FILENAME_PREFIX_LEN \
    (sizeof(CONTENT_DISPOSITION_FILENAME_PREFIX) - 1)
#define CONTENT_ENCODING_PREFIX "content_encoding="
#define CONTENT_ENCODING_PREFIX_LEN (sizeof(CONTENT_ENCODING_PREFIX) - 1)
#define EXPIRES_PREFIX "expires="
#define EXPIRES_PREFIX_LEN (sizeof(EXPIRES_PREFIX) - 1)
#define X_AMZ_META_PREFIX "x-amz-meta-"
#define X_AMZ_META_PREFIX_LEN (sizeof(X_AMZ_META_PREFIX) - 1)
#define USE_SERVER_SIDE_ENCRYPTION_PREFIX "useServerSideEncryption="
#define USE_SERVER_SIDE_ENCRYPTION_PREFIX_LEN \
    (sizeof(USE_SERVER_SIDE_ENCRYPTION_PREFIX) - 1)
#define IF_MODIFIED_SINCE_PREFIX "if_modified_since="
#define IF_MODIFIED_SINCE_PREFIX_LEN (sizeof(IF_MODIFIED_SINCE_PREFIX) - 1)
#define IF_NOT_MODIFIED_SINCE_PREFIX "ifNotmodifiedSince="
#define IF_NOT_MODIFIED_SINCE_PREFIX_LEN \
    (sizeof(IF_NOT_MODIFIED_SINCE_PREFIX) - 1)
#define IF_MATCH_PREFIX "ifMatch="
#define IF_MATCH_PREFIX_LEN (sizeof(IF_MATCH_PREFIX) - 1)
#define IF_NOT_MATCH_PREFIX "ifNotMatch="
#define IF_NOT_MATCH_PREFIX_LEN (sizeof(IF_NOT_MATCH_PREFIX) - 1)
#define START_BYTE_PREFIX "start_byte="
#define START_BYTE_PREFIX_LEN (sizeof(START_BYTE_PREFIX) - 1)
#define BYTE_COUNT_PREFIX "byte_count="
#define BYTE_COUNT_PREFIX_LEN (sizeof(BYTE_COUNT_PREFIX) - 1)

#define THREAD_NUM_PREFIX "thread_num="
#define THREAD_NUM_PREFIX_LEN (sizeof(THREAD_NUM_PREFIX) - 1)
#define CHECK_POINT "check_point="
#define CHECK_POINT_LEN (sizeof(CHECK_POINT) - 1)

#define ALL_DETAILS_PREFIX "allDetails="
#define ALL_DETAILS_PREFIX_LEN (sizeof(ALL_DETAILS_PREFIX) - 1)
#define NO_STATUS_PREFIX "noStatus="
#define NO_STATUS_PREFIX_LEN (sizeof(NO_STATUS_PREFIX) - 1)
#define RESOURCE_PREFIX "resource="
#define RESOURCE_PREFIX_LEN (sizeof(RESOURCE_PREFIX) - 1)
#define TARGET_BUCKET_PREFIX "targetBucket="
#define TARGET_BUCKET_PREFIX_LEN (sizeof(TARGET_BUCKET_PREFIX) - 1)
#define TARGET_PREFIX_PREFIX "targetPrefix="
#define TARGET_PREFIX_PREFIX_LEN (sizeof(TARGET_PREFIX_PREFIX) - 1)
#define KEY_PREFIX "key="
#define KEY_PREFIX_LEN (sizeof(KEY_PREFIX) - 1)
#define UPLOADID_PREFIX "upload_id="
#define UPLOADID_PREFIX_LEN (sizeof(UPLOADID_PREFIX) - 1)
#define AUTO_SPLIT_PREFIX "auto_split="
#define AUTO_SPLIT_PREFIX_LEN (sizeof(AUTO_SPLIT_PREFIX) - 1)
#define VERSIONID_PREFIX "version_id="
#define VERSIONID_PREFIX_LEN (sizeof(VERSIONID_PREFIX) - 1)
#define RECIVE_STREAM_LENGTH 256
#define STATUS_PREFIX "status="
#define STATUS_PREFIX_LEN (sizeof(STATUS_PREFIX) - 1)
#define GRANTEE_PREFIX "grantee="
#define GRANTEE_PREFIX_LEN (sizeof(GRANTEE_PREFIX) - 1)
#define PERMISSION_PREFIX "permission="
#define PERMISSION_PREFIX_LEN (sizeof(PERMISSION_PREFIX) - 1)
#define OWNERID_PREFIX "ownerID="
#define OWNERID_PREFIX_LEN (sizeof(OWNERID_PREFIX) - 1)
#define OWNERID_DISPLAY_NAME_PREFIX "owner_display_name="
#define OWNERID_DISPLAY_NAME_PREFIX_LEN (sizeof(OWNERID_DISPLAY_NAME_PREFIX) - 1)
#define ID_PREFIX "id="
#define ID_PREFIX_LEN (sizeof(ID_PREFIX) - 1)
#define DAYS_PREFIX "days="
#define DAYS_PREFIX_LEN (sizeof(DAYS_PREFIX) - 1)
#define DATE_PREFIX "date="
#define DATE_PREFIX_LEN (sizeof(DATE_PREFIX) - 1)
#define POLICY_PREFIX "policy="
#define POLICY_PREFIX_LEN (sizeof(POLICY_PREFIX) - 1)
#define WEBSITE_REDIRECT_LOCATION_PREFIX "website_redirect_location="
#define WEBSITE_REDIRECT_LOCATION_PREFIX_LEN (sizeof(WEBSITE_REDIRECT_LOCATION_PREFIX) - 1)
#define BODY_PREFIX "body="
#define BODY_PREFIX_LEN (sizeof(BODY_PREFIX) - 1)
#define RULE_COUNT_PREFIX "ruleCount="
#define RULE_COUNT_PREFIX_LEN (sizeof(RULE_COUNT_PREFIX) - 1)
#define ALLOWED_METHOD_PREFIX "allowed_method="
#define ALLOWED_METHOD_PREFIX_LEN (sizeof(ALLOWED_METHOD_PREFIX) - 1)
#define ALLOWED_ORIGIN_PREFIX "allowed_origin="
#define ALLOWED_ORIGIN_PREFIX_LEN (sizeof(ALLOWED_ORIGIN_PREFIX) - 1)
#define ALLOWED_HEADER_PREFIX "allowed_header="
#define ALLOWED_HEADER_PREFIX_LEN (sizeof(ALLOWED_HEADER_PREFIX) - 1)
#define MAX_AGE_SECONDS_PREFIX "max_age_seconds="
#define MAX_AGE_SECONDS_PREFIX_LEN (sizeof(MAX_AGE_SECONDS_PREFIX) - 1)
#define EXPOSE_HEADER_PREFIX "expose_header="
#define EXPOSE_HEADER_PREFIX_LEN (sizeof(EXPOSE_HEADER_PREFIX) - 1)
#define METADATA_DIRECTIVE_PREFIX "metadataDirective="
#define METADATA_DIRECTIVE_PREFIX_LEN (sizeof(METADATA_DIRECTIVE_PREFIX) - 1)

#define REDIRECT_ALL_HOSTNAME_PREFIX "redirectAllHostName="
#define REDIRECT_ALL_HOSTNAME_PREFIX_LEN (sizeof(REDIRECT_ALL_HOSTNAME_PREFIX) - 1)
#define REDIRECT_ALL_PROTOCOL_PREFIX "redirectAllProtocol="
#define REDIRECT_ALL_PROTOCOL_PREFIX_LEN (sizeof(REDIRECT_ALL_PROTOCOL_PREFIX) - 1)
#define SUFFIX_PREFIX "suffix="
#define SUFFIX_PREFIX_LEN (sizeof(SUFFIX_PREFIX) - 1)
#define KEY_PREFIX "key="
#define KEY_PREFIX_LEN (sizeof(KEY_PREFIX) - 1)
#define KEY_PREFIX_EQUALS_PREFIX "key_prefix_equals="
#define KEY_PREFIX_EQUALS_PREFIX_LEN (sizeof(KEY_PREFIX_EQUALS_PREFIX) - 1)
#define REPLACE_KEY_PREFIX_WITH_PREFIX "replace_key_prefix_with="
#define REPLACE_KEY_PREFIX_WITH_PREFIX_LEN (sizeof(REPLACE_KEY_PREFIX_WITH_PREFIX) - 1)
#define REPLACE_KEY_WITH_PREFIX "replace_key_with="
#define REPLACE_KEY_WITH_PREFIX_LEN (sizeof(REPLACE_KEY_WITH_PREFIX) - 1)
#define HTTP_ERROR_CODE_RETURNED_EQUALS_PREFIX "http_errorcode_returned_equals="
#define HTTP_ERROR_CODE_RETURNED_EQUALS_PREFIX_LEN (sizeof(HTTP_ERROR_CODE_RETURNED_EQUALS_PREFIX) - 1)
#define HTTP_REDIRECT_CODE_PREFIX "http_redirect_code="
#define HTTP_REDIRECT_CODE_PREFIX_LEN (sizeof(HTTP_REDIRECT_CODE_PREFIX) - 1)
#define HOSTNAME_PREFIX "host_name="
#define HOSTNAME_PREFIX_LEN (sizeof(HOSTNAME_PREFIX) - 1)
#define PROTOCOL_PREFIX "protocol="
#define PROTOCOL_PREFIX_LEN (sizeof(PROTOCOL_PREFIX) - 1)
#define QUIET_PREFIX "quiet="
#define QUIET_PREFIX_LEN (sizeof(QUIET_PREFIX) - 1)
#define PARTNUMBER_PREFIX "part_number="
#define PARTNUMBER_PREFIX_LEN (sizeof(PARTNUMBER_PREFIX) - 1)

#define MAXPARTS_PREFIX "maxparts="
#define MAXPARTS_PREFIX_LEN (sizeof(MAXPARTS_PREFIX) - 1)
#define PARTNUMBER_MARKER_PREFIX "partNumberMarker="
#define PARTNUMBER_MARKER_PREFIX_LEN (sizeof(PARTNUMBER_MARKER_PREFIX) - 1)

#define TOPIC_PREFIX "topic="
#define TOPIC_PREFIX_LEN (sizeof(TOPIC_PREFIX) - 1)
#define EVENTS_PREFIX "events="
#define EVENTS_PREFIX_LEN (sizeof(EVENTS_PREFIX) - 1)
#define NAME_PREFIX "name="
#define NAME_PREFIX_LEN (sizeof(NAME_PREFIX) - 1)
#define VALUE_PREFIX "value="
#define VALUE_PREFIX_LEN (sizeof(VALUE_PREFIX) - 1)
#define TIER_PREFIX "tier="
#define TIER_PREFIX_LEN (sizeof(TIER_PREFIX) - 1)
#define STORAGE_CLASS_PREFIX "storage_class="
#define STORAGE_CLASS_PREFIX_LEN (sizeof(STORAGE_CLASS_PREFIX) - 1)
#define ORIGIN_PREFIX "origin="
#define ORIGIN_PREFIX_LEN (sizeof(ORIGIN_PREFIX) - 1)
#define REQUEST_HEADER_PREFIX "requestHeader="
#define REQUEST_HEADER_PREFIX_LEN (sizeof(REQUEST_HEADER_PREFIX) - 1)
#define RH_NUMBER_PREFIX "rhNumber="
#define RH_NUMBER_PREFIX_LEN (sizeof(RH_NUMBER_PREFIX) - 1)
#define ENABLE_CHECKPOINT "enable_check_point="
#define ENABLE_CHECKPOINT_LEN (sizeof(ENABLE_CHECKPOINT) - 1)
#define CHECKPOINT_FILE "checkpointFile="
#define CHECKPOINT_FILE_LEN (sizeof(CHECKPOINT_FILE) - 1)
#define PART_SIZE "part_size="
#define PART_SIZE_LEN (sizeof(PART_SIZE) - 1)
#define TASK_NUM "task_num="
#define TASK_NUM_LEN (sizeof(TASK_NUM) - 1)
#define IMAGE_PROCESS "imageProc="
#define IMAGE_PROCESS_LEN (sizeof(IMAGE_PROCESS) - 1)
#define LOW_SPEED "lowspeed="
#define LOW_SPEED_LEN (sizeof(LOW_SPEED) - 1)
#define CONNECTION "connection="
#define CONNECTION_LEN (sizeof(CONNECTION) - 1)
#define KEY_MARKER_PREFIX "key_marker="
#define KEY_MARKER_PREFIX_LEN (sizeof(KEY_MARKER_PREFIX) - 1)
#define RULE_NUMBER "rule_num="
#define RULE_NUMBER_LEN (sizeof(RULE_NUMBER) - 1)
#define RULE_DAYS_PREFIX "expdays="
#define RULE_DAYS_PREFIX_LEN (sizeof(RULE_DAYS_PREFIX) - 1)
#define RULE_DATE_PREFIX "date="
#define RULE_DATE_PREFIX_LEN (sizeof(RULE_DATE_PREFIX) - 1)
#define RULE_ID_PREFIX "id="
#define RULE_ID_PREFIX_LEN (sizeof(RULE_ID_PREFIX) - 1)
#define RULE_STATUS_PREFIX "status="
#define RULE_STATUS_PREFIX_LEN (sizeof(RULE_STATUS_PREFIX) - 1)
#define RULE_PREFIX "prefix="
#define RULE_PREFIX_LEN (sizeof(RULE_PREFIX) - 1)
#define RULE_NONCURRNET_DAY_PREFIX "noncurrent_days="
#define RULE_NONCURRNET_DAY_PREFIX_LEN (sizeof(RULE_NONCURRNET_DAY_PREFIX) - 1)
#define RULE_TRANSITION_PREFIX "cur_transition="
#define RULE_TRANSITION_PREFIX_LEN (sizeof(RULE_TRANSITION_PREFIX) - 1)
#define RULE_NONCURRENT_TRANSITION_PREFIX "noncurrent_transition="
#define RULE_NONCURRENT_TRANSITION_PREFIX_LEN (sizeof(RULE_NONCURRENT_TRANSITION_PREFIX) - 1)
#define KEY_ALLOWED_METHOD_LEN (sizeof(KEY_ALLOWED_METHOD) - 1)
#define KEY_ALLOWED_METHOD "allowed_method="
#define KEY_AM_NUM_LEN (sizeof(KEY_AM_NUM) - 1)
#define KEY_AM_NUM "am_num="
#define KEY_REQUEST_HEADER_LEN (sizeof(KEY_REQUEST_HEADER) - 1)
#define KEY_REQUEST_HEADER "request_header="
#define KEY_REQUEST_HEADER_NUM_LEN (sizeof(KEY_REQUEST_HEADER_NUM) - 1)
#define KEY_REQUEST_HEADER_NUM "rh_num="
#define KEY_CKEY_LEN (sizeof(KEY_CKEY) - 1)
#define KEY_CKEY "ckey="
#define KEY_CORIGIN_LEN (sizeof(KEY_CORIGIN) - 1)
#define KEY_CORIGIN "corigin="
#define ALLOWED_METHOD_NUM "allowed_method_num="
#define ALLOWED_METHOD_NUM_LEN (sizeof(ALLOWED_METHOD_NUM) - 1)
#define ALLOWED_ORIGIN_NUM "allowed_origin_num="
#define ALLOWED_ORIGIN_NUM_LEN (sizeof(ALLOWED_ORIGIN_NUM) - 1)
#define ALLOWED_HEADER_NUM "allowed_header_num="
#define ALLOWED_HEADER_NUM_LEN (sizeof(ALLOWED_HEADER_NUM) - 1)
#define EXPOSE_HEADER_NUM "expose_header_num="
#define EXPOSE_HEADER_NUM_LEN (sizeof(EXPOSE_HEADER_NUM) - 1)

// loging
#define grantee_type_str "grantee_type="
#define grantee_type_str_length (sizeof("grantee_type=") - 1)
#define user_id_str "user_id="
#define user_id_str_length (sizeof("user_id=") - 1)
#define display_name_str "display_name="
#define display_name_length (sizeof("display_name=") - 1)
#define permission_str "permission="
#define permission_length (sizeof("permission=") - 1)
#define email_address_str "email_address="
#define email_address_length (sizeof("email_address=") - 1)
#define owner_id_str "owner_id="
#define owner_id_str_length (sizeof(owner_id_str) - 1)
#define owner_display_name_str "owner_display_name="
#define owner_display_name_length (sizeof(owner_display_name_str) - 1)
#define delivered_str "delivered="
#define delivered_length (sizeof("delivered=") - 1)


//tmp auth
#define TMP_AUTH_EXPIRES_PREFIX "tmp_auth_expires="
#define TMP_AUTH_EXPIRES_PREFIX_LEN (sizeof(TMP_AUTH_EXPIRES_PREFIX) - 1)

#define TIER_PREFIX "tier="
#define TIER_PREFIX_LEN (sizeof(TIER_PREFIX) - 1)
#define IS_COPY_PREFIX "is_copy="
#define IS_COPY_PREFIX_LEN (sizeof(IS_COPY_PREFIX) - 1)

#define INIT_CERT "init_path="
#define INIT_CERT_LEN (sizeof(INIT_CERT) - 1)

#define INIT_CERT_BYBUFFER "init_buffer="
#define INIT_CERT_BYBUFFER_LEN (sizeof(INIT_CERT_BYBUFFER) - 1)

#define INIT_CERT_BUFFER_LEN "buffer_len="
#define INIT_CERT_BUFFER_LEN_LEN (sizeof(INIT_CERT_BUFFER_LEN) - 1)

#define USE_OBS_AUTH "use_obs_auth"
#define USE_OBS_AUTH_LEN (sizeof(USE_OBS_AUTH) - 1)
#define USE_S3_AUTH "use_s3_auth"
#define USE_S3_AUTH_LEN (sizeof(USE_S3_AUTH) - 1)

// posix add 
#define BUCKET_QUOTA "quota="
#define BUCKET_QUOTA_LEN (sizeof(BUCKET_QUOTA) - 1)

#define BUCKET_LIST_TYPE "list_type="
#define BUCKET_LIST_TYPE_LEN (sizeof(BUCKET_LIST_TYPE) - 1)

#define PAUSE_THREAD_NUM 3

/***********************结构定义*************************************/

static struct option longOptionsG[] = 
{
    {"force",                   no_argument,            0,   'f' },
    {"vhost-style",             no_argument,            0,   'h' },
    {"unencrypted",             no_argument,            0,   'u' },
    {"show-properties",         no_argument,            0,   's' },
    {"retries",                 no_argument,            0,   'r' },
    {0,                         0,                      0,    0  }
};


/********************公共函数******************************************/
static uint64_t convertInt(const char *str, const char *paramName)
{
    uint64_t ret = 0;

    while (*str) {
        if (!isdigit(*str)) {
            fprintf(stderr, "\nERROR: Nondigit in %s parameter: %s\n", paramName, str);
        }
        ret *= 10;
        ret += (*str++ - '0');
    }
    printf("ret:%lu\n",ret);
    return ret;
}

static void usageExit()
{
    printf("error");

}

static obs_status responsePropertiesCallback(const obs_response_properties *properties, void *callback_data)
{
    (void) callback_data;

    if (!showResponsePropertiesG) {
        return OBS_STATUS_OK;
    }

#define print_nonnull(name, field)                                 \
    do {                                                           \
        if (properties-> field) {                                  \
            printf("%s: %s\n", name, properties-> field);          \
        }                                                          \
    } while (0)
    
    print_nonnull("ETag", etag);
    print_nonnull("expiration", expiration);
    print_nonnull("website_redirect_location", website_redirect_location);
    print_nonnull("version_id", version_id);
    if (properties->last_modified > 0) {
        char timebuf[256] = {0};
        time_t t = (time_t) properties->last_modified;
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
        printf("Last-Modified: %s\n", timebuf);
    }
    int i;
    for (i = 0; i < properties->meta_data_count; i++) {
        printf("x-amz-meta-%s: %s\n", properties->meta_data[i].name,
               properties->meta_data[i].value);
    }
    return OBS_STATUS_OK;
}

static void responseCompleteCallback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    (void) callback_data;

    statusG = status;
    common_error_handle(error);
}


static void put_object_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    put_object_callback_data *data = (put_object_callback_data *)callback_data;
    data->put_status = status;
}

static void progress_callback(uint64_t now, uint64_t total, void* callback_data)
{
    if (total)
    {
        printf("progress is %d%% \n", (now * 100) / total);
    }
}
// head object ---------------------------------------------------------------
static void test_head_object_new(int argc, char **argv, int optindex)
{
    // init struct obs_option
    if (optindex == argc) {
        fprintf(stderr, "\nERROR: Missing parameter: bucket\n");
    }

    obs_options option;
    init_obs_options(&option);
    head_object_data data = {0};

    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);
  
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    char *key = argv[optindex++];

    obs_response_handler response_handler =
    { 
       &head_properties_callback,
       &head_complete_callback
    };
    while (optindex < argc)
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }

    obs_head_object(&option,key, &response_handler, &data);

    if (data.ret_status == OBS_STATUS_OK) {
       printf("head object successfully. \n");
    }
    else {
       printf("head object failed(%s).\n", obs_get_status_name(data.ret_status));
    }
    
}

static void test_head_bucket_new(int argc, char **argv, int optindex)
{
    // init struct obs_option
    if (optindex == argc) {
        fprintf(stderr, "\nERROR: Missing parameter: bucket\n");
    }
    obs_options option;
    init_obs_options(&option);
    obs_status ret_status = OBS_STATUS_BUTT;

    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);
  
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
       0, &head_complete_callback
    };

    while (optindex < argc)
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
           option.bucket_options.certificate_info = ca_info;       
           option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }

    obs_head_bucket(&option,&response_handler, &ret_status);

    if (ret_status == OBS_STATUS_OK) {
       printf("head bucket successfully. \n");
    }
    else {
       printf("head bucket failed(%s).\n", obs_get_status_name(ret_status));
    }
    
}


static void test_batch_delete_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    char *key_file = argv[optindex];
    printf("Bucket's name is == %s \n", bucket_name);
    int num = 0;
    int i = 0;

    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    obs_object_info objectinfo[OBS_MAX_DELETE_OBJECT_NUMBER];
    
    FILE *fd = fopen(key_file, "r");
    char *key_version[OBS_MAX_DELETE_OBJECT_NUMBER];
    char *key = NULL;
    char *version_id = NULL;
    char *delim = " ";
    char *find = NULL;

    
    while(!feof(fd))
    {
        if (num >= OBS_MAX_DELETE_OBJECT_NUMBER)
        {
            break;
        }
        key_version[num] = (char *) malloc(1024);
        memset_s(key_version[num], sizeof(key_version[num]),0, 1024);  
        fgets(key_version[num],1024,fd);
        find = strchr(key_version[num], '\n');
        if (find)
        {
            *find = '\0';
        }
        if (!feof(fd))
        {   
            key = strtok(key_version[num],delim);
            version_id = strtok(NULL,delim); 
            objectinfo[num].key = key;
            objectinfo[num].version_id = !strcmp(version_id, "0") ? NULL : version_id;
            num++;
            
        }
    }
    
    fclose(fd);
    
    obs_delete_object_info delobj;
    memset_s(&delobj,sizeof(obs_delete_object_info),0,sizeof(obs_delete_object_info));
    delobj.keys_number = num;

    obs_delete_object_handler handler =
    { 
        {0, &response_complete_callback},
        &delete_objects_data_callback
    };
    
    while (optindex < argc)
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
           option.bucket_options.certificate_info = ca_info;       
           option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    option.bucket_options.host_name = HOST_NAME;
    batch_delete_objects(&option, objectinfo, &delobj, 0, &handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test batch_delete_objects successfully. \n");
    }
    else
    {
        printf("test batch_delete_objects faied(%s).\n", obs_get_status_name(ret_status));
    }

    for(i=0; i<num; i++)
    {
        
        free(key_version[i]);
    }
}

static obs_canned_acl get_acl_from_argv(char *param)
{
    obs_canned_acl ret_acl = OBS_CANNED_ACL_PRIVATE;
    char *val = &(param[CANNED_ACL_PREFIX_LEN]);
    if (!strcmp(val, "private")) {
        ret_acl = OBS_CANNED_ACL_PRIVATE;
    }
    else if (!strcmp(val, "public-read")) {
        ret_acl = OBS_CANNED_ACL_PUBLIC_READ;
    }
    else if (!strcmp(val, "public-read-write")) {
        ret_acl = OBS_CANNED_ACL_PUBLIC_READ_WRITE;
    }
    else if (!strcmp(val, "authenticated-read ")) {
        ret_acl = OBS_CANNED_ACL_AUTHENTICATED_READ;
    }
    else if (!strcmp(val, "bucket-owner-read")) {
        ret_acl = OBS_CANNED_ACL_BUCKET_OWNER_READ;
    }
    else if (!strcmp(val, "log-delivery-write")) {
        ret_acl = OBS_CANNED_ACL_LOG_DELIVERY_WRITE;
    }
    else if (!strcmp(val, "bucket-owner-full-control")) {
        ret_acl = OBS_CANNED_ACL_BUCKET_OWNER_FULL_CONTROL;
    }
    else if (!strcmp(val, "public-read-delivered")) {
        ret_acl = OBS_CANNED_ACL_PUBLIC_READ_DELIVERED;
    }
    else if (!strcmp(val, "public-read-write-delivered")) {
        ret_acl = OBS_CANNED_ACL_PUBLIC_READ_WRITE_DELIVERED;
    }
    else {
        fprintf(stderr, "\nERROR: Unknown canned ACL: %s\n", val);
    }

    return ret_acl;
}

static obs_storage_class get_storage_class_from_argv(char *param)
{
    obs_storage_class ret_storage_class = OBS_STORAGE_CLASS_STANDARD;
    char *val = &(param[STORAGE_CLASS_PREFIX_LEN]);
    printf("storage_class is: %s\n", val);
    if (!strcmp(val, "standard")) {
        ret_storage_class = OBS_STORAGE_CLASS_STANDARD;
    }else if (!strcmp(val, "standard_ia")) {
        ret_storage_class = OBS_STORAGE_CLASS_STANDARD_IA;
    }else if (!strcmp(val, "glacier")) {
        ret_storage_class = OBS_STORAGE_CLASS_GLACIER;
    }else {
        fprintf(stderr, "ERROR: Unknown storage class: %s.\n", val);
    }

    return ret_storage_class;
}

static obs_protocol get_protocol_from_argv(char *param)
{
    obs_protocol ret_protocol = OBS_PROTOCOL_HTTP;
    char *val = &(param[PROTOCOL_PREFIX_LEN]);
    printf("protocol is: %s\n", val);
    if (!strcmp(val, "http")) {
        ret_protocol = OBS_PROTOCOL_HTTP;
    }else if (!strcmp(val, "https")) {
        ret_protocol = OBS_PROTOCOL_HTTPS;
    }else {
        fprintf(stderr, "ERROR: Unknown protocol: %s.\n", val);
    }

    return ret_protocol;
}



// create bucket ---------------------------------------------------------------
static void test_create_bucket_new(int argc, char **argv, int optindex)
{
    obs_options option;
    char *location = NULL;
    obs_status ret_status = OBS_STATUS_OK;
    obs_canned_acl bucket_acl = OBS_CANNED_ACL_PRIVATE;
    temp_auth_configure tempauth;
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));
    obs_status status = OBS_STATUS_OK;
    
    if (optindex == argc) {
        fprintf(stderr, "\nERROR: Missing parameter: bucket\n");
    }
    
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);

    if(optindex < argc)
    {
        char *cert_info = argv[optindex];
        if (!strncmp(cert_info, INIT_CERT, INIT_CERT_LEN))
        {   
            if (!strcmp(&cert_info[INIT_CERT_LEN],"0"))
            {
                status = init_certificate_by_path(OBS_PROTOCOL_HTTP, OBS_NO_CERTIFICATE, NULL, 0);
            }
            else if (!strcmp(&cert_info[INIT_CERT_LEN],"1"))
            {
                status = init_certificate_by_path(OBS_PROTOCOL_HTTPS, OBS_DEFAULT_CERTIFICATE, NULL, 0);
            }
            else
            {
                status = init_certificate_by_path(OBS_PROTOCOL_HTTPS, OBS_DEFINED_CERTIFICATE, &cert_info[INIT_CERT_LEN], 
                    strlen(&cert_info[INIT_CERT_LEN]));
            }
            
        }
        if (!strncmp(cert_info, INIT_CERT_BYBUFFER, INIT_CERT_BYBUFFER_LEN))
        {
            char *buffer_len = argv[optindex+1];
            if (!strncmp(buffer_len, INIT_CERT_BUFFER_LEN, INIT_CERT_BUFFER_LEN_LEN))
            {
                status = init_certificate_by_buffer(&cert_info[INIT_CERT_BYBUFFER_LEN], atoi(&buffer_len[INIT_CERT_BUFFER_LEN_LEN]));
            }
        }

        if (status != OBS_STATUS_OK)
        {
            printf("init certificate failed\n");
            return;
        }
    }

    init_obs_options(&option);

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, LOCATION_PREFIX, LOCATION_PREFIX_LEN)) {
            location = &(param[LOCATION_PREFIX_LEN]);
            printf("locationconstrint is: %s\n", location);
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            bucket_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN)) {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = &(param[CERTIFICATE_INFO_PREFIX_LEN]);
        }
        //tmp auth
        else if (!strncmp(param, TMP_AUTH_EXPIRES_PREFIX,TMP_AUTH_EXPIRES_PREFIX_LEN)){
            tempauth.callback_data = (void *)(&ptrResult);
            int auth_expire = atoi(&param[TMP_AUTH_EXPIRES_PREFIX_LEN]);
            tempauth.expires = auth_expire;
            tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
            option.temp_auth = &tempauth;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    create_bucket(&option, bucket_acl, location, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("create Bucket [%s] successfully. \n", bucket_name);
    }
    else
    {
        printf("create Bucket [%s] failed.\n", bucket_name);
    }
}


static void test_create_pfs_bucket_new(int argc, char **argv, int optindex)
{
    obs_options option;
    char *location = NULL;
    obs_status ret_status = OBS_STATUS_OK;
    obs_canned_acl bucket_acl = OBS_CANNED_ACL_PRIVATE;
    temp_auth_configure tempauth;
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));
    obs_status status = OBS_STATUS_OK;
    
    if (optindex == argc) {
        fprintf(stderr, "\nERROR: Missing parameter: bucket\n");
    }
    
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);

    if(optindex < argc)
    {
        char *cert_info = argv[optindex];
        if (!strncmp(cert_info, INIT_CERT, INIT_CERT_LEN))
        {   
            if (!strcmp(&cert_info[INIT_CERT_LEN],"0"))
            {
                status = init_certificate_by_path(OBS_PROTOCOL_HTTP, OBS_NO_CERTIFICATE, NULL, 0);
            }
            else if (!strcmp(&cert_info[INIT_CERT_LEN],"1"))
            {
                status = init_certificate_by_path(OBS_PROTOCOL_HTTPS, OBS_DEFAULT_CERTIFICATE, NULL, 0);
            }
            else
            {
                status = init_certificate_by_path(OBS_PROTOCOL_HTTPS, OBS_DEFINED_CERTIFICATE, &cert_info[INIT_CERT_LEN], 
                    strlen(&cert_info[INIT_CERT_LEN]));
            }
            
        }
        if (!strncmp(cert_info, INIT_CERT_BYBUFFER, INIT_CERT_BYBUFFER_LEN))
        {
            char *buffer_len = argv[optindex+1];
            if (!strncmp(buffer_len, INIT_CERT_BUFFER_LEN, INIT_CERT_BUFFER_LEN_LEN))
            {
                status = init_certificate_by_buffer(&cert_info[INIT_CERT_BYBUFFER_LEN], atoi(&buffer_len[INIT_CERT_BUFFER_LEN_LEN]));
            }
        }

        if (status != OBS_STATUS_OK)
        {
            printf("init certificate failed\n");
            return;
        }
    }

    init_obs_options(&option);

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, LOCATION_PREFIX, LOCATION_PREFIX_LEN)) {
            location = &(param[LOCATION_PREFIX_LEN]);
            printf("locationconstrint is: %s\n", location);
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            bucket_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = &(param[CERTIFICATE_INFO_PREFIX_LEN]);
        }
        //tmp auth
        else if (!strncmp(param, TMP_AUTH_EXPIRES_PREFIX,TMP_AUTH_EXPIRES_PREFIX_LEN)){
            tempauth.callback_data = (void *)(&ptrResult);
            int auth_expire = atoi(&param[TMP_AUTH_EXPIRES_PREFIX_LEN]);
            tempauth.expires = auth_expire;
            tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
            option.temp_auth = &tempauth;
        }
    }

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;

    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    create_pfs_bucket(&option, bucket_acl, location, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("create Bucket [%s] successfully. \n", bucket_name);
    }
    else
    {
        printf("create Bucket [%s] failed,ret(%d).\n", bucket_name,ret_status);
    }
}


// delete bucket ---------------------------------------------------------------
static void test_delete_bucket_new(int argc, char **argv, int optindex)
{
    if (optindex == argc) {
        fprintf(stderr, "\nERROR: Missing parameter: bucket\n");
    }

    obs_options option;
    obs_status  ret_status = OBS_STATUS_BUTT;
    init_obs_options(&option);
    
    char *bucket_name =  argv[optindex++];
    printf("Bucket's name is: %s, it will be deleted\n", bucket_name);

    
    
    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        NULL,
        &response_complete_callback
    };

    delete_bucket(&option, &response_handler, &ret_status);

    if (ret_status == OBS_STATUS_OK) 
    {
        printf("delete bucket successfully. \n");
    }
    else
    {
        printf("delete bucket failed(%s).\n", obs_get_status_name(ret_status));
    }
}


// put object ---------------------------------------------------------------
static int growbuffer_append(growbuffer **gb, const char *data, int dataLen)
{
    while (dataLen) {
        growbuffer *buf = *gb ? (*gb)->prev : 0;
        if (!buf || (buf->size == sizeof(buf->data))) {
            buf = (growbuffer *) malloc(sizeof(growbuffer));
            if (!buf) {
                return 0;
            }
            memset_s(buf, sizeof(growbuffer), 0, sizeof(growbuffer));
            buf->size = 0;
            buf->start = 0;
            if (*gb && (*gb)->prev) {
                buf->prev = (*gb)->prev;
                buf->next = *gb;
                (*gb)->prev->next = buf;
                (*gb)->prev = buf;
            }
            else {
                buf->prev = buf->next = buf;
                *gb = buf;
            }
        }

        int toCopy = (sizeof(buf->data) - buf->size);
        if (toCopy > dataLen) {
            toCopy = dataLen;
        }

        memcpy_s(&(buf->data[buf->size]), sizeof(buf->data)-buf->size, data, toCopy);
        
        buf->size += toCopy, data += toCopy, dataLen -= toCopy;
    }

    return 1;
}

static void growbuffer_read(growbuffer **gb, int amt, int *amtReturn, 
                            char *buffer)
{
    *amtReturn = 0;

    growbuffer *buf = *gb;

    if (!buf) {
        return;
    }

    *amtReturn = (buf->size > amt) ? amt : buf->size;

    memcpy_s(buffer, *amtReturn, &(buf->data[buf->start]), *amtReturn);
    
    buf->start += *amtReturn, buf->size -= *amtReturn;

    if (buf->size == 0) {
        if (buf->next == buf) {
            *gb = 0;
        }
        else {
            *gb = buf->next;
            buf->prev->next = buf->next;
            buf->next->prev = buf->prev;
        }
        free(buf);
    }
}

static void growbuffer_destroy(growbuffer *gb)
{
    growbuffer *start = gb;

    while (gb) {
        growbuffer *next = gb->next;
        free(gb);
        gb = (next == start) ? 0 : next;
    }
}


static int put_object_data_callback(int buffer_size, char *buffer,
                                 void *callback_data)
{
    put_object_callback_data *data = (put_object_callback_data *) callback_data;
    int ret = 0;

    if (data->content_length) {
        int toRead = ((data->content_length > (unsigned) buffer_size) ?
                    (unsigned) buffer_size : data->content_length);
        if (data->gb) {
            growbuffer_read(&(data->gb), toRead, &ret, buffer);
        }
        else if (data->infile) {
            ret = fread(buffer, 1, toRead, data->infile);
        }
    }

    data->content_length -= ret;

    if (data->content_length && !data->noStatus) {
        printf("%llu bytes remaining ", 
               (unsigned long long) data->content_length);
        printf("(%d%% complete) ...\n",
               (int) (((data->originalContentLength - 
                        data->content_length) * 100) /
                        data->originalContentLength));
    }

    return ret;
}
uint64_t read_bytes_from_file(char *localfile, put_object_callback_data *data)
{
        uint64_t content_length = 0;    
        const char *body = 0;
    if (localfile) {
        if (!content_length) {
            struct stat statbuf;
            if (stat(localfile, &statbuf) == -1) {
                fprintf(stderr, "\nERROR: Failed to stat file %s: ",
                localfile);
                perror(0);
                exit(-1);
            }
            content_length = statbuf.st_size;
        }
        if (!(data->infile = fopen(localfile, "rb"))) {
            fprintf(stderr, "\nERROR: Failed to open input file %s: ",
            localfile);
            perror(0);
            exit(-1);
        }
    }
    else {
    if (!content_length) {
        while (1) {
                int amtRead = strlen(body);
                if (amtRead == 0) {
                break;
                }
                growbuffer_append(&(data->gb), body, amtRead);
                content_length += amtRead;    
                if (amtRead <= (int) (strlen(body))) {      
                break;
                }
            }
        }
        else {
            growbuffer_append(&(data->gb), body, content_length);
        }
    }
    data->content_length = data->originalContentLength = content_length;
    return content_length;
}


static void test_append_object_new(int argc, char **argv, int optindex)
{
    char *file_name = 0;
    uint64_t content_length = 0;
    obs_options option;
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    char *body = 0;
    char *position = "0";
    
    printf("Bucket's name is == %s, object's name is == %s. \n", bucket_name, key);
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_put_properties put_properties;
    init_put_properties(&put_properties);


    //check parameters
    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, FILENAME_PREFIX, FILENAME_PREFIX_LEN)) {
            file_name = &(param[FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_LENGTH_PREFIX, CONTENT_LENGTH_PREFIX_LEN)) {
            content_length = convertInt(&(param[CONTENT_LENGTH_PREFIX_LEN]), "content_length");
        }
        else if (!strncmp(param, WEBSITE_REDIRECT_LOCATION_PREFIX, WEBSITE_REDIRECT_LOCATION_PREFIX_LEN)) {
            put_properties.website_redirect_location = &(param[WEBSITE_REDIRECT_LOCATION_PREFIX_LEN]);
        }
        else if (!strncmp(param, BODY_PREFIX, BODY_PREFIX_LEN)) {
            body = &(param[BODY_PREFIX_LEN]);
        }
        else if (!strncmp(param, CACHE_CONTROL_PREFIX, CACHE_CONTROL_PREFIX_LEN)) {
            put_properties.cache_control = &(param[CACHE_CONTROL_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_TYPE_PREFIX, CONTENT_TYPE_PREFIX_LEN)) {
            put_properties.content_type = &(param[CONTENT_TYPE_PREFIX_LEN]);
        }
        else if (!strncmp(param, MD5_PREFIX, MD5_PREFIX_LEN)) {
            put_properties.md5 = &(param[MD5_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_DISPOSITION_FILENAME_PREFIX, CONTENT_DISPOSITION_FILENAME_PREFIX_LEN)) {
            put_properties.md5 = &(param[CONTENT_DISPOSITION_FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_ENCODING_PREFIX, CONTENT_ENCODING_PREFIX_LEN)) {
            put_properties.content_encoding = &(param[CONTENT_ENCODING_PREFIX_LEN]);
        }
        else if (!strncmp(param, EXPIRES_PREFIX, EXPIRES_PREFIX_LEN)) {
            put_properties.expires = parseIso8601Time(&(param[EXPIRES_PREFIX_LEN]));
        }
        else if (!strncmp(param, START_BYTE_PREFIX, START_BYTE_PREFIX_LEN)) {
            put_properties.start_byte = convertInt(&(param[START_BYTE_PREFIX_LEN]), "start_byte");
        }
        else if (!strncmp(param, X_AMZ_META_PREFIX, X_AMZ_META_PREFIX_LEN)) {
            char *name = param;
            char *value = name;
            
             while (*value && (*value != '=')) {
                value ++;
             }
            *value = 0;
            value++;
            if (!put_properties.meta_data)
            {
                put_properties.meta_data = (obs_name_value *) malloc(sizeof(obs_name_value)*50);
                memset_s(put_properties.meta_data, sizeof(put_properties.meta_data),0, sizeof(obs_name_value)*50);
            }
            put_properties.meta_data[put_properties.meta_data_count].name = name;
            put_properties.meta_data[put_properties.meta_data_count++].value = value;
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            put_properties.canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN)) {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {      
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, OBJECT_POSITION, OBJECT_POSITION_LEN)) {
            position = &(param[OBJECT_POSITION_LEN]);
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    //read from local file to buffer
    put_object_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(put_object_callback_data));

    data.infile = 0;
    data.gb = 0;
    data.noStatus = 1;

    if (file_name) {
        if (!content_length) {
            content_length =  read_bytes_from_file(file_name, &data);
        }
        //Open the file
        if (!(data.infile = fopen(file_name, "rb" FOPEN_EXTRA_FLAGS))) {
            fprintf(stderr, "\nERROR: Failed to open input file %s: ", file_name);
            return;
        }
    }
    else 
    {
        if (!content_length) {
            while (1) {
                int amtRead = strlen(body);
                if (amtRead == 0) {
                    break;
                }
                growbuffer_append(&(data.gb), body, amtRead);
                content_length += amtRead;
                if (amtRead <= (int) (strlen(body))) {
                    break;
                }
            }
        }
        else {
            growbuffer_append(&(data.gb), body, content_length);
        }
    }

    data.content_length = data.originalContentLength = content_length;
    
    obs_append_object_handler putobjectHandler =
    { 
        { &responsePropertiesCallback,
          &put_object_complete_callback},
        &put_object_data_callback
    };
    
    append_object(&option,key,content_length,position,&put_properties,0,&putobjectHandler,&data);

    if (OBS_STATUS_OK == data.put_status) {
        printf("put object [%s,%s] successfully. \n", bucket_name,key);
    }
    else
    {
        printf("put object [%s,%s] faied(%s).\n", bucket_name,key,
            obs_get_status_name(data.put_status));
    }

    if (put_properties.meta_data)
    {
        free(put_properties.meta_data);
    }
}

// put object------------------------------------------------------------
static void test_put_object_new(int argc, char **argv, int optindex)
{
    char *file_name = 0;
    uint64_t content_length = 0;
    obs_options option;
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    char *body = 0;
    
    temp_auth_configure tempauth;
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));
    
    printf("Bucket's name is == %s, object's name is == %s. \n", bucket_name, key);
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    
    put_properties.upload_limit = 819200;
    put_properties.meta_data = (obs_name_value*)malloc(sizeof(obs_name_value)*10);
    memset_s(put_properties.meta_data, sizeof(put_properties.meta_data),0, sizeof(obs_name_value)*10);

    //check parameters
    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, FILENAME_PREFIX, FILENAME_PREFIX_LEN)) {
            file_name = &(param[FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_LENGTH_PREFIX, CONTENT_LENGTH_PREFIX_LEN)) {
            content_length = convertInt(&(param[CONTENT_LENGTH_PREFIX_LEN]), "content_length");
        }
        else if (!strncmp(param, WEBSITE_REDIRECT_LOCATION_PREFIX, WEBSITE_REDIRECT_LOCATION_PREFIX_LEN)) {
            put_properties.website_redirect_location = &(param[WEBSITE_REDIRECT_LOCATION_PREFIX_LEN]);
        }
        else if (!strncmp(param, BODY_PREFIX, BODY_PREFIX_LEN)) {
            body = &(param[BODY_PREFIX_LEN]);
        }
        else if (!strncmp(param, CACHE_CONTROL_PREFIX, CACHE_CONTROL_PREFIX_LEN)) {
            put_properties.cache_control = &(param[CACHE_CONTROL_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_TYPE_PREFIX, CONTENT_TYPE_PREFIX_LEN)) {
            put_properties.content_type = &(param[CONTENT_TYPE_PREFIX_LEN]);
        }
        else if (!strncmp(param, MD5_PREFIX, MD5_PREFIX_LEN)) {
            put_properties.md5 = &(param[MD5_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_DISPOSITION_FILENAME_PREFIX, CONTENT_DISPOSITION_FILENAME_PREFIX_LEN)) {
            put_properties.md5 = &(param[CONTENT_DISPOSITION_FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_ENCODING_PREFIX, CONTENT_ENCODING_PREFIX_LEN)) {
            put_properties.content_encoding = &(param[CONTENT_ENCODING_PREFIX_LEN]);
        }
        else if (!strncmp(param, EXPIRES_PREFIX, EXPIRES_PREFIX_LEN)) {
            put_properties.expires = parseIso8601Time(&(param[EXPIRES_PREFIX_LEN]));
        }
        else if (!strncmp(param, START_BYTE_PREFIX, START_BYTE_PREFIX_LEN)) {
            put_properties.start_byte = convertInt(&(param[START_BYTE_PREFIX_LEN]), "start_byte");
        }
        else if (!strncmp(param, X_AMZ_META_PREFIX, X_AMZ_META_PREFIX_LEN)) {
            char *name = &(param[X_AMZ_META_PREFIX_LEN]);
            char *value = name;
            while (*value && (*value != '=')) {
                value ++;
            }
            *value ++ =0;
            
            put_properties.meta_data[put_properties.meta_data_count].name = name;
            put_properties.meta_data[put_properties.meta_data_count].value = value;
            put_properties.meta_data_count++;
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            put_properties.canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN)) {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        //tmp auth
        else if (!strncmp(param, TMP_AUTH_EXPIRES_PREFIX,TMP_AUTH_EXPIRES_PREFIX_LEN)){
            tempauth.callback_data = (void *)(&ptrResult);
            int auth_expire = atoi(&param[TMP_AUTH_EXPIRES_PREFIX_LEN]);
            tempauth.expires = auth_expire;
            tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
            option.temp_auth = &tempauth;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    //read from local file to buffer
    put_object_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(put_object_callback_data));

    data.infile = 0;
    data.gb = 0;
    data.noStatus = 1;

    if (file_name) {
        if (!content_length) {
            content_length =  read_bytes_from_file(file_name, &data);
        }
        //Open the file
        if (!(data.infile = fopen(file_name, "rb" FOPEN_EXTRA_FLAGS))) {
            fprintf(stderr, "\nERROR: Failed to open input file %s: ", file_name);
            return;
        }
    }
    else 
    {
        if (!content_length) {
            while (1) {
                int amtRead = strlen(body);
                if (amtRead == 0) {
                    break;
                }
                growbuffer_append(&(data.gb), body, amtRead);
                content_length += amtRead;
                if (amtRead <= (int) (strlen(body))) {
                    break;
                }
            }
        }
        else {
            growbuffer_append(&(data.gb), body, content_length);
        }
    }

    data.content_length = data.originalContentLength = content_length;
    
    obs_put_object_handler putobjectHandler =
    { 
        { &responsePropertiesCallback,
          &put_object_complete_callback},
          &put_object_data_callback,
          &progress_callback
    };
    
    put_object(&option,key,content_length,&put_properties,0,&putobjectHandler,&data);

    if (OBS_STATUS_OK == data.put_status) {
        printf("put object [%s,%s] successfully. \n", bucket_name,key);
    }
    else
    {
        printf("put object [%s,%s] faied(%s).\n", bucket_name,key,
            obs_get_status_name(data.put_status));
    }
}



// get object ---------------------------------------------------------------
void set_image_proc(char *strImageProcConfig, obs_get_conditions *get_conditions)
{
    image_process_mode imageProcMode = 0;
    char *strCmds_StyleName = strchr(strImageProcConfig, '/');
    int stringLen = 0;
    if ( (strCmds_StyleName != NULL) && (strlen(strImageProcConfig) >= strlen("style"))
            && (strlen(strImageProcConfig) >= strlen("image")))
    {
        strCmds_StyleName ++;
        get_conditions->image_process_config = (image_process_configure *)malloc(sizeof(image_process_configure));
        memset_s(get_conditions->image_process_config,sizeof(get_conditions->image_process_config),0,sizeof(image_process_configure));
        get_conditions->image_process_config->cmds_stylename = strCmds_StyleName;
        
        if (strncmp(strImageProcConfig, "style", strlen("style")) == 0) {
            get_conditions->image_process_config->image_process_mode = obs_image_process_style;
        }
        else if (strncmp(strImageProcConfig, "image", strlen("image")) == 0) {
            get_conditions->image_process_config->image_process_mode = obs_image_process_cmd;
        }
        else 
        {
            if (get_conditions->image_process_config)
            {
                free(get_conditions->image_process_config);
                get_conditions->image_process_config = NULL;
            }
            fprintf(stderr, "\nERROR: image process para is not valid \n,config it like '%s' or '%s'", "image_process_config=image/commands", "image_process_config=style/stylename");
        }
    }

    return ;
}

obs_get_conditions* malloc_init_condition()
{
    obs_get_conditions *get_conditions = (obs_get_conditions *)malloc(sizeof(obs_get_conditions));
    memset_s(get_conditions, sizeof(get_conditions),0, sizeof(obs_get_conditions));
    init_get_properties(get_conditions); 
    return get_conditions;
}

void test_get_object_new(int argc, char **argv, int optindex)
{
    
    char *bucket_name   = argv[optindex++];
    char *key           = argv[optindex++];
    char *file_name     = 0;
    obs_object_info object_info;
    obs_options option;
    obs_get_conditions *get_conditions = NULL;
    temp_auth_configure tempauth;
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));
    
    memset_s(&object_info, sizeof(object_info),0, sizeof(obs_object_info));
    object_info.key = key;
    printf("Bucket's name is == %s, object's name is == %s. \n", bucket_name, key);

    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, FILENAME_PREFIX, FILENAME_PREFIX_LEN)) {
            file_name = &(param[FILENAME_PREFIX_LEN]);
            printf("file_name is: %s\n", file_name);
        }
        else if (!strncmp(param, IF_MODIFIED_SINCE_PREFIX, IF_MODIFIED_SINCE_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->if_modified_since = parseIso8601Time(&(param[IF_MODIFIED_SINCE_PREFIX_LEN]));
        }
        else if (!strncmp(param, IF_NOT_MODIFIED_SINCE_PREFIX, IF_NOT_MODIFIED_SINCE_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->if_not_modified_since = parseIso8601Time(&(param[IF_NOT_MODIFIED_SINCE_PREFIX_LEN]));
        }
        else if (!strncmp(param, IF_MATCH_PREFIX, IF_MATCH_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->if_match_etag = &(param[IF_MATCH_PREFIX_LEN]);
        }
        else if (!strncmp(param, IF_NOT_MATCH_PREFIX, IF_NOT_MATCH_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->if_not_match_etag = &(param[IF_NOT_MATCH_PREFIX_LEN]);
        }
        else if (!strncmp(param, START_BYTE_PREFIX, START_BYTE_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->start_byte = convertInt(&(param[START_BYTE_PREFIX_LEN]), "start_byte");
        }
        else if (!strncmp(param, BYTE_COUNT_PREFIX, BYTE_COUNT_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->byte_count = convertInt(&(param[BYTE_COUNT_PREFIX_LEN]), "byte_count");
        }
        else if (!strncmp(param, VERSIONID_PREFIX, VERSIONID_PREFIX_LEN)) {
            object_info.version_id = &(param[VERSIONID_PREFIX_LEN]);
        }
        else if (!strncmp(param, IMAGE_PROCESS, IMAGE_PROCESS_LEN)) {
            set_image_proc(&(param[IMAGE_PROCESS_LEN]), get_conditions);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {  
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS; 
        }
        //tmp auth
        else if (!strncmp(param, TMP_AUTH_EXPIRES_PREFIX,TMP_AUTH_EXPIRES_PREFIX_LEN)){
            tempauth.callback_data = (void *)(&ptrResult);
            int auth_expire = atoi(&param[TMP_AUTH_EXPIRES_PREFIX_LEN]);
            tempauth.expires = auth_expire;
            tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
            option.temp_auth = &tempauth;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    if (get_conditions)
    {
        get_conditions->download_limit = 819200;
    }

    get_object_callback_data callback_data = {0};
    callback_data.outfile = write_to_file(file_name);
    obs_get_object_handler getobjectHandler =
    { 
        { &responsePropertiesCallback,
          &get_object_complete_callback },
        &get_object_data_callback
    };
    
    get_object(&option, &object_info, get_conditions, 0, &getobjectHandler, &callback_data);  
    if (OBS_STATUS_OK == callback_data.ret_status) {
        printf("get object successfully. \n");
    }
    else
    {
        printf("get object faied(%s).\n", obs_get_status_name(callback_data.ret_status));
    }
    
    fclose(callback_data.outfile);
    if(get_conditions)
    { 
        free(get_conditions->image_process_config);
        free(get_conditions);
    }
}


// delete object ---------------------------------------------------------------
static void test_delete_object_new(int argc, char **argv, int optindex)
{ 
    obs_options option; 
    obs_object_info object_info;
    obs_status  ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];

    init_obs_options(&option); 
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
 
    memset_s(&object_info,sizeof(object_info), 0, sizeof(obs_object_info));
    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, KEY_PREFIX, KEY_PREFIX_LEN)) {
            object_info.key = &(param[KEY_PREFIX_LEN]);
        }
        else if (!strncmp(param, VERSIONID_PREFIX, VERSIONID_PREFIX_LEN)) {
            object_info.version_id = &(param[VERSIONID_PREFIX_LEN]);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) { 
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;            
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    obs_response_handler resqonseHandler =
    { 
        NULL,
        &response_complete_callback 
    };
    
    delete_object(&option, &object_info, &resqonseHandler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("delete object [%s] successfully. \n", object_info.key);
    }
    else
    {
        printf("delete object [%s] faied(%s).\n", object_info.key, 
            obs_get_status_name(ret_status));
    }
}

// list object ---------------------------------------------------------------
static void test_list_bucket_object_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);
    obs_options option;
    
    init_obs_options(&option); 
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    char *prefix = 0, *marker = 0, *delimiter = 0;
    int maxkeys = 0, allDetails = 0;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, PREFIX_PREFIX, PREFIX_PREFIX_LEN)) {
            prefix = &(param[PREFIX_PREFIX_LEN]);
        }
        else if (!strncmp(param, MARKER_PREFIX, MARKER_PREFIX_LEN)) {
            marker = &(param[MARKER_PREFIX_LEN]);
        }
        else if (!strncmp(param, MAXKEYS_PREFIX, MAXKEYS_PREFIX_LEN)) {
            maxkeys = convertInt(&(param[MAXKEYS_PREFIX_LEN]), "maxkeys");
        }
        else if (!strncmp(param, DELIMITER_PREFIX, DELIMITER_PREFIX_LEN)) {
            delimiter = &(param[DELIMITER_PREFIX_LEN]);
        }
        else if (!strncmp(param, ALL_DETAILS_PREFIX, ALL_DETAILS_PREFIX_LEN)) {
            const char *ad = &(param[ALL_DETAILS_PREFIX_LEN]);
            if (!strcmp(ad, "true") || !strcmp(ad, "TRUE") ||
            !strcmp(ad, "yes") || !strcmp(ad, "YES") || !strcmp(ad, "1")) {
                allDetails = 1;
            }
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = &(param[CERTIFICATE_INFO_PREFIX_LEN]);
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    
    list_object_callback_data data;
    memset_s(&data,sizeof(data) ,0, sizeof(list_object_callback_data));
    snprintf_s(data.next_marker, sizeof(data.next_marker),
        sizeof(data.next_marker)-1 , "%s", marker);
    data.keyCount = 0;
    data.allDetails = allDetails;
    data.is_truncated = 0;

    obs_list_objects_handler list_bucket_objects_handler =
    { 
        { NULL, &list_object_complete_callback},
        &list_objects_callback
    };

    
    list_bucket_objects(&option, prefix, data.next_marker,delimiter, maxkeys, &list_bucket_objects_handler, &data);

    if (OBS_STATUS_OK == data.ret_status) {
        printf("list object successfully. \n");
    }
    else
    {
        printf("list object faied(%s).\n", obs_get_status_name(data.ret_status));
    }
}


/*******************test_list_bucket***********************************************/
static obs_bucket_list_type get_list_type_from_argv(char *param)
{
    obs_bucket_list_type ret_list_type = OBS_BUCKET_LIST_ALL;
    char *val = param;
    printf("list_type is: %s\n", val);
    if (!strcmp(val, "object")) {
        ret_list_type = OBS_BUCKET_LIST_OBJECT;
    }else if (!strcmp(val, "pfs")) {
        ret_list_type = OBS_BUCKET_LIST_PFS;
    }else {
        fprintf(stderr, "ERROR: Unknown list_type: %s.\n", val);
    }

    return ret_list_type;
}

void test_list_bucket_new_s3(int argc, char **argv, int optindex)
{
    obs_options option;
    init_obs_options(&option); 
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = &(param[CERTIFICATE_INFO_PREFIX_LEN]);
        }
        else if (!strncmp(param, BUCKET_LIST_TYPE, BUCKET_LIST_TYPE_LEN)) {
            option.bucket_options.bucket_list_type = get_list_type_from_argv(&(param[BUCKET_LIST_TYPE_LEN]));
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    
    list_service_data data;
    memset_s(&data, sizeof(data),0, sizeof(list_service_data));
    
    obs_list_service_handler listHandler =
    { 
        {NULL, &list_bucket_complete_callback },
        &listServiceCallback
    };
    
    list_bucket(&option,&listHandler,&data);
    if (data.ret_status == OBS_STATUS_OK) 
    {
        printf("list bucket successfully. \n");
    }
    else
    {
        printf("list bucket failed(%s).\n", obs_get_status_name(data.ret_status));
    }
}

void test_list_bucket_new_obs(int argc, char **argv, int optindex)
{
    obs_options option;
    init_obs_options(&option); 
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = &(param[CERTIFICATE_INFO_PREFIX_LEN]);
        }
        else if (!strncmp(param, BUCKET_LIST_TYPE, BUCKET_LIST_TYPE_LEN)) {
            option.bucket_options.bucket_list_type = get_list_type_from_argv(&(param[BUCKET_LIST_TYPE_LEN]));
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    
    list_service_data data;
    memset_s(&data,sizeof(data) ,0, sizeof(list_service_data));
    
    obs_list_service_obs_handler listHandler =
    { 
        {NULL, &list_bucket_complete_callback },
        &listServiceObsCallback
    };
    
    list_bucket_obs(&option,&listHandler,&data);
    if (data.ret_status == OBS_STATUS_OK) 
    {
        printf("list bucket successfully. \n");
    }
    else
    {
        printf("list bucket failed(%s).\n", obs_get_status_name(data.ret_status));
    }
}

void test_list_bucket_new(int argc, char **argv, int optindex)
{
    //if(demoUseObsApi == OBS_USE_API_S3) {
    //    test_list_bucket_new_s3(argc, argv, optindex);
    //} else {
    //    test_list_bucket_new_obs(argc, argv, optindex);
    //}
    
    obs_options option;
    init_obs_options(&option); 
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = &(param[CERTIFICATE_INFO_PREFIX_LEN]);
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else if (!strncmp(param, BUCKET_LIST_TYPE, BUCKET_LIST_TYPE_LEN)) {
            option.bucket_options.bucket_list_type = get_list_type_from_argv(&(param[BUCKET_LIST_TYPE_LEN]));
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    
    list_service_data data;
    memset_s(&data, sizeof(data),0, sizeof(list_service_data));
    
    
    if (option.request_options.auth_switch == OBS_S3_TYPE)
    {
        obs_list_service_handler listHandler =
        { 
            {NULL, &list_bucket_complete_callback },
            &listServiceCallback
        };
    
        list_bucket(&option,&listHandler,&data);
    }
    else
    {
        obs_list_service_obs_handler listHandler =
        { 
            {NULL, &list_bucket_complete_callback },
            &listServiceObsCallback
        };
        
        list_bucket_obs(&option,&listHandler,&data);
    }
    
    if (data.ret_status == OBS_STATUS_OK) 
    {
        printf("list bucket successfully. \n");
    }
    else
    {
        printf("list bucket failed(%s).\n", obs_get_status_name(data.ret_status));
    }
}

/******************test_init_upload_part_new*****************************************/
static void test_init_upload_part_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    obs_status ret_status = OBS_STATUS_OK;
    obs_options option;
    
    printf("Bucket's name is == %s, object's name is == %s. \n", bucket_name, key);
    obs_put_properties put_properties;
    init_put_properties(&put_properties);

    init_obs_options(&option); 
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    char upload_id_return[256] = {0};
    obs_response_handler Handler =
    { 
        NULL, &response_complete_callback 
    };
    
    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, CACHE_CONTROL_PREFIX, CACHE_CONTROL_PREFIX_LEN)) {
            put_properties.cache_control = &(param[CACHE_CONTROL_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_TYPE_PREFIX, CONTENT_TYPE_PREFIX_LEN)) {
            put_properties.content_type = &(param[CONTENT_TYPE_PREFIX_LEN]);
        }
        else if (!strncmp(param, MD5_PREFIX, MD5_PREFIX_LEN)) {
            put_properties.md5 = &(param[MD5_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_DISPOSITION_FILENAME_PREFIX, CONTENT_DISPOSITION_FILENAME_PREFIX_LEN)) {
            put_properties.content_disposition_filename = &(param[CONTENT_DISPOSITION_FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_ENCODING_PREFIX, CONTENT_ENCODING_PREFIX_LEN)) {
            put_properties.content_encoding = &(param[CONTENT_ENCODING_PREFIX_LEN]);
        }
        else if (!strncmp(param, WEBSITE_REDIRECT_LOCATION_PREFIX, WEBSITE_REDIRECT_LOCATION_PREFIX_LEN)) {
            put_properties.website_redirect_location = &(param[WEBSITE_REDIRECT_LOCATION_PREFIX_LEN]);
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            put_properties.canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN)) {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = &(param[CERTIFICATE_INFO_PREFIX_LEN]);
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    initiate_multi_part_upload(&option, key, 100, upload_id_return, &put_properties, 0, 
                &Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("upload_id= %s \n",upload_id_return);
        printf("test init upload part successfully. \n");
    }
    else
    {
        printf("test init upload part faied(%s).\n", obs_get_status_name(ret_status));
    }
}
    
/*****************************list versions********************************************/
static void test_list_versions_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);
    obs_options option;
    
    init_obs_options(&option); 
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    char *prefix = 0, *key_marker = 0, *delimiter = 0, *version_id_marker = NULL;
    int maxkeys = 0;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, PREFIX_PREFIX, PREFIX_PREFIX_LEN)) {
            prefix = &(param[PREFIX_PREFIX_LEN]);
        }
        else if (!strncmp(param, KEY_MARKER_PREFIX, KEY_MARKER_PREFIX_LEN)) {
            key_marker = &(param[KEY_MARKER_PREFIX_LEN]);
        }
        else if (!strncmp(param, MAXKEYS_PREFIX, MAXKEYS_PREFIX_LEN)) {
            maxkeys = convertInt(&(param[MAXKEYS_PREFIX_LEN]), "maxkeys");
        }
        else if (!strncmp(param, DELIMITER_PREFIX, DELIMITER_PREFIX_LEN)) {
            delimiter = &(param[DELIMITER_PREFIX_LEN]);
        }
        else if (!strncmp(param, VERSIONIDMARKER_PREFIX, VERSIONIDMARKER_PREFIX_LEN)) {
            version_id_marker = &(param[VERSIONIDMARKER_PREFIX_LEN]);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = &(param[CERTIFICATE_INFO_PREFIX_LEN]);
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    
    obs_list_versions_handler list_versions_handler =
    {
        { &response_properties_callback, &list_versions_complete_callback },
        &listVersionsCallback
    };

    list_versions_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(list_versions_callback_data)); 

    list_versions(&option, prefix, key_marker, delimiter, maxkeys, version_id_marker,
                 &list_versions_handler, &data); 
    if (OBS_STATUS_OK == data.ret_status) {
        printf("list versions successfully. \n");
    }
    else
    {
        printf("list versions failed(%s).\n", obs_get_status_name(data.ret_status));
    }
}

// set bucket version ------------------------------------------------------
static void test_set_bucket_version(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    set_bucket_version_configuration(&option, OBS_VERSION_STATUS_ENABLED, 
            &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket version successfully. \n");
    }
    else
    {
        printf("set bucket version failed(%s).\n", obs_get_status_name(ret_status));
    }

}
void test_set_bucket_policy(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];

    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    char *policy = 0;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, POLICY_PREFIX, POLICY_PREFIX_LEN)) {
            policy = &(param[POLICY_PREFIX_LEN]);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    obs_response_handler response_handler =
    { 
        &response_properties_callback, &response_complete_callback
    };

    set_bucket_policy(&option, policy, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket policy successfully. \n");
    }
    else
    {
        printf("set bucket policy failed(%s).\n", obs_get_status_name(ret_status));
    }

}

void test_get_bucket_policy(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }


    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    char policy[1024]="";
    get_bucket_policy(&option, sizeof(policy), policy, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("policy=(%s)\nget bucket policy successfully.\n", policy);
    }
    else
    {
        printf("get bucket policy failed(%s).\n", obs_get_status_name(ret_status));
    }

}

//delete bucket policy
void test_delete_bucket_policy(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    delete_bucket_policy(&option, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("delete bucket policy successfully.\n");
    }
    else
    {
        printf("delete bucket policy failed(%s).\n", obs_get_status_name(ret_status));
    }

}

static obs_storage_class get_storage_class(char *val)
{
    obs_storage_class ret_storage_class = OBS_STORAGE_CLASS_STANDARD;
    printf("storage_class is: %s\n", val);
    if (!strcmp(val, "standard")) {
        ret_storage_class = OBS_STORAGE_CLASS_STANDARD;
    }else if (!strcmp(val, "standard_ia")) {
        ret_storage_class = OBS_STORAGE_CLASS_STANDARD_IA;
    }else if (!strcmp(val, "glacier")) {
        ret_storage_class = OBS_STORAGE_CLASS_GLACIER;
    }else {
        fprintf(stderr, "ERROR: Unknown storage class: %s.\n", val);
    }

    return ret_storage_class;
}

char* get_lifecycle_para(char* prev_para, char *prefix, int prefix_len)
{
   char *next_para = prev_para;
   while(next_para && *next_para != '\0')
    {
        if(!strncmp(next_para, prefix, prefix_len))
        {
            *(next_para - 1) = '\0';
            break;
        }
        next_para++;
    }
    return next_para;
}

/*
command:  ./object_test set_bucket_lifecycle bucket-name 1 
rule=id=test3+prefix=/dadf/dfadwerw/werw/safd/+expdays=70+status=Enabled+noncurrent_days=60+cur_transition=0-50-standard_ia+noncurrent_transition=40-standard_ia

can lack: expdays, noncurrent_days, cur_transition, noncurrent_transition
*/
static void test_set_bucket_lifecycle_new(int argc, char **argv, int optindex)
{
    char *file_name = 0;
    uint64_t content_length = 0;
    obs_options option;
    char *bucket_name = argv[optindex++];
    char *body = 0;
    char* noncurrent_version_transition = NULL;
    char *transition = NULL;
    char *temp = NULL;

    printf("Bucket's name is == %s. \n", bucket_name);
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_lifecycle_conf bucket_lifecycle_conf[10]={0};
    int i = 0;
    int j = 0;
    char *list[10] = {0};

    int rule_number = atoi(argv[optindex++]);
    for(i = 0; i< rule_number; i++)
    {
        char *rule = argv[optindex ++];
        printf("rule:%s \n", rule);
        char *id = get_lifecycle_para(rule,RULE_ID_PREFIX,RULE_ID_PREFIX_LEN);
        bucket_lifecycle_conf[i].id = &id[RULE_ID_PREFIX_LEN];   
        
        char *prefix = get_lifecycle_para(id,RULE_PREFIX,RULE_PREFIX_LEN);
        printf("id:%s \n ", bucket_lifecycle_conf[i].id);
        bucket_lifecycle_conf[i].prefix = &prefix[RULE_PREFIX_LEN];
        
        char *days = get_lifecycle_para(prefix,RULE_DAYS_PREFIX,RULE_DAYS_PREFIX_LEN);
        if(*days != '\0')
        {
            bucket_lifecycle_conf[i].days= &days[RULE_DAYS_PREFIX_LEN];   
        }
        else
        {
            days = prefix;
        }
        
        char *date = get_lifecycle_para(days,RULE_DATE_PREFIX,RULE_DATE_PREFIX_LEN);
        if (*date != '\0')
        {
            bucket_lifecycle_conf[i].date = &date[RULE_DATE_PREFIX_LEN];
        }
        else
        {
            date = days;
        }
        
        char *status = get_lifecycle_para(date,RULE_STATUS_PREFIX,RULE_STATUS_PREFIX_LEN);
        bucket_lifecycle_conf[i].status= &status[RULE_STATUS_PREFIX_LEN];
        
        char *noncurrent_days = get_lifecycle_para(status,RULE_NONCURRNET_DAY_PREFIX,RULE_NONCURRNET_DAY_PREFIX_LEN);
        if (*noncurrent_days != '\0')
        {
            bucket_lifecycle_conf[i].noncurrent_version_days = &noncurrent_days[RULE_NONCURRNET_DAY_PREFIX_LEN];
        }
        else
        {
            noncurrent_days = status;
        }
        
        transition = get_lifecycle_para(noncurrent_days,RULE_TRANSITION_PREFIX,RULE_TRANSITION_PREFIX_LEN);
        
        printf("tran:%s \n", transition);
        if (*transition != '\0')
        { 
            noncurrent_version_transition = get_lifecycle_para(transition,RULE_NONCURRENT_TRANSITION_PREFIX,RULE_NONCURRENT_TRANSITION_PREFIX_LEN);
            bucket_lifecycle_conf[i].transition = (obs_lifecycle_transtion *)malloc(sizeof(obs_lifecycle_transtion)*10);
            memset_s(bucket_lifecycle_conf[i].transition,sizeof(bucket_lifecycle_conf[i].transition),0,sizeof(obs_lifecycle_transtion));
            transition = &transition[RULE_TRANSITION_PREFIX_LEN];
            char *temp_transition = strtok(transition,",");
            j = 0;
            while(temp_transition)
            {
                 list[j++] = temp_transition;
                 temp_transition= strtok(NULL,",");
                 
            }
            bucket_lifecycle_conf[i].transition_num = j;
            for(j =0; j<bucket_lifecycle_conf[i].transition_num; j++)
            {
                temp = strtok(list[j],"-");
                bucket_lifecycle_conf[i].transition[j].date = strcmp(temp, "0") == 0 ? NULL:temp;
                temp = strtok(NULL,"-");
                bucket_lifecycle_conf[i].transition[j].days = strcmp(temp, "0") == 0 ? NULL:temp;
                temp = strtok(NULL,"-");
                bucket_lifecycle_conf[i].transition[j].storage_class =  get_storage_class(temp);
                printf("transition, date:%s,days:%s,storage:%d \n",bucket_lifecycle_conf[i].transition[j].date, 
                    bucket_lifecycle_conf[i].transition[j].days, bucket_lifecycle_conf[i].transition[j].storage_class);
            }
        }
        else
        {
            transition = noncurrent_days;
            noncurrent_version_transition = get_lifecycle_para(transition,RULE_NONCURRENT_TRANSITION_PREFIX,RULE_NONCURRENT_TRANSITION_PREFIX_LEN);
        }

        printf("non:%s \n", noncurrent_version_transition);
        
        if (noncurrent_version_transition != NULL && *noncurrent_version_transition != '\0')
        {
            j = 0;
            noncurrent_version_transition = &noncurrent_version_transition[RULE_NONCURRENT_TRANSITION_PREFIX_LEN];
            
            bucket_lifecycle_conf[i].noncurrent_version_transition = (obs_lifecycle_noncurrent_transtion *)malloc(sizeof(obs_lifecycle_noncurrent_transtion)*10 );
            memset_s(bucket_lifecycle_conf[i].noncurrent_version_transition,sizeof(bucket_lifecycle_conf[i].noncurrent_version_transition),0,sizeof(obs_lifecycle_noncurrent_transtion));
 
            char *temp_noncurrent_transition = strtok(noncurrent_version_transition,",");
            while(temp_noncurrent_transition)
            {
                list[j++] = temp_noncurrent_transition;
                temp_noncurrent_transition= strtok(NULL,",");
            }
            bucket_lifecycle_conf[i].noncurrent_version_transition_num = j; 

            for(j=0; j<bucket_lifecycle_conf[i].noncurrent_version_transition_num; j++)
            {
                bucket_lifecycle_conf[i].noncurrent_version_transition[j].noncurrent_version_days = strtok(list[j],"-");
                temp = strtok(NULL,"-");
                bucket_lifecycle_conf[i].noncurrent_version_transition[j].storage_class = get_storage_class(temp); 
                 
                printf("noncur_transition,days:%s,storage:%d \n",bucket_lifecycle_conf[i].noncurrent_version_transition[j].noncurrent_version_days, 
                    bucket_lifecycle_conf[i].noncurrent_version_transition[j].storage_class);
            }  
        }
        
    }
 
    obs_status ret_status = OBS_STATUS_OK;
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    
    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    
    set_bucket_lifecycle_configuration(&option, bucket_lifecycle_conf, rule_number, 
        &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket lifcycle [%s] successfully. \n", bucket_name);
    }
    else
    {
        printf("set bucket lifcycle [%s] faied(%s).\n", bucket_name,
            obs_get_status_name(ret_status));
    }
    
    for(i = 0; i< rule_number; i++)
    {
        if(bucket_lifecycle_conf[i].noncurrent_version_transition)
        {
            free(bucket_lifecycle_conf[i].noncurrent_version_transition);
        }
        if(bucket_lifecycle_conf[i].transition)
        {
            free(bucket_lifecycle_conf[i].transition);
        }
    }
}

static void test_get_lifecycle_config_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option; 
    
    printf("Bucket's name is == %s. \n", bucket_name);
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    // 设置回调函数
    obs_lifecycle_handler lifeCycleHandlerEx =
    {
        {&response_properties_callback, &response_complete_callback},
        &getBucketLifecycleConfigurationCallbackEx
    };
    
    get_bucket_lifecycle_configuration(&option, &lifeCycleHandlerEx, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("get_lifecycle_config successfully.\n");
    }
    else
    {
        printf("get_lifecycle_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}

static void test_delete_lifecycle_config_new(int argc, char **argv, int optindex)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);

    // 设置option
    init_obs_options(&option);    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    delete_bucket_lifecycle_configuration(&option, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("test_delete_lifecycle_config successfully.\n");
    }
    else
    {
        printf("test_delete_lifecycle_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}

static void test_set_bucket_version_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    char *bucket_name = argv[optindex++];
    char *status = argv[optindex++];
    printf("Bucket's name is == %s,status(%s). \n", bucket_name,status);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    set_bucket_version_configuration(&option, status, 
        &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket version successfully. \n");
    }
    else
    {
        printf("set bucket version failed(%s).\n", obs_get_status_name(ret_status));
    }

}

void test_get_bucket_version_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    printf("Bucket's name is == %s. \n", bucket_name);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    char status[OBS_COMMON_LEN_256] = {0};
    get_bucket_version_configuration(&option, sizeof(status), status, 
        &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("get bucket version successfully.\n status=(%s)\n", status);
    }
    else
    {
        printf("get bucket version failed(%s).\n", obs_get_status_name(ret_status));
    }

}
void test_get_bucket_storage_info_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);
    obs_status ret_status = OBS_STATUS_BUTT;
    
    obs_options option; 
    char capacity[OBS_COMMON_LEN_256] = {0};
    char obj_num[OBS_COMMON_LEN_256] = {0};
    
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        NULL,
        &response_complete_callback
    };
     while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    get_bucket_storage_info(&option, OBS_COMMON_LEN_256, capacity, OBS_COMMON_LEN_256, obj_num, 
        &response_handler, &ret_status);     
    if (ret_status == OBS_STATUS_OK) {
         printf("get_bucket_storage_info success,bucket=%s objNum=%s capacity=%s\n", 
            bucket_name, obj_num, capacity);
    }
    else
    {
       printf("head bucket failed(%s).\n", obs_get_status_name(ret_status));
    }

}

void test_set_bucket_acl_byhead_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option; 
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    char *acl = argv[optindex++];
    obs_canned_acl canned_acl = OBS_CANNED_ACL_PRIVATE;
    if (!strncmp(acl, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
        canned_acl = get_acl_from_argv(acl);
    }
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    set_bucket_acl_by_head(&option, canned_acl, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket acl by head successfully. \n");
    }
    else
    {
        printf("set bucket acl by head failed(%s).\n", obs_get_status_name(ret_status));
    }
}

manager_acl_info* malloc_acl_info()
{
    manager_acl_info *aclinfo = (manager_acl_info*)malloc(sizeof(manager_acl_info));
    memset_s(aclinfo, sizeof(manager_acl_info), 0, sizeof(manager_acl_info));
    
    aclinfo->acl_grants = (obs_acl_grant*)malloc(sizeof(obs_acl_grant)*100);
    memset_s(aclinfo->acl_grants, sizeof(obs_acl_grant)*100, 0, sizeof(obs_acl_grant)*100);
    aclinfo->acl_grant_count_return = (int*)malloc(sizeof(int));
    *(aclinfo->acl_grant_count_return) = 100;

    aclinfo->owner_id = (char *)malloc(100);
    memset_s(aclinfo->owner_id,100,0,100);
    aclinfo->owner_display_name = (char *)malloc(100);
    memset_s(aclinfo->owner_display_name,100,0,100);
    return aclinfo;
}

void free_acl_info(manager_acl_info **acl)
{
    manager_acl_info *aclinfo = *acl;
    free(aclinfo->acl_grants);
    free(aclinfo->owner_display_name);
    free(aclinfo->owner_id);
    free(aclinfo->acl_grant_count_return);
    free(aclinfo);
}


void test_get_bucket_acl_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    manager_acl_info *aclinfo = malloc_acl_info();
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    get_bucket_acl(&option, aclinfo, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status)
    {
        printf("get bucket acl successfully: -------------");
        printf("%s %s\n", aclinfo->owner_id, aclinfo->owner_display_name);
        if (aclinfo->acl_grant_count_return)
        {
            print_grant_info(*aclinfo->acl_grant_count_return, aclinfo->acl_grants);
        }
    }
    else
    {
        printf("get bucket acl failed(%s).\n", obs_get_status_name(ret_status));
    }

    free_acl_info(&aclinfo);
}

void test_set_object_acl_byhead_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option; 
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    char *version_id = argv[optindex++];
    if (!strcmp(version_id,"0"))
    {
        version_id = NULL;
    }
    printf("Bucket's name is == %s. key=%s, version_id = %s.\n", bucket_name,key,version_id);
       
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    char *acl = argv[optindex++];
    obs_canned_acl canned_acl = OBS_CANNED_ACL_PRIVATE;
    if (!strncmp(acl, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
        canned_acl = get_acl_from_argv(acl);
    }
    obs_object_info object_info;
    object_info.key = key;
    object_info.version_id = version_id;
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    set_object_acl_by_head(&option, &object_info, canned_acl, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket acl by head successfully. \n");
    }
    else
    {
        printf("set bucket acl by head failed(%s).\n", obs_get_status_name(ret_status));
    }
}


void test_get_object_acl_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);
    char *key = argv[optindex++];
    char *version_id = argv[optindex++];
    if (!strcmp(version_id,"0"))
    {
        version_id = NULL;
    }
    printf("Bucket's name is == %s. key=%s, version_id = %s.\n", bucket_name,key,version_id);
    
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    
    manager_acl_info *aclinfo = malloc_acl_info();
    aclinfo->object_info.key = key;
    aclinfo->object_info.version_id = version_id;
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    get_object_acl(&option, aclinfo, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status)
    {
        printf("get object acl successfully: -------------");
        printf("%s %s %s %s object delivered is %d\n", aclinfo->owner_id, aclinfo->owner_display_name,
            aclinfo->object_info.key, aclinfo->object_info.version_id, aclinfo->object_delivered);
        if (aclinfo->acl_grant_count_return)
        {
            print_grant_info(*aclinfo->acl_grant_count_return, aclinfo->acl_grants);
        }
    }
    else
    {
        printf("get object acl failed(%s).\n", obs_get_status_name(ret_status));
    }

    free_acl_info(&aclinfo);
}

void init_acl_info(manager_acl_info *aclinfo)
{
    memset_s(aclinfo, sizeof(manager_acl_info), 0, sizeof(manager_acl_info));

    aclinfo->acl_grants = (obs_acl_grant*)malloc(sizeof(obs_acl_grant)*2);
    memset_s( aclinfo->acl_grants, 2 * sizeof(obs_acl_grant), 0, 2 *sizeof(obs_acl_grant));
    strcpy_s(aclinfo->acl_grants->grantee.canonical_user.id,sizeof(aclinfo->acl_grants->grantee.canonical_user.id), "userid1"); 
    strcpy_s(aclinfo->acl_grants->grantee.canonical_user.display_name, sizeof(aclinfo->acl_grants->grantee.canonical_user.display_name),"name1"); 
    aclinfo->acl_grants->grantee_type = OBS_GRANTEE_TYPE_LOG_DELIVERY;
    aclinfo->acl_grants->permission = OBS_PERMISSION_WRITE ; 
    aclinfo->acl_grants->bucket_delivered = BUCKET_DELIVERED_FALSE;

    strcpy_s((aclinfo->acl_grants + 1)->grantee.canonical_user.id,sizeof((aclinfo->acl_grants + 1)->grantee.canonical_user.id), "userid1");
    strcpy_s((aclinfo->acl_grants + 1)->grantee.canonical_user.display_name,sizeof((aclinfo->acl_grants + 1)->grantee.canonical_user.display_name), "name1");
    (aclinfo->acl_grants + 1)->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
    (aclinfo->acl_grants + 1)->permission = OBS_PERMISSION_READ_ACP ; 
    (aclinfo->acl_grants + 1)->bucket_delivered = BUCKET_DELIVERED_TRUE; 

    aclinfo->acl_grant_count_return = (int*)malloc(sizeof(int));
    *(aclinfo->acl_grant_count_return) = 2;

    aclinfo->owner_id = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo->owner_id,sizeof(aclinfo->owner_id),0,sizeof(aclinfo->owner_id));
    aclinfo->owner_display_name = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo->owner_display_name,sizeof(aclinfo->owner_display_name),0,sizeof(aclinfo->owner_display_name));
    strcpy_s(aclinfo->owner_id,sizeof(aclinfo->owner_id), "domainiddomainiddomainiddo006000");   
    strcpy_s(aclinfo->owner_display_name,sizeof(aclinfo->owner_display_name), "domainnamedom006000");

    memset_s(&aclinfo->object_info,sizeof(aclinfo->object_info),0,sizeof(aclinfo->object_info));
}

void deinitialize_acl_info(manager_acl_info *aclinfo)
{  
    free(aclinfo->acl_grants);
    free(aclinfo->owner_display_name);
    free(aclinfo->owner_id);
    free(aclinfo->acl_grant_count_return);
}

obs_acl_group* set_grant_acl(int argc, char **argv, int optindex, obs_options* option)
{
    char* delim=",";
    char *count = argv[optindex++];
    int acl_grant_count = convertInt(count, "grant_count");
    obs_acl_group *g = (obs_acl_group *)malloc(sizeof(obs_acl_group));
    memset_s(g,sizeof(g), 0, sizeof(obs_acl_group));
    
    int aclGrantCount = 0;
    obs_acl_grant *acl_grants = (obs_acl_grant*)malloc(sizeof(obs_acl_grant)*10);
    memset_s(acl_grants,sizeof(acl_grants) ,0, sizeof(obs_acl_grant) * 10); 
    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, grantee_type_str, grantee_type_str_length))
        {
            char *grand_type= &param[grantee_type_str_length];
            aclGrantCount = 0;
            char* p=strtok(grand_type,delim);
            while(p!=NULL){
                acl_grants[aclGrantCount++].grantee_type = convertInt(p, "grantee_type");
                p=strtok(NULL,delim);
            }
        }
        else if (!strncmp(param, user_id_str, user_id_str_length))
        {
            char *user_id= &param[user_id_str_length];
            aclGrantCount = 0;
            char* p=strtok(user_id,delim);
            while(p!=NULL){
                strcpy_s(acl_grants[aclGrantCount++].grantee.canonical_user.id, 
                    sizeof(acl_grants[aclGrantCount++].grantee.canonical_user.id),p);
                p=strtok(NULL,delim);
            }
        }
        else if (!strncmp(param, display_name_str, display_name_length))
        {
            char *display_name= &param[display_name_length];
            aclGrantCount = 0;
            char* p=strtok(display_name,delim);
            while(p!=NULL){
                strcpy_s(acl_grants[aclGrantCount++].grantee.canonical_user.display_name, 
                    sizeof(acl_grants[aclGrantCount++].grantee.canonical_user.display_name),p);
                p=strtok(NULL,delim);
            }
        }
        else if (!strncmp(param, permission_str, permission_length))
        {
            char *permission = &param[permission_length];
            aclGrantCount = 0;
            char* p=strtok(permission,delim);
            while(p!=NULL){
                acl_grants[aclGrantCount++].permission = convertInt(p, "permission");
                p=strtok(NULL,delim);
            } 
        } 
        else if (!strncmp(param, delivered_str, delivered_length))
        {
            char *delivered = &param[delivered_length];
            aclGrantCount = 0;
            char* p=strtok(delivered,delim);
            while(p!=NULL){
                acl_grants[aclGrantCount++].bucket_delivered = convertInt(p, "delivered");
                p=strtok(NULL,delim);
            } 
        }
        else if (!strncmp(param, email_address_str, email_address_length))
        {
            char *email_str = &param[email_address_length];
            aclGrantCount = 0;
            char* p=strtok(email_str,delim);
            while(p!=NULL){
                strcpy_s(acl_grants[aclGrantCount++].grantee.huawei_customer_by_email.email_address,
                    sizeof(acl_grants[aclGrantCount++].grantee.huawei_customer_by_email.email_address),p);
                p=strtok(NULL,delim);
            } 
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option->bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option->bucket_options.certificate_info = ca_info;       
            option->bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option->request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option->request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    g->acl_grants = acl_grants;
    g->acl_grant_count = acl_grant_count;

    return g;
}

//test set bucket acl
void test_set_bucket_acl_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    manager_acl_info aclinfo;    
    memset_s(&aclinfo,sizeof(aclinfo) ,0, sizeof(manager_acl_info));
    aclinfo.acl_grant_count_return = (int*)malloc(sizeof(int));
    *(aclinfo.acl_grant_count_return) = 0;

    //1. aclinfo related params
    char *param = argv[optindex ++];
    if (!strncmp(param, owner_id_str, owner_id_str_length))
    {
        aclinfo.owner_id = &param[owner_id_str_length];
    }
    
    param = argv[optindex ++];
    if (!strncmp(param, owner_display_name_str, owner_display_name_length))
    {
        aclinfo.owner_display_name = &param[owner_display_name_length];
    }
    
    obs_acl_group *g = set_grant_acl(argc, argv, optindex, &option);
    *(aclinfo.acl_grant_count_return) = g->acl_grant_count;
    aclinfo.acl_grants = g->acl_grants;
    
    memset_s(&aclinfo.object_info,sizeof(aclinfo.object_info),0,sizeof(aclinfo.object_info));

    set_bucket_acl(&option, &aclinfo, &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket acl successfully. \n");
    }
    else
    {
        printf("set bucket acl failed(%s).\n", obs_get_status_name(ret_status));
    }

    if (aclinfo.acl_grant_count_return)
    {
        free(aclinfo.acl_grant_count_return);
    }
    if (g)
    {
        free(g);
    }
}

void test_set_object_acl_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    char *version_id = argv[optindex++];
    if (!strcmp(version_id,"0"))
    {
        version_id = NULL;
    }
    printf("Bucket's name is == %s. key=%s, version_id = %s.\n", bucket_name,key,version_id);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    manager_acl_info aclinfo;
    init_acl_info(&aclinfo);

    aclinfo.object_info.key = key;
    aclinfo.object_info.version_id = version_id;
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, delivered_str, delivered_length))
        {
            aclinfo.object_delivered = convertInt(&(param[delivered_length]), "delivered");
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }else if (!strncmp(param, owner_id_str, owner_id_str_length))
        {
            aclinfo.owner_id = &param[owner_id_str_length];
        }
    }
    set_object_acl(&option, &aclinfo, &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set object acl successfully. \n");
    }
    else
    {
        printf("set object acl failed(%s).\n", obs_get_status_name(ret_status));
    }
    deinitialize_acl_info(&aclinfo);
}

// set bucket tagging ------------------------------------------------------
static void test_set_bucket_tagging_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    
    obs_options option; 
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };
    
    int num = atoi(argv[optindex++]);

    obs_name_value tagginglist[10] = {0};
    int i=0;
    
    for(;i<num;i++)
    {
         tagginglist[i].name = argv[optindex++];
         tagginglist[i].value  = argv[optindex++];
         printf("key:%s, value:%s \n", tagginglist[i].name, tagginglist[i].value);
    }
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    
    set_bucket_tagging(&option, tagginglist, num, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket tagging successfully. \n");
    }
    else
    {
        printf("set bucket tagging failed(%s).\n", obs_get_status_name(ret_status));
    }

}
 
//get bucket tagging
void test_get_bucket_tagging_new(int argc, char **argv, int optindex)
{
    obs_options option;
    init_obs_options(&option);
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
     obs_get_bucket_tagging_handler response_handler = 
    {
         {&response_properties_callback, &get_tagging_complete_callback}, 
            &get_bucket_tagging_callback
    };

    TaggingInfo tagging_info;
    memset_s(&tagging_info,sizeof(tagging_info) ,0, sizeof(TaggingInfo));
    tagging_info.ret_status = OBS_STATUS_BUTT;

    get_bucket_tagging(&option, &response_handler, &tagging_info);

    if (OBS_STATUS_OK == tagging_info.ret_status) {
        printf("get bucket tagging successfully.\n");
    }
    else
    {
        printf("get bucket tagging failed(%s).\n", obs_get_status_name(tagging_info.ret_status));
    }

}

//delete bucket tagging
void test_delete_bucket_tagging_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    
    delete_bucket_tagging(&option, &response_handler, &ret_status);
    
    if (ret_status == OBS_STATUS_OK) {
        printf("delete bucket tagging successfully.\n");
    }
    else
    {
        printf("delete bucket tagging failed(%s).\n", obs_get_status_name(ret_status));
    }

}



// set bucket quota ------------------------------------------------------
static void test_set_bucket_quota_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    char *bucket_name = argv[optindex++];
    uint64_t bucketquota = atol(argv[optindex++]);
    printf("Bucket's name is == %s, bucketquota= %lu. \n", bucket_name, bucketquota);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    set_bucket_quota(&option, bucketquota, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) 
    {
        printf("set bucket quota successfully. \n");
    }
    else
    {
        printf("set bucket quota failed(%s).\n", obs_get_status_name(ret_status));
    }
}

// get bucket quota ------------------------------------------------------
static void test_get_bucket_quota_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    printf("Bucket's name is == %s. \n", bucket_name);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };
    
    uint64_t bucketquota = 0;
    get_bucket_quota(&option, &bucketquota, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("Bucket=%s  Quota=%lu \n get bucket quota successfully. \n ",
            bucket_name, bucketquota);
    }
    else
    {
        printf("get bucket quota failed(%s).\n", obs_get_status_name(ret_status));
    }
}


typedef char(*pointer_char_256)[OBS_COMMON_LEN_256];

pointer_char_256 parserString(char* sourceString, const char* delimiter)
{
    char *saveptr1, *str1, *token;
    int j;
    pointer_char_256 p;
    for (j = 1, str1 = sourceString; ; j++, str1 = NULL) {
        token = strtok_r(str1, delimiter, &saveptr1);
        if (token == NULL)
            break;

        if (j == 1) {
            p = (pointer_char_256)malloc(sizeof(char) * OBS_COMMON_LEN_256);
            if (!p)
                exit(EXIT_FAILURE);
        }
        else {
            p = (pointer_char_256)realloc(p, j * sizeof(char) * OBS_COMMON_LEN_256);
            if (!p)
                exit(EXIT_FAILURE);
        }

        snprintf_s(p[j - 1] , OBS_COMMON_LEN_256 - 1, OBS_COMMON_LEN_256 - 2, "%s",  token);
    }
    return p;
}

const char ** parserString2CPP(char* sourceString, const char* delimiter, int *num)
{
    char *saveptr1, *str1, *token;
    int j;
    char ** pp = 0;
    char * pointer_char = 0;

    for (j = 1, str1 = sourceString; ; j++, str1 = NULL) {
        token = strtok_r(str1, delimiter, &saveptr1);
        if (token == NULL)
            break;

        if (j == 1) {
            pp = (char **)malloc(sizeof(char **));
            if (!pp)
                exit(EXIT_FAILURE);
        }
        else {
            pp = (char **)realloc(pp, j * sizeof(char **));
            if (!pp)
                exit(EXIT_FAILURE);
        }

        pp[j-1] = (char *)malloc(sizeof(char) * OBS_COMMON_LEN_256);
        if (!pp[j - 1])
            exit(EXIT_FAILURE);
        snprintf_s(pp[j-1] , OBS_COMMON_LEN_256, OBS_COMMON_LEN_256-1,"%s",  token);
    }
    if (pp)
        *num = j - 1;
    return (const char **) pp;
}

void test_object_option(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];
    obs_options option;
    init_obs_options(&option);

    const char *delimiter = ",";
    char *cKey = 0; 
    char *cOrigin = 0; 
    pointer_char_256 allowed_method = 0;
    unsigned int am_num; 
    pointer_char_256 request_header = 0;
    unsigned int rh_num;

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, PROTOCOL_PREFIX, KEY_ALLOWED_METHOD_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, KEY_ALLOWED_METHOD, KEY_ALLOWED_METHOD_LEN)) {
            allowed_method = parserString(&(param[KEY_ALLOWED_METHOD_LEN]), delimiter);
        }
        else if (!strncmp(param, KEY_AM_NUM, KEY_AM_NUM_LEN)) {
            am_num = convertInt(&(param[KEY_AM_NUM_LEN]), "am_num");
        }
        else if (!strncmp(param, KEY_REQUEST_HEADER, KEY_REQUEST_HEADER_LEN)) {
            request_header = parserString(&(param[KEY_REQUEST_HEADER_LEN]), delimiter);
        }
        else if (!strncmp(param, KEY_REQUEST_HEADER_NUM, KEY_REQUEST_HEADER_NUM_LEN)) {
            rh_num = convertInt(&(param[KEY_REQUEST_HEADER_NUM_LEN]), "rh_num");
        }
        else if (!strncmp(param, KEY_CKEY, KEY_CKEY_LEN)) {
            cKey = &(param[KEY_CKEY_LEN]);
        }
        else if (!strncmp(param, KEY_CORIGIN, KEY_CORIGIN_LEN)) {
            cOrigin = &(param[KEY_CORIGIN_LEN]);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    obs_response_handler resqonseHandler =
    {
        &response_properties_callback,
        &response_complete_callback
    };

    obs_options_object(&option, cKey, cOrigin, allowed_method, am_num, request_header,
            rh_num, &resqonseHandler, &ret_status);

    if (OBS_STATUS_OK == ret_status)
    {
        printf("object option successfully. \n");
    }
    else
    {
        printf("object option failed(%s).\n", obs_get_status_name(ret_status));
    }

    if (allowed_method)
        free(allowed_method);
    if (request_header)
        free(request_header);
}

void test_bucket_option(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];
    obs_options option;
    init_obs_options(&option);

    const char *delimiter = ",";
    char *cKey = 0; 
    char *cOrigin = 0; 
    pointer_char_256 allowed_method = 0;
    unsigned int am_num; 
    pointer_char_256 request_header = 0;
    unsigned int rh_num;

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, PROTOCOL_PREFIX, KEY_ALLOWED_METHOD_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, KEY_ALLOWED_METHOD, KEY_ALLOWED_METHOD_LEN)) {
            allowed_method = parserString(&(param[KEY_ALLOWED_METHOD_LEN]), delimiter);
        }
        else if (!strncmp(param, KEY_AM_NUM, KEY_AM_NUM_LEN)) {
            am_num = convertInt(&(param[KEY_AM_NUM_LEN]), "am_num");
        }
        else if (!strncmp(param, KEY_REQUEST_HEADER, KEY_REQUEST_HEADER_LEN)) {
            request_header = parserString(&(param[KEY_REQUEST_HEADER_LEN]), delimiter);
        }
        else if (!strncmp(param, KEY_REQUEST_HEADER_NUM, KEY_REQUEST_HEADER_NUM_LEN)) {
            rh_num = convertInt(&(param[KEY_REQUEST_HEADER_NUM_LEN]), "rh_num");
        }
        else if (!strncmp(param, KEY_CORIGIN, KEY_CORIGIN_LEN)) {
            cOrigin = &(param[KEY_CORIGIN_LEN]);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    obs_response_handler resqonseHandler =
    {
        &response_properties_callback,
        &response_complete_callback
    };

    obs_options_bucket(&option, cOrigin, allowed_method, am_num, request_header,
            rh_num, &resqonseHandler, &ret_status);

    if (OBS_STATUS_OK == ret_status)
    {
        printf("bucket option successfully. \n");
    }
    else
    {
        printf("bucket option failed(%s).\n", obs_get_status_name(ret_status));
    }

    if (allowed_method)
        free(allowed_method);
    if (request_header)
        free(request_header);
}
void test_set_bucket_cors(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];
    obs_options option;
    init_obs_options(&option);

    const char *delimiter = ",";
    obs_bucket_cors_conf bucketCorsConf; 
    memset_s(&bucketCorsConf,sizeof(bucketCorsConf) ,0, sizeof(obs_bucket_cors_conf));

    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    // 设置回调函数
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    int allowed_method_num = 0, allowed_origin_num = 0, allowed_header_num = 0, expose_header_num = 0;


    char *id = 0;

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, ID_PREFIX, ID_PREFIX_LEN)) {
            bucketCorsConf.id = &(param[ID_PREFIX_LEN]);
        }       
        else if (!strncmp(param, MAX_AGE_SECONDS_PREFIX, MAX_AGE_SECONDS_PREFIX_LEN)) {
            bucketCorsConf.max_age_seconds = &(param[MAX_AGE_SECONDS_PREFIX_LEN]);
        }       
        else if (!strncmp(param, ALLOWED_METHOD_PREFIX, ALLOWED_METHOD_PREFIX_LEN)) {
            bucketCorsConf.allowed_method = parserString2CPP(&(param[ALLOWED_METHOD_PREFIX_LEN]), delimiter, &allowed_method_num);
        } 
        else if (!strncmp(param, ALLOWED_METHOD_NUM, ALLOWED_METHOD_NUM_LEN)) {
            bucketCorsConf.allowed_method_number = convertInt(&(param[ALLOWED_METHOD_NUM_LEN]), "allowed_method_num");
        }
        else if (!strncmp(param, ALLOWED_ORIGIN_PREFIX, ALLOWED_ORIGIN_PREFIX_LEN)) {
            bucketCorsConf.allowed_origin =  parserString2CPP(&(param[ALLOWED_ORIGIN_PREFIX_LEN]), delimiter, &allowed_origin_num);
        }           
        else if (!strncmp(param, ALLOWED_ORIGIN_NUM, ALLOWED_ORIGIN_NUM_LEN)) {
            bucketCorsConf.allowed_origin_number = convertInt(&(param[ALLOWED_ORIGIN_NUM_LEN]), "allowed_origin_num");
        }
        else if (!strncmp(param, ALLOWED_HEADER_PREFIX, ALLOWED_HEADER_PREFIX_LEN)) {
            bucketCorsConf.allowed_header =  parserString2CPP(&(param[ALLOWED_HEADER_PREFIX_LEN]), delimiter, &allowed_header_num);
        }           
        else if (!strncmp(param, ALLOWED_HEADER_NUM, ALLOWED_HEADER_NUM_LEN)) {
            bucketCorsConf.allowed_header_number = convertInt(&(param[ALLOWED_HEADER_NUM_LEN]), "allowed_header_num");
        }           
        else if (!strncmp(param, EXPOSE_HEADER_PREFIX, EXPOSE_HEADER_PREFIX_LEN)) {
            bucketCorsConf.expose_header = parserString2CPP(&(param[EXPOSE_HEADER_PREFIX_LEN]), delimiter, &expose_header_num);
        }   
        else if (!strncmp(param, EXPOSE_HEADER_NUM, EXPOSE_HEADER_NUM_LEN)) {
            bucketCorsConf.expose_header_number = convertInt(&(param[EXPOSE_HEADER_NUM_LEN]), "expose_header_num");
        }               
        else if (!strncmp(param, PROTOCOL_PREFIX, KEY_ALLOWED_METHOD_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }       
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    set_bucket_cors_configuration(&option, &bucketCorsConf, 1, &response_handler, &ret_status); 
    if (OBS_STATUS_OK == ret_status) {
        printf("set_bucket_cors successfully.\n");
    }
    else {
        printf("set_bucket_cors failed(%s).\n", obs_get_status_name(ret_status));
    }

    int i = 0;
    for (i = allowed_method_num - 1; i >= 0; i--)
    {
        if (bucketCorsConf.allowed_method[i])
            free((void *)bucketCorsConf.allowed_method[i]);
    }   
    if (bucketCorsConf.allowed_method)
        free(bucketCorsConf.allowed_method); 

    for (i = allowed_origin_num - 1; i >= 0; i--)
    {
        if (bucketCorsConf.allowed_origin[i])
            free((void *)bucketCorsConf.allowed_origin[i]);
    }   
    if (bucketCorsConf.allowed_origin)
        free((void *)bucketCorsConf.allowed_origin);

    for (i = allowed_header_num - 1; i >= 0; i--)
    {
        if (bucketCorsConf.allowed_header[i])
            free((void *)bucketCorsConf.allowed_header[i]);
    }   
    if (bucketCorsConf.allowed_header)
        free((void *)bucketCorsConf.allowed_header);

    for (i = expose_header_num - 1; i >= 0; i--)
    {
        if (bucketCorsConf.expose_header[i])
            free((void *)bucketCorsConf.expose_header[i]);
    }   
    if (bucketCorsConf.expose_header)
        free((void *)bucketCorsConf.expose_header);

}

void test_get_cors_config(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];
    obs_options option;


    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style     = gDefaultURIStyle;
    
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    // 设置回调函数
    obs_cors_handler cors_handler_info =
    {
        {&response_properties_callback, &response_complete_callback},
        &get_cors_info_callback
    };

    get_bucket_cors_configuration(&option, &cors_handler_info, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("get_cors_config successfully.\n");
    }
    else {
        printf("get_cors_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}

static void test_delete_cors_config(int argc, char **argv, int optindex)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];

    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style     = gDefaultURIStyle;
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    // 设置回调函数
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    delete_bucket_cors_configuration(&option, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("delete_cors_config successfully.\n");
    }
    else
    {
        printf("delete_cors_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}

// set bucket logging with grant ------------------------------------------------------
static void test_set_bucket_logging_with_grant_new_s3(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    obs_acl_group *g;
    obs_status ret_status = OBS_STATUS_BUTT;
    char *target_bucket = argv[optindex++];
    char *target_prefix = argv[optindex++];
    
    obs_options option;
    init_obs_options(&option);
    printf("Bucket's name is == %s. \n", bucket_name);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;  
    
    obs_response_handler response_handler =
    { 
        0,  &response_complete_callback
    };

    g = set_grant_acl(argc, argv, optindex, &option);
        
    set_bucket_logging_configuration(&option, target_bucket, target_prefix, 
            g, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket(%s) logging successfully. \n", bucket_name);
    }
    else
    {
        printf("set bucket logging failed(%s).\n",obs_get_status_name(ret_status));
    }
    if(g)
    {
        if (g->acl_grants)
        {
            free(g->acl_grants);
        }
        free(g);
    }
}

static void test_set_bucket_logging_with_grant_new_obs(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    obs_acl_group *g;
    obs_status ret_status = OBS_STATUS_BUTT;
    char *target_bucket = argv[optindex++];
    char *target_prefix = argv[optindex++];
    //char *agency = argv[optindex++];
    
    obs_options option;
    init_obs_options(&option);
    printf("Bucket's name is == %s. \n", bucket_name);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;  
    
    obs_response_handler response_handler =
    { 
        0,  &response_complete_callback
    };

    g = set_grant_acl(argc, argv, optindex, &option);
        
    set_bucket_logging_configuration_obs(&option, target_bucket, target_prefix, "agency_test",
            g, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket(%s) logging successfully. \n", bucket_name);
    }
    else
    {
        printf("set bucket logging failed(%s).\n",obs_get_status_name(ret_status));
    }
    if(g)
    {
        if (g->acl_grants)
        {
            free(g->acl_grants);
        }
        free(g);
    }
}

static void test_set_bucket_logging_with_grant_new(int argc, char **argv, int optindex)
{
    //if(demoUseObsApi == OBS_USE_API_S3) {
    //    test_set_bucket_logging_with_grant_new_s3(argc, argv, optindex);
    //} else {
    //    test_set_bucket_logging_with_grant_new_obs(argc, argv, optindex);
    //}
    
    char *bucket_name = argv[optindex++];
    obs_acl_group *g;
    obs_status ret_status = OBS_STATUS_BUTT;
    char *target_bucket = argv[optindex++];
    char *target_prefix = argv[optindex++];
    
    obs_options option;
    init_obs_options(&option);
    printf("Bucket's name is == %s. \n", bucket_name);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;  
    
    obs_response_handler response_handler =
    { 
        0,  &response_complete_callback
    };

    g = set_grant_acl(argc, argv, optindex, &option);
        
    if(option.request_options.auth_switch == OBS_S3_TYPE)
    {
        set_bucket_logging_configuration(&option, target_bucket, target_prefix, 
            g, &response_handler, &ret_status);
    }
    else
    {
        set_bucket_logging_configuration_obs(&option, target_bucket, target_prefix, "agency_test",
            g, &response_handler, &ret_status);
    }

    
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket(%s) logging successfully. \n", bucket_name);
    }
    else
    {
        printf("set bucket logging failed(%s).\n",obs_get_status_name(ret_status));
    }
    if(g)
    {
        if (g->acl_grants)
        {
            free(g->acl_grants);
        }
        free(g);
    }
}

// get bucket logging
void test_get_bucket_logging_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    printf("Bucket's name is == %s. \n", bucket_name);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    obs_response_handler response_handler = 
    {
         0, &response_complete_callback
    };
     
    bucket_logging_message logging_message;
    init_bucket_get_logging_message(&logging_message);

    get_bucket_logging_configuration(&option, &response_handler, &logging_message, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) 
    {
        printf("get bucket(%s) logging successfully. \n", bucket_name);
        if (logging_message.target_bucket) 
        {
            printf("Target_Bucket: %s\n", logging_message.target_bucket);
            if ( logging_message.target_prefix) 
            {
                printf("Target_Prefix: %s\n",  logging_message.target_prefix);
            }
            print_grant_info(*logging_message.acl_grant_count, logging_message.acl_grants);
        }
        else 
        {
            printf("Service logging is not enabled for this bucket.\n");
        }
    }
    else
    {
        printf("get bucket logging failed(%s).\n", obs_get_status_name(ret_status));
    }

    destroy_logging_message(&logging_message);
}

// set bucket storage class ------------------------------------------------------
void test_set_bucket_storage_class_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    obs_status ret_status = OBS_STATUS_BUTT;
    int storage_class_policy = convertInt(argv[optindex++], "storage_class");
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    printf("Bucket's name is == %s, storageclass: %d. \n", bucket_name, storage_class_policy);

    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
   
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };
    set_bucket_storage_class_policy(&option, storage_class_policy, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket storage class successfully. \n");
    }
    else
    {
        printf("set bucket storage class failed(%s).\n", 
                            obs_get_status_name(ret_status));
    }
}

//get bucket storage class policy
static void test_get_bucket_storage_class_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    printf("Bucket's name is == %s. \n", bucket_name);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }

    obs_get_bucket_storage_class_handler getBucketStorageResponse = 
    {
        {0, &response_complete_callback}, 
        &get_bucket_storageclass_handler
    };

    get_bucket_storage_class_policy(&option, &getBucketStorageResponse, &ret_status);

    if (OBS_STATUS_OK == ret_status) 
    {
        printf("get bucket storage class successfully.\n");
    }
    else
    {
        printf("get bucket storage class failed(%s).\n", obs_get_status_name(ret_status));
    }
}

static void test_set_bucket_website_all_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_set_bucket_redirect_all_conf set_bucket_redirect_all;

    char *redirect_host_name = argv[optindex++];
    char *redirect_protocol = argv[optindex++];
    set_bucket_redirect_all.host_name = redirect_host_name;
    set_bucket_redirect_all.protocol = redirect_protocol;

    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    set_bucket_website_configuration(&option, &set_bucket_redirect_all, NULL, 
        &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket website all successfully. \n");
    }
    else
    {
        printf("set bucket website all failed(%s).\n", obs_get_status_name(ret_status));
    }

}

void test_get_bucket_website_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
     obs_get_bucket_websiteconf_handler response_handler = 
    {
         {&response_properties_callback, &response_complete_callback}, 
            &get_bucket_websiteconf_callback
    };
    get_bucket_website_configuration(&option, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("get bucket website successfully.\n");
    }
    else
    {
        printf("get bucket website failed(%s).\n", obs_get_status_name(ret_status));
    }
    
}
void test_delete_bucket_website_new(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s. \n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    
    delete_bucket_website_configuration(&option, &response_handler, &ret_status);
    
    if (statusG == OBS_STATUS_OK) {
        printf("delete bucket website successfully.\n");
    }
    else
    {
        printf("delete bucket website failed(%s).\n", obs_get_status_name(ret_status));
    }

}

void *upload_thread_proc(void * thread_param)
{
    test_concurrent_upload_file_callback_data *concurrent_temp = (test_concurrent_upload_file_callback_data *)thread_param;
    
    obs_upload_part_info uploadPartInfo;
    uploadPartInfo.part_number = concurrent_temp->part_num;
    uploadPartInfo.upload_id = concurrent_temp->upload_id;
    
    obs_upload_handler uploadHandler =
    { 
        {&concurrent_response_properties_callback, &concurrent_upload_file_complete_callback},
        &test_concurrent_upload_part_data_callback
    };
    
    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);
    
  
    upload_part(concurrent_temp->option,concurrent_temp->key,&uploadPartInfo,
                concurrent_temp->part_size, &putProperties, 0, &uploadHandler, concurrent_temp);
    if (OBS_STATUS_OK == concurrent_temp->ret_status) {
        printf("test upload part %d successfully. \n", uploadPartInfo.part_number);
    }
    else
    {
    
        abort_multi_part_upload(concurrent_temp->option, concurrent_temp->key, concurrent_temp->upload_id,0, 0);
        printf("test upload part %d faied(%s).\n",uploadPartInfo.part_number,
            obs_get_status_name(concurrent_temp->ret_status));
    }
}


static void start_upload_threads(test_upload_file_callback_data data,
    char *concurrent_upload_id, uint64_t filesize, FILE ** uploadFilePool,
    char *key, obs_options option,test_concurrent_upload_file_callback_data *concurrent_upload_file)
{
     int partCount = data.part_num;
     test_concurrent_upload_file_callback_data *concurrent_temp;
     concurrent_temp = concurrent_upload_file;
     int err;
     int i= 0;
     
     for(i=0; i <partCount; i++)
     {
        memset_s(concurrent_temp[i].etag,sizeof(concurrent_temp[i].etag) ,0,sizeof(concurrent_temp[i].etag));
        concurrent_temp[i].part_num = i+1;
        concurrent_temp[i].infile = uploadFilePool[i];
        concurrent_temp[i].upload_id = concurrent_upload_id;
        concurrent_temp[i].key = key;
        concurrent_temp[i].option = &option;
        concurrent_temp[i].offset = 0;
        if(i == partCount-1)
        {
            concurrent_temp[i].part_size = filesize - (data.part_size)*i;
            concurrent_temp[i].start_byte = (data.part_size)*i;
        }
        else
        {
            concurrent_temp[i].part_size = data.part_size;
            concurrent_temp[i].start_byte= (data.part_size)*i;
        }
     }
     
     pthread_t * arrThread = (pthread_t *)malloc(sizeof(pthread_t)*partCount);
     for(i = 0; i < partCount;i++)
     {  
        err = pthread_create(&arrThread[i], NULL,upload_thread_proc,(void *)&concurrent_upload_file[i]);
        if(err != 0)
        {
            printf("create thread failed i[%d]\n",i);
        }        
     }
     
     for(i = 0; i< partCount; i++)
     {
        err = pthread_join(arrThread[i], NULL);
        if(err != 0)
        {
            printf("join thread failed i[%d]\n",i);
        }
     }
     
     if(arrThread)
     {
        free(arrThread);
        arrThread = NULL;
     }  
}

static void test_concurrent_upload_part(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char concurrent_upload_id[2048]={0};
    int uploadSize = 5L * 1024 * 1024;                         // upload part size
    uint64_t filesize =0; 
    int i=0;
    // file total size
    char *filename = 0;
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    obs_canned_acl canned_acl = OBS_CANNED_ACL_PRIVATE;

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, FILENAME_PREFIX, FILENAME_PREFIX_LEN)) {
            filename = &(param[FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, UPLOAD_SLICE_SIZE, UPLOAD_SLICE_SIZE_LEN)) {
            uploadSize = convertInt(&(param[UPLOAD_SLICE_SIZE_LEN]), "upload_slice_size");
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    obs_response_handler Handler =
    { 
        &response_properties_callback, &response_complete_callback 
    };
        
    obs_complete_multi_part_upload_handler complete_multi_handler =
    { 
        {&response_properties_callback, &response_complete_callback},
        &CompleteMultipartUploadCallback
    };
        
    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);
    putProperties.canned_acl = canned_acl;
    
    //大文件信息:文件指针，文件大小，按照分段大小的分段数
    test_upload_file_callback_data data;
    memset_s(&data,sizeof(data), 0, sizeof(test_upload_file_callback_data));
    filesize = get_file_info(filename,&data);
    data.noStatus = 1;
    data.part_size = uploadSize;
    data.part_num = (filesize % uploadSize == 0) ? (filesize / uploadSize) : (filesize / uploadSize +1);
    uploadFilePool = init_uploadfilepool(uploadFilePool, data.part_num, filename);

    //初始化上传段任务返回uploadId: uploadIdReturn
    char uploadIdReturn[256] = {0};
    int upload_id_return_size = 255;
    initiate_multi_part_upload(&option,key,upload_id_return_size,uploadIdReturn, &putProperties,
        0,&Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test init upload part return uploadIdReturn(%s). \n", uploadIdReturn);
        strcpy_s(concurrent_upload_id,sizeof(concurrent_upload_id),uploadIdReturn);
    }
    else
    {
        printf("test init upload part faied(%s).\n", obs_get_status_name(ret_status));
    }
    //并发上传
    test_concurrent_upload_file_callback_data *concurrent_upload_file;
    concurrent_upload_file =(test_concurrent_upload_file_callback_data *)malloc(
                sizeof(test_concurrent_upload_file_callback_data)*(data.part_num+1));
    if(concurrent_upload_file == NULL)
    {
        printf("malloc test_concurrent_upload_file_callback_data failed!!!");
        return;
    }
    test_concurrent_upload_file_callback_data *concurrent_upload_file_complete = concurrent_upload_file;
    start_upload_threads(data, concurrent_upload_id,filesize, uploadFilePool,key, option, concurrent_upload_file_complete);

    // 合并段
    obs_complete_upload_Info *upload_Info = (obs_complete_upload_Info *)malloc(
        sizeof(obs_complete_upload_Info)*data.part_num);
    if(upload_Info == NULL)
    {
        printf("malloc upload_Info failed!!!");
        free(concurrent_upload_file);
        return;
    }   
    deinit_uploadfilepool(uploadFilePool, data.part_num);
    uploadFilePool = NULL;
    for(i=0; i<data.part_num; i++)
    {
        upload_Info[i].part_number = concurrent_upload_file_complete[i].part_num;
        upload_Info[i].etag = concurrent_upload_file_complete[i].etag;
    }
    printf("test_concurrent_upload_part debug1\n");
    complete_multi_part_upload(&option,key,uploadIdReturn, data.part_num,upload_Info,&putProperties,&complete_multi_handler,&ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("test complete upload successfully. \n");
    }
    else
    {
        printf("test complete upload faied(%s).\n", obs_get_status_name(ret_status));
    }
    if(concurrent_upload_file)
    {
        free(concurrent_upload_file);
        free(upload_Info);
        concurrent_upload_file = NULL;
        upload_Info = NULL;
    }
}


static void test_concurrent_copy_part(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char concurrent_upload_id[2048]={0};
    uint64_t start_byte =0;
    uint64_t byte_count =0;
    int i=0;
    char* part_num =0;

    char* destinationBucket = 0;
    char* destinationKey = 0;

    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    obs_canned_acl canned_acl = OBS_CANNED_ACL_PRIVATE;

    obs_options option;
    init_obs_options(&option);
    
    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, PART_NUM, PART_NUM_LEN)) {
            part_num = &(param[PART_NUM_LEN]);
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, DESTINATION_BUCKET, DESTINATION_BUCKET_LEN)) {
            destinationBucket = &(param[DESTINATION_BUCKET_LEN]);    
        }
        else if (!strncmp(param, DESTINATION_KEY, DESTINATION_KEY_LEN)) {
            destinationKey = &(param[DESTINATION_KEY_LEN]);    
        }
        else if (!strncmp(param, BYTE_COUNT_PREFIX, BYTE_COUNT_PREFIX_LEN)) {
            byte_count = convertInt(&(param[BYTE_COUNT_PREFIX_LEN]), "byte_count");
        }
        else if (!strncmp(param, START_BYTE_PREFIX, START_BYTE_PREFIX_LEN)) {
            start_byte = convertInt(&(param[START_BYTE_PREFIX_LEN]), "start_byte");
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = destinationBucket;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler Handler =
    { 
        &response_properties_callback, &response_complete_callback 
    };
    obs_list_parts_handler listHandler =
    { 
        {NULL, &list_part_complete_callback},
        &listPartsCallbackEx
    };

    //初始化上传段任务返回uploadId: uploadIdReturn
    char uploadIdReturn[256] = {0};
    int upload_id_return_size = 255;
    initiate_multi_part_upload(&option,destinationKey,upload_id_return_size,uploadIdReturn, 0,
        0,&Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test init upload part return uploadIdReturn(%s). \n", uploadIdReturn);
        strcpy_s(concurrent_upload_id,sizeof(concurrent_upload_id),uploadIdReturn);
    }
    else
    {
        printf("test init upload part faied(%s).\n", obs_get_status_name(ret_status));
    }
    
    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    put_properties.byte_count = byte_count;
    put_properties.start_byte = start_byte;
    
    char etag_return[256] = {0};
    obs_copy_destination_object_info object_info;
    memset_s(&object_info, sizeof(obs_copy_destination_object_info), 0, sizeof(obs_copy_destination_object_info));
    object_info.destination_bucket = destinationBucket;
    object_info.destination_key = destinationKey;
    object_info.etag_return = etag_return;
    object_info.etag_return_size = 265;
    
    obs_upload_part_info copypart;
    memset_s(&copypart, sizeof(obs_upload_part_info), 0, sizeof(obs_upload_part_info));
    copypart.part_number = atol(part_num);
    copypart.upload_id = uploadIdReturn;
    option.bucket_options.bucket_name = bucket_name;
    copy_part(&option, key, &object_info, &copypart, &put_properties,0,&Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf(" copy part  successfully. \n");
    }
    else
    {
        printf("copy part  failed(%s).\n", obs_get_status_name(ret_status));
        
    }
    //list parts
    list_part_info listpart;
    listpart.upload_id = uploadIdReturn;
    listpart.max_parts = atol(part_num);
    listpart.part_number_marker = 0;
    list_parts_callback_data data;
    memset_s(&data, sizeof(data), 0, sizeof(list_parts_callback_data));
    option.bucket_options.bucket_name = destinationBucket;
    do{
        list_parts(&option, destinationKey,&listpart, &listHandler, &data);
    }while (OBS_STATUS_OK == data.ret_status && data.isTruncated 
                && (data.keyCount < listpart.max_parts));
    
    if (OBS_STATUS_OK == data.ret_status) 
    {
        printf("list parts OK.\n");
    }
    else 
    {
        printf("list parts failed(%s).\n", obs_get_status_name(data.ret_status));
    }
    
    abort_multi_part_upload(&option, destinationKey, uploadIdReturn,&Handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("abort_multi_part_upload  successfully. \n");
    }
    else
    {
        printf("abort_multi_part_upload  failed(%s).\n", obs_get_status_name(ret_status));
    }
}

static double g_progress = 0;
void test_progress_callback(double progress, uint64_t uploadedSize, uint64_t fileTotalSize, void *callback_data){
    if (progress == 100 || (g_progress < progress && progress - g_progress > 2)) {
        printf("test_progress_callback progress=%f  uploadedSize=%u fileTotalSize=%lu  callback_data=%p\n", progress, uploadedSize, fileTotalSize, callback_data);
        g_progress = progress;
    }
}

typedef struct obs_pause_upload_file
{
    int *pause_flag;
    char *filename;
} obs_pause_upload_file;

void *test_multi_thread_upload_file(void *pause_file)
{
    obs_pause_upload_file *pause_upload_flag = (obs_pause_upload_file*)pause_file;
    printf("pause_flag = %d, filename=%s\n", *(pause_upload_flag->pause_flag), pause_upload_flag->filename);

    obs_status ret_status = OBS_STATUS_BUTT;
    uint64_t uploadSliceSize =40L * 1024 * 1024;                   // upload part slice size
    int thread_num = 10;
    int check_point = 0;
    char *filename = pause_upload_flag->filename;    // the upload file name 
    char *bucket_name = "yan-test-411522";
    char *key = "pause_test";

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);

    obs_upload_file_configuration uploadFileInfo;
    memset_s(&uploadFileInfo,sizeof(obs_upload_file_configuration),0,sizeof(obs_upload_file_configuration));
    uploadFileInfo.check_point_file = NULL;
    uploadFileInfo.enable_check_point = check_point;
    uploadFileInfo.part_size = uploadSliceSize;
    uploadFileInfo.task_num = thread_num;
    uploadFileInfo.upload_file = filename;
    uploadFileInfo.pause_upload_flag = pause_upload_flag->pause_flag;

    obs_upload_file_server_callback server_callback;
    init_server_callback(&server_callback);

    cJSON *body = NULL;
    char *out = NULL;
    body = cJSON_CreateObject();

    cJSON_AddStringToObject(body, "bucket", "test_json");
    cJSON_AddStringToObject(body, "etag", "test_etag");
    out = cJSON_PrintUnformatted(body);

    server_callback.callback_url = "http://xxxxxx";
    server_callback.callback_host = NULL;
    server_callback.callback_body = out;
    server_callback.callback_body_type = "application/json";

    obs_upload_file_response_handler Handler =
    {
        {&response_properties_callback, &response_complete_callback},
        &uploadFileResultCallback,
        &test_progress_callback
    };

    upload_file(&option, key, 0, &uploadFileInfo, server_callback,&Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test upload file successfully. \n");
    } else {
        printf("test upload file faied(%s).\n", obs_get_status_name(ret_status));
    }
    cJSON_Delete(body);
    cJSON_free(out);
    printf("pause_flag = %d, filename=%s\n", *(pause_upload_flag->pause_flag), pause_upload_flag->filename);
    return ((void*)0);
}

void test_pause_upload_file()
{
    int err = 0;
    int i = 0;
    obs_pause_upload_file *pause_file[PAUSE_THREAD_NUM];
    int pause_flag0 = 0;
    int pause_flag1 = 0;
    int pause_flag2 = 0;

    for (i = 0; i < PAUSE_THREAD_NUM; i++) {
        pause_file[i] = (obs_pause_upload_file*)malloc(sizeof(obs_pause_upload_file));
        pause_file[i]->filename = (char*)malloc(sizeof(char));
        pause_file[i]->pause_flag = (int*)malloc(sizeof(int));
        memset_s(pause_file[i], sizeof(obs_pause_upload_file), 0, sizeof(obs_pause_upload_file));
    }

    pause_file[0]->pause_flag = &pause_flag0;
    pause_file[0]->filename = "huaweicloud-obs-sdk-c-linux_L3-20220326152318.tar.gz";
    pause_file[1]->pause_flag = &pause_flag1;
    pause_file[1]->filename = "esdk_obs_c.tar.gz";
    pause_file[2]->pause_flag = &pause_flag2;
    pause_file[2]->filename = "huaweicloud-obs-sdk-c-linux.tgz";

    pthread_t *arrThread = (pthread_t*)malloc(sizeof(pthread_t) * PAUSE_THREAD_NUM);
    for (i = 0; i < PAUSE_THREAD_NUM; i++) {
        err = pthread_create(&arrThread[i], NULL, test_multi_thread_upload_file, (void *)pause_file[i]);
        if (err != 0) {
            printf("create thread for test_pause_upload_file failed. i:%d, err:%d\n", i, err);
        }
    }

    sleep(2);
    pause_upload_file(pause_file[0]->pause_flag);

    for (i = 0; i < PAUSE_THREAD_NUM; i++) {
        pthread_join(arrThread[i], NULL);
    }

}

static void test_upload_file(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    uint64_t uploadSliceSize =40L * 1024 * 1024;                   // upload part slice size
    int thread_num = 10;
    int check_point = 0;
    char *filename = 0;    // the upload file name 
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    int pause_upload_flag = 0;

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, FILENAME_PREFIX, FILENAME_PREFIX_LEN)) {
            filename = &(param[FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, UPLOAD_SLICE_SIZE, UPLOAD_SLICE_SIZE_LEN)) {
            uploadSliceSize = convertInt(&(param[UPLOAD_SLICE_SIZE_LEN]), "upload_slice_size");
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, THREAD_NUM_PREFIX, THREAD_NUM_PREFIX_LEN)) {
            thread_num = convertInt(&(param[THREAD_NUM_PREFIX_LEN]), "thread_num");
        }
        else if (!strncmp(param, CHECK_POINT, CHECK_POINT_LEN)) {
            check_point = convertInt(&(param[CHECK_POINT_LEN]), "check_point");
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    option.request_options.server_cert_path = NULL;    //set server cert , example: /etc/certs/cabundle.pem

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);

    obs_upload_file_configuration uploadFileInfo;
    memset_s(&uploadFileInfo,sizeof(obs_upload_file_configuration),0,sizeof(obs_upload_file_configuration));
    uploadFileInfo.check_point_file = NULL;
    uploadFileInfo.enable_check_point = check_point;
    uploadFileInfo.part_size = uploadSliceSize;
    uploadFileInfo.task_num = thread_num;
    uploadFileInfo.upload_file = filename;
    uploadFileInfo.pause_upload_flag = &pause_upload_flag;

    obs_upload_file_server_callback server_callback;
    init_server_callback(&server_callback);

    cJSON *body = NULL;
    char *out = NULL;
    body = cJSON_CreateObject();

    cJSON_AddStringToObject(body, "bucket", "test_json");
    cJSON_AddStringToObject(body, "etag", "test_etag");
    out = cJSON_PrintUnformatted(body);

    server_callback.callback_url = "http://8.45.3.18:3000/backend/v1/json";
    server_callback.callback_host = NULL;
    server_callback.callback_body = out;
    server_callback.callback_body_type = "application/json";

    obs_upload_file_response_handler Handler =
    { 
        {&response_properties_callback, &response_complete_callback},
        &uploadFileResultCallback,
        &test_progress_callback
    };

    upload_file(&option, key, 0, &uploadFileInfo, server_callback,&Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test upload file successfully. \n");
    }
    else
    {
        printf("test upload file faied(%s).\n", obs_get_status_name(ret_status));
    }
    cJSON_Delete(body);
}

static void test_download_file(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    uint64_t uploadSliceSize = 5L * 1024 * 1024;                   // upload part slice size
    int thread_num = 0;
    int check_point = 0;
    char *filename =0 ;                               // the upload file name 
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];

    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, FILENAME_PREFIX, FILENAME_PREFIX_LEN)) {
            filename = &(param[FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, UPLOAD_SLICE_SIZE, UPLOAD_SLICE_SIZE_LEN)) {
            uploadSliceSize = convertInt(&(param[UPLOAD_SLICE_SIZE_LEN]), "upload_slice_size");
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, THREAD_NUM_PREFIX, THREAD_NUM_PREFIX_LEN)) {
            thread_num = convertInt(&(param[THREAD_NUM_PREFIX_LEN]), "thread_num");
        }
        else if (!strncmp(param, CHECK_POINT, CHECK_POINT_LEN)) {
            check_point = convertInt(&(param[CHECK_POINT_LEN]), "check_point");
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    obs_get_conditions getConditions;
    memset_s(&getConditions,sizeof(obs_get_conditions),0,sizeof(obs_get_conditions));
    init_get_properties(&getConditions);
    
    obs_download_file_configuration downloadFileConfig;
    memset_s(&downloadFileConfig,sizeof(obs_download_file_configuration),0,
                    sizeof(obs_download_file_configuration));
    downloadFileConfig.check_point_file = NULL;
    downloadFileConfig.enable_check_point = check_point;
    downloadFileConfig.part_size = uploadSliceSize;
    downloadFileConfig.task_num = thread_num;
    downloadFileConfig.downLoad_file= filename;

    obs_download_file_response_handler Handler =
    { 
        {&response_properties_callback, &response_complete_callback },
        &downloadFileResultCallback
    };

    download_file(&option, key, 0,&getConditions,0,&downloadFileConfig, 
            &Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test download file successfully. \n");
    }
    else
    {
        printf("test download file faied(%s).\n", obs_get_status_name(ret_status));
    }
}

static void test_set_bucket_website_conf_new(int argc, char **argv, int optindex)
{
    obs_options option;
    char *bucket_name = argv[optindex++];  
    printf("Bucket's name is == %s. \n", bucket_name);

    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_set_bucket_website_conf set_bucket_website_conf; 
    char *param = argv[optindex ++];
    if (!strncmp(param, SUFFIX_PREFIX, SUFFIX_PREFIX_LEN))
    {
       set_bucket_website_conf.suffix = &param[SUFFIX_PREFIX_LEN]; 
    }
    param = argv[optindex ++];
    if (!strncmp(param, KEY_PREFIX, KEY_PREFIX_LEN))
    {
       set_bucket_website_conf.key = &param[KEY_PREFIX_LEN]; 
    }
    bucket_website_routingrule temp[10] = {0};
    memset_s(temp,sizeof(temp), 0, sizeof(bucket_website_routingrule)*10);
    set_bucket_website_conf.routingrule_info = temp; 
    int rule_number = 0;
    char *delim = ",";

    while (optindex < argc) {
        param = argv[optindex ++];
        if (!strncmp(param, KEY_PREFIX_EQUALS_PREFIX, KEY_PREFIX_EQUALS_PREFIX_LEN))
        {
           char *prefix= &param[KEY_PREFIX_EQUALS_PREFIX_LEN];
           rule_number = 0;
           char* p=strtok(prefix,delim);
           while(p!=NULL){
               temp[rule_number++].key_prefix_equals = p;
               p=strtok(NULL,delim);
           }
        }
        else if (!strncmp(param, REPLACE_KEY_PREFIX_WITH_PREFIX, REPLACE_KEY_PREFIX_WITH_PREFIX_LEN))
        {
           char *replace_key_prefix= &param[REPLACE_KEY_PREFIX_WITH_PREFIX_LEN];
           rule_number = 0;
           char* p=strtok(replace_key_prefix,delim);
           while(p!=NULL){
               temp[rule_number++].replace_key_prefix_with = p;
               p=strtok(NULL,delim);
           }
        }
        else if (!strncmp(param, REPLACE_KEY_WITH_PREFIX, REPLACE_KEY_WITH_PREFIX_LEN))
        {
           char *replace_key= &param[REPLACE_KEY_WITH_PREFIX_LEN];
           rule_number = 0;
           char* p=strtok(replace_key,delim);
           while(p!=NULL){
               temp[rule_number++].replace_key_with = p;
               p=strtok(NULL,delim);
           }
        }
        else if (!strncmp(param, HTTP_ERROR_CODE_RETURNED_EQUALS_PREFIX, HTTP_ERROR_CODE_RETURNED_EQUALS_PREFIX_LEN))
        {
           char *error_code = &param[HTTP_ERROR_CODE_RETURNED_EQUALS_PREFIX_LEN];
           rule_number = 0;
           char* p=strtok(error_code,delim);
           while(p!=NULL){
               temp[rule_number++].http_errorcode_returned_equals = p;
               p=strtok(NULL,delim);
           }
        }
        else if (!strncmp(param, HTTP_REDIRECT_CODE_PREFIX, HTTP_REDIRECT_CODE_PREFIX_LEN))
        {
           char *redirect_code = &param[HTTP_REDIRECT_CODE_PREFIX_LEN];
           rule_number = 0;
           char* p=strtok(redirect_code,delim);
           while(p!=NULL){
               temp[rule_number++].http_redirect_code = p;
               p=strtok(NULL,delim);
           }
        }
        else if (!strncmp(param, HOSTNAME_PREFIX, HOSTNAME_PREFIX_LEN))
        {
           char *host_name = &param[HOSTNAME_PREFIX_LEN];
           rule_number = 0;
           char* p=strtok(host_name,delim);
           while(p!=NULL){
               temp[rule_number++].host_name = p;
               p=strtok(NULL,delim);
           }
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN))
        {
           char *protocol = &param[PROTOCOL_PREFIX_LEN];
           rule_number = 0;
           char* p=strtok(protocol,delim);
           while(p!=NULL){
               temp[rule_number++].protocol = p;
               p=strtok(NULL,delim);
           }
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
           fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    set_bucket_website_conf.routingrule_count = rule_number;
 
    obs_status ret_status = OBS_STATUS_OK;
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    set_bucket_website_configuration(&option, NULL, &set_bucket_website_conf, 
        &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket website conf successfully. \n");
    }
    else
    {
        printf("set bucket website conf failed(%s).\n", obs_get_status_name(ret_status));
    }

}

// ./object_test restore_object bucketname key days [tier=""] [version_id=""]
void test_restore_object(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    char *days = argv[optindex++];


    obs_object_info object_info;
    memset_s(&object_info,sizeof(object_info) ,0, sizeof(obs_object_info));
    object_info.key =key;

    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_tier tier = 0;

    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
        }
        else if (!strncmp(param, VERSIONID_PREFIX, VERSIONID_PREFIX_LEN)) {
            object_info.version_id = &(param[VERSIONID_PREFIX_LEN]);
        }
        else if (!strncmp(param, TIER_PREFIX, TIER_PREFIX_LEN)) {
            tier = (obs_tier)convertInt(&(param[TIER_PREFIX_LEN]), "tier");

        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }

    obs_response_handler handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };

    restore_object(&option, &object_info, days,tier,&handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("restore object successfully. \n");
    }
    else
    {
        printf("restore object faied(%s).\n", obs_get_status_name(ret_status));
        return;
    }

}

// ./object_test copy_object source_bucket source_key target_bucket target_key  is_copy [version_id]
void test_copy_object(int argc, char **argv, int optindex)
{
    char *source_bucket = argv[optindex++];
    char *source_key = argv[optindex++];
    char *target_bucket = argv[optindex++];
    char *target_key = argv[optindex++];
    obs_status ret_status = OBS_STATUS_BUTT;
    char eTag[OBS_COMMON_LEN_256] = {0};
    int64_t lastModified;

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = source_bucket;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;


    obs_copy_destination_object_info objectinfo ={0};
    objectinfo.destination_bucket = target_bucket;
    objectinfo.destination_key = target_key;
    objectinfo.etag_return = eTag;
    objectinfo.etag_return_size = sizeof(eTag);
    objectinfo.last_modified_return = &lastModified;

    obs_put_properties put_properties;
    init_put_properties(&put_properties);

    unsigned int is_copy = 0;
    char *version_id = 0;

    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
        }
        else if (!strncmp(param, VERSIONID_PREFIX, VERSIONID_PREFIX_LEN)) {
            version_id = &(param[VERSIONID_PREFIX_LEN]);
        }
        else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN)) {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }
        else if (!strncmp(param, IS_COPY_PREFIX, IS_COPY_PREFIX_LEN)) {
            is_copy = convertInt(&(param[IS_COPY_PREFIX_LEN]), "is_copy");

        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }

    obs_response_handler responseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };

    copy_object(&option, source_key, version_id, &objectinfo, is_copy, &put_properties, 0, &responseHandler,&ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("test_copy_object  successfully. \n");

    }
    else
    {
        printf("test_copy_object failed(%s).\n", obs_get_status_name(ret_status));
    }

}


// put object------------------------------------------------------------
static void test_put_object_with_encrypt(int argc, char **argv, int optindex)
{
    char *file_name = 0;
    uint64_t content_length = 0;
    obs_options option;
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    
    printf("Bucket's name is == %s, object's name is == %s. \n", bucket_name, key);
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    //服务端加密
    /*SSE-KMS加密*/
    server_side_encryption_params encryption_params;
    memset_s(&encryption_params,sizeof(encryption_params) ,0, sizeof(server_side_encryption_params));
    //不设置 系统会生成默认的加密密钥

    /*SSE-C*/
    char* buffer = "K7QkYpBkM5+hcs27fsNkUnNVaobncnLht/rCB2o/9Cw=";

    //check parameters
    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, FILENAME_PREFIX, FILENAME_PREFIX_LEN)) {
            file_name = &(param[FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_LENGTH_PREFIX, CONTENT_LENGTH_PREFIX_LEN)) {
            content_length = convertInt(&(param[CONTENT_LENGTH_PREFIX_LEN]), "content_length");
        }
        else if (!strncmp(param, USE_KMS, USE_KMS_LEN)) {
            encryption_params.encryption_type = OBS_ENCRYPTION_KMS;
            if(demoUseObsApi == OBS_USE_API_OBS) {
                encryption_params.kms_server_side_encryption = "kms";
            } else {
                encryption_params.kms_server_side_encryption = "aws:kms";
            }
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_SSEC, USE_SSEC_LEN)) {
            encryption_params.encryption_type = OBS_ENCRYPTION_SSEC;
            encryption_params.ssec_customer_algorithm = "AES256";
            encryption_params.ssec_customer_key = buffer;
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    //read from local file to buffer
    put_object_callback_data data;
    memset_s(&data,sizeof(data), 0, sizeof(put_object_callback_data));

    data.infile = 0;
    data.gb = 0;
    data.noStatus = 1;

    if (file_name) {
        if (!content_length) {
            content_length =  read_bytes_from_file(file_name, &data);
        }
        //Open the file
        if (!(data.infile = fopen(file_name, "rb" FOPEN_EXTRA_FLAGS))) {
            fprintf(stderr, "\nERROR: Failed to open input file %s: ", file_name);
            return;
        }
    }   

    data.content_length = data.originalContentLength = content_length;
    
    obs_put_object_handler putobjectHandler =
    { 
        { &responsePropertiesCallback,
          &put_object_complete_callback},
        &put_object_data_callback
    };
    
    put_object(&option,key,content_length,&put_properties,&encryption_params,&putobjectHandler,&data);

    if (OBS_STATUS_OK == data.put_status) {
        printf("put object [%s,%s]  with encrypt successfully. \n", bucket_name,key);
    }
    else
    {
        printf("put object [%s,%s]  with encrypt faied(%s).\n", bucket_name,key,
            obs_get_status_name(data.put_status));
    }
}

void test_get_object_with_encrypt(int argc, char **argv, int optindex)
{
    
    char *bucket_name   = argv[optindex++];
    char *key           = argv[optindex++];
    char *file_name     = 0;
    obs_object_info object_info;
    obs_options option;
    obs_get_conditions *get_conditions = NULL;
    
    memset_s(&object_info, sizeof(object_info),0, sizeof(obs_object_info));
    object_info.key =key;
    printf("Bucket's name is == %s, object's name is == %s. \n", bucket_name, key);

    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    server_side_encryption_params encryption_params;
    memset_s(&encryption_params,sizeof(encryption_params), 0, sizeof(server_side_encryption_params));

    char* buffer = "K7QkYpBkM5+hcs27fsNkUnNVaobncnLht/rCB2o/9Cw=";

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, FILENAME_PREFIX, FILENAME_PREFIX_LEN)) {
            file_name = &(param[FILENAME_PREFIX_LEN]);
            printf("file_name is: %s\n", file_name);
        }
        else if (!strncmp(param, IF_MODIFIED_SINCE_PREFIX, IF_MODIFIED_SINCE_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->if_modified_since = parseIso8601Time(&(param[IF_MODIFIED_SINCE_PREFIX_LEN]));
        }
        else if (!strncmp(param, IF_NOT_MODIFIED_SINCE_PREFIX, IF_NOT_MODIFIED_SINCE_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->if_not_modified_since = parseIso8601Time(&(param[IF_NOT_MODIFIED_SINCE_PREFIX_LEN]));
        }
        else if (!strncmp(param, IF_MATCH_PREFIX, IF_MATCH_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->if_match_etag = &(param[IF_MATCH_PREFIX_LEN]);
        }
        else if (!strncmp(param, IF_NOT_MATCH_PREFIX, IF_NOT_MATCH_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->if_not_match_etag = &(param[IF_NOT_MATCH_PREFIX_LEN]);
        }
        else if (!strncmp(param, START_BYTE_PREFIX, START_BYTE_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->start_byte = convertInt(&(param[START_BYTE_PREFIX_LEN]), "start_byte");
        }
        else if (!strncmp(param, BYTE_COUNT_PREFIX, BYTE_COUNT_PREFIX_LEN)) {
            if (!get_conditions)
            {
                get_conditions = malloc_init_condition();
            }
            get_conditions->byte_count = convertInt(&(param[BYTE_COUNT_PREFIX_LEN]), "byte_count");
        }
        else if (!strncmp(param, VERSIONID_PREFIX, VERSIONID_PREFIX_LEN)) {
            object_info.version_id = &(param[VERSIONID_PREFIX_LEN]);
        }
        else if (!strncmp(param, IMAGE_PROCESS, IMAGE_PROCESS_LEN)) {
            set_image_proc(&(param[IMAGE_PROCESS_LEN]), get_conditions);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {  
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS; 
        }
        else if (!strncmp(param, USE_KMS, USE_KMS_LEN)) {
            encryption_params.encryption_type = OBS_ENCRYPTION_KMS;
            if(demoUseObsApi == OBS_USE_API_OBS) {
                encryption_params.kms_server_side_encryption = "kms";
            } else {
                encryption_params.kms_server_side_encryption = "aws:kms";
            }
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, USE_SSEC, USE_SSEC_LEN)) {
            encryption_params.encryption_type = OBS_ENCRYPTION_SSEC;
            encryption_params.ssec_customer_algorithm = "AES256";
            encryption_params.ssec_customer_key = buffer;
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        //tmp auth
        else if (!strncmp(param, TMP_AUTH_EXPIRES_PREFIX,TMP_AUTH_EXPIRES_PREFIX_LEN)){
            temp_auth_configure tempauth;
            tempAuthResult  ptrResult;
            memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));
            tempauth.callback_data = (void *)(&ptrResult);
            int auth_expire = atoi(&param[TMP_AUTH_EXPIRES_PREFIX_LEN]);
            tempauth.expires = auth_expire;
            tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
            option.temp_auth = &tempauth;
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    get_object_callback_data callback_data = {0};
    callback_data.outfile = write_to_file(file_name);
    obs_get_object_handler getobjectHandler =
    { 
        { &responsePropertiesCallback,
          &get_object_complete_callback },
        &get_object_data_callback
    };
    
    get_object(&option, &object_info, get_conditions, &encryption_params, &getobjectHandler, &callback_data);  
    if (OBS_STATUS_OK == callback_data.ret_status) {
        printf("get object successfully. \n");
    }
    else
    {
        printf("get object faied(%s).\n", obs_get_status_name(callback_data.ret_status));
    }
    
    fclose(callback_data.outfile);
    if(get_conditions)
    { 
        free(get_conditions->image_process_config);
        free(get_conditions);
    }
}

static void test_set_notification_configuration_new(int argc, char **argv, int optindex)
{
    obs_status  ret_status = OBS_STATUS_BUTT;
    obs_options option;
    char *bucket_name = argv[optindex++];  
    printf("Bucket's name is == %s. \n", bucket_name);

    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style     = gDefaultURIStyle;

    char *delim = ",";
    char *topic_delim = "-";
    obs_smn_notification_configuration notification_conf; 
    obs_smn_topic_configuration topic_conf[10];
    obs_smn_event_enum topic_event[10][10];
    obs_smn_filter_rule filter_rule[10][10];
    int event_num = 0;
    int filter_rule_num = 0;
    int topic_num = 0;
    int i = 0;
    char *tmp[10] = {0};

    memset_s(topic_event,sizeof(topic_event), 0, sizeof(obs_smn_event_enum)*10*10);
    memset_s(filter_rule,sizeof(filter_rule),0, sizeof(obs_smn_filter_rule)*10*10);
    
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, ID_PREFIX, ID_PREFIX_LEN))
        {
            char *id = &param[ID_PREFIX_LEN];
            topic_num = 0;
            char* p=strtok(id,topic_delim);
            while(p!=NULL){
               topic_conf[topic_num++].id = p;
               p=strtok(NULL,topic_delim);
            }
        }
        else if (!strncmp(param, TOPIC_PREFIX, TOPIC_PREFIX_LEN))
        {
            char *topic = &param[TOPIC_PREFIX_LEN];
            topic_num = 0;
            char* p=strtok(topic,topic_delim);
            while(p!=NULL){
               topic_conf[topic_num++].topic = p;
               p=strtok(NULL,topic_delim);
            }
         
        }
        else if (!strncmp(param, EVENTS_PREFIX, EVENTS_PREFIX_LEN))
        {
            char *group_event = &param[EVENTS_PREFIX_LEN];
            topic_num = 0;
            char *event = strtok(group_event,topic_delim);
            while(event != NULL){ 
                topic_conf[topic_num].event = topic_event[topic_num];
                tmp[topic_num] = event;
                topic_num ++;
                event = strtok(NULL,topic_delim);
            }
            
            for(i = 0; i < topic_num; i++)
            {
                event_num = 0;
                event = tmp[i];
                char* p=strtok(event,delim);
                while(p!=NULL){
                    topic_conf[i].event[event_num++] = atoi(p);
                    p=strtok(NULL,delim);
                }   
                topic_conf[i].event_num = event_num;
            }
                        
        }
        else if (!strncmp(param, NAME_PREFIX, NAME_PREFIX_LEN))
        {
            char *group_name = &param[NAME_PREFIX_LEN];
            topic_num = 0;
            char *name = strtok(group_name,topic_delim);
            while(name != NULL){ 
                topic_conf[topic_num].filter_rule = filter_rule[topic_num];
                tmp[topic_num] = name;
                topic_num ++;
                name = strtok(NULL,topic_delim); 
            }

            for(i = 0; i < topic_num; i++)
            {
                filter_rule_num = 0;
                name = tmp[i];
                char* p=strtok(name,delim);
                while(p!=NULL){
                   topic_conf[i].filter_rule[filter_rule_num++].name = atoi(p);
                   p=strtok(NULL,delim);
                }
            }
        }
        else if (!strncmp(param, VALUE_PREFIX, VALUE_PREFIX_LEN))
        {
            char *group_value = &param[VALUE_PREFIX_LEN];
            topic_num = 0;
            char *value = strtok(group_value,topic_delim);
            while(value != NULL){
                topic_conf[topic_num].filter_rule = filter_rule[topic_num];
                tmp[topic_num] = value;
                topic_num ++;
                value = strtok(NULL,topic_delim);    
            }

            for(i = 0; i < topic_num; i++)
            {
                filter_rule_num = 0;
                value = tmp[i];
                char* p=strtok(value,delim);
                while(p!=NULL){
                    topic_conf[i].filter_rule[filter_rule_num++].value = p;
                    p=strtok(NULL,delim);
                }
                topic_conf[i].filter_rule_num = filter_rule_num;
            }
        }
        else if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }
    
    memset_s(&notification_conf,sizeof(notification_conf), 0, sizeof(obs_smn_notification_configuration)); 
    notification_conf.topic_conf = topic_conf; 
    notification_conf.topic_conf_num = topic_num;

    set_notification_configuration(&option, &notification_conf, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set_notification_configuration successfully.\n");
    }
    else {
        printf("set_notification_configuration failed(%s).\n",
                    obs_get_status_name(ret_status));
    }
}

static void test_get_notification_configuration_new(int argc, char **argv, int optindex)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    char *bucket_name = argv[optindex++];  
    printf("Bucket's name is == %s. \n", bucket_name);
    
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style     = gDefaultURIStyle;
    
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, USE_OBS_AUTH,USE_OBS_AUTH_LEN)){
            option.request_options.auth_switch = OBS_OBS_TYPE;
        }
        else if (!strncmp(param, USE_S3_AUTH,USE_S3_AUTH_LEN)){
            option.request_options.auth_switch = OBS_S3_TYPE;
        }
    }
    
    obs_smn_handler notification_handler_info =
    {
        {&response_properties_callback, &response_complete_callback},
        &get_notification_info_callback
    };

    get_notification_configuration(&option, &notification_handler_info, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("get_notification_config successfully.\n");
    }
    else{
        printf("get_notification_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}

/* posix add new func */
static void test_modify_object_new(int argc, char **argv, int optindex)
{
    char *file_name = 0;
    uint64_t content_length = 0;
    obs_options option;
    char *bucket_name = argv[optindex++];
    char *key = argv[optindex++];
    char *body = 0;
    uint64_t position = 0;
    
    printf("Bucket's name is == %s, object's name is == %s. \n", bucket_name, key);
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_put_properties put_properties;
    init_put_properties(&put_properties);


    //check parameters
    while (optindex < argc) {
        char *param = argv[optindex ++];
        if (!strncmp(param, FILENAME_PREFIX, FILENAME_PREFIX_LEN)) {
            file_name = &(param[FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_LENGTH_PREFIX, CONTENT_LENGTH_PREFIX_LEN)) {
            content_length = convertInt(&(param[CONTENT_LENGTH_PREFIX_LEN]), "content_length");
        }
        else if (!strncmp(param, BODY_PREFIX, BODY_PREFIX_LEN)) {
            body = &(param[BODY_PREFIX_LEN]);
        }
        else if (!strncmp(param, CACHE_CONTROL_PREFIX, CACHE_CONTROL_PREFIX_LEN)) {
            put_properties.cache_control = &(param[CACHE_CONTROL_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_TYPE_PREFIX, CONTENT_TYPE_PREFIX_LEN)) {
            put_properties.content_type = &(param[CONTENT_TYPE_PREFIX_LEN]);
        }
        else if (!strncmp(param, MD5_PREFIX, MD5_PREFIX_LEN)) {
            put_properties.md5 = &(param[MD5_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_DISPOSITION_FILENAME_PREFIX, CONTENT_DISPOSITION_FILENAME_PREFIX_LEN)) {
            put_properties.md5 = &(param[CONTENT_DISPOSITION_FILENAME_PREFIX_LEN]);
        }
        else if (!strncmp(param, CONTENT_ENCODING_PREFIX, CONTENT_ENCODING_PREFIX_LEN)) {
            put_properties.content_encoding = &(param[CONTENT_ENCODING_PREFIX_LEN]);
        }
        else if (!strncmp(param, EXPIRES_PREFIX, EXPIRES_PREFIX_LEN)) {
            put_properties.expires = parseIso8601Time(&(param[EXPIRES_PREFIX_LEN]));
        }
        else if (!strncmp(param, START_BYTE_PREFIX, START_BYTE_PREFIX_LEN)) {
            put_properties.start_byte = convertInt(&(param[START_BYTE_PREFIX_LEN]), "start_byte");
        }
        else if (!strncmp(param, X_AMZ_META_PREFIX, X_AMZ_META_PREFIX_LEN)) {
            char *name = param;
            char *value = name;
            
             while (*value && (*value != '=')) {
                value ++;
             }
            *value = 0;
            value++;
            if (!put_properties.meta_data)
            {
                put_properties.meta_data = (obs_name_value *) malloc(sizeof(obs_name_value)*50);
                memset_s(put_properties.meta_data, sizeof(put_properties.meta_data),0, sizeof(obs_name_value)*50);
            }
            put_properties.meta_data[put_properties.meta_data_count].name = name;
            put_properties.meta_data[put_properties.meta_data_count++].value = value;
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            put_properties.canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) {      
            option.bucket_options.certificate_info = ca_info;       
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }
        else if (!strncmp(param, OBJECT_POSITION, OBJECT_POSITION_LEN)) {
            position = convertInt(&(param[OBJECT_POSITION_LEN]), "modify_position");;
        }
        else {
            fprintf(stderr, "\nERROR: Unknown param: %s\n", param);
        }
    }

    //read from local file to buffer
    put_object_callback_data data;
    memset_s(&data,sizeof(data), 0, sizeof(put_object_callback_data));

    data.infile = 0;
    data.gb = 0;
    data.noStatus = 1;

    if (file_name) {
        if (!content_length) {
            content_length =  read_bytes_from_file(file_name, &data);
        }
        //Open the file
        if (!(data.infile = fopen(file_name, "rb" FOPEN_EXTRA_FLAGS))) {
            fprintf(stderr, "\nERROR: Failed to open input file %s: ", file_name);
            return;
        }
    }
    else 
    {
        if (!content_length) {
            while (1) {
                int amtRead = strlen(body);
                if (amtRead == 0) {
                    break;
                }
                growbuffer_append(&(data.gb), body, amtRead);
                content_length += amtRead;
                if (amtRead <= (int) (strlen(body))) {
                    break;
                }
            }
        }
        else {
            growbuffer_append(&(data.gb), body, content_length);
        }
    }

    data.content_length = data.originalContentLength = content_length;
    
    obs_modify_object_handler putobjectHandler =
    { 
        { &responsePropertiesCallback,
          &put_object_complete_callback},
          &put_object_data_callback
    };
    
    modify_object(&option,key,content_length,position,&put_properties, 0, &putobjectHandler,&data);

    if (OBS_STATUS_OK == data.put_status) {
        printf("modify object [%s,%s] successfully. \n", bucket_name,key);
    }
    else
    {
        printf("modify object [%s,%s] faied(%s).\n", bucket_name,key,
            obs_get_status_name(data.put_status));
    }

    if (put_properties.meta_data)
    {
        free(put_properties.meta_data);
    }
}

void test_rename_object_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    char *source_key  = argv[optindex++];
    char *target_key  = argv[optindex++];
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
        }
    }
    obs_response_handler responseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };
    rename_object(&option, source_key, target_key, &responseHandler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test_rename_object  successfully. \n");
    }
    else
    {
        printf("test_rename_object failed(%s).\n", obs_get_status_name(ret_status));
    }
    return;
}
void test_truncate_object_new(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    char *key  = argv[optindex++];
    uint64_t object_length = convertInt(argv[optindex++], "object_length");
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    while (optindex < argc) 
    {
        char *param = argv[optindex ++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) 
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
        else if (!strncmp(param, CERTIFICATE_INFO_PREFIX, CERTIFICATE_INFO_PREFIX_LEN)) 
        {
            option.bucket_options.certificate_info = ca_info;       
        }
    }
    obs_response_handler responseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };
    truncate_object(&option, key, object_length, &responseHandler, &ret_status); 
    if (OBS_STATUS_OK == ret_status) {
        printf("truncate_object %s for length %lu successfully.\n", key, object_length);
    }
    else {
        printf("truncate_object %s for length %lu failed(%s).\n", key, object_length,
            obs_get_status_name(ret_status));
    }
    return;
}


int main(int argc, char **argv)
{
    while (1) {
        int idx = 0;
        int c = getopt_long(argc, argv, "fhusr:", longOptionsG, &idx);
        if (c == -1) {
            //End of options
            break;
        }

        switch (c) {
            case 'f':
                forceG = 1;
                break;
            case 'h':
                printf("-h options\n");
                break;
            case 'u':
                protocolG = OBS_PROTOCOL_HTTPS;
                printf("protocol is: %d\n", protocolG);
                break;
            case 's':
                printf("-s options\n");
                break;
            case 'r':
                printf("-r options\n");
                break;
            default:
                fprintf(stderr, "\nERROR: Unknown option: -%c\n", c);
        }
    }
    
    strcpy_s(ACCESS_KEY_ID,sizeof(ACCESS_KEY_ID), getenv("ACCESS_KEY_ID"));
    strcpy_s(SECRET_ACCESS_KEY, sizeof(SECRET_ACCESS_KEY),getenv("SECRET_ACCESS_KEY"));      
    strcpy_s(HOST_NAME, sizeof(HOST_NAME),getenv("OBS_TEST_HOSTNAME")); 
    strcpy_s(locationconstraint,sizeof(locationconstraint),"R1");   //location
    get_certificate_info(ca_info, sizeof(ca_info));
    
    /*--------------check argv-------------*/
    if (optind == argc) {
        printf("optind = %d\n", optind);
        printf("argc = %d\n", argc);
        fprintf(stderr, "\n\nERROR: Missing argument: command\n\n");
    }

    const char *command = argv[optind++];
    printf("command = %s\n", command);

    set_obs_log_path("/var/log/OBS_SDK_C", false);                   //此行代码用于示例设置日志文件路径功能，在实际使用中请注释该行
    obs_initialize(OBS_INIT_ALL);
    
    initialize_break_point_lock();

    // bucket test
    if (!strcmp(command, "create_bucket")) {
         test_create_bucket_new(argc, argv, optind);
    }
     // posix add func with create posix bucket.
    else if (!strcmp(command, "create_pfs_bucket")) {
         test_create_pfs_bucket_new(argc, argv, optind);
    }
    else if (!strcmp(command, "list_bucket")) {
        test_list_bucket_new(argc, argv, optind);
    }
    else if (!strcmp(command, "delete_bucket")) {
        test_delete_bucket_new(argc, argv, optind);
    }
    else if (!strcmp(command, "head_bucket")) {
        test_head_bucket_new(argc, argv, optind);
    }
    else if (!strcmp(command, "list_bucket_object")) {
        test_list_bucket_object_new(argc, argv, optind);
    }
    else if (!strcmp(command, "list_versions")) {
        test_list_versions_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_policy")) {
        test_set_bucket_policy(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_policy")) {
        test_get_bucket_policy(argc, argv, optind);
    }
    else if (!strcmp(command, "delete_bucket_policy")) {
        test_delete_bucket_policy(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_lifecycle")) {
        test_set_bucket_lifecycle_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_lifecycle")){
        test_get_lifecycle_config_new(argc, argv, optind);
    }
    else if (!strcmp(command, "del_bucket_lifecycle")){
        test_delete_lifecycle_config_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_version")){
        test_set_bucket_version_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_version")){
        test_get_bucket_version_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_storage_info")){
        test_get_bucket_storage_info_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_acl_byhead")){
        test_set_bucket_acl_byhead_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_acl")){
        test_get_bucket_acl_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_acl")){
        test_set_bucket_acl_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_tagging")){
        test_set_bucket_tagging_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_tagging")){
        test_get_bucket_tagging_new(argc, argv, optind);
    }
    else if (!strcmp(command, "del_bucket_tagging")){
        test_delete_bucket_tagging_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_quota")){
        test_set_bucket_quota_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_quota")){
        test_get_bucket_quota_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_logging")){
        test_set_bucket_logging_with_grant_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_logging")){
        test_get_bucket_logging_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_storage_class")){
        test_set_bucket_storage_class_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_storage_class")){
        test_get_bucket_storage_class_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_website_all")){
        test_set_bucket_website_all_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_website")){
        test_get_bucket_website_new(argc, argv, optind);
    }
    else if (!strcmp(command, "del_bucket_website")){
        test_delete_bucket_website_new(argc, argv, optind);
    }
    else if (!strcmp(command, "bucket_option")) {
        test_bucket_option(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_cors")) {
        test_set_bucket_cors(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_cors")) {
        test_get_cors_config(argc, argv, optind);
    }
    else if (!strcmp(command, "delete_bucket_cors")) {
        test_delete_cors_config(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_website_conf")){
        test_set_bucket_website_conf_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_bucket_notification_conf")){
        test_set_notification_configuration_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_bucket_notification_conf")){
        test_get_notification_configuration_new(argc, argv, optind);
    }
    // object test
    else if (!strcmp(command, "put_object")) {
        test_put_object_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_object")) {
        test_get_object_new(argc, argv, optind);
    }
    else if (!strcmp(command, "delete_object")) {
        test_delete_object_new(argc, argv, optind);
    }
    else if (!strcmp(command, "init_upload_part")) {
        test_init_upload_part_new(argc, argv, optind);
    }
    else if (!strcmp(command, "append_object")) {
        test_append_object_new(argc, argv, optind);
    }
    else if (!strcmp(command, "head_object")) {
        test_head_object_new(argc, argv, optind);
    }
    else if (!strcmp(command, "batch_delete")){
        test_batch_delete_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_object_acl_byhead")){
        test_set_object_acl_byhead_new(argc, argv, optind);
    }
    else if (!strcmp(command, "get_object_acl")){
        test_get_object_acl_new(argc, argv, optind);
    }
    else if (!strcmp(command, "set_object_acl")){
        test_set_object_acl_new(argc, argv, optind);
    }
    else if (!strcmp(command, "object_option")) {
        test_object_option(argc, argv, optind);
    }
    else if (!strcmp(command, "concurrent_upload_part")) {
        test_concurrent_upload_part(argc, argv, optind);
    } 
    else if (!strcmp(command, "concurrent_copy_part")) {
        test_concurrent_copy_part(argc, argv, optind);
    } 
    else if (!strcmp(command, "upload_file")) {
        test_upload_file(argc, argv, optind);
    }
    else if (!strcmp(command, "download_file")) {
        test_download_file(argc, argv, optind);
    }
    else if (!strcmp(command, "restore_object")) {
        test_restore_object(argc, argv, optind);
    }
    else if (!strcmp(command, "copy_object")) {
        test_copy_object(argc, argv, optind);
    }
    else if (!strcmp(command, "put_object_with_encrypt")) {
        test_put_object_with_encrypt(argc, argv, optind);
    }
    else if (!strcmp(command, "get_object_with_encrypt")) {
        test_get_object_with_encrypt(argc, argv, optind);
    }
    /* posix add func */
    else if (!strcmp(command, "modify_object")) {
        test_modify_object_new(argc, argv, optind);
    }
    /* posix add func */
    else if (!strcmp(command, "rename_object")) {
        test_rename_object_new(argc, argv, optind);
    }
    /* posix add func */
    else if (!strcmp(command, "truncate_object")) {
        test_truncate_object_new(argc, argv, optind);
    }
    else if (!strcmp(command, "pause_upload_file")) {
        test_pause_upload_file();
    }

    
    deinitialize_break_point_lock();
    obs_deinitialize();
}


