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
#include "request_util.h"
#include <openssl/md5.h> 

#if defined WIN32
#include <io.h>
#include <share.h>
#include <process.h>
CRITICAL_SECTION  g_csThreadCheckpoint;
CRITICAL_SECTION  g_csThreadCheckpoint_download;
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined __GNUC__ || defined LINUX
#include <unistd.h>
#include <pthread.h>
pthread_mutex_t g_mutexThreadCheckpoint;
pthread_mutex_t g_mutexThreadCheckpoint_download;
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>

void initialize_break_point_lock()
{
#if defined __GNUC__ || defined LINUX
    pthread_mutex_init(&g_mutexThreadCheckpoint_download, NULL);
    pthread_mutex_init(&g_mutexThreadCheckpoint, NULL);
#else
    InitializeCriticalSection(&g_csThreadCheckpoint_download);
    InitializeCriticalSection(&g_csThreadCheckpoint);
#endif
}

void deinitialize_break_point_lock()
{
#if defined __GNUC__ || defined LINUX
    pthread_mutex_destroy(&g_mutexThreadCheckpoint_download);
    pthread_mutex_destroy(&g_mutexThreadCheckpoint);
#else
    DeleteCriticalSection(&g_csThreadCheckpoint_download);
    DeleteCriticalSection(&g_csThreadCheckpoint);
#endif

}

int getUploadFileSummary(upload_file_summary *pstUploadFileSummay, const char * file_name)
{
    if (file_name == NULL)
    {
        return -1;
    }
    else
    {
        int ret_stat = -1;

#if defined __GNUC__ || defined LINUX  
        struct stat statbuf;
        ret_stat = stat(file_name, &statbuf);
#else
        struct _stati64 statbuf;
        ret_stat = _stati64(file_name, &statbuf);
#endif
        if (ret_stat == -1)
        {
            return -1;
        }
        else
        {
            pstUploadFileSummay->fileSize = statbuf.st_size;
            pstUploadFileSummay->lastModify = statbuf.st_mtime;
        }
    }

    return 0;
}


void cleanUploadList(upload_file_part_info * uploadPartList)
{
    upload_file_part_info * ptrUploadPart = uploadPartList;
    upload_file_part_info * ptrUploadPartNext = uploadPartList;
    while (ptrUploadPart)
    {
        ptrUploadPartNext = ptrUploadPart->next;

        free(ptrUploadPart);
        ptrUploadPart = NULL;

        ptrUploadPart = ptrUploadPartNext;
    }
}


part_upload_status GetUploadStatusEnum(const char * strStatus)
{
    if (!strcmp(strStatus, "UPLOAD_NOTSTART"))
    {
        return UPLOAD_NOTSTART;
    }
    else if (!strcmp(strStatus, "UPLOADING"))
    {
        return UPLOADING;
    }
    else if (!strcmp(strStatus, "UPLOAD_FAILED"))
    {
        return UPLOAD_FAILED;
    }
    else if (!strcmp(strStatus, "UPLOAD_SUCCESS"))
    {
        return UPLOAD_SUCCESS;
    }
    else
    {
        return UPLOAD_NOTSTART;
    }
}

void parse_xmlnode_fileinfo_paramSet(upload_file_summary *pstUploadFileSummary,
    xmlNodePtr fileinfoNode, xmlChar *nodeContent)
{
    if (!xmlStrcmp(fileinfoNode->name, (xmlChar *)"filesize"))
    {
        pstUploadFileSummary->fileSize = parseUnsignedInt((char*)nodeContent);
    }
    else if (!xmlStrcmp((xmlChar *)fileinfoNode->name, (xmlChar *)"lastmodify"))
    {
        pstUploadFileSummary->lastModify = parseUnsignedInt((char*)nodeContent);
    }
    else if (!xmlStrcmp(fileinfoNode->name, (xmlChar *)"md5"))
    {
    }
    else if (!xmlStrcmp(fileinfoNode->name, (xmlChar *)"checksum"))
    {
        pstUploadFileSummary->fileCheckSum = (int)parseUnsignedInt((char*)nodeContent);
    }
}

errno_t parse_xmlnode_fileinfo_paramCpy(upload_file_summary *pstUploadFileSummary,
    xmlNodePtr fileinfoNode, xmlChar *nodeContent)
{
    errno_t err = EOK;
    if (!xmlStrcmp(fileinfoNode->name, (xmlChar *)"uploadid"))
    {
        err = memcpy_s(pstUploadFileSummary->upload_id, MAX_SIZE_UPLOADID, nodeContent, strlen((char*)nodeContent) + 1);
    }
    else if (!xmlStrcmp(fileinfoNode->name, (xmlChar *)"bucketname"))
    {
        err = memcpy_s(pstUploadFileSummary->bucket_name, MAX_BKTNAME_SIZE, nodeContent, strlen((char*)nodeContent) + 1);
    }
    else if (!xmlStrcmp(fileinfoNode->name, (xmlChar *)"key"))
    {
        err = memcpy_s(pstUploadFileSummary->key, MAX_KEY_SIZE, nodeContent, strlen((char*)nodeContent) + 1);
    }
    return err;
}

void parse_xmlnode_fileinfo(upload_file_summary * pstUploadFileSummary, xmlNodePtr fileinfoNode)
{
    while (fileinfoNode != NULL)
    {
        xmlChar *nodeContent = xmlNodeGetContent(fileinfoNode);
        errno_t err = EOK;

        parse_xmlnode_fileinfo_paramSet(pstUploadFileSummary, fileinfoNode, nodeContent);
        err = parse_xmlnode_fileinfo_paramCpy(pstUploadFileSummary, fileinfoNode, nodeContent);

        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "parse_xmlnode_fileinfo: memcpy_s failed !");
        }

        xmlFree(nodeContent);
        fileinfoNode = fileinfoNode->next;
    }

    return;
}


void parse_xmlnode_partsinfo_noEtag(upload_file_part_info *uploadPartNode, xmlNodePtr partinfoNode,
    xmlChar *nodeContent)
{
    if (!xmlStrcmp(partinfoNode->name, (xmlChar *)"partNum"))
    {
        uploadPartNode->part_num = (int)parseUnsignedInt((char*)nodeContent) - 1;
    }
    else if (!xmlStrcmp(partinfoNode->name, (xmlChar *)"startByte"))
    {
        uploadPartNode->start_byte = parseUnsignedInt((char*)nodeContent);
    }
    else if (!xmlStrcmp(partinfoNode->name, (xmlChar *)"partSize"))
    {
        uploadPartNode->part_size = parseUnsignedInt((char*)nodeContent);
    }
    else if (!xmlStrcmp(partinfoNode->name, (xmlChar*)"uploadStatus"))
    {
        uploadPartNode->uploadStatus = GetUploadStatusEnum((char*)nodeContent);
    }
}

int parse_xmlnode_partsinfo(upload_file_part_info ** uploadPartList, xmlNodePtr partNode, int *partCount)
{
    upload_file_part_info * pstUploadPart = NULL;
    upload_file_part_info * uploadPartNode = NULL;
    int partCountTmp = 0;
    xmlNodePtr partinfoNode = NULL;
    *uploadPartList = NULL;

    while (partNode)
    {
        if (strncmp((char*)partNode->name, "part", strlen("part")))
        {
            partNode = partNode->next;
            continue;
        }

        uploadPartNode = (upload_file_part_info *)malloc(sizeof(upload_file_part_info));
        if (uploadPartNode == NULL)
        {
            COMMLOG(OBS_LOGERROR, "int readCheckpointFile, malloc for uploadPartNode failed");
            cleanUploadList(*uploadPartList);
            partCountTmp = 0;
            *partCount = 0;
            return -1;
        }
        uploadPartNode->next = NULL;
        partCountTmp++;

        partinfoNode = partNode->xmlChildrenNode;
        while (partinfoNode != NULL)
        {
            xmlChar *nodeContent = xmlNodeGetContent(partinfoNode);
            COMMLOG(OBS_LOGINFO, "name:%s content %s\n", partinfoNode->name, nodeContent);
            if (!xmlStrcmp(partinfoNode->name, (xmlChar *)"etag"))
            {
                memset_s(uploadPartNode->etag, MAX_SIZE_ETAG, 0, MAX_SIZE_ETAG);

                errno_t err = EOK;
                err = memcpy_s(uploadPartNode->etag, MAX_SIZE_ETAG, nodeContent, strlen((char*)nodeContent));
                if (err != EOK)
                {
                    COMMLOG(OBS_LOGWARN, "parse_xmlnode_partsinfo: memcpy_s failed!\n");
                    free(uploadPartNode);
                    return -1;
                }
            }
            else {
                parse_xmlnode_partsinfo_noEtag(uploadPartNode, partinfoNode, nodeContent);
            }

            xmlFree(nodeContent);
            partinfoNode = partinfoNode->next;
        }

        uploadPartNode->prev = pstUploadPart;
        if (pstUploadPart == NULL)
        {
            pstUploadPart = uploadPartNode;
            *uploadPartList = uploadPartNode;
        }
        else
        {
            pstUploadPart->next = uploadPartNode;
            pstUploadPart = pstUploadPart->next;
        }

        partNode = partNode->next;
    }

    *partCount = partCountTmp;
    return 0;
}



