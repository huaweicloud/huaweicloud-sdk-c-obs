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
#include <pthread.h>
#include <unistd.h>
#else
#include "getopt.h"
#endif

#include "eSDKOBS.h"
#include "demo_common.h"
#include "securec.h"

obs_status statusG = OBS_STATUS_OK;
int  showResponsePropertiesG = 1;
char errorDetailsG[4096] = { 0 };
char locationconstraint[2048]={0};
char ACCESS_KEY_ID[2048]={0};
char SECRET_ACCESS_KEY[2048]={0};
char HOST_NAME[2048]={0};
char BUCKET_NAME[2048]={0};
obs_protocol protocolG = OBS_PROTOCOL_HTTP;
obs_canned_acl canned_acl = OBS_CANNED_ACL_BUCKET_OWNER_FULL_CONTROL;
int forceG = 0;
char *ca_file = "./client.pem";
char ca_info[2048] = {0};
int  demoUseObsApi = OBS_USE_API_S3;
char UPLOAD_ETAG[2][256]={{0},{0}};
char OBJECT_VER[2][256]={0};
char UPLOAD_ID[2048]={0};
obs_uri_style gDefaultURIStyle = OBS_URI_STYLE_VIRTUALHOST;

int get_certificate_info(char *buffer, int buffer_length)
{
    int content_length = 0;
    FILE *fp = fopen(ca_file, "r");
    if (fp)
    {
        while(1)
        {
            int rc = fread(buffer, sizeof(char), buffer_length, fp);
            if (rc <= 0)
            {
                break;
            }
            content_length += rc;
        }
        fclose(fp);
    }
    return content_length;
}

void printError()
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

void create_and_write_file(char *filename, unsigned int file_size)
{
    unsigned int i = 0;
    truncate(filename, 0);
    FILE *write_file_test =  write_to_file(filename);
    for (i = 0; i < file_size; i++)
    {
         fwrite("1", 1, 1, write_file_test);
    }
    fclose(write_file_test);
}


obs_status response_properties_callback(const obs_response_properties *properties, void *callback_data)
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
    print_nonnull("expiration", expiration);
    print_nonnull("website_redirect_location", website_redirect_location);
    print_nonnull("version_id", version_id);
    print_nonnull("storage_class", storage_class);
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


void common_error_handle(const obs_error_details *error)
{
    int len = 0;
    if (error && error->message) {
        len += snprintf_s(&(errorDetailsG[len]), sizeof(errorDetailsG) - len, sizeof(errorDetailsG) - len - 1 ,
                        "  Message: %s\n", error->message);
    }
    if (error && error->resource) {
        len += snprintf_s(&(errorDetailsG[len]), sizeof(errorDetailsG) - len, sizeof(errorDetailsG) - len - 1,
                        "  Resource: %s\n", error->resource);
    }
    if (error && error->further_details) {
        len += snprintf_s(&(errorDetailsG[len]), sizeof(errorDetailsG) - len, sizeof(errorDetailsG) - len - 1,
                        "  Further Details: %s\n", error->further_details);
    }
    if (error && error->extra_details_count) {
        len += snprintf_s(&(errorDetailsG[len]), sizeof(errorDetailsG) - len, sizeof(errorDetailsG) - len - 1,
                        "%s", "  Extra Details:\n");
        int i;
        for (i = 0; i < error->extra_details_count; i++) {
            len += snprintf_s(&(errorDetailsG[len]), 
                            sizeof(errorDetailsG) - len, sizeof(errorDetailsG) - len - 1,"    %s: %s\n",
                            error->extra_details[i].name,
                            error->extra_details[i].value);
        }
    }
    return;
}

void response_complete_callback(obs_status status,
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
    common_error_handle(error);
}

