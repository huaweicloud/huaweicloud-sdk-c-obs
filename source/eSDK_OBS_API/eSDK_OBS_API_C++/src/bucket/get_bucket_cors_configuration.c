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
#define OBS_MAX_CORS_COUNT 65536

static obs_status add_one_get_cors_data(const char *element_path,
    get_bucket_cors_data *gbccDataEx)
{
    int nIndex = gbccDataEx->bccd_number - 1;

    if (!strcmp(element_path, "CORSConfiguration/CORSRule"))
    {
        bucket_cors_conf_data* bcc_data = (bucket_cors_conf_data*)malloc(sizeof(bucket_cors_conf_data));
        if (!bcc_data)
        {
            COMMLOG(OBS_LOGERROR, "malloc bucket_cors_conf_data failed.");
            return OBS_STATUS_OutOfMemory;
        }
        memset_s(bcc_data, sizeof(bucket_cors_conf_data), 0, sizeof(bucket_cors_conf_data));
        gbccDataEx->bcc_data[gbccDataEx->bccd_number] = bcc_data;
        gbccDataEx->bccd_number++;
    }

    if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedMethod")) {
        gbccDataEx->bcc_data[nIndex]->allowedMethodCount++;
        gbccDataEx->bcc_data[nIndex]->allowedMethodes[gbccDataEx->bcc_data[nIndex]->allowedMethodCount][0] = 0;
        gbccDataEx->bcc_data[nIndex]->allowedMethodLens[gbccDataEx->bcc_data[nIndex]->allowedMethodCount] = 0;
    }
    else if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedOrigin")) {
        gbccDataEx->bcc_data[nIndex]->allowedOriginCount++;
        gbccDataEx->bcc_data[nIndex]->allowedOrigines[gbccDataEx->bcc_data[nIndex]->allowedOriginCount][0] = 0;
        gbccDataEx->bcc_data[nIndex]->allowedOriginLens[gbccDataEx->bcc_data[nIndex]->allowedOriginCount] = 0;
    }
    else if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedHeader")) {
        gbccDataEx->bcc_data[nIndex]->allowedHeaderCount++;
        gbccDataEx->bcc_data[nIndex]->allowedHeaderes[gbccDataEx->bcc_data[nIndex]->allowedHeaderCount][0] = 0;
        gbccDataEx->bcc_data[nIndex]->allowedHeaderLens[gbccDataEx->bcc_data[nIndex]->allowedHeaderCount] = 0;
    }
    else if (!strcmp(element_path, "CORSConfiguration/CORSRule/ExposeHeader")) {
        gbccDataEx->bcc_data[nIndex]->exposeHeaderCount++;
        gbccDataEx->bcc_data[nIndex]->exposeHeaderes[gbccDataEx->bcc_data[nIndex]->exposeHeaderCount][0] = 0;
        gbccDataEx->bcc_data[nIndex]->exposeHeaderLens[gbccDataEx->bcc_data[nIndex]->exposeHeaderCount] = 0;
    }

    return OBS_STATUS_OK;
}

