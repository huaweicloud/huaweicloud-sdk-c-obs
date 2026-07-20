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
#include "put_doc_common.h"
#include "request_util.h"
#include "util.h"
#include <openssl/md5.h>

int put_doc_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    put_doc_data *data = (put_doc_data *)callback_data;

    if (!data->doc_len) {
        return 0;
    }

    int remaining = (data->doc_len - data->doc_bytes_written);
    int toCopy = buffer_size > remaining ? remaining : buffer_size;

    if (!toCopy) {
        return 0;
    }

    errno_t err = EOK;
    err = memcpy_s(buffer, buffer_size, &(data->doc[data->doc_bytes_written]), toCopy);
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    data->doc_bytes_written += toCopy;

    return toCopy;
}

obs_status put_doc_properties_callback(const obs_response_properties *response_properties,
    void *callback_data)
{
    put_doc_data *data = (put_doc_data *)callback_data;
    if (data->response_properties_callback)
    {
        return (*(data->response_properties_callback))
            (response_properties, data->callback_data);
    }
    return OBS_STATUS_OK;
}

void put_doc_complete_callback(obs_status request_status,
    const obs_error_details *obs_error_info, void *callback_data)
{
    COMMLOG(OBS_LOGINFO, "Enter %s successfully !", __FUNCTION__);

    put_doc_data *data = (put_doc_data *)callback_data;

    (void)(*(data->response_complete_callback))(request_status, obs_error_info,
        data->callback_data);

    free(data);
    data = NULL;
    COMMLOG(OBS_LOGINFO, "Leave %s successfully !", __FUNCTION__);
}

void put_doc_compute_md5(put_doc_data *data)
{
    unsigned char doc_md5[MD5_DIGEST_LENGTH];
    /* MD5 is required by OBS API protocol for Content-MD5 header, not used for security purposes */
    MD5((unsigned char *)data->doc, (size_t)data->doc_len, doc_md5);
    base64Encode(doc_md5, sizeof(doc_md5), data->doc_md5);
}
