#include "request_util.h"
#include "file_utils.h"

obs_status init_access_label(Access_label_data * dir_access_label_json_datas)
{
	if (dir_access_label_json_datas == NULL) {
		COMMLOG(OBS_LOGERROR, "try initializing a NULL pointer of %s!", SYMBOL_NAME_STR(Access_label_data));
		return OBS_STATUS_InvalidArgument;
	}
	int labelsSize = sizeof(dir_access_label_json_datas->labels);
	errno_t ret = memset_s(dir_access_label_json_datas->labels, labelsSize, 0, labelsSize);
	if (ret != EOK) {
		CheckAndLogNoneZero(ret, SYMBOL_NAME_STR(memset_s), __FUNCTION__, __LINE__);
		return OBS_STATUS_Security_Function_Failed;
	}

	dir_access_label_json_datas->labels_len = 0;
	dir_access_label_json_datas->json_current_offset = 0;
	dir_access_label_json_datas->json_str = NULL;
	dir_access_label_json_datas->json_str_remain_size = 0;
	dir_access_label_json_datas->status = OBS_STATUS_BUTT;
	dir_access_label_json_datas->callback_data = NULL;
	return OBS_STATUS_OK;
}

static char* compose_dir_access_label_json(char dir_access_labels[MAX_LABEL_NUM][MAX_LABEL_LENGTH], int dir_access_labels_len) {
	if (dir_access_labels == NULL) {
		COMMLOG(OBS_LOGERROR, "get_dir_access_label_json failed ! dir_access_label is NULL");
		return NULL;
	}
	else if (dir_access_labels_len <= 0) {
		COMMLOG(OBS_LOGERROR, "get_dir_access_label_json failed ! dir_access_label->access_labels_len is %d", dir_access_labels_len);
		return NULL;
	}
	cJSON *jsonRoot = cJSON_CreateObject();
	if (!CheckAndLogNULL(jsonRoot, SYMBOL_NAME_STR(jsonRoot),
		SYMBOL_NAME_STR(cJSON_CreateObject), __FUNCTION__, __LINE__)) {
		return NULL;
	}
	cJSON *accesslabel = cJSON_CreateArray();
	if (!CheckAndLogNULL(accesslabel, SYMBOL_NAME_STR(accesslabel),
		SYMBOL_NAME_STR(cJSON_CreateArray), __FUNCTION__, __LINE__)) {
		cJSON_Delete(jsonRoot);
		return NULL;
	}
	for (int i = 0; i < dir_access_labels_len; ++i) {
		cJSON* accesslabelJson = cJSON_CreateString(dir_access_labels[i]);
		int isNotNull = CheckAndLogNULL(accesslabelJson, SYMBOL_NAME_STR(accesslabelJson),
			SYMBOL_NAME_STR(cJSON_CreateString), __FUNCTION__, __LINE__);
		if (!(isNotNull != 0 && cJSON_AddItemToArray(accesslabel, accesslabelJson))) {
			COMMLOG(OBS_LOGERROR, "cJSON_AddItemToArray failed in item %d", i);
			cJSON_Delete(accesslabel);
			cJSON_Delete(jsonRoot);
			return NULL;
		}
	}
	if (!cJSON_AddItemToObject(jsonRoot, "accesslabel", accesslabel)) {
		COMMLOG(OBS_LOGERROR, "cJSON_AddItemToObject failed in %s, line %d", __FUNCTION__, __LINE__);
		cJSON_Delete(accesslabel);
		cJSON_Delete(jsonRoot);
		return NULL;
	}

	char *json_str = cJSON_PrintUnformatted(jsonRoot);
	CheckAndLogNULL(json_str, SYMBOL_NAME_STR(json_str),
		SYMBOL_NAME_STR(cJSON_PrintUnformatted), __FUNCTION__, __LINE__);
	cJSON_Delete(jsonRoot);
	return json_str;
}

char* get_access_label_subResource(obs_use_api use_api) {
	switch (use_api)
	{
	case OBS_USE_API_OBS:
		return "x-obs-accesslabel";
	case OBS_USE_API_S3:
		return "x-amz-accesslabel";
	default:
		COMMLOG(OBS_LOGERROR, "unknown obs_use_api :%d, use default: x-obs-accesslabel", use_api);
		return "x-obs-accesslabel";
	}
}

