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
#include <cstdlib>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>

#include "securec.h"
#include "securectype.h"
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <process.h>

#include "eSDKOBS.h"
#include "cJSON.h"

#ifndef SLEEP_UNITS_PER_SECOND
#define SLEEP_UNITS_PER_SECOND 1
#endif

#ifndef SLEEP_UNITS_PER_SECOND_WIN
#define SLEEP_UNITS_PER_SECOND_WIN 1000
#endif



#ifdef _MSC_VER
#define snprintf_s _snprintf_s
#endif


#define SSEC_KEY_MD5_LENGTH 64

static obs_status statusG = OBS_STATUS_OK;

static int  showResponsePropertiesG = 1;
static char errorDetailsG[4096] = { 0 };
static char locationconstraint[2048]={0};
static char ACCESS_KEY_ID[2048]={0};
static char SECRET_ACCESS_KEY[2048]={0};
static char HOST_NAME[2048]={0};
static char BUCKET_NAME[2048]={0};
static char UPLOAD_ID[2048]={0};


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


static void printError()
{
    if (statusG < OBS_STATUS_AccessDenied) {
        fprintf(stderr, "\nERROR: %s\n", obs_get_status_name(statusG));
    }
    else {
        fprintf(stderr, "\nERROR: %s\n", obs_get_status_name(statusG));
        fprintf(stderr, "%s\n", errorDetailsG);
    }
}

FILE * write_to_file(char *localfile)
{
    FILE *outfile = 0;
    if (localfile) {
        struct stat buf;
        if (stat(localfile, &buf) == -1) {
            outfile = fopen(localfile, "wb");
        }
        else {
            outfile = fopen(localfile, "a");
        }
        
        if (!outfile) {
            fprintf(stderr, "\nERROR: Failed to open output file %s: ",
                    localfile);
            perror(0);
            exit(-1);
        }
    }
    else if (showResponsePropertiesG) {
        fprintf(stderr, "\nERROR: get -s requires a file_name parameter\n");
    }
    else {
        outfile = stdout;
    }
    return outfile;
}

static obs_status response_properties_callback(const obs_response_properties *properties, void *callback_data)
{
    if (properties == NULL)
    {
        printf("error! obs_response_properties is null!");
        if(callback_data != NULL)
        {
            obs_sever_callback_data *data = (obs_sever_callback_data *)callback_data;
            printf("server_callback buf is %s ,len is %d",
                data->buffer, data->buffer_len);
            return OBS_STATUS_OK;
        }else {
            printf("error! obs_sever_callback_data is null!");
            return OBS_STATUS_OK;
        }
    }
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
	if (properties->content_length) {                                  
            printf("content_length: %d\n", properties->content_length);          
    }    
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

static uint64_t convertInt(const char *str, const char *paramName)
{
    uint64_t ret = 0;

    while (*str) {
        if (!isdigit(*str)) {
            fprintf(stderr, "\nERROR: Nondigit in %s parameter: %c\n", 
                    paramName, *str);
            // usageExit(stderr);
        }
        ret *= 10;
        ret += (*str++ - '0');
    }

    return ret;
}


static void response_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    (void) callback_data;

    if (callback_data)
    {
        obs_status *ret_status = (obs_status *)callback_data;
        *ret_status = status;
    }
    else
    {
        statusG = status;
    }
    int len = 0;
    if (error && error->message) {
        len += snprintf_s(&(errorDetailsG[len]), sizeof(errorDetailsG) - len, _TRUNCATE,
                        "  Message: %s\n", error->message);
    }
    if (error && error->resource) {
        len += snprintf_s(&(errorDetailsG[len]), sizeof(errorDetailsG) - len, _TRUNCATE,
                        "  Resource: %s\n", error->resource);
    }
    if (error && error->further_details) {
        len += snprintf_s(&(errorDetailsG[len]), sizeof(errorDetailsG) - len, _TRUNCATE,
                        "  Further Details: %s\n", error->further_details);
    }
    if (error && error->extra_details_count) {
        len += snprintf_s(&(errorDetailsG[len]), sizeof(errorDetailsG) - len, _TRUNCATE,
                        "%s", "  Extra Details:\n");
        int i;
        for (i = 0; i < error->extra_details_count; i++) {
            len += snprintf_s(&(errorDetailsG[len]), 
                            sizeof(errorDetailsG) - len,  _TRUNCATE, "    %s: %s\n", 
                            error->extra_details[i].name,
                            error->extra_details[i].value);
        }
    }
}
// head object ---------------------------------------------------------------
static void test_head_object(int argc, char **argv, int optindex)
{
    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    char *key = argv[optindex++];
    printf("key is: %s\n", key);


    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
        
    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };

    obs_head_object(&option,key, &response_handler, &ret_status);

    if (statusG == OBS_STATUS_OK) {
        printf("head bucket successfully. \n");
    }
    else {
        printf("head bucket failed(%s).\n", obs_get_status_name(ret_status));
    }

}

void print_grant_info(int acl_grant_count,obs_acl_grant *acl_grants)
{
    
    int i;
    for (i = 0; i < acl_grant_count; i++) 
    {
        obs_acl_grant *grant = acl_grants + i;
        const char *type;
        char composedId[OBS_MAX_GRANTEE_USER_ID_SIZE + 
                        OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE + 16] = {0};
        const char *id;

        switch (grant->grantee_type) {
        case OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL:
            type = "Email";
            id = grant->grantee.huawei_customer_by_email.email_address;
            break;
        case OBS_GRANTEE_TYPE_CANONICAL_USER:
            type = "UserID";
			snprintf_s(composedId, sizeof(composedId), sizeof(composedId) - 1,
                    "%s (%s)", grant->grantee.canonical_user.id,
                    grant->grantee.canonical_user.display_name);
            id = composedId;
            break;
        case OBS_GRANTEE_TYPE_ALL_OBS_USERS:
            type = "Group";
            id = "Authenticated AWS Users";
            break;
        default:
            type = "Group";
            id = "All Users";
            break;
        }
        const char *perm;
        switch (grant->permission) {
        case OBS_PERMISSION_READ:
            perm = "READ";
            break;
        case OBS_PERMISSION_WRITE:
            perm = "WRITE";
            break;
        case OBS_PERMISSION_READ_ACP:
            perm = "READ_ACP";
            break;
        case OBS_PERMISSION_WRITE_ACP:
            perm = "WRITE_ACP";
            break;
        default:
            perm = "FULL_CONTROL";
            break;
        }
        printf("%-6s  %-90s  %-12s\n", type, id, perm);
    }
}

//set object metadata---------------------------------------------------------------
static void test_set_object_metadata(char* bucket_name, char * key, char *version_id)
{
	obs_canned_acl canned_acl = OBS_CANNED_ACL_PRIVATE;
	char *content_encoding = NULL;
	char *expires = NULL;

	obs_object_info objectinfo;
	memset_s(&objectinfo,sizeof(objectinfo),0,sizeof(obs_object_info));
	objectinfo.key=key;
	objectinfo.version_id=version_id;
	// init struct obs_option
	obs_options option;
	obs_status ret_status = OBS_STATUS_BUTT;
	init_obs_options(&option);

	option.bucket_options.host_name = HOST_NAME;
	option.bucket_options.bucket_name = bucket_name;
	option.bucket_options.access_key = ACCESS_KEY_ID;
	option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
	option.bucket_options.uri_style = OBS_URI_STYLE_VIRTUALHOST;
	option.request_options.auth_switch = OBS_OBS_TYPE;


	obs_response_handler response_handler =
	{ 
		&response_properties_callback,
		&response_complete_callback
	};


	server_side_encryption_params encryption_params;
	memset_s(&encryption_params, sizeof(encryption_params),0, sizeof(server_side_encryption_params));


	obs_put_properties put_properties;
	init_put_properties(&put_properties);
	put_properties.canned_acl = canned_acl;
	put_properties.content_encoding = content_encoding;
	put_properties.content_type = "text/json";
	put_properties.metadata_action = OBS_REPLACE_NEW;

	set_object_metadata(&option,&objectinfo,&put_properties, NULL, &response_handler,0);

	if (statusG == OBS_STATUS_OK) {
		printf("set object metadata  successfully. \n");
	}
	else
	{
		printf("set object metadata  failed.\n");
		printError();
	}
}



// get object metadata---------------------------------------------------------------
static void test_get_object_metadata(char * key, char *version_id)
{

    obs_object_info objectinfo;
    memset_s(&objectinfo,sizeof(objectinfo),0,sizeof(obs_object_info));
    objectinfo.key=key;
    objectinfo.version_id=version_id;
    // init struct obs_option
    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };

    get_object_metadata(&option,&objectinfo,0, &response_handler,0);

    if (statusG == OBS_STATUS_OK) {
        printf("get object metadata  successfully. \n");
    }
    else
    {
        printf("get object metadata  failed.\n");
        printError();
    }
}
typedef struct __tempAuthResult
{
    char tmpAuthUrl[1024];
    char actualHeaders[1024];
}tempAuthResult;

void tempAuthCallBack_getResult(char *tempAuthUrl, uint64_t tempAuthUrlLen, char *tempAuthActualHeaders,
    uint64_t tempAuthActualHeadersLen, void *callbackData)
{
    int urlLen = 0;
    tempAuthResult * ptrResult = (tempAuthResult *)callbackData;
    urlLen = strlen(tempAuthUrl);
    snprintf_s(ptrResult->tmpAuthUrl,1024,1023,"%s",tempAuthUrl);
    snprintf_s(ptrResult->actualHeaders,1024,1023,"%s",tempAuthActualHeaders);

    if((tempAuthUrl!=NULL)&&(strlen(tempAuthUrl) != 0))
    {

        const char *cfileName = "tmpAuthUrl.txt";
        FILE * fpoutfile = NULL;

        if (cfileName)
        {
            struct stat buf;
            if (stat(cfileName, &buf) == -1) 
            {
                fpoutfile = fopen(cfileName, "w");
            }
            else 
            {
                fpoutfile = fopen(cfileName, "a");
            }

            if(fpoutfile == NULL)
            {
                fprintf(stderr, "open %s failed", cfileName);
                exit(-1);
            }
            fwrite("\n",1,1,fpoutfile);
            fwrite(ptrResult->tmpAuthUrl,strlen(ptrResult->tmpAuthUrl),1,fpoutfile);

            fwrite("\n",1,1,fpoutfile);

            fwrite(ptrResult->actualHeaders,strlen(ptrResult->actualHeaders),1,fpoutfile);

            fclose(fpoutfile);
        }
    }
}

// create bucket ---------------------------------------------------------------
static void test_create_bucket(int argc, char **argv, int optindex)
{
    char *location = NULL;
    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_canned_acl canned_acl = OBS_CANNED_ACL_PRIVATE;
    temp_auth_configure tempauth;
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));
    obs_status status = OBS_STATUS_OK;

	if (optindex == argc) {
        fprintf(stderr, "\nERROR: Missing parameter: bucket\n");
    }
	char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);
	
    init_obs_options(&option);

    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, LOCATION_PREFIX, LOCATION_PREFIX_LEN)) {
            location = &(param[LOCATION_PREFIX_LEN]);
            printf("locationconstrint is: %s\n", location);
        }
        else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN)) {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }
        else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
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

    create_bucket(&option, canned_acl, location, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("create Bucket [%s] successfully. \n", bucket_name);
    }
    else
    {
        printf("create Bucket [%s] failed.\n", bucket_name);
    }
}

// create bucket ---------------------------------------------------------------
static void test_create_bucket_with_option(obs_canned_acl canned_acl, char *bucket_name,
                                   obs_storage_class storage_class_value, char *bucket_region)
{
    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.certificate_info = NULL;

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    option.bucket_options.storage_class = storage_class_value;
    create_bucket(&option, canned_acl, bucket_region, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("create bucket with option successfully. \n");
    }
    else
    {
        printf("create bucket with option failed(%s).\n", obs_get_status_name(ret_status));
    }

}



// delete bucket ---------------------------------------------------------------
static void test_delete_bucket(int argc, char **argv, int optindex)
{
    char *location = NULL;
    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_canned_acl canned_acl = OBS_CANNED_ACL_PRIVATE;
    temp_auth_configure tempauth;
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));
    obs_status status = OBS_STATUS_OK;

    if (optindex == argc) {
        fprintf(stderr, "\nERROR: Missing parameter: bucket\n");
    }
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);
    
    init_obs_options(&option);

    while (optindex < argc) {
        char *param = argv[optindex++];
    }
    

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        NULL,
        &response_complete_callback
    };

    delete_bucket(&option, &response_handler, &ret_status);

    if (ret_status == OBS_STATUS_OK) {
        printf("delete bucket successfully. \n");
    }
    else
    {
        printf("delete bucket failed(%s).\n", obs_get_status_name(ret_status));
    }
}

// set bucket quota ------------------------------------------------------
static void test_set_bucket_quota( char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;   
    uint64_t bucketquota = 104857600;

    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

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
static void test_get_bucket_quota(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    
    uint64_t bucketquota = 0;
    get_bucket_quota(&option, &bucketquota,&response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("Bucket=%s  Quota=%d\n get bucket quota successfully. \n ",bucket_name,bucketquota);
    }
    else
    {
        printf("get bucket quota failed(%s).\n", obs_get_status_name(ret_status));
    }

}

//set bucket policy
void test_set_bucket_policy(char *bucket_name)
{
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };

    char bucket_policy[1024] = {0};
    sprintf_s(bucket_policy,sizeof(bucket_policy), 
        "{\"Version\":\"2008-10-17\",\"Id\":\"111\","
        "\"Statement\":[{\"Sid\": \"AddPerm\", \"Action\": [\"s3:GetObject\" ]," 
        "\"Effect\": \"Allow\",\"Resource\": \"arn:aws:s3:::%s/*\",\"Principal\":\"*\"} ] }", 
        bucket_name);
    set_bucket_policy(&option, bucket_policy, &response_handler,0);
    
    if (statusG == OBS_STATUS_OK) {
        printf("set bucket policy successfully. \n");
    }
    else
    {
        printf("set bucket policy failed.\n");
        printError();
    }

}

//get bucket policy
void test_get_bucket_policy(char *bucket_name)
{
    obs_options *option;
    option = (obs_options *)malloc(sizeof(obs_options));
    init_obs_options(option);

    option->bucket_options.host_name = HOST_NAME;
    option->bucket_options.bucket_name = bucket_name;
    option->bucket_options.access_key = ACCESS_KEY_ID;
    option->bucket_options.secret_access_key = SECRET_ACCESS_KEY;

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    char policy[1024]="";
    get_bucket_policy(option, sizeof(policy), policy, &response_handler, 0);
    
    if (statusG == OBS_STATUS_OK) {
        printf("get bucket policy successfully.\n policy=(%s)\n",policy);
    }
    else
    {
        printf("get bucket policy failed.\n");
        printError();
    }

    free(option);
}

//delete bucket policy
void test_delete_bucket_policy(char *bucket_name)
{
    obs_options *option;
    option = (obs_options *)malloc(sizeof(obs_options));
    init_obs_options(option);

    option->bucket_options.host_name = HOST_NAME;
    option->bucket_options.bucket_name = bucket_name;
    option->bucket_options.access_key = ACCESS_KEY_ID;
    option->bucket_options.secret_access_key = SECRET_ACCESS_KEY;

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    delete_bucket_policy(option, &response_handler,0);
    
    if (statusG == OBS_STATUS_OK) {
        printf("delete bucket policy successfully.\n");
    }
    else
    {
        printf("delete bucket policy failed.\n");
        printError();
    }

    free(option);
}

