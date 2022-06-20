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

static obs_status get_bucket_quotaxml_callback(const char *element_path, const char *data,
    int data_len, void *callback_data)
{
    get_bucket_common_data *bucket_quota_data = (get_bucket_common_data *)callback_data;
    int fit = 1;
    if (data && !strcmp(element_path, "Quota/StorageQuota")) {
        string_buffer_append(bucket_quota_data->common_data, data, data_len, fit);
    }

    //(void) fit;
    if (!fit) {
        COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
    }
    return OBS_STATUS_OK;
}


static obs_status get_bucket_quota_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    get_bucket_common_data *bucket_quota_data = (get_bucket_common_data *)callback_data;
    if (bucket_quota_data->responsePropertiesCallback)
    {
        return (*(bucket_quota_data->responsePropertiesCallback))
            (response_properties, bucket_quota_data->callback_data);
    }
    return OBS_STATUS_OK;
}


static obs_status get_bucket_quotadata_callback(int buffer_size, const char *buffer,
    void *callback_data)
{
    get_bucket_common_data *gbqData = (get_bucket_common_data *)callback_data;
    return simplexml_add(&(gbqData->simpleXml), buffer, buffer_size);
}

static void get_bucket_quota_complete_callback(obs_status status,
    const obs_error_details *error_details,
    void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    get_bucket_common_data *bucket_quota_data = (get_bucket_common_data *)callback_data;
    if (OBS_STATUS_OK == status)
    {
        *(bucket_quota_data->ul_return) = atol(bucket_quota_data->common_data);
    }

    (void)(*(bucket_quota_data->responseCompleteCallback))(status, error_details,
        bucket_quota_data->callback_data);

    simplexml_deinitialize(&(bucket_quota_data->simpleXml));

    free(bucket_quota_data);
    bucket_quota_data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}


/*****************************************************************************
*   Prototype    : get_bucket_quota
*   Description  :
*   Input        : obs_options *options
*                  int storagequota_return_size
*                  char *storagequota_return
*                  obs_response_handler *handler
*                  void *callback_data
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2018/5/31
*           Author       : clw
*           Modification : Created function
*
*****************************************************************************/
void get_bucket_quota(const obs_options *options, uint64_t *storagequota_return,
    obs_response_handler *handler, void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);

    COMMLOG(OBS_LOGINFO, "get bucket quota start!");

    get_bucket_common_data *bucket_quota_data = (get_bucket_common_data *)malloc(sizeof(get_bucket_common_data));
    if (!bucket_quota_data)
    {
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        COMMLOG(OBS_LOGERROR, "malloc get_bucket_quota_data failed !");
        return;
    }
    memset_s(bucket_quota_data, sizeof(get_bucket_common_data), 0, sizeof(get_bucket_common_data));

    simplexml_initialize(&(bucket_quota_data->simpleXml), &get_bucket_quotaxml_callback, bucket_quota_data);

    bucket_quota_data->responsePropertiesCallback = handler->properties_callback;
    bucket_quota_data->responseCompleteCallback = handler->complete_callback;
    bucket_quota_data->callback_data = callback_data;
    bucket_quota_data->ul_return = storagequota_return;

    string_buffer_initialize(bucket_quota_data->common_data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.httpRequestType = http_request_type_get;
    params.properties_callback = &get_bucket_quota_properties_callback;
    params.fromObsCallback = &get_bucket_quotadata_callback;
    params.complete_callback = &get_bucket_quota_complete_callback;
    params.callback_data = bucket_quota_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "quota";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "get bucket quota finish!");
}