int readCheckpointFile(upload_file_summary * pstUploadFileSummary,
    upload_file_part_info ** uploadPartList,
    int *partCount, char * file_name)
{
    xmlNodePtr curNode;
    xmlDocPtr doc;

    if (check_file_is_valid(file_name) == -1)
    {
        return -1;
    }

    curNode = get_xmlnode_from_file(file_name, &doc);
    if (NULL == curNode)
    {
        return -1;
    }

    *uploadPartList = NULL;
    *partCount = 0;

    if (xmlStrcmp((xmlChar *)curNode->name, BAD_CAST "uploadinfo"))
    {
        COMMLOG(OBS_LOGERROR, "document of the wrong type, root node != uploadinfo");
        xmlFreeDoc(doc);
        return -1;
    }
    curNode = curNode->xmlChildrenNode;
    while (curNode != NULL)
    {
        if (!xmlStrcmp(curNode->name, (xmlChar *)"fileinfo"))
        {
            xmlNodePtr fileinfoNode = curNode->xmlChildrenNode;
            parse_xmlnode_fileinfo(pstUploadFileSummary, fileinfoNode);
        }

        if (!xmlStrcmp(curNode->name, (xmlChar *)"partsinfo"))
        {
            xmlNodePtr partNode = curNode->xmlChildrenNode;
            if (-1 == parse_xmlnode_partsinfo(uploadPartList, partNode, partCount))
            {
                xmlFreeDoc(doc);
                return -1;
            }
        }

        curNode = curNode->next;
    }

    xmlFreeDoc(doc);
    return 0;
}


int isUploadFileChanged(upload_file_summary *pstNewSummary, upload_file_summary * pstOldSummary)
{
    if ((pstNewSummary->fileSize == pstOldSummary->fileSize)
        && (pstNewSummary->lastModify == pstOldSummary->lastModify))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


static obs_status listPartsCallback_Ex_Intern(obs_uploaded_parts_total_info* uploaded_parts,
    obs_list_parts *parts,
    void *callback_data)
{
    if (uploaded_parts || parts || callback_data)
    {
        return OBS_STATUS_OK;
    }
    return OBS_STATUS_OK;
}


int checkUploadFileInfo(upload_file_summary * pstUploadInfo, const obs_options *options, const char * keyIn)
{
    obs_list_parts_handler listPartsHandler =
    {
        { &ListPartsPropertiesCallback_Intern, &ListPartsCompleteCallback_Intern }, &listPartsCallback_Ex_Intern
    };

    lisPartResult stListPartResult;
    memset_s(&stListPartResult, sizeof(lisPartResult), 0, sizeof(lisPartResult));

    if ((strlen((const char *)pstUploadInfo->bucket_name) == 0)
        || strcmp((const char *)pstUploadInfo->bucket_name, options->bucket_options.bucket_name))
    {
        return 0;
    }
    if ((strlen((const char *)pstUploadInfo->key) == 0) || strcmp((const char *)pstUploadInfo->key, keyIn))
    {
        return 0;
    }

    if (strlen((const char *)pstUploadInfo->upload_id) == 0)
    {
        return 0;
    }
    list_part_info listpart;
    memset_s(&listpart, sizeof(list_part_info), 0, sizeof(list_part_info));
    listpart.max_parts = 100;
    listpart.upload_id = pstUploadInfo->upload_id;
    //call list parts here
    list_parts(options, (char *)pstUploadInfo->key, &listpart, &listPartsHandler, &stListPartResult);
    if (stListPartResult.retStatus == OBS_STATUS_OK)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


void abortMultipartUploadAndFree(const obs_options *options, char *key,
    const char * upload_id, const char *checkpointFilename, EN_FILE_ACTION enAction)
{
    int fdTemp = -1;
    if (strlen(upload_id) != 0)
    {
        obs_response_handler response_handler =
        {
            &ListPartsPropertiesCallback_Intern, &ListPartsCompleteCallback_Intern
        };
        lisPartResult stListPartResult;
        memset_s(&stListPartResult, sizeof(lisPartResult), 0, sizeof(lisPartResult));
        //abort the upload task here;
        abort_multi_part_upload(options, key, upload_id, &response_handler, &stListPartResult);
    }

    if ((checkpointFilename == NULL) || (enAction == DO_NOTHING))
    {
        return;
    }

    //clean up the checkpoint file here.
    if (enAction == CLEAN_FILE)
    {
#if defined __GNUC__ || defined LINUX
        fdTemp = open(checkpointFilename, O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

#else
        (void)_sopen_s(&fdTemp, checkpointFilename, _O_CREAT | _O_TRUNC, _SH_DENYNO,
            _S_IREAD | _S_IWRITE);
#endif 
    }
    else if (enAction == DELETE_FILE)
    {
        (void)remove(checkpointFilename);
    }

    if (fdTemp != -1)
    {
        close(fdTemp);
        fdTemp = -1;
    }
}


int setPartList(upload_file_summary *pstUploadFileSummaryNew, uint64_t uploadPartsize,
    upload_file_part_info ** uploadPartList, int *partCount, int isFirstTime)
{
    int partCountTemp = 0;
    upload_file_part_info * uploadPartListTemp = NULL;
    upload_file_part_info * pstuploadPartListTemp = NULL;
    upload_file_part_info * pstpUloadPartPrev = NULL;
    uint64_t lastPartSize = 0;
    int i = 0;

    upload_file_summary stUploadFileSummaryOld;
    memset_s(&stUploadFileSummaryOld, sizeof(upload_file_summary), 0, sizeof(upload_file_summary));

    // read the content from the check point file, and get the  parts list to upload this time
    if (!isFirstTime)
    {
        return 0;
    }
    // this means the client did not want to do checkpoint, or this is the first time to upload this file
    // or the upload file has been changed.
    //we should start to upload the whole file

    //calculate the total count of the parts and the last part size
    partCountTemp = (int)(pstUploadFileSummaryNew->fileSize / uploadPartsize);
    lastPartSize = pstUploadFileSummaryNew->fileSize % uploadPartsize;

    *partCount = partCountTemp;
    //malloc  and set for part list,set the parts info to the part list
    for (i = 0; i < partCountTemp; i++)
    {
        pstuploadPartListTemp = (upload_file_part_info*)malloc(sizeof(upload_file_part_info));
        if (pstuploadPartListTemp == NULL)
        {
            COMMLOG(OBS_LOGERROR, "in %s failed to malloc for uploadPartListTemp !", __FUNCTION__);
            cleanUploadList(uploadPartListTemp);
            uploadPartListTemp = NULL;
            return -1;
        }
        pstuploadPartListTemp->next = NULL;
        pstuploadPartListTemp->part_num = i;
        pstuploadPartListTemp->start_byte = uploadPartsize * i;
        pstuploadPartListTemp->part_size = uploadPartsize;
        memset_s(pstuploadPartListTemp->etag, sizeof(pstuploadPartListTemp->etag), 0, sizeof(pstuploadPartListTemp->etag));

        pstuploadPartListTemp->uploadStatus = UPLOAD_NOTSTART;
        if (i == 0)
        {
            pstuploadPartListTemp->prev = NULL;
            pstpUloadPartPrev = NULL;
            uploadPartListTemp = pstuploadPartListTemp;
        }
        else
        {
            pstuploadPartListTemp->prev = pstpUloadPartPrev;
            pstpUloadPartPrev->next = pstuploadPartListTemp;
        }

        pstpUloadPartPrev = pstuploadPartListTemp;
    }

    if (lastPartSize != 0)
    {
        pstuploadPartListTemp = (upload_file_part_info*)malloc(sizeof(upload_file_part_info));
        if (pstuploadPartListTemp == NULL)
        {
            COMMLOG(OBS_LOGERROR, "in %s failed to malloc for uploadPartListTemp !", __FUNCTION__);
            cleanUploadList(uploadPartListTemp);
            uploadPartListTemp = NULL;
            return -1;
        }
        pstuploadPartListTemp->prev = pstpUloadPartPrev;

        if (pstpUloadPartPrev)
        {
            pstpUloadPartPrev->next = pstuploadPartListTemp;
        }

        pstuploadPartListTemp->part_num = i;
        pstuploadPartListTemp->start_byte = uploadPartsize * i;
        pstuploadPartListTemp->part_size = lastPartSize;
        memset_s(pstuploadPartListTemp->etag, sizeof(pstuploadPartListTemp->etag), 0, sizeof(pstuploadPartListTemp->etag));

        pstuploadPartListTemp->uploadStatus = UPLOAD_NOTSTART;
        pstuploadPartListTemp->next = NULL;
        *partCount = partCountTemp + 1;
    }
    else
    {
        pstuploadPartListTemp = NULL;
    }
    *uploadPartList = uploadPartListTemp;
    return 0;
}


int writeCheckpointFile(upload_file_summary * pstUploadFileSummary,
    upload_file_part_info * uploadPartList,
    int partCount, const char * file_name)
{
    char str_content[512] = { 0 };
    xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");
    char contentBuff[64];
    int i = 0;
    int nRel = -1;
    upload_file_part_info * ptrUploadPart = NULL;

    //create the root node    
    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"uploadinfo");

    xmlNodePtr node_fileinfo = xmlNewNode(NULL, BAD_CAST"fileinfo");
    xmlNodePtr node_partsinfo = xmlNewNode(NULL, BAD_CAST"partsinfo");
    //set the root node <uploadinfo>    
    xmlDocSetRootElement(doc, root_node);
    //add <fileinfo> and <partsinfo> under <uploadinfo>    

    int ret = sprintf_s(str_content, ARRAY_LENGTH_512, "%llu", (long long unsigned int)pstUploadFileSummary->fileSize);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);

    //add <fileinfo> node under <uploadinfo>    
    xmlAddChild(root_node, node_fileinfo);
    //add <filesize> under <fileinfo>    
    ret = sprintf_s(str_content, ARRAY_LENGTH_512, "%llu", (long long unsigned int)pstUploadFileSummary->fileSize);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "filesize", BAD_CAST str_content);

    //add <lastmodify> under <fileinfo>    
    ret = sprintf_s(str_content, ARRAY_LENGTH_512, "%llu", (long long unsigned int)pstUploadFileSummary->lastModify);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "lastmodify", BAD_CAST str_content);

    //add <fileMd5> under <fileinfo>    
    //snprintf(str_content,512,"%d",pstUploadFileSummary->fileMd5);    
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "md5", BAD_CAST "");

    //add <fileMd5> under <fileinfo>    
    ret = sprintf_s(str_content, ARRAY_LENGTH_512, "%d", pstUploadFileSummary->fileCheckSum);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "checksum", BAD_CAST str_content);

    //add <uploadid> under <fileinfo>    
    ret = sprintf_s(str_content, ARRAY_LENGTH_512, "%s", pstUploadFileSummary->upload_id);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "uploadid", BAD_CAST str_content);

    // add <bucketname> under <fileinfo>   
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "bucketname", BAD_CAST pstUploadFileSummary->bucket_name);

    //add <key> under <fileinfo>  
    xmlNewTextChild(node_fileinfo, NULL, BAD_CAST "key", BAD_CAST pstUploadFileSummary->key);

    //add <partsinfo> under <uploadinfo>
    xmlAddChild(root_node, node_partsinfo);


    if ((partCount) && (uploadPartList == NULL))
    {
        xmlFreeDoc(doc);
        return -1;
    }

    ptrUploadPart = uploadPartList;
    for (i = 0; i < partCount; i++)
    {
        xmlNodePtr partNode = NULL;
        ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "part%d", i + 1);
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        partNode = xmlNewNode(NULL, BAD_CAST contentBuff);//here  contentBuff indicat the name of the xml node

        ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%d", i + 1);
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        xmlNewChild(partNode, NULL, BAD_CAST "partNum", BAD_CAST contentBuff);

        if (ptrUploadPart != NULL)
        {
            ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%s", ptrUploadPart->etag);
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            xmlNewChild(partNode, NULL, BAD_CAST "etag", BAD_CAST contentBuff);

            ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%llu", (long long unsigned int)ptrUploadPart->start_byte);
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            xmlNewChild(partNode, NULL, BAD_CAST "startByte", BAD_CAST contentBuff);

            ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%llu", (long long unsigned int)ptrUploadPart->part_size);
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            xmlNewChild(partNode, NULL, BAD_CAST "partSize", BAD_CAST contentBuff);

            ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%s", g_uploadStatus[ptrUploadPart->uploadStatus]);
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            xmlNewChild(partNode, NULL, BAD_CAST "uploadStatus", BAD_CAST contentBuff);
            ptrUploadPart = ptrUploadPart->next;
        }
        xmlAddChild(node_partsinfo, partNode);
    }


    //store the xml file    
    nRel = xmlSaveFile(file_name, doc);
    if (nRel != -1) {
        COMMLOG(OBS_LOGERROR, "one xml doc is written in %d bytes\n", nRel);
    }

    //free all the node int the doc    
    xmlFreeDoc(doc);
    return 0;
}


