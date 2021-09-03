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
#ifndef SIMPLEXML_H
#define SIMPLEXML_H

#include "eSDKOBS.h"


typedef obs_status (SimpleXmlCallback)(const char *elementPath, const char *data,
                                     int dataLen, void *callback_data);

typedef struct simple_xml
{
    void *xmlParser;

    SimpleXmlCallback *callback;

    void *callback_data;

    char elementPath[512];

    int elementPathLen;

    obs_status status;
} simple_xml;


// Simple XML parsing
// ----------------------------------------------------------------------------

void simplexml_initialize(simple_xml *simpleXml, SimpleXmlCallback *callback,
                          void *callback_data);

obs_status simplexml_add(simple_xml *simpleXml, const char *data, int dataLen);

void simplexml_deinitialize(simple_xml *simpleXml);

#endif /* SIMPLEXML_H */