// set bucket version ------------------------------------------------------
static void test_set_bucket_version(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    char *status = argv[optindex++];
    if (strcmp(status, OBS_VERSION_STATUS_ENABLED) && strcmp(status, OBS_VERSION_STATUS_SUSPENDED))
    {
        printf("Do not support %s\n", status);
        exit(-1);
    }

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
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

//get bucket version
void test_get_bucket_version(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    char status[OBS_COMMON_LEN_256] = {0};
    get_bucket_version_configuration(&option, sizeof(status), status, 
        &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("get bucket version successfully.\n policy=(%s)\n", status);
    }
    else
    {
        printf("get bucket version failed(%s).\n", obs_get_status_name(ret_status));
    }

}

// set bucket storage class ------------------------------------------------------
static void test_set_bucket_storage_class(char *bucket_name, obs_storage_class storage_class_policy)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    set_bucket_storage_class_policy(&option, storage_class_policy, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket storage class successfully. \n");
    }
    else
    {
        printf("set bucket storage class failed(%s).\n", obs_get_status_name(ret_status));
    }

}

obs_status getBucketStoragePolicyHandler(const char * storage_class, void * callBackData)
{
    printf("Bucket storage class is: %s\n",storage_class);
    return OBS_STATUS_OK;
}

//get bucket storage class policy
static void test_get_bucket_storage_class(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    obs_get_bucket_storage_class_handler getBucketStorageResponse = {response_handler,&getBucketStoragePolicyHandler};

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

// set bucket tagging ------------------------------------------------------
static void test_set_bucket_tagging(int argc, char** argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option; 
    init_obs_options(&option);

    char* bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    char tagKey[][OBS_COMMON_LEN_256] = {{"k1"},{"k2"},{"k3"},{"k4"},{"k5"},{"k6"},{"k7"},{"k8"},{"k9"},{"k10"}};
    char tagValue[][OBS_COMMON_LEN_256] = {{"v1"},{"v2"},{"v3"},{"v4"},{"v5"},{"v6"},{"v7"},{"v8"},{"v9"},{"v10"}};
    obs_name_value tagginglist[10] = {0};
    int i=0;
    for(;i<10;i++)
    {
         tagginglist[i].name = tagKey[i];
         tagginglist[i].value  = tagValue[i];
    } 
    
    set_bucket_tagging(&option, tagginglist, 8, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket tagging successfully. \n");
    }
    else
    {
        printf("set bucket tagging failed(%s).\n", obs_get_status_name(ret_status));
    }

}

typedef struct tagkv
{
    char key[250];
    char value[250];
}tagkv;

typedef struct TaggingInfo
{
    int tagCount;
    tagkv taglist[10];
    obs_status ret_status;
}TaggingInfo;

void printTagInfo(TaggingInfo* infoToPrint)
{
    int i;
    printf("etag number is %d\n",infoToPrint->tagCount);

    for(i=0;i<infoToPrint->tagCount;i++)
    {
        printf("key:[%s], value[%s]\n",infoToPrint->taglist[i].key, infoToPrint->taglist[i].value);
    }
}

obs_status get_bucket_tagging_callback(int tagging_count, obs_name_value *tagging_list, void *callback_data)
{
    int tag_num = 0;

    TaggingInfo * tag_info = (TaggingInfo *)callback_data;

    tag_info->tagCount =  tagging_count;

    if(tagging_count > 0 )
    { 
        for(tag_num=0;tag_num<tagging_count;tag_num++)
        {
            memcpy_s(tag_info->taglist[tag_num].key, sizeof(tag_info->taglist[tag_num].key),(&tagging_list[tag_num])->name,strlen((&tagging_list[tag_num])->name)+1);
            memcpy_s(tag_info->taglist[tag_num].value, sizeof(tag_info->taglist[tag_num].value),(&tagging_list[tag_num])->value,strlen((&tagging_list[tag_num])->value)+1);
        }
    }
    printTagInfo(tag_info);
    return OBS_STATUS_OK;
}

static void get_bucket_tagging_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    TaggingInfo *tagging_info = (TaggingInfo*)callback_data;
    tagging_info->ret_status = status;
}

//get bucket tagging
void test_get_bucket_tagging(int argc, char** argv, int optindex)
{
    char* bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name); 

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

     obs_get_bucket_tagging_handler response_handler = 
    {
         {&response_properties_callback, &get_bucket_tagging_complete_callback}, 
            &get_bucket_tagging_callback
    };

    TaggingInfo tagging_info;
    memset_s(&tagging_info, sizeof(tagging_info),0, sizeof(TaggingInfo));
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
void test_delete_bucket_tagging(int argc, char** argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    char* bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    
    delete_bucket_tagging(&option, &response_handler, &ret_status);
    
    if (statusG == OBS_STATUS_OK) {
        printf("delete bucket tagging successfully.\n");
    }
    else
    {
        printf("delete bucket tagging failed(%s).\n", obs_get_status_name(ret_status));
    }

}

static void set_log_delivery_acl(char *bucket_name_target)
{
    obs_options *option;
    
    option = (obs_options *)malloc(sizeof(obs_options));
    init_obs_options(option);

    option->bucket_options.host_name = HOST_NAME;
    option->bucket_options.bucket_name = bucket_name_target;
    option->bucket_options.access_key = ACCESS_KEY_ID;
    option->bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    manager_acl_info aclinfo;
    memset_s(&aclinfo, sizeof(manager_acl_info), 0, sizeof(manager_acl_info));
    
    aclinfo.acl_grants = (obs_acl_grant*)malloc(sizeof(obs_acl_grant)*2);
    strcpy_s(aclinfo.acl_grants->grantee.canonical_user.id, 
		sizeof(aclinfo.acl_grants->grantee.canonical_user.id),"userid1");
    strcpy_s(aclinfo.acl_grants->grantee.canonical_user.display_name, 
		sizeof(aclinfo.acl_grants->grantee.canonical_user.display_name),"name1");
    aclinfo.acl_grants->grantee_type = OBS_GRANTEE_TYPE_LOG_DELIVERY;
    aclinfo.acl_grants->permission = OBS_PERMISSION_WRITE ; 

    strcpy_s((aclinfo.acl_grants + 1)->grantee.canonical_user.id, 
		sizeof((aclinfo.acl_grants + 1)->grantee.canonical_user.id),"userid1");
    strcpy_s((aclinfo.acl_grants + 1)->grantee.canonical_user.display_name, 
		sizeof((aclinfo.acl_grants + 1)->grantee.canonical_user.display_name),"name1");
    (aclinfo.acl_grants + 1)->grantee_type = OBS_GRANTEE_TYPE_LOG_DELIVERY;
    (aclinfo.acl_grants + 1)->permission = OBS_PERMISSION_READ_ACP ; 

    aclinfo.acl_grant_count_return = (int*)malloc(sizeof(int));
    *(aclinfo.acl_grant_count_return) = 2;

    aclinfo.owner_id = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo.owner_id,sizeof(aclinfo.owner_id),0,sizeof(aclinfo.owner_id));
    aclinfo.owner_display_name = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo.owner_display_name,sizeof(aclinfo.owner_display_name),0,sizeof(aclinfo.owner_display_name));
    strcpy_s(aclinfo.owner_id, 
		sizeof(aclinfo.owner_id),"domainiddomainiddomainiddo000400");
    strcpy_s(aclinfo.owner_display_name, 
		sizeof(aclinfo.owner_display_name),"displayname");
    
    memset_s(&aclinfo.object_info,sizeof(aclinfo.object_info),0,sizeof(aclinfo.object_info));
    set_object_acl(option, &aclinfo, &response_handler,0);
    
    if (statusG == OBS_STATUS_OK) {
        printf("set bucket-target(%s) log delivery acl successfully. \n",bucket_name_target);
    }
    else
    {
        printf("set bucket-target(%s) log delivery acl failed.\n",bucket_name_target);
        printError();
    }
    free(aclinfo.acl_grants);
    free(aclinfo.owner_display_name);
    free(aclinfo.owner_id);
    free(aclinfo.acl_grant_count_return);
    free(option);
}


static void test_set_bucket_logging_without_grant(char *bucket_name_src, char *bucket_name_target)
{
    set_log_delivery_acl(bucket_name_target);
    obs_status ret_status = OBS_STATUS_BUTT;
    
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name_src;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
        

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    set_bucket_logging_configuration(&option, bucket_name_target, "prefix-log", 
        NULL, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket(%s) logging successfully. \n",bucket_name_src);
    }
    else
    {
        printf("set bucket logging failed(%s).\n",obs_get_status_name(ret_status));
    }

}

void test_close_bucket_logging(char *bucket_name_src)
{
    
    obs_status ret_status = OBS_STATUS_BUTT;

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name_src;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
        

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    set_bucket_logging_configuration(&option, NULL, NULL, 
        NULL, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("close bucket(%s) logging successfully. \n",bucket_name_src);
    }
    else
    {
        printf("close bucket logging failed(%s).\n",obs_get_status_name(ret_status));
    }
}

void init_bucket_get_logging_message(bucket_logging_message *logging_message)
{
    logging_message->target_bucket = (char *)malloc(sizeof(char)*OBS_MAX_HOSTNAME_SIZE);
    memset_s(logging_message->target_bucket, OBS_MAX_HOSTNAME_SIZE, 0, OBS_MAX_HOSTNAME_SIZE);
    logging_message->target_bucket_size = OBS_MAX_HOSTNAME_SIZE;

    logging_message->target_prefix = (char *)malloc(sizeof(char)*OBS_MAX_KEY_SIZE);
    memset_s(logging_message->target_prefix, OBS_MAX_KEY_SIZE, 0, OBS_MAX_KEY_SIZE);
    logging_message->target_prefix_size = OBS_MAX_KEY_SIZE;

    logging_message->acl_grants = (obs_acl_grant*)malloc(sizeof(obs_acl_grant)*OBS_MAX_ACL_GRANT_COUNT);
    memset_s(logging_message->acl_grants, sizeof(obs_acl_grant)*OBS_MAX_ACL_GRANT_COUNT, 0, sizeof(obs_acl_grant)*OBS_MAX_ACL_GRANT_COUNT);
    logging_message->acl_grant_count = (int *)malloc(sizeof(int));
    *(logging_message->acl_grant_count) = 0;
}

void destroy_logging_message(bucket_logging_message *logging_message)
{
    free(logging_message->target_bucket);
    free(logging_message->target_prefix);
    free(logging_message->acl_grants);
    free(logging_message->acl_grant_count);
}

// get bucket logging
void test_get_bucket_logging(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
     obs_response_handler response_handler = 
    {
         &response_properties_callback, &response_complete_callback
    };
     
    bucket_logging_message logging_message;
    init_bucket_get_logging_message(&logging_message);

    get_bucket_logging_configuration(&option, &response_handler, &logging_message, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) 
    {
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

// set bucket website conf------------------------------------------------------
static void test_set_bucket_website_conf(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_set_bucket_website_conf set_bucket_website_conf; 
    set_bucket_website_conf.suffix = "index.html"; 
    set_bucket_website_conf.key = "Error.html"; 
    set_bucket_website_conf.routingrule_count = 2; 
    bucket_website_routingrule temp[2];
    memset_s(&temp[0], sizeof(temp[0]),0, sizeof(bucket_website_routingrule));
    memset_s(&temp[1], sizeof(temp[1]),0, sizeof(bucket_website_routingrule));
    set_bucket_website_conf.routingrule_info = temp; 
    temp[0].key_prefix_equals = "key_prefix1"; 
    temp[0].replace_key_prefix_with = "replace_key_prefix1"; 
    temp[0].http_errorcode_returned_equals="404"; 
    temp[0].http_redirect_code = NULL; 
    temp[0].host_name = "www.example.com"; 
    temp[0].protocol = "http"; 

    temp[1].key_prefix_equals = "key_prefix2"; 
    temp[1].replace_key_prefix_with = "replace_key_prefix2"; 
    temp[1].http_errorcode_returned_equals="404"; 
    temp[1].http_redirect_code = NULL; 
    temp[1].host_name = "www.xxx.com"; 
    temp[1].protocol = "http"; 

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    set_bucket_website_configuration(&option, NULL, &set_bucket_website_conf, 
        &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket website conf successfully. \n");
    }
    else
    {
        printf("set bucket website conf failed(%s).\n", obs_get_status_name(ret_status));
    }

}
// set bucket website all------------------------------------------------------
static void test_set_bucket_website_all(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_set_bucket_redirect_all_conf set_bucket_redirect_all;

    set_bucket_redirect_all.host_name = "www.example.com";
    set_bucket_redirect_all.protocol = "https";

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    set_bucket_website_configuration(&option, &set_bucket_redirect_all, NULL, 
        &response_handler, &ret_status);
    if (statusG == OBS_STATUS_OK) {
        printf("set bucket website all successfully. \n");
    }
    else
    {
        printf("set bucket website all failed(%s).\n", obs_get_status_name(ret_status));
    }

}

// get bucket website
obs_status get_bucket_websiteconf_callback(const char *hostname,
                                const char *protocol,
                                const char *suffix,
                                const char *key,
                                const bucket_website_routingrule *websiteconf,
                                int webdatacount,
                                void *callback_data)
{
    (void)callback_data;
    int i = 0;
    printf("redirectAllRequestsTo hostname : %s\n", hostname);
    printf("redirectAllRequestsTo protocol : %s\n", protocol);
    printf("suffix : %s\n", suffix);
    printf("key : %s\n", key);
    for(i = 0; i < webdatacount; i++)
    {
        printf("key_prefix_equals : %s\n", websiteconf[i].key_prefix_equals);
        printf("http_errorcode_returned_equals : %s\n", websiteconf[i].http_errorcode_returned_equals);
        printf("replace_key_prefix_with : %s\n", websiteconf[i].replace_key_prefix_with);
        printf("replace_key_with : %s\n", websiteconf[i].replace_key_with);
        printf("http_redirect_code : %s\n", websiteconf[i].http_redirect_code);
        printf("hostname : %s\n", websiteconf[i].host_name);
        printf("protocol : %s\n", websiteconf[i].protocol);
    }
    return OBS_STATUS_OK;
}

void test_get_bucket_website(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
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

//delete bucket website
void test_delete_bucket_website(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

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

void init_acl_info(manager_acl_info *aclinfo)
{
    memset_s(aclinfo, sizeof(manager_acl_info), 0, sizeof(manager_acl_info));

    aclinfo->acl_grants = (obs_acl_grant*)malloc(sizeof(obs_acl_grant)*2);
    strcpy_s(aclinfo->acl_grants->grantee.canonical_user.id, sizeof(aclinfo->acl_grants->grantee.canonical_user.id),"userid1");
    strcpy_s(aclinfo->acl_grants->grantee.canonical_user.display_name, sizeof(aclinfo->acl_grants->grantee.canonical_user.display_name),"name1");
    aclinfo->acl_grants->grantee_type = OBS_GRANTEE_TYPE_LOG_DELIVERY;
    aclinfo->acl_grants->permission = OBS_PERMISSION_WRITE ; 

	strcpy_s((aclinfo->acl_grants + 1)->grantee.canonical_user.id, sizeof(aclinfo->acl_grants + 1)->grantee.canonical_user.id,"userid1");
	strcpy_s((aclinfo->acl_grants + 1)->grantee.canonical_user.display_name, sizeof(aclinfo->acl_grants + 1)->grantee.canonical_user.display_name,"name1");
    (aclinfo->acl_grants + 1)->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
    (aclinfo->acl_grants + 1)->permission = OBS_PERMISSION_READ_ACP ; 

    aclinfo->acl_grant_count_return = (int*)malloc(sizeof(int));
    *(aclinfo->acl_grant_count_return) = 2;

    aclinfo->owner_id = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo->owner_id,sizeof(aclinfo->owner_id),0,sizeof(aclinfo->owner_id));
    aclinfo->owner_display_name = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo->owner_display_name,sizeof(aclinfo->owner_display_name),0,sizeof(aclinfo->owner_display_name));
    strcpy_s(aclinfo->owner_id, sizeof(aclinfo->owner_id),"domainiddomainiddomainiddo000400");   
    strcpy_s(aclinfo->owner_display_name, sizeof(aclinfo->owner_display_name),"displayname");

    memset_s(&aclinfo->object_info,sizeof(aclinfo->object_info),0,sizeof(aclinfo->object_info));
}

void deinitialize_acl_info(manager_acl_info *aclinfo)
{  
    free(aclinfo->acl_grants);
    free(aclinfo->owner_display_name);
    free(aclinfo->owner_id);
    free(aclinfo->acl_grant_count_return);
}
//test set bucket acl
void test_set_bucket_acl(int argc, char** argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    
    char* bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    manager_acl_info aclinfo;
    init_acl_info(&aclinfo);
    
    while(optindex < argc){
        char* param = argv[optindex++];
        if (!strncmp(param, OWNERID_PREFIX, OWNERID_PREFIX_LEN))
        {
            aclinfo.owner_id = &(param[OWNERID_PREFIX_LEN]);
        }else if (!strncmp(param, OWNERID_DISPLAY_NAME_PREFIX, OWNERID_DISPLAY_NAME_PREFIX_LEN)){
            strcpy_s(aclinfo.owner_display_name, sizeof(aclinfo.owner_display_name),&(param[OWNERID_DISPLAY_NAME_PREFIX_LEN]));
        }else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)){
            option.bucket_options.protocol = get_protocol_from_argv(param);   
        }else if (!strncmp(param, user_id_str, user_id_str_length)){
            strcpy_s(aclinfo.acl_grants->grantee.canonical_user.id, sizeof(aclinfo.acl_grants->grantee.canonical_user.id),&(param[user_id_str_length]));
        }else if (!strncmp(param, display_name_str, display_name_length)){
            strcpy_s(aclinfo.acl_grants->grantee.canonical_user.display_name, &(param[display_name_length]));
        }else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)){
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }else if (isdigit(param[0])){
            *(aclinfo.acl_grant_count_return) = convertInt(param, "grant_count");
        }else if (!strncmp(param, permission_str, permission_length)){
            char *permission = &(param[permission_length]);
            int tmp = convertInt(permission, "permission");
            switch(tmp){
                case 0:
                    aclinfo.acl_grants->permission = OBS_PERMISSION_READ;
                    break;

                case 1:
                    aclinfo.acl_grants->permission = OBS_PERMISSION_WRITE;
                    break;
                            
                case 2:
                    aclinfo.acl_grants->permission = OBS_PERMISSION_READ_ACP;
                    break;

                case 3:
                    aclinfo.acl_grants->permission = OBS_PERMISSION_WRITE_ACP;
                    break;

                case 4:
                    aclinfo.acl_grants->permission = OBS_PERMISSION_FULL_CONTROL;
                    break;

                default:
                    aclinfo.acl_grants->permission = OBS_PERMISSION_BUTT;
                    break;
            }
        }else if(!strncmp(param, grantee_type_str, grantee_type_str_length)){
            char *grantee_type = &(param[grantee_type_str_length]);
            int tmp = convertInt(grantee_type, "grantee_type");
            switch(tmp){
                case 0:
                    aclinfo.acl_grants->grantee_type = OBS_GRANTEE_TYPE_HUAWEI_CUSTOMER_BYEMAIL;
                    break;

                case 1:
                    aclinfo.acl_grants->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
                    break;

                case 2:
                    aclinfo.acl_grants->grantee_type = OBS_GRANTEE_TYPE_ALL_OBS_USERS;
                    break;

                case 3:
                    aclinfo.acl_grants->grantee_type = OBS_GRANTEE_TYPE_ALL_USERS;
                    break;

                case 4:
                    aclinfo.acl_grants->grantee_type = OBS_GRANTEE_TYPE_LOG_DELIVERY;
                    break;

                default:
                    aclinfo.acl_grants->grantee_type = OBS_GRANTEE_TYPE_BUTT;
                    break;
            }
        }
    }


    set_bucket_acl(&option, &aclinfo, &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket acl successfully. \n");
    }
    else
    {
        printf("set bucket acl failed(%s).\n", obs_get_status_name(ret_status));
    }
    deinitialize_acl_info(&aclinfo);
}