static obs_status get_cors_xml_callback(const char *element_path,
    const char *data, int data_len,
    void *callback_data)
{
    int fit = 1;
    obs_status ret_status = OBS_STATUS_OK;
    get_bucket_cors_data *gbccDataEx = (get_bucket_cors_data *)callback_data;
    int nIndex = gbccDataEx->bccd_number - 1;

    if (data) {
        if (!strcmp(element_path, "CORSConfiguration/CORSRule/ID")) {
            string_buffer_append(gbccDataEx->bcc_data[nIndex]->id, data, data_len, fit);
        }
        else if (!strcmp(element_path, "CORSConfiguration/CORSRule/MaxAgeSeconds")) {
            string_buffer_append(gbccDataEx->bcc_data[nIndex]->max_age_seconds, data, data_len, fit);
        }
        else if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedMethod")) {
            int which = gbccDataEx->bcc_data[nIndex]->allowedMethodCount;
            gbccDataEx->bcc_data[nIndex]->allowedMethodLens[which] +=
                snprintf_s(gbccDataEx->bcc_data[nIndex]->allowedMethodes[which],
                    sizeof(gbccDataEx->bcc_data[nIndex]->allowedMethodes[which]),
                    sizeof(gbccDataEx->bcc_data[nIndex]->allowedMethodes[which]) -
                    gbccDataEx->bcc_data[nIndex]->allowedMethodLens[which] - 1,
                    "%.*s", data_len, data);
            if (gbccDataEx->bcc_data[nIndex]->allowedMethodLens[which] >=
                (int) sizeof(gbccDataEx->bcc_data[nIndex]->allowedMethodes[which])) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
        else if (!strcmp(element_path, "CORSConfiguration/CORSRule/AllowedOrigin")) {
            int which = gbccDataEx->bcc_data[nIndex]->allowedOriginCount;
            gbccDataEx->bcc_data[nIndex]->allowedOriginLens[which] +=
                snprintf_s(gbccDataEx->bcc_data[nIndex]->allowedOrigines[which],
                    sizeof(gbccDataEx->bcc_data[nIndex]->allowedOrigines[which]),
                    sizeof(gbccDataEx->bcc_data[nIndex]->allowedOrigines[which]) -
                    gbccDataEx->bcc_data[nIndex]->allowedOriginLens[which] - 1,
                    "%.*s", data_len, data);
            if (gbccDataEx->bcc_data[nIndex]->allowedOriginLens[which] >=
                (int) sizeof(gbccDataEx->bcc_data[nIndex]->allowedOrigines[which])) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
        else if (!strcmp(element_path,
            "CORSConfiguration/CORSRule/AllowedHeader")) {
            int which = gbccDataEx->bcc_data[nIndex]->allowedHeaderCount;
            gbccDataEx->bcc_data[nIndex]->allowedHeaderLens[which] +=
                snprintf_s(gbccDataEx->bcc_data[nIndex]->allowedHeaderes[which],
                    sizeof(gbccDataEx->bcc_data[nIndex]->allowedHeaderes[which]),
                    sizeof(gbccDataEx->bcc_data[nIndex]->allowedHeaderes[which]) -
                    gbccDataEx->bcc_data[nIndex]->allowedHeaderLens[which] - 1,
                    "%.*s", data_len, data);
            if (gbccDataEx->bcc_data[nIndex]->allowedHeaderLens[which] >=
                (int) sizeof(gbccDataEx->bcc_data[nIndex]->allowedHeaderes[which])) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
        else if (!strcmp(element_path, "CORSConfiguration/CORSRule/ExposeHeader")) {
            int which = gbccDataEx->bcc_data[nIndex]->exposeHeaderCount;
            gbccDataEx->bcc_data[nIndex]->exposeHeaderLens[which] +=
                snprintf_s(gbccDataEx->bcc_data[nIndex]->exposeHeaderes[which],
                    sizeof(gbccDataEx->bcc_data[nIndex]->exposeHeaderes[which]),
                    sizeof(gbccDataEx->bcc_data[nIndex]->exposeHeaderes[which]) -
                    gbccDataEx->bcc_data[nIndex]->exposeHeaderLens[which] - 1,
                    "%.*s", data_len, data);
            if (gbccDataEx->bcc_data[nIndex]->exposeHeaderLens[which] >=
                (int) sizeof(gbccDataEx->bcc_data[nIndex]->exposeHeaderes[which])) {
                return OBS_STATUS_XmlParseFailure;
            }
        }
    }
    else
    {
        ret_status = add_one_get_cors_data(element_path, gbccDataEx);
        if (OBS_STATUS_OK != ret_status)
        {
            return ret_status;
        }
    }

    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}


static get_bucket_cors_data* init_get_cors_data(obs_cors_handler *handler,
    void *callback_data)
{
    get_bucket_cors_data *gbccDataEx = NULL;
    bucket_cors_conf_data *bcc_data = NULL;

    gbccDataEx = (get_bucket_cors_data *)malloc(sizeof(get_bucket_cors_data));
    if (!gbccDataEx) {
        COMMLOG(OBS_LOGERROR, "malloc cors config data failed.");
        return NULL;
    }
    memset_s(gbccDataEx, sizeof(get_bucket_cors_data), 0, sizeof(get_bucket_cors_data));

    simplexml_initialize(&(gbccDataEx->simple_xml_info), &get_cors_xml_callback, gbccDataEx);

    gbccDataEx->response_properties_callback = handler->response_handler.properties_callback;
    gbccDataEx->response_complete_callback = handler->response_handler.complete_callback;
    gbccDataEx->get_cors_callback = handler->get_cors_callback;
    gbccDataEx->callback_data = callback_data;

    bcc_data = (bucket_cors_conf_data*)malloc(sizeof(bucket_cors_conf_data));
    if (!bcc_data)
    {
        COMMLOG(OBS_LOGERROR, "malloc bucket_cors_conf_data failed.");
        free(gbccDataEx);
        return NULL;
    }
    memset_s(bcc_data, sizeof(bucket_cors_conf_data), 0, sizeof(bucket_cors_conf_data));
    gbccDataEx->bcc_data[0] = bcc_data;
    gbccDataEx->bccd_number = 1;

    return gbccDataEx;
}

static void free_obs_bucket_cors_conf(obs_bucket_cors_conf* bucketCorsConf, int conf_num)
{
    int i = 0;
    for (i = 0; i < conf_num; i++)
    {
        if (bucketCorsConf[i].allowed_method)
        {
            free(bucketCorsConf[i].allowed_method);
            bucketCorsConf[i].allowed_method = NULL;
        }

        if (bucketCorsConf[i].allowed_origin)
        {
            free(bucketCorsConf[i].allowed_origin);
            bucketCorsConf[i].allowed_origin = NULL;
        }

        if (bucketCorsConf[i].allowed_header)
        {
            free(bucketCorsConf[i].allowed_header);
            bucketCorsConf[i].allowed_header = NULL;
        }

        if (bucketCorsConf[i].expose_header)
        {
            free(bucketCorsConf[i].expose_header);
            bucketCorsConf[i].expose_header = NULL;
        }
    }

    free(bucketCorsConf);
    return;
}
static const char** set_return_cors_value(char in_char[][1024], int char_count)
{
    const char **out_char = NULL;
    int i = 0;
    if (char_count <= 0 || char_count > OBS_MAX_CORS_COUNT) {
        COMMLOG(OBS_LOGERROR, "parameter of malloc is out of range in function: %s,line %d", __FUNCTION__, __LINE__);
        return NULL;
    }
    out_char = (const char**)malloc(sizeof(char *) * char_count);
    if (NULL == out_char)
    {
        return NULL;
    }
    int ret = memset_s(out_char, sizeof(char *) * char_count, 0, sizeof(char *) * char_count);
    if(ret != 0){
        COMMLOG(OBS_LOGERROR, "memset_s failed in function: %s, line: %d", __FUNCTION__, __LINE__);
        free(out_char);
        return NULL;
    }
    for (i = 0; i < char_count; i++)
    {
        out_char[i] = in_char[i];
    }

    return out_char;
}


static obs_status make_get_cors_callbackEx(get_bucket_cors_data *gbccDataEx)
{
    obs_status iRet = OBS_STATUS_OK;
    int i = 0;
    int is_get_data_err = 0;
    int nCount = gbccDataEx->bccd_number - 1;
    if (nCount <= 0)
    {
        COMMLOG(OBS_LOGERROR, "bccd is empty,bccd_num(%u).", gbccDataEx->bccd_number);
        return OBS_STATUS_InternalError;
    }

    obs_bucket_cors_conf* bucketCorsConf = (obs_bucket_cors_conf*)malloc(sizeof(obs_bucket_cors_conf) * nCount);
    if (NULL == bucketCorsConf)
    {
        COMMLOG(OBS_LOGERROR, "malloc obs_bucket_cors_conf failed.");
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(bucketCorsConf, sizeof(obs_bucket_cors_conf) * nCount, 0, sizeof(obs_bucket_cors_conf) * nCount);

    for (; i < nCount; ++i)
    {
        COMMLOG(OBS_LOGINFO, "get cors config err,Method(%d),Origin(%d),Header(%d),expose(%d).",
            gbccDataEx->bcc_data[i]->allowedMethodCount, gbccDataEx->bcc_data[i]->allowedOriginCount,
            gbccDataEx->bcc_data[i]->allowedHeaderCount, gbccDataEx->bcc_data[i]->exposeHeaderCount);
        is_get_data_err = (gbccDataEx->bcc_data[i]->allowedMethodCount < 1)
            || (gbccDataEx->bcc_data[i]->allowedOriginCount < 1);
        if (is_get_data_err)
        {
            free_obs_bucket_cors_conf(bucketCorsConf, i);
            return OBS_STATUS_OutOfMemory;
        }

        bucketCorsConf[i].id = gbccDataEx->bcc_data[i]->id;

        bucketCorsConf[i].allowed_method = set_return_cors_value(
            gbccDataEx->bcc_data[i]->allowedMethodes, gbccDataEx->bcc_data[i]->allowedMethodCount);
        bucketCorsConf[i].allowed_method_number = gbccDataEx->bcc_data[i]->allowedMethodCount;

        bucketCorsConf[i].allowed_origin = set_return_cors_value(
            gbccDataEx->bcc_data[i]->allowedOrigines, gbccDataEx->bcc_data[i]->allowedOriginCount);
        bucketCorsConf[i].allowed_origin_number = gbccDataEx->bcc_data[i]->allowedOriginCount;

        if (gbccDataEx->bcc_data[i]->allowedHeaderCount > 0)
        {
            bucketCorsConf[i].allowed_header = set_return_cors_value(
                gbccDataEx->bcc_data[i]->allowedHeaderes, gbccDataEx->bcc_data[i]->allowedHeaderCount);
            bucketCorsConf[i].allowed_header_number = gbccDataEx->bcc_data[i]->allowedHeaderCount;
        }

        bucketCorsConf[i].max_age_seconds = gbccDataEx->bcc_data[i]->max_age_seconds;

        if (gbccDataEx->bcc_data[i]->exposeHeaderCount > 0)
        {
            bucketCorsConf[i].expose_header = set_return_cors_value(
                gbccDataEx->bcc_data[i]->exposeHeaderes, gbccDataEx->bcc_data[i]->exposeHeaderCount);
            bucketCorsConf[i].expose_header_number = gbccDataEx->bcc_data[i]->exposeHeaderCount;
        }

        COMMLOG(OBS_LOGINFO, "get cors config err,Method(%p),Origin(%p),Header(%p),expose(%p).",
            bucketCorsConf[i].allowed_method, bucketCorsConf[i].allowed_origin,
            bucketCorsConf[i].allowed_header, bucketCorsConf[i].expose_header);
        is_get_data_err = (NULL == bucketCorsConf[i].allowed_method)
            || (NULL == bucketCorsConf[i].allowed_origin)
            || (gbccDataEx->bcc_data[i]->allowedHeaderCount > 0
                && NULL == bucketCorsConf[i].allowed_header)
            || (gbccDataEx->bcc_data[i]->exposeHeaderCount > 0
                && NULL == bucketCorsConf[i].expose_header);
        if (is_get_data_err)
        {
            free_obs_bucket_cors_conf(bucketCorsConf, i + 1);
            return OBS_STATUS_OutOfMemory;
        }
    }

    iRet = (*(gbccDataEx->get_cors_callback))(bucketCorsConf, nCount, gbccDataEx->callback_data);

    free_obs_bucket_cors_conf(bucketCorsConf, nCount);

    return iRet;
}


static obs_status get_cors_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    get_bucket_cors_data *gbccDataEx = (get_bucket_cors_data *)callback_data;
    if (gbccDataEx->response_properties_callback)
    {
        return (*(gbccDataEx->response_properties_callback))
            (response_properties, gbccDataEx->callback_data);
    }
    return OBS_STATUS_OK;
}

static obs_status get_cors_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
    get_bucket_cors_data *gbccDataEx = (get_bucket_cors_data *)callback_data;

    return simplexml_add(&(gbccDataEx->simple_xml_info), buffer, buffer_size);
}


static void get_cors_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);
    get_bucket_cors_data *gbccDataEx = (get_bucket_cors_data *)callback_data;

    if (gbccDataEx->bccd_number && OBS_STATUS_OK == request_status) {
        request_status = make_get_cors_callbackEx(gbccDataEx);
    }

    (*(gbccDataEx->response_complete_callback))(request_status, obs_error_info,
        gbccDataEx->callback_data);

    simplexml_deinitialize(&(gbccDataEx->simple_xml_info));

    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void get_bucket_cors_configuration(const obs_options *options, obs_cors_handler *handler,
    void *callback_data)
{
    request_params params;
    get_bucket_cors_data *gbccDataEx = NULL;
    obs_use_api use_api = OBS_USE_API_S3;

    COMMLOG(OBS_LOGINFO, "get_bucket_cors_configuration start !");

    gbccDataEx = init_get_cors_data(handler, callback_data);
    
    if (NULL == gbccDataEx)
    {
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, &handler->response_handler, callback_data)) {
		return;
	}

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_cors_properties_callback;
    params.fromObsCallback = &get_cors_data_callback;
    params.complete_callback = &get_cors_complete_callback;
    params.callback_data = gbccDataEx;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "cors";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "get_bucket_lifecycle_configuration finish.");
}
