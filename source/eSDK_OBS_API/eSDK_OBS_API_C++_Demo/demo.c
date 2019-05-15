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


// head object ---------------------------------------------------------------
static void test_head_object(char *key, char *bucket_name)
{
    obs_options option;
    head_object_data data = {0};
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = OBS_URI_STYLE_VIRTUALHOST;
    option.request_options.http2_switch = OBS_HTTP2_OPEN;
    
    obs_response_handler response_handler =
    { 
        &head_properties_callback,
        &head_complete_callback
    };

    obs_head_object(&option,key, &response_handler, &data);

    if (data.ret_status == OBS_STATUS_OK) {
        printf("head object %s successfully.\n", key);
    }
    else {
        printf("head object %s failed(%s).\n", key, obs_get_status_name(data.ret_status));
    }
}

// get object metadata---------------------------------------------------------------
static void test_get_object_metadata(char *key, char *version_id)
{

    obs_object_info objectinfo;
    memset(&objectinfo,0,sizeof(obs_object_info));
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
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };

    get_object_metadata(&option,&objectinfo,0, &response_handler,&ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("get object %s metadata successfully.\n", key);
    }
    else
    {
        printf("get object %s metadata failed(%s).\n", key, obs_get_status_name(ret_status));
    }
}

// create bucket ---------------------------------------------------------------
static void test_create_bucket(obs_canned_acl canned_acl, char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    option.bucket_options.uri_style = OBS_URI_STYLE_VIRTUALHOST;

    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    create_bucket(&option, canned_acl, NULL, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("create bucket %s successfully.\n", bucket_name);
    }
    else
    {
        printf("create bucket %s failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
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
    option.bucket_options.uri_style = gDefaultURIStyle;
    option.bucket_options.certificate_info = NULL;

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    option.bucket_options.storage_class = storage_class_value;
    create_bucket(&option, canned_acl, bucket_region, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("create bucket %s with option successfully.\n", bucket_name);
    }
    else
    {
        printf("create bucket %s with option failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

}



// delete bucket ---------------------------------------------------------------
static void test_delete_bucket(char *bucket_name)
{
    obs_options option;
    obs_status  ret_status = OBS_STATUS_BUTT;
    
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

    delete_bucket(&option, &response_handler, &ret_status);

    if (ret_status == OBS_STATUS_OK) {
        printf("delete bucket %s successfully.\n", bucket_name);
    }
    else
    {
        printf("delete bucket %s failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
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
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    set_bucket_quota(&option, bucketquota, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) 
    {
        printf("set bucket %s quota successfully.\n", bucket_name);
    }
    else
    {
        printf("set bucket %s quota failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
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
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };
    
    uint64_t bucketquota = 0;
    get_bucket_quota(&option, &bucketquota, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf(" quota=%lu\nget bucket %s quota successfully.\n",
            bucketquota, bucket_name);
    }
    else
    {
        printf("get bucket %s quota failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

//set bucket policy
void test_set_bucket_policy(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        &response_properties_callback, &response_complete_callback
    };

    char bucket_policy[1024] = {0};
    if(demoUseObsApi == OBS_USE_API_S3) {
        sprintf(bucket_policy,
            "{\"Version\":\"2008-10-17\",\"Id\":\"111\","
            "\"Statement\":[{\"Sid\": \"AddPerm\", \"Action\": [\"s3:GetObject\" ]," 
            "\"Effect\": \"Allow\",\"Resource\": \"arn:aws:s3:::%s/*\",\"Principal\":\"*\"} ] }", 
            bucket_name);
    } else {
        sprintf(bucket_policy,
            "{\"Statement\":[{\"Sid\": \"AddPerm\", \"Action\": [\"GetObject\" ]," 
            "\"Effect\": \"Allow\",\"Resource\": \"%s/*\",\"Principal\":\"*\"} ] }", 
            bucket_name);

    }
    
    set_bucket_policy(&option, bucket_policy, &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket %s policy successfully.\n", bucket_name);
    }
    else
    {
        printf("set bucket %s policy failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

//get bucket policy
void test_get_bucket_policy(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };
    
    char policy[1024]="";
    get_bucket_policy(&option, sizeof(policy), policy, &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf(" policy=(%s)\nget bucket %s policy successfully.\n", policy, bucket_name);
    }
    else
    {
        printf("get bucket %s policy failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

//delete bucket policy
void test_delete_bucket_policy(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };
    
    delete_bucket_policy(&option, &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("delete bucket %s policy successfully.\n", bucket_name);
    }
    else
    {
        printf("delete bucket %s policy failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

// set bucket version ------------------------------------------------------
static void test_set_bucket_version(char *bucket_name, char *version_status)
{
    obs_status ret_status = OBS_STATUS_BUTT;
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
    set_bucket_version_configuration(&option, version_status, 
        &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket %s version %s successfully.\n", bucket_name, version_status);
    }
    else
    {
        printf("set bucket %s version failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

}

//get bucket version
void test_get_bucket_version(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    char status[OBS_COMMON_LEN_256] = {0};
    get_bucket_version_configuration(&option, sizeof(status), status, 
        &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf(" versioning state = %s\nget bucket %s version successfully.\n", status, bucket_name);
    }
    else
    {
        printf("get bucket %s version failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

}

// set bucket storage class ------------------------------------------------------
static void test_set_bucket_storage_class(char *bucket_name, 
                 obs_storage_class storage_class_policy)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };
    set_bucket_storage_class_policy(&option, storage_class_policy, 
                &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket %s storage class successfully.\n", bucket_name);
    }
    else
    {
        printf("set bucket %s storage class failed(%s).\n", 
                            bucket_name, obs_get_status_name(ret_status));
    }
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
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_get_bucket_storage_class_handler getBucketStorageResponse = 
    {
        {0, &response_complete_callback}, 
        &get_bucket_storageclass_handler
    };

    get_bucket_storage_class_policy(&option, &getBucketStorageResponse, &ret_status);

    if (OBS_STATUS_OK == ret_status) 
    {
        printf("get bucket %s storage class successfully.\n", bucket_name);
    }
    else
    {
        printf("get bucket %s storage class failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

}

// set bucket tagging ------------------------------------------------------
static void test_set_bucket_tagging(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    
    obs_options option; 
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
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
        printf("set bucket %s tagging successfully.\n", bucket_name);
    }
    else
    {
        printf("set bucket %s tagging failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

}
 
//get bucket tagging
void test_get_bucket_tagging(char *bucket_name)
{
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

     obs_get_bucket_tagging_handler response_handler = 
    {
         {&response_properties_callback, &get_tagging_complete_callback}, 
            &get_bucket_tagging_callback
    };

    TaggingInfo tagging_info;
    memset(&tagging_info, 0, sizeof(TaggingInfo));
    tagging_info.ret_status = OBS_STATUS_BUTT;

    get_bucket_tagging(&option, &response_handler, &tagging_info);

    if (OBS_STATUS_OK == tagging_info.ret_status) {
        printf("get bucket %s tagging successfully.\n", bucket_name);
    }
    else
    {
        printf("get bucket %s tagging failed(%s).\n", bucket_name, obs_get_status_name(tagging_info.ret_status));
    }

}

//delete bucket tagging
void test_delete_bucket_tagging(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    
    delete_bucket_tagging(&option, &response_handler, &ret_status);
    
    if (statusG == OBS_STATUS_OK) {
        printf("delete bucket %s tagging successfully.\n", bucket_name);
    }
    else
    {
        printf("delete bucket %s tagging failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

}

// set bucket logging with grant ------------------------------------------------------
void set_log_delivery_acl(char *bucket_name_target)
{
    obs_options *option;
    
    option = (obs_options *)malloc(sizeof(obs_options));
    init_obs_options(option);

    option->bucket_options.host_name = HOST_NAME;
    option->bucket_options.bucket_name = bucket_name_target;
    option->bucket_options.access_key = ACCESS_KEY_ID;
    option->bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option->bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };

    manager_acl_info aclinfo;
    memset_s(&aclinfo, sizeof(manager_acl_info), 0, sizeof(manager_acl_info));
    
    aclinfo.acl_grants = (obs_acl_grant*)malloc(sizeof(obs_acl_grant)*2);

    if(demoUseObsApi == OBS_USE_API_S3) {
        aclinfo.acl_grants->grantee_type = OBS_GRANTEE_TYPE_LOG_DELIVERY;
        (aclinfo.acl_grants + 1)->grantee_type = OBS_GRANTEE_TYPE_LOG_DELIVERY;
    } else {
        aclinfo.acl_grants->grantee_type = OBS_GRANTEE_TYPE_ALL_USERS;
        (aclinfo.acl_grants + 1)->grantee_type = OBS_GRANTEE_TYPE_ALL_USERS;
    }
    aclinfo.acl_grants->permission = OBS_PERMISSION_WRITE ; 
    (aclinfo.acl_grants + 1)->permission = OBS_PERMISSION_READ_ACP ; 		

    aclinfo.acl_grant_count_return = (int*)malloc(sizeof(int));
    *(aclinfo.acl_grant_count_return) = 2;

    aclinfo.owner_id = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo.owner_id,sizeof(aclinfo.owner_id),0,sizeof(aclinfo.owner_id));
    aclinfo.owner_display_name = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo.owner_display_name,sizeof(aclinfo.owner_display_name),0,sizeof(aclinfo.owner_display_name));
    strcpy(aclinfo.owner_id, "domainiddomainiddomainiddo000400");   
    strcpy(aclinfo.owner_display_name, "displayname");
    
    memset(&aclinfo.object_info,0,sizeof(aclinfo.object_info));
    set_object_acl(option, &aclinfo, &response_handler,0);
    
    if (statusG == OBS_STATUS_OK) {
        printf("set bucket-target %s log delivery acl successfully.\n", bucket_name_target);
    }
    else
    {
        printf("set bucket-target %s log delivery acl failed.\n", bucket_name_target);
        printError();
    }
    free(aclinfo.acl_grants);
    free(aclinfo.owner_display_name);
    free(aclinfo.owner_id);
    free(aclinfo.acl_grant_count_return);
    free(option);
}

static void test_set_bucket_logging_with_grant(char *bucket_name_src, char *bucket_name_target)
{
    set_log_delivery_acl(bucket_name_target);
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name_src;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;    
    
    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    
    int aclGrantCount = 2;
    obs_acl_grant acl_grants[2] = {0};
    acl_grants[0].grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
    strcpy(acl_grants[0].grantee.canonical_user.id, "userid1");
    strcpy(acl_grants[0].grantee.canonical_user.display_name, "dis1");
    acl_grants[0].permission = OBS_PERMISSION_FULL_CONTROL;

    acl_grants[1].grantee_type = OBS_GRANTEE_TYPE_ALL_OBS_USERS;
    acl_grants[1].permission = OBS_PERMISSION_READ;
    
    obs_acl_group g;
    g.acl_grants = acl_grants;
    g.acl_grant_count = aclGrantCount;

    if(demoUseObsApi == OBS_USE_API_S3)
        set_bucket_logging_configuration(&option, bucket_name_target, "prefix-log", 
            &g, &response_handler, &ret_status);
    else
        set_bucket_logging_configuration_obs(&option, bucket_name_target, "prefix-log", "agency_test",
            &g, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket %s logging with grant successfully.\n", bucket_name_src);
    }
    else
    {
        printf("set bucket %s logging with grant failed(%s).\n", bucket_name_src, obs_get_status_name(ret_status));
    }

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
    option.bucket_options.uri_style = gDefaultURIStyle;    

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    if(demoUseObsApi == OBS_USE_API_S3)
    {
        set_bucket_logging_configuration(&option, bucket_name_target, "prefix-log",
            NULL, &response_handler, &ret_status);
    }
    else
    {
        set_bucket_logging_configuration_obs(&option, bucket_name_target, "prefix-log", "agency_test",
            NULL, &response_handler, &ret_status);
    }
    
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket %s logging without grant successfully.\n",bucket_name_src);
    }
    else
    {
        printf("set bucket %s logging without grant failed(%s).\n", bucket_name_src, obs_get_status_name(ret_status));
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
    option.bucket_options.uri_style = gDefaultURIStyle;    

    obs_response_handler response_handler =
    { 
        0,
        &response_complete_callback
    };
    if(demoUseObsApi == OBS_USE_API_S3)
    {
        set_bucket_logging_configuration(&option, NULL, NULL, 
                NULL, &response_handler, &ret_status);
    }
    else
    {
        set_bucket_logging_configuration_obs(&option, NULL, NULL, NULL,
            NULL, &response_handler, &ret_status);
    }
    if (ret_status == OBS_STATUS_OK) {
        printf("close bucket %s logging successfully.\n", bucket_name_src);
    }
    else
    {
        printf("close bucket %s logging failed(%s).\n", bucket_name_src, obs_get_status_name(ret_status));
    }
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
    option.bucket_options.uri_style = gDefaultURIStyle;
    
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
            printf(" Target_Bucket: %s\n", logging_message.target_bucket);
            if (logging_message.target_prefix) 
            {
                printf(" Target_Prefix: %s\n",  logging_message.target_prefix);
            }
            if (logging_message.agency && logging_message.agency[0] != '\0') 
            {
                printf(" Agency: %s\n",  logging_message.agency);
            }
            print_grant_info(*logging_message.acl_grant_count, logging_message.acl_grants);
            printf("get bucket %s logging successfully.\n", bucket_name);
        }
        else 
        {
            printf("Service logging is not enabled for this bucket %s.\n", bucket_name);
        }
    }
    else
    {
        printf("get bucket %s logging failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
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
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_set_bucket_website_conf set_bucket_website_conf; 
    set_bucket_website_conf.suffix = "index.html"; 
    set_bucket_website_conf.key = "Error.html"; 
    set_bucket_website_conf.routingrule_count = 2; 
    bucket_website_routingrule temp[2];
    memset(&temp[0], 0, sizeof(bucket_website_routingrule));
    memset(&temp[1], 0, sizeof(bucket_website_routingrule));
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
        printf("set bucket %s website conf successfully.\n", bucket_name);
    }
    else
    {
        printf("set bucket %s website conf failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
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
    option.bucket_options.uri_style = gDefaultURIStyle;

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
        printf("set bucket %s website all successfully.\n", bucket_name);
    }
    else
    {
        printf("set bucket %s website all failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

}

// get bucket website
void test_get_bucket_website(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
     obs_get_bucket_websiteconf_handler response_handler = 
    {
         {&response_properties_callback, &response_complete_callback}, 
            &get_bucket_websiteconf_callback
    };
    get_bucket_website_configuration(&option, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("get bucket  %s website successfully.\n", bucket_name);
    }
    else
    {
        printf("get bucket %s website failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
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
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        &response_properties_callback,
        &response_complete_callback
    };
    
    delete_bucket_website_configuration(&option, &response_handler, &ret_status);
    
    if (statusG == OBS_STATUS_OK) {
        printf("delete bucket %s website successfully.\n", bucket_name);
    }
    else
    {
        printf("delete bucket %s website failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
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
    memset_s( aclinfo->acl_grants, 2 * sizeof(obs_acl_grant), 0, 2 *sizeof(obs_acl_grant));
    strcpy(aclinfo->acl_grants->grantee.canonical_user.id, "userid1"); 
    strcpy(aclinfo->acl_grants->grantee.canonical_user.display_name, "name1"); 
    aclinfo->acl_grants->grantee_type = OBS_GRANTEE_TYPE_LOG_DELIVERY;
    aclinfo->acl_grants->permission = OBS_PERMISSION_WRITE;
    aclinfo->acl_grants->bucket_delivered = BUCKET_DELIVERED_FALSE;

    strcpy((aclinfo->acl_grants + 1)->grantee.canonical_user.id, "userid1"); 
    strcpy((aclinfo->acl_grants + 1)->grantee.canonical_user.display_name, "name1"); 
    (aclinfo->acl_grants + 1)->grantee_type = OBS_GRANTEE_TYPE_CANONICAL_USER;
    (aclinfo->acl_grants + 1)->permission = OBS_PERMISSION_READ_ACP;
    (aclinfo->acl_grants + 1)->bucket_delivered = BUCKET_DELIVERED_TRUE; 

    aclinfo->acl_grant_count_return = (int*)malloc(sizeof(int));
    *(aclinfo->acl_grant_count_return) = 2;

    aclinfo->owner_id = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo->owner_id,sizeof(aclinfo->owner_id),0,sizeof(aclinfo->owner_id));
    aclinfo->owner_display_name = (char *)malloc(sizeof(char)*100);
    memset_s(aclinfo->owner_display_name,sizeof(aclinfo->owner_display_name),0,sizeof(aclinfo->owner_display_name));
    strcpy(aclinfo->owner_id, "domainiddomainiddomainiddo000400");   
    strcpy(aclinfo->owner_display_name, "displayname");

    memset(&aclinfo->object_info,0,sizeof(aclinfo->object_info));
}

void deinitialize_acl_info(manager_acl_info *aclinfo)
{  
    free(aclinfo->acl_grants);
    free(aclinfo->owner_display_name);
    free(aclinfo->owner_id);
    free(aclinfo->acl_grant_count_return);
}
//test set bucket acl
void test_set_bucket_acl(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
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

    manager_acl_info aclinfo;
    init_acl_info(&aclinfo);
    
    set_bucket_acl(&option, &aclinfo, &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket %s acl successfully.\n", bucket_name);
    }
    else {
        printf("set bucket %s acl failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
    deinitialize_acl_info(&aclinfo);
}

void test_get_bucket_acl(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
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
    
    get_bucket_acl(&option, aclinfo, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status)
    {
        printf("get bucket %s acl: -------------", bucket_name);
        printf("%s %s\n", aclinfo->owner_id, aclinfo->owner_display_name);
        if (aclinfo->acl_grant_count_return)
        {
            print_grant_info(*aclinfo->acl_grant_count_return, aclinfo->acl_grants);
        }
    }
    else
    {
        printf("get bucket %s acl failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

    free_acl_info(&aclinfo);
}

void test_set_bucket_acl_byhead(char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
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

    obs_canned_acl canned_acl = OBS_CANNED_ACL_PUBLIC_READ_WRITE;
    set_bucket_acl_by_head(&option, canned_acl, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set bucket %s acl by head successfully.\n", bucket_name);
    }
    else
    {
        printf("set bucket %s acl by head failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

//test set object acl
void test_set_object_acl(char *key, char *version_id, char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
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

    manager_acl_info aclinfo;
    init_acl_info(&aclinfo);

    aclinfo.object_info.key = key;
    aclinfo.object_info.version_id = version_id;
    
    set_object_acl(&option, &aclinfo, &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set object %s acl successfully.\n", key);
    }
    else
    {
        printf("set object %s acl failed(%s).\n", key, obs_get_status_name(ret_status));
    }
    deinitialize_acl_info(&aclinfo);
}

void test_get_object_acl(char *key, char *version_id, char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
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
    
    get_object_acl(&option, aclinfo, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status)
    {
        printf("get object %s acl: -------------", key);
        printf("%s %s %s %s object delivered is %d\n", aclinfo->owner_id, aclinfo->owner_display_name,
            aclinfo->object_info.key, aclinfo->object_info.version_id, aclinfo->object_delivered);
        if (aclinfo->acl_grant_count_return)
        {
            print_grant_info(*aclinfo->acl_grant_count_return, aclinfo->acl_grants);
        }
    }
    else
    {
        printf("get object %s acl failed(%s).\n", key, obs_get_status_name(ret_status));
    }

    free_acl_info(&aclinfo);
}

void test_set_object_acl_byhead(char *key, char *version_id, char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
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

    obs_canned_acl canned_acl = OBS_CANNED_ACL_PUBLIC_READ;
    obs_object_info object_info;
    object_info.key = key;
    object_info.version_id = version_id;
    set_object_acl_by_head(&option, &object_info, canned_acl, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("set object %s acl by head successfully.\n", key);
    }
    else
    {
        printf("set object %s acl by head failed(%s).\n", key, obs_get_status_name(ret_status));
    }
}

// list objects--------------------------------------------------------------
static void test_list_bucket_objects(char *bucket_name)
{
    obs_options option;
    int maxkeys  =  100;
    
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_list_objects_handler list_bucket_objects_handler =
    {
        { &response_properties_callback, &list_object_complete_callback },
        &list_objects_callback
    };

    list_object_callback_data data;
    memset(&data, 0, sizeof(list_object_callback_data));
    data.allDetails = 1;
    list_bucket_objects(&option, NULL, data.next_marker, NULL, maxkeys, &list_bucket_objects_handler, &data); 
    if (OBS_STATUS_OK == data.ret_status) {
        printf("list bucket %s objects successfully.\n", bucket_name);
    }
    else
    {
        printf("list bucket %s objects failed(%s).\n", bucket_name, obs_get_status_name(data.ret_status));
    }

}

/*****************************list versions********************************************/
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
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_list_versions_handler list_versions_handler =
    {
        { &response_properties_callback, &list_versions_complete_callback },
        &listVersionsCallback
    };

    list_versions_callback_data data;
    memset(&data, 0, sizeof(list_versions_callback_data));
    
    list_versions(&option, prefix, key_marker, delimiter, maxkeys, version_id_marker,
                &list_versions_handler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("list versions %s successfully.\n", bucket_name);
    }
    else
    {
        printf("list versions %s failed(%s).\n", bucket_name, obs_get_status_name(data.ret_status));
    }
}

/*****************test_get_bucket_storage_info************************************************/
static void test_get_bucket_storage_info(char *bucket_name)
{
    obs_options option; 
    char capacity[OBS_COMMON_LEN_256] = {0};
    char obj_num[OBS_COMMON_LEN_256] = {0};
    obs_status ret_status = OBS_STATUS_BUTT;
    
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
    
    get_bucket_storage_info(&option, OBS_COMMON_LEN_256, capacity, OBS_COMMON_LEN_256, obj_num, 
        &response_handler, &ret_status); 
    
    if (ret_status == OBS_STATUS_OK) {
         printf(" objNum=%s capacity=%s\nget bucket %s storage info success\n", 
            obj_num, capacity, bucket_name);
    }
    else
    {
       printf("get bucket %s storage info failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

}

/***********************test_set_bucket_lifecycle_configuration*****************************/
static void test_set_bucket_lifecycle_configuration(char *bucket_name)
{
    obs_options option;
    obs_status  ret_status = OBS_STATUS_BUTT;

    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    // 设置完成的回调函数
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    obs_lifecycle_conf bucket_lifecycle_conf[2];
    memset(bucket_lifecycle_conf, 0, sizeof(obs_lifecycle_conf)*2); 

    //生命周期规则的id
    bucket_lifecycle_conf[0].id = "test1"; 
    // 指定前缀"test"
    bucket_lifecycle_conf[0].prefix = "a/b/c/test"; 
    // 指定满足前缀的对象创建10天后过期 
    bucket_lifecycle_conf[0].days = "10"; 
    // 指定满足前缀的对象的历史版本20天后过期 
    bucket_lifecycle_conf[0].noncurrent_version_days = "20";
    // 该生命周期规则生效
    bucket_lifecycle_conf[0].status = "Enabled"; 

     //生命周期规则的id
    bucket_lifecycle_conf[1].id = "test2"; 
    // 指定前缀"test"
    bucket_lifecycle_conf[1].prefix = "bcd/"; 
    // 指定满足前缀的对象创建10天后过期 
    bucket_lifecycle_conf[1].days = "10"; 
    // 指定满足前缀的对象的历史版本20天后过期 
    bucket_lifecycle_conf[1].noncurrent_version_days = "20";
    // 该生命周期规则生效
    bucket_lifecycle_conf[1].status = "Enabled"; 


    set_bucket_lifecycle_configuration(&option, bucket_lifecycle_conf, 2, 
            &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket %s lifecycle configuration success.\n", bucket_name);
    }
    else
    {
        printf("set bucket %s lifecycle configuration failed(%s).\n", 
                bucket_name, obs_get_status_name(ret_status));
    }
}


static void test_set_bucket_lifecycle_configuration2(char *bucket_name)
{
    obs_options option;
    obs_status  ret_status = OBS_STATUS_BUTT;

    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    // 设置完成的回调函数
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    obs_lifecycle_conf bucket_lifecycle_conf;
    memset(&bucket_lifecycle_conf, 0, sizeof(obs_lifecycle_conf)); 
    
    //生命周期规则的id
    bucket_lifecycle_conf.id = "test3"; 
    // 指定前缀"test"
    bucket_lifecycle_conf.prefix = "a/b/c/"; 
    // 该生命周期规则生效
    bucket_lifecycle_conf.status = "Enabled"; 
    
    obs_lifecycle_transtion transition;
    memset(&transition, 0, sizeof(obs_lifecycle_transtion));
    // 指定满足前缀的对象创建30天后转换 
    transition.days = "30";
    // 指定对象转换后的存储类型 
    transition.storage_class = OBS_STORAGE_CLASS_STANDARD_IA;
    bucket_lifecycle_conf.transition = &transition;
    bucket_lifecycle_conf.transition_num = 1;

    obs_lifecycle_noncurrent_transtion noncurrent_transition;
    memset(&noncurrent_transition, 0, sizeof(obs_lifecycle_noncurrent_transtion));
    // 指定满足前缀的对象的历史版本30天后转换 
    noncurrent_transition.noncurrent_version_days = "30";
    // 指定满足前缀的对象的历史版本转换后的存储类型 
    noncurrent_transition.storage_class = OBS_STORAGE_CLASS_STANDARD_IA;
    bucket_lifecycle_conf.noncurrent_version_transition = &noncurrent_transition;
    bucket_lifecycle_conf.noncurrent_version_transition_num = 1;
  

    set_bucket_lifecycle_configuration(&option, &bucket_lifecycle_conf, 1, 
                    &response_handler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("set bucket %s lifecycle configuration2 success.\n", bucket_name);
    }
    else
    {
        printf("set bucket %s lifecycle configuration2 failed(%s).\n", 
                    bucket_name, obs_get_status_name(ret_status));
    }
}


/*********************get_lifecycle_config*****************************************************/
static void test_get_lifecycle_config(char *bucket_name)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    
    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    // 设置回调函数
    obs_lifecycle_handler lifeCycleHandlerEx =
    {
        {&response_properties_callback, &response_complete_callback},
        &getBucketLifecycleConfigurationCallbackEx
    };
    
    get_bucket_lifecycle_configuration(&option, &lifeCycleHandlerEx, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("get %s lifecycle config success.\n", bucket_name);
    }
    else
    {
        printf("get %s lifecycle config failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}


/*********************test_delete_lifecycle_config******************************************/
static void test_delete_lifecycle_config(char *bucket_name)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;

    // 设置option
    init_obs_options(&option);    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    delete_bucket_lifecycle_configuration(&option, &response_handler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("test delete %s lifecycle config success.\n", bucket_name);
    }
    else
    {
        printf("test delete %s lifecycle config failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

/***********************test_set_bucket_cors*********************************************/
static void test_set_bucket_cors(char *bucket_name)
{
    obs_options option;
    obs_status  ret_status = OBS_STATUS_BUTT;

    obs_bucket_cors_conf bucketCorsConf; 
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

    
    char *id_1= "1"; 
    // 指定浏览器对特定资源的预取(OPTIONS)请求返回结果的缓存时间,单位为秒 
    char *max_age_seconds = "100";
    // 指定允许的跨域请求方法(GET/PUT/DELETE/POST/HEAD) 
    const char* allowedMethod_1[5] = {"GET","PUT","HEAD","POST","DELETE"};
    // 指定允许跨域请求的来源 
    const char* allowedOrigin_1[2] = {"obs.xxx.com", "www.xxx.com"}; 
    // 指定允许用户从应用程序中访问的header
    const char* allowedHeader_1[2] = {"header-1", "header-2"}; 
    // 响应中带的附加头域
    const char* exposeHeader_1[2]  = {"hello", "world"}; 
     
    memset(&bucketCorsConf, 0, sizeof(obs_bucket_cors_conf)); 
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
        printf("set bucket %s cors success.\n", bucket_name);
    }
    else {
        printf("set_bucket %s cors failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

/*********************test_get_cors_config*****************************************************/
static void test_get_cors_config(char *bucket_name)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style     = gDefaultURIStyle;
    // 设置回调函数
    obs_cors_handler cors_handler_info =
    {
        {&response_properties_callback, &response_complete_callback},
        &get_cors_info_callback
    };

    get_bucket_cors_configuration(&option, &cors_handler_info, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("get %s cors config success.\n", bucket_name);
    }
    else {
        printf("get %s cors config failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

/*********************test_delete_cors_config******************************************/
static void test_delete_cors_config(char *bucket_name)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style     = gDefaultURIStyle;
    // 设置回调函数
    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    delete_bucket_cors_configuration(&option, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("delete %s cors config success.\n", bucket_name);
    }
    else
    {
        printf("delete %s cors config failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
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
    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style     = gDefaultURIStyle;
    // 设置回调函数
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };
    // 设置通知配置的id，该配置唯一标示
    topic_conf.id    = "Id001"; 
    // 设置事件通知主题的URN
    topic_conf.topic = "urn:smn:R1:ea79855fbe0642718cb4df1551c3cb4e:test_cwx298983";
    // 设置通知的操作
    topic_conf.event = topic1_event; 
    topic_conf.event[0] = SMN_EVENT_OBJECT_CREATED_PUT; 
    topic_conf.event[1] = SMN_EVENT_OBJECT_CREATED_POST; 
    topic_conf.event_num = 2; 
    // 设置通知对象的过滤规则
    filter_rule.name = OBS_SMN_FILTER_PREFIX; 
    filter_rule.value = "aaa"; 
    topic_conf.filter_rule = &filter_rule; 
    topic_conf.filter_rule_num = 1; 
      
    memset(&notification_conf, 0, sizeof(obs_smn_notification_configuration)); 
    notification_conf.topic_conf = &topic_conf; 
    notification_conf.topic_conf_num = 1;

    set_notification_configuration(&option, &notification_conf, &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("set %s notification configuration success.\n", bucket_name);
    }
    else {
        printf("set %s notification configuration failed(%s).\n",
                    bucket_name, obs_get_status_name(ret_status));
    }
}

/******************test_get_notification_config****************************************/
static void test_get_notification_config(char *bucket_name)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style     = gDefaultURIStyle;
    // 设置回调函数
    obs_smn_handler notification_handler_info =
    {
        {&response_properties_callback, &response_complete_callback},
        &get_notification_info_callback
    };

    get_notification_configuration(&option, &notification_handler_info, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("get %s notification config success.\n", bucket_name);
    }
    else{
        printf("get %s notification config failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

static void test_close_notification_configuration(char *bucket_name)
{
    obs_options option; 
    obs_status  ret_status = OBS_STATUS_BUTT;
    obs_smn_notification_configuration notification_conf; 
    
    // 设置option
    init_obs_options(&option);
    option.bucket_options.host_name     = HOST_NAME;
    option.bucket_options.bucket_name   = bucket_name;
    option.bucket_options.access_key    = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style     = gDefaultURIStyle;
    // 设置回调函数
    obs_response_handler response_handler =
    { 
        NULL, &response_complete_callback
    };

    memset(&notification_conf, 0, sizeof(obs_smn_notification_configuration)); 
    set_notification_configuration(&option, &notification_conf, 
                    &response_handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("close %s notification configuration success.\n", bucket_name);
    }
    else {
        printf("close %s notification configuration failed(%s).\n",
                    bucket_name, obs_get_status_name(ret_status));
    }
}

/******************test_bucket_option****************************************/
void test_bucket_option(char *bucket_name)
{   
    obs_status  ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    char* cOrigin = "obs.xxx.com"; 
    char allowed_method[5][256]={"GET","PUT","HEAD","POST","DELETE"}; 
    unsigned int am = 5; 
    char requestHeader[][256] = {"header-1", "header-2"};  
    unsigned int rhNumber = 2; 
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    //option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
    option.bucket_options.uri_style = gDefaultURIStyle;
   
    obs_response_handler resqonseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };

    obs_options_bucket(&option, cOrigin, allowed_method, am, requestHeader, 
            rhNumber, &resqonseHandler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) {
        printf("bucket %s option successfully.\n", bucket_name);
    }
    else
    {
        printf("bucket %s option failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }
}

// put object ---------------------------------------------------------------
static void test_put_object_from_file(char *bucket_name, char *key, char *file_name)
{
    uint64_t content_length = 0;
    
    // 初始化option
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    // 初始化上传对象属性
    obs_put_properties put_properties;
    init_put_properties(&put_properties);

    // 初始化存储上传数据的结构体
    put_file_object_callback_data data;
    memset(&data, 0, sizeof(put_file_object_callback_data));
    // 打开文件，并获取文件长度
    content_length = open_file_and_get_length(file_name, &data);

    // 设置回调函数
    obs_put_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_file_complete_callback },
        &put_file_data_callback
    };
    
    put_object(&option, key, content_length, &put_properties, 0, &putobjectHandler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("put object %s from file successfully.\n", key);
    }
    else
    {
        printf("put object %s failed(%s).\n",  
               key, obs_get_status_name(data.ret_status));
    }
}


static void test_put_object_from_buffer(char *bucket_name, char *key)
{
    // 待上传的buffer
    char *buffer = "abcdefg";
    // 待上传的buffer的长度
    int buffer_size = strlen(buffer);

    // 初始化option
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    // 初始化上传对象属性
    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    
    //初始化存储上传数据的结构体
    put_buffer_object_callback_data data;
    memset(&data, 0, sizeof(put_buffer_object_callback_data));
    // 把buffer赋值到上传数据结构中
    data.put_buffer = buffer;
    // 设置buffersize
    data.buffer_size = buffer_size;

    // 设置回调函数
    obs_put_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_buffer_complete_callback },
          &put_buffer_data_callback
    };

    put_object(&option, key, buffer_size, &put_properties,0,&putobjectHandler,&data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("put object %s from buffer successfully.\n", key);
    }
    else
    {
        printf("put object %s from buffer failed(%s).\n", key, obs_get_status_name(data.ret_status));
    }
}

static void test_put_object_by_strorageclass(char *key, char *file_name, char *bucket_name, obs_storage_class storage_class)
{

    uint64_t content_length = 0;
    // 初始化option
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    option.bucket_options.storage_class = storage_class;

    // 初始化上传对象属性
    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    //read from local file to buffer

    // 初始化存储上传数据的结构体
    put_file_object_callback_data data;
    memset(&data, 0, sizeof(put_file_object_callback_data));
    // 打开文件，并获取文件长度
    content_length = open_file_and_get_length(file_name, &data);

    // 设置回调函数
    obs_put_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_file_complete_callback },
        &put_file_data_callback
    };
    
    put_object(&option,key,content_length,&put_properties,0,&putobjectHandler,&data);

    if (OBS_STATUS_OK == data.ret_status) {
        printf("put object %s from file successfully.\n", key);
    }
    else
    {
        printf("put object %s failed(%s).\n",  
               key, obs_get_status_name(data.ret_status));
    }
}


static void test_put_object_by_kms_encrypt(char *bucket_name, char *key)
{
    // 待上传的buffer
    char *buffer = "11111111";
    // 待上传的buffer的长度
    int buffer_size = strlen(buffer);

    // 初始化option
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
    // 初始化上传对象属性
    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    
    //初始化存储上传数据的结构体
    put_buffer_object_callback_data data;
    memset(&data, 0, sizeof(put_buffer_object_callback_data));
    // 把buffer赋值到上传数据结构中
    data.put_buffer = buffer;
    // 设置buffersize
    data.buffer_size = buffer_size;

    //服务端解密
    /*SSE-KMS解密*/
    server_side_encryption_params encryption_params;
    memset(&encryption_params, 0, sizeof(server_side_encryption_params));
    encryption_params.encryption_type = OBS_ENCRYPTION_KMS;
    if(demoUseObsApi == OBS_USE_API_S3) {
        encryption_params.kms_server_side_encryption = "aws:kms";
    } else {
        encryption_params.kms_server_side_encryption = "kms";
    }
    //该参数可以不填 系统有默认的加密密钥
    //encryption_params.kms_key_id = "sichuan:domainiddomainiddomainiddoma0001:key/4f1cd4de-ab64-4807-920a-47fc42e7f0d0";

    // 设置回调函数
    obs_put_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_buffer_complete_callback },
          &put_buffer_data_callback
    };

    put_object(&option, key, buffer_size, &put_properties,&encryption_params,&putobjectHandler,&data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("put object %s by_kms_encrypt successfully.\n", key);
    }
    else
    {
        printf("put object %s by_kms_encrypt encryption failed(%s).\n", key, obs_get_status_name(data.ret_status));
    }
}

static void test_put_object_by_aes_encrypt(char *bucket_name, char *key)
{
    // 待上传的buffer
    char *buffer = "11111111";
    // 待上传的buffer的长度
    int buffer_size = strlen(buffer);

    // 初始化option
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = BUCKET_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
    // 初始化上传对象属性
    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    
    //初始化存储上传数据的结构体
    put_buffer_object_callback_data data;
    memset(&data, 0, sizeof(put_buffer_object_callback_data));
    // 把buffer赋值到上传数据结构中
    data.put_buffer = buffer;
    // 设置buffersize
    data.buffer_size = buffer_size;

    //服务端解密
    /*SSE-KMS加密*/
    server_side_encryption_params encryption_params;
    memset(&encryption_params, 0, sizeof(server_side_encryption_params));
    encryption_params.encryption_type = OBS_ENCRYPTION_SSEC;
    encryption_params.ssec_customer_algorithm = "AES256";
    encryption_params.ssec_customer_key = 
                    "K7QkYpBkM5+hcs27fsNkUnNVaobncnLht/rCB2o/9Cw=";
    // 设置回调函数
    obs_put_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_buffer_complete_callback },
          &put_buffer_data_callback
    };

    put_object(&option, key, buffer_size, &put_properties,
                &encryption_params,&putobjectHandler,&data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("put object %s by_aes_encrypt successfully.\n", key);
    }
    else
    {
        printf("put object %s by_aes_encrypt encryption failed(%s).\n",
                    key, obs_get_status_name(data.ret_status));
    }
}


// get object ---------------------------------------------------------------
static void test_get_object(char *bucket_name, char *key)
{
    char *file_name = "./test";
    obs_object_info object_info;
    
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    memset(&object_info, 0, sizeof(obs_object_info));
    object_info.key =key;
    
    get_object_callback_data data;
    data.ret_status = OBS_STATUS_BUTT;
    data.outfile = write_to_file(file_name);

    obs_get_conditions getcondition;
    memset(&getcondition, 0, sizeof(obs_get_conditions));
    init_get_properties(&getcondition);
    // 读取的开始位置
    getcondition.start_byte = 0;
    // 读取长度，默认0，读到对象尾
    getcondition.byte_count = 100;
    obs_get_object_handler get_object_handler =
    { 
        {&get_properties_callback, &get_object_complete_callback},
        &get_object_data_callback
    };
    
    get_object(&option, &object_info, &getcondition, 0, &get_object_handler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("get %s object successfully.\n", bucket_name);
    }
    else
    {
        printf("get %s object faied(%s).\n", bucket_name, obs_get_status_name(data.ret_status));
    }
    fclose(data.outfile);
}

static void test_get_object_by_kms_encrypt(char *bucket_name, char *key)
{
    char *file_name = "./test_by_kms";
    obs_object_info object_info;
    
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
    
    memset(&object_info, 0, sizeof(obs_object_info));
    object_info.key =key;
    
    get_object_callback_data data;
    data.ret_status = OBS_STATUS_BUTT;
    data.outfile = write_to_file(file_name);

    obs_get_conditions getcondition;
    memset(&getcondition, 0, sizeof(obs_get_conditions));
    init_get_properties(&getcondition);
    // 读取的开始位置
    getcondition.start_byte = 0;
    // 读取长度，默认0，读到对象尾
    getcondition.byte_count = 100;
    obs_get_object_handler get_object_handler =
    { 
        { NULL, &get_object_complete_callback},
        &get_object_data_callback
    };
    // kms加密的对象下载，不需要传入秘钥
    get_object(&option, &object_info, &getcondition, 0, &get_object_handler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("get object %s by_kms successfully.\n", key);
    }
    else
    {
        printf("get object %s by_kms faied(%s).\n", key, obs_get_status_name(data.ret_status));
    }
    fclose(data.outfile);
}

static void test_get_object_by_aes_encrypt(char *bucket_name, char *key)
{
    char *file_name = "./test_by_aes";
    obs_object_info object_info;
    
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;

    // SSE加密的对象下载，需要传入SSE的秘钥
    server_side_encryption_params encryption_params;
    memset(&encryption_params, 0, sizeof(server_side_encryption_params));
    encryption_params.encryption_type = OBS_ENCRYPTION_SSEC;
    encryption_params.ssec_customer_algorithm = "AES256";
    encryption_params.ssec_customer_key = "K7QkYpBkM5+hcs27fsNkUnNVaobncnLht/rCB2o/9Cw=";
    
    memset(&object_info, 0, sizeof(obs_object_info));
    object_info.key =key;
    
    get_object_callback_data data;
    data.ret_status = OBS_STATUS_BUTT;
    data.outfile = write_to_file(file_name);

    obs_get_conditions getcondition;
    memset(&getcondition, 0, sizeof(obs_get_conditions));
    init_get_properties(&getcondition);
    // 读取的开始位置
    getcondition.start_byte = 0;
    // 读取长度，默认0，读到对象尾
    getcondition.byte_count = 100;
    obs_get_object_handler get_object_handler =
    { 
        { NULL, &get_object_complete_callback},
        &get_object_data_callback
    };
    
    get_object(&option, &object_info, &getcondition, &encryption_params, &get_object_handler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("get object %s by_aes successfully.\n", key);
    }
    else
    {
        printf("get object %s by_aes faied(%s).\n", key, obs_get_status_name(data.ret_status));
    }
    fclose(data.outfile);
}


// delete object ---------------------------------------------------------------
static void test_delete_object(char *key, char *version_id, char *bucket_name)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_object_info object_info;
    memset(&object_info, 0, sizeof(obs_object_info));
    object_info.key = key;
    object_info.version_id = version_id;

    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    //option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler resqonseHandler =
    { 
        NULL, &response_complete_callback 
    };
    
    delete_object(&option,&object_info,&resqonseHandler, &ret_status);
    if (OBS_STATUS_OK == ret_status) 
    {
        printf("delete object %s successfully.\n", key);
    }
    else
    {
        printf("delete object %s failed(%s).\n", key, obs_get_status_name(ret_status));
    }

}

static void test_batch_delete_objects(char *bucket_name, char *key1, char *key2)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_object_info objectinfo[100];
    objectinfo[0].key = key1;
    objectinfo[0].version_id = OBJECT_VER[0];
    objectinfo[1].key = key2;
    objectinfo[1].version_id = OBJECT_VER[1];

    obs_delete_object_info delobj;
    memset_s(&delobj,sizeof(obs_delete_object_info),0,sizeof(obs_delete_object_info));
    delobj.keys_number = 2;

    obs_delete_object_handler handler =
    { 
        {&response_properties_callback, &response_complete_callback},
        &delete_objects_data_callback
    };

    batch_delete_objects(&option, objectinfo, &delobj, 0, &handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test batch_delete_objects %s %s successfully.\n", key1, key2);
    }
    else
    {
        printf("test batch_delete_objects %s %s faied(%s).\n", key1, key2, obs_get_status_name(ret_status));
    }
}


static void test_copy_object(char *key, char *version_id, char *source_bucket, 
    char *target_bucket, char *destinationKey)
{
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
    //option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;

    obs_copy_destination_object_info objectinfo ={0};
    objectinfo.destination_bucket = target_bucket;
    objectinfo.destination_key = destinationKey;
    objectinfo.etag_return = eTag;
    objectinfo.etag_return_size = sizeof(eTag);
    objectinfo.last_modified_return = &lastModified;

    obs_response_handler responseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };
   
    copy_object(&option, key, version_id, &objectinfo, 1, NULL, 0, &responseHandler,&ret_status);
   
    if (OBS_STATUS_OK == ret_status) {
        printf("test_copy_object %s %s successfully.\n", key, destinationKey);

    }
    else
    {
        printf("test_copy_object %s %s failed(%s).\n", key, destinationKey, obs_get_status_name(ret_status));
    }

}

static void test_restore_object(char *key, char *versionid, char *days, char *bucket_name)
{
    obs_object_info object_info;
    memset(&object_info, 0, sizeof(obs_object_info));
    object_info.key = key;
    object_info.version_id = versionid;

    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_tier tier = OBS_TIER_EXPEDITED;
        
    obs_response_handler handler =
    { 
      &response_properties_callback,
      &response_complete_callback
    };
    
    restore_object(&option, &object_info, days, tier, &handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("restore object %s successfully.\n", key);
    }
    else
    {
        printf("restore object %s faied(%s).\n", key, obs_get_status_name(ret_status));
        return;
    }
}


// list bucket ---------------------------------------------------------------
static void test_list_bucket_s3()
{
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    list_service_data data;
    memset(&data, 0, sizeof(list_service_data));
    data.allDetails = 1;

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

static void test_list_bucket_obs()
{
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = OBS_URI_STYLE_PATH;
    
    list_service_data data;
    memset(&data, 0, sizeof(list_service_data));
    data.allDetails = 1;
    
    obs_list_service_obs_handler listHandler =
    { 
        {NULL, &list_bucket_complete_callback },
        &listServiceObsCallback
    };
    
    list_bucket_obs(&option, &listHandler, &data);
    if (data.ret_status == OBS_STATUS_OK) 
    {
        printf("list bucket obs successfully.\n");
    }
    else
    {
        printf("list bucket obs failed(%s).\n", obs_get_status_name(data.ret_status));
    }
}

static void test_list_bucket()
{
    if(demoUseObsApi == OBS_USE_API_S3) {
        test_list_bucket_s3();
    } else {
        test_list_bucket_obs();
    }
}

void test_object_option(char *bucket_name, char *cKey)
{   
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    char* cOrigin = "obs.xxx.com"; 
    char allowed_method[5][256]={"GET","PUT","HEAD","POST","DELETE"}; 
    unsigned int am = 5; 
    char requestHeader[][256] = {"header-1", "header-2"};  
    unsigned int rhNumber = 2; 
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    //option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;
    option.bucket_options.uri_style = gDefaultURIStyle;
   
    obs_response_handler resqonseHandler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };

    obs_options_object(&option, cKey, cOrigin, allowed_method, am, requestHeader,
         rhNumber, &resqonseHandler, &ret_status);
    
    if (OBS_STATUS_OK == ret_status) 
    {
        printf("object %s option successfully.\n", cKey);
    }
    else
    {
        printf("object %s option failed(%s).\n", cKey, obs_get_status_name(ret_status));
    }
}

static void test_head_bucket(char *bucket_name)
{
    head_object_data data = {0};
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    obs_response_handler response_handler =
    { 
        &head_properties_callback,
        &head_complete_callback
    };

    obs_head_bucket(&option, &response_handler, &data);

    if (OBS_STATUS_OK == data.ret_status) 
    {
        printf("head bucket %s successfully.\n", bucket_name);
    }
    else 
    {
        printf("head bucket %s failed(%s).\n", bucket_name, obs_get_status_name(data.ret_status));
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
    option->bucket_options.uri_style = gDefaultURIStyle;
    
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
        printf("get bucket %s metadata with cors successfully.\n", bucket_name);
    }
    else 
    {
        printf("get bucket %s metadata with cors failed.\n", bucket_name);
        printError();
    }

     free(option);
}

static void test_init_upload_part(char *bucket_name, char *key)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    char upload_id[OBS_COMMON_LEN_256] = {0};
    int upload_id_size = OBS_COMMON_LEN_256;
    
    obs_response_handler handler =
    { 
        &response_properties_callback,
        &response_complete_callback 
    };
        
    initiate_multi_part_upload(&option, key, upload_id_size, upload_id, NULL, 0,
                &handler, &ret_status);
    if (OBS_STATUS_OK == ret_status)
    {
        printf("test init upload part %s successfully. uploadId= %s\n", key, upload_id);
        strcpy(UPLOAD_ID, upload_id);
    }
    else
    {
        printf("test init upload part %s faied(%s).\n", key, obs_get_status_name(ret_status));
    }
}

static void test_upload_part(char *filename, char *bucket_name, char *key)
{
    uint64_t uploadSliceSize =5L * 1024 * 1024;                   // upload part slice size
    uint64_t uploadSize = uploadSliceSize;                         // upload part size
    uint64_t filesize =0;                                          // file total size

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);

    obs_upload_part_info uploadPartInfo;
    memset(&uploadPartInfo, 0, sizeof(obs_upload_part_info));

    obs_upload_handler Handler =
    { 
        {&response_properties_callback, &upload_part_complete_callback},
        &test_upload_file_data_callback
    };

    test_upload_file_callback_data data;
    memset(&data, 0, sizeof(test_upload_file_callback_data));
    //read local file total size :uploadTotalLength
    filesize = get_file_info(filename,&data);
    data.noStatus = 1;
    data.part_size = uploadSize;
    data.part_num = (filesize % uploadSize == 0) ? (filesize / uploadSize) : (filesize / uploadSize +1);
    
    // upload first part
    uploadPartInfo.part_number= 1;
    uploadPartInfo.upload_id=UPLOAD_ID;
    data.start_byte  = 0;
    upload_part(&option,key,&uploadPartInfo,uploadSize,&putProperties,0,&Handler,&data);
    
    if (OBS_STATUS_OK == data.ret_status) {
        printf("test upload part 1 %s successfully.\n", key);
    }
    else
    {
        printf("test upload part 1 %s faied(%s).\n", key, obs_get_status_name(data.ret_status));
        return ;
    }
    
    //upload second part
    uploadPartInfo.part_number= 2;
    uploadPartInfo.upload_id=UPLOAD_ID;
    filesize = get_file_info(filename,&data);
    uploadSize =filesize - uploadSize;
    data.part_size = uploadSize;
    data.start_byte = uploadSliceSize;
    
    fseek(data.infile, data.start_byte, SEEK_SET);
    upload_part(&option,key,&uploadPartInfo,uploadSize, &putProperties,0,&Handler,&data);
    
    if (OBS_STATUS_OK == data.ret_status) {
        printf("test upload part 2 %s successfully.\n", key);
    }
    else
    {
        printf("test upload part 2 %s faied(%s).\n", key, obs_get_status_name(data.ret_status));
    }
    fclose(data.infile);
}


static void test_complete_upload(char *uploadId, char etag[][256], char *bucket_name, char *key)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    int number=2;
    obs_complete_upload_Info info[2];
    info[0].part_number= 1;
    info[0].etag=&etag[0][0];
    info[1].part_number= 2;
    info[1].etag=&etag[1][0];

    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);
    

    obs_complete_multi_part_upload_handler Handler =
    { 
        {&response_properties_callback,
         &response_complete_callback},
        &CompleteMultipartUploadCallback
    };
    
    complete_multi_part_upload(&option,key,uploadId,number,info,&putProperties,
                &Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test complete upload %s successfully.\n", key);
    }
    else
    {
        printf("test complete upload %s faied(%s).\n", key, obs_get_status_name(ret_status));
    }
}

static void test_abort_multi_part_upload(char *unloadId, char *bucket_name, char *key)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler responseHandler =
    { 
        &response_properties_callback, &response_complete_callback
    };

    abort_multi_part_upload(&option, key, unloadId,&responseHandler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("abort_multi_part_upload %s successfully.\n", key);
    }
    else
    {
        printf("abort_multi_part_upload %s failed(%s).\n", key, obs_get_status_name(ret_status));
    }
}


static void test_list_parts(char *bucket_name, char *key)
{
    list_part_info listpart;
    listpart.upload_id = UPLOAD_ID;
    listpart.max_parts = 2;
    listpart.part_number_marker = 0;
    
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_list_parts_handler Handler =
    { 
        {NULL, &list_part_complete_callback},
        &listPartsCallbackEx
    };

    list_parts_callback_data data;
    memset(&data, 0, sizeof(list_parts_callback_data));
    
    do{
        list_parts(&option, key, &listpart, &Handler, &data);
    }while (OBS_STATUS_OK == data.ret_status && data.isTruncated 
                && (data.keyCount < listpart.max_parts));
    
    if (OBS_STATUS_OK == data.ret_status) 
    {
        printf("list parts %s OK.\n", key);
    }
    else 
    {
        printf("list parts %s failed(%s).\n", key, obs_get_status_name(data.ret_status));
    }
}

static void test_copy_part(char *bucket_name, char *srcKey, char *dstKey)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char etagreturn[256] ={0};
    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    //option.bucket_options.protocol = OBS_PROTOCOL_HTTPS;

    /*SSE-KMS加密*/
    server_side_encryption_params encryption_params;
    memset(&encryption_params, 0, sizeof(server_side_encryption_params));
    /*encryption_params.use_kms = '1';
    encryption_params.kms_server_side_encryption = "aws:kms";
    //该参数可以为NULL 默认系统的密钥
    encryption_params.kms_key_id = "sichuan:domainiddomainiddomainiddoma0001:key/4f1cd4de-ab64-4807-920a-47fc42e7f0d0";*/

    /*SSE-C*/
    /*char* buffer = "K7QkYpBkM5+hcs27fsNkUnNVaobncnLht/rCB2o/9Cw=";
    encryption_params.use_ssec = '1';
    encryption_params.ssec_customer_algorithm = "AES256";
    encryption_params.ssec_customer_key = buffer;
    
    //解密源对象参数;用于将一个加密对象拷贝给另外一个对象
    char *des_key = "K7QkYpBkM5+hcs27fsNkUnNVaobncnLht/rCB2o/9Cw=";
    encryption_params.des_ssec_customer_algorithm = "AES256";
    encryption_params.des_ssec_customer_key = des_key;*/

    obs_put_properties putProperties={0};
    init_put_properties(&putProperties);

    obs_copy_destination_object_info object_info;
    memset(&object_info, 0, sizeof(obs_copy_destination_object_info));
    object_info.destination_bucket = BUCKET_NAME;
    object_info.destination_key = dstKey;
    object_info.etag_return = etagreturn;
    object_info.etag_return_size = 256;

    obs_upload_part_info copypart;
    memset(&copypart, 0, sizeof(obs_upload_part_info));

    obs_response_handler responseHandler =
    { 
        &response_properties_callback, &response_complete_callback
    };
    // copy first part
    copypart.part_number = 1;
    copypart.upload_id = UPLOAD_ID;
    copy_part(&option, srcKey, &object_info, &copypart, 
              &putProperties,&encryption_params,&responseHandler, &ret_status);

    if (OBS_STATUS_OK == ret_status) {
        printf("copy part 1 %s %s successfully.\n", srcKey, dstKey);
    }
    else
    {
        printf("copy part 1 %s %s failed(%s).\n", srcKey, dstKey, obs_get_status_name(ret_status));
    }

    // copy first part
    copypart.part_number = 2;
    copypart.upload_id = UPLOAD_ID;
    copy_part(&option, srcKey, &object_info, &copypart, 
              &putProperties,&encryption_params,&responseHandler, &ret_status);

    if (ret_status == OBS_STATUS_OK) {
        printf("copy part 2 %s %s successfully.\n", srcKey, dstKey);
    }
    else
    {
        printf("copy part 2 %s %s failed(%s).\n", srcKey, dstKey, obs_get_status_name(ret_status));
    }
}


static void test_upload_file(char *bucket_name, char *filename, char *key)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    uint64_t uploadSliceSize = 5L * 1024 * 1024;                  // upload part slice size

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
    uploadFileInfo.enable_check_point = 0;
    uploadFileInfo.part_size = uploadSliceSize;
    uploadFileInfo.task_num = 10;
    uploadFileInfo.upload_file = filename;

    obs_upload_file_response_handler Handler =
    { 
        {&response_properties_callback, &response_complete_callback},
        &uploadFileResultCallback
    };

    upload_file(&option, key, 0, &uploadFileInfo, &Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test upload file %s %s successfully.\n", filename, key);
    }
    else
    {
        printf("test upload file  %s %s faied(%s).\n", filename, key, obs_get_status_name(ret_status));
    }
}


static void test_download_file(char *bucket_name, char *filename, char *key)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    uint64_t uploadSliceSize =5L * 1024 * 1024;                   // upload part slice size

    obs_options option;
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_get_conditions getConditions;
    memset_s(&getConditions,sizeof(obs_get_conditions),0,sizeof(obs_get_conditions));
    init_get_properties(&getConditions);
    
    obs_download_file_configuration downloadFileConfig;
    memset_s(&downloadFileConfig,sizeof(obs_download_file_configuration),0,
                    sizeof(obs_download_file_configuration));
    downloadFileConfig.check_point_file = NULL;
    downloadFileConfig.enable_check_point = 1;
    downloadFileConfig.part_size = uploadSliceSize;
    downloadFileConfig.task_num = 2;
    downloadFileConfig.downLoad_file= filename;

    obs_download_file_response_handler Handler =
    { 
        {&response_properties_callback, &response_complete_callback },
        &downloadFileResultCallback
    };

    download_file(&option, key, 0,&getConditions,0,&downloadFileConfig, 
            &Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test download file %s %s successfully.\n", filename, key);
    }
    else
    {
        printf("test download file %s %s faied(%s).\n", filename, key, obs_get_status_name(ret_status));
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
    
    upload_part(concurrent_temp->option, concurrent_temp->key, &uploadPartInfo,
                concurrent_temp->part_size, &putProperties, 0, &uploadHandler, concurrent_temp);
    if (OBS_STATUS_OK == concurrent_temp->ret_status) {
        printf("test upload part %u successfully. \n", uploadPartInfo.part_number);
    }
    else
    {
        printf("test upload part %u faied(%s).\n",uploadPartInfo.part_number,
            obs_get_status_name(concurrent_temp->ret_status));
    }
}


static void start_upload_threads(test_upload_file_callback_data data,
                                 char *concurrent_upload_id, uint64_t filesize, char *key,
                                 obs_options option,test_concurrent_upload_file_callback_data *concurrent_upload_file)
{
     int partCount = data.part_num;
     test_concurrent_upload_file_callback_data *concurrent_temp;
     concurrent_temp = concurrent_upload_file;
     int err;
     int i= 0;
     for(i=0; i <partCount; i++)
     {
        memset(concurrent_temp[i].etag, 0,sizeof(concurrent_temp[i].etag));
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


static void test_concurrent_upload_part(char *bucket_name, char *filename, char *key, uint64_t uploadSliceSize)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    char concurrent_upload_id[2048]={0};
    uint64_t uploadSize = uploadSliceSize;                         // upload part size
    uint64_t filesize =0; 
    int i=0;
    // file total size

    obs_options option;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

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
    //putProperties.canned_acl = OBS_CANNED_ACL_BUCKET_OWNER_FULL_CONTROL;
    
    //大文件信息:文件指针，文件大小，按照分段大小的分段数
    test_upload_file_callback_data data;
    memset(&data, 0, sizeof(test_upload_file_callback_data));
    filesize = get_file_info(filename,&data);
    data.noStatus = 1;
    data.part_size = uploadSize;
    data.part_num = (filesize % uploadSize == 0) ? (filesize / uploadSize) : (filesize / uploadSize +1);

    //初始化上传段任务返回uploadId: uploadIdReturn
    char uploadIdReturn[256] = {0};
    int upload_id_return_size = 255;
    initiate_multi_part_upload(&option,key,upload_id_return_size,uploadIdReturn, &putProperties,
        0,&Handler, &ret_status);
    if (OBS_STATUS_OK == ret_status) {
        printf("test init upload part return uploadIdReturn(%s). \n", uploadIdReturn);
        strcpy(concurrent_upload_id,uploadIdReturn);
    }
    else
    {
        printf("test init upload part faied(%s).\n", obs_get_status_name(ret_status));
    }
    
    //并发上传
    test_concurrent_upload_file_callback_data *concurrent_upload_file;
    concurrent_upload_file =(test_concurrent_upload_file_callback_data *)malloc(
                sizeof(test_concurrent_upload_file_callback_data)*100);
    if(concurrent_upload_file == NULL)
    {
        printf("malloc test_concurrent_upload_file_callback_data failed!!!");
        return ;
    }
    test_concurrent_upload_file_callback_data *concurrent_upload_file_complete = concurrent_upload_file;
    start_upload_threads(data, concurrent_upload_id,filesize, key, option, concurrent_upload_file_complete);

    // 合并段
    obs_complete_upload_Info *upload_Info = (obs_complete_upload_Info *)malloc(
                sizeof(obs_complete_upload_Info)*data.part_num);
    for(i=0; i<data.part_num; i++)
    {
        upload_Info[i].part_number = concurrent_upload_file_complete[i].part_num;
        upload_Info[i].etag = concurrent_upload_file_complete[i].etag;
    }
    complete_multi_part_upload(&option, key, uploadIdReturn, data.part_num,upload_Info,&putProperties,&complete_multi_handler,&ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("test complete upload %s %s successfully.\n", filename, key);
    }
    else
    {
        printf("test complete upload %s %s faied(%s).\n", filename, key, obs_get_status_name(ret_status));
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

static void test_append_object_from_file(char *bucket_name, char *key, char *file_name, char *position)
{
    uint64_t content_length = 0;
    
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    //read from local file to buffer
    put_file_object_callback_data data;
    memset(&data, 0, sizeof(put_file_object_callback_data));
    data.infile = 0;
    content_length = open_file_and_get_length(file_name, &data);

    obs_append_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_file_complete_callback },
        &put_file_data_callback
    };
    
    append_object(&option,key,content_length,position,&put_properties,0,&putobjectHandler,&data);

    if (OBS_STATUS_OK == data.ret_status) {
        printf("append object %s from file successfully.\n", key);
    }
    else
    {
        printf("append object %s failed(%s).\n", key, obs_get_status_name(data.ret_status));
    }
}

static void test_append_object_from_buffer(char *bucket_name, char *key, char *buffer, int buffer_size,char *position)
{
    uint64_t content_length = 0;
    printf("buffer_size2 = %d\n",buffer_size);

    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    //初始化存储上传数据的结构体
    put_buffer_object_callback_data data;
    memset(&data, 0, sizeof(put_buffer_object_callback_data));
    // 把buffer赋值到上传数据结构中
    data.put_buffer = buffer;
    // 设置buffersize
    data.buffer_size = buffer_size;

    obs_append_object_handler putobjectHandler =
    { 
        { &response_properties_callback, &put_buffer_complete_callback },
          &put_buffer_data_callback
    };
    
    append_object(&option,key,buffer_size,position,&put_properties,0,&putobjectHandler,&data);

    if (OBS_STATUS_OK == data.ret_status) {
        printf("append object %s from buffer successfully.\n", key);
    }
    else
    {
        printf("append object %s from buffer failed(%s).\n",
            key, obs_get_status_name(data.ret_status));
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
    option.bucket_options.uri_style = gDefaultURIStyle;

    
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
        printf("gen create bucket %s url successfully.\n", bucket_name);
    }
    else
    {
        printf("gen create bucket %s url failed(%s).\n", bucket_name, obs_get_status_name(ret_status));
    }

}
static void test_gen_signed_url_put_object(char *bucket_name, char *key)
{
    // 上传的文件
    char file_name[256] = "./test.txt";
    uint64_t content_length = 0;

    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    // 初始化上传对象属性
    obs_put_properties put_properties;
    init_put_properties(&put_properties);

    // 初始化存储上传数据的结构体
    put_file_object_callback_data data;
    memset(&data, 0, sizeof(put_file_object_callback_data));
    // 打开文件，并获取文件长度
    content_length = open_file_and_get_length(file_name, &data);

    // 设置回调函数
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
        printf("gen put object %s url successfully.\n", key);
    }
    else
    {
        printf("gen put object %s url failed(%s).\n",  
               key, obs_get_status_name(data.ret_status));
    }

}

static void test_gen_signed_url_get_object(char *bucket_name, char *key, char *versionid)
{
    char *file_name = "./test";
    obs_object_info object_info;
    memset(&object_info, 0, sizeof(obs_object_info));
    object_info.key =key;
    object_info.version_id = versionid;

    obs_options option;
    obs_status ret_status = OBS_STATUS_BUTT;
    init_obs_options(&option);

    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    get_object_callback_data data;
    data.ret_status = OBS_STATUS_BUTT;
    data.outfile = write_to_file(file_name);

    obs_get_conditions getcondition;
    memset(&getcondition, 0, sizeof(obs_get_conditions));
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
    if (OBS_STATUS_OK == data.ret_status) {
        printf("gen_signed_url %s get object successfully.\n", key);
    }
    else
    {
        printf("gen_signed_url %s get object faied(%s).\n", key, obs_get_status_name(data.ret_status));
    }
}

/*1.*/
static void test_create_bucket_with_cert(obs_canned_acl canned_acl, char *bucket_name, 
    int cert_type, char *path, int path_length)
{
    obs_status ret_status = OBS_STATUS_BUTT;
    obs_options option;
    char ca_buffer[2048] = {0};
    int length = 0;
    int rc = 0;

    if (0 == cert_type)
    {
        ret_status = init_certificate_by_path(OBS_PROTOCOL_HTTP, OBS_NO_CERTIFICATE, NULL, 0);
    }
    else if (1 == cert_type)
    {
        ret_status = init_certificate_by_path(OBS_PROTOCOL_HTTPS, OBS_DEFAULT_CERTIFICATE, NULL, 0);
    }
    else if (2 == cert_type && path != NULL)
    {
        
        ret_status = init_certificate_by_path(OBS_PROTOCOL_HTTPS, OBS_DEFINED_CERTIFICATE, path, path_length);
    }
    else if (3 == cert_type)
    {
        length = get_certificate_info(ca_buffer, sizeof(ca_buffer));
        ret_status = init_certificate_by_buffer(ca_buffer, length);
    }

    if (ret_status != OBS_STATUS_OK)
    {
        printf("init cert failed(%s)\n", obs_get_status_name(ret_status));
        return;
    }
    
    init_obs_options(&option);
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;

    obs_response_handler response_handler =
    { 
        0, &response_complete_callback
    };

    create_bucket(&option, canned_acl, NULL, &response_handler, &ret_status);
    if (ret_status == OBS_STATUS_OK) {
        printf("create bucket with cert successfully. \n");
    }
    else
    {
        printf("create bucket with cert failed(%s).\n", obs_get_status_name(ret_status));
    }

}

static void test_list_multipart_uploads(char *bucket_name)
{
    obs_options option;
    init_obs_options(&option);
    
    option.bucket_options.host_name = HOST_NAME;
    option.bucket_options.bucket_name = bucket_name;
    option.bucket_options.access_key = ACCESS_KEY_ID;
    option.bucket_options.secret_access_key = SECRET_ACCESS_KEY;
    option.bucket_options.uri_style = gDefaultURIStyle;
    
    list_multipart_uploads_callback_data data;
    memset(&data, 0, sizeof(list_multipart_uploads_callback_data));
    
    obs_list_multipart_uploads_handler listHandler =
    { 
        {&response_properties_callback, &listMultiPartUploadsCompleteCallback},
        &listMultiPartUploadsCallback
    };
    
    list_multipart_uploads(&option, NULL, NULL, NULL, NULL, 32, &listHandler, &data);
    if (data.ret_status == OBS_STATUS_OK) 
    {
        printf("list multipart %s successfully.\n", bucket_name);
    }
    else
    {
        printf("list multipart %s failed(%s).\n", bucket_name, obs_get_status_name(data.ret_status));
    }
    
}

int main(int argc, char **argv)
{
    strcpy(ACCESS_KEY_ID,"UDSIAMSTUBTEST000400");
    strcpy(SECRET_ACCESS_KEY,"Udsiamstubtest000000UDSIAMSTUBTEST000400");
    strcpy(HOST_NAME,"10.178.221.235");
    strcpy(BUCKET_NAME,"esdk-c-test");
     
    obs_canned_acl canned_acl = OBS_CANNED_ACL_BUCKET_OWNER_FULL_CONTROL;
    char bucket_src[]="bucket-src";
    char bucket_target[]="bucket-target";
    char bucket_version[]="bucket-version";
    char bucket_obj_acl[]="bucket-obj-acl";

    /*------ obs init------*/
    obs_initialize(OBS_INIT_ALL);
    set_online_request_max_count(10);

    /*------ bucket test------*/
    test_create_bucket(canned_acl, BUCKET_NAME);
    test_create_bucket(canned_acl, bucket_src);
    test_create_bucket(canned_acl, bucket_target);
    test_create_bucket_with_option(canned_acl, bucket_version, OBS_STORAGE_CLASS_GLACIER, "R1");
    test_get_bucket_storage_class(bucket_version);
    
    //head bucket
    test_head_bucket(BUCKET_NAME);
    // list bucket
    test_list_bucket();
    // quota
    test_set_bucket_quota(bucket_src);
    test_get_bucket_quota(bucket_src);
    //policy
    test_set_bucket_policy(bucket_src);
    test_get_bucket_policy(bucket_src);
    test_delete_bucket_policy(bucket_src);
    //version
    test_set_bucket_version(bucket_version, OBS_VERSION_STATUS_ENABLED);
    test_get_bucket_version(bucket_version);
    // storage class
    test_set_bucket_storage_class(bucket_src, OBS_STORAGE_CLASS_GLACIER);
    test_get_bucket_storage_class(bucket_src);
    //tagging
    test_set_bucket_tagging(bucket_src);
    test_get_bucket_tagging(bucket_src);
    test_delete_bucket_tagging(bucket_src); 
    test_get_bucket_tagging(bucket_src);
    // test bucket logging
    test_set_bucket_logging_without_grant(bucket_src, bucket_target);
    test_get_bucket_logging(bucket_src);
    test_set_bucket_logging_with_grant(bucket_target, bucket_src);
    test_get_bucket_logging(bucket_target);
    test_close_bucket_logging(bucket_src);
    test_get_bucket_logging(bucket_src);
    //website 
    test_set_bucket_website_all(bucket_src);
    test_get_bucket_website(bucket_src);
    test_set_bucket_website_conf(bucket_src);
    test_get_bucket_website(bucket_src);
    test_delete_bucket_website(bucket_src); 
    test_get_bucket_website(bucket_src);
    //acl
    test_set_bucket_acl(bucket_src);
    test_get_bucket_acl(bucket_src);
    test_set_bucket_acl_byhead(bucket_src);
    test_get_bucket_acl(bucket_src);
    //notification
    test_set_notification_configuration(bucket_src);   
    test_get_notification_config(bucket_src);
    test_close_notification_configuration(bucket_src);
    test_get_notification_config(bucket_src);
    //delete bucket
    test_delete_bucket(bucket_src);
    test_delete_bucket(bucket_target);
    // get storage info
    test_get_bucket_storage_info(BUCKET_NAME);
    // lifecycle
    test_set_bucket_lifecycle_configuration(BUCKET_NAME); 
    test_get_lifecycle_config(BUCKET_NAME);
    test_set_bucket_lifecycle_configuration2(BUCKET_NAME);
    test_get_lifecycle_config(BUCKET_NAME);
    test_delete_lifecycle_config(BUCKET_NAME);
    //cors
    test_set_bucket_cors(BUCKET_NAME);   
    test_get_cors_config(BUCKET_NAME);
    test_delete_cors_config(BUCKET_NAME);
    //option rely on cors configuration
    test_set_bucket_cors(BUCKET_NAME); 
    test_bucket_option(BUCKET_NAME);
    test_object_option(BUCKET_NAME, "key");
    test_delete_cors_config(BUCKET_NAME);
    // get bucket metadata
    test_get_bucket_metadata_with_cors(BUCKET_NAME);

     /*------ object test------*/
    create_and_write_file("./put_file_test.txt", 8*1024);
    create_and_write_file("./test.txt", 8*1024);
    // put object
    test_put_object_from_file(BUCKET_NAME, "put_file_test", "./put_file_test.txt");
    test_put_object_from_buffer(BUCKET_NAME, "put_buffer_test");
    init_certificate_by_path(OBS_PROTOCOL_HTTPS, OBS_DEFAULT_CERTIFICATE, NULL, 0);
    test_put_object_by_kms_encrypt(BUCKET_NAME, "put_buffer_kms");
    test_put_object_by_aes_encrypt(BUCKET_NAME, "put_buffer_aes");
    init_certificate_by_path(OBS_PROTOCOL_HTTP, OBS_NO_CERTIFICATE, NULL, 0);
    // get object
    test_get_object(BUCKET_NAME, "put_buffer_test");
    init_certificate_by_path(OBS_PROTOCOL_HTTPS, OBS_DEFAULT_CERTIFICATE, NULL, 0);
    test_get_object_by_kms_encrypt(BUCKET_NAME, "put_buffer_kms");
    test_get_object_by_aes_encrypt(BUCKET_NAME, "put_buffer_aes");
    init_certificate_by_path(OBS_PROTOCOL_HTTP, OBS_NO_CERTIFICATE, NULL, 0);
    // head object
    test_head_object("put_buffer_test",BUCKET_NAME);
    //list objects
    test_list_bucket_objects(BUCKET_NAME);
	// copy part
    test_init_upload_part(BUCKET_NAME, "test516");
    test_copy_part(BUCKET_NAME, "put_file_test", "test516");
    test_list_parts(BUCKET_NAME, "test516");
    test_list_multipart_uploads(BUCKET_NAME);
    test_abort_multi_part_upload(UPLOAD_ID, BUCKET_NAME, "test516");
    // mulit part	
    create_and_write_file("./test1.txt", 8*1024*1024);
    test_init_upload_part(BUCKET_NAME, "test517");
    test_upload_part("./test1.txt", BUCKET_NAME, "test517");
    test_list_parts(BUCKET_NAME, "test517");
    test_list_multipart_uploads(BUCKET_NAME);
    test_complete_upload(UPLOAD_ID, UPLOAD_ETAG, BUCKET_NAME, "test517");
    // 上传文件
    test_upload_file(BUCKET_NAME, "./test1.txt", "test54");
    // 下载文件
    test_download_file(BUCKET_NAME, "./test518", "test54");
    // Concurrent partial upload
    test_concurrent_upload_part(BUCKET_NAME, "./test1.txt", "test551", 5L * 1024 * 1024);
    // 获取对象属性
    test_get_object_metadata("put_file_test", NULL);
    // test object acl
    test_create_bucket(canned_acl, bucket_obj_acl);
    test_set_bucket_version(bucket_obj_acl, OBS_VERSION_STATUS_ENABLED);
    test_put_object_by_strorageclass("obj3", "./put_file_test.txt", bucket_obj_acl, OBS_STORAGE_CLASS_STANDARD);
    test_list_versions(bucket_obj_acl, NULL);
    test_set_object_acl_byhead("obj3", OBJECT_VER[0], bucket_obj_acl);
    test_set_object_acl("obj3", OBJECT_VER[0], bucket_obj_acl);
    test_get_object_acl("obj3", OBJECT_VER[0], bucket_obj_acl);
    // test batch delete
    test_put_object_by_strorageclass("obj5","./put_file_test.txt", bucket_obj_acl, OBS_STORAGE_CLASS_STANDARD);
    test_list_versions(bucket_obj_acl, NULL);
    test_batch_delete_objects(bucket_obj_acl, "obj3", "obj5");
    //test copy object
    test_put_object_by_strorageclass("obj4", "./put_file_test.txt", bucket_obj_acl, OBS_STORAGE_CLASS_STANDARD);
    test_copy_object("obj4", OBJECT_VER[0], bucket_obj_acl, BUCKET_NAME, "objn");
    test_list_versions(bucket_obj_acl, NULL);
    test_delete_object("obj4", OBJECT_VER[0], bucket_obj_acl);
    // test restore
    char *bucket_restore = "bucket-restore-api";
    char *obj_restore_key = "obj-restore1";
    char *days = "10";
    test_put_object_by_strorageclass(obj_restore_key, "./put_file_test.txt", BUCKET_NAME, OBS_STORAGE_CLASS_GLACIER); 
    test_restore_object(obj_restore_key, NULL, days, BUCKET_NAME);
    // append object
    test_append_object_from_file(BUCKET_NAME, "obj524", "./put_file_test.txt", "0");
    char *buffer = "hello.";
    test_append_object_from_buffer(BUCKET_NAME, "obj525", buffer, strlen(buffer), "0");
    test_append_object_from_buffer(BUCKET_NAME, "obj525", buffer, strlen(buffer), "6");
    // 生成临时鉴权的URL
    test_gen_signed_url_put_object(BUCKET_NAME, "put_file_test");
    test_gen_signed_url_get_object(BUCKET_NAME, "put_file_test", NULL);
    test_list_bucket_objects(BUCKET_NAME);
    test_delete_object("put_file_test", NULL, BUCKET_NAME);
    test_delete_object("put_buffer_test", NULL, BUCKET_NAME);
    test_delete_object("put_buffer_kms", NULL, BUCKET_NAME);
    test_delete_object("put_buffer_aes", NULL, BUCKET_NAME);
    test_delete_object("test517", NULL, BUCKET_NAME);
    test_delete_object("test54", NULL, BUCKET_NAME);
    test_delete_object("test551", NULL, BUCKET_NAME);
    test_delete_object(obj_restore_key, NULL, BUCKET_NAME);
    test_delete_object("objn", NULL, BUCKET_NAME);
    test_delete_object("obj524", NULL, BUCKET_NAME);
    test_delete_object("obj525", NULL, BUCKET_NAME);
    test_delete_bucket(BUCKET_NAME);
    test_delete_bucket(bucket_version);
    test_delete_bucket(bucket_obj_acl);

    char *bucket_cert = "bucket-cert";
    test_create_bucket_with_cert(OBS_CANNED_ACL_PUBLIC_READ, bucket_cert, 0, NULL, 0);
    test_delete_bucket(bucket_cert);
    
    test_create_bucket_with_cert(OBS_CANNED_ACL_PUBLIC_READ, bucket_cert, 1, NULL, 0);
    test_delete_bucket(bucket_cert);
    
    test_create_bucket_with_cert(OBS_CANNED_ACL_PUBLIC_READ, bucket_cert, 2, "./client.pem", 12);
    test_delete_bucket(bucket_cert);
    
    test_create_bucket_with_cert(OBS_CANNED_ACL_PUBLIC_READ, bucket_cert, 3, NULL, 0);  
    test_delete_bucket(bucket_cert);
    /*------ deinitialize------*/
    obs_deinitialize();   
}



