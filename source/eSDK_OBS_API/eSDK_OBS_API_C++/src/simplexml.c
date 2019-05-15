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
#include <libxml/parser.h>
#include <string.h>
#include "simplexml.h"
#include "securec.h"

static xmlEntityPtr saxGetEntity(void *user_data, const xmlChar *name)
{
    (void) user_data;

    return xmlGetPredefinedEntity(name);
}


static void saxStartElement(void *user_data, const xmlChar *nameUtf8,
                            const xmlChar **attr)
{
    (void) attr;

    simple_xml *simpleXml = (simple_xml *) user_data;

    if (simpleXml->status != OBS_STATUS_OK) {
        return;
    }
    char *name = (char *) nameUtf8;
    int len = strlen(name);

    if ((simpleXml->elementPathLen + len + 1) >= 
        (int) sizeof(simpleXml->elementPath)) {
        simpleXml->status = OBS_STATUS_XmlParseFailure;
        return;
    }

    if (simpleXml->elementPathLen) {
        simpleXml->elementPath[simpleXml->elementPathLen++] = '/';
    }
    strcpy_s(&(simpleXml->elementPath[simpleXml->elementPathLen]), 
    sizeof(simpleXml->elementPath)-simpleXml->elementPathLen, name);
    simpleXml->elementPathLen += len;
}


static void saxEndElement(void *user_data, const xmlChar *name)
{
    (void) name;

    simple_xml *simpleXml = (simple_xml *) user_data;

    if (simpleXml->status != OBS_STATUS_OK) {
        return;
    }

    simpleXml->status = (*(simpleXml->callback))
        (simpleXml->elementPath, 0, 0, simpleXml->callback_data);

    while ((simpleXml->elementPathLen > 0) &&
           (simpleXml->elementPath[simpleXml->elementPathLen] != '/')) {
        simpleXml->elementPathLen--;
    }

    simpleXml->elementPath[simpleXml->elementPathLen] = 0;
}


static void saxCharacters(void *user_data, const xmlChar *ch, int len)
{
    simple_xml *simpleXml = (simple_xml *) user_data;

    if (simpleXml->status != OBS_STATUS_OK) {
        return;
    }

    simpleXml->status = (*(simpleXml->callback))
        (simpleXml->elementPath, (char *) ch, len, simpleXml->callback_data);
}


static void saxError(void *user_data, const char *msg, ...)
{
    (void) msg;

    simple_xml *simpleXml = (simple_xml *) user_data;

    if (simpleXml->status != OBS_STATUS_OK) {
        return;
    }

    simpleXml->status = OBS_STATUS_XmlParseFailure;
}
static struct _xmlSAXHandler saxHandlerG =
{
    0, // internalSubsetSAXFunc
    0, // isStandaloneSAXFunc
    0, // hasInternalSubsetSAXFunc
    0, // hasExternalSubsetSAXFunc
    0, // resolveEntitySAXFunc
    &saxGetEntity, // getEntitySAXFunc
    0, // entityDeclSAXFunc
    0, // notationDeclSAXFunc
    0, // attributeDeclSAXFunc
    0, // elementDeclSAXFunc
    0, // unparsedEntityDeclSAXFunc
    0, // setDocumentLocatorSAXFunc
    0, // startDocumentSAXFunc
    0, // endDocumentSAXFunc
    &saxStartElement, // startElementSAXFunc
    &saxEndElement, // endElementSAXFunc
    0, // referenceSAXFunc
    &saxCharacters, // charactersSAXFunc
    0, // ignorableWhitespaceSAXFunc
    0, // processingInstructionSAXFunc
    0, // commentSAXFunc
    0, // warningSAXFunc
    &saxError, // errorSAXFunc
    &saxError, // fatalErrorSAXFunc
    0, // getParameterEntitySAXFunc
    &saxCharacters, // cdataBlockSAXFunc
    0, // externalSubsetSAXFunc
    0, // initialized
    0, // _private
    0, // startElementNsSAX2Func
    0, // endElementNsSAX2Func
    0 // xmlStructuredErrorFunc serror;
};
void simplexml_initialize(simple_xml *simpleXml, 
                          SimpleXmlCallback *callback, void *callback_data)
{
    simpleXml->callback = callback;
    simpleXml->callback_data = callback_data;
    simpleXml->elementPathLen = 0;
    simpleXml->status = OBS_STATUS_OK;
    simpleXml->xmlParser = 0;
    memset_s(simpleXml->elementPath, sizeof(simpleXml->elementPath), 0, sizeof(simpleXml->elementPath));
}


void simplexml_deinitialize(simple_xml *simpleXml)
{
    if (simpleXml->xmlParser) {
        xmlFreeParserCtxt((xmlParserCtxtPtr)simpleXml->xmlParser);
    }
}


obs_status simplexml_add(simple_xml *simpleXml, const char *data, int dataLen)
{
    if (!simpleXml->xmlParser &&((simpleXml->xmlParser = xmlCreatePushParserCtxt(&saxHandlerG, simpleXml, 0, 0, 0)) == NULL)) {
        return OBS_STATUS_InternalError;
    }

    if (xmlParseChunk((xmlParserCtxtPtr) simpleXml->xmlParser, 
                      data, dataLen, 0)) {
        return OBS_STATUS_XmlParseFailure;
    }

    return simpleXml->status;
}