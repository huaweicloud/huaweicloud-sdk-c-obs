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
#ifndef ERROR_PARSER_H
#define ERROR_PARSER_H

#include "eSDKOBS.h"
#include "simplexml.h"
#include "string_buffer.h"


#define EXTRA_DETAILS_SIZE 8

typedef struct error_parser
{
    obs_error_details obsErrorDetails;

    simple_xml errorXmlParser;

    int errorXmlParserInitialized;

    string_buffer(code, 1024);

    string_buffer(message, 1024);

    string_buffer(resource, 1024);

    string_buffer(further_details, 1024);
    
    obs_name_value extra_details[EXTRA_DETAILS_SIZE];

    string_multibuffer(extraDetailsNamesValues, EXTRA_DETAILS_SIZE * 1024);
} error_parser;


// Always call this
void error_parser_initialize(error_parser *errorParser);

obs_status error_parser_add(error_parser *errorParser, const char *buffer,
                          int buffer_size);

void error_parser_convert_status(error_parser *errorParser, obs_status *status);

// Always call this
void error_parser_deinitialize(error_parser *errorParser);


#endif /* ERROR_PARSER_H */