void test_get_bucket_acl(int argc, char** argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    char* bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    while(optindex < argc){
        char* param = argv[optindex++];
        if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN))
        {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
    }


    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    manager_acl_info *aclinfo = malloc_acl_info();
    
    get_bucket_acl(&option, aclinfo, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status)
    {
        printf("get bucket acl: -------------");
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

void test_set_bucket_acl_byhead(int argc, char** argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option; 
    init_obs_options(&option);
    obs_canned_acl canned_acl = OBS_CANNED_ACL_AUTHENTICATED_READ;


    char *bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    while(optindex < argc){
        char *param = argv[optindex++];
        if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)){
            canned_acl = get_acl_from_argv(param);
        }else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)){
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }
    }

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    set_bucket_acl_by_head(&option, canned_acl, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket acl by head successfully. \n");
    }
    else
    {
        printf("set bucket acl by head failed(%s).\n", obs_get_status_name(ret_status));
    }
}

//test set object acl
void test_set_object_acl(int argc, char** argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    
    char *bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    char *key = argv[optindex++];
    printf("key is: %s\n", key);

    char *version_id = argv[optindex++];
    printf("version_id is: %s\n", version_id);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    manager_acl_info aclinfo;
    init_acl_info(&aclinfo);

    aclinfo.object_info.key = key;
    aclinfo.object_info.version_id = version_id;
    
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

void test_get_object_acl(int argc, char** argv, int optindex)
{
    char* bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    char* key = argv[optindex++];
    printf("key is: %s\n", key);


    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    manager_acl_info *aclinfo = malloc_acl_info();
    aclinfo->object_info.key = key;
    
    while(optindex < argc){
        char* param = argv[optindex++];
        if (!strncmp(param, VERSIONID_PREFIX, VERSIONID_PREFIX_LEN)){
            aclinfo->object_info.version_id = &(param[VERSIONID_PREFIX_LEN]);
        }
    }


    get_object_acl(&option, aclinfo, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status)
    {
        printf("get object acl: -------------");
        printf("%s %s %s %s\n", aclinfo->owner_id, aclinfo->owner_display_name,
            aclinfo->object_info.key, aclinfo->object_info.version_id);
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

void test_set_object_acl_byhead(int argc, char** argv, int optindex)
{
    char* bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);

    char* key = argv[optindex++];
    printf("key is: %s\n", key);


    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option; 
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    obs_canned_acl canned_acl = OBS_CANNED_ACL_AUTHENTICATED_READ;
    obs_object_info object_info;
    object_info.key = key;

    while(optindex < argc){
        char* param = argv[optindex++];
        if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)){
            canned_acl = get_acl_from_argv(param);
        }else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)){
            option.bucket_options.protocol =  get_protocol_from_argv(param);
        }else if (!strncmp(param, VERSIONID_PREFIX, VERSIONID_PREFIX_LEN)){
            object_info.version_id = &(param[VERSIONID_PREFIX_LEN]);
        }
    }


    set_object_acl_by_head(&option, &object_info, canned_acl, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set object acl by head successfully. \n");
    }
    else
    {
        printf("set object acl by head failed(%s).\n", obs_get_status_name(ret_status));
    }
}

// list objects--------------------------------------------------------------
typedef struct list_bucket_callback_data
{
    int is_truncated;
    char next_marker[1024];
    int keyCount;
    int allDetails;
    obs_status ret_status;
} list_bucket_callback_data;

static void printListBucketHeader(int allDetails)
{
    printf("%-50s  %-20s  %-5s", 
           "   Key", 
           "   Last Modified", "Size");
    if (allDetails) {
        printf("  %-34s  %-64s  %-12s", 
               "   ETag", 
               "   Owner ID",
               "Display Name");
    }
    printf("\n");
    printf("--------------------------------------------------  "
           "--------------------  -----");
    if (allDetails) {
        printf("  ----------------------------------  "
               "-------------------------------------------------"
               "---------------  ------------");
    }
    printf("\n");
}


static obs_status list_objects_callback(int is_truncated, const char *next_marker,
                                   int contents_count, 
                                   const obs_list_objects_content *contents,
                                   int common_prefixes_count,
                                   const char **common_prefixes,
                                   void *callback_data)
{
    list_bucket_callback_data *data = 
        (list_bucket_callback_data *) callback_data;

    data->is_truncated = is_truncated;
    // This is tricky.  S3 doesn't return the NextMarker if there is no
    // delimiter.  Why, I don't know, since it's still useful for paging
    // through results.  We want NextMarker to be the last content in the
    // list, so set it to that if necessary.
    if ((!next_marker || !next_marker[0]) && contents_count) {
        next_marker = contents[contents_count - 1].key;
    }
    if (next_marker) {
        snprintf_s(data->next_marker, sizeof(data->next_marker), sizeof(data->next_marker)-1,"%s",
                next_marker);
    }
    else {
        data->next_marker[0] = 0;
    }
    
    if (contents_count && !data->keyCount) {
        printListBucketHeader(data->allDetails);
    }

    int i;
    for (i = 0; i < contents_count; i++) {
        const obs_list_objects_content *content = &(contents[i]);
        char timebuf[256] = {0};

        time_t t = (time_t) content->last_modified;
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", 
                gmtime(&t));
        char sizebuf[16] = {0};
        if (content->size < 100000) {
            sprintf_s(sizebuf, sizeof(sizebuf),"%5llu", (unsigned long long) content->size);
        }
        else if (content->size < (1024 * 1024)) {
            sprintf_s(sizebuf, sizeof(sizebuf),"%4lluK", 
                    ((unsigned long long) content->size) / 1024ULL);
        }
        else if (content->size < (10 * 1024 * 1024)) {
            float f = (float)content->size;
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf),"%1.2fM", f);
        }
        else if (content->size < (1024 * 1024 * 1024)) {
            sprintf_s(sizebuf, sizeof(sizebuf),"%4lluM", 
                    ((unsigned long long) content->size) / 
                    (1024ULL * 1024ULL));
        }
        else {
            float f = (float)(content->size / 1024);
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf),"%1.2fG", f);
        }
        printf("%-50s  %s  %s", content->key, timebuf, sizebuf);
        if (data->allDetails) {
            printf("  %-34s  %-64s  %-12s   %-12s",
                   content->etag, 
                   content->owner_id ? content->owner_id : "",
                   content->owner_display_name ? content->owner_display_name : "",
                   content->storage_class?
                   content->storage_class:""
                   );
        }
        printf("\n");
    }

    data->keyCount += contents_count;

    for (i = 0; i < common_prefixes_count; i++) {
        printf("\nCommon Prefix: %s\n", common_prefixes[i]);
    }
    printf("contents_count:%d\n", contents_count);

    return OBS_STATUS_OK;
}

static void listobjects_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    list_bucket_callback_data *data = (list_bucket_callback_data *)callback_data;
    data->ret_status = status;
}

static void test_list_bucket_objects(int argc, char **argv, int optindex)
{
    obs_options option;
    int maxkeys  =  100;
    
    init_obs_options(&option);
    
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_list_objects_handler list_bucket_objects_handler =
    {
        { &response_properties_callback, &listobjects_complete_callback },
        &list_objects_callback
    };

    list_bucket_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(list_bucket_callback_data)); 
    list_bucket_objects(&option, NULL, data.next_marker,NULL, maxkeys, &list_bucket_objects_handler, &data); 
    if (OBS_STATUS_OK == data.ret_status) {
        printf("list bucket objects successfully. \n");
    }
    else
    {
        printf("list bucket objects failed(%s).\n", obs_get_status_name(data.ret_status));
    }

}

/*****************************list versions********************************************/
typedef struct list_versions_callback_data
{
    char bucket_name[1024];
    char prefix[1024];
    char key_marker[1024];
    char delimiter[1024];
    int max_keys;
    int is_truncated;
    char next_key_marker[1024];
    char next_versionId_marker[1024];
    int keyCount;
    int allDetails;
    obs_status ret_status;
} list_versions_callback_data;

static void printListVersionsHeader(int allDetails)
{
    printf("%-30s  %-20s  %-5s  %-64s", 
           "              Key", 
           "    Last Modified", " Size" , "                            VersionId");
    if (allDetails) {
        printf("  %-34s  %-32s  %-12s", 
               "               ETag", 
               "          Owner ID",
               "Display Name");
    }
    printf("\n");
    printf("------------------------------  "
           "--------------------  -----  ----------------------------------------------------------------");
    if (allDetails) {
        printf("  ----------------------------------  --------------------------------  ------------");
    }
    printf("\n");
}


static obs_status listVersionsCallback(int is_truncated, const char *next_key_marker, const char *next_versionId_marker,
                                   const obs_list_versions *list_versions, void *callback_data)
{
    list_versions_callback_data *data = 
        (list_versions_callback_data *) callback_data;

    data->is_truncated = is_truncated;
    
    if ((!next_key_marker || !next_key_marker[0]) && list_versions->versions_count) {
        next_key_marker = list_versions->versions[list_versions->versions_count - 1].key;
    }
    if (next_key_marker) {
        snprintf_s(data->next_key_marker, sizeof(data->next_key_marker), 
			sizeof(data->next_key_marker)-1,"%s",next_key_marker);
    }
    else {
        data->next_key_marker[0] = 0;
    }

    if ((!next_versionId_marker || !next_versionId_marker[0]) && list_versions->versions_count) {
        next_versionId_marker = list_versions->versions[list_versions->versions_count - 1].version_id;
    }
    if (next_versionId_marker) {
        snprintf_s(data->next_versionId_marker, sizeof(data->next_versionId_marker), 
			sizeof(data->next_versionId_marker)-1,"%s",next_versionId_marker);
    }
    else {
        data->next_versionId_marker[0] = 0;
    }
       
    if (NULL != list_versions->bucket_name)
    {
        printf("Name = %s\n", list_versions->bucket_name);
    }
       
    if (NULL != list_versions->prefix)
    {
        printf("prefix = %s\n", list_versions->prefix);
    }
    if (NULL != list_versions->key_marker)
    {
        printf("key_marker = %s\n", list_versions->key_marker);
    }
    if (NULL != list_versions->delimiter)
    {
        printf("delimiter = %s\n", list_versions->delimiter);
    }
    if (NULL != list_versions->max_keys)
    {
        printf("max_keys = %s\n", list_versions->max_keys);
    }

    if (list_versions->versions_count && !data->keyCount) {
        printListVersionsHeader(data->allDetails);
    }

    int i;

    for (i = 0; i < list_versions->versions_count; i++) {
        obs_version *version = &(list_versions->versions[i]);
        char timebuf[256] = {0};
        time_t t = (time_t) version->last_modified;
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", 
                 gmtime(&t));
        char sizebuf[16] = {0};
        if (version->size < 100000) {
            sprintf_s(sizebuf, sizeof(sizebuf),"%5llu", (unsigned long long) version->size);
        }
        else if (version->size < (1024 * 1024)) {
            sprintf_s(sizebuf, sizeof(sizebuf),"%4lluK", 
                    ((unsigned long long) version->size) / 1024ULL);
        }
        else if (version->size < (10 * 1024 * 1024)) {
            float f = version->size;
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf),"%1.2fM", f);
        }
        else if (version->size < (1024 * 1024 * 1024)) {
            sprintf_s(sizebuf, sizeof(sizebuf),"%4lluM", 
                    ((unsigned long long) version->size) / 
                    (1024ULL * 1024ULL));
        }
        else {
            float f = (version->size / 1024);
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf),"%1.2fG", f);
        }
        printf("%-30s  %s  %s  %s", version->key, timebuf, sizebuf, version->version_id);
        if (data->allDetails) {
            printf("  %-34s  %-32s  %-12s",
                   version->etag, 
                   version->owner_id ? version->owner_id : "",
                   version->owner_display_name ? 
                   version->owner_display_name : "");
        }
        printf("\n");
    }
    printf("versions_count=%d\n", list_versions->versions_count);

    printf("---------------------------------------------------------------------------------\n");
    for (i=0; i< list_versions->common_prefixes_count; i++)
    {
        printf("commonPrefix => prefix = %s\n", *(list_versions->common_prefixes + i));
    }

    data->keyCount += list_versions->versions_count;
    return OBS_STATUS_OK;
} 

static void list_versions_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    list_versions_callback_data *data = (list_versions_callback_data*)callback_data;
    data->ret_status = status;
}