obs_status head_properties_callback(const obs_response_properties *properties, void *callback_data)
{
    head_object_data *data = (head_object_data *)callback_data;
    data->object_length = properties->content_length;
    printf("  object length: %d \n", data->object_length);
    if(properties->request_id)
        printf("     request id: %s \n", properties->request_id);
    if(properties->request_id2)
        printf("   request id 2: %s \n", properties->request_id2);
    if(properties->version_id)
        printf("     version id: %s \n", properties->version_id);
    if(properties->storage_class)
        printf("  storage class: %s \n", properties->storage_class);
    if(properties->bucket_location)
        printf("bucket location: %s \n", properties->bucket_location);
    if(properties->obs_version)
        printf("    obs version: %s \n", properties->obs_version);
    if(properties->restore)
        printf("        restore: %s \n", properties->restore);
    if(properties->obs_object_type)
        printf("    object type: %s \n", properties->obs_object_type);
    if(properties->obs_next_append_position)
        printf("append position: %s \n", properties->obs_next_append_position);
}

void head_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    head_object_data *data = (head_object_data *)callback_data;
    data->ret_status = status;
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
            snprintf_s(composedId, sizeof(composedId), sizeof(composedId)-1,
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
        if(demoUseObsApi == OBS_USE_API_S3) {
            printf("%-6s  %-90s  %-12s\n", type, id, perm);
        }
        else {
            const char *delivered;
            if (grant->bucket_delivered == BUCKET_DELIVERED_FALSE)
                delivered = "false";
            else
                delivered = "true";
            printf("%-6s  %-90s  %-12s  %-8s\n", type, id, perm, delivered);
        }
    }
}

obs_status get_bucket_storageclass_handler(const char * storage_class, void * callBackData)
{
    printf(" Bucket storage class is: %s\n",storage_class);
    return OBS_STATUS_OK;
}


void printTagInfo(TaggingInfo* infoToPrint)
{
    int i;
    printf(" etag number is %d\n",infoToPrint->tagCount);

    for(i=0;i<infoToPrint->tagCount;i++)
    {
        printf(" key:[%s], value[%s]\n",infoToPrint->taglist[i].key, infoToPrint->taglist[i].value);
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
            memcpy_s(tag_info->taglist[tag_num].key,sizeof(tag_info->taglist[tag_num].key),(&tagging_list[tag_num])->name,strlen((&tagging_list[tag_num])->name)+1);
            memcpy_s(tag_info->taglist[tag_num].value, sizeof(tag_info->taglist[tag_num].value),(&tagging_list[tag_num])->value,strlen((&tagging_list[tag_num])->value)+1);
        }
    }
    printTagInfo(tag_info);
    return OBS_STATUS_OK;
}

void get_tagging_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    TaggingInfo *tagging_info = (TaggingInfo*)callback_data;
    tagging_info->ret_status = status;
}

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

void printListBucketHeader(int allDetails)
{
    printf("%-50s  %-20s  %-5s", 
           "   Key", 
           "   Last Modified", "Size");
    if (allDetails) {
        printf("  %-36s  %-64s  %-20s  %-16s  %-16s", 
               "   ETag", 
               "   Owner ID",
               "Display Name",
               "Storage Class",
               "Type");
    }
    printf("\n");
    printf("--------------------------------------------------  "
           "--------------------  -----");
    if (allDetails) {
        printf("  ------------------------------------  "
               "----------------------------------------------------------------"
               "  --------------------  ----------------  ----------------");
    }
    printf("\n");
}

void printListServiceHeader(int allDetails)
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

void printListServiceObsHeader(int allDetails)
{
    printf("%-56s  %-20s  %-20s", "                         Bucket",
            "      Created",
            "     Location");
    if (allDetails) {
        printf("  %-64s", 
                "                            Owner ID");
    }
    printf("\n");
    printf("--------------------------------------------------------  "
            "--------------------  "
            "--------------------");
    if (allDetails) {
        printf("  -------------------------------------------------");
    }
    printf("\n");
}