void DividUploadPartListSetNode(upload_file_part_info *pstSrcListNode, upload_file_part_info **pstTmpDoneList,
    upload_file_part_info **pstTmpNotDoneList)
{
    upload_file_part_info *pstTmpDoneNode = NULL;
    upload_file_part_info *pstTmpNotDoneNode = NULL;
    upload_file_part_info *pstTmpList = NULL;
    upload_file_part_info *pstTmpNode = NULL;
    while (pstSrcListNode)
    {
        //1.set the node and list to process, store in pstTmpNode and pstTmpList
        if (pstSrcListNode->uploadStatus == UPLOAD_SUCCESS)
        {
            pstTmpList = *pstTmpDoneList;
            pstTmpNode = pstTmpDoneNode;
        }
        else
        {
            pstTmpList = *pstTmpNotDoneList;
            pstTmpNode = pstTmpNotDoneNode;
        }

        //2,add the node from listSrc after pstTmpNode
        if (pstTmpList == NULL)
        {
            pstTmpNode = pstSrcListNode;
            pstTmpList = pstSrcListNode;
            pstTmpNode->prev = NULL;
        }
        else
        {
            pstTmpNode->next = pstSrcListNode;
            pstSrcListNode->prev = pstTmpNode;
            pstTmpNode = pstTmpNode->next;
        }

        //3,set the pstTmpNode and pstTmpList, back to done list or not done list
        if (pstSrcListNode->uploadStatus == UPLOAD_SUCCESS)
        {
            *pstTmpDoneList = pstTmpList;
            pstTmpDoneNode = pstTmpNode;
        }
        else
        {
            *pstTmpNotDoneList = pstTmpList;
            pstTmpNotDoneNode = pstTmpNode;
        }
        pstSrcListNode = pstSrcListNode->next;
        pstTmpNode->next = NULL;
    }
}

int DividUploadPartList(upload_file_part_info * listSrc, upload_file_part_info ** listDone, upload_file_part_info ** listNotDone)
{
    //we assume that the listSrc is sorted in ascending order
    upload_file_part_info *pstSrcListNode = listSrc;
    upload_file_part_info *pstTmpDoneList = NULL;
    upload_file_part_info *pstTmpNotDoneList = NULL;


    DividUploadPartListSetNode(pstSrcListNode, &pstTmpDoneList,
        &pstTmpNotDoneList);
    *listDone = pstTmpDoneList;
    *listNotDone = pstTmpNotDoneList;
    return 0;

}


int addUploadPartNodeToListMiddle(upload_file_part_info **pstTempNode, upload_file_part_info *partNode)
{
    upload_file_part_info *pMiddleTempNode = *pstTempNode;
    while (pMiddleTempNode)
    {
        if (pMiddleTempNode->part_num > partNode->part_num)
        {
            partNode->next = pMiddleTempNode;
            partNode->prev = pMiddleTempNode->prev;
            pMiddleTempNode->prev->next = partNode;
            pMiddleTempNode->prev = partNode;
            return 1;
        }
        else
        {
            if (pMiddleTempNode->next != NULL)//avoid moving to hte next of the last node
            {
                pMiddleTempNode = pMiddleTempNode->next;
                *pstTempNode = pMiddleTempNode;
            }
            else
            {
                break;
            }
        }
    }
    return 0;
}

void addUploadPartNodeToList(upload_file_part_info  **listToAdd, upload_file_part_info *partNode)
{
    upload_file_part_info * pstTempNode = *listToAdd;
    partNode->next = NULL;
    partNode->prev = NULL;

    //need to add before the first node.
    if (pstTempNode == NULL)
    {
        *listToAdd = partNode;
        return;
    }
    if (pstTempNode->part_num > partNode->part_num)
    {
        partNode->next = pstTempNode;
        pstTempNode->prev = partNode;

        *listToAdd = partNode;
        return;
    }
    //need to add at the middle of the list.
    if (addUploadPartNodeToListMiddle(&pstTempNode, partNode))
        return;
    // add the nod after the last node
    if ((pstTempNode != NULL) && (pstTempNode->next == NULL))
    {
        pstTempNode->next = partNode;
        partNode->prev = pstTempNode;
    }
}

int GetUploadPartListToProcess(upload_file_part_info **listDone, upload_file_part_info ** listNotDones,
    int partCountIn, int * partCountOut, int task_num)
{
    int i = 0;
    int nodeCoutNotDone = 0;
    upload_file_part_info * pstTempNodeNotDone = *listNotDones;
    upload_file_part_info * pstTempNodeNotDoneNext = *listNotDones;

    for (i = 0; i < partCountIn; i++)
    {
        if (pstTempNodeNotDone)
        {
            pstTempNodeNotDoneNext = pstTempNodeNotDone->next;
            addUploadPartNodeToList(listDone, pstTempNodeNotDone);

            pstTempNodeNotDone = pstTempNodeNotDoneNext;
            if (pstTempNodeNotDone)
            {
                pstTempNodeNotDoneNext = pstTempNodeNotDone->next;
            }
        }
    }

    *listNotDones = pstTempNodeNotDone;
    pstTempNodeNotDone = *listNotDones;

    while (pstTempNodeNotDone)
    {
        nodeCoutNotDone++;
        pstTempNodeNotDone = pstTempNodeNotDone->next;
    }

    if (nodeCoutNotDone > MAX_THREAD_NUM)
    {
        *partCountOut = MAX_THREAD_NUM;
    }
    else
    {
        *partCountOut = nodeCoutNotDone;
    }

    if (task_num < *partCountOut)
    {
        *partCountOut = task_num;
    }

    return 0;

}


static obs_status uploadPartCompletePropertiesCallback
(const obs_response_properties *properties, void *callback_data)
{
    upload_file_callback_data * cbd = (upload_file_callback_data *)callback_data;

    if (properties->etag)
    {
        errno_t err = strcpy_s(cbd->stUploadFilePartInfo->etag, MAX_SIZE_ETAG, properties->etag);
        CheckAndLogNoneZero(err, "strcpy_s", __FUNCTION__, __LINE__);
    }


    if (cbd->enableCheckPoint)
    {
        char pathToUpdate[1024];

        if (properties->etag)
        {
            int ret = sprintf_s(pathToUpdate, ARRAY_LENGTH_1024, "%s%d/%s", "uploadinfo/partsinfo/part", cbd->part_num + 1, "etag");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
#if defined(WIN32)
            EnterCriticalSection(&g_csThreadCheckpoint);
#endif

#if defined __GNUC__ || defined LINUX
            pthread_mutex_lock(&g_mutexThreadCheckpoint);
#endif
            ret = updateCheckPoint(pathToUpdate, properties->etag, cbd->checkpointFilename);
            if (ret == -1) {
                COMMLOG(OBS_LOGWARN, "Failed to update checkpoint in function: %s.", __FUNCTION__);
            }
#if defined(WIN32)
            LeaveCriticalSection(&g_csThreadCheckpoint);
#endif

#if defined __GNUC__ || defined LINUX
            pthread_mutex_unlock(&g_mutexThreadCheckpoint);
#endif
        }
    }


    if (cbd->respHandler->complete_callback)
    {
        (cbd->respHandler->properties_callback)(properties,
            cbd->callbackDataIn);
    }
    return OBS_STATUS_OK;
}

