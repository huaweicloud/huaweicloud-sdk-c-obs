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
#ifndef DEMO_COMMON_H
#define DEMO_COMMON_H


extern obs_status statusG;
extern int  showResponsePropertiesG;
extern char errorDetailsG[4096];
extern char locationconstraint[2048];
extern char ACCESS_KEY_ID[2048];
extern char SECRET_ACCESS_KEY[2048];
extern char HOST_NAME[2048];
extern char BUCKET_NAME[2048];
extern char UPLOAD_ID[2048];
extern obs_protocol protocolG;
extern obs_canned_acl canned_acl;
extern int forceG;
extern char *ca_file;
extern char ca_info[2048];
extern int  demoUseObsApi;
extern char UPLOAD_ETAG[][256];
extern char OBJECT_VER[][256];
extern obs_uri_style gDefaultURIStyle;

// struct------------------------------------------
typedef struct head_object_data
{
    obs_status ret_status;
    int object_length;
}head_object_data;


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

typedef struct list_object_callback_data
{
    int is_truncated;
    char next_marker[1024];
    int keyCount;
    int allDetails;
    obs_status ret_status;
} list_object_callback_data;

typedef struct list_bucket_callback_data
{
    int is_truncated;
    char next_marker[1024];
    int keyCount;
    int allDetails;
    obs_status ret_status;
} list_bucket_callback_data;

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

