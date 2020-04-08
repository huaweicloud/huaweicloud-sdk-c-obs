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
#include "eSDKOBS.h"
#include "request.h"
#include "simplexml.h"
#include "securec.h"
#include "common.h"

#define MAX_PARTS 32
#define SECONDS_TO_AN_HOUR 3600
#define ACL_XML_DOC_MAXSIZE (64 * 1024)
#define MAX_NAME_LEN  256
#define BLS_XML_DOC_MAXSIZE (64 * 1024)

#define MAX_SIZE_UPLOADID 64
#define MAX_SIZE_ETAG 64
#define DEFAULT_PART_SIZE (5*1024*1024)
#define MAX_PART_SIZE (5*1024*1024*1024ll)
#define MAX_BKTNAME_SIZE 1024
#define MAX_KEY_SIZE 1024
#define MAX_THREAD_NUM 100
#define MAX_READ_ONCE (5*1024*1024)
#define ONE_PART_REQUEST_XML_LEN 256
#define MAX_XML_DEPTH 4



#define appendXmlDocument(fmt, ...)                             \
    do {                                                        \
        *xmlDocumentLenReturn += snprintf_sec                     \
            (&(xmlDocument[*xmlDocumentLenReturn]),             \
             xmlDocumentBufferSize - *xmlDocumentLenReturn,     \
             xmlDocumentBufferSize - *xmlDocumentLenReturn - 1, \
             fmt, __VA_ARGS__);                                 \
        if (*xmlDocumentLenReturn >= xmlDocumentBufferSize) {   \
            return OBS_STATUS_XmlDocumentTooLarge;                 \
        } \
    } while (0)


#define MAX_NUM_TAGGING 10

char * g_uploadStatus[STATUS_BUTT] = 
{
    "UPLOAD_NOTSTART",
    "UPLOADING",
    "UPLOAD_FAILED",
    "UPLOAD_SUCCESS"
};

char * g_storageClass[OBS_STORAGE_CLASS_BUTT] = 
{
    "STANDARD",
    "STANDARD_IA",
    "GLACIER"
};

char * g_downloadStatus[DOWN_STATUS_BUTT] = 
{
    "DOWNLOAD_NOTSTART",
    "DOWNLOADING",
    "DOWNLOAD_FAILED",
    "DOWNLOAD_SUCCESS",
    "COMBINE_SUCCESS"
};

typedef enum
{
    UPLOAD_FILE_INFO,
    DOWNLOAD_FILE_INFO
}exml_root;

typedef enum
{
    DO_NOTHING,
    CLEAN_FILE,
    DELETE_FILE
}EN_FILE_ACTION;

typedef struct initiate_multi_part_upload_data
{
    simple_xml simpleXml;
    obs_response_properties_callback *responsePropertiesCallback;
    obs_response_complete_callback *responseCompleteCallback;
    void *callback_data;
    int upload_id_return_size;
    char *upload_id_return;
    string_buffer(uploadID, 256);
} initiate_multi_part_upload_data;

typedef struct complete_multi_part_upload_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_response_complete_callback *responseCompleteCallback;
    obs_complete_multi_part_upload_callback *complete_multipart_upload_callback;
    void *callback_data;
    char* doc;
    int docLen, docBytesWritten;
    string_buffer(location, 256);
    string_buffer(etag, 256);
    string_buffer(bucket, 256);
    string_buffer(key, 256);
} complete_multi_part_upload_data;

typedef struct parts_info
{
    unsigned part_number;
    string_buffer(last_modified, 256);
    string_buffer(etag, 256);
    string_buffer(size, 24);
} parts_info;

typedef struct list_parts_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_list_parts_callback_ex *list_parts_callback_ex;
    obs_response_complete_callback *responseCompleteCallback;
    void *callback_data;

    string_buffer(initiator_id, 1024);
    string_buffer(initiator_display_name, 1024);
    string_buffer(owner_id, 1024);
    string_buffer(owner_display_name, 1024);
    string_buffer(storage_class, 64);
    string_buffer(is_truncated, 64);
    
    unsigned int nextpart_number_marker;
    int parts_count;
    parts_info parts[MAX_PARTS];

} list_parts_data;


typedef struct copy_object_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_response_complete_callback *responseCompleteCallback;
    void *callback_data;

    int64_t *last_modified_return;
    int etag_return_size;
    char *etag_return;
    int eTagReturnLen;

    string_buffer(last_modified, 256);
} copy_object_data;

typedef struct convert_acl_data_info
{
    char *owner_id;
    int ownerIdLen;
    char *owner_display_name;
    int ownerDisplayNameLen;
    int *acl_grant_count_return;
    obs_acl_grant *acl_grants;
    obs_object_delivered *object_delivered;

    string_buffer(email_address, OBS_MAX_GRANTEE_EMAIL_ADDRESS_SIZE);
    string_buffer(userId, OBS_MAX_GRANTEE_USER_ID_SIZE);
    string_buffer(userDisplayName, OBS_MAX_GRANTEE_DISPLAY_NAME_SIZE);
    string_buffer(groupUri, 128);
    string_buffer(permission, 32);
    string_buffer(bucket_delivered, 32);
    obs_use_api use_api;
} convert_acl_data_info;