static void test_list_versions(char *bucket_name,char *version_id_marker)
{
    char *prefix = "o";
    char *key_marker = "obj";  
    char *delimiter = "/"; 
    int maxkeys  =  10;
    
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_list_versions_handler list_versions_handler =
    {
        { &response_properties_callback, &list_versions_complete_callback },
        &listVersionsCallback
    };

    list_versions_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(list_bucket_callback_data)); 
    
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

/*****************test_get_bucket_storage_info************************************************/
static void test_get_bucket_storage_info(int argc, char **argv, int optindex)
{
    obs_options option; 
    char capacity[OBS_COMMON_LEN_256] = {0};
    char obj_num[OBS_COMMON_LEN_256] = {0};
    obs_status ret_status = OBS_STATUS_BUTT;
    
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    printf("bucket name is: %s\n", bucket_name);

    while(optindex < argc){
        char *param = argv[optindex++];
        if (!strcmp(param, "https"))
        {
            option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
        }else if (!strcmp(param, "http")){
            option.bucket_options.protocol = OBS_PROTOCOL_HTTP;
        }
    }
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler response_handler =
    { 
        NULL,
        &response_complete_callback
    };
    
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

/***********************test_set_bucket_lifecycle_configuration*****************************/
static void test_set_bucket_lifecycle_configuration()
{
    obs_options option;
    obs_status  ret_status = OBS_STATUS_BUTT;

    // set option
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    // Set the completed callback function
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    obs_lifecycle_conf bucket_lifecycle_conf;
    memset_s(&bucket_lifecycle_conf, sizeof(bucket_lifecycle_conf),0, sizeof(obs_lifecycle_conf)); 

    //Lifecycle rule id
    bucket_lifecycle_conf.id = "test1"; 
    // Specify the prefix "test"
    bucket_lifecycle_conf.prefix = "test"; 
    // Specify that the object that meets the prefix expires after 10 days of creation
    bucket_lifecycle_conf.days = "10"; 
    // Specifies that the historical version of the object that satisfies the prefix expires after 20 days 
    bucket_lifecycle_conf.noncurrent_version_days = "20";
    // Enable lifecycle rule 
    bucket_lifecycle_conf.status = "Enabled"; 

    set_bucket_lifecycle_configuration(&option, &bucket_lifecycle_conf, 1, 
            &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket lifecycle configuration success.\n");
    }
    else
    {
        printf("set bucket lifecycle configuration failed(%s).\n", 
                obs_get_status_name(ret_status));
    }
}


static void test_set_bucket_lifecycle_configuration2()
{
    obs_options option;
    obs_status  ret_status = OBS_STATUS_BUTT;

    // set option
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    // Set the completed callback function
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    obs_lifecycle_conf bucket_lifecycle_conf;
    memset_s(&bucket_lifecycle_conf, sizeof(bucket_lifecycle_conf),0, sizeof(obs_lifecycle_conf)); 
    
    //Lifecycle rule id
    bucket_lifecycle_conf.id = "test3"; 
    // Specify the prefix "test"
    bucket_lifecycle_conf.prefix = "bcd"; 
    // Enable lifecycle rule 
    bucket_lifecycle_conf.status = "Enabled"; 
    
    obs_lifecycle_transtion transition;
    memset_s(&transition, sizeof(transition),0, sizeof(obs_lifecycle_transtion));
    // Specify that the object that satisfies the prefix is ??converted after 30 days of creation. 
    transition.days = "30";
    // Specify the storage type after the object is converted
    transition.storage_class = OBS_STORAGE_CLASS_STANDARD_IA;
    bucket_lifecycle_conf.transition = &transition;
    bucket_lifecycle_conf.transition_num = 1;

    obs_lifecycle_noncurrent_transtion noncurrent_transition;
    memset_s(&noncurrent_transition, sizeof(noncurrent_transition),0, sizeof(obs_lifecycle_noncurrent_transtion));
    // Specify a historical version of the object that satisfies the prefix to convert after 30 days 
    noncurrent_transition.noncurrent_version_days = "30";
    // Specifies the storage type of the historical version of the object that satisfies the prefix 
    noncurrent_transition.storage_class = OBS_STORAGE_CLASS_STANDARD_IA;
    bucket_lifecycle_conf.noncurrent_version_transition = &noncurrent_transition;
    bucket_lifecycle_conf.noncurrent_version_transition_num = 1;
  

    set_bucket_lifecycle_configuration(&option, &bucket_lifecycle_conf, 1, 
                    &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket lifecycle configuration2 success.\n");
    }
    else
    {
        printf("set bucket lifecycle configuration2 failed(%s).\n", 
                    obs_get_status_name(ret_status));
    }
}


/*********************get_lifecycle_config*****************************************************/
obs_status getBucketLifecycleConfigurationCallbackEx (obs_lifecycle_conf* bucketLifeCycleConf,
                                unsigned int blccNumber,
                                void *callback_data)
{
    (void)callback_data;
    unsigned int i = 0;
    
    #define print_nonull(name, field)                                 \
    do {                                                           \
        if (field && field[0]) {                                  \
            printf("%s: %s\n", name, field);          \
        }                                                          \
    } while (0)
    
    for(i = 0; i < blccNumber; i++)
    {
        printf("-----------------------------------------------\n");
        print_nonull("id", bucketLifeCycleConf[i].id);
        print_nonull("prefix", bucketLifeCycleConf[i].prefix);
        print_nonull("status", bucketLifeCycleConf[i].status);
        print_nonull("days", bucketLifeCycleConf[i].days);
        print_nonull("date", bucketLifeCycleConf[i].date);
    }
    printf("-----------------------------------------------\n");
    return OBS_STATUS_OK;
}


static void test_get_lifecycle_config()
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    
    // set option
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    // Set callback function
    obs_lifecycle_handler lifeCycleHandlerEx =
    {
        {&response_properties_callback, &response_complete_callback},
        &getBucketLifecycleConfigurationCallbackEx
    };
    
    get_bucket_lifecycle_configuration(&option, &lifeCycleHandlerEx, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("get_lifecycle_config success.\n");
    }
    else
    {
        printf("get_lifecycle_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}


/*********************test_delete_lifecycle_config******************************************/
static void test_delete_lifecycle_config()
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;

    // set option
    init_obs_options(&option);    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    delete_bucket_lifecycle_configuration(&option, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("test_delete_lifecycle_config success.\n");
    }
    else
    {
        printf("test_delete_lifecycle_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}

/***********************test_set_bucket_cors*********************************************/
static void test_set_bucket_cors()
{
    obs_options option;
    obs_status  ret_status = OBS_STATUS_BUTT;

    obs_bucket_cors_conf bucketCorsConf; 
    // set option
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    // Set callback function
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    
    char *id_1= "1"; 
    // Specifies the cache time in seconds for the browser to return results for a specific resource's prefetch (OPTIONS) request. 
    char *max_age_seconds = "100";
    // Specify allowed cross-domain request methods(GET/PUT/DELETE/POST/HEAD) 
    const char* allowedMethod_1[5] = {"GET","PUT","HEAD","POST","DELETE"};
    // Specify the source that allows cross-domain requests 
    const char* allowedOrigin_1[2] = {"obs.xxx.com", "www.xxx.com"}; 
    // Specify a header that allows users to access from the application
    const char* allowedHeader_1[2] = {"header-1", "header-2"}; 
    // Attachment header field in response
    const char* exposeHeader_1[2]  = {"hello", "world"}; 
     
    memset_s(&bucketCorsConf, sizeof(bucketCorsConf),0, sizeof(obs_bucket_cors_conf)); 
    bucketCorsConf.id = id_1; 
    bucketCorsConf.max_age_seconds = max_age_seconds; 
    bucketCorsConf.allowed_method = allowedMethod_1; 
    bucketCorsConf.allowed_method_number = 5; 
    bucketCorsConf.allowed_origin = allowedOrigin_1;
    bucketCorsConf.allowed_origin_number = 2; 
    bucketCorsConf.allowed_header = allowedHeader_1; 
    bucketCorsConf.allowed_header_number = 2; 
    bucketCorsConf.expose_header = exposeHeader_1; 
    bucketCorsConf.expose_header_number = 2; 
     
    set_bucket_cors_configuration(&option, &bucketCorsConf, 1, &response_handler, &ret_status); 
    if (OBS_STATUS_OK == ret_status) {
        printf("set_bucket_cors success.\n");
    }
    else {
        printf("set_bucket_cors failed(%s).\n", obs_get_status_name(ret_status));
    }
}

/*********************test_get_cors_config*****************************************************/
obs_status get_cors_info_callback(obs_bucket_cors_conf* bucket_cors_conf,
                                 unsigned int bcc_number,
                                void *callback_data)
{
    (void)callback_data;
    
    unsigned int i = 0;
    for (i=0; i<bcc_number; ++i)
    {
        printf("------------------------------------------------------\n");
        printf("id = %s\nmaxAgeSeconds = %s\n", bucket_cors_conf[i].id, bucket_cors_conf[i].max_age_seconds);
        unsigned int j;
        for(j = 0; j < bucket_cors_conf[i].allowed_method_number; j++)
        {
            printf("allowedMethodes = %s\n", bucket_cors_conf[i].allowed_method[j]);
        }
        for(j = 0; j < bucket_cors_conf[i].allowed_origin_number; j++)
        {
            printf("allowedOrigines = %s\n", bucket_cors_conf[i].allowed_origin[j]);
        }
        for(j = 0; j < bucket_cors_conf[i].allowed_header_number; j++)
        {
            printf("allowedHeaderes = %s\n", bucket_cors_conf[i].allowed_header[j]);
        }
        for(j = 0; j < bucket_cors_conf[i].expose_header_number; j++)
        {
            printf("exposeHeaderes = %s\n", bucket_cors_conf[i].expose_header[j]);
        }
    }
    printf("------------------------------------------------------\n");
    return OBS_STATUS_OK;
}

static void test_get_cors_config()
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    // set option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = BUCKET_NAME;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    // Set callback function
    obs_cors_handler cors_handler_info =
    {
        {&response_properties_callback, &response_complete_callback},
        &get_cors_info_callback
    };

    get_bucket_cors_configuration(&option, &cors_handler_info, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("get_cors_config success.\n");
    }
    else {
        printf("get_cors_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}

/*********************test_delete_cors_config******************************************/
static void test_delete_cors_config()
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    // set option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = BUCKET_NAME;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    // Set callback function
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    delete_bucket_cors_configuration(&option, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("delete_cors_config success.\n");
    }
    else
    {
        printf("delete_cors_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}


/***********************test_set_notification_configuration*********************************************/
static void test_set_notification_configuration(char *bucket_name)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    obs_smn_notification_configuration notification_conf; 
    obs_smn_topic_configuration topic_conf;
    obs_smn_event_enum topic1_event[2];
    obs_smn_filter_rule filter_rule;
    // set option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    // Set callback function
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };
    // Set the id of the notification configuration, which is uniquely labeled
    topic_conf.id    = "Id001"; 
    // Set the URN of the event notification topic
    topic_conf.topic = "urn:smn:southchina:ea79855fbe0642718cb4df1551c3cb4e:test_xxxxxx";
    // Set notification action
    topic_conf.event = topic1_event; 
    topic_conf.event[0] = SMN_EVENT_OBJECT_CREATED_ALL; 
    topic_conf.event[1] = SMN_EVENT_OBJECT_CREATED_POST; 
    topic_conf.event_num = 2; 
    // Set the filtering rules for notification objects
    filter_rule.name = OBS_SMN_FILTER_PREFIX; 
    filter_rule.value = "aaa"; 
    topic_conf.filter_rule = &filter_rule; 
    topic_conf.filter_rule_num = 1; 
      
    memset_s(&notification_conf, sizeof(notification_conf),0, sizeof(obs_smn_notification_configuration)); 
    notification_conf.topic_conf = &topic_conf; 
    notification_conf.topic_conf_num = 1;

    set_notification_configuration(&option, &notification_conf, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set_notification_configuration success.\n");
    }
    else {
        printf("set_notification_configuration failed(%s).\n",
                    obs_get_status_name(ret_status));
    }
}

/******************test_get_notification_config****************************************/
obs_status get_notification_info_callback(obs_smn_notification_configuration* notification_conf,
                                void *callback_data)
{
    int i =0;
    int j =0;
    (void)callback_data;
    printf("topicNum=%d\n", notification_conf->topic_conf_num);

    for (i=0; i < notification_conf->topic_conf_num; i++)
    {
        printf("********************num(%d)*************************", i);
        printf("id: %s \n", notification_conf->topic_conf[i].id);
        printf("topic: %s \n", notification_conf->topic_conf[i].topic);
        printf("event_num: %s \n", notification_conf->topic_conf[i].event_num);
        for (j =0; j < notification_conf->topic_conf[i].event_num; j++)
        {
            printf("    event:d%\n",  notification_conf->topic_conf[i].event[j]);
        }
        
        printf("filter_rule_num: %s \n", notification_conf->topic_conf[i].filter_rule_num);
        for (j =0; j < notification_conf->topic_conf[i].filter_rule_num; j++)
        {
            printf("    name:d%, value:%s\n",  
                notification_conf->topic_conf[i].filter_rule[j].name,
                notification_conf->topic_conf[i].filter_rule[j].value);
        }
    }
	    return OBS_STATUS_OK;
}

static void test_get_notification_config(char *bucket_name)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    // set option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    // Set callback function
    obs_smn_handler notification_handler_info =
    {
        {&response_properties_callback, &response_complete_callback},
        &get_notification_info_callback
    };

    get_notification_configuration(&option, &notification_handler_info, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("get_notification_config success.\n");
    }
    else{
        printf("get_notification_config failed(%s).\n", obs_get_status_name(ret_status));
    }
}

static void test_close_notification_configuration(char *bucket_name)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    obs_smn_notification_configuration notification_conf; 
    
    // set option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    // Set callback function
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    memset_s(&notification_conf, sizeof(notification_conf),0, sizeof(obs_smn_notification_configuration)); 
    set_notification_configuration(&option, &notification_conf, 
                    &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("close_notification_configuration success.\n");
    }
    else {
        printf("close_notification_configuration failed(%s).\n",
                    obs_get_status_name(ret_status));
    }
}



void test_bucket_option(char *bucket_name)
{   
    obs_options *option;
    option = (obs_options *)malloc(sizeof(obs_options));
    init_obs_options(option);

    char* cOrigin = "obs.xxx.com"; 
    char allowed_method[5][256]={"GET","PUT","HEAD","POST","DELETE"}; 
    unsigned int am = 5; 
    char requestHeader[][256] = {"header-1", "header-2"};  
    unsigned int rhNumber = 2; 
    
    option->bucket_options.host_name = HOST_NAME;
    option->bucket_options.bucket_name = bucket_name;
    option->bucket_options.access_key = ACCESS_KEY_ID;
    option->bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option->bucket_options.protocol = OBS_PROTOCOL_HTTPS;
   
    obs_response_handler resqonseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };

    obs_options_bucket(option, cOrigin, allowed_method, am, requestHeader, rhNumber, &resqonseHandler, 0);
    
    if (statusG == OBS_STATUS_OK) {
        printf("bucket option successfully. \n");
    }
    else
    {
        printf("bucket option failed.\n");
        printError();
    }

    free(option);
}

// put object ---------------------------------------------------------------
typedef struct growbuffer
{
    int size;
    int start;
    char data[64 * 1024];
} growbuffer;


typedef struct put_file_object_callback_data
{
    FILE *infile;
    uint64_t content_length;
    obs_status ret_status;
} put_file_object_callback_data;

typedef struct put_buffer_object_callback_data
{
    char *put_buffer;
    uint64_t buffer_size;
    uint64_t cur_offset;
    obs_status ret_status;
} put_buffer_object_callback_data;


static void put_file_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    put_file_object_callback_data *data = (put_file_object_callback_data *)callback_data;
    data->ret_status = status;
}

static void put_buffer_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    put_buffer_object_callback_data *data = (put_buffer_object_callback_data *)callback_data;
    data->ret_status = status;
}



static int put_file_data_callback(int buffer_size, char *buffer,
                                 void *callback_data)
{
    put_file_object_callback_data *data = 
        (put_file_object_callback_data *) callback_data;
    
    int ret = 0;
    if (data->content_length) {
        int toRead = ((data->content_length > (unsigned) buffer_size) ?
                    (unsigned) buffer_size : data->content_length);
        ret = fread(buffer, 1, toRead, data->infile);
    }

    uint64_t originalContentLength = data->content_length;
    data->content_length -= ret;
    if (data->content_length) {
        printf("%llu bytes remaining ", (unsigned long long)data->content_length);
        printf("(%d%% complete) ...\n",
             (int)(((originalContentLength - data->content_length) * 100) / originalContentLength));
    }

    return ret;
}

static int put_buffer_data_callback(int buffer_size, char *buffer,
                                 void *callback_data)
{
    put_buffer_object_callback_data *data = 
        (put_buffer_object_callback_data *) callback_data;
    
    int toRead = 0;
    if (data->buffer_size) {
        toRead = ((data->buffer_size > (unsigned) buffer_size) ?
                    (unsigned) buffer_size : data->buffer_size);
        memcpy_s(buffer, sizeof(buffer),data->put_buffer + data->cur_offset, toRead);
    }
    
    uint64_t originalContentLength = data->buffer_size;
    data->buffer_size -= toRead;    
    data->cur_offset += toRead;
    if (data->buffer_size ) {
        printf("%llu bytes remaining ", (unsigned long long)data->buffer_size);
        printf("(%d%% complete) ...\n", 
            (int) (((originalContentLength - data->buffer_size) * 100) / originalContentLength));
    }

    return toRead;
}

uint64_t open_file_and_get_length(char *localfile, put_file_object_callback_data *data)
{
    uint64_t content_length = 0;
    if (!content_length) 
    {
        struct stat statbuf;
        if (stat(localfile, &statbuf) == -1)
        {
            fprintf(stderr, "\nERROR: Failed to stat file %s: ",
            localfile);
            perror(0);
            exit(-1);
        }
        content_length = statbuf.st_size;
		printf("content_length 1 = %d\n",content_length);
    }
    if (!(data->infile = fopen(localfile, "rb"))) 
    {
        fprintf(stderr, "\nERROR: Failed to open input file %s: ",
        localfile);
        perror(0);
        exit(-1);
    }    
    data->content_length = content_length;
    return content_length;
}

static void progress_callback(uint64_t now, uint64_t total, void* callback_data)
{
    if (total)
    {
        printf("progress is %d%% \n", (now * 100) / total);
    }
}

static void test_put_object_from_file(int argc, char **argv, int optindex)
{
    obs_canned_acl canned_acl = OBS_CANNED_ACL_PRIVATE;
    char *content_encoding = NULL;
    char *expires = NULL;

    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);

    // name of upload object
    char *key = argv[optindex++];
    printf("key is == %s \n", key);
    // file to upload
    char *file_name = argv[optindex++];
    printf("file_name is == %s \n", file_name);
    uint64_t content_length = 0;
    
    // Initialize option
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;

    temp_auth_configure tempauth;
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));

    while (optindex < argc){
        char *param = argv[optindex++];
        if (!strncmp(param, TMP_AUTH_EXPIRES_PREFIX,TMP_AUTH_EXPIRES_PREFIX_LEN)){
            tempauth.callback_data = (void *)(&ptrResult);
            int auth_expire = atoi(&param[TMP_AUTH_EXPIRES_PREFIX_LEN]);
            tempauth.expires = auth_expire;
            tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
            option.temp_auth = &tempauth;
        }else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            canned_acl = get_acl_from_argv(param);
        }else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }else if (!strncmp(param, CONTENT_ENCODING_PREFIX, CONTENT_ENCODING_PREFIX_LEN)) {
            content_encoding = &(param[CONTENT_ENCODING_PREFIX_LEN]);
        }else if (!strncmp(param, EXPIRES_PREFIX, EXPIRES_PREFIX_LEN)){
            expires = &(param[EXPIRES_PREFIX_LEN]);
        }else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN)) {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }
    }


    // Initialize upload object properties
    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    put_properties.canned_acl = canned_acl;
    put_properties.content_encoding = content_encoding;
    if (expires != NULL)
    {
        put_properties.expires = convertInt(expires, "expires");
    }
	
    //Server encryption
    /*SSE-KMS encryption*/
    server_side_encryption_params encryption_params;
    memset_s(&encryption_params, sizeof(encryption_params),0, sizeof(server_side_encryption_params));
    //encryption_params.use_kms = '1';
    //encryption_params.kms_server_side_encryption = "kms";
    //Do not set the system will generate a default encryption key
    //encryption_params.kms_key_id = "sichuan:domainiddomainiddomainiddoma0001:key/xxxxxxxxxxxxxxxxxx";

    /*SSE-C*/
    /*char* buffer = "xxxxxxxxxxxxx";
    encryption_params.use_ssec = '1';
    encryption_params.ssec_customer_algorithm = "AES256";
    encryption_params.ssec_customer_key = buffer;*/

    // Initialize the structure that stores the uploaded data
    put_file_object_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(put_file_object_callback_data));
    // Open the file and get the file length
    content_length = open_file_and_get_length(file_name, &data);
	printf("content_length = %d\n",content_length);
    // Set callback function
    obs_put_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_file_complete_callback },
        &put_file_data_callback,
        &progress_callback
    };
    
    put_object(&option, key, content_length, &put_properties, &encryption_params, &putobjectHandler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("put object from file successfully. \n");
    }
    else
    {
        printf("put object failed(%s).\n",  
               obs_get_status_name(data.ret_status));
    }
}