typedef struct growbuffer
{
    int size;
    int start;
    char data[64 * 1024];
    struct growbuffer *prev, *next;
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


typedef struct put_object_callback_data
{
    FILE *infile;
    growbuffer *gb;
    uint64_t content_length, originalContentLength;
    int noStatus;
    obs_status put_status;
} put_object_callback_data;


typedef struct get_object_callback_data
{
    FILE *outfile;
    obs_status ret_status;
}get_object_callback_data;

typedef struct list_service_data
{
    int headerPrinted;
    int allDetails;
    obs_status ret_status;
} list_service_data;

typedef struct test_upload_file_callback_data
{
    FILE *infile;
    int part_num;
    uint64_t part_size;
    uint64_t start_byte;
    int noStatus;
    obs_status ret_status;
} test_upload_file_callback_data;


typedef struct list_parts_callback_data
{
    int isTruncated;
    char initiatorId[1024];
    char initiatorDisplayName[1024];
    char ownerId[1024];
    char ownerDisplayName[1024];    
    unsigned int nextPartNumberMarker;
    char storageClass[64];
    int keyCount;
    int allDetails;
    obs_status ret_status;
}list_parts_callback_data;

typedef struct _test_concurrent_upload_file_callback_data
{
    FILE *infile;
    char etag[1024];
    char *upload_id;
    unsigned int part_num;
    uint64_t part_size;
    uint64_t start_byte;
    uint64_t offset;
    obs_options *option;
    char * key;
    obs_status ret_status;
}test_concurrent_upload_file_callback_data;

typedef struct __tempAuthResult
{
    char tmpAuthUrl[1024];
    char actualHeaders[1024];
}tempAuthResult;

typedef struct list_multipart_uploads_callback_data
{
    obs_status ret_status;
}list_multipart_uploads_callback_data;

// common handle-------------------------------------
void printError();
FILE * write_to_file(char *localfile);
void common_error_handle(const obs_error_details *error);
void create_and_write_file(char *filename, unsigned int file_size);
void print_grant_info(int acl_grant_count,obs_acl_grant *acl_grants);
void printListBucketHeader(int allDetails);
void printListServiceHeader(int allDetails);
void printListVersionsHeader(int allDetails);
uint64_t open_file_and_get_length(char *localfile, put_file_object_callback_data *data);



// callback-----------------------------------
obs_status response_properties_callback(const obs_response_properties *properties, void *callback_data);
void response_complete_callback(obs_status status, const obs_error_details *error, 
                                     void *callback_data);
obs_status head_properties_callback(const obs_response_properties *properties, void *callback_data);
void head_complete_callback(obs_status status, const obs_error_details *error, 
                                     void *callback_data);
obs_status get_bucket_storageclass_handler(const char * storage_class, void * callBackData);
obs_status get_bucket_tagging_callback(int tagging_count, obs_name_value *tagging_list, void *callback_data);
void get_tagging_complete_callback(obs_status status, const obs_error_details *error, 
                                     void *callback_data);
obs_status get_bucket_websiteconf_callback(const char *hostname,
                                const char *protocol,
                                const char *suffix,
                                const char *key,
                                const bucket_website_routingrule *websiteconf,
                                int webdatacount,
                                void *callback_data);
obs_status list_objects_callback(int is_truncated, const char *next_marker,
                                   int contents_count, 
                                   const obs_list_objects_content *contents,
                                   int common_prefixes_count,
                                   const char **common_prefixes,
                                   void *callback_data);
void list_object_complete_callback(obs_status status, const obs_error_details *error, 
                                     void *callback_data);
obs_status listVersionsCallback(int is_truncated, const char *next_key_marker, const char *next_versionId_marker,
        const obs_list_versions *list_versions, void *callback_data);
void list_versions_complete_callback(obs_status status, const obs_error_details *error, 
        void *callback_data);

obs_status getBucketLifecycleConfigurationCallbackEx (obs_lifecycle_conf* bucketLifeCycleConf,
                                unsigned int blccNumber,  void *callback_data);

obs_status get_cors_info_callback(obs_bucket_cors_conf* bucket_cors_conf,
                                 unsigned int bcc_number,
                                void *callback_data);
obs_status get_notification_info_callback(obs_smn_notification_configuration* notification_conf,
                                void *callback_data);

void put_file_complete_callback(obs_status status,
                                     const obs_error_details *error, 
                                     void *callback_data);
void put_buffer_complete_callback(obs_status status, const obs_error_details *error, 
                                     void *callback_data);
int put_file_data_callback(int buffer_size, char *buffer, void *callback_data);
int put_buffer_data_callback(int buffer_size, char *buffer, void *callback_data);
obs_status get_properties_callback(const obs_response_properties *properties, void *callback_data);
obs_status set_online_request_max_rate(uint64_t online_request_rate);
void initialize_get_token_lock();
void deinitialize_get_token_lock();
void preduce_token();
int get_token(int buffer_size);
obs_status get_object_data_callback(int buffer_size, const char *buffer,
                                      void *callback_data);
void get_object_complete_callback(obs_status status, const obs_error_details *error, 
                                     void *callback_data);

obs_status delete_objects_data_callback(int contentsCount, obs_delete_objects *delobjs,
                                            void *callbackData);
obs_status listServiceCallback(const char *owner_id, 
                                    const char *owner_display_name,
                                    const char *bucket_name,
                                    int64_t creationDate, void *callback_data);
obs_status listServiceObsCallback(const char *owner_id, 
                                  const char *bucket_name,
                                  int64_t creationDate,
                                  const char *location,
                                  void *callback_data);
void list_bucket_complete_callback(obs_status status,
                                 const obs_error_details *error, 
                                 void *callback_data);


void upload_part_from_file(test_upload_file_callback_data *data);

int test_upload_file_data_callback(int buffer_size, char *buffer, void *callback_data);
int test_concurrent_upload_part_data_callback(int buffer_size, char *buffer, void *callback_data);


uint64_t get_file_info(char *localfile, test_upload_file_callback_data *data);

obs_status CompleteMultipartUploadCallback(const char *location, 
                                         const char *bucket,
                                         const char *key,
                                         const char* eTag,
                                         void *callbackData);
void upload_part_complete_callback(obs_status status,
                                 const obs_error_details *error, 
                                 void *callback_data);
obs_status listPartsCallbackEx(obs_uploaded_parts_total_info* uploadedParts,
                                      obs_list_parts *parts,
                                      void *callbackData);
void list_part_complete_callback(obs_status status,
                                 const obs_error_details *error, 
                                 void *callback_data);
void listMultiPartUploadsCompleteCallback(obs_status status,
                                          const obs_error_details *error, 
                                          void *callback_data);
obs_status listMultiPartUploadsCallback(int is_truncated, const char *next_marker,
                                        const char *next_uploadId_marker, int uploads_count, 
                                        const obs_list_multipart_upload *uploads, int common_prefixes_count, 
                                        const char **common_prefixes, void *callback_data);
void uploadFileResultCallback(obs_status status,
                              char *resultMsg,
                              int partCountReturn,
                              obs_upload_file_part_info * uploadInfoList,
                              void *callbackData);
void downloadFileResultCallback(obs_status status,
                                char *resultMsg,
                                int partCountReturn,
                                obs_download_file_part_info * downloadInfoList,
                                void *callbackData);
obs_status concurrent_response_properties_callback(
        const obs_response_properties *properties, void *callback_data);

void concurrent_upload_file_complete_callback(obs_status status, const obs_error_details *error, 
                                 void *callback_data);

obs_status DeleteObjectsDataCallback(int contentsCount, 
                                            obs_delete_objects *delobjs,
                                            void *callbackData);

int get_certificate_info(char *buffer, int buffer_length);
void tempAuthCallBack_getResult(char *tempAuthUrl, size_t tempAuthUrlLen, char *tempAuthActualHeaders,
    size_t tempAuthActualHeadersLen, void *callbackData);

void init_bucket_get_logging_message(bucket_logging_message *logging_message);
void destroy_logging_message(bucket_logging_message *logging_message);

FILE** init_uploadfilepool(FILE **fd, uint64_t part_num, char *filename);
void deinit_uploadfilepool(FILE **fd, uint64_t part_num);

#endif /* UTIL_H */