obs_status list_objects_callback(int is_truncated, const char *next_marker,
                                   int contents_count, 
                                   const obs_list_objects_content *contents,
                                   int common_prefixes_count,
                                   const char **common_prefixes,
                                   void *callback_data)
{
    list_object_callback_data *data = (list_object_callback_data *) callback_data;

    data->is_truncated = is_truncated;
    // This is tricky.  S3 doesn't return the NextMarker if there is no
    // delimiter.  Why, I don't know, since it's still useful for paging
    // through results.  We want NextMarker to be the last content in the
    // list, so set it to that if necessary.
    if ((!next_marker || !next_marker[0]) && contents_count) {
        next_marker = contents[contents_count - 1].key;
    }
    if (next_marker) {
        snprintf_s(data->next_marker, sizeof(data->next_marker), sizeof(data->next_marker)-1, "%s",
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
            sprintf_s(sizebuf, sizeof(sizebuf), "%4lluK",
                    ((unsigned long long) content->size) / 1024ULL);
        }
        else if (content->size < (10 * 1024 * 1024)) {
            float f = content->size;
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf),"%1.2fM", f);
        }
        else if (content->size < (1024 * 1024 * 1024)) {
            sprintf_s(sizebuf, sizeof(sizebuf), "%4lluM",
                    ((unsigned long long) content->size) / 
                    (1024ULL * 1024ULL));
        }
        else {
            float f = (content->size / 1024);
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf), "%1.2fG", f);
        }
        printf("%-50s  %s  %s", content->key, timebuf, sizebuf);
        if (data->allDetails) {
            printf("  %-36s  %-64s  %-20s  %-16s  %-16s",
                   content->etag, 
                   content->owner_id ? content->owner_id : "",
                   content->owner_display_name ? content->owner_display_name : "",
                   content->storage_class ? content->storage_class:"",
                   content->type ? content->type:""
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

void list_object_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    list_object_callback_data *data = (list_object_callback_data *) callback_data;
    data->ret_status = status;
}

void printListVersionsHeader(int allDetails)
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

obs_status listVersionsCallback(int is_truncated, const char *next_key_marker, const char *next_versionId_marker,
        const obs_list_versions *list_versions, void *callback_data)
{
    list_versions_callback_data *data = 
        (list_versions_callback_data *) callback_data;

    data->is_truncated = is_truncated;

    if ((!next_key_marker || !next_key_marker[0]) && list_versions->versions_count) {
        next_key_marker = list_versions->versions[list_versions->versions_count - 1].key;
    }
    if (next_key_marker) {
        snprintf_s(data->next_key_marker, sizeof(data->next_key_marker), sizeof(data->next_key_marker)-1,"%s", 
                next_key_marker);
    }
    else {
        data->next_key_marker[0] = 0;
    }

    if ((!next_versionId_marker || !next_versionId_marker[0]) && list_versions->versions_count) {
        next_versionId_marker = list_versions->versions[list_versions->versions_count - 1].version_id;
    }
    if (next_versionId_marker) {
        snprintf_s(data->next_versionId_marker, sizeof(data->next_versionId_marker), sizeof(data->next_versionId_marker)-1, "%s",
                next_versionId_marker);
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
            sprintf_s(sizebuf, sizeof(sizebuf), "%1.2fM", f);
        }
        else if (version->size < (1024 * 1024 * 1024)) {
            sprintf_s(sizebuf, sizeof(sizebuf), "%4lluM",
                    ((unsigned long long) version->size) / 
                    (1024ULL * 1024ULL));
        }
        else {
            float f = (version->size / 1024);
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf), "%1.2fG", f);
        }
        printf("%-30s  %s  %s  %s", version->key, timebuf, sizebuf, version->version_id);
        if(i == 0)
            strncpy_s(OBJECT_VER[0],sizeof(OBJECT_VER[0]), version->version_id, 255);
        else if(i == 1)
            strncpy_s(OBJECT_VER[1], sizeof(OBJECT_VER[1]), version->version_id, 255);
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

void list_versions_complete_callback(obs_status status,
        const obs_error_details *error, 
        void *callback_data)
{
    list_versions_callback_data *data = (list_versions_callback_data*)callback_data;
    data->ret_status = status;
}


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
        printf("event_num: %d \n", notification_conf->topic_conf[i].event_num);
        for (j =0; j < notification_conf->topic_conf[i].event_num; j++)
        {
            printf("    event:%d\n",  notification_conf->topic_conf[i].event[j]);
        }
        
        printf("filter_rule_num: %d \n", notification_conf->topic_conf[i].filter_rule_num);
        for (j =0; j < notification_conf->topic_conf[i].filter_rule_num; j++)
        {
            printf("    name:%d, value:%s\n",  
                notification_conf->topic_conf[i].filter_rule[j].name,
                notification_conf->topic_conf[i].filter_rule[j].value);
        }
    }
    return OBS_STATUS_OK;
}