static void  uploadPartCompleteCallback(obs_status status,
    const obs_error_details *error,
    void *callback_data)
{
    upload_file_callback_data * cbd = (upload_file_callback_data *)callback_data;

    if (status == OBS_STATUS_OK)
    {
        cbd->stUploadFilePartInfo->uploadStatus = UPLOAD_SUCCESS;
    }
    else
    {
        cbd->stUploadFilePartInfo->uploadStatus = UPLOAD_FAILED;
    }
    if (cbd->enableCheckPoint)
    {
        char pathToUpdate[1024];
        char contentToSet[32];

        int ret = sprintf_s(pathToUpdate, ARRAY_LENGTH_1024, "%s%d/%s", "uploadinfo/partsinfo/part", cbd->part_num + 1, "uploadStatus");
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        if (status == OBS_STATUS_OK)
        {
            ret = sprintf_s(contentToSet, ARRAY_LENGTH_32, "%s", "UPLOAD_SUCCESS");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        }
        else
        {
            ret = sprintf_s(contentToSet, ARRAY_LENGTH_32, "%s", "UPLOAD_FAILED");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        }
#if defined(WIN32)
        EnterCriticalSection(&g_csThreadCheckpoint);
#endif

#if defined __GNUC__ || defined LINUX
        pthread_mutex_lock(&g_mutexThreadCheckpoint);
#endif

        ret = updateCheckPoint(pathToUpdate, contentToSet, cbd->checkpointFilename);
        if (ret == -1) {
            COMMLOG(OBS_LOGWARN, "Failed to update checkpoint in function: %s.", __FUNCTION__);
        }
#if defined(WIN32)
        LeaveCriticalSection(&g_csThreadCheckpoint);
#endif

#if defined __GNUC__ || defined LINUX
        pthread_mutex_unlock(&g_mutexThreadCheckpoint);
#endif

    }

    if (cbd->respHandler->complete_callback)
    {
        (cbd->respHandler->complete_callback)(status,
            error, cbd->callbackDataIn);
    }
    return;
}

static int  uploadPartCallback(int buffer_size, char * buffer, void *callback_data)
{
    upload_file_callback_data * cbd = (upload_file_callback_data *)callback_data;
    int fdUpload = cbd->fdUploadFile;
    int bytesRead = 0;
    if (fdUpload == -1)
    {
        return -1;
    }
    else
    {
        if (cbd->bytesRemaining)
        {
            int toRead = (int)((cbd->bytesRemaining > (unsigned)buffer_size) ?
                (unsigned)buffer_size : cbd->bytesRemaining);

            bytesRead = read(fdUpload, buffer, toRead);
            cbd->bytesRemaining -= bytesRead;
        }
    }
    return bytesRead;

}


static void uploadProgressCallback(uint64_t ulnow, uint64_t utotal, void *callback_data){
    upload_file_callback_data * cbd =  (upload_file_callback_data *)callback_data;

    upload_file_progress_info *progressInfo = cbd->progressInfo;
    uint64_t total_upload = progressInfo->uploadedSize;
    if (ulnow == 0) {
        return;
    }
    if (progressInfo == NULL) {
        COMMLOG(OBS_LOGWARN, "progressInfo is null");
        return;
    }

    COMMLOG(OBS_LOGDEBUG, "uploadProgressCallback ulnow %lu  %p %p %lu\n", ulnow, callback_data, cbd->progressCallback, progressInfo->totalFileSize);
    if (progressInfo->progressArr[progressInfo->index] > ulnow) {
        COMMLOG(OBS_LOGWARN, "progressnow greatter than ulnow");
        return;
    }

    progressInfo->progressArr[progressInfo->index] = ulnow;
    for (int i = 0; i < progressInfo->arrSize; i++) {
        total_upload += progressInfo->progressArr[i];
    }

    COMMLOG(OBS_LOGDEBUG, "uploadProgressCallback total_upload %lu\n", total_upload);
    if(cbd->progressCallback)
    {
        (cbd->progressCallback)((double)(100.0*total_upload/progressInfo->totalFileSize), total_upload, progressInfo->totalFileSize, cbd->callbackDataIn);
    } else {
        COMMLOG(OBS_LOGWARN, "user not set progressCallback , cb is null");
    }
}

#if defined (WIN32)
unsigned __stdcall UploadThreadProc_win32(void* param)
{
    upload_file_for_win32 *tmp = (upload_file_for_win32*)param;
    upload_file_proc_data *pstPara = tmp->upload_data;
    char * uploadFileName = pstPara->stUploadParams->fileNameUpload;
    uint64_t start_byte = pstPara->stUploadFilePartInfo->start_byte;
    uint64_t part_size = pstPara->stUploadFilePartInfo->part_size;
    int part_num = pstPara->stUploadFilePartInfo->part_num;
    server_side_encryption_params * pstEncrypParam = NULL;
    char *szUpload = NULL;
    pstPara->thread_start = 1;

    int fd = -1;
    uint64_t curPos;
    HANDLE arrEvent = tmp->hEvent;
    ResetEvent(arrEvent);
    while (1) {
        DWORD waitRet = WaitForSingleObject(arrEvent, SLEEP_TIMES_FOR_WAIT);
        if (waitRet == WAIT_OBJECT_0) {
            COMMLOG(OBS_LOGINFO, "%#p is exit from this thread\n", arrEvent);
            if (fd != -1) {
                _close(fd);
                fd = -1;
            }
            break;
        } else {
            (void)_sopen_s(&fd, uploadFileName, _O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD);

            if (fd == -1)
            {
                COMMLOG(OBS_LOGINFO, "open upload file failed, partnum[%d]\n", part_num);
            }
            else
            {
                obs_upload_handler uploadResponseHandler =
                {
                    {&uploadPartCompletePropertiesCallback,
                    &uploadPartCompleteCallback},
                    &uploadPartCallback,
                    &uploadProgressCallback
                };
                upload_file_callback_data  data;
                obs_put_properties stPutProperties;
                obs_name_value metaProperties[OBS_MAX_METADATA_COUNT] = { 0 };

                curPos = _lseeki64(fd, start_byte, SEEK_SET);

                szUpload = pstPara->stUploadParams->upload_id;
                memset_s(&data, sizeof(upload_file_callback_data), 0, sizeof(upload_file_callback_data));


                data.bytesRemaining = pstPara->stUploadFilePartInfo->part_size;
                data.totalBytes = pstPara->stUploadFilePartInfo->part_size;
                data.callbackDataIn = pstPara->callBackData;
                data.checkpointFilename = pstPara->stUploadParams->fileNameCheckpoint;
                data.enableCheckPoint = pstPara->stUploadParams->enable_check_point;
                data.fdUploadFile = fd;
                data.part_num = part_num;
                data.respHandler = pstPara->stUploadParams->response_handler;
                data.taskHandler = 0;
                data.stUploadFilePartInfo = pstPara->stUploadFilePartInfo;
                data.progressCallback = pstPara->stUploadParams->progress_callback;
                data.progressInfo = &pstPara->stUploadProgressInfo;

                pstEncrypParam = pstPara->stUploadParams->pstServerSideEncryptionParams;
                if (data.enableCheckPoint == 1)
                {
                    char pathToUpdate[ARRAY_LENGTH_2014];
                    char contentToSet[ARRAY_LENGTH_64];

                    sprintf_s(pathToUpdate, ARRAY_LENGTH_2014, "%s%u/%s", "uploadinfo/partsinfo/part", part_num + 1, "uploadStatus");
                    sprintf_s(contentToSet, ARRAY_LENGTH_64, "%s", "UPLOADING");
                    EnterCriticalSection(&g_csThreadCheckpoint);
                    updateCheckPoint(pathToUpdate, contentToSet, pstPara->stUploadParams->fileNameCheckpoint);
                    LeaveCriticalSection(&g_csThreadCheckpoint);
                }
                memset_s(&stPutProperties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));

                stPutProperties.expires = -1;
                stPutProperties.canned_acl = OBS_CANNED_ACL_PUBLIC_READ_WRITE;
                stPutProperties.meta_data = metaProperties;
                pstPara->stUploadFilePartInfo->uploadStatus = UPLOADING;
                if ((pstEncrypParam) && (pstEncrypParam->encryption_type == OBS_ENCRYPTION_KMS))
                {
                    pstEncrypParam = NULL;
                }
                obs_upload_part_info upload_part_info;
                memset_s(&upload_part_info, sizeof(obs_upload_part_info), 0, sizeof(obs_upload_part_info));

                upload_part_info.part_number = part_num + 1;
                upload_part_info.upload_id = szUpload;
                upload_part_info.arrEvent = arrEvent;
                upload_part(pstPara->stUploadParams->options, pstPara->stUploadParams->objectName,
                    &upload_part_info, part_size, &stPutProperties, pstEncrypParam, &uploadResponseHandler, &data);
            }
            if (fd != -1)
            {
                _close(fd);
                fd = -1;
            }
            pstPara->thread_end = 1;
            COMMLOG(OBS_LOGINFO, "has been called here. upload success. uploadFileName:%s, start_byte:%d, part_size:%d, part_num:%d. \n",
                uploadFileName, start_byte, part_size, part_num);
            return 1;
        }
    }

}
#endif

#if defined __GNUC__ || defined LINUX
void cleanup_fd(void *arg)
{
    int *fd = (int*)arg;
    if (*fd != -1)
    {
        close(*fd);
        *fd = -1;
    }
}

