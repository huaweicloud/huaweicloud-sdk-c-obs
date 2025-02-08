#include "request_util.h"
#include "file_utils.h"
#include "bucket.h"
#include "bucket_trash_configuration.h"
#include "common.h"
obs_status init_bucket_trash_configuration_xml(bucket_trash_configuration * obs_trash_configuration,
	update_bucket_common_data *bucket_common_data);
void set_bucket_trash_configuration(const obs_options *options, bucket_trash_configuration* obs_trash_configuration,
    obs_response_handler *handler, void *callback_data)
{
	COMMLOG(OBS_LOGINFO, "start to %s!", __FUNCTION__);
	if (OBS_STATUS_OK != check_bucket_trash_config_params(__FUNCTION__, options, 
		obs_trash_configuration, handler, callback_data)) {
		return;
	}
    request_params params;
	obs_use_api use_api = OBS_USE_API_S3;
	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, handler, callback_data)) {
		return;
	}
    obs_put_properties properties = { 0 };
    update_bucket_common_data *trash_configuration_data = 
		(update_bucket_common_data *)malloc(sizeof(update_bucket_common_data));
    if (!CheckAndLogNULL(trash_configuration_data, SYMBOL_NAME_STR(trash_configuration_data), 
		SYMBOL_NAME_STR(malloc), __FUNCTION__, __LINE__))
    {
		check_before_complete(handler->complete_callback, 
			OBS_STATUS_OutOfMemory, 0, callback_data, __FUNCTION__, __LINE__);
        return;
    }
    errno_t err = memset_s(trash_configuration_data, sizeof(update_bucket_common_data), 0, sizeof(update_bucket_common_data));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memset_s), __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(trash_configuration_data);
		check_before_complete(handler->complete_callback, 
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}
    obs_status status = OBS_STATUS_BUTT;
    status = init_bucket_trash_configuration_xml(obs_trash_configuration, trash_configuration_data);
    if (status != OBS_STATUS_OK)
    {
        COMMLOG(OBS_LOGERROR, "init_set_bucket_trash_configuration failed!");
		CHECK_NULL_FREE(trash_configuration_data);
		check_before_complete(handler->complete_callback, status, 0, callback_data, __FUNCTION__, __LINE__);
        return;
    }
    trash_configuration_data->complete_callback = handler->complete_callback;
    trash_configuration_data->callback_data = callback_data;
    trash_configuration_data->properties_callback = handler->properties_callback;

    err = memcpy_s(&params.bucketContext, sizeof(obs_bucket_context), &options->bucket_options,
        sizeof(obs_bucket_context));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memcpy_s), __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(trash_configuration_data);
		check_before_complete(handler->complete_callback, 
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}
    err = memcpy_s(&params.request_option, sizeof(obs_http_request_option), &options->request_options,
        sizeof(obs_http_request_option));
	if (checkIfErrorAndLogStrError(SYMBOL_NAME_STR(memcpy_s), __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(trash_configuration_data);
		check_before_complete(handler->complete_callback,
			OBS_STATUS_Security_Function_Failed, 0, callback_data, __FUNCTION__, __LINE__);
		return;
	}

    params.put_properties = &properties;
    params.httpRequestType = http_request_type_put;
    params.properties_callback = &update_bucket_common_properties_callback;
    params.toObsCallback = &update_bucket_common_data_callback;
    params.toObsCallbackTotalSize = trash_configuration_data->docLen;
    params.complete_callback = &update_bucket_common_complete_callback_no_free;
    params.callback_data = trash_configuration_data;
    params.isCheckCA = is_check_ca(options);
    params.storageClassFormat = no_need_storage_class;
    params.subResource = get_bucket_trash_configuration_sub_resource(use_api);
    params.temp_auth = options->temp_auth;
    params.use_api = use_api;
    request_perform(&params);
	CHECK_NULL_FREE(trash_configuration_data);
	COMMLOG(OBS_LOGINFO, "end %s!", __FUNCTION__);
}

char* get_bucket_trash_configuration_sub_resource(obs_use_api use_api) {
	switch (use_api)
	{
	case OBS_USE_API_OBS:
		return "x-obs-trash";
	case OBS_USE_API_S3:
		return "x-obs-trash";
	default:
		COMMLOG(OBS_LOGERROR, "unknown obs_use_api :%d, use default: x-obs-trash", use_api);
		return "x-obs-trash";
	}
}

obs_status init_bucket_trash_configuration_xml(bucket_trash_configuration * obs_trash_configuration,
	update_bucket_common_data *bucket_common_data)
{
	if (obs_trash_configuration == NULL) {
		COMMLOG(OBS_LOGERROR, "try generate a xml from a NULL pointer of %s!", SYMBOL_NAME_STR(bucket_trash_configuration));
		return OBS_STATUS_InvalidArgument;
	}

	bucket_common_data->docLen = snprintf_s(bucket_common_data->doc, sizeof(bucket_common_data->doc), _TRUNCATE,
		"<BucketTrashConfiguration><ReservedDays>%d</ReservedDays></BucketTrashConfiguration>", obs_trash_configuration->reserved_days);
	if (bucket_common_data->docLen < 0)
	{
		CheckAndLogNeg(bucket_common_data->docLen, "snprintf_s", __FUNCTION__, __LINE__);
		return OBS_STATUS_Security_Function_Failed;
	}

	bucket_common_data->docBytesWritten = 0;

	return OBS_STATUS_OK;
}

obs_status check_bucket_trash_config_params(const char* function, const obs_options *options, 
	bucket_trash_configuration* obs_trash_configuration,
	obs_response_handler *handler, void *callback_data) {
	
	obs_status ret = check_options_and_handler_params(function, options, handler, callback_data);

	if (ret != OBS_STATUS_OK) {
		return ret;
	}

	if (!CheckAndLogNULL(obs_trash_configuration, SYMBOL_NAME_STR(obs_trash_configuration),
		__FUNCTION__, function, __LINE__))
	{
		check_before_complete(handler->complete_callback,
			OBS_STATUS_InvalidArgument, 0, callback_data, __FUNCTION__, __LINE__);
		return OBS_STATUS_InvalidArgument;
	}
	return OBS_STATUS_OK;
}