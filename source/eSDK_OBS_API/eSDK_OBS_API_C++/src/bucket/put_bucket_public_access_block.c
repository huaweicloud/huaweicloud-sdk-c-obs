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
**********************************************************************************/
#include "bucket.h"
#include "request_util.h"

static obs_status append_xml_bool_element(char *buf, int *offset, int *remaining,
    const char *tag, int value)
{
    int len = snprintf_s(buf + *offset, *remaining, _TRUNCATE,
        "<%s>%s</%s>", tag, value ? "true" : "false", tag);
    if (len < 0)
    {
        return OBS_STATUS_InternalError;
    }
    *offset += len;
    *remaining -= len;
    return OBS_STATUS_OK;
}

static obs_status append_public_access_block_elements(char *buf, int *offset, int *remaining,
    const obs_bucket_public_access_block *public_access_block)
{
    obs_status status;

    status = append_xml_bool_element(buf, offset, remaining,
        "BlockPublicAcls", public_access_block->block_public_acls);
    if (status != OBS_STATUS_OK) return status;

    status = append_xml_bool_element(buf, offset, remaining,
        "IgnorePublicAcls", public_access_block->ignore_public_acls);
    if (status != OBS_STATUS_OK) return status;

    status = append_xml_bool_element(buf, offset, remaining,
        "BlockPublicPolicy", public_access_block->block_public_policy);
    if (status != OBS_STATUS_OK) return status;

    status = append_xml_bool_element(buf, offset, remaining,
        "RestrictPublicBuckets", public_access_block->restrict_public_buckets);
    return status;
}

static obs_status init_put_bucket_public_access_block_cbdata(
    const obs_bucket_public_access_block *public_access_block,
    update_bucket_common_data **data)
{
    obs_status status;
    update_bucket_common_data *bpa_data =
        (update_bucket_common_data *)malloc(sizeof(update_bucket_common_data));
    if (!bpa_data)
    {
        *data = NULL;
        return OBS_STATUS_OutOfMemory;
    }
    memset_s(bpa_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));

    int offset = 0;
    int remaining = sizeof(bpa_data->doc) - 1;

    // Start XML document
    int len = snprintf_s(bpa_data->doc + offset, remaining, _TRUNCATE,
        "<PublicAccessBlockConfiguration>");
    if (len < 0)
    {
        *data = NULL;
        CHECK_NULL_FREE(bpa_data);
        return OBS_STATUS_InternalError;
    }
    offset += len;
    remaining -= len;

    // Add boolean elements
    if (public_access_block)
    {
        status = append_public_access_block_elements(bpa_data->doc, &offset, &remaining,
            public_access_block);
        if (status != OBS_STATUS_OK)
        {
            *data = NULL;
            CHECK_NULL_FREE(bpa_data);
            return status;
        }
    }

    // End XML document
    len = snprintf_s(bpa_data->doc + offset, remaining, _TRUNCATE,
        "</PublicAccessBlockConfiguration>");
    if (len < 0)
    {
        *data = NULL;
        CHECK_NULL_FREE(bpa_data);
        return OBS_STATUS_InternalError;
    }
    offset += len;

    bpa_data->docLen = offset;
    bpa_data->docBytesWritten = 0;
    *data = bpa_data;

    return OBS_STATUS_OK;
}

void put_bucket_public_access_block(
    const obs_options *options,
    const obs_bucket_public_access_block *public_access_block,
    obs_response_handler *handler,
    void *callback_data)
{
    request_params params;
    obs_use_api use_api = OBS_USE_API_S3;
    // NULL check will happen before this
    // Validate parameters
    if (!options || !handler) {
        COMMLOG(OBS_LOGERROR, "Invalid parameters: options or handler is NULL");
        return;
    }
    obs_put_properties  properties;
    update_bucket_common_data  *bpa_data = NULL;
    obs_status status = OBS_STATUS_OK;

    COMMLOG(OBS_LOGINFO, "put bucket public access block start!");

    // Validate parameters

    status = init_put_bucket_public_access_block_cbdata(public_access_block, &bpa_data);
    if (status != OBS_STATUS_OK)
    {
        COMMLOG(OBS_LOGERROR, "Malloc update_bucket_common_data failed!");
        if (handler && handler->complete_callback) {
            (void)(*(handler->complete_callback))(status, 0, callback_data);
        }
        return;
    }
    bpa_data->complete_callback = handler->complete_callback;
    bpa_data->callback_data = callback_data;
    bpa_data->properties_callback = handler->properties_callback;

    memset_s(&params, sizeof(request_params), 0, sizeof(request_params));
    memset_s(&properties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));
    properties.canned_acl = OBS_CANNED_ACL_PRIVATE;

    errno_t err = EOK;
    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    params.put_properties = &properties;
    params.httpRequestType = http_request_type_put;
    params.properties_callback = &update_bucket_common_properties_callback;
    params.toObsCallback = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = bpa_data->docLen;
    params.complete_callback = &update_bucket_common_complete_callback;
    params.callback_data = bpa_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = "publicAccessBlock";
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);

    COMMLOG(OBS_LOGINFO, "put bucket public access block finish!");
}
