#include "request_util.h"
#include "file_utils.h"
#include "bucket.h"
#include "bucket_trash_configuration.h"

static obs_status get_bucket_trash_configuration_xml_callback(const char *element_path, const char *data,
	int data_len, void *callback_data)
{
	get_bucket_trash_config_data *bucket_trash_configuration = (get_bucket_trash_config_data *)callback_data;
	int fit = 1;
	if (data && !strcmp(element_path, "BucketTrashConfiguration/ReservedDays")) {
		string_buffer_append(bucket_trash_configuration->common_data, data, data_len, fit);
	}

	//(void) fit;
	if (!fit) {
		COMMLOG(OBS_LOGDEBUG, "%s: fit is 0.", __FUNCTION__);
	}
	return OBS_STATUS_OK;
}

static obs_status get_trash_configuration_properties_callback(const obs_response_properties *response_properties,
	void *callback_data)
{
	get_bucket_trash_config_data *bucket_bucket_trash_configuration = (get_bucket_trash_config_data *)callback_data;
	if (bucket_bucket_trash_configuration->responsePropertiesCallback)
	{
		return (*(bucket_bucket_trash_configuration->responsePropertiesCallback))
			(response_properties, bucket_bucket_trash_configuration->callback_data);
	}
	return OBS_STATUS_OK;
}

static obs_status get_bucket_trash_configuration_callback(int buffer_size, const char *buffer,
	void *callback_data)
{
	get_bucket_trash_config_data *gbqData = (get_bucket_trash_config_data *)callback_data;
	return simplexml_add(&(gbqData->simpleXml), buffer, buffer_size);
}

static void get_bucket_trash_configuration_complete_callback(obs_status status,
	const obs_error_details *error_details,
	void *callback_data)
{
	COMMLOG(OBS_LOGDEBUG, "Enter %s", __FUNCTION__);

	get_bucket_trash_config_data *bucket_trash_configuration_data = (get_bucket_trash_config_data *)callback_data;
	if (OBS_STATUS_OK == status)
	{
		COMMLOG(OBS_LOGINFO, "bucket_trash_configuration string is %s", bucket_trash_configuration_data->common_data);
		(*(bucket_trash_configuration_data->bucket_trash_configuration_return)).reserved_days =
			atol(bucket_trash_configuration_data->common_data);
	}

	(void)(*(bucket_trash_configuration_data->responseCompleteCallback))(status, error_details,
		bucket_trash_configuration_data->callback_data);

	COMMLOG(OBS_LOGDEBUG, "Leave %s", __FUNCTION__);
}

void get_bucket_trash_configuration(const obs_options *options, bucket_trash_configuration* obs_trash_configuration_return,
    obs_response_handler *handler, void *callback_data)
{
	COMMLOG(OBS_LOGINFO, "start to %s!", __FUNCTION__);
	if (OBS_STATUS_OK != check_bucket_trash_config_params(__FUNCTION__, options,
		obs_trash_configuration_return, handler, callback_data)) {
		return;
	}
	request_params params;
	obs_use_api use_api = OBS_USE_API_S3;
	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, handler, callback_data)) {
		return;
	}

	get_bucket_trash_config_data *bucket_trash_configuration_data = 
		(get_bucket_trash_config_data *)malloc(sizeof(get_bucket_trash_config_data));

	if (!CheckAndLogNULL(bucket_trash_configuration_data, SYMBOL_NAME_STR(bucket_trash_configuration_data),
		SYMBOL_NAME_STR(malloc), __FUNCTION__, __LINE__))
	{
		check_before_complete(handler->complete_callback,
			OBS_STATUS_OutOfMemory, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}
	errno_t err = memset_s(bucket_trash_configuration_data, 
		sizeof(get_bucket_trash_config_data), 0, sizeof(get_bucket_trash_config_data));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memset_s), __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(bucket_trash_configuration_data);
		check_before_complete(handler->complete_callback,
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

	simplexml_initialize(&(bucket_trash_configuration_data->simpleXml), 
		&get_bucket_trash_configuration_xml_callback, bucket_trash_configuration_data);

	bucket_trash_configuration_data->responsePropertiesCallback = handler->properties_callback;
	bucket_trash_configuration_data->responseCompleteCallback = handler->complete_callback;
	bucket_trash_configuration_data->callback_data = callback_data;
	bucket_trash_configuration_data->bucket_trash_configuration_return = obs_trash_configuration_return;

	string_buffer_initialize(bucket_trash_configuration_data->common_data);

	params.httpRequestType = http_request_type_get;
	params.properties_callback = &get_trash_configuration_properties_callback;
	params.fromObsCallback = &get_bucket_trash_configuration_callback;
	params.complete_callback = &get_bucket_trash_configuration_complete_callback;
	params.callback_data = bucket_trash_configuration_data;
	params.isCheckCA = is_check_ca(options);
	params.storageClassFormat = no_need_storage_class;
	params.subResource = get_bucket_trash_configuration_sub_resource(use_api);
	params.temp_auth = options->temp_auth;
	params.use_api = use_api;
	request_perform(&params);
	simplexml_deinitialize(&(bucket_trash_configuration_data->simpleXml));
	CHECK_NULL_FREE(bucket_trash_configuration_data);
	COMMLOG(OBS_LOGINFO, "end %s!", __FUNCTION__);
}