void log_dir_access_label_data(Access_label_data * dir_access_label_datas) {
	obs_status status = dir_access_label_datas->status;
	OBS_LOGLEVEL logLevel = OBS_LOGERROR;
	if (dir_access_label_datas == NULL) {
		COMMLOG(OBS_LOGERROR, "failed to log json_str, because %s is NULL.", SYMBOL_NAME_STR(Access_label_data));
		return;
	}
	else if (status == OBS_STATUS_OK) {
		logLevel = OBS_LOGDEBUG;
	}
	else {
		logLevel = OBS_LOGERROR;
	}
	char* structName = SYMBOL_NAME_STR(Access_label_data);
	COMMLOG(logLevel, "%s.obs_status is %s", structName, obs_get_status_name(status));
	COMMLOG(logLevel, "%s.json_str is %s", structName, dir_access_label_datas->json_str);
	COMMLOG(logLevel, "%s.labels_len is %d", structName, dir_access_label_datas->labels_len);
	int labels_len = dir_access_label_datas->labels_len;
	for (int i = 0; i < labels_len; ++i) {
		COMMLOG(logLevel, "%s.labels[%d] is %s", structName, i, dir_access_label_datas->labels[i]);
	}
}

bool isLabelLengthLegal(char label[MAX_LABEL_LENGTH]) {
	for (int i = 0; i < MAX_LABEL_LENGTH; i++) {
		if ('\0' == label[i]) {
			return true;
		}
	}
	return false;
}

obs_status check_label_data_and_log(obs_response_handler *handler,
	Access_label_data * dir_access_labels) {
	if (dir_access_labels == NULL) {
		COMMLOG(OBS_LOGERROR, "parameter %s is NULL", SYMBOL_NAME_STR(dir_access_labels));
		(void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, dir_access_labels);
		return OBS_STATUS_InvalidArgument;
	}
	if (dir_access_labels->labels_len == 0 && dir_access_labels->json_str == NULL) {
		COMMLOG(OBS_LOGERROR, "parameter %s is 0", SYMBOL_NAME_STR(dir_access_labels->labels_len));
		COMMLOG(OBS_LOGERROR, "parameter %s is NULL", SYMBOL_NAME_STR(dir_access_labels->json_str));
		(void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, dir_access_labels);
		return OBS_STATUS_InvalidArgument;
	}

	int labels_len = dir_access_labels->labels_len;
	for (int i = 0; i < labels_len; ++i) {
		if (!isLabelLengthLegal(dir_access_labels->labels[i])) {
			COMMLOG(OBS_LOGERROR, "%s.labels[%d] doesn't contains \\0, label max length is %d", SYMBOL_NAME_STR(Access_label_data), i, MAX_LABEL_LENGTH - 1);
			(void)(*(handler->complete_callback))(OBS_STATUS_InvalidArgument, 0, dir_access_labels);
			return OBS_STATUS_InvalidArgument;
		}
	}
	return OBS_STATUS_OK;
}

void set_access_label(const obs_options *options,
 char* key, put_access_label_handler * handler,
	Access_label_data * dir_access_labels)
{
	COMMLOG(OBS_LOGINFO, "Enter set_access_label successfully !");
	obs_use_api use_api = OBS_USE_API_S3;
	request_params params;
	if (OBS_STATUS_OK != copy_options_and_init_params(options, &params, &use_api, &handler->response_handler, dir_access_labels)) {
		return;
	}
	if (OBS_STATUS_OK != check_label_data_and_log(&handler->response_handler, dir_access_labels)) {
		return;
	}
	params.httpRequestType = http_request_type_put;
	params.key = key;
	params.temp_auth = options->temp_auth;
	params.subResource = get_access_label_subResource(use_api);
	params.toObsCallback = handler->put_access_label_callback_impl;
	char* cjson_str = NULL;
	if (dir_access_labels->json_str == NULL) {
		cjson_str = compose_dir_access_label_json(dir_access_labels->labels, dir_access_labels->labels_len);
		if (cjson_str == NULL) {

		}
		dir_access_labels->json_str = cjson_str;
	}
	if (dir_access_labels->json_str_remain_size <= 0 && 
		dir_access_labels->json_str != NULL) {
		dir_access_labels->json_str_remain_size = strlen(dir_access_labels->json_str);
	}
	params.toObsCallbackTotalSize = dir_access_labels->json_str_remain_size;
	params.callback_data = dir_access_labels;
	params.isCheckCA = is_check_ca(options);
	params.use_api = use_api;
	params.properties_callback = handler->response_handler.properties_callback;
	params.complete_callback = handler->response_handler.complete_callback;
	params.storageClassFormat = no_need_storage_class;
	request_perform(&params);
	log_dir_access_label_data(dir_access_labels);
	cJSON_free(cjson_str);
	COMMLOG(OBS_LOGINFO, "Leave set_access_label successfully !");
}