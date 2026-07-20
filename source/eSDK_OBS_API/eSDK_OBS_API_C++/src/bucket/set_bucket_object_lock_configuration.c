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
#include "request_util.h"
#include "file_utils.h"
#include "bucket.h"
#include "bucket_object_lock_configuration.h"
#include "common.h"
#include "put_doc_common.h"
#include <openssl/md5.h>
#include "securec.h"

#define OBJECT_LOCK_XML_DOC_MAXSIZE (8 * 1024)

typedef struct set_object_lock_data
{
	put_doc_data base;  /* MUST be first member */
	char doc_buf[OBJECT_LOCK_XML_DOC_MAXSIZE];
} set_object_lock_data;

/**
 * Helper that performs snprintf_s into the XML buffer and checks for overflow.
 * On success, updates *offset with the new position.
 * Returns OBS_STATUS_OK on success, OBS_STATUS_Security_Function_Failed on overflow.
 */
static obs_status append_xml_snippet(char *xml_doc, int xml_doc_size,
	int *offset, const char *format, ...)
{
	va_list args;
	int written;

	va_start(args, format);
	written = vsnprintf_s(xml_doc + *offset, xml_doc_size - *offset,
		_TRUNCATE, format, args);
	va_end(args);

	if (written < 0 || *offset + written >= xml_doc_size) {
		CheckAndLogNeg(*offset + written, "snprintf_s", __FUNCTION__, __LINE__);
		return OBS_STATUS_Security_Function_Failed;
	}
	*offset += written;
	return OBS_STATUS_OK;
}

/**
 * Validate the retention parameters: mode must be non-NULL,
 * days and years must be specified exclusively one at a time,
 * and values must be in valid ranges.
 */
static obs_status validate_retention_params(const obs_worm_default_retention *retention)
{
	if (retention->mode == NULL) {
		COMMLOG(OBS_LOGERROR, "retention mode is NULL!");
		return OBS_STATUS_InvalidArgument;
	}

	// 验证 days 和 years 二选一
	if ((retention->days > 0 && retention->years > 0) ||
		(retention->days == 0 && retention->years == 0)) {
		COMMLOG(OBS_LOGERROR, "days and years must be specified exclusively one!");
		return OBS_STATUS_InvalidArgument;
	}

	// 验证取值范围
	if (retention->days > 0 && (retention->days < 1 || retention->days > 36500)) {
		COMMLOG(OBS_LOGERROR, "days must be between 1 and 36500!");
		return OBS_STATUS_InvalidArgument;
	}

	if (retention->years > 0 && (retention->years < 1 || retention->years > 100)) {
		COMMLOG(OBS_LOGERROR, "years must be between 1 and 100!");
		return OBS_STATUS_InvalidArgument;
	}

	return OBS_STATUS_OK;
}

/**
 * Append the DefaultRetention XML section:
 *   <Rule><DefaultRetention><Mode>...</Mode><Days>...</Days></DefaultRetention></Rule>
 * or <Rule><DefaultRetention><Mode>...</Mode><Years>...</Years></DefaultRetention></Rule>
 */
static obs_status append_retention_xml(char *xml_doc, int xml_doc_size,
	int *offset, const obs_worm_default_retention *retention)
{
	obs_status status;

	status = append_xml_snippet(xml_doc, xml_doc_size, offset,
		"<Rule><DefaultRetention>");
	if (status != OBS_STATUS_OK) {
		return status;
	}

	// 添加 Mode
	status = append_xml_snippet(xml_doc, xml_doc_size, offset,
		"<Mode>%s</Mode>", retention->mode);
	if (status != OBS_STATUS_OK) {
		return status;
	}

	// 添加 Days 或 Years
	if (retention->days > 0) {
		status = append_xml_snippet(xml_doc, xml_doc_size, offset,
			"<Days>%u</Days>", retention->days);
	} else {
		status = append_xml_snippet(xml_doc, xml_doc_size, offset,
			"<Years>%u</Years>", retention->years);
	}
	if (status != OBS_STATUS_OK) {
		return status;
	}

	status = append_xml_snippet(xml_doc, xml_doc_size, offset,
		"</DefaultRetention></Rule>");
	return status;
}