static void test_put_object_from_buffer(int argc, char **argv, int optindex)
{
    obs_canned_acl canned_acl = OBS_CANNED_ACL_PRIVATE;
    char *content_encoding = NULL;
    char *expires = NULL;

    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);
    
    // name of upload object
    char *key = argv[optindex++];
    printf("key's name is == %s \n", key);
    //  buffer to be uploaded
    char *buffer = argv[optindex++];
    printf("buffer's name is == %s \n", buffer);
    // Length of the buffer to be uploaded
    int buffer_size = strlen(buffer);

    // Initialize option
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    temp_auth_configure tempauth;
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));

    while (optindex < argc){
        char *param = argv[optindex++];
        if (!strncmp(param, TMP_AUTH_EXPIRES_PREFIX,TMP_AUTH_EXPIRES_PREFIX_LEN)){
            tempauth.callback_data = (void *)(&ptrResult);
            int auth_expire = atoi(&param[TMP_AUTH_EXPIRES_PREFIX_LEN]);
            tempauth.expires = auth_expire;
            tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
            option.temp_auth = &tempauth;
        }else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            canned_acl = get_acl_from_argv(param);
        }else if (!strncmp(param, PROTOCOL_PREFIX, PROTOCOL_PREFIX_LEN)) {
            option.bucket_options.protocol = get_protocol_from_argv(param);
        }else if (!strncmp(param, CONTENT_ENCODING_PREFIX, CONTENT_ENCODING_PREFIX_LEN)) {
            content_encoding = &(param[CONTENT_ENCODING_PREFIX_LEN]);
        }else if (!strncmp(param, EXPIRES_PREFIX, EXPIRES_PREFIX_LEN)){
            expires = &(param[EXPIRES_PREFIX_LEN]);
        }else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN)) {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }
    }


    // Initialize upload object properties
    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    put_properties.canned_acl = canned_acl;
    put_properties.content_encoding = content_encoding;
    if (expires != NULL)
    {
        put_properties.expires = convertInt(expires, "expires");
    }
    

    //Initialize the structure that stores the uploaded data
    put_buffer_object_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(put_buffer_object_callback_data));
    // Assign buffer to the uploaded data structure
    data.put_buffer = buffer;
    // Set buffersize
    data.buffer_size = buffer_size;

    // Set callback function
    obs_put_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_buffer_complete_callback },
          &put_buffer_data_callback,
          &progress_callback
    };

    put_object(&option, key, buffer_size, &put_properties,0,&putobjectHandler,&data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("put object from buffer successfully. \n");
    }
    else
    {
        printf("put object from buffer failed(%s).\n", obs_get_status_name(data.ret_status));
    }
}


// get object ---------------------------------------------------------------

typedef struct get_object_callback_data
{
    FILE *outfile;
    obs_status ret_status;
}get_object_callback_data;

static obs_status get_object_data_callback(int buffer_size, const char *buffer,
                                      void *callback_data)
{
    get_object_callback_data *data = (get_object_callback_data *) callback_data;
    size_t wrote = fwrite(buffer, 1, buffer_size, data->outfile);
    return ((wrote < (size_t) buffer_size) ? 
            OBS_STATUS_AbortedByCallback : OBS_STATUS_OK);
}

static void get_object_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    get_object_callback_data *data = (get_object_callback_data *) callback_data;
    data->ret_status = status;
}

static void test_get_object(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);

    char *key = argv[optindex++];
    printf("key is == %s \n", key);


    char *file_name = argv[optindex++];
    printf("file_name is == %s \n", file_name);



    obs_object_info object_info;
    
    obs_options option;
    init_obs_options(&option);

    temp_auth_configure tempauth;
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(ptrResult),0,sizeof(tempAuthResult));

    while (optindex < argc){
        char *param = argv[optindex++];
        if (!strncmp(param, TMP_AUTH_EXPIRES_PREFIX,TMP_AUTH_EXPIRES_PREFIX_LEN)){
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
    
	option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;

	//Server encryption
    /*SSE-KMS encryption*/
    server_side_encryption_params encryption_params;
    memset_s(&encryption_params, sizeof(encryption_params),0, sizeof(server_side_encryption_params));
    //encryption_params.use_kms = '1';
    //encryption_params.kms_server_side_encryption = "aws:kms";
	//This parameter can be left blank. The system has a default encryption key.
    //encryption_params.kms_key_id = "sichuan:domainiddomainiddomainiddoma0001:key/xxxxxxxxxxxxxxxxxxxxx";

    /*SSE-C*/
    /*char* buffer = "xxxxxxxxxxxxx";
    encryption_params.use_ssec = '1';
    encryption_params.ssec_customer_algorithm = "AES256";
    encryption_params.ssec_customer_key = buffer;*/

    memset_s(&object_info, sizeof(object_info),0, sizeof(obs_object_info));
    object_info.key =key;
    
    get_object_callback_data data;
    data.ret_status = OBS_STATUS_BUTT;
    data.outfile = write_to_file(file_name);

    obs_get_conditions getcondition;
    memset_s(&getcondition, sizeof(getcondition),0, sizeof(obs_get_conditions));
    init_get_properties(&getcondition);
    // The starting position of the reading
    getcondition.start_byte = 0;
    // Read length, default 0: read to the end of the object
    getcondition.byte_count = 100;
    obs_get_object_handler get_object_handler =
    { 
        { &response_properties_callback,
          &get_object_complete_callback},
        &get_object_data_callback
    };
    
    get_object(&option, &object_info, &getcondition, &encryption_params, &get_object_handler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("get object successfully. \n");
    }
    else
    {
        printf("get object faied(%s).\n", obs_get_status_name(data.ret_status));
    }
    fclose(data.outfile);
}

// delete object ---------------------------------------------------------------

static void test_delete_object(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_object_info object_info;
    memset_s(&object_info, sizeof(object_info),0, sizeof(obs_object_info));
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);
    object_info.key = argv[optindex++];
    printf("key is == %s \n", object_info.key);

    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
    
    
    obs_response_handler resqonseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };
    
    delete_object(&option,&object_info,&resqonseHandler, &ret_status);
    if (OBS_STATUS_OK == ret_status) 
    {
        printf("delete object successfully. \n");
    }
    else
    {
        printf("delete object failed(%s).\n", obs_get_status_name(ret_status));
    }

}

static obs_status delete_objects_data_callback(int contentsCount, 
                                            obs_delete_objects *delobjs,
                                            void *callbackData)
{
    int i;
    for (i = 0; i < contentsCount; i++) {
        const obs_delete_objects*content = &(delobjs[i]);
        int iRet = atoi(content->code);
        if(0 != iRet)
        {
            printf("delete object result:\nobject key:%s\nerror code:%s\nerror message:%s\n", 
                content->key, content->code, content->message);
        }
        else
        {
            printf("delete object result:\nobject key:%s\nerror code:%s\n", content->key, content->code);
        }            
    }
    return OBS_STATUS_OK;
}

static void test_batch_delete_objects(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    printf("bucket name is: %s\n", bucket_name);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_object_info objectinfo[1000];
    char tmp[1000][10];
    for (int i = 0; i < 1000; ++i)
    {
        sprintf_s(tmp[i], sizeof(tmp[i]),"obj%d", i);
        objectinfo[i].key = tmp[i];
        objectinfo[i].version_id = 0;
    }


    obs_delete_object_info delobj;
    memset_s(&delobj,sizeof(obs_delete_object_info),0,sizeof(obs_delete_object_info));
    delobj.keys_number = 1000;

    obs_delete_object_handler handler =
    { 
        {&response_properties_callback, &response_complete_callback},
        &delete_objects_data_callback
    };

    batch_delete_objects(&option, objectinfo, &delobj, 0, &handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test batch_delete_objects successfully. \n");
    }
    else
    {
        printf("test batch_delete_objects faied(%s).\n", obs_get_status_name(ret_status));
    }
}


static void test_copy_object(int argc, char **argv, int optindex)
{

    char *source_bucket = argv[optindex++];
    printf("source_bucket's name is == %s \n", source_bucket);

    char *source_key = argv[optindex++];
    printf("source_key is == %s \n", source_key);

    char *target_bucket = argv[optindex++];
    printf("target_bucket's name is == %s \n", target_bucket);

    char *target_key = argv[optindex++];
    printf("target_key is == %s \n", target_key);

    char *version_id = NULL;

    obs_options option;
    init_obs_options(&option);

    while (optindex < argc){
        char *param = argv[optindex++];
        if (!strncmp(param, VERSIONID_PREFIX, VERSIONID_PREFIX_LEN)) {
            version_id = &(param[VERSIONID_PREFIX_LEN]);
            printf("version_id is: %s\n", version_id);
        }else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN))
        {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }
    }



    obs_status ret_status = OBS_STATUS_BUTT;
    char eTag[OBS_COMMON_LEN_256] = {0};
    int64_t lastModified;
    
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = source_bucket;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
	option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;

	
	//Server encryption
    /*SSE-KMS encryption*/
    server_side_encryption_params encryption_params;
    memset_s(&encryption_params, sizeof(encryption_params),0, sizeof(server_side_encryption_params));
    //encryption_params.use_kms = '1';
    //encryption_params.kms_server_side_encryption = "aws:kms";
    //This parameter can be NULL the default system key
    //encryption_params.kms_key_id = "sichuan:domainiddomainiddomainiddoma0001:key/xxxxxxxxxxxxxxxxxxxxxxxxxxx";

    /*SSE-C*/
    /*char* buffer = "xxxxxxxxxxxxx";
    encryption_params.use_ssec = '1';
    encryption_params.ssec_customer_algorithm = "AES256";
    encryption_params.ssec_customer_key = buffer;
    
    //Decrypt source object parameters; used to copy one encrypted object to another object
    char *des_key = "xxxxxxxxxxxxx";
    encryption_params.des_ssec_customer_algorithm = "AES256";
    encryption_params.des_ssec_customer_key = des_key;*/
	
    obs_copy_destination_object_info objectinfo ={0};
    objectinfo.destination_bucket = target_bucket;
    objectinfo.destination_key = target_key;
    objectinfo.etag_return = eTag;
    objectinfo.etag_return_size = sizeof(eTag);
    objectinfo.last_modified_return = &lastModified;

    obs_response_handler responseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };

   
    copy_object(&option, source_key, version_id, &objectinfo, 1, NULL, &encryption_params,&responseHandler,&ret_status);
   
    if (OBS_STATUS_OK == ret_status) {
        printf("test_copy_object  successfully. \n");

    }
    else
    {
        printf("test_copy_object failed(%s).\n", obs_get_status_name(ret_status));
    }

}

