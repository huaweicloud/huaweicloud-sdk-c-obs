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
#include "object.h"
#include "request_util.h"
#include "file_utils.h"
#include "common.h"
#include "put_doc_common.h"
#include <openssl/md5.h>
#include "securec.h"
#include "bucket.h"  // For BASE64_MD5_LEN and MD5_LEN
#include "string_buffer.h"  // For string_buffer macros

#define RETENTION_XML_DOC_MAXSIZE (2 * 1024)

typedef struct set_retention_data
{
	put_doc_data base;  /* MUST be first member */
	char doc_buf[RETENTION_XML_DOC_MAXSIZE];
} set_retention_data;

static obs_status init_retention_xml(obs_object_retention *retention, char *xml_doc,
	int xml_doc_size, int *xml_doc_len)
{
	int offset = 0;

	if (retention == NULL) {
		COMMLOG(OBS_LOGERROR, "try generate a xml from a NULL pointer of retention!");
		return OBS_STATUS_InvalidArgument;
	}

	// 验证参数
	if (retention->mode == NULL) {
		COMMLOG(OBS_LOGERROR, "retention mode is NULL!");
		return OBS_STATUS_InvalidArgument;
	}

	if (retention->retain_until_date <= 0) {
		COMMLOG(OBS_LOGERROR, "retain_until_date must be greater than 0!");
		return OBS_STATUS_InvalidArgument;
	}

	// 开始 XML 文档
	offset += snprintf_s(xml_doc + offset, xml_doc_size - offset, _TRUNCATE,
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
		"<Retention>");
	if (offset < 0 || offset >= xml_doc_size) {
		CheckAndLogNeg(offset, "snprintf_s", __FUNCTION__, __LINE__);
		return OBS_STATUS_Security_Function_Failed;
	}

	// 添加 Mode
	offset += snprintf_s(xml_doc + offset, xml_doc_size - offset, _TRUNCATE,
		"<Mode>%s</Mode>", retention->mode);
	if (offset < 0 || offset >= xml_doc_size) {
		CheckAndLogNeg(offset, "snprintf_s", __FUNCTION__, __LINE__);
		return OBS_STATUS_Security_Function_Failed;
	}

	// 添加 RetainUntilDate
	offset += snprintf_s(xml_doc + offset, xml_doc_size - offset, _TRUNCATE,
		"<RetainUntilDate>%lld</RetainUntilDate>",
		(long long int)retention->retain_until_date);
	if (offset < 0 || offset >= xml_doc_size) {
		CheckAndLogNeg(offset, "snprintf_s", __FUNCTION__, __LINE__);
		return OBS_STATUS_Security_Function_Failed;
	}

	// 结束 XML 文档
	offset += snprintf_s(xml_doc + offset, xml_doc_size - offset, _TRUNCATE,
		"</Retention>");
	if (offset < 0 || offset >= xml_doc_size) {
		CheckAndLogNeg(offset, "snprintf_s", __FUNCTION__, __LINE__);
		return OBS_STATUS_Security_Function_Failed;
	}

	*xml_doc_len = offset;
	return OBS_STATUS_OK;
}

static char* get_retention_sub_resource(obs_use_api use_api) {
	switch (use_api)
	{
	case OBS_USE_API_OBS:
		return "retention";
	case OBS_USE_API_S3:
		return "retention";
	default:
		COMMLOG(OBS_LOGERROR, "unknown obs_use_api: %d, use default: retention", use_api);
		return "retention";
	}
}

void set_object_retention(const obs_options *options, const char *key,
	obs_object_retention *retention, const char *version_id,
	obs_response_handler *handler, void *callback_data)
{
	COMMLOG(OBS_LOGINFO, "start to %s!", __FUNCTION__);

	// 参数验证
	obs_status status = check_options_and_handler_params(__FUNCTION__, options, handler, callback_data);
	if (status != OBS_STATUS_OK) {
		return;
	}

	if (!CheckAndLogNULL(key, SYMBOL_NAME_STR(key),
		SYMBOL_NAME_STR(key),
		__FUNCTION__, __LINE__) ||
		!CheckAndLogNULL(retention, SYMBOL_NAME_STR(retention),
		SYMBOL_NAME_STR(retention),
		__FUNCTION__, __LINE__))
	{
		check_before_complete(handler->complete_callback,
			OBS_STATUS_InvalidArgument, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	request_params params;
	obs_use_api use_api = OBS_USE_API_S3;
	memset_s(&params, sizeof(params), 0, sizeof(params));

	// 使用栈上缓冲区构建查询字符串
	string_buffer(queryParams, 256);
	string_buffer_initialize(queryParams);

	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, handler, callback_data)) {
		return;
	}

	obs_put_properties properties = { 0 };

	set_retention_data *data = (set_retention_data *)malloc(sizeof(set_retention_data));
	if (!CheckAndLogNULL(data, SYMBOL_NAME_STR(data),
		SYMBOL_NAME_STR(malloc), __FUNCTION__, __LINE__))
	{
		check_before_complete(handler->complete_callback,
			OBS_STATUS_OutOfMemory, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	errno_t err = memset_s(data, sizeof(set_retention_data), 0, sizeof(set_retention_data));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memset_s), __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(data);
		check_before_complete(handler->complete_callback,
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	data->base.doc = data->doc_buf;

	status = init_retention_xml(retention, data->base.doc, sizeof(data->doc_buf), &data->base.doc_len);
	if (status != OBS_STATUS_OK)
	{
		COMMLOG(OBS_LOGERROR, "init_retention_xml failed!");
		CHECK_NULL_FREE(data);
		check_before_complete(handler->complete_callback, status, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	data->base.response_complete_callback = handler->complete_callback;
	data->base.response_properties_callback = handler->properties_callback;
	data->base.callback_data = callback_data;
	data->base.doc_bytes_written = 0;

	put_doc_compute_md5(&data->base);

	err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
		sizeof(obs_bucket_context));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memcpy_s), __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(data);
		check_before_complete(handler->complete_callback,
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
		sizeof(obs_http_request_option));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memcpy_s), __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(data);
		check_before_complete(handler->complete_callback,
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	// 设置对象键
	params.key = key;

	// 设置 versionId（如果提供）- 通过 queryParams 传递
	if (version_id != NULL) {
		int fit = 1;
		string_buffer_append(queryParams, "versionId=", 10, fit);
		string_buffer_append(queryParams, version_id, strlen(version_id), fit);
		(void)fit;
	}


	params.queryParams = queryParams[0] ? queryParams : 0;
	params.put_properties = &properties;
	params.httpRequestType = http_request_type_put;
	params.properties_callback = &put_doc_properties_callback;
	params.toObsCallback = &put_doc_data_callback;
	params.toObsCallbackTotalSize = data->base.doc_len;
	params.complete_callback = &put_doc_complete_callback;
	params.callback_data = data;
	params.isCheckCA = is_check_ca(options);
	params.storageClassFormat = no_need_storage_class;
	params.subResource = get_retention_sub_resource(use_api);
	params.temp_auth = options->temp_auth;
	params.use_api = use_api;

	// 设置 Content-MD5 和 Content-Type
	properties.md5 = data->base.doc_md5;
	properties.content_type = "application/xml";

	request_perform(&params);

	COMMLOG(OBS_LOGINFO, "end %s!", __FUNCTION__);
}
