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

void generate_redirect(update_bucket_common_data *website_data,
    obs_set_bucket_website_conf *set_bucket_website_conf, int i)
{

    int tmplen = 0;
    int mark = 0;

    if (set_bucket_website_conf->routingrule_info[i].protocol)
    {
        char* pprotocol = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].protocol, &pprotocol);
        tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<Protocol>%s</Protocol>",
            mark ? pprotocol : set_bucket_website_conf->routingrule_info[i].protocol);
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
        website_data->docLen += tmplen;
        free(pprotocol);
    }
    if (set_bucket_website_conf->routingrule_info[i].host_name)
    {
        char* phostName = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].host_name, &phostName);
        tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE, "<HostName>%s</HostName>",
            mark ? phostName : set_bucket_website_conf->routingrule_info[i].host_name);
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
        website_data->docLen += tmplen;
        free(phostName);
    }
    if (set_bucket_website_conf->routingrule_info[i].replace_key_prefix_with)
    {
        char* preplaceKeyPrefixWith = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].replace_key_prefix_with, &preplaceKeyPrefixWith);
        tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<ReplaceKeyPrefixWith>%s</ReplaceKeyPrefixWith>",
            mark ? preplaceKeyPrefixWith : set_bucket_website_conf->routingrule_info[i].replace_key_prefix_with);
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
        website_data->docLen += tmplen;
        free(preplaceKeyPrefixWith);
    }
    if (set_bucket_website_conf->routingrule_info[i].replace_key_with)
    {
        char* preplaceKeyWith = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].replace_key_with, &preplaceKeyWith);
        tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<ReplaceKeyWith>%s</ReplaceKeyWith>",
            mark ? preplaceKeyWith : set_bucket_website_conf->routingrule_info[i].replace_key_with);
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
        website_data->docLen += tmplen;
        free(preplaceKeyWith);
    }
    if (set_bucket_website_conf->routingrule_info[i].http_redirect_code)
    {
        char*phttpRedirectCode = 0;
        mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].http_redirect_code, &phttpRedirectCode);
        tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<HttpRedirectCode>%s</HttpRedirectCode>",
            mark ? phttpRedirectCode : set_bucket_website_conf->routingrule_info[i].http_redirect_code);
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
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

    tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
        sizeof(website_data->doc) - website_data->docLen,
        _TRUNCATE, "<RoutingRules>");
    CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
    website_data->docLen += tmplen;

    for (i = 0; i < set_bucket_website_conf->routingrule_count; i++)
    {
        tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
            sizeof(website_data->doc) - website_data->docLen,
            _TRUNCATE, "<RoutingRule><Condition>");
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
        website_data->docLen += tmplen;
        if (set_bucket_website_conf->routingrule_info[i].key_prefix_equals)
        {
            char* pkeyPrefixEquals = 0;
            mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].key_prefix_equals, &pkeyPrefixEquals);
            tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
                sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
                "<KeyPrefixEquals>%s</KeyPrefixEquals>",
                mark ? pkeyPrefixEquals : set_bucket_website_conf->routingrule_info[i].key_prefix_equals);
            CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
            website_data->docLen += tmplen;
            free(pkeyPrefixEquals);
        }
        if (set_bucket_website_conf->routingrule_info[i].http_errorcode_returned_equals)
        {
            char* phttpErrorCodeReturnedEquals = 0;
            mark = pcre_replace(set_bucket_website_conf->routingrule_info[i].http_errorcode_returned_equals, &phttpErrorCodeReturnedEquals);
            tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
                sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
                "<HttpErrorCodeReturnedEquals>%s</HttpErrorCodeReturnedEquals>",
                mark ? phttpErrorCodeReturnedEquals : set_bucket_website_conf->routingrule_info[i].http_errorcode_returned_equals);
            CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
            website_data->docLen += tmplen;
            free(phttpErrorCodeReturnedEquals);
        }

        tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
            sizeof(website_data->doc) - website_data->docLen,
            _TRUNCATE, "</Condition><Redirect>");
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
        website_data->docLen += tmplen;

        generate_redirect(website_data, set_bucket_website_conf, i);

        tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
            sizeof(website_data->doc) - website_data->docLen,
            _TRUNCATE, "</Redirect></RoutingRule>");
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
        website_data->docLen += tmplen;
    }

    tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
        sizeof(website_data->doc) - website_data->docLen,
        _TRUNCATE, "</RoutingRules>");
    CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
    website_data->docLen += tmplen;

}

