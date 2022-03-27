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

update_bucket_common_data* init_create_bucket_cbdata(const char *location_constraint, obs_use_api use_api)
{
    update_bucket_common_data *bucket_data = (update_bucket_common_data *)malloc(sizeof(update_bucket_common_data));
    if (!bucket_data)
    {
        return NULL;
    }
    memset_s(bucket_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));

    char*plocationConstraint = 0;
    if (location_constraint)
    {
        int mark = pcre_replace(location_constraint, &plocationConstraint);
        if (use_api == OBS_USE_API_S3) {
            bucket_data->docLen = snprintf_s(bucket_data->doc, sizeof(bucket_data->doc), _TRUNCATE,
                "<CreateBucketConfiguration><LocationConstraint>"
                "%s</LocationConstraint></CreateBucketConfiguration>",
                mark ? plocationConstraint : location_constraint);
        }
        else {
            bucket_data->docLen = snprintf_s(bucket_data->doc, sizeof(bucket_data->doc), _TRUNCATE,
                "<CreateBucketConfiguration><Location>"
                "%s</Location></CreateBucketConfiguration>",
                mark ? plocationConstraint : location_constraint);
        }
        CheckAndLogNeg(bucket_data->docLen, "snprintf_s", __FUNCTION__, __LINE__);
        bucket_data->docBytesWritten = 0;
        if (mark)
        {
            free(plocationConstraint);
            plocationConstraint = NULL;
        }
    }
    else
    {
        bucket_data->docLen = 0;
    }

    return bucket_data;
}

void prepare_create_bucket_data(update_bucket_common_data *bucket_data, obs_response_handler *handler, void *callback_data) {
    bucket_data->complete_callback = handler->complete_callback;
    bucket_data->callback_data = callback_data;
    bucket_data->properties_callback = handler->properties_callback;
}

void prepare_create_bucket_params(request_params *params, const obs_options *options,
    obs_put_properties *properties, update_bucket_common_data *bucket_data, obs_use_api use_api) {
    errno_t err = EOK;
    err = memcpy_s(&(params->bucketContext), sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&(params->request_option), sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params->put_properties = properties;
    params->httpRequestType = http_request_type_put;
    params->properties_callback = &update_bucket_common_properties_callback;
    params->toObsCallback = &update_bucket_common_data_callback;
    params->toObsCallbackTotalSize = bucket_data->docLen;
    params->complete_callback = &update_bucket_common_complete_callback;
    params->callback_data = bucket_data;
    params->isCheckCA = options->bucket_options.certificate_info ? 1 : 0;
    params->storageClassFormat = default_storage_class;
    params->temp_auth = options->temp_auth;
    params->use_api = use_api;
}


void create_bucket(const obs_options *options, obs_canned_acl canned_acl,
    const char *location_constraint, obs_response_handler *handler,
    void *callback_data)
{
    request_params      params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    obs_put_properties  properties;
    update_bucket_common_data    *bucket_data = NULL;

    COMMLOG(OBS_LOGINFO, "create bucket start!");

    bucket_data = init_create_bucket_cbdata(location_constraint, use_api);
    if (!bucket_data)
    {
        COMMLOG(OBS_LOGERROR, "Malloc update_bucket_common_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    prepare_create_bucket_data(bucket_data, handler, callback_data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = canned_acl;

    prepare_create_bucket_params(&params, options, &properties, bucket_data, use_api);
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "create bucket finish!");
}

void create_bucket_with_params(const obs_options *options, const obs_create_bucket_params *param,
    obs_response_handler *handler, void *callback_data)
{
    request_params      params;
    obs_use_api use_api = OBS_USE_API_S3;
    //if create bucket with 3az,must be use obs protocol
    if (OBS_REDUNDANCY_3AZ == param->az_redundancy)
    {
        use_api = OBS_USE_API_OBS;
    }
    else
    {
        set_use_api_switch(options, &use_api);
    }
    obs_put_properties  properties;
    update_bucket_common_data    *bucket_data = NULL;

    COMMLOG(OBS_LOGINFO, "create bucket start!");

    bucket_data = init_create_bucket_cbdata(param->location_constraint, use_api);
    if (!bucket_data)
    {
        COMMLOG(OBS_LOGERROR, "Malloc update_bucket_common_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, 0);
        return;
    }
    prepare_create_bucket_data(bucket_data, handler, callback_data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = param->canned_acl;
    properties.az_redundancy = param->az_redundancy;

    prepare_create_bucket_params(&params, options, &properties, bucket_data, use_api);
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "create bucket finish!");
}



// create pfs bucket.
void create_pfs_bucket(const obs_options *options, obs_canned_acl canned_acl,
    const char *location_constraint, obs_response_handler *handler,
    void *callback_data)
{
    request_params      params;
    obs_use_api use_api = OBS_USE_API_S3;
    set_use_api_switch(options, &use_api);
    obs_put_properties  properties;
    update_bucket_common_data    *bucket_data = NULL;

    COMMLOG(OBS_LOGINFO, "create pfs bucket start!");
    bucket_data = init_create_bucket_cbdata(location_constraint, use_api);
    if (!bucket_data)
    {
        COMMLOG(OBS_LOGERROR, "Malloc update_bucket_common_data failed!");
        (void)(*(handler->complete_callback))(OBS_STATUS_OutOfMemory, 0, callback_data);
        return;
    }
    prepare_create_bucket_data(bucket_data, handler, callback_data);

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = canned_acl;

    prepare_create_bucket_params(&params, options, &properties, bucket_data, use_api);
    params.bucketContext.bucket_type = OBS_BUCKET_PFS;

    request_perform(&params);
    COMMLOG(OBS_LOGINFO, "create pfs bucket finish!");

    return;
}
