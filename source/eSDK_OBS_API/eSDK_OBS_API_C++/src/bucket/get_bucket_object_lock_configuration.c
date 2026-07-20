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
#include "simplexml.h"
#include "securec.h"

#define OBJECT_LOCK_RESPONSE_DOC_MAXSIZE (8 * 1024)

typedef struct get_object_lock_data
{
	simple_xml simpleXml;

	obs_response_properties_callback *response_properties_callback;
	obs_response_complete_callback *response_complete_callback;
	obs_get_bucket_object_lock_callback *get_object_lock_callback;
	void *callback_data;

	string_buffer(object_lock_enabled, 64);
	string_buffer(mode, 64);
	string_buffer(days, 32);
	string_buffer(years, 32);
	int in_default_retention;
	int in_rule;

	obs_bucket_object_lock_configuration *object_lock_config_return;
} get_object_lock_data;

static obs_status get_bucket_object_lock_xml_callback(const char *element_path,
	const char *data, int data_len, void *callback_data)
{
	get_object_lock_data *pObjectLockData = (get_object_lock_data *)callback_data;
	int fit = 1;

	if (!strcmp(element_path, "ObjectLockConfiguration/Rule"))
	{
		pObjectLockData->in_rule = 1;
	}

	if (data)
	{
		// 解析 ObjectLockEnabled
		if (!strcmp(element_path, "ObjectLockConfiguration/ObjectLockEnabled"))
		{
			string_buffer_append(pObjectLockData->object_lock_enabled, data, data_len, fit);
		}
		// 解析 Mode（在 DefaultRetention 内）
		else if (!strcmp(element_path, "ObjectLockConfiguration/Rule/DefaultRetention/Mode"))
		{
			string_buffer_append(pObjectLockData->mode, data, data_len, fit);
			pObjectLockData->in_default_retention = 1;
		}
		// 解析 Days（在 DefaultRetention 内）
		else if (!strcmp(element_path, "ObjectLockConfiguration/Rule/DefaultRetention/Days"))
		{
			string_buffer_append(pObjectLockData->days, data, data_len, fit);
			pObjectLockData->in_default_retention = 1;
		}
		// 解析 Years（在 DefaultRetention 内）
		else if (!strcmp(element_path, "ObjectLockConfiguration/Rule/DefaultRetention/Years"))
		{
			string_buffer_append(pObjectLockData->years, data, data_len, fit);
			pObjectLockData->in_default_retention = 1;
		}
	}

	(void)fit;
	return OBS_STATUS_OK;
}

static obs_status get_object_lock_data_callback(int buffer_size, const char *buffer, void *callback_data)
{
	get_object_lock_data *data = (get_object_lock_data *)callback_data;
	return simplexml_add(&(data->simpleXml), buffer, buffer_size);
}

static obs_status get_object_lock_properties_callback(const obs_response_properties *response_properties,
	void *callback_data)
{
	get_object_lock_data *data = (get_object_lock_data *)callback_data;
	if (data->response_properties_callback)
	{
		return (*(data->response_properties_callback))(response_properties, data->callback_data);
	}
	return OBS_STATUS_OK;
}

static obs_status build_retention(get_object_lock_data *data,
	obs_bucket_object_lock_configuration *config)
{
	obs_worm_default_retention *retention = NULL;

	if (!data->in_rule || (data->mode[0] == '\0' &&
		data->days[0] == '\0' && data->years[0] == '\0')) {
		return OBS_STATUS_OK;
	}

	retention = (obs_worm_default_retention *)malloc(sizeof(obs_worm_default_retention));
	if (retention == NULL) {
		COMMLOG(OBS_LOGERROR, "malloc retention failed!");
		return OBS_STATUS_OutOfMemory;
	}
	memset_s(retention, sizeof(obs_worm_default_retention),
		0, sizeof(obs_worm_default_retention));

	if (data->mode[0] != '\0') {
		retention->mode = strdup(data->mode);
		if (retention->mode == NULL) {
			COMMLOG(OBS_LOGERROR, "strdup mode failed!");
			free(retention);
			return OBS_STATUS_OutOfMemory;
		}
	}

	if (data->days[0] != '\0') {
		retention->days = (unsigned int)atoi(data->days);
	}
	if (data->years[0] != '\0') {
		retention->years = (unsigned int)atoi(data->years);
	}

	config->default_retention = retention;
	return OBS_STATUS_OK;
}

/**
 * @brief 从解析数据构建obs_bucket_object_lock_configuration结构体
 * @return OBS_STATUS_OK 表示成功, 其他表示失败
 */
static obs_status build_object_lock_config(get_object_lock_data *data,
	obs_bucket_object_lock_configuration **config_out)
{
	obs_status status;
	obs_bucket_object_lock_configuration *config = (obs_bucket_object_lock_configuration *)
		malloc(sizeof(obs_bucket_object_lock_configuration));
	if (config == NULL) {
		COMMLOG(OBS_LOGERROR, "malloc object_lock_config_return failed!");
		return OBS_STATUS_OutOfMemory;
	}
	memset_s(config, sizeof(obs_bucket_object_lock_configuration),
		0, sizeof(obs_bucket_object_lock_configuration));

	// 设置 ObjectLockEnabled
	if (data->object_lock_enabled[0] != '\0') {
		config->object_lock_enabled = strdup(data->object_lock_enabled);
		if (config->object_lock_enabled == NULL) {
			COMMLOG(OBS_LOGERROR, "strdup object_lock_enabled failed!");
			free(config);
			return OBS_STATUS_OutOfMemory;
		}
	}

	// 设置 DefaultRetention
	status = build_retention(data, config);
	if (status != OBS_STATUS_OK) {
		if (config->object_lock_enabled) {
			free(config->object_lock_enabled);
		}
		free(config);
		return status;
	}

	*config_out = config;
	return OBS_STATUS_OK;
}

