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
