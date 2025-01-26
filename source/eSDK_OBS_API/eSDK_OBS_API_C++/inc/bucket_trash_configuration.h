#ifndef BUCKET_TRASH_CONFIGURATION
#define BUCKET_TRASH_CONFIGURATION
#include "eSDKOBS.h"
char* get_bucket_trash_configuration_sub_resource(obs_use_api use_api);
obs_status check_bucket_trash_config_params(const char* function, const obs_options *options, 
	bucket_trash_configuration* obs_trash_configuration,
	obs_response_handler *handler, void *callback_data);
#endif #BUCKET_TRASH_CONFIGURATION