void *UploadThreadProc_linux(void* param)
{
    int oldstate = 0;
    int oldtype = 0;

    upload_file_proc_data * pstPara = (upload_file_proc_data *)param;
    char * uploadFileName = pstPara->stUploadParams->fileNameUpload;
    uint64_t start_byte = pstPara->stUploadFilePartInfo->start_byte;
    uint64_t part_size = pstPara->stUploadFilePartInfo->part_size;
    int part_num = pstPara->stUploadFilePartInfo->part_num;
    server_side_encryption_params * pstEncrypParam = NULL;
    char *szUpload = NULL;
    pstPara->thread_start = 1;

    int fd = -1;
    pthread_cleanup_push(pthread_mutex_unlock, (void*)&g_mutexThreadCheckpoint);
    pthread_cleanup_push(cleanup_fd, (void*)&fd);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

    fd = open(uploadFileName, O_RDONLY);
    if (fd == -1)
    {
        COMMLOG(OBS_LOGINFO, "open upload file failed, partnum[%d]\n", part_num);
    }
    else
    {
        obs_upload_handler uploadResponseHandler =
        {
            {&uploadPartCompletePropertiesCallback,
            &uploadPartCompleteCallback},
            &uploadPartCallback,
            &uploadProgressCallback
        };
        upload_file_callback_data  data;
        obs_put_properties stPutProperties;
        obs_name_value metaProperties[OBS_MAX_METADATA_COUNT];
        memset_s(metaProperties, sizeof(obs_name_value)*OBS_MAX_METADATA_COUNT, 0, sizeof(obs_name_value)*OBS_MAX_METADATA_COUNT);

        lseek(fd, (long long int)start_byte, SEEK_SET);
        szUpload = pstPara->stUploadParams->upload_id;

        memset_s(&data, sizeof(upload_file_callback_data), 0, sizeof(upload_file_callback_data));
        data.bytesRemaining = pstPara->stUploadFilePartInfo->part_size;
        data.totalBytes = pstPara->stUploadFilePartInfo->part_size;
        data.callbackDataIn = pstPara->callBackData;
        data.checkpointFilename = pstPara->stUploadParams->fileNameCheckpoint;
        data.enableCheckPoint = pstPara->stUploadParams->enable_check_point;
        data.fdUploadFile = fd;
        data.part_num = part_num;
        data.respHandler = pstPara->stUploadParams->response_handler;
        data.taskHandler = 0;
        data.stUploadFilePartInfo = pstPara->stUploadFilePartInfo;
        data.progressCallback = pstPara->stUploadParams->progress_callback;
        data.progressInfo = &pstPara->stUploadProgressInfo;

        pstEncrypParam = pstPara->stUploadParams->pstServerSideEncryptionParams;

        if (data.enableCheckPoint == 1)
        {
            char pathToUpdate[1024];
            char contentToSet[32];

            sprintf_s(pathToUpdate, ARRAY_LENGTH_1024, "%s%d/%s", "uploadinfo/partsinfo/part", part_num, "uploadStatus");
            sprintf_s(contentToSet, ARRAY_LENGTH_32, "%s", "UPLOADING");
            pthread_mutex_lock(&g_mutexThreadCheckpoint);
            updateCheckPoint(pathToUpdate, contentToSet, pstPara->stUploadParams->fileNameCheckpoint);
            pthread_mutex_unlock(&g_mutexThreadCheckpoint);

        }
        memset_s(&stPutProperties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));


        stPutProperties.expires = -1;
        stPutProperties.canned_acl = OBS_CANNED_ACL_PUBLIC_READ_WRITE;
        stPutProperties.meta_data = metaProperties;
        pstPara->stUploadFilePartInfo->uploadStatus = UPLOADING;
        if ((pstEncrypParam) && (pstEncrypParam->encryption_type == OBS_ENCRYPTION_KMS))
        {
            pstEncrypParam = NULL;
        }
        obs_upload_part_info upload_part_info;
        memset_s(&upload_part_info, sizeof(obs_upload_part_info), 0, sizeof(obs_upload_part_info));

        upload_part_info.part_number = part_num + 1;
        upload_part_info.upload_id = szUpload;
        upload_part(pstPara->stUploadParams->options, pstPara->stUploadParams->objectName,
            &upload_part_info, part_size, &stPutProperties, pstEncrypParam, &uploadResponseHandler, &data);
    }

    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
    pstPara->thread_end = 1;
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);

    return NULL;
}
#endif

#ifdef WIN32
void observe_return_value_for_WaitForSingleObject(DWORD waitRet, HANDLE arrHandle)
{
    switch (waitRet) {
        case WAIT_TIMEOUT:
            COMMLOG(OBS_LOGINFO, "%#p not call, TIMEOUT", arrHandle);
            break;
        case WAIT_ABANDONED:
            COMMLOG(OBS_LOGINFO, "%#p Mutex object", arrHandle);
            break;
        case WAIT_OBJECT_0:
            COMMLOG(OBS_LOGINFO, "%#p been called object, will exit thread", arrHandle);
            break;
        default:
            COMMLOG(OBS_LOGINFO, "%#p WAIT_FAILED.", arrHandle);
            break;
    }
}

void startUploadThreads_win32(upload_file_proc_data * uploadFileProcDataList,
                              upload_file_part_info *pstOnePartInfo,
                              int partCount, void* callback_data, upload_params *pstUploadParams)
{
    unsigned  uiThread2ID;
    DWORD   dwExitCode = 0;
    DWORD   dwExitCode1 = 0;
    int i = 0;
    int err = 1;
    HANDLE * arrHandle = (HANDLE *)malloc(sizeof(HANDLE)*partCount);
    HANDLE * arrEvent = (HANDLE *)malloc(sizeof(HANDLE)*partCount);

    upload_file_for_win32 *uploadFileDataForWin32 =
        (upload_file_for_win32*)malloc(sizeof(upload_file_for_win32) * partCount);
    if (uploadFileDataForWin32 == NULL || arrHandle == NULL || arrEvent == NULL) {
        COMMLOG(OBS_LOGERROR, "malloc failed! uploadFileDataForWin32:%p, arrHandle:%p, arrEvent:%p\n",
            uploadFileDataForWin32, arrHandle, arrEvent);
        if (pstUploadParams->response_handler->complete_callback) {
            (pstUploadParams->response_handler->complete_callback)(OBS_STATUS_InternalError, 0, callback_data);
        }
        return;
    }

    for (i = 0; i < partCount; i++)
    {
        arrEvent[i] = (HANDLE)CreateEvent(NULL, TRUE, FALSE, NULL);
        uploadFileDataForWin32[i].upload_data = &uploadFileProcDataList[i];
        uploadFileDataForWin32[i].hEvent = arrEvent[i];
        arrHandle[i] = (HANDLE)_beginthreadex(NULL, 0, UploadThreadProc_win32,
                                              &uploadFileDataForWin32[i], CREATE_SUSPENDED, &uiThread2ID);
        if (arrHandle[i] == 0) {
            GetExitCodeThread(arrHandle[i], &dwExitCode);
            COMMLOG(OBS_LOGERROR, "create thread i[%d] failed exit code = %u \n", i, dwExitCode);
        }
        pstOnePartInfo->threadHandler = arrHandle[i];
        pstOnePartInfo = pstOnePartInfo->next;
    }
    for (i = 0; i < partCount; i++)
    {
        ResumeThread(arrHandle[i]);
    }

    for (i = 0; i < partCount; i++) {
        while (1) {
            if(*(uploadFileProcDataList[i].stUploadParams->pause_upload_flag) == 1) {
                SetEvent(arrEvent[i]);
                DWORD waitRet = WaitForSingleObject(arrHandle[i], INFINITE);
                observe_return_value_for_WaitForSingleObject(waitRet, arrHandle[i]);

                if (uploadFileProcDataList[i].stUploadParams->enable_check_point == 1) {
                    char pathToUpdate[ARRAY_LENGTH_1024];
                    char contentToSet[ARRAY_LENGTH_32];
                    int ret = sprintf_s(pathToUpdate,ARRAY_LENGTH_1024,"%s%d/%s","uploadinfo/partsinfo/part", i + 1, "uploadStatus");
                    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
                    ret = sprintf_s(contentToSet,ARRAY_LENGTH_32,"%s","UPLOAD_FAILED");
                    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
                    EnterCriticalSection(&g_csThreadCheckpoint);
                    updateCheckPoint(pathToUpdate, contentToSet, uploadFileProcDataList[i].stUploadParams->fileNameCheckpoint);
                    LeaveCriticalSection(&g_csThreadCheckpoint);
                }

                uploadFileProcDataList[i].stUploadFilePartInfo->uploadStatus = UPLOAD_FAILED;
                COMMLOG(OBS_LOGERROR, "task i:%d is aborted by user!", i);
                obs_response_complete_callback *complete_callback = uploadFileProcDataList[i].stUploadParams->response_handler->complete_callback;
                if (complete_callback) {
                    (complete_callback)(OBS_STATUS_AbortedByCallback, 0, uploadFileProcDataList[i].callBackData);
                }
                break;
            } else if(uploadFileProcDataList[i].thread_start == 1 && uploadFileProcDataList[i].thread_end == 1) {
                err = WaitForSingleObject(arrHandle[i],INFINITE);
                if(err != 0) {
                    COMMLOG(OBS_LOGINFO, "exit thread failed i[%d]\n",i);
                }
                break;
            } else {
                Sleep(SLEEP_TIMES_FOR_WIN32);
            }
        }
    }

    for (i = 0; i < partCount; i++)
    {
        CloseHandle(arrHandle[i]);
        CloseHandle(arrEvent[i]);
    }

    if (arrHandle)
    {
        free(arrHandle);
        arrHandle = NULL;
    }
    if (arrEvent) {
        free(arrEvent);
        arrEvent = NULL;
    }
}
#endif // WIN32