static void test_restore_object(int argc, char **argv, int optindex)
{

    obs_object_info object_info;
    memset_s(&object_info, sizeof(object_info),0, sizeof(obs_object_info));

    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);

    char *key = argv[optindex++];
    printf("key is == %s \n", key);

    char *days = argv[optindex++];
    printf("days is == %s \n", days);

    obs_tier tier = OBS_TIER_EXPEDITED;
    char *version_id;

    while (optindex < argc){
        char *param = argv[optindex++];
        if (!strncmp(param, VERSIONID_PREFIX, VERSIONID_PREFIX_LEN)) {
            version_id = &(param[VERSIONID_PREFIX_LEN]);
            printf("version_id is: %s\n", version_id);
        }else if (!strncmp(param, TIER_PREFIX, TIER_PREFIX_LEN)){
            char* tmp = &(param[TIER_PREFIX_LEN]);
            switch(tmp[0]){
                case '0':
                    tier = OBS_TIER_NULL;
                    break;

                case '1':
                    tier = OBS_TIER_STANDARD;
                    break;

                case '2':
                    tier = OBS_TIER_EXPEDITED;
                    break;

                case '3':
                    tier = OBS_TIER_BULK;
                    break;

                default:
                    break;
            }
        }
    }

    object_info.key = key;
    object_info.version_id = version_id;

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
        
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
    // download file
    char *file_name = "C:\\a.txt";
    get_object_callback_data data;
    data.ret_status = OBS_STATUS_BUTT;
    data.outfile = write_to_file(file_name);

    obs_get_object_handler getobjectHandler =
    { 
        { &response_properties_callback,
          &get_object_complete_callback},
        &get_object_data_callback
    };
    get_object(&option,&object_info, 0,0,&getobjectHandler,&data);
    fclose(data.outfile);
    if (statusG == OBS_STATUS_OK) {
        printf("get object successfully. \n");
    }
    else
    {
        printf("get object faied.\n");
        printError();
    }
}


// list object ---------------------------------------------------------------

typedef struct list_service_data
{
    int headerPrinted;
    int allDetails;
    obs_status ret_status;
} list_service_data;


static void printListServiceHeader(int allDetails)
{
    printf("%-56s  %-20s", "                         Bucket",
            "      Created");
    if (allDetails) {
        printf("  %-64s  %-12s", 
                "                            Owner ID",
                "Display Name");
    }
    printf("\n");
    printf("--------------------------------------------------------  "
            "--------------------");
    if (allDetails) {
        printf("  -------------------------------------------------"
                "---------------  ------------");
    }
    printf("\n");
}


static obs_status listServiceCallback(const char *owner_id, 
                                    const char *owner_display_name,
                                    const char *bucket_name,
                                    int64_t creationDate, void *callback_data)
{
    list_service_data *data = (list_service_data *) callback_data;

    if (!data->headerPrinted) {
        data->headerPrinted = 1;
        printListServiceHeader(data->allDetails);
    }

    char timebuf[256] = {0};
    if (creationDate >= 0) {
        time_t t = (time_t) creationDate;
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
    }
    else {
        timebuf[0] = 0;
    }

    printf("%-56s  %-20s", bucket_name, timebuf);
    if (data->allDetails) {
        printf("  %-64s  %-12s", owner_id ? owner_id : "", 
               owner_display_name ? owner_display_name : "");
    }
    printf("\n");

    return OBS_STATUS_OK;
}

void list_bucket_complete_callback(obs_status status,
                                 const obs_error_details *error, 
                                 void *callback_data)
{
    list_service_data *data = (list_service_data *)callback_data;
    data->ret_status = status;
}

static void test_list_bucket()
{
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    list_service_data data;
    memset_s(&data, sizeof(data),0, sizeof(list_service_data));
    
    obs_list_service_handler listHandler =
    { 
        {&response_properties_callback,
        &list_bucket_complete_callback },
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

void test_object_option(char *bucket_name)
{   
    obs_options *option;
    option = (obs_options *)malloc(sizeof(obs_options));
    init_obs_options(option);

    char *cKey = "key"; 
    char* cOrigin = "obs.xxx.com"; 
    char allowed_method[5][256]={"GET","PUT","HEAD","POST","DELETE"}; 
    unsigned int am = 5; 
    char requestHeader[][256] = {"header-1", "header-2"};  
    unsigned int rhNumber = 2; 
    
    option->bucket_options.host_name = HOST_NAME;
    option->bucket_options.bucket_name = bucket_name;
    option->bucket_options.access_key = ACCESS_KEY_ID;
    option->bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option->bucket_options.protocol = OBS_PROTOCOL_HTTPS;

   
    obs_response_handler resqonseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };

    obs_options_object(option, cKey, cOrigin, allowed_method, am, requestHeader, rhNumber, &resqonseHandler, 0);
    
    if (statusG == OBS_STATUS_OK) 
    {
        printf("object option successfully. \n");
    }
    else
    {
        printf("object option failed.\n");
        printError();
    }

    free(option);
}

static void test_head_bucket(int argc, char **argv, int optindex)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    char *bucket_name = argv[optindex++];
    printf("bucket_name is: %s\n", bucket_name);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    obs_head_bucket(&option, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) 
    {
        printf("head bucket successfully. \n");
    }
    else 
    {
        printf("head bucket failed(%s).\n", obs_get_status_name(ret_status));
    }

}

static void test_get_bucket_metadata_with_cors(char *bucket_name)
{
    obs_options *option;
    option = (obs_options *)malloc(sizeof(obs_options));
    init_obs_options(option);
    
    option->bucket_options.host_name = HOST_NAME;
    option->bucket_options.bucket_name = bucket_name;
    option->bucket_options.access_key = ACCESS_KEY_ID;
    option->bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    
    char* origin = "origin"; 
    char request_header[][OBS_COMMON_LEN_256] = {"type=123", "123=34"}; 
    unsigned int number = 2; 

    get_bucket_metadata_with_corsconf(option, origin, request_header, number, &response_handler);
    if (statusG == OBS_STATUS_OK) 
    {
        printf("get bucket metadata with cors successfully. \n");
    }
    else 
    {
        printf("get bucket metadata with cors failed.\n");
        printError();
    }

     free(option);
}

static void test_init_upload_part(char *key)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    char upload_id[OBS_COMMON_LEN_256] = {0};
    int upload_id_size = OBS_COMMON_LEN_256;
    
    obs_response_handler handler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };
        
    initiate_multi_part_upload(&option, key, upload_id_size, upload_id, NULL, 0, &handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status)
    {
        printf("test init upload part successfully. uploadId= %s\n", upload_id);
        strcpy_s(UPLOAD_ID, sizeof(UPLOAD_ID),upload_id);
    }
    else
    {
        printf("test init upload part faied(%s).\n", obs_get_status_name(ret_status));
    }
}

typedef struct test_upload_file_callback_data
{
    FILE *infile;
    int part_num;
    uint64_t part_size;
    uint64_t start_byte;
    int noStatus;
} test_upload_file_callback_data;

void upload_part_from_file(test_upload_file_callback_data *data)
{
    int result = fseek(data->infile, data->start_byte, SEEK_SET);
    printf("result :=%d\n",result);
}

static int test_upload_file_data_callback(int buffer_size, char *buffer,
                                 void *callback_data)
{
    test_upload_file_callback_data *data = 
        (test_upload_file_callback_data *) callback_data;
    int ret=0;
    fseek(data->infile, data->start_byte, SEEK_SET);
    int toRead = ((data->part_size> (unsigned) buffer_size) ?
                    (unsigned) buffer_size : data->part_size);
    ret = fread(buffer, 1, toRead, data->infile);

    data->start_byte += toRead;
	
	return ret;
}

uint64_t get_file_info(char *localfile, test_upload_file_callback_data *data)
{
    data->infile = 0;
    uint64_t content_length = 0;
    if (!content_length) 
    {
        struct stat statbuf;
        if (stat(localfile, &statbuf) == -1)
        {
            fprintf(stderr, "\nERROR: Failed to stat file %s: ",
            localfile);
            perror(0);
            exit(-1);
        }
        content_length = statbuf.st_size;
    }
    if (!(data->infile = fopen(localfile, "rb"))) 
    {
        fprintf(stderr, "\nERROR: Failed to open input file %s: ",
        localfile);
        perror(0);
        exit(-1);
    }    
    return content_length;
}

static void test_upload_part(char *filename, char *key)
{
    uint64_t uploadSliceSize =5L * 1024 * 1024;                   // upload part slice size
    uint64_t uploadSize = uploadSliceSize;                         // upload part size
    uint64_t filesize =0;                                          // file total size

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);

    obs_upload_part_info uploadPartInfo;
    memset_s(&uploadPartInfo, sizeof(uploadPartInfo),0, sizeof(obs_upload_part_info));

    obs_upload_handler Handler =
    { 
        {&response_properties_callback,
         &response_complete_callback},
        &test_upload_file_data_callback
    };

    test_upload_file_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(test_upload_file_callback_data));
    //read local file total size :uploadTotalLength
    filesize = get_file_info(filename,&data);
    data.noStatus = 1;
    data.part_size = uploadSize;
    data.part_num = (filesize % uploadSize == 0) ? (filesize / uploadSize) : (filesize / uploadSize +1);
    
    // upload first part
    uploadPartInfo.part_number= 1;
    uploadPartInfo.upload_id=UPLOAD_ID;
    data.start_byte  = 0;
    upload_part(&option, key, &uploadPartInfo,uploadSize,&putProperties,0,&Handler,&data);
    
    if (statusG == OBS_STATUS_OK) {
        printf("test upload part 1 successfully. \n");
    }
    else
    {
        printf("test upload part 1 faied.\n");
        printError();
        return ;
    }
    
    //upload second part
    uploadPartInfo.part_number= 2;
    uploadPartInfo.upload_id=UPLOAD_ID;
    filesize = get_file_info(filename,&data);
    uploadSize =filesize - uploadSize;
    data.part_size = uploadSize;
    data.start_byte = uploadSliceSize;
    
   // fseek(data.infile, data.start_byte, SEEK_SET);
    upload_part(&option,key,&uploadPartInfo,uploadSize, &putProperties,0,&Handler,&data);
    
    if (statusG == OBS_STATUS_OK) {
        printf("test upload part 2 successfully. \n");
    }
    else
    {
        printf("test upload part 2 faied.\n");
        printError();
        return ;
    }
}

obs_status CompleteMultipartUploadCallback(const char *location, 
                                         const char *bucket,
                                         const char *key,
                                         const char* eTag,
                                         void *callbackData)
{
    (void)callbackData;
    printf("location = %s \nbucket = %s \nkey = %s \neTag = %s \n",location,bucket,key,eTag);
    return OBS_STATUS_OK;
}


static void test_complete_upload(char *filename, char *key)
{
    int number=2;
    char *uploadId="000001636E352A8C5778CD5C763BBBAB";
    obs_complete_upload_Info info[2];
    info[0].part_number= 1;
    info[0].etag="65fe0e161b35c8deead213871033f7fa";
    info[1].part_number= 2;
    info[1].etag="0433d5ffc28450be3b6cf25ab8955267";

    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);
    

    obs_complete_multi_part_upload_handler Handler =
    { 
        {&response_properties_callback,
         &response_complete_callback},
        &CompleteMultipartUploadCallback
    };
    
    complete_multi_part_upload(&option,key,uploadId,number,info,&putProperties,&Handler,0);
    if (statusG == OBS_STATUS_OK) {
        printf("test complete upload successfully. \n");
    }
    else
    {
        printf("test complete upload faied.\n");
        printError();
    }
}

static void test_abort_multi_part_upload(char *key)
{
    char *unloadId = UPLOAD_ID;

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler responseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };

    abort_multi_part_upload(&option, key, unloadId,&responseHandler, 0);

    if (statusG == OBS_STATUS_OK) {
        printf("abort_multi_part_upload  successfully. \n");
    }
    else
    {
        printf("abort_multi_part_upload  failed.\n");
        printError();
    }
}

typedef struct list_parts_callback_data
{
    int isTruncated;
    char initiatorId[1024];
    char initiatorDisplayName[1024];
    char ownerId[1024];
    char ownerDisplayName[1024];    
    char nextPartNumberMarker[256];
    char storageClass[64];
    
    int keyCount;
    int allDetails;
}list_parts_callback_data;


static obs_status listPartsCallbackEx(obs_uploaded_parts_total_info* uploadedParts,
                                      obs_list_parts *parts,
                                      void *callbackData)
{
    list_parts_callback_data *data = 
        (list_parts_callback_data *) callbackData;

    FILE * fp = fopen("partsInfo","w");
    char buffToWrite[256] = {0};
    data->isTruncated = uploadedParts->is_truncated;

    unsigned int nextPartNumberMarker = uploadedParts->nextpart_number_marker;
    int partsCount = uploadedParts->parts_count;
    
    if ((0 == nextPartNumberMarker) && partsCount) {
        nextPartNumberMarker = parts[partsCount - 1].part_number;
    }
    if (nextPartNumberMarker) {
        snprintf_s(data->nextPartNumberMarker, sizeof(data->nextPartNumberMarker), 
				sizeof(data->nextPartNumberMarker)-1,"%s",nextPartNumberMarker);
    }
    else {
        data->nextPartNumberMarker[0] = 0;
    }

    printf("initializeId: %s\n", uploadedParts->initiator_id);
    printf("initiatorDisplayName: %s\n",uploadedParts->initiator_display_name);
    printf("ownerId: %s\n",uploadedParts->owner_id);
    printf("ownerDisplayName: %s\n",uploadedParts->owner_display_name);
    printf("IsTruncated : %u\n", uploadedParts->is_truncated);
    printf("NextPartNumberMarker : %u\n", nextPartNumberMarker);
    printf("Storage Class is : %s\n", uploadedParts->storage_class);

    int i;
    for (i = 0; i < partsCount; i++) {
        const obs_list_parts *part = &(parts[i]);
        char timebuf[256] = {0};

        time_t t = (time_t) part->last_modified;
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", 
                gmtime(&t));
        char sizebuf[16] = {0};
        if (part->size < 100000) {
            sprintf_s(sizebuf, sizeof(sizebuf),"%5llu", (unsigned long long) part->size);
        }
        else if (part->size < (1024 * 1024)) {
            sprintf_s(sizebuf, sizeof(sizebuf),"%4lluK", 
                    ((unsigned long long) part->size) / 1024ULL);
        }
        else if (part->size < (10 * 1024 * 1024)) {
            float f = part->size;
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf),"%1.2fM", f);
        }
        else if (part->size < (1024 * 1024 * 1024)) {
            sprintf_s(sizebuf, sizeof(sizebuf),"%4lluM", 
                    ((unsigned long long) part->size) / 
                    (1024ULL * 1024ULL));
        }
        else {
            float f = (part->size / 1024);
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf),"%1.2fG", f);
        }
    printf("-----------------------------------RESULT BEG------------------------------\n");
    printf("PartNumber : %u\n", part->part_number);
    printf("LastModified : %s\n", timebuf);
    printf("ETag : %s\n", part->etag);
    printf("Size : %s\n", sizebuf);
    printf("-----------------------------------RESULT END------------------------------\n");
    printf("\n");   
        if(fp)
        {
           sprintf_s(buffToWrite,sizeof(buffToWrite),"%u/%s\n",part->part_number,part->etag);
           fputs(buffToWrite,fp);
        }
    }
    if(fp)
    {
       fclose(fp);
       fp = NULL;
    }

    printf("partsCount : %d\n", partsCount);

    data->keyCount += partsCount;

    return OBS_STATUS_OK;

}

static void test_list_parts(char *key)
{
    list_part_info listpart;
    listpart.upload_id = UPLOAD_ID;
    listpart.max_parts = 2;
    listpart.part_number_marker = 0;
    
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_list_parts_handler Handler =
    { 
        {&response_properties_callback, &response_complete_callback },
        &listPartsCallbackEx
    };

    list_parts_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(list_parts_callback_data));
    
    do{
        list_parts(&option,key,&listpart, &Handler,&data);
        if(statusG != OBS_STATUS_OK)
        {
            break;
        }
    }while (data.isTruncated && (listpart.max_parts > 0 || (data.keyCount < listpart.max_parts)));
    
    if (statusG == OBS_STATUS_OK) 
    {
        printf("ListParts OK\n");
    }
    else 
    {
        printError();
    }
}

