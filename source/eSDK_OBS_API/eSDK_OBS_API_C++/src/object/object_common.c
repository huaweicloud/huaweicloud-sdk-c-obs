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
#include <stdlib.h>
#include <string.h>
#include "eSDKOBS.h"
#include "request.h"
#include "securec.h"
#include "object.h"
#include "file_utils.h"
#include "request_util.h"
#include <openssl/md5.h> 

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined __GNUC__ || defined LINUX
#include <unistd.h>
#include <pthread.h>
#endif

#if defined WIN32
#define read _read
#define close _close
#define write _write
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>


int check_file_is_valid(char *file_name)
{
    int ret_stat = -1;
#if defined __GNUC__ || defined LINUX  
    struct stat statbuf;
    ret_stat = stat(file_name, &statbuf);
#else
    struct _stati64 statbuf;
    ret_stat = file_stati64(file_name, &statbuf);
#endif
    if (ret_stat == -1)
    {
        COMMLOG(OBS_LOGERROR, "%s file[%s] is not exist", "readCheckpointFile", file_name);
        return -1;
    }

    if (statbuf.st_size == 0)
    {
        COMMLOG(OBS_LOGERROR, "%s checkpoint file[%s] size is 0 !", "readCheckpointFile", file_name);
        return -1;
    }

    return 0;
}

xmlNodePtr get_xmlnode_from_file(const char * file_name, xmlDocPtr *doc)
{
    xmlNodePtr curNode;
    *doc = checkPointFileRead(file_name, "utf-8", XML_PARSE_RECOVER);
    if (NULL == *doc)
    {
        COMMLOG(OBS_LOGERROR, "Document not parsed successfully.");
        return NULL;
    }

    curNode = xmlDocGetRootElement(*doc);
    if (NULL == curNode)
    {
        COMMLOG(OBS_LOGERROR, "empty document");
        return NULL;
    }

    return curNode;
}

int updataCheckPointFindNode(xmlNodePtr *curNode, unsigned int strNum, char(*strArry)[32])
{
    int i = 0;
    for (i = 1; i < strNum; i++)
    {
        while ((*curNode) != NULL)
        {
            if (!xmlStrcmp((*curNode)->name, BAD_CAST strArry[i]))
            {
                break;
            }
            (*curNode) = (*curNode)->next;
        }
        if ((*curNode) == NULL)
        {
            break;
        }
        if ((strNum - 1) > i)
        {
            (*curNode) = (*curNode)->children;
        }
    }
    return i;
}

void checkAndXmlFreeDoc(xmlDocPtr* doc) {

	if (doc != NULL && *doc != NULL) {
		xmlFreeDoc(*doc);
		*doc = NULL;
	}
}

int updateCheckPoint(char * elementPath, const char * content, const char * file_name)
{
    xmlNodePtr curNode;
    xmlDocPtr doc;           //the doc pointer to parse the file
    unsigned int i = 0;
    char * strToParse = elementPath;
    char strArry[MAX_XML_DEPTH][32] = { {0} };
    unsigned int strNum = 0;

    char* p = strtok(strToParse, "/");
    while (p != NULL && strNum < MAX_XML_DEPTH) {
        int ret = strncpy_s(strArry[strNum], ARRAY_LENGTH_32, p, strlen(p) + 1);
        CheckAndLogNoneZero(ret, "strncpy_s", __FUNCTION__, __LINE__);
        p = strtok(NULL, "/");
        strNum++;
    }

    curNode = get_xmlnode_from_file(file_name, &doc);
    if (NULL == curNode)
    {
        COMMLOG(OBS_LOGERROR, "empty document");
		checkAndXmlFreeDoc(&doc);
        return -1;
    }

    if (xmlStrcmp(curNode->name, BAD_CAST strArry[0]))
    {
        COMMLOG(OBS_LOGERROR, "document of the wrong type, root node != strArry[0]");
		checkAndXmlFreeDoc(&doc);
        return -1;
    }

    curNode = curNode->xmlChildrenNode;
    i = updataCheckPointFindNode(&curNode, strNum, strArry);

    if ((i == strNum) && (curNode != NULL))
    {
        xmlNodeSetContent(curNode, (const xmlChar *)content);
        checkPointFileSave(file_name, doc);
    }
    else
    {
		checkAndXmlFreeDoc(&doc);
        return -1;
    }


	checkAndXmlFreeDoc(&doc);
    return 0;
}

int isXmlFileValid(const char * file_name, exml_root xmlRootIn)
{
    xmlNodePtr curNode;
    int retVal = 0;
    xmlDocPtr doc;           //the doc pointer to parse the file
    doc = checkPointFileRead(file_name, "utf-8", XML_PARSE_RECOVER); //parse the xml file

    if (NULL == doc)
    {
        COMMLOG(OBS_LOGERROR, "Document not parsed successfully. ");
        xmlFreeDoc(doc);
        return 0;
    }

    curNode = xmlDocGetRootElement(doc); //get the root node
    if (NULL == curNode)
    {
        COMMLOG(OBS_LOGERROR, "empty document");
        xmlFreeDoc(doc);
        return 0;
    }

    if ((!xmlStrcmp(curNode->name, BAD_CAST "uploadinfo")) && (xmlRootIn == UPLOAD_FILE_INFO))
    {
        retVal = 1;
    }

    if ((!xmlStrcmp(curNode->name, BAD_CAST "downloadinfo")) && (xmlRootIn == DOWNLOAD_FILE_INFO))
    {
        retVal = 1;
    }
    xmlFreeDoc(doc);
    return retVal;

}