#if defined __GNUC__ || defined LINUX
void startUploadThreads_linux(upload_params * pstUploadParams, int partCount, void* callback_data,
    upload_file_proc_data * uploadFileProcDataList)
{
    int i = 0;
    int err;
    pthread_t * arrThread = (pthread_t *)malloc(sizeof(pthread_t) * partCount);
    if (arrThread == NULL) {
        COMMLOG(OBS_LOGWARN, "startUploadThreads: pthread_t malloc failed!\n");
        if (pstUploadParams->response_handler->complete_callback) {
            (pstUploadParams->response_handler->complete_callback)(OBS_STATUS_InternalError, 0, callback_data);
        }
        return;
    }

    for (i = 0; i < partCount; i++) {
        err = pthread_create(&arrThread[i], NULL, UploadThreadProc_linux, (void *)&uploadFileProcDataList[i]);
        if (err != 0) {
            COMMLOG(OBS_LOGINFO, "create thread failed i[%d]\n", i);
        }
    }

    for (i = 0; i < partCount; i++) {
        while (1) {
            if(*(pstUploadParams->pause_upload_flag) == 1) {
                pthread_mutex_lock(&g_mutexThreadCheckpoint);
                err = pthread_cancel(arrThread[i]);
                pthread_mutex_unlock(&g_mutexThreadCheckpoint);
                if(err != 0) {
                    COMMLOG(OBS_LOGINFO, "cancel thread failed i[%d]\n",i);
                }

                err = pthread_join(arrThread[i], NULL);
                if(err != 0) {
                    COMMLOG(OBS_LOGINFO, "join thread failed i[%d]\n",i);
                }

                if (uploadFileProcDataList[i].stUploadParams->enable_check_point == 1) {
                    char pathToUpdate[ARRAY_LENGTH_1024];
                    char contentToSet[ARRAY_LENGTH_32];
                    int ret = sprintf_s(pathToUpdate,ARRAY_LENGTH_1024,"%s%d/%s","uploadinfo/partsinfo/part", i + 1, "uploadStatus");
                    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
                    ret = sprintf_s(contentToSet,ARRAY_LENGTH_32,"%s","UPLOAD_FAILED");
                    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
                    pthread_mutex_lock(&g_mutexThreadCheckpoint);
                    ret = updateCheckPoint(pathToUpdate, contentToSet, uploadFileProcDataList[i].stUploadParams->fileNameCheckpoint);
                    if (ret == -1) {
                        COMMLOG(OBS_LOGWARN, "Failed to update checkpoint in function: %s.", __FUNCTION__);
                    }
                    pthread_mutex_unlock(&g_mutexThreadCheckpoint);
                }

                uploadFileProcDataList[i].stUploadFilePartInfo->uploadStatus = UPLOAD_FAILED;
                COMMLOG(OBS_LOGERROR, "task i:%d is aborted by user!", i);
                if (pstUploadParams->response_handler->complete_callback) {
                    (pstUploadParams->response_handler->complete_callback)(OBS_STATUS_AbortedByCallback, 0, callback_data);
                }
                break;
            } else if(uploadFileProcDataList[i].thread_start == 1 && uploadFileProcDataList[i].thread_end == 1) {
                err = pthread_join(arrThread[i], NULL);
                if(err != 0) {
                    COMMLOG(OBS_LOGINFO, "join thread failed i[%d]\n",i);
                }
                break;
            } else {
                sleep(SLEEP_TIMES_FOR_LINUX);
            }
        }
    }

    if (arrThread) {
        free(arrThread);
        arrThread = NULL;
    }
}
#endif

void startUploadThreads(upload_params * pstUploadParams,
    upload_file_part_info * uploadFilePartInfoList,
    int partCount, void* callback_data)
{
    int i = 0;
    upload_file_proc_data * uploadFileProcDataList = (upload_file_proc_data *)malloc(sizeof(upload_file_proc_data)*partCount);
    if (uploadFileProcDataList == NULL) {
        COMMLOG(OBS_LOGWARN, "startUploadThreads: uploadFileProcDataList malloc failed!\n");
        if (pstUploadParams->response_handler->complete_callback) {
            (pstUploadParams->response_handler->complete_callback)(OBS_STATUS_InternalError, 0, callback_data);
        }
        return;
    }

    uint64_t *uploadFileProgress = (uint64_t *)malloc(sizeof(uint64_t)*partCount);
    if (uploadFileProgress == NULL) {
        COMMLOG(OBS_LOGWARN, "startUploadThreads: uploadFileProgress malloc failed!\n");
        if (pstUploadParams->response_handler->complete_callback) {
            (pstUploadParams->response_handler->complete_callback)(OBS_STATUS_InternalError, 0, callback_data);
        }
        return;
    }
    memset_s(uploadFileProgress, sizeof(uint64_t)*partCount, 0, sizeof(uint64_t)*partCount);

    upload_file_proc_data * pstUploadFileProcData = uploadFileProcDataList;
    upload_file_part_info *pstOnePartInfo = uploadFilePartInfoList;
    memset_s(uploadFileProcDataList, sizeof(upload_file_proc_data)*partCount, 0, sizeof(upload_file_proc_data)*partCount);

    for (i = 0; i < partCount; i++) {
        pstUploadFileProcData[i].stUploadParams = pstUploadParams;
        pstUploadFileProcData[i].stUploadFilePartInfo = pstOnePartInfo;
        pstUploadFileProcData[i].callBackData = callback_data;
        pstUploadFileProcData[i].thread_start = 0;
        pstUploadFileProcData[i].thread_end = 0;

        pstOnePartInfo = pstOnePartInfo->next;
    }
    pstOnePartInfo = uploadFilePartInfoList;

    for (i = 0; i < partCount; i++) {
        pstUploadFileProcData[i].stUploadProgressInfo.arrSize = partCount;
        pstUploadFileProcData[i].stUploadProgressInfo.index = i;
        pstUploadFileProcData[i].stUploadProgressInfo.progressArr = uploadFileProgress;
        pstUploadFileProcData[i].stUploadProgressInfo.totalFileSize = pstUploadParams->totalFileSize;
        pstUploadFileProcData[i].stUploadProgressInfo.uploadedSize = pstUploadParams->uploadedSize;
    }
#ifdef WIN32
    startUploadThreads_win32(uploadFileProcDataList, pstOnePartInfo, partCount, callback_data, pstUploadParams);
#endif

#if defined __GNUC__ || defined LINUX
    startUploadThreads_linux(pstUploadParams, partCount, callback_data, uploadFileProcDataList);
#endif

    if (uploadFileProcDataList) {
        free(uploadFileProcDataList);
        uploadFileProcDataList = NULL;
    }
    if (uploadFileProgress) {
        free(uploadFileProgress);
        uploadFileProgress = NULL;
    }
}


int isPrevPartComplete(upload_file_part_info *ptrUploadPartPrev, int *isAllSuccess)
{
    while (ptrUploadPartPrev)
    {
        if (ptrUploadPartPrev->uploadStatus != UPLOAD_SUCCESS)
        {
            *isAllSuccess = 0;
            if (ptrUploadPartPrev->uploadStatus != UPLOAD_FAILED)
            {
                return 0;
            }
        }
        ptrUploadPartPrev = ptrUploadPartPrev->prev;
    }
    return 1;
}

int isNextPartComplete(upload_file_part_info *ptrUploadPartNext, int *isAllSuccess)
{
    while (ptrUploadPartNext)
    {
        if (ptrUploadPartNext->uploadStatus != UPLOAD_SUCCESS)
        {
            *isAllSuccess = 0;
            if (ptrUploadPartNext->uploadStatus != UPLOAD_FAILED)
            {
                return 0;
            }
        }
        ptrUploadPartNext = ptrUploadPartNext->next;
    }
    return 1;
}

int isAllPartsComplete(upload_file_part_info * uploadPartNode, int * isAllSuccess)
{
    upload_file_part_info * ptrUploadPartPrev = uploadPartNode;
    upload_file_part_info * ptrUploadPartNext = uploadPartNode;
    *isAllSuccess = 1;

    if (uploadPartNode == NULL)
    {
        *isAllSuccess = 0;
        return 0;
    }
    if (!isPrevPartComplete(ptrUploadPartPrev, isAllSuccess)) {
        return 0;
    }

    if (!isNextPartComplete(ptrUploadPartNext, isAllSuccess))
    {
        return 0;
    }
    return 1;
}


obs_status CompleteMultipartUploadCallback_Intern(const char *location,
    const char *bucket,
    const char *key,
    const char *etag,
    void *callback_data)
{
    (void)callback_data;
    COMMLOG(OBS_LOGINFO, "location = %s \n bucket = %s \n key = %s \n etag = %s \n", location, bucket, key, etag);
    return OBS_STATUS_OK;
}