typedef struct get_acl_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback; 
    obs_response_complete_callback *responseCompleteCallback; 
    void *callback_data;

    int *acl_grant_count_return;
    obs_acl_grant *acl_grants;
    char *owner_id;
    char *owner_display_name;
    obs_object_delivered *object_delivered;
    string_buffer(aclXmlDocument, ACL_XML_DOC_MAXSIZE); // char aclXmlDocument[ACL_XML_DOC_MAXSIZE + 1]; int aclXmlDocumentLen
    obs_use_api use_api;
} get_acl_data;

typedef struct set_acl_data
{
    obs_response_properties_callback *responsePropertiesCallback;
    obs_response_complete_callback *responseCompleteCallback;
    void *callback_data;

    int aclXmlDocumentLen;
    char aclXmlDocument[ACL_XML_DOC_MAXSIZE];
    int aclXmlDocumentBytesWritten;
} set_acl_data; 

typedef struct set_sal_data
{
    obs_response_properties_callback *responsePropertiesCallback;
    obs_response_complete_callback *responseCompleteCallback;
    void *callback_data;

    int salXmlDocumentLen;
    char salXmlDocument[BLS_XML_DOC_MAXSIZE];
    int salXmlDocumentBytesWritten;
} set_sal_data;

typedef struct _upload_file_part_info
{
    int part_num;
    char etag[MAX_SIZE_ETAG];
    uint64_t start_byte;
    uint64_t part_size;
    part_upload_status uploadStatus;
    struct _upload_file_part_info * prev;
    struct _upload_file_part_info * next; 
    void * threadHandler;
}upload_file_part_info;

typedef struct _upload_file_summary
{
    uint64_t fileSize;
    time_t lastModify;
    int fileMd5[MAX_SIZE_ETAG];
    int fileCheckSum;
    char upload_id[MAX_SIZE_UPLOADID];
    char  bucket_name[MAX_BKTNAME_SIZE];
    char  key[MAX_KEY_SIZE];
}upload_file_summary;

typedef struct _upload_params
{
    int enable_check_point;
    char * fileNameCheckpoint;
    char * upload_id;
    char * fileNameUpload; 
    char * objectName;
    
    const obs_options *options;
    server_side_encryption_params *pstServerSideEncryptionParams;
    obs_response_handler * response_handler;
    void * callBackData;
}upload_params;

typedef struct
{
    upload_params * stUploadParams;
    upload_file_part_info  *stUploadFilePartInfo;
    void * callBackData;
}upload_file_proc_data;

typedef struct _upload_file_callback_data
{
    char *checkpointFilename;//the fd of checkpoint file
    int taskHandler;
    int fdUploadFile;
    int part_num;
    int enableCheckPoint;
    uint64_t totalBytes;
    uint64_t bytesRemaining;    
    const obs_response_handler * respHandler;
    upload_file_part_info *stUploadFilePartInfo;// this store the info about one part        
    void * callbackDataIn;//the callback data pass from client
}upload_file_callback_data;

typedef struct
{
    uint64_t objectLength;
    uint64_t lastModify;
    char etag[MAX_SIZE_ETAG];
    obs_storage_class storage_class;
    char  bucket_name[MAX_BKTNAME_SIZE];
    char  key[MAX_KEY_SIZE];
}download_file_summary;

typedef struct _download_file_part_info
{
    int part_num;
    char etag[MAX_SIZE_ETAG];
    uint64_t start_byte;
    uint64_t part_size;
    download_status downloadStatus;
    struct _download_file_part_info * prev;
    struct _download_file_part_info * next; 
}download_file_part_info;

typedef struct _download_params
{
    int enable_check_point;
    char * fileNameCheckpoint;
    char * objectName;
    char * version_id;
    char * fileNameStore;
    
    const obs_options *options;
    server_side_encryption_params *pstServerSideEncryptionParams;
    obs_get_conditions *get_conditions;
    obs_response_handler * response_handler;
    void * callBackData;	
}download_params;

typedef struct 
{
    download_file_summary * pstFileInfo;
    obs_status retStatus;
}get_object_metadata_callback_data;

typedef struct
{
    download_params * pstDownloadParams;
    download_file_part_info  *pstDownloadFilePartInfo;
    void * callBackData;
    void * xmlWriteMutex;
}download_file_proc_data;

typedef struct _download_file_callback_data
{
    char *checkpointFilename;//the file_name of checkpoint file
    int taskHandler;
    int fdStorefile;
    int enableCheckPoint;
    uint64_t totalBytes;
    uint64_t bytesRemaining;    
    obs_response_handler * respHandler;
    download_file_part_info *pstDownloadFilePartInfo;// this store the info about one part        
    void * callbackDataIn;//the callback data pass from client
    void * xmlWriteMutex;
}download_file_callback_data;

typedef struct  delete_object_contents
{
    string_buffer(key, 1024);
    string_buffer(code, 256);
    string_buffer(message, 256);
    string_buffer(delete_marker, 24);
    string_buffer(delete_marker_version_id, 256);
} delete_object_contents;

typedef struct delete_object_data
{
    simple_xml simpleXml;

    obs_response_properties_callback *responsePropertiesCallback;
    obs_response_complete_callback *responseCompleteCallback;
    obs_delete_object_data_callback *delete_object_data_callback;
    void *callback_data;
    char doc[OBS_MAX_DELETE_OBJECT_DOC];
    int docLen, docBytesWritten;
    int contents_count;
    delete_object_contents contents[OBS_MAX_DELETE_OBJECT_NUMBER];
} delete_object_data;

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