void put_file_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    put_file_object_callback_data *data = (put_file_object_callback_data *)callback_data;
    data->ret_status = status;
}

void put_buffer_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    put_buffer_object_callback_data *data = (put_buffer_object_callback_data *)callback_data;
    data->ret_status = status;
}


int put_file_data_callback(int buffer_size, char *buffer,
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

int put_buffer_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    put_buffer_object_callback_data *data = 
        (put_buffer_object_callback_data *) callback_data;
    
    int toRead = 0;
    if (data->buffer_size) {
        toRead = ((data->buffer_size > (unsigned) buffer_size) ?
                    (unsigned) buffer_size : data->buffer_size);
        memcpy_s(buffer,buffer_size ,data->put_buffer + data->cur_offset, toRead);
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
    const char *body = 0;
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
    data->content_length = content_length;
    return content_length;
}

obs_status get_properties_callback(const obs_response_properties *properties, void *callback_data)
{
    if(properties->request_id)
        printf("     request id: %s \n", properties->request_id);
    if(properties->request_id2)
        printf("   request id 2: %s \n", properties->request_id2);
    if(properties->version_id)
        printf("     version id: %s \n", properties->version_id);
    if(properties->storage_class)
        printf("  storage class: %s \n", properties->storage_class);
    if(properties->bucket_location)
        printf("bucket location: %s \n", properties->bucket_location);
    if(properties->obs_version)
        printf("    obs version: %s \n", properties->obs_version);
    if(properties->restore)
        printf("        restore: %s \n", properties->restore);
    if(properties->obs_object_type)
        printf("    object type: %s \n", properties->obs_object_type);
    if(properties->obs_next_append_position)
        printf("append position: %s \n", properties->obs_next_append_position);
}

int token_bucket = 0;
time_t produce_time = 0;

uint64_t LIMIT_FLOW_MAX_SPEED = 0;
pthread_mutex_t g_mutexThreadGetToken;

obs_status set_online_request_max_rate(uint64_t online_request_rate)
{
    if (online_request_rate <= 0)
    {
        return OBS_STATUS_InvalidParameter;
    }
    
    LIMIT_FLOW_MAX_SPEED = online_request_rate;
    
    return OBS_STATUS_OK;
}

void initialize_get_token_lock()
{
    pthread_mutex_init(&g_mutexThreadGetToken,NULL);
}

void deinitialize_get_token_lock()
{
    pthread_mutex_destroy(&g_mutexThreadGetToken); 
}

void preduce_token()
{
    if(token_bucket == LIMIT_FLOW_MAX_SPEED)
    {
        return;
    }

    int times = 0; //

    if(produce_time == 0)
    {
        produce_time = time(0);
        times = 1;
    }
    else
    {
        time_t cur_time = time(0);
        times = (cur_time - produce_time);
        if(times > 0)
        {
            produce_time = cur_time;
        }
    }

    if(times > 0)
    {
        token_bucket = LIMIT_FLOW_MAX_SPEED;
    }
}

int get_token(int buffer_size)
{
    if(0 == LIMIT_FLOW_MAX_SPEED)
    {
        return 1;
    }
    preduce_token();

    if(token_bucket < buffer_size)
    {
        printf("has token %d  need token %d.\n", token_bucket, buffer_size);
        return 0;
    }

    token_bucket -= buffer_size;
    return 1;
}

obs_status get_object_data_callback(int buffer_size, const char *buffer,
                                      void *callback_data)
{
    //获取令牌，开始写入数据到本地文件
    pthread_mutex_lock(&g_mutexThreadGetToken);
    while(0 == get_token(buffer_size)){
        sleep(1);
    }
    pthread_mutex_unlock(&g_mutexThreadGetToken);
    
    get_object_callback_data *data = (get_object_callback_data *) callback_data;
    size_t wrote = fwrite(buffer, 1, buffer_size, data->outfile);
    return ((wrote < (size_t) buffer_size) ? 
            OBS_STATUS_AbortedByCallback : OBS_STATUS_OK);
}

void get_object_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data)
{
    get_object_callback_data *data = (get_object_callback_data *) callback_data;
    data->ret_status = status;
}

