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
#include <ctype.h>
#include <string.h>
#include "response_headers_handler.h"

#define MAX(a,b) (((a)>(b))?(a):(b))

int prefix_cmp(const char *header, const char* prefix, int namelen)
{
	return strncmp(header, prefix, namelen) == 0 && namelen == (int)(strlen(prefix));
}

void response_headers_handler_initialize(response_headers_handler *handler)
{
    memset(&handler->responseProperties, 0, sizeof(obs_response_properties));
    handler->responseProperties.last_modified = -1;
    handler->done = 0;
    string_multibuffer_initialize(handler->responsePropertyStrings);
    string_multibuffer_initialize(handler->responseMetaDataStrings);
}

void parse_xml_header(response_headers_handler *handler, char *header, 
	int namelen, int valuelen, char *c)
{
	int fit;
	obs_response_properties *responseProperties = &(handler->responseProperties);

	if (prefix_cmp(header, "x-amz-request-id", namelen)||prefix_cmp(header, "x-obs-request-id", namelen)) {
		responseProperties->request_id = 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-reserved-indicator", namelen)) {
		responseProperties->reserved_indicator = 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-amz-id-2", namelen)||prefix_cmp(header, "x-obs-id-2", namelen)) {
		responseProperties->request_id2 = 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "Content-Type", namelen)) {
		responseProperties->content_type = 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "Content-Length", namelen)) {
		handler->responseProperties.content_length = 0;
		while (*c) {
			handler->responseProperties.content_length *= 10;
			handler->responseProperties.content_length += (*c++ - '0');
		}
	}
	else_if (prefix_cmp(header, "Server", namelen)) {
		responseProperties->server = 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "ETag", namelen)) {
		responseProperties->etag = 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-amz-expiration", namelen)||prefix_cmp(header, "x-obs-expiration", namelen)) {
		responseProperties->expiration= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if(prefix_cmp(header, "x-amz-website-redirect-location", namelen)||prefix_cmp(header, "x-obs-website-redirect-location", namelen)) {
		responseProperties->website_redirect_location= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-amz-version-id", namelen)||prefix_cmp(header, "x-obs-version-id", MAX(namelen, 16))) {
		responseProperties->version_id= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "Access-Control-Allow-Origin", namelen)) {
		responseProperties->allow_origin= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "Access-Control-Allow-Headers", namelen)) {
		responseProperties->allow_headers= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "Access-Control-Max-Age", namelen)) {
		responseProperties->max_age= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "Access-Control-Allow-Methods", namelen)) {
		responseProperties->allow_methods= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "Access-Control-Expose-Headers", namelen)) {
		responseProperties->expose_headers= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-default-storage-class", namelen)||prefix_cmp(header, "x-obs-storage-class", namelen)) {
		responseProperties->storage_class=
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c,
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-amz-storage-class", namelen)) {
		responseProperties->storage_class=
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c,
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-amz-server-side-encryption", namelen)||prefix_cmp(header, "x-obs-server-side-encryption", namelen)) {
		responseProperties->server_side_encryption= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
		responseProperties->use_server_side_encryption = 1;
	}
	else_if (prefix_cmp(header, "x-amz-server-side-encryption-aws-kms-key-id", namelen)||prefix_cmp(header, "x-obs-server-side-encryption-aws-kms-key-id", namelen)) {
		responseProperties->kms_key_id= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-amz-server-side-encryption-customer-algorithm", namelen)||prefix_cmp(header, "x-obs-server-side-encryption-customer-algorithm", namelen)) {
		responseProperties->customer_algorithm= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
		if(prefix_cmp(c,"AES256",sizeof("AES256")-1))
		{
			responseProperties->use_server_side_encryption = 1;
		}
	}
	else_if (prefix_cmp(header, "x-amz-server-side-encryption-customer-key-MD5", namelen)||prefix_cmp(header, "x-obs-server-side-encryption-customer-key-MD5", namelen)) {
		responseProperties->customer_key_md5= 
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c, 
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-obs-bucket-location", namelen)) {
		responseProperties->bucket_location=
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c,
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-obs-version", namelen)) {
		responseProperties->obs_version =
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c,
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-amz-restore", namelen)||prefix_cmp(header, "x-obs-restore", namelen)) {
		responseProperties->restore =
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c,
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-obs-object-type", namelen)) {
		responseProperties->obs_object_type =
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c,
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-obs-next-append-position", namelen)) {
		responseProperties->obs_next_append_position =
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c,
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-amz-epid", namelen)) {
		responseProperties->obs_head_epid  =
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c,
			valuelen, fit);
	}
	else_if (prefix_cmp(header, "x-obs-epid", namelen)) {
		responseProperties->obs_head_epid  =
			string_multibuffer_current(handler->responsePropertyStrings);
		string_multibuffer_add(handler->responsePropertyStrings, c,
			valuelen, fit);
	}
	else_if (prefix_cmp(header, OBS_METADATA_HEADER_NAME_PREFIX, 
		sizeof(OBS_METADATA_HEADER_NAME_PREFIX) - 1) ||
		prefix_cmp(header, "x-obs-meta-", sizeof("x-obs-meta-") - 1)) 
	{
		if (handler->responseProperties.meta_data_count ==
			sizeof(handler->responseMetaData)) {
				return;
		}
		char *metaName = &(header[sizeof(OBS_METADATA_HEADER_NAME_PREFIX) - 1]);
		int metaNameLen = 
			(namelen - (sizeof(OBS_METADATA_HEADER_NAME_PREFIX) - 1));
		char *copiedName = string_multibuffer_current(handler->responseMetaDataStrings);
		string_multibuffer_add(handler->responseMetaDataStrings, metaName, metaNameLen, fit);
		if (!fit) {
			return;
		}

		char *copiedValue = string_multibuffer_current(handler->responseMetaDataStrings);
		string_multibuffer_add(handler->responseMetaDataStrings, c, valuelen, fit);
		if (!fit) {
			return;
		}

		if (!handler->responseProperties.meta_data_count) {
			handler->responseProperties.meta_data = handler->responseMetaData;
		}

		obs_name_value *metaHeader = &(handler->responseMetaData
			[handler->responseProperties.meta_data_count++]);
		metaHeader->name = copiedName;
		metaHeader->value = copiedValue;
	}

	return;
} 

void response_headers_handler_add(response_headers_handler *handler,
                                  char *header, int len)
{
    char *end = &(header[len]);
    if (handler->done) {
        return;
    }
    
    if (handler->responsePropertyStringsSize == (sizeof(handler->responsePropertyStrings) - 1)) {
        return;
    }
    if (len < 3) {
        return;
    }
    
    while (is_blank(*header)) {
        header++;
    }
    end -= 3;
    while ((end > header) && is_blank(*end)) {
        end--;
    }
    if (!is_blank(*end)) {
        end++;
    }

    if (end == header) {
        return;
    }

    *end = 0;
    char *c = header;
    while (*c && (*c != ':')) {
        c++;
    }
    
    int namelen = c - header;
    c++;
    while (is_blank(*c)) {
        c++;
    }
    int valuelen = (end - c) + 1;
    
    parse_xml_header(handler, header, namelen, valuelen, c);

    return;
}
void response_headers_handler_done(response_headers_handler *handler, CURL *curl)
{
    time_t last_modified = -1;
    if (curl_easy_getinfo
        (curl, CURLINFO_FILETIME, &last_modified) == CURLE_OK) {
        handler->responseProperties.last_modified = last_modified;
    }
    
    handler->done = 1;
}