static void test_copy_part()
{
    char *key = "testXXX";
    char etagreturn[256] ={0};
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
	option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;

	//Server encryption
    /*SSE-KMS encryption*/
    server_side_encryption_params encryption_params;
    memset_s(&encryption_params, sizeof(encryption_params),0, sizeof(server_side_encryption_params));
    /*encryption_params.use_kms = '1';
    encryption_params.kms_server_side_encryption = "aws:kms";
    //This parameter can be NULL the default system key
    encryption_params.kms_key_id = "sichuan:domainiddomainiddomainiddoma0001:key/xxxxxxxxxxxxxxx";*/

    /*SSE-C*/
    /*char* buffer = "xxxxxxxxxxxxx";
    encryption_params.use_ssec = '1';
    encryption_params.ssec_customer_algorithm = "AES256";
    encryption_params.ssec_customer_key = buffer;
    
    //Decrypt source object parameters; used to copy one encrypted object to another object
    char *des_key = "xxxxxxxxxxxxx";
    encryption_params.des_ssec_customer_algorithm = "AES256";
    encryption_params.des_ssec_customer_key = des_key;*/

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);

    obs_copy_destination_object_info object_info;
    memset_s(&object_info, sizeof(object_info),0, sizeof(obs_copy_destination_object_info));
    object_info.destination_bucket = "esdk-c-test";
    object_info.destination_key = "testXXX.txt";
    object_info.etag_return = etagreturn;
    object_info.etag_return_size = 256;

    obs_upload_part_info copypart;
    memset_s(&copypart, sizeof(copypart),0, sizeof(obs_upload_part_info));

    obs_response_handler responseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    // copy first part
    copypart.part_number = 1;
    copypart.upload_id = UPLOAD_ID;
    copy_part(&option, key, &object_info, &copypart, 
              &putProperties,&encryption_params,&responseHandler, 0);

    if (statusG == OBS_STATUS_OK) {
        printf(" copy part 1 successfully. \n");
    }
    else
    {
        printf("copy part 1 failed.\n");
        printError();
    }

    // copy first part
    copypart.part_number = 2;
    copypart.upload_id = UPLOAD_ID;
    copy_part(&option, key, &object_info, &copypart, 
              &putProperties,&encryption_params,&responseHandler, 0);

    if (statusG == OBS_STATUS_OK) {
        printf(" copy part 2 successfully. \n");
    }
    else
    {
        printf("copy part 2 failed.\n");
        printError();
    }
}

void uploadFileResultCallback(obs_status status,
                              char *resultMsg,
                              int partCountReturn,
                              obs_upload_file_part_info * uploadInfoList,
                              void *callbackData)
{
    int i=0;
    obs_upload_file_part_info * pstUploadInfoList = uploadInfoList;
    printf("status return is %d\n",status);
    printf("%s",resultMsg);
    printf("partCount[%d]\n",partCountReturn);
    for(i=0;i<partCountReturn;i++)
    {
        printf("partNum[%d],startByte[%llu],partSize[%llu],status[%d]\n",
        pstUploadInfoList->part_num,
        pstUploadInfoList->start_byte,
        pstUploadInfoList->part_size,
        pstUploadInfoList->status_return);
        pstUploadInfoList++;
    }
}


static void test_upload_file(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    printf("bucket name is: %s\n", bucket_name);

    char *key = argv[optindex++];
    printf("key is: %s\n", key);

    char *filename = argv[optindex++];
    filename += FILENAME_PREFIX_LEN;
    printf("filename is: %s\n", filename);

    uint64_t uploadSliceSize =5L * 1024 * 1024;                   // upload part slice size
    int pause_upload_flag = 0;
    obs_options option;
    init_obs_options(&option);

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);


    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    option.request_options.server_cert_path = NULL;    //set server cert , example: /etc/certs/cabundle.pem


    obs_upload_file_configuration uploadFileInfo;
    memset_s(&uploadFileInfo,sizeof(obs_upload_file_configuration),0,sizeof(obs_upload_file_configuration));
    uploadFileInfo.check_point_file = NULL;
    uploadFileInfo.enable_check_point = 0;
    uploadFileInfo.part_size = uploadSliceSize;
    uploadFileInfo.task_num = 1;
    uploadFileInfo.upload_file = filename;
    uploadFileInfo.pause_upload_flag = &pause_upload_flag;

    while (optindex < argc){
        char *param = argv[optindex++];
        if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            putProperties.canned_acl = get_acl_from_argv(param);
        }else if(!strncmp(param, THREAD_NUM_PREFIX, THREAD_NUM_PREFIX_LEN)){
            char* thread_num = &(param[THREAD_NUM_PREFIX_LEN]);
            uploadFileInfo.task_num = atoi(thread_num);
        }else if (!strncmp(param, CHECK_POINT, CHECK_POINT_LEN))
        {
            char* check_point = &(param[CHECK_POINT_LEN]);
            uploadFileInfo.enable_check_point = atoi(check_point);            
        }else if (!strncmp(param, UPLOAD_SLICE_SIZE, UPLOAD_SLICE_SIZE_LEN))
        {
            uploadFileInfo.part_size = convertInt(&(param[UPLOAD_SLICE_SIZE_LEN]), "uploadSliceSize");
        }

    }
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
        {&response_properties_callback,
        &response_complete_callback},
        &uploadFileResultCallback
    };
    cJSON_Delete(body);
    cJSON_free(out);
    upload_file(&option, key, 0, &uploadFileInfo, server_callback, &Handler, 0);
    if (statusG == OBS_STATUS_OK) {
        printf("test upload file successfully. \n");
    }
    else
    {
    printf("test upload file faied.\n");
    printError();
    return ;
    }
}

void downloadFileResultCallback(obs_status status,
                                char *resultMsg,
                                int partCountReturn,
                                obs_download_file_part_info * downloadInfoList,
                                void *callbackData)
{
    int i=0;
    obs_download_file_part_info * pstDownloadInfoList = downloadInfoList;
    printf("status return is %d\n",status);
    printf("%s",resultMsg);
    printf("partCount[%d]\n",partCountReturn);
    for(i=0;i<partCountReturn;i++)
    {
        printf("partNum[%d],startByte[%llu],partSize[%llu],status[%d]\n",
        pstDownloadInfoList->part_num,
        pstDownloadInfoList->start_byte,
        pstDownloadInfoList->part_size,
        pstDownloadInfoList->status_return);
        pstDownloadInfoList++;
    }
}


static void test_download_file(char *filename, char *key)
{
    uint64_t uploadSliceSize =5L * 1024 * 1024;                   // upload part slice size

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_get_conditions getConditions;
    memset_s(&getConditions,sizeof(obs_get_conditions),0,sizeof(obs_get_conditions));
    init_get_properties(&getConditions);
    
    obs_download_file_configuration downloadFileConfig;
    memset_s(&downloadFileConfig,sizeof(obs_download_file_configuration),0,sizeof(obs_download_file_configuration));
    downloadFileConfig.check_point_file = NULL;
    downloadFileConfig.enable_check_point = 1;
    downloadFileConfig.part_size = uploadSliceSize;
    downloadFileConfig.task_num = 10;
    downloadFileConfig.downLoad_file= filename;

    obs_download_file_response_handler Handler =
    { 
        {&response_properties_callback,
      &response_complete_callback },
        &downloadFileResultCallback
    };

    download_file(&option, key, 0,&getConditions,0,&downloadFileConfig,&Handler, 0);
    if (statusG == OBS_STATUS_OK) {
        printf("test download file successfully. \n");
    }
    else
    {
        printf("test download file faied.\n");
        printError();
    }
}

static obs_status DeleteObjectsDataCallback(int contentsCount, 
                                            obs_delete_objects *delobjs,
                                            void *callbackData)
{
    (void)callbackData;
    int i;
    for (i = 0; i < contentsCount; i++) {
        const obs_delete_objects*content = &(delobjs[i]);
        int iRet = atoi(content->code);
        if(0 != iRet)
        {
            printf("delete object result:\nobject key:%s\nerror code:%s\nerror message:%s\n", content->key, content->code, content->message);
        }
        else
        {
            printf("delete object result:\nobject key:%s\nerror code:%s\n", content->key, content->code);
        }            
    }
    return OBS_STATUS_OK;
}

typedef struct _test_concurrent_upload_part_callback_data
{
    FILE *infile;
    char etag[1024];
    char *upload_id;
    unsigned int part_num;
    uint64_t part_size;
    uint64_t start_byte;
    obs_options *option;
    char * key;
}test_concurrent_upload_part_callback_data;


static obs_status concurrent_response_properties_callback(const obs_response_properties *properties, void *callback_data)
{
    test_concurrent_upload_part_callback_data *concurrent_callback_data =
        (test_concurrent_upload_part_callback_data*)callback_data;

    if (!showResponsePropertiesG) {
        return OBS_STATUS_OK;
    }

#define print_nonnull(name, field)                                 \
    do {                                                           \
        if (properties-> field) {                                  \
            printf("%s: %s\n", name, properties-> field);          \
        }                                                          \
    } while (0)
    if(properties->etag)
    {
        strcpy_s(concurrent_callback_data->etag,sizeof(concurrent_callback_data->etag),properties->etag);
    }
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

static int test_concurrent_upload_part_data_callback(int buffer_size, char *buffer,
                                 void *callback_data)
{
    test_concurrent_upload_part_callback_data *data = 
        (test_concurrent_upload_part_callback_data *) callback_data;
    int ret=0;
    fseek(data->infile, data->start_byte, SEEK_SET);
    int toRead = ((data->part_size> (unsigned) buffer_size) ?
                    (unsigned) buffer_size : data->part_size);
    ret = fread(buffer, 1, toRead, data->infile);

    return ret;
}

void upload_thread_proc(void * thread_param)
{
    
    test_concurrent_upload_part_callback_data *concurrent_temp = (test_concurrent_upload_part_callback_data *)thread_param;
    
    obs_upload_part_info uploadPartInfo;
    uploadPartInfo.part_number = concurrent_temp->part_num;
    uploadPartInfo.upload_id = concurrent_temp->upload_id;
    obs_upload_handler uploadHandler =
    { 
        {&concurrent_response_properties_callback,
        &response_complete_callback},
        &test_concurrent_upload_part_data_callback
    };
    upload_part(concurrent_temp->option,concurrent_temp->key,&uploadPartInfo,
                concurrent_temp->part_size,0,0,&uploadHandler,concurrent_temp);
    if (statusG == OBS_STATUS_OK) {
        printf("test upload part %u successfully. \n", uploadPartInfo.part_number);
    }
    else
    {
        printf("test upload part %u faied.\n",uploadPartInfo.part_number);
        printError();
        return ;
    }
	 //_endthread(); 
}

static void start_upload_threads(test_upload_file_callback_data data,
                                 char *concurrent_upload_id, uint64_t filesize, char *key,
                                 obs_options option,test_concurrent_upload_part_callback_data *concurrent_upload_file)
{
     int partCount = data.part_num;
     test_concurrent_upload_part_callback_data *concurrent_temp;
     concurrent_temp = concurrent_upload_file;
     int i= 0;  
     for(i=0; i <partCount; i++)
     {
        memset_s(concurrent_temp[i].etag, sizeof(concurrent_temp[i].etag),0,sizeof(concurrent_temp[i].etag));
        concurrent_temp[i].part_num = i + 1;
        concurrent_temp[i].infile = data.infile;
        concurrent_temp[i].upload_id = concurrent_upload_id;
        concurrent_temp[i].key = key;
        concurrent_temp[i].option = &option;
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
     
     HANDLE * arrHandle = (HANDLE *)malloc(sizeof(HANDLE)*partCount);
     for(i = 0; i <partCount;i++)
     {  
        arrHandle[i] = (HANDLE)_beginthread(upload_thread_proc,0,&concurrent_upload_file[i]); 
     }

     for(i=0;i<partCount;i++)
     {
        WaitForSingleObject(arrHandle[i],INFINITE);
     }

	 printf("partCount=%d\n",partCount);
	 for(i = 0; i <partCount;i++)
     {
 		FindClose(arrHandle[i]);
	 }
     if(arrHandle)
     {
        free(arrHandle);
        arrHandle = NULL;
     }
}



static void test_concurrent_upload_part(int argc, char **argv, int optindex)
{
    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);

    char *key = argv[optindex++];
    printf("key is == %s \n", key);

    char *filename = argv[optindex++];
    printf("filename is == %s \n", filename);

    char *uploadSliceSize = argv[optindex++];
    printf("uploadSliceSize is == %s \n", uploadSliceSize);

    char concurrent_upload_id[2048]={0};
    uint64_t uploadSize = convertInt(uploadSliceSize, "uploadSliceSize");                         // upload part size
    uint64_t filesize =0; 
    int i=0;
    // file total size

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);
    putProperties.canned_acl = OBS_CANNED_ACL_BUCKET_OWNER_FULL_CONTROL;


    while (optindex < argc){
        char *param = argv[optindex++];
        if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            putProperties.canned_acl = get_acl_from_argv(param);
        }
    } 

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_response_handler Handler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };
        
    obs_complete_multi_part_upload_handler complete_multi_handler =
    { 
        {&response_properties_callback,
         &response_complete_callback},
        &CompleteMultipartUploadCallback
    };
        
    
    //Large file information: file pointer, file size, number of segments according to segment size
    test_upload_file_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(test_upload_file_callback_data));
    if (filesize == 0)
    {
        filesize = get_file_info(filename,&data);
    }
    data.noStatus = 1;
    data.part_size = uploadSize;
    data.part_num = (filesize % uploadSize == 0) ? (filesize / uploadSize) : (filesize / uploadSize +1);

    //Initialize the upload segment task to return uploadId: uploadIdReturn
    char uploadIdReturn[256] = {0};
    int upload_id_return_size = 255;
    initiate_multi_part_upload(&option,key,upload_id_return_size,uploadIdReturn, &putProperties,0,&Handler, 0);
    if (statusG == OBS_STATUS_OK) {
        printf("test init upload part successfully. \n");
        strcpy_s(concurrent_upload_id,sizeof(concurrent_upload_id),uploadIdReturn);
    }
    else
    {
        printf("test init upload part faied.\n");
        printError();
    }
    //Concurrent upload segment
    test_concurrent_upload_part_callback_data *concurrent_upload_file;
    concurrent_upload_file =(test_concurrent_upload_part_callback_data *)malloc(sizeof(test_concurrent_upload_part_callback_data)*(data.part_num + 1));
    if(concurrent_upload_file == NULL)
    {
        printf("malloc test_concurrent_upload_file_callback_data failed!!!\n");
        return ;
    }
    test_concurrent_upload_part_callback_data *concurrent_upload_file_complete = concurrent_upload_file;
    start_upload_threads(data, concurrent_upload_id,filesize,key, option, concurrent_upload_file_complete);

    // Merging segment
    obs_complete_upload_Info *upload_Info=(obs_complete_upload_Info *)malloc(sizeof(obs_complete_upload_Info)*(data.part_num));
	if(upload_Info == NULL)
    {
        printf("malloc obs_complete_upload_Info failed!!!\n");
        return ;
    }
    for(i=0; i<data.part_num; i++)
    {
        upload_Info[i].part_number = concurrent_upload_file_complete[i].part_num;
        upload_Info[i].etag = concurrent_upload_file[i].etag;
    }
    complete_multi_part_upload(&option,key,uploadIdReturn, data.part_num,upload_Info,&putProperties,&complete_multi_handler,0);
    if (statusG == OBS_STATUS_OK) {
        printf("test complete upload successfully. \n");
    }
    else
    {
        printf("test complete upload faied.\n");
        printError();
    }
    if(concurrent_upload_file)
    {
        free(concurrent_upload_file);
        concurrent_upload_file = NULL;
    }
	if(upload_Info)
    {
        free(upload_Info);
        upload_Info = NULL;
    }
}