int completeUploadFileParts(upload_file_part_info * pstUploadInfoList, int partCount,
    const obs_options *options, char * key, obs_upload_file_server_callback server_callback,
    const char * upload_id, obs_response_handler *handler)
{
    obs_complete_upload_Info * pstUploadInfo = NULL;
    obs_complete_upload_Info * upInfoList = NULL;
    int i = 0;

    upload_file_part_info * pstSrcUploadInfo = pstUploadInfoList;
    obs_complete_multi_part_upload_handler response_handler =
    {
        {handler->properties_callback, handler->complete_callback},
        &CompleteMultipartUploadCallback_Intern
    };

    obs_put_properties stPutProperties;
    memset_s(&stPutProperties, sizeof(obs_put_properties), 0, sizeof(obs_put_properties));

    stPutProperties.expires = -1;
    stPutProperties.canned_acl = OBS_CANNED_ACL_PUBLIC_READ_WRITE;
    stPutProperties.server_callback = server_callback;

    upInfoList = (obs_complete_upload_Info *)malloc(sizeof(obs_complete_upload_Info) * partCount);
    if (upInfoList == NULL)
    {
        COMMLOG(OBS_LOGERROR, "in completeUploadFileParts, malloc for upInfoList failed");
        return -1;
    }


    pstUploadInfo = upInfoList;
    for (i = 0; i < partCount; i++)
    {
        if (!pstSrcUploadInfo)
        {
            COMMLOG(OBS_LOGERROR, "due to some reasons, some part is not upload ,can not complete\n");
            return -1;
        }
        pstUploadInfo->etag = pstSrcUploadInfo->etag;
        pstUploadInfo->part_number = pstSrcUploadInfo->part_num + 1;
        pstUploadInfo++;
        pstSrcUploadInfo = pstSrcUploadInfo->next;
    }
    //call complete part here
    complete_multi_part_upload(options, key, upload_id, partCount, upInfoList,
        &stPutProperties, &response_handler, 0);
    //release the data
    if (upInfoList)
    {
        free(upInfoList);
        upInfoList = NULL;
    }

    return 0;
}


int set_isFirstTimeSuccess(const obs_options *options, char *key, int isFirstTime, int uploadfileChanged,
    upload_file_summary *pstUploadFileSum, upload_file_part_info **pstUploadPartList, int *partCount,
    const char* checkpointFilename, upload_file_summary stUploadFileSummaryOld)
{
    uploadfileChanged = isUploadFileChanged(pstUploadFileSum, &stUploadFileSummaryOld);
    if (uploadfileChanged || checkUploadFileInfo(&stUploadFileSummaryOld, options, key) == 0)
    {
        //here the upload info is not available
        isFirstTime = 1;
        abortMultipartUploadAndFree(options, key, stUploadFileSummaryOld.upload_id, checkpointFilename, CLEAN_FILE);
        if (pstUploadPartList)
        {
            cleanUploadList(*pstUploadPartList);
            pstUploadPartList = NULL;
            *partCount = 0;
        }
    }
    else
    {
        isFirstTime = 0;
        errno_t err = EOK;
        err = memcpy_s(pstUploadFileSum, sizeof(upload_file_summary), &stUploadFileSummaryOld, sizeof(upload_file_summary));
        CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    }
    return isFirstTime;
}


int set_isFirstTime(const obs_options *options, char *key, obs_upload_file_configuration *upload_file_config,
    upload_file_part_info **pstUploadPartList, int *partCount,
    upload_file_summary *pstUploadFileSum)
{
    int isFirstTime = 1;
    int retVal = -1;
    int readCheckPointResult = 0;
    int uploadfileChanged = 0;
    char checkpointFilename[1024] = { 0 };
    upload_file_summary stUploadFileSummaryOld;

    if (!upload_file_config->enable_check_point)
    {
        return isFirstTime;
    }

    if (upload_file_config->check_point_file)
    {
        errno_t err = EOK;
        err = memcpy_s(checkpointFilename, ARRAY_LENGTH_1024, upload_file_config->check_point_file,
            strlen(upload_file_config->check_point_file) + 1);
        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "set_isFirstTime: memcpy_s failed!");
        }
    }

    retVal = setCheckPointFile(upload_file_config->upload_file, checkpointFilename, &isFirstTime, UPLOAD_FILE_INFO);
    if (!upload_file_config->check_point_file)
    {
        upload_file_config->check_point_file = checkpointFilename;
    }
    if (retVal == -1)
    {
        //no need to return here, we can continue but treat enable_check_point as false
        upload_file_config->enable_check_point = 0;
        return isFirstTime;
    }

    readCheckPointResult = readCheckpointFile(&stUploadFileSummaryOld, pstUploadPartList, partCount, checkpointFilename);
    if (readCheckPointResult == 0)//read success
    {
        isFirstTime = set_isFirstTimeSuccess(options, key, isFirstTime, uploadfileChanged, pstUploadFileSum,
            pstUploadPartList, partCount, checkpointFilename, stUploadFileSummaryOld);
    }

    return isFirstTime;
}


int get_uploadId_for_uploadFile_initUpload(const obs_options *options, char *key, obs_upload_file_configuration *upload_file_config,
    char *upload_id, upload_params *pstUploadParams, upload_file_part_info *pstUploadPartList,
    obs_response_handler *commonHandler, char *checkpointFilename, int isFirstTime)
{
    if ((isFirstTime == 1) || (NULL == pstUploadParams->upload_id) || (strlen(pstUploadParams->upload_id) == 0))
    {
        initiate_multi_part_upload(options, key, MAX_SIZE_UPLOADID, upload_id, 0, 0, commonHandler, 0);
        if (strlen(upload_id) == 0)
        {
            if (upload_file_config->enable_check_point)
            {
                (void)remove(checkpointFilename);
            }
            if (pstUploadPartList)
            {
                cleanUploadList(pstUploadPartList);
                pstUploadPartList = NULL;
            }
            return -1;
        }
    }
    else if (strlen(pstUploadParams->upload_id) != 0)
    {
        errno_t err = strcpy_s(upload_id, MAX_SIZE_UPLOADID, pstUploadParams->upload_id);
        CheckAndLogNoneZero(err, "strcpy_s", __FUNCTION__, __LINE__);
    }
    return 0;
}


int get_uploadId_for_uploadFile(const obs_options *options, char *key,
    obs_upload_file_configuration *upload_file_config,
    char *upload_id, upload_params *pstUploadParams, upload_file_part_info * pstUploadPartList,
    int set_partlist_retVal,
    obs_response_handler *commonHandler, char *checkpointFilename, int isFirstTime)
{
    if (set_partlist_retVal == -1)
    {
        COMMLOG(OBS_LOGINFO, "set_partlist_retVal = %d", set_partlist_retVal);
        if (upload_file_config->enable_check_point)
        {
            (void)remove(checkpointFilename);
        }

        if (pstUploadPartList)
        {
            cleanUploadList(pstUploadPartList);
            pstUploadPartList = NULL;
        }
        return -1;
    }

    if (!(upload_file_config->check_point_file) || !strcmp(upload_file_config->check_point_file, checkpointFilename)) {
        upload_file_config->check_point_file = checkpointFilename;
    }

    // init upload task   
    if (get_uploadId_for_uploadFile_initUpload(options, key, upload_file_config, upload_id, pstUploadParams,
        pstUploadPartList, commonHandler, checkpointFilename, isFirstTime))
    {
        return -1;
    }

    return 0;
}


void upload_complete_handle_allSuccess(const obs_options *options, char *key, obs_upload_file_response_handler *handler,
    upload_file_part_info * pstUploadPartList, int partCount, const char *upload_id, int isAllSuccess,
    obs_upload_file_configuration *upload_file_config, obs_upload_file_server_callback server_callback,
    const char *checkpointFilename, obs_upload_file_part_info *resultInfo, void *callback_data)
{
    int is_true = 0;
    int retComplete = -1;
    if (isAllSuccess == 1)
    {
        retComplete = completeUploadFileParts(pstUploadPartList, partCount, options, key, server_callback,
            upload_id, &handler->response_handler);
        is_true = ((retComplete == 0) && upload_file_config->enable_check_point);
        if (is_true)
        {
            (void)remove(checkpointFilename);
        }
        if (retComplete == 0)
        {
            if (handler->upload_file_callback)
            {
                handler->upload_file_callback(OBS_STATUS_OK, "upload file success!\n", 0, NULL, callback_data);
            }
        }
        else if (handler->upload_file_callback)
        {
            handler->upload_file_callback(OBS_STATUS_InternalError,
                "complete multi part failed!\n", 0, NULL, callback_data);
        }
    }
    else
    {
        upload_file_part_info *printNode = pstUploadPartList;
        obs_upload_file_part_info *pstPartInfoRet;
        resultInfo = (obs_upload_file_part_info*)malloc(sizeof(obs_upload_file_part_info)*partCount);
        if (resultInfo == NULL)
        {
            COMMLOG(OBS_LOGERROR, "malloc resultInfo failed in upload_complete_handle_allSuccess\n");
            return;
        }
        memset_s(resultInfo, sizeof(obs_upload_file_part_info)*partCount, 0,
            sizeof(obs_upload_file_part_info)*partCount);
        pstPartInfoRet = resultInfo;
        while (printNode)
        {
            pstPartInfoRet->part_num = printNode->part_num + 1;
            pstPartInfoRet->part_size = printNode->start_byte;
            pstPartInfoRet->status_return = printNode->uploadStatus;
            printNode = printNode->next;
            pstPartInfoRet++;
        }
        if (handler->upload_file_callback)
        {
            handler->upload_file_callback(OBS_STATUS_InternalError,
                "some part success , some parts failed!\n", partCount, resultInfo, callback_data);
        }
        free(resultInfo);
        resultInfo = NULL;
    }
    is_true = (((isAllSuccess == 0) || (retComplete != 0))
        && (upload_file_config->enable_check_point == 0));
    if (is_true)
    {
        abortMultipartUploadAndFree(options, key, upload_id, NULL, DO_NOTHING);
    }
    return;
}


