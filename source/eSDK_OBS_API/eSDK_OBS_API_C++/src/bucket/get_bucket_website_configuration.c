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
#include "bucket.h"
#include "request_util.h"
#include <openssl/md5.h> 

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
    data->webdata_count = 0;
    initialize_bucketwebsite(data->webdata);
}


obs_status make_get_bucket_websitedata_callback(obs_bucket_website_configuration_data *data)
{
    obs_status status = OBS_STATUS_OK;
    bucket_website_routingrule *websitein = NULL;
    if (data->webdata_count > 0)
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
    int i;
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
        (data->hostname, data->protocol, data->suffix, data->key, websitein, webdata_count, data->callback_data);

    CHECK_NULL_FREE(websitein);
    return status;
}

obs_status get_bucket_websiteconf_xml_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    obs_bucket_website_configuration_data *website_data =
        (obs_bucket_website_configuration_data *)callback_data;

    int fit = 1;

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

    if (!strcmp(element_path, "WebsiteConfiguration/RedirectAllRequestsTo/HostName"))
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
    else_if(!strcmp(element_path, "WebsiteConfiguration/RoutingRules/RoutingRule/Redirect/ReplaceKeyPrefixWith")) {
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

    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}


obs_status get_bucket_websiteconf_properties_callback
(const obs_response_properties *response_properties, void *callback_data)
{
    obs_bucket_website_configuration_data *websiteconf_data =
        (obs_bucket_website_configuration_data *)callback_data;
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
        (obs_bucket_website_configuration_data *)callback_data;
    return simplexml_add(&(websiteconf_data->simpleXml), buffer, buffer_size);
}

void get_bucket_websiteconf_complete_callback(obs_status status, const obs_error_details *error_details,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    obs_bucket_website_configuration_data *websiteconf_data =
        (obs_bucket_website_configuration_data *)callback_data;
    if (OBS_STATUS_OK == status)
    {
        obs_status ret = make_get_bucket_websitedata_callback(websiteconf_data);
        if (ret != OBS_STATUS_OK) {
            COMMLOG(OBS_LOGWARN, "Failed to call make_get_bucket_websitedata_callback, status: %d", ret);
        }
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
        (obs_bucket_website_configuration_data*)malloc(sizeof(obs_bucket_website_configuration_data));
    if (!websiteconf_data)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
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
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_websiteconf_properties_callback;
    params.fromObsCallback = &get_bucket_websiteconf_data_callback;
    params.complete_callback = &get_bucket_websiteconf_complete_callback;
    params.callback_data = websiteconf_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "website";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket website configuration finish!");
}