/**
 * @brief 释放obs_bucket_object_lock_configuration结构体
 */
static void free_object_lock_config(obs_bucket_object_lock_configuration *config)
{
	if (config == NULL) {
		return;
	}
	CHECK_NULL_FREE(config->object_lock_enabled);
	if (config->default_retention != NULL) {
		CHECK_NULL_FREE(config->default_retention->mode);
		CHECK_NULL_FREE(config->default_retention);
	}
	CHECK_NULL_FREE(config);
}

static void get_object_lock_complete_callback(obs_status request_status,
	const obs_error_details *obs_error_info,
	void *callback_data)
{
	COMMLOG(OBS_LOGINFO, "Enter %s successfully!", __FUNCTION__);

	get_object_lock_data *data = (get_object_lock_data *)callback_data;
	obs_bucket_object_lock_configuration *object_lock_config_return = NULL;
	obs_status status = OBS_STATUS_OK;

	if (request_status == OBS_STATUS_OK && data->get_object_lock_callback) {
		status = build_object_lock_config(data, &object_lock_config_return);
		if (status == OBS_STATUS_OK) {
			data->object_lock_config_return = object_lock_config_return;
		}
	}

	if (data->get_object_lock_callback) {
		(*(data->get_object_lock_callback))(object_lock_config_return, data->callback_data);
	}

	// 释放临时内存
	free_object_lock_config(data->object_lock_config_return);

	if (data->response_complete_callback) {
		(*(data->response_complete_callback))(
			status != OBS_STATUS_OK ? status : request_status,
			obs_error_info, data->callback_data);
	}

	simplexml_deinitialize(&(data->simpleXml));

	free(data);
	data = NULL;

	COMMLOG(OBS_LOGINFO, "Leave %s successfully!", __FUNCTION__);
}

void get_bucket_object_lock_configuration(const obs_options *options,
	obs_get_bucket_object_lock_handler *handler, void *callback_data)
{
	COMMLOG(OBS_LOGINFO, "start to %s!", __FUNCTION__);

	obs_status status = check_options_and_handler_params(__FUNCTION__, options,
		&handler->response_handler, callback_data);
	if (status != OBS_STATUS_OK) {
		return;
	}

	request_params params;
	obs_use_api use_api = OBS_USE_API_S3;

	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api,
		&handler->response_handler, callback_data)) {
		return;
	}

	get_object_lock_data *data = (get_object_lock_data *)malloc(sizeof(get_object_lock_data));
	if (!CheckAndLogNULL(data, SYMBOL_NAME_STR(data),
		SYMBOL_NAME_STR(malloc), __FUNCTION__, __LINE__))
	{
		check_before_complete(handler->response_handler.complete_callback,
			OBS_STATUS_OutOfMemory, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	errno_t err = memset_s(data, sizeof(get_object_lock_data), 0, sizeof(get_object_lock_data));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memset_s), __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(data);
		check_before_complete(handler->response_handler.complete_callback,
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	data->response_complete_callback = handler->response_handler.complete_callback;
	data->response_properties_callback = handler->response_handler.properties_callback;
	data->get_object_lock_callback = handler->get_bucket_object_lock_callback;
	data->callback_data = callback_data;

	// 初始化 SimpleXML
	simplexml_initialize(&(data->simpleXml), &get_bucket_object_lock_xml_callback, data);

	err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
		sizeof(obs_bucket_context));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memcpy_s), __FUNCTION__, __LINE__, err)) {
		simplexml_deinitialize(&(data->simpleXml));
		CHECK_NULL_FREE(data);
		check_before_complete(handler->response_handler.complete_callback,
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
		sizeof(obs_http_request_option));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memcpy_s), __FUNCTION__, __LINE__, err)) {
		simplexml_deinitialize(&(data->simpleXml));
		CHECK_NULL_FREE(data);
		check_before_complete(handler->response_handler.complete_callback,
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	params.httpRequestType = http_request_type_get;
	params.properties_callback = &get_object_lock_properties_callback;
	params.fromObsCallback = &get_object_lock_data_callback;
	params.complete_callback = &get_object_lock_complete_callback;
	params.callback_data = data;
	params.isCheckCA = is_check_ca(options);
	params.storageClassFormat = no_need_storage_class;
	params.subResource = get_bucket_object_lock_sub_resource(use_api);
	params.temp_auth = options->temp_auth;
	params.use_api = use_api;

	request_perform(&params);

	COMMLOG(OBS_LOGINFO, "end %s!", __FUNCTION__);
}

obs_status parse_bucket_object_lock_xml(const char *xml_doc, int xml_doc_len,
	obs_bucket_object_lock_configuration *object_lock_config_return)
{
	// 此函数可用于单元测试，简化版实现
	(void)xml_doc;
	(void)xml_doc_len;
	(void)object_lock_config_return;
	return OBS_STATUS_OK;
}