int open_file(const char * file_name, int *ret_stat, int *file_size)
{
    int fd = 0;

#if defined __GNUC__ || defined LINUX  
    struct stat statbuf;
    *ret_stat = stat(file_name, &statbuf);
#else
    struct _stati64 statbuf;
    *ret_stat = file_stati64(file_name, &statbuf);
#endif
    if (*ret_stat == -1)
    {
#if defined __GNUC__ || defined LINUX
        fd = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
#else
        (void)file_sopen_s(&fd, file_name, _O_RDWR | _O_CREAT | _O_BINARY, _SH_DENYNO,
            _S_IREAD | _S_IWRITE);
#endif
        if (fd != -1)
        {
			checkAndLogStrError(SYMBOL_NAME_STR(open), __FUNCTION__, __LINE__);
            close(fd);
        }
    }
    else
    {
        *file_size = statbuf.st_size;
    }

    return fd;
}


int set_check_pointFile_with_name(const char * checkPointFileName,
    int * isFirstTimeUpload, exml_root xmlRootIn)
{
    int retVal = 0;
    int fd = -1;
    int ret_stat = -1;
    int file_size = 0;

    fd = open_file(checkPointFileName, &ret_stat, &file_size);
    if (fd == -1)
    {
        COMMLOG(OBS_LOGERROR, "%s create checkpoint file failed !", "setCheckPointFile");
        return -1;
    }

    if (ret_stat == -1)
    {
        *isFirstTimeUpload = 1;
        retVal = 0;
    }
    else
    {
        retVal = 0;
        if (file_size != 0)
        {
            retVal = isXmlFileValid(checkPointFileName, xmlRootIn);
        }

        if (retVal == 1)
        {
            *isFirstTimeUpload = 0;
        }
        else
        {
            retVal = -1;
            COMMLOG(OBS_LOGERROR, "%s check point file exist but is not valid !", "setCheckPointFile");
        }
    }

    return retVal;
}

int set_check_pointFile_with_null(const char * uploadFileName, char * checkPointFileName,
    int * isFirstTimeUpload, exml_root xmlRootIn)
{
    int retVal = 0;
    int fd = -1;
    int ret_stat = 0;
    int isxmlValid = -1;
    int file_size = 0;

    int ret = checkpoint_file_path_printf(checkPointFileName, ARRAY_LENGTH_1024, uploadFileName);
    CheckAndLogNeg(ret, "checkpoint_file_path_printf", __FUNCTION__, __LINE__);
    while (ret_stat == 0)
    {
        fd = open_file(checkPointFileName, &ret_stat, &file_size);
        if (fd == -1)
        {
            COMMLOG(OBS_LOGERROR, "%s create checkpoint file failed !", "setCheckPointFile");
            return -1;
        }

        if (ret_stat == -1)
        {
            *isFirstTimeUpload = 1;
            retVal = 0;
            break;
        }

        if (file_size != 0)
        {
            isxmlValid = isXmlFileValid(checkPointFileName, xmlRootIn);
        }

        if (isxmlValid == 1)
        {
            *isFirstTimeUpload = 0;
            retVal = 0;
            break;
        }

        retVal = file_path_append(checkPointFileName, ARRAY_LENGTH_1024);
        if (retVal != 0)
        {
            retVal = -1;
            break;
        }
    }

    return retVal;
}


 int setCheckPointFile(const char * uploadFileName, char * checkPointFileName,
    int * isFirstTimeUpload, exml_root xmlRootIn)
{
    *isFirstTimeUpload = 1;

    if (uploadFileName == NULL)
    {
        return -1;
    }

    if ((checkPointFileName != NULL) && (checkPointFileName[0] != '\0'))
    {
        return set_check_pointFile_with_name(checkPointFileName, isFirstTimeUpload, xmlRootIn);
    }
    else
    {
        return set_check_pointFile_with_null(uploadFileName, checkPointFileName, isFirstTimeUpload,
            xmlRootIn);
    }
}


 void ListPartsCompleteCallback_Intern(obs_status status,
    const obs_error_details *error,
    void *callback_data)
{
    lisPartResult * pstResult = (lisPartResult *)callback_data;

    pstResult->retStatus = status;
    //the following 4 lines is just to avoid warning
    if (error)
    {
        return;
    }
    return;
}

 obs_status ListPartsPropertiesCallback_Intern(const obs_response_properties *properties,
    void *callback_data)
{
    if (properties || callback_data)
    {
        return OBS_STATUS_OK;
    }
    return OBS_STATUS_OK;
}

 obs_status copyObjectDataCallback(int buffer_size, const char *buffer,
    void *callback_data)
{

    copy_object_data *coData = (copy_object_data *)callback_data;

    return simplexml_add(&(coData->simpleXml), buffer, buffer_size);
}
