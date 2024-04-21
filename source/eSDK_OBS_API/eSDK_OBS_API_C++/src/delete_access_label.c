#include "request_util.h"
#include "access_label.h"
void delete_access_label(const obs_options *options, char* key,
	obs_response_handler *handler,
	void* callback_data) {
	request_params params;
	obs_use_api use_api = OBS_USE_API_S3;
	COMMLOG(OBS_LOGINFO, "Enter delete_object successfully !");
	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, handler, callback_data)) {
		return;
	}
	params.temp_auth = options->temp_auth;
	params.httpRequestType = http_request_type_delete;
	params.key = key;
	params.subResource = get_access_label_subResource(use_api);
	params.properties_callback = handler->properties_callback;
	params.complete_callback = handler->complete_callback;
	params.callback_data = callback_data;
	params.isCheckCA = is_check_ca(options);
	params.storageClassFormat = no_need_storage_class;
	params.use_api = use_api;
	request_perform(&params);
	COMMLOG(OBS_LOGINFO, "Leave delete_object successfully !");
}