static obs_status init_bucket_object_lock_xml_internal(obs_bucket_object_lock_configuration *object_lock_config,
	char *xml_doc, int xml_doc_size, int *xml_doc_len)
{
	int offset = 0;
	obs_status status;

	if (object_lock_config == NULL) {
		COMMLOG(OBS_LOGERROR, "try generate a xml from a NULL pointer of object_lock_config!");
		return OBS_STATUS_InvalidArgument;
	}

	// 开始 XML 文档
	status = append_xml_snippet(xml_doc, xml_doc_size, &offset,
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
		"<ObjectLockConfiguration xmlns=\"http://obs.cn-north-4.myhuaweicloud.com/doc/2015-06-30/\">");
	if (status != OBS_STATUS_OK) {
		return status;
	}

	// 添加 ObjectLockEnabled 元素
	if (object_lock_config->object_lock_enabled != NULL) {
		status = append_xml_snippet(xml_doc, xml_doc_size, &offset,
			"<ObjectLockEnabled>%s</ObjectLockEnabled>",
			object_lock_config->object_lock_enabled);
		if (status != OBS_STATUS_OK) {
			return status;
		}
	}

	// 添加 Rule 和 DefaultRetention 元素（如果存在）
	if (object_lock_config->default_retention != NULL) {
		status = validate_retention_params(object_lock_config->default_retention);
		if (status != OBS_STATUS_OK) {
			return status;
		}

		status = append_retention_xml(xml_doc, xml_doc_size, &offset,
			object_lock_config->default_retention);
		if (status != OBS_STATUS_OK) {
			return status;
		}
	}

	// 结束 XML 文档
	status = append_xml_snippet(xml_doc, xml_doc_size, &offset,
		"</ObjectLockConfiguration>");
	if (status != OBS_STATUS_OK) {
		return status;
	}

	*xml_doc_len = offset;
	return OBS_STATUS_OK;
}

obs_status init_bucket_object_lock_xml(obs_bucket_object_lock_configuration *object_lock_config,
	char *xml_doc, int xml_doc_size, int *xml_doc_len)
{
	return init_bucket_object_lock_xml_internal(object_lock_config, xml_doc, xml_doc_size, xml_doc_len);
}

char* get_bucket_object_lock_sub_resource(obs_use_api use_api) {
	switch (use_api)
	{
	case OBS_USE_API_OBS:
		return "object-lock";
	case OBS_USE_API_S3:
		return "object-lock";
	default:
		COMMLOG(OBS_LOGERROR, "unknown obs_use_api: %d, use default: object-lock", use_api);
		return "object-lock";
	}
}

obs_status check_bucket_object_lock_config_params(const char* function, const obs_options *options,
	obs_bucket_object_lock_configuration *object_lock_config,
	obs_response_handler *handler, void *callback_data)
{
	obs_status ret = check_options_and_handler_params(function, options, handler, callback_data);

	if (ret != OBS_STATUS_OK) {
		return ret;
	}

	if (!CheckAndLogNULL(object_lock_config, SYMBOL_NAME_STR(object_lock_config),
		__FUNCTION__, function, __LINE__))
	{
		check_before_complete(handler->complete_callback,
			OBS_STATUS_InvalidArgument, 0, callback_data, __FUNCTION__, __LINE__);
		return OBS_STATUS_InvalidArgument;
	}

	return OBS_STATUS_OK;
}

void set_bucket_object_lock_configuration(const obs_options *options,
	obs_bucket_object_lock_configuration *object_lock_config,
	obs_response_handler *handler, void *callback_data)
{
	COMMLOG(OBS_LOGINFO, "start to %s!", __FUNCTION__);

	if (OBS_STATUS_OK != check_bucket_object_lock_config_params(__FUNCTION__, options,
		object_lock_config, handler, callback_data)) {
		return;
	}

	request_params params;
	obs_use_api use_api = OBS_USE_API_S3;

	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, handler, callback_data)) {
		return;
	}

	obs_put_properties properties = { 0 };

	set_object_lock_data *data = (set_object_lock_data *)malloc(sizeof(set_object_lock_data));
	if (!CheckAndLogNULL(data, SYMBOL_NAME_STR(data),
		SYMBOL_NAME_STR(malloc), __FUNCTION__, __LINE__))
	{
		check_before_complete(handler->complete_callback,
			OBS_STATUS_OutOfMemory, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	errno_t err = memset_s(data, sizeof(set_object_lock_data), 0, sizeof(set_object_lock_data));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memset_s), __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(data);
		check_before_complete(handler->complete_callback,
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	data->base.doc = data->doc_buf;

	obs_status status = init_bucket_object_lock_xml_internal(object_lock_config, data->base.doc,
		sizeof(data->doc_buf), &data->base.doc_len);
	if (status != OBS_STATUS_OK)
	{
		COMMLOG(OBS_LOGERROR, "init_bucket_object_lock_xml failed!");
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

	params.put_properties = &properties;
	params.httpRequestType = http_request_type_put;
	params.properties_callback = &put_doc_properties_callback;
	params.toObsCallback = &put_doc_data_callback;
	params.toObsCallbackTotalSize = data->base.doc_len;
	params.complete_callback = &put_doc_complete_callback;
	params.callback_data = data;
	params.isCheckCA = is_check_ca(options);
	params.storageClassFormat = no_need_storage_class;
	params.subResource = get_bucket_object_lock_sub_resource(use_api);
	params.temp_auth = options->temp_auth;
	params.use_api = use_api;

	// 设置 Content-MD5 和 Content-Type
	properties.md5 = data->base.doc_md5;
	properties.content_type = "application/xml";

	request_perform(&params);

	COMMLOG(OBS_LOGINFO, "end %s!", __FUNCTION__);
}