obs_status delete_objects_data_callback(int contentsCount, 
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

obs_status listServiceCallback(const char *owner_id, 
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

obs_status listServiceObsCallback(const char *owner_id, 
                                  const char *bucket_name,
                                  int64_t creationDate,
                                  const char *location,
                                  void *callback_data)
{
    list_service_data *data = (list_service_data *) callback_data;

    if (!data->headerPrinted) {
        data->headerPrinted = 1;
        printListServiceObsHeader(data->allDetails);
    }

    char timebuf[256] = {0};
    if (creationDate >= 0) {
        time_t t = (time_t) creationDate;
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
    }
    else {
        timebuf[0] = 0;
    }

    printf("%-56s  %-20s  %-20s", bucket_name, timebuf, location);
    if (data->allDetails) {
        printf("  %-64s", owner_id ? owner_id : "");
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


void upload_part_from_file(test_upload_file_callback_data *data)
{
    int result = fseek(data->infile, data->start_byte, SEEK_SET);
    printf("result :=%d\n",result);
}

int test_upload_file_data_callback(int buffer_size, char *buffer,
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

int test_concurrent_upload_part_data_callback(int buffer_size, char *buffer,
                                 void *callback_data)
{
    test_concurrent_upload_file_callback_data *data = 
        (test_concurrent_upload_file_callback_data *) callback_data;
    int ret=0;
    fseek(data->infile, data->start_byte + data->offset, SEEK_SET);
    int toRead = ((data->part_size> (unsigned) buffer_size) ?
                    (unsigned) buffer_size : data->part_size);
    ret = fread(buffer, 1, toRead, data->infile);
    data->offset += ret;
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


void upload_part_complete_callback(obs_status status,
                                 const obs_error_details *error, 
                                 void *callback_data)
{
    test_upload_file_callback_data *data = (test_upload_file_callback_data *)callback_data;
    data->ret_status = status;
}


obs_status listPartsCallbackEx(obs_uploaded_parts_total_info* uploadedParts,
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
        data->nextPartNumberMarker = nextPartNumberMarker;
    }
    else {
        data->nextPartNumberMarker = 0;
    }

    printf("initializeId: %s\n", uploadedParts->initiator_id);
    printf("initiatorDisplayName: %s\n",uploadedParts->initiator_display_name);
    printf("ownerId: %s\n",uploadedParts->owner_id);
    printf("ownerDisplayName: %s\n",uploadedParts->owner_display_name);
    printf("IsTruncated : %d\n", uploadedParts->is_truncated);
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
            sprintf_s(sizebuf, sizeof(sizebuf), "%1.2fM", f);
        }
        else if (part->size < (1024 * 1024 * 1024)) {
            sprintf_s(sizebuf, sizeof(sizebuf), "%4lluM",
                    ((unsigned long long) part->size) / 
                    (1024ULL * 1024ULL));
        }
        else {
            float f = (part->size / 1024);
            f /= (1024 * 1024);
            sprintf_s(sizebuf, sizeof(sizebuf), "%1.2fG", f);
        }
        printf("-----------------------------------RESULT BEG------------------------------\n");
        printf("PartNumber : %u\n", part->part_number);
        printf("LastModified : %s\n", timebuf);
        printf("ETag : %s\n", part->etag);
        if(i == 0)
            strncpy_s(UPLOAD_ETAG[0], sizeof(UPLOAD_ETAG[0]),part->etag, 255);
        else if(i == 1)
            strncpy_s(UPLOAD_ETAG[1], sizeof(UPLOAD_ETAG[1]), part->etag, 255);
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

void list_part_complete_callback(obs_status status,
                                 const obs_error_details *error, 
                                 void *callback_data)
{
    list_parts_callback_data *data = (list_parts_callback_data *)callback_data;
    data->ret_status = status;
}

void listMultiPartUploadsCompleteCallback(obs_status status,
                                 const obs_error_details *error, 
                                 void *callback_data)
{
    list_multipart_uploads_callback_data *data = (list_multipart_uploads_callback_data *)callback_data;
    data->ret_status = status;
}

obs_status listMultiPartUploadsCallback(int is_truncated, const char *next_marker,
            const char *next_uploadId_marker, int uploads_count, 
            const obs_list_multipart_upload *uploads, int common_prefixes_count, 
            const char **common_prefixes, void *callback_data)
{
    int i;
    printf("is_truncated : %d\n", is_truncated);
    if(next_marker)
        printf("next_marker : %s\n", next_marker);
    if(next_uploadId_marker)
        printf("next_uploadId_marker : %s\n", next_uploadId_marker);
    printf("uploads_count : %d\n", uploads_count);
    for(i = 0; i < uploads_count; i++) {
        printf("uploads_count[%d] : %s\n", i, uploads[i].upload_id);
        if(i == 0)
            strncpy_s(UPLOAD_ID, sizeof(UPLOAD_ID),uploads[i].upload_id, 2047);
        printf("key[%d] : %s\n", i, uploads[i].key);
    }
    printf("common_prefixes_count : %d\n", common_prefixes_count);
    for(i = 0; i < common_prefixes_count; i++) {
        printf("uploads_count[%d] : %s\n", i, common_prefixes[i]);
    }
    return OBS_STATUS_OK;
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
        printf("partNum[%d],startByte[%lu],partSize[%lu],status[%d]\n",
        pstUploadInfoList->part_num,
        pstUploadInfoList->start_byte,
        pstUploadInfoList->part_size,
        pstUploadInfoList->status_return);
        pstUploadInfoList++;
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
        printf("partNum[%d],startByte[%lu],partSize[%lu],status[%d]\n",
        pstDownloadInfoList->part_num,
        pstDownloadInfoList->start_byte,
        pstDownloadInfoList->part_size,
        pstDownloadInfoList->status_return);
        pstDownloadInfoList++;
    }
}

obs_status concurrent_response_properties_callback(
        const obs_response_properties *properties, void *callback_data)
{
    if (properties == NULL && callback_data != NULL)
    {
        obs_sever_callback_data *data = (obs_sever_callback_data *)callback_data;
        printf("server_callback buf is %s ,len is %d",
            data->buffer, data->buffer_len);
        return OBS_STATUS_OK;
    }
    test_concurrent_upload_file_callback_data *concurrent_callback_data =
        (test_concurrent_upload_file_callback_data*)callback_data;

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

void concurrent_upload_file_complete_callback(obs_status status,
                                 const obs_error_details *error, 
                                 void *callback_data)
{
    test_concurrent_upload_file_callback_data *data = 
            (test_concurrent_upload_file_callback_data *)callback_data;
    data->ret_status = status;
}

obs_status DeleteObjectsDataCallback(int contentsCount, 
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

void tempAuthCallBack_getResult(char *tempAuthUrl, uint64_t tempAuthUrlLen, char *tempAuthActualHeaders,
    uint64_t tempAuthActualHeadersLen, void *callbackData)
{
    if (tempAuthUrl == NULL || tempAuthUrlLen == 0 || tempAuthActualHeaders == NULL || tempAuthActualHeadersLen == 0) {
        printf("null pointer or zero length detected in function: %s ,line: %d", __FUNCTION__, __LINE__);
        return;
    }
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

    logging_message->agency = (char *)malloc(sizeof(char)*OBS_COMMON_LEN_256);
    memset_s(logging_message->agency, OBS_COMMON_LEN_256, 0, OBS_COMMON_LEN_256);
    logging_message->agency_size = OBS_COMMON_LEN_256;

}

void destroy_logging_message(bucket_logging_message *logging_message)
{
    free(logging_message->target_bucket);
    free(logging_message->target_prefix);
    free(logging_message->acl_grants);
    free(logging_message->acl_grant_count);
    free(logging_message->agency);
}

FILE** init_uploadfilepool(FILE **fd, uint64_t part_num, char *filename)
{
    fd = (FILE**)malloc(sizeof(FILE*)*part_num);
    int i = 0;
    for (; i < part_num; i++)
    {
        if (!(fd[i] = fopen(filename, "rb")))
        {
            fprintf(stderr, "\nERROR: Failed to open input file %s: ", filename);
            perror(0);
            exit(-1);
        }
    }
    return fd;
}

void deinit_uploadfilepool(FILE **fd, uint64_t part_num)
{
    int i = 0;
    for (; i < part_num; i++)
    {
        fclose(fd[i]);
    }
    free(fd);
}