obs_status generate_websiteconf_doc(update_bucket_common_data **data,
    obs_set_bucket_website_conf *set_bucket_website_conf,
    obs_response_handler *handler)
{
    update_bucket_common_data *website_data = *data;
    int tmplen = 0;
    int mark = 0;

    if (NULL == set_bucket_website_conf->suffix)
    {
        COMMLOG(OBS_LOGERROR, "set_bucket_website_conf suffix is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return OBS_STATUS_InvalidParameter;
    }

    website_data->docLen = snprintf_s(website_data->doc, sizeof(website_data->doc),
        _TRUNCATE, "<WebsiteConfiguration>");

    char* psuffix = 0;
    mark = pcre_replace(set_bucket_website_conf->suffix, &psuffix);
    if (website_data->docLen >= 0 && website_data->docLen < sizeof(website_data->doc)) {
        tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<IndexDocument><Suffix>%s</Suffix></IndexDocument>",
            mark ? psuffix : set_bucket_website_conf->suffix);
    } else {
        tmplen = 0;
            COMMLOG(OBS_LOGERROR, "website_data->docLen is not beetween 0 and sizeof(website_data->doc)"
                "in function: %s, line: %d", __FUNCTION__, __LINE__);
    }
    CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
    website_data->docLen += tmplen;
    free(psuffix);

    if (set_bucket_website_conf->key)
    {
        char*pkey = 0;
        mark = pcre_replace(set_bucket_website_conf->key, &pkey);
        if (website_data->docLen >= 0 && website_data->docLen < sizeof(website_data->doc)) {
            tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
                sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
                "<ErrorDocument><Key>%s</Key></ErrorDocument>",
                mark ? pkey : set_bucket_website_conf->key);
        } else {
            tmplen = 0;
            COMMLOG(OBS_LOGERROR, "website_data->docLen is not beetween 0 and sizeof(website_data->doc)"
                "in function: %s, line: %d", __FUNCTION__, __LINE__);
        }
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
        website_data->docLen += tmplen;
        free(pkey);
    }

    generate_routingrules(website_data, set_bucket_website_conf);

    tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
        sizeof(website_data->doc) - website_data->docLen,
        _TRUNCATE, "</WebsiteConfiguration>");
    CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
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

    website_data->docLen = snprintf_s(website_data->doc, sizeof(website_data->doc),
        _TRUNCATE, "<WebsiteConfiguration>");

    if (website_data->docLen < 0)
    {
        COMMLOG(OBS_LOGERROR, "snprintf_s error!");
        return OBS_STATUS_InternalError;
    }

    if (NULL == set_bucket_redirect_all->host_name)
    {
        COMMLOG(OBS_LOGERROR, "set_bucket_redirect_all host_name is NULL!");
        (void)(*(handler->complete_callback))(OBS_STATUS_InvalidParameter, 0, 0);
        return OBS_STATUS_InvalidParameter;
    }

    tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
        sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
        "<RedirectAllRequestsTo>");
    CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
    website_data->docLen += tmplen;

    char* phostName = 0;
    mark = pcre_replace(set_bucket_redirect_all->host_name, &phostName);
    tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
        sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
        "<HostName>%s</HostName>", mark ? phostName : set_bucket_redirect_all->host_name);
    CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);

    website_data->docLen += tmplen;
    free(phostName);
    phostName = NULL;

    if (set_bucket_redirect_all->protocol)
    {
        char*pprotocol = 0;
        mark = pcre_replace(set_bucket_redirect_all->protocol, &pprotocol);
        tmplen = snprintf_s(website_data->doc + website_data->docLen,
            sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
            "<Protocol>%s</Protocol>", mark ? pprotocol : set_bucket_redirect_all->protocol);
        CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
        website_data->docLen += tmplen;
        free(pprotocol);
        pprotocol = NULL;
    }

    tmplen = snprintf_s((website_data->doc) + (website_data->docLen),
        sizeof(website_data->doc) - website_data->docLen, _TRUNCATE,
        "</RedirectAllRequestsTo></WebsiteConfiguration>");
    CheckAndLogNeg(tmplen, "snprintf_s", __FUNCTION__, __LINE__);
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
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_put;
    params.properties_callback = &update_bucket_common_properties_callback;
    params.toObsCallback = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = website_data->docLen;
    params.complete_callback = &update_bucket_common_complete_callback;
    params.callback_data = website_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "website";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "set bucket website configuration finish!");
}