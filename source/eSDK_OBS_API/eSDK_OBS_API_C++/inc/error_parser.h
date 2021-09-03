/** **************************************************************************
 *
 * Copyright 2008 Bryan Ischo <bryan@ischo.com>
 *
 * This file is part of libs3.
 *
 * libs3 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, version 3 of the License.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of this library and its programs with the
 * OpenSSL library, and distribute linked combinations including the two.
 *
 * libs3 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with libs3, in a file named COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 ************************************************************************** **/

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