void upload_complete_handle(const obs_options *options, char *key, obs_upload_file_response_handler *handler,
    upload_file_part_info * pstUploadPartList, int partCount, const char *upload_id,
    obs_upload_file_configuration *upload_file_config, obs_upload_file_server_callback server_callback,
    const char *checkpointFilename, void *callback_data)
{
    int isAllSuccess = 0;
    obs_upload_file_part_info * resultInfo = NULL;
    if (isAllPartsComplete(pstUploadPartList, &isAllSuccess) == 1)
    {
        upload_complete_handle_allSuccess(options, key, handler, pstUploadPartList, partCount, upload_id,
            isAllSuccess, upload_file_config, server_callback, checkpointFilename, resultInfo, callback_data);
        return;
    }

    upload_file_part_info * printNode = pstUploadPartList;
    if (!upload_file_config->enable_check_point)
    {
        abortMultipartUploadAndFree(options, key, upload_id, NULL, DO_NOTHING);
    }

    obs_upload_file_part_info * pstPartInfoRet = NULL;
    resultInfo = (obs_upload_file_part_info*)malloc(sizeof(obs_upload_file_part_info)*partCount);
    if (resultInfo == NULL)
    {
        COMMLOG(OBS_LOGERROR, "malloc resultInfo failed in upload_complete_handle\n");
        return;
    }
    memset_s(resultInfo, sizeof(obs_upload_file_part_info)*partCount, 0,
        sizeof(obs_upload_file_part_info)*partCount);
    pstPartInfoRet = resultInfo;

    while (printNode)
    {
        COMMLOG(OBS_LOGERROR, "part_num[%d], status[%s]\n", printNode->part_num,
            g_uploadStatus[printNode->uploadStatus]);

        pstPartInfoRet->part_num = printNode->part_num + 1;
        pstPartInfoRet->part_size = printNode->part_size;
        pstPartInfoRet->start_byte = printNode->start_byte;
        pstPartInfoRet->status_return = printNode->uploadStatus;
        printNode = printNode->next;
        pstPartInfoRet++;
    }

    if (handler->upload_file_callback)
    {
        handler->upload_file_callback(OBS_STATUS_InternalError,
            "some part success, some parts failed!\n", partCount, resultInfo, callback_data);
    }
    free(resultInfo);
    resultInfo = NULL;

    COMMLOG(OBS_LOGERROR, "leave upload_complete_handle success\n");
    return;
}


int upload_file_setParams(upload_file_summary *stUploadFileSum, const obs_options *options, char *key,
    char *upload_id, server_side_encryption_params *encryption_params, char *checkpointFilename,
    obs_upload_file_configuration *upload_file_config, obs_upload_file_response_handler *handler,
    void *callback_data, errno_t err, int isFirstTime, int is_ture, int partCount,
    upload_file_part_info *pstUploadPartList, upload_params *stUploadParams)
{
    err = memcpy_s(stUploadFileSum->bucket_name, MAX_BKTNAME_SIZE, options->bucket_options.bucket_name, strlen(options->bucket_options.bucket_name) + 1);
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(stUploadFileSum->key, MAX_KEY_SIZE, key, strlen(key) + 1);
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    err = memcpy_s(stUploadFileSum->upload_id, MAX_SIZE_UPLOADID, upload_id, strlen(upload_id) + 1);
    CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    is_ture = ((upload_file_config->enable_check_point == 1) && (isFirstTime == 1));
    if (is_ture)
    {
        int ret = writeCheckpointFile(stUploadFileSum, pstUploadPartList, partCount, checkpointFilename);
        if (ret == -1) {
            COMMLOG(OBS_LOGWARN, "Failed to write checkpoint file.");
        }
    }
    stUploadParams->fileNameCheckpoint = checkpointFilename;
    stUploadParams->enable_check_point = upload_file_config->enable_check_point;
    stUploadParams->callBackData = callback_data;
    stUploadParams->fileNameUpload = upload_file_config->upload_file;
    stUploadParams->objectName = key;
    stUploadParams->options = options;
    stUploadParams->pstServerSideEncryptionParams = encryption_params;
    stUploadParams->response_handler = &(handler->response_handler);
    stUploadParams->upload_id = upload_id;
    stUploadParams->progress_callback = handler->progress_callback;

    return is_ture;
}

static void calcTotalUploadedSize(upload_params * pstUploadParams,
                                  upload_file_part_info * uploadFilePartInfoList,
                                  int partCount){
    upload_file_part_info *pstOnePartInfo = uploadFilePartInfoList;
    int i = 0;
    while(pstOnePartInfo && (partCount == 0 || i < partCount)) {
        if (pstOnePartInfo->uploadStatus == UPLOAD_SUCCESS) {
            pstUploadParams->uploadedSize += pstOnePartInfo->part_size;
        }
        pstOnePartInfo = pstOnePartInfo->next;
        i++;
    }
}

void pause_upload_file(int *pause_flag)
{
    COMMLOG(OBS_LOGINFO, "Enter pause_upload_file successfully ! pause_flag = %d",
        *pause_flag);
    *pause_flag = 1;
    COMMLOG(OBS_LOGINFO, "pause_flag is change to %d", *pause_flag);
}

void upload_file(const obs_options *options, char *key, server_side_encryption_params *encryption_params,
    obs_upload_file_configuration *upload_file_config, obs_upload_file_server_callback server_callback,
    obs_upload_file_response_handler *handler, void *callback_data)
{
    COMMLOG(OBS_LOGERROR, "*pause_upload_flag is %d",
        (*(upload_file_config->pause_upload_flag)));
    int isFirstTime = 1;
    int retVal = -1;
    upload_file_part_info * pstUploadPartList = NULL;
    upload_file_summary stUploadFileSum;
    upload_file_part_info * pstUploadPartListDone = NULL;
    upload_file_part_info * pstUploadPartListNotDone = NULL;
    int partCount = 0;
    int partCountToProc = 0;
    char checkpointFilename[1024] = { 0 };
    char upload_id[MAX_SIZE_UPLOADID];
    upload_params stUploadParams;
    int is_ture = 0;
    uint64_t uploadPartSize = 0;

    memset_s(&stUploadParams, sizeof(upload_params), 0, sizeof(upload_params));
    memset_s(&stUploadFileSum, sizeof(upload_file_summary), 0, sizeof(upload_file_summary));

    errno_t err = EOK;
    if (upload_file_config->check_point_file)
    {
        err = memcpy_s(checkpointFilename, ARRAY_LENGTH_1024, upload_file_config->check_point_file,
            strlen(upload_file_config->check_point_file) + 1);
        CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    }
    //get the summary of the upload file 
    retVal = getUploadFileSummary(&stUploadFileSum, upload_file_config->upload_file);
    if ((retVal == -1) || (stUploadFileSum.fileSize == 0))
    {
        COMMLOG(OBS_LOGERROR, "the upload file is not exist or it's size is 0\n");
        (void)(*(handler->response_handler.complete_callback))(OBS_STATUS_InvalidPart, 0, callback_data);
        return;
    }

    //set the check point file
    isFirstTime = set_isFirstTime(options, key, upload_file_config,
        &pstUploadPartList, &partCount, &stUploadFileSum);

    if (upload_file_config->check_point_file)
    {
        err = memcpy_s(checkpointFilename, ARRAY_LENGTH_1024, upload_file_config->check_point_file,
            strlen(upload_file_config->check_point_file) + 1);
        CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    }
    is_ture = ((upload_file_config->part_size == 0)
        || (upload_file_config->part_size > MAX_PART_SIZE));
    uploadPartSize = is_ture ? MAX_PART_SIZE : upload_file_config->part_size;
    uploadPartSize = (uploadPartSize > stUploadFileSum.fileSize) ? stUploadFileSum.fileSize : uploadPartSize;

    //set the part list to upload
    retVal = setPartList(&stUploadFileSum, uploadPartSize, &pstUploadPartList, &partCount, isFirstTime);
    stUploadParams.upload_id = stUploadFileSum.upload_id;
    stUploadParams.totalFileSize = stUploadFileSum.fileSize;
    stUploadParams.pause_upload_flag = upload_file_config->pause_upload_flag;

    retVal = get_uploadId_for_uploadFile(options, key, upload_file_config, upload_id, &stUploadParams,
        pstUploadPartList, retVal, &(handler->response_handler), checkpointFilename, isFirstTime);
    if (-1 == retVal)
    {
        return;
    }
    is_ture = upload_file_setParams(&stUploadFileSum, options, key, upload_id, encryption_params, checkpointFilename,
        upload_file_config, handler, callback_data, err, isFirstTime, is_ture, partCount, pstUploadPartList,
        &stUploadParams);

    (void)DividUploadPartList(pstUploadPartList, &pstUploadPartListDone, &pstUploadPartListNotDone);

    calcTotalUploadedSize(&stUploadParams, pstUploadPartListDone, 0);
    COMMLOG(OBS_LOGDEBUG, "upload_file before uploadedSize=%lu\n", stUploadParams.uploadedSize);

    //start upload part threads now
    partCountToProc = 0;
    upload_file_config->task_num = (upload_file_config->task_num == 0) ? MAX_THREAD_NUM : upload_file_config->task_num;
    while (pstUploadPartListNotDone)
    {
#if defined (WIN32)
        Sleep(1000);
#else
        sleep(1);
#endif

        (void)GetUploadPartListToProcess(&pstUploadPartListDone, &pstUploadPartListNotDone,
            partCountToProc, &partCountToProc, upload_file_config->task_num);

        if (*(upload_file_config->pause_upload_flag) == 1) {
            COMMLOG(OBS_LOGERROR, "pstUploadPartListNotDone:%p is aborted by user!", pstUploadPartListNotDone);
            if (stUploadParams.response_handler->complete_callback) {
                (stUploadParams.response_handler->complete_callback)(OBS_STATUS_AbortedByCallback, 0, callback_data);
            }
            break;
        }

        startUploadThreads(&stUploadParams, pstUploadPartListNotDone, partCountToProc, callback_data);
        calcTotalUploadedSize(&stUploadParams, pstUploadPartListNotDone, partCountToProc);
    }
    pstUploadPartList = pstUploadPartListDone;
    upload_complete_handle(options, key, handler, pstUploadPartList, partCount, upload_id,
        upload_file_config, server_callback, checkpointFilename, callback_data);

    if (pstUploadPartList)
    {
        cleanUploadList(pstUploadPartList);
        pstUploadPartList = NULL;
    }

    return;
}