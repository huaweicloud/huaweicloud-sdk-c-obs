#include "request_util.h"
#include "access_label.h"
#include "file_utils.h"

static obs_status parse_dir_access_label_json(Access_label_data *data) {
	if (data->json_str == NULL) {
		COMMLOG(OBS_LOGERROR, "parse_dir_access_label_json failed! dir_access_label_data.json_str is NULL");
		return OBS_STATUS_InvalidArgument;
	}

	// parse dir_access_label_data.json_str
	cJSON *jsonRoot = cJSON_Parse(data->json_str);
	if (!CheckAndLogNULL(jsonRoot, SYMBOL_NAME_STR(jsonRoot), 
		SYMBOL_NAME_STR(cJSON_Parse), __FUNCTION__, __LINE__)) {
		return OBS_STATUS_JSON_PARSE_ERROR;
	}
	// parse accesslabel
	cJSON *accesslabel = cJSON_GetObjectItem(jsonRoot, "accesslabel");
	if (!CheckAndLogNULL(accesslabel, SYMBOL_NAME_STR(accesslabel),
		SYMBOL_NAME_STR(cJSON_GetObjectItem), __FUNCTION__, __LINE__)) {
		cJSON_Delete(jsonRoot);
		return OBS_STATUS_JSON_PARSE_ERROR;
	}
	int accesslabel_size = cJSON_GetArraySize(accesslabel);
	data->labels_len = accesslabel_size;
	for (int i = 0; i < accesslabel_size; i++) {
		cJSON *item = cJSON_GetArrayItem(accesslabel, i);
		if (!CheckAndLogNULL(item, SYMBOL_NAME_STR(item),
			SYMBOL_NAME_STR(cJSON_GetArrayItem), __FUNCTION__, __LINE__)) {
			COMMLOG(OBS_LOGERROR, "item index %d", i);
			cJSON_Delete(jsonRoot);
			return OBS_STATUS_JSON_PARSE_ERROR;
		}
		strcpy_s(data->labels[i], MAX_LABEL_LENGTH, item->valuestring);
	}
	cJSON_Delete(jsonRoot);
	return OBS_STATUS_OK;
}

#define MAX_LABEL_JSON_LENGTH (512 * 54 + 511 + 6 + 13 + 1)
char* initialize_json() {
	char* json_str = (char*)malloc(sizeof(char)*MAX_LABEL_JSON_LENGTH);
	if (!CheckAndLogNULL(json_str, SYMBOL_NAME_STR(json_str), SYMBOL_NAME_STR(malloc), __FUNCTION__, __LINE__)) {
		return NULL;
	}
	else {
		errno_t ret = memset_s(json_str, sizeof(char)*MAX_LABEL_JSON_LENGTH, 0, sizeof(char)*MAX_LABEL_JSON_LENGTH);
		CheckAndLogNoneZero(ret, SYMBOL_NAME_STR(memset_s), __FUNCTION__, __LINE__);
		return json_str;
	}
}

void get_access_label(const obs_options *options,
 char* key, get_access_label_handler * handler,
	Access_label_data * dir_access_labels)
{
	COMMLOG(OBS_LOGINFO, "Enter get_access_label successfully !");
	obs_use_api use_api = OBS_USE_API_S3;
	request_params params;
	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, &handler->response_handler, dir_access_labels)) {
		return;
	}
	// if user didn't alloc memory for json_str, alloc mem here
	char* json_alloc = NULL;
	char* json_alloc_user = dir_access_labels->json_str;
	int userAlloced = json_alloc_user != NULL;
	if (!userAlloced) {
		COMMLOG(OBS_LOGINFO, "user didn't alloc memory for json_str.");
		json_alloc = initialize_json();
		if (json_alloc == NULL) {
			(void)(*(handler->response_handler.complete_callback))(OBS_STATUS_OutOfMemory, 0, dir_access_labels);
			return;
		}
		dir_access_labels->json_str = json_alloc;
	}
	params.httpRequestType = http_request_type_get;
	params.temp_auth = options->temp_auth;
	params.key = key;
	params.subResource = get_access_label_subResource(use_api);
	params.properties_callback = handler->response_handler.properties_callback;
	params.complete_callback = handler->response_handler.complete_callback;
	params.fromObsCallback = handler->get_access_label_callback_impl;
	params.callback_data = dir_access_labels;
	params.isCheckCA = is_check_ca(options);
	params.use_api = use_api;
	params.storageClassFormat = no_need_storage_class;
	request_perform(&params);
	if (OBS_STATUS_OK == dir_access_labels->status) {
		// parse dir_access_label_json only when request_perform success
		dir_access_labels->status = parse_dir_access_label_json(dir_access_labels);
	}
	if (OBS_STATUS_OK != dir_access_labels->status) {
		(void)(*(handler->response_handler.complete_callback))(dir_access_labels->status, 0, dir_access_labels);
	}
	log_dir_access_label_data(dir_access_labels);
	dir_access_labels->json_str = json_alloc_user;
	CHECK_NULL_FREE(json_alloc);
	COMMLOG(OBS_LOGINFO, "Leave get_access_label successfully !");
}