static void test_append_object_from_file(char *key, char *file_name, char * position)
{
    uint64_t content_length = 0;
    
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    //read from local file to buffer
    put_file_object_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(put_file_object_callback_data));
    data.infile = 0;
    content_length = open_file_and_get_length(file_name, &data);
    printf("content_length ; = %d\n",content_length);
    obs_append_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_file_complete_callback },
        &put_file_data_callback
    };
    
    append_object(&option,key,content_length,position,&put_properties,0,&putobjectHandler,&data);

    if (OBS_STATUS_OK == data.ret_status) {
        printf("append object from file successfully. \n");
    }
    else
    {
        printf("append object failed(%s).\n", obs_get_status_name(data.ret_status));
    }
}

static void test_append_object_from_buffer(int argc, char **argv, int optindex)
{

    obs_options option;
    init_obs_options(&option);

    obs_put_properties put_properties;
    init_put_properties(&put_properties);

    char *bucket_name = argv[optindex++];
    printf("Bucket's name is == %s \n", bucket_name);

    char *key = argv[optindex++];
    printf("key is == %s \n", key);

    char *buffer = argv[optindex++];
    printf("buffer is == %s \n", buffer);

    char *position = argv[optindex++];
    printf("position is == %s \n", position);

    uint64_t buffer_size = strlen(buffer);
    printf("buffer_size = %d\n",buffer_size);


    while (optindex < argc) {
        char *param = argv[optindex++];
        if (!strncmp(param, WEBSITE_REDIRECT_LOCATION_PREFIX, WEBSITE_REDIRECT_LOCATION_PREFIX_LEN)) {
            put_properties.website_redirect_location = &(param[WEBSITE_REDIRECT_LOCATION_PREFIX_LEN]);
        }else if (!strncmp(param, CANNED_ACL_PREFIX, CANNED_ACL_PREFIX_LEN)) {
            put_properties.canned_acl = get_acl_from_argv(param);
        }
        else if (!strncmp(param, STORAGE_CLASS_PREFIX, STORAGE_CLASS_PREFIX_LEN)) {
            option.bucket_options.storage_class = get_storage_class_from_argv(param);
        }else if(!strncmp(param, EXPIRES_PREFIX, EXPIRES_PREFIX_LEN)){
            char *expires = &(param[EXPIRES_PREFIX_LEN]);
            put_properties.expires = convertInt(expires, "expires");
        }else if (!strncmp(param, X_AMZ_META_PREFIX, X_AMZ_META_PREFIX_LEN))
        {
            
        }

    }
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    


    //Initialize the structure that stores the uploaded data
    put_buffer_object_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(put_buffer_object_callback_data));
    // Assign buffer to the uploaded data structure
    data.put_buffer = buffer;
    // Set buffersize
    data.buffer_size = buffer_size;

    obs_append_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_buffer_complete_callback },
          &put_buffer_data_callback
    };
    
    append_object(&option,key,buffer_size,position,&put_properties,0,&putobjectHandler,&data);

    if (OBS_STATUS_OK == data.ret_status) {
        printf("append object from buffer successfully. \n");
    }
    else
    {
        printf("append object from buffer failed(%s).\n",
            obs_get_status_name(data.ret_status));
    }
}

// create bucket
static void test_gen_signed_url_create_bucket(obs_canned_acl canned_acl, char *bucket_name)
{
    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    

    
    temp_auth_configure tempauth;
    
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(tempAuthResult),0,sizeof(tempAuthResult));
    tempauth.callback_data = (void *)(&ptrResult);   
    tempauth.expires = 10;
    tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
    option.temp_auth = &tempauth;
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    create_bucket(&option, canned_acl, NULL, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("gen create bucket url successfully. \n");
    }
    else
    {
        printf("gen create bucket url failed(%s).\n", obs_get_status_name(ret_status));
    }

}
static void test_gen_signed_url_put_object()
{
    // name of upload object
    char *key = "put_file_test";
    // file to upload
    char file_name[256] = "C:\\a.txt";
    uint64_t content_length = 0;

    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    // Initialize upload object properties
    obs_put_properties put_properties;
    init_put_properties(&put_properties);

    // Initialize the structure that stores the uploaded data
    put_file_object_callback_data data;
    memset_s(&data, sizeof(data),0, sizeof(put_file_object_callback_data));
    // Open the file and get the file length
    content_length = open_file_and_get_length(file_name, &data);

    // Set callback function
    obs_put_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_file_complete_callback },
        &put_file_data_callback
    };
    
    temp_auth_configure tempauth;
    
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(tempAuthResult),0,sizeof(tempAuthResult));
    tempauth.callback_data = (void *)(&ptrResult);   
    tempauth.expires = 10;
    tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
    option.temp_auth = &tempauth;
    

    put_object(&option, key, content_length, &put_properties, 0, &putobjectHandler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("gen put object url successfully. \n");
    }
    else
    {
        printf("gen put object url failed(%s).\n",  
               obs_get_status_name(data.ret_status));
    }

}

static void test_gen_signed_url_get_object(char *key, char *versionid)
{
    char *file_name = "C:\\a.txt";
    obs_object_info object_info;
    memset_s(&object_info, sizeof(object_info),0, sizeof(obs_object_info));
    object_info.key =key;
    object_info.version_id = versionid;

    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    
    
    get_object_callback_data data;
    data.ret_status = OBS_STATUS_BUTT;
    data.outfile = write_to_file(file_name);

    obs_get_conditions getcondition;
    memset_s(&getcondition, sizeof(getcondition),0, sizeof(obs_get_conditions));
    init_get_properties(&getcondition);
    getcondition.start_byte = 0;
    getcondition.byte_count = 100;
    //getcondition.image_process_config.image_process_mode = obs_image_process_cmd;
    //getcondition.image_process_config.cmds_stylename = "resize,m_fixed,w_100,h_100/rotate,90";
    //getcondition.if_match_etag = <etag>;
    //getcondition.if_modified_since= <mkdified>;
    //getcondition.if_not_match_etag = <not_etag>;
    //getcondition.if_not_modified_since = <not_mkdified>;

    obs_get_object_handler getobjectHandler =
    { 
        { &response_properties_callback,
          &get_object_complete_callback},
        &get_object_data_callback
    };

    temp_auth_configure tempauth;
    
    tempAuthResult  ptrResult;
    memset_s(&ptrResult,sizeof(tempAuthResult),0,sizeof(tempAuthResult));
    tempauth.callback_data = (void *)(&ptrResult);   
    tempauth.expires = 10;
    tempauth.temp_auth_callback = &tempAuthCallBack_getResult;
    option.temp_auth = &tempauth;
    
    get_object(&option,&object_info,&getcondition,0,&getobjectHandler,&data);
    fclose(data.outfile);
    if (statusG == OBS_STATUS_OK) {
        printf("get object successfully. \n");
    }
    else
    {
        printf("get object faied.\n");
        printError();
    }


}

static double g_progress = 0;
void test_progress_callback(double progress, uint64_t uploadedSize, uint64_t fileTotalSize, void *callback_data) {
	if (progress == 100 || (g_progress < progress && progress - g_progress > 2)) {
		printf("test_progress_callback progress=%f  uploadedSize=%u fileTotalSize=%lu  callback_data=%p\n", progress, uploadedSize, fileTotalSize, callback_data);
		g_progress = progress;
	}
}
typedef struct pause_concurrent_upload_file
{
    char *bucket;
    char *key;
    char *file_name;
    int *pause_upload_flag;
} pause_concurrent_upload_file;

unsigned int __stdcall test_pause_concurrent_upload_file(void *param)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    pause_concurrent_upload_file *pause_param = (pause_concurrent_upload_file*)param;

    uint64_t uploadSliceSize =40L * 1024 * 1024;           // upload part slice size
    obs_options option;
    init_obs_options(&option);
    obs_put_properties putProperties = { 0 };
    init_put_properties(&putProperties);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = pause_param->bucket;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.request_options.auth_switch = OBS_OBS_TYPE;
    option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;

    obs_upload_file_configuration uploadFileInfo;
    memset_s(&uploadFileInfo, sizeof(obs_upload_file_configuration), 0, sizeof(obs_upload_file_configuration));
    uploadFileInfo.check_point_file = NULL;
    uploadFileInfo.enable_check_point = 0;
    uploadFileInfo.part_size = uploadSliceSize;
    uploadFileInfo.task_num = 1;
    uploadFileInfo.upload_file = pause_param->file_name;
    uploadFileInfo.pause_upload_flag = pause_param->pause_upload_flag;

    obs_upload_file_server_callback server_callback;
    init_server_callback(&server_callback);
    server_callback.callback_url = "http://xx.xx.xx.xx:port/backend/v1/json";
    server_callback.callback_host = NULL;
    server_callback.callback_body = "{\"vid\":\"0d8b255141eb7fb860bb9ae3b3649f65\",\"bucket\":\"$(bucket)\",\"keepsource\":\"0\",\"size\":\"$(fsize)\",\"etag\":\"$(etag)\",\"userid\":\"0d8b255141\",\"fileid\":\"1648624028591";
    server_callback.callback_body_type = "application/json";

    obs_upload_file_response_handler Handler =
    {
        {&response_properties_callback,
        &response_complete_callback},
        &uploadFileResultCallback,
        &test_progress_callback
    };

    upload_file(&option, pause_param->key, 0, &uploadFileInfo, server_callback, &Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test upload file successfully. \n");
    }
    else
    {
        printf("test upload file faied(%s).\n", obs_get_status_name(ret_status));
    }
    return ret_status;
}

static obs_status test_pause_upload_file(int argc, char ** argv, int optindex)
{
    int pause_upload_flag1 = 0;
    int pause_upload_flag2 = 0;
    int task_num = 2;

    pause_concurrent_upload_file param[2];
    char *bucket_name = argv[optindex++];
    printf("bucket name is: %s\n", bucket_name);
    char *key1 = argv[optindex++];
    char *key2 = argv[optindex++];
    printf("key1 is: %s, key2 is: %s\n", key1, key2);
    char *filename1 = argv[optindex++];
    char *filename2 = argv[optindex++];
    printf("filename1 is: %s, filename2 is: %s\n", filename1, filename2);
    param[0].bucket = bucket_name;
    param[0].key = key1;
    param[0].file_name = filename1;
    param[0].pause_upload_flag = &pause_upload_flag1;
    param[1].bucket = bucket_name;
    param[1].key = key2;
    param[1].file_name = filename2;
    param[1].pause_upload_flag = &pause_upload_flag2;

    HANDLE *arrHandle = (HANDLE*)malloc(sizeof(HANDLE) * task_num);
    unsigned uiThread2ID;
    DWORD   dwExitCode = 0;
    int err = 1;
    int i = 0;

    for (i = 0; i < task_num; i++) {
        arrHandle[i] = (HANDLE)_beginthreadex(NULL, 0, test_pause_concurrent_upload_file, (void*)&param[i], CREATE_SUSPENDED, &uiThread2ID);
        if (0 == arrHandle[i]) {
            GetExitCodeThread(arrHandle[i], &dwExitCode);
            printf("create thread failed. error is %u\n", dwExitCode);
        }
    }

    for (i = 0; i < task_num; i++) {
        ResumeThread(arrHandle[i]);
    }

    Sleep(60000);
    pause_upload_file(param[0].pause_upload_flag);
    printf("has call pause_upload_file.\n");

    for (i = 0; i < task_num; i++) {
        err = WaitForSingleObject(arrHandle[i], INFINITE);
        if (err != 0) {
            printf("WaitForSingleObject failed. err:%d\n", err);
        }
    }
    for (i = 0; i < task_num; i++) {
        CloseHandle(arrHandle[i]);
    }
    if (arrHandle) {
        free(arrHandle);
        arrHandle = NULL;
    }
    printf("test_pause_upload_file success.\n");
    return OBS_STATUS_OK;
}

int main(int argc, char **argv)
{
    int optind =1;
    strcpy_s(ACCESS_KEY_ID,sizeof(ACCESS_KEY_ID),"xxxxxxxxxxxxxxxxx");
    strcpy_s(SECRET_ACCESS_KEY,sizeof(SECRET_ACCESS_KEY),"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");   
    strcpy_s(HOST_NAME,sizeof(HOST_NAME),"xx.xx.xx.xx");  
 
    /*--------------check argv-------------*/
    if (optind == argc) {
        printf("optind = %d\n", optind);
        printf("argc = %d\n", argc);
        fprintf(stderr, "\n\nERROR: Missing argument: command\n\n");
    }
    const char *command = argv[optind++];
    printf("command = %s\n", command);

    set_obs_log_path("D:\\log", false);                   //此行代码用于示例设置日志文件路径功能，在实际使用中请注释该行
    obs_initialize(OBS_INIT_ALL);

	if (!strcmp(command, "create_bucket")) {
       test_create_bucket(argc, argv, optind);
    }

    if (!strcmp(command, "delete_bucket")) {
       test_delete_bucket(argc, argv, optind);
    }

    if (!strcmp(command, "head_bucket")) {
       test_head_bucket(argc, argv, optind);
    }

    if (!strcmp(command, "head_object")) {
       test_head_object(argc, argv, optind);
    }

    if (!strcmp(command, "put_object_from_buffer")) {
       test_put_object_from_buffer(argc, argv, optind);
    }

    if (!strcmp(command, "put_object_from_file")) {
       test_put_object_from_file(argc, argv, optind);
    }

    if (!strcmp(command, "get_object")) {
       test_get_object(argc, argv, optind);
    }

    if (!strcmp(command, "delete_object")) {
        test_delete_object(argc, argv, optind);
    }

    if (!strcmp(command, "copy_object")) {
        test_copy_object(argc, argv, optind);
    }

    if (!strcmp(command, "restore_object")) {
        test_restore_object(argc, argv, optind);
    }

    if (!strcmp(command, "list_bucket_object")) {
        test_list_bucket_objects(argc, argv, optind);
    }

    if (!strcmp(command, "concurrent_upload_part")) {
        test_concurrent_upload_part(argc, argv, optind);
    }

    if (!strcmp(command, "list_bucket"))
    {
        test_list_bucket();
    }

    if (!strcmp(command, "append_object_from_buffer"))
    {
        test_append_object_from_buffer(argc, argv, optind);
    }

    if (!strcmp(command, "batch_delete_objects"))
    {
        test_batch_delete_objects(argc, argv, optind);
    }

    if (!strcmp(command, "upload_file"))
    {
        test_upload_file(argc, argv, optind);
    }

    if (!strcmp(command, "set_bucket_version"))
    {
        test_set_bucket_version(argc, argv, optind);
    }

    if (!strcmp(command, "get_bucket_version"))
    {
        test_get_bucket_version(argc, argv, optind);
    }

    if (!strcmp(command, "get_bucket_storage_info"))
    {
        test_get_bucket_storage_info(argc, argv, optind);
    }

    if (!strcmp(command, "set_bucket_acl_by_head"))
    {
        test_set_bucket_acl_byhead(argc, argv, optind);
    }

    if (!strcmp(command, "set_bucket_acl"))
    {
        test_set_bucket_acl(argc, argv, optind);
    }

    if (!strcmp(command, "get_bucket_acl"))
    {
        test_get_bucket_acl(argc, argv, optind);
    }

    if (!strcmp(command, "set_object_acl_by_head"))
    {
        test_set_object_acl_byhead(argc, argv, optind);
    }

    if (!strcmp(command, "get_object_acl"))
    {
        test_get_object_acl(argc, argv, optind);
    }

    if (!strcmp(command, "set_object_acl"))
    {
        test_set_object_acl(argc, argv, optind);
    }

    if (!strcmp(command, "set_bucket_tagging"))
    {
        test_set_bucket_tagging(argc, argv, optind);
    }

    if (!strcmp(command, "get_bucket_tagging"))
    {
        test_get_bucket_tagging(argc, argv, optind);
    }

    if (!strcmp(command, "delete_bucket_tagging"))
    {
        test_delete_bucket_tagging(argc, argv, optind);
    }

    if (!strcmp(command, "pause_upload_file")) {
        test_pause_upload_file(argc, argv , optind);
    }

    obs_deinitialize();   
}


