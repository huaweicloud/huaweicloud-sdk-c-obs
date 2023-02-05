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
#include "object.h"
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
#include <io.h>
#include <share.h>
#include <process.h>
#endif

#define LENGTH_1024 1024
#define LENGTH_32 32

obs_storage_class getStorageClassEnum(const char * storage_class_value)
{
    if (!strcmp(storage_class_value, "STANDARD"))
    {
        return OBS_STORAGE_CLASS_STANDARD;
    }
    else if (!strcmp(storage_class_value, "STANDARD_IA"))
    {
        return OBS_STORAGE_CLASS_STANDARD_IA;
    }
    else if (!strcmp(storage_class_value, "GLACIER"))
    {
        return OBS_STORAGE_CLASS_GLACIER;
    }
    else
    {
        return OBS_STORAGE_CLASS_STANDARD;
    }
}

static obs_status GetObjectMetadataPropertiesCallback_Intern
(const obs_response_properties *properties, void *callback_data)
{
    get_object_metadata_callback_data * cb = (get_object_metadata_callback_data *)callback_data;
    download_file_summary * pstFileInfo = cb->pstFileInfo;

    pstFileInfo->objectLength = properties->content_length;
    pstFileInfo->lastModify = properties->last_modified;
    if (properties->etag)
    {
        errno_t err = EOK;
        err = memcpy_s(pstFileInfo->etag, MAX_SIZE_ETAG, properties->etag, strlen(properties->etag));
        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "GetObjectMetadataPropertiesCallback_Intern: memcpy_s failed!\n");
        }
    }

    if (properties->storage_class)
    {
        pstFileInfo->storage_class = getStorageClassEnum(properties->storage_class);
    }

    return OBS_STATUS_OK;
}

static void GetObjectMetadataCompleteCallback_Intern(obs_status status,
    const obs_error_details *error,
    void *callback_data)
{
    get_object_metadata_callback_data * cb = (get_object_metadata_callback_data *)callback_data;
    (void)error;
    cb->retStatus = status;
}

obs_status getObjectInfo(download_file_summary * downloadFileInfo,
    const obs_options *options, char *key, char* version_id,
    server_side_encryption_params *encryption_params)
{
    get_object_metadata_callback_data stGetObjectMetadataCallBackData;

    obs_response_handler getObjMetadataHandler = {
       &GetObjectMetadataPropertiesCallback_Intern, &GetObjectMetadataCompleteCallback_Intern
    };

    memset_s(&stGetObjectMetadataCallBackData, sizeof(get_object_metadata_callback_data),
        0, sizeof(get_object_metadata_callback_data));

    stGetObjectMetadataCallBackData.pstFileInfo = downloadFileInfo;

    obs_object_info object_info;
    memset_s(&object_info, sizeof(obs_object_info), 0, sizeof(obs_object_info));
    object_info.key = key;
    object_info.version_id = version_id;
    get_object_metadata(options, &object_info, encryption_params,
        &getObjMetadataHandler, &stGetObjectMetadataCallBackData);

    return stGetObjectMetadataCallBackData.retStatus;
}

obs_status restoreGlacierObject(const obs_options *options, char * key, char * version_id)
{

    obs_tier tier = OBS_TIER_EXPEDITED;
    char *days = "1";
    lisPartResult retResult;

    const obs_response_handler handler =
    {
        &ListPartsPropertiesCallback_Intern,&ListPartsCompleteCallback_Intern,
    };
    retResult.retStatus = OBS_STATUS_OK;
    obs_object_info  object_info;
    memset_s(&object_info, sizeof(obs_object_info), 0, sizeof(obs_object_info));
    object_info.key = key;
    object_info.version_id = version_id;
    restore_object(options, &object_info, days, tier, &handler, &retResult);
    return retResult.retStatus;
}

download_status GetDownloadStatusEnum(const char * strStatus)
{
    if (!strcmp(strStatus, "DOWNLOAD_NOTSTART"))
    {
        return DOWNLOAD_NOTSTART;
    }
    else if (!strcmp(strStatus, "DOWNLOADING"))
    {
        return DOWNLOADING;
    }
    else if (!strcmp(strStatus, "DOWNLOAD_FAILED"))
    {
        return DOWNLOAD_FAILED;
    }
    else if (!strcmp(strStatus, "DOWNLOAD_SUCCESS"))
    {
        return DOWNLOAD_SUCCESS;
    }
    else if (!strcmp(strStatus, "COMBINE_SUCCESS"))
    {
        return COMBINE_SUCCESS;
    }
    else
    {
        return DOWNLOAD_NOTSTART;
    }
}

void cleanDownloadList(download_file_part_info * downloadPartListinfo)
{
    download_file_part_info * ptrDownloadPart = downloadPartListinfo;
    download_file_part_info * ptrDownloadPartNext = downloadPartListinfo;
    while (ptrDownloadPart)
    {
        ptrDownloadPartNext = ptrDownloadPart->next;

        free(ptrDownloadPart);
        ptrDownloadPart = NULL;

        ptrDownloadPart = ptrDownloadPartNext;
    }
}

errno_t parse_download_xmlnode_objectinfo_xmlCmp(xmlNodePtr objectinfoNode,
    download_file_summary *pstDownLoadSummary, xmlChar *nodeContent,
    errno_t err)
{
    if (!xmlStrcmp(objectinfoNode->name, (xmlChar *)"ContentLength"))
    {
        pstDownLoadSummary->objectLength = parseUnsignedInt((char*)nodeContent);
    }
    else if (!xmlStrcmp((xmlChar *)objectinfoNode->name, (xmlChar*)"lastmodify"))
    {
        pstDownLoadSummary->lastModify = parseUnsignedInt((char*)nodeContent);
    }
    else if (!xmlStrcmp(objectinfoNode->name, (xmlChar *)"etag"))
    {
        err = memcpy_s(pstDownLoadSummary->etag, MAX_SIZE_ETAG, (char*)nodeContent, strlen((char*)nodeContent) + 1);
    }
    else if (!xmlStrcmp(objectinfoNode->name, (xmlChar*)"storageclass"))
    {
        pstDownLoadSummary->storage_class = getStorageClassEnum((char*)nodeContent);
    }
    else if (!xmlStrcmp(objectinfoNode->name, (xmlChar*)"bucketname"))
    {
        err = memcpy_s(pstDownLoadSummary->bucket_name, MAX_BKTNAME_SIZE, nodeContent, strlen((char*)nodeContent) + 1);
    }
    else if (!xmlStrcmp(objectinfoNode->name, (xmlChar*)"key"))
    {
        err = memcpy_s(pstDownLoadSummary->key, MAX_KEY_SIZE, nodeContent, strlen((char*)nodeContent) + 1);
    }
    return err;
}


void parse_download_xmlnode_objectinfo(xmlNodePtr curNode,
    download_file_summary * pstDownLoadSummary)
{
    xmlNodePtr objectinfoNode = curNode->xmlChildrenNode;
    while (objectinfoNode != NULL)
    {
        xmlChar *nodeContent = xmlNodeGetContent(objectinfoNode);
        errno_t err = EOK;

        err = parse_download_xmlnode_objectinfo_xmlCmp(objectinfoNode,
            pstDownLoadSummary, nodeContent, err);
        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "parse_download_xmlnode_objectinfo: memcpy_s failed!\n");
        }

        xmlFree(nodeContent);
        objectinfoNode = objectinfoNode->next;
    }

    return;
}

int parse_download_xmlnode_partsinfo_xmlCmp(xmlNodePtr partinfoNode,
    download_file_part_info *downloadPartNode, xmlChar *nodeContent)
{
    if (!xmlStrcmp(partinfoNode->name, (xmlChar*)"partNum"))
    {
        downloadPartNode->part_num = (int)parseUnsignedInt((char*)nodeContent) - 1;
    }
    else if (!xmlStrcmp(partinfoNode->name, (xmlChar*)"partNum"))
    {
        memset_s(downloadPartNode->etag, MAX_SIZE_ETAG, 0, MAX_SIZE_ETAG);
        errno_t err = EOK;
        err = memcpy_s(downloadPartNode->etag, MAX_SIZE_ETAG, nodeContent, strlen((char*)nodeContent) + 1);
        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "parse_download_xmlnode_partsinfo: memcpy_s failed!\n");
            free(downloadPartNode);
            downloadPartNode = NULL;
            return -1;
        }
    }
    else if (!xmlStrcmp(partinfoNode->name, (xmlChar*)"startByte"))
    {
        downloadPartNode->start_byte = parseUnsignedInt((char*)nodeContent);
    }
    else if (!xmlStrcmp(partinfoNode->name, (xmlChar*)"partSize"))
    {
        downloadPartNode->part_size = parseUnsignedInt((char*)nodeContent);
    }
    else if (!xmlStrcmp(partinfoNode->name, (xmlChar*)"downloadStatus"))
    {
        downloadPartNode->downloadStatus = GetDownloadStatusEnum((char*)nodeContent);
    }
    return 0;
}

int parse_download_xmlnode_partsinfo(xmlNodePtr curNode,
    download_file_part_info **downloadPartList,
    int *partCount)
{
    download_file_part_info * downloadPartNode = NULL;
    download_file_part_info * pstDownloadPart = NULL;
    int partCountTmp = 0;

    xmlNodePtr partNode = curNode->xmlChildrenNode;
    xmlNodePtr partinfoNode = NULL;
    while (partNode)
    {
        if (strncmp((char*)partNode->name, "part", strlen("part")))
        {
            partNode = partNode->next;
            continue;
        }

        downloadPartNode = (download_file_part_info *)malloc(sizeof(download_file_part_info));
        if (downloadPartNode == NULL)
        {
            COMMLOG(OBS_LOGERROR, "int readCheckpointFile_Download, malloc for uploadPartNode failed");
            cleanDownloadList(*downloadPartList);
            partCountTmp = 0;
            *partCount = 0;
            return -1;
        }
        downloadPartNode->next = NULL;
        partCountTmp++;

        partinfoNode = partNode->xmlChildrenNode;
        while (partinfoNode != NULL)
        {
            xmlChar *nodeContent = xmlNodeGetContent(partinfoNode);
            COMMLOG(OBS_LOGINFO, "name:%s content %s\n", partinfoNode->name, nodeContent);
            //get parts info and store in uploadPartList
            if (parse_download_xmlnode_partsinfo_xmlCmp(partinfoNode,
                downloadPartNode, nodeContent))
            {
                return -1;
            }

            xmlFree(nodeContent);
            partinfoNode = partinfoNode->next;
        }

        downloadPartNode->prev = pstDownloadPart;
        if (pstDownloadPart == NULL)
        {
            pstDownloadPart = downloadPartNode;
            *downloadPartList = downloadPartNode;
        }
        else
        {
            pstDownloadPart->next = downloadPartNode;
            pstDownloadPart = pstDownloadPart->next;
        }

        partNode = partNode->next;
    }

    *partCount = partCountTmp;
    return 0;
}


int readCheckpointFile_Download_xmlCmp(xmlNodePtr curNode, download_file_summary *pstDownLoadSummary,
    download_file_part_info **downloadPartList, int *partCount, int ret_stat)
{
    while (curNode != NULL)
    {
        if (!xmlStrcmp(curNode->name, (xmlChar*)"objectinfo"))
        {
            parse_download_xmlnode_objectinfo(curNode, pstDownLoadSummary);
        }
        if (!xmlStrcmp(curNode->name, (xmlChar*)"partsinfo"))
        {
            ret_stat = parse_download_xmlnode_partsinfo(curNode, downloadPartList,
                partCount);
            if (-1 == ret_stat)
            {
                break;
            }
        }
        curNode = curNode->next;
    }
    return ret_stat;
}

int readCheckpointFile_Download(download_file_summary * pstDownLoadSummary,
    download_file_part_info **downloadPartList,
    int *partCount, char *file_name)
{
    xmlNodePtr curNode;
    xmlDocPtr doc;           //the doc pointer to parse the file
    int ret_stat = 0;

    if (check_file_is_valid(file_name) == -1)
    {
        return -1;
    }

    curNode = get_xmlnode_from_file(file_name, &doc);
    if (NULL == curNode)
    {
        return -1;
    }

    if (xmlStrcmp((xmlChar *)curNode->name, BAD_CAST "downloadinfo"))
    {
        COMMLOG(OBS_LOGERROR, "document of the wrong type, root node != downloadinfo");
        xmlFreeDoc(doc);
        return -1;
    }
    curNode = curNode->xmlChildrenNode;
    ret_stat = readCheckpointFile_Download_xmlCmp(curNode, pstDownLoadSummary,
        downloadPartList, partCount, ret_stat);

    xmlFreeDoc(doc);
    return ret_stat;
}

void removeTempFiles(const char * fileName, download_file_part_info * downloadPartList, int removeAll)
{
    download_file_part_info * partNode = downloadPartList;
    char fileNameTemp[1024] = { 0 };
    if ((fileName == NULL) || (downloadPartList == NULL))
    {
        return;
    }

    if (removeAll)
    {
        remove(fileName);
    }

    while (partNode)
    {
#if defined WIN32
        Sleep(0);
#endif

#if defined __GNUC__ || defined LINUX
        sleep(0);
#endif
        if (partNode->downloadStatus != COMBINE_SUCCESS)
        {
            int ret = sprintf_s(fileNameTemp, ARRAY_LENGTH_1024, "%s.%d", fileName, partNode->part_num);
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            remove(fileNameTemp);
        }
        partNode = partNode->next;
    }
}

int setDownloadpartList(download_file_summary *pstDownLoadFileSummaryNew, uint64_t downloadPartsize,
    download_file_part_info ** downloadPartList, int *partCount)
{
    int partCountTemp = 0;
    download_file_part_info * downloadPartListTemp = NULL;
    download_file_part_info * pstdownloadPartListTemp = NULL;
    download_file_part_info * pstDownloadPartPrev = NULL;
    uint64_t lastPartSize = 0;
    int i = 0;
    COMMLOG(OBS_LOGERROR, "download  pstDownLoadFileSummaryNew->objectLength = %d  ;downloadPartsize = %d",
        pstDownLoadFileSummaryNew->objectLength, downloadPartsize);
    partCountTemp = (int)(pstDownLoadFileSummaryNew->objectLength / downloadPartsize);
    lastPartSize = pstDownLoadFileSummaryNew->objectLength % downloadPartsize;
    *partCount = partCountTemp;
    for (i = 0; i < partCountTemp; i++)
    {
        pstdownloadPartListTemp = (download_file_part_info*)malloc(sizeof(download_file_part_info));
        if (pstdownloadPartListTemp == NULL)
        {
            COMMLOG(OBS_LOGERROR, "in %s failed to malloc for uploadPartListTemp !", __FUNCTION__);
            cleanDownloadList(pstdownloadPartListTemp);
            pstdownloadPartListTemp = NULL;
            return -1;
        }
        pstdownloadPartListTemp->next = NULL;
        pstdownloadPartListTemp->part_num = i;
        pstdownloadPartListTemp->start_byte = downloadPartsize * i;
        pstdownloadPartListTemp->part_size = downloadPartsize;
        pstdownloadPartListTemp->downloadStatus = DOWNLOAD_NOTSTART;
        memset_s(pstdownloadPartListTemp->etag, MAX_SIZE_ETAG, 0, MAX_SIZE_ETAG);
        if (i == 0)
        {
            pstdownloadPartListTemp->prev = NULL;
            pstDownloadPartPrev = NULL;
            downloadPartListTemp = pstdownloadPartListTemp;
        }
        else
        {
            pstdownloadPartListTemp->prev = pstDownloadPartPrev;
            pstDownloadPartPrev->next = pstdownloadPartListTemp;
        }

        pstDownloadPartPrev = pstdownloadPartListTemp;
    }
    if (lastPartSize != 0)
    {
        pstdownloadPartListTemp = (download_file_part_info*)malloc(sizeof(download_file_part_info));
        if (pstdownloadPartListTemp == NULL)
        {
            COMMLOG(OBS_LOGERROR, "in %s failed to malloc for uploadPartListTemp !", __FUNCTION__);
            cleanDownloadList(downloadPartListTemp);
            downloadPartListTemp = NULL;
            return -1;
        }
        pstdownloadPartListTemp->prev = pstDownloadPartPrev;

        if (pstDownloadPartPrev)
        {
            pstDownloadPartPrev->next = pstdownloadPartListTemp;
        }
        COMMLOG(OBS_LOGERROR, "download 4");

        pstdownloadPartListTemp->part_num = i;
        pstdownloadPartListTemp->start_byte = downloadPartsize * i;
        pstdownloadPartListTemp->part_size = lastPartSize;
        pstdownloadPartListTemp->downloadStatus = DOWNLOAD_NOTSTART;
        pstdownloadPartListTemp->next = NULL;
        memset_s(pstdownloadPartListTemp->etag, MAX_SIZE_ETAG, 0, MAX_SIZE_ETAG);
        *partCount = partCountTemp + 1;
    }
    else
    {
        pstdownloadPartListTemp = NULL;
    }
    COMMLOG(OBS_LOGERROR, "download 5");
    *downloadPartList = downloadPartListTemp;
    return 0;
}

int writeCheckpointFile_Download(download_file_summary * pstDownloadFileSummary,
    download_file_part_info * downloadPartList, int partCount, char * file_name)
{
    //new doc    
    char str_content[512] = { 0 };
    xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");
    char contentBuff[64];
    int i = 0;
    int nRel = -1;

    download_file_part_info * ptrDownloadPart = NULL;

    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"downloadinfo");

    xmlNodePtr node_objectinfo = xmlNewNode(NULL, BAD_CAST"objectinfo");
    xmlNodePtr node_partsinfo = xmlNewNode(NULL, BAD_CAST"partsinfo");
    //set the root node <uploadinfo>    
    xmlDocSetRootElement(doc, root_node);
    //add <object_info> and <partsinfo> under <downloadinfo>        

    //add <object_info> node under <downloadinfo>    
    xmlAddChild(root_node, node_objectinfo);


    //add <filesize> under <object_info>    
    int ret = sprintf_s(str_content, ARRAY_LENGTH_512, "%llu", (long long unsigned int)pstDownloadFileSummary->objectLength);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "ContentLength", BAD_CAST str_content);

    //add <lastmodify> under <object_info>    
    ret = sprintf_s(str_content, ARRAY_LENGTH_512, "%llu", (long long unsigned int)pstDownloadFileSummary->lastModify);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "lastmodify", BAD_CAST str_content);

    //add <etag> under <object_info>    
    //snprintf(str_content,512,"%d",pstUploadFileSummary->fileMd5);  
    //sprintf_sec(str_content,512,"%s",pstDownloadFileSummary->etag); 
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "etag", BAD_CAST pstDownloadFileSummary->etag);

    //add <storageclass> under <object_info>    
    ret = sprintf_s(str_content, ARRAY_LENGTH_512, "%s", g_storageClass[pstDownloadFileSummary->storage_class]);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "storageclass", BAD_CAST str_content);

    // add <bucketname> under <object_info>   
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "bucketname", BAD_CAST pstDownloadFileSummary->bucket_name);

    //add <key> under <object_info>  
    xmlNewTextChild(node_objectinfo, NULL, BAD_CAST "key", BAD_CAST pstDownloadFileSummary->key);

    //add <partsinfo> under <downloadinfo>
    xmlAddChild(root_node, node_partsinfo);

    if ((partCount) && (downloadPartList == NULL))
    {
        xmlFreeDoc(doc);
        return -1;
    }
    ptrDownloadPart = downloadPartList;
    for (i = 0; i < partCount; i++)
    {
        xmlNodePtr partNode = NULL;
        ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "part%d", i + 1);
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        partNode = xmlNewNode(NULL, BAD_CAST contentBuff);//here  contentBuff indicat the name of the xml node

        ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%d", i + 1);
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        xmlNewChild(partNode, NULL, BAD_CAST "partNum", BAD_CAST contentBuff);

        if (ptrDownloadPart != NULL)
        {
            ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%s", ptrDownloadPart->etag);
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            xmlNewChild(partNode, NULL, BAD_CAST "etag", BAD_CAST contentBuff);

            ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%llu", (long long unsigned int)ptrDownloadPart->start_byte);
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            xmlNewChild(partNode, NULL, BAD_CAST "startByte", BAD_CAST contentBuff);

            ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%llu", (long long unsigned int)ptrDownloadPart->part_size);
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            xmlNewChild(partNode, NULL, BAD_CAST "partSize", BAD_CAST contentBuff);

            ret = sprintf_s(contentBuff, ARRAY_LENGTH_64, "%s", g_downloadStatus[ptrDownloadPart->downloadStatus]);
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            xmlNewChild(partNode, NULL, BAD_CAST "downloadStatus", BAD_CAST contentBuff);
            ptrDownloadPart = ptrDownloadPart->next;
        }
        xmlAddChild(node_partsinfo, partNode);
    }


    //store the xml file    
    nRel = xmlSaveFile(file_name, doc);
    if (nRel != -1) {
        COMMLOG(OBS_LOGINFO, "%s file[%s] is not exist", "readCheckpointFile", file_name);
    }
    //free all the node int the doc    
    xmlFreeDoc(doc);
    return 0;
}

void DividDownloadPartListSetNode(download_file_part_info *pstSrcListNode, download_file_part_info **pstTmpDoneList,
    download_file_part_info **pstTmpNotDoneList)
{
    download_file_part_info *pstTmpDoneNode = NULL;
    download_file_part_info *pstTmpNotDoneNode = NULL;
    download_file_part_info *pstTmpList = NULL;
    download_file_part_info *pstTmpNode = NULL;
    while (pstSrcListNode) {
        //1. set the node and list to process, store in pstTmpNode and pstTmpList
        if ((pstSrcListNode->downloadStatus == DOWNLOAD_SUCCESS) || (pstSrcListNode->downloadStatus == COMBINE_SUCCESS))
        {
            pstTmpList = *pstTmpDoneList;
            pstTmpNode = pstTmpDoneNode;
        }
        else
        {
            pstTmpList = *pstTmpNotDoneList;
            pstTmpNode = pstTmpNotDoneNode;
        }
        //2. add the node from listSrc after pstTmpNode
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

        //3. set the pstTmpNode and pstTmpList, back to done list or not done list
        if ((pstSrcListNode->downloadStatus == DOWNLOAD_SUCCESS)
            || (pstSrcListNode->downloadStatus == COMBINE_SUCCESS))
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

int DividDownloadPartList(download_file_part_info * listSrc, download_file_part_info ** listDone,
    download_file_part_info ** listNotDone)
{
    //we assume that the listSrc is sorted in ascending order
    download_file_part_info *pstSrcListNode = listSrc;
    download_file_part_info *pstTmpDoneList = NULL;
    download_file_part_info *pstTmpNotDoneList = NULL;


    DividDownloadPartListSetNode(pstSrcListNode, &pstTmpDoneList,
        &pstTmpNotDoneList);

    *listDone = pstTmpDoneList;
    *listNotDone = pstTmpNotDoneList;
    return 0;

}

int addDownloadPartNodeToListMiddle(download_file_part_info **pstTempNode,
    download_file_part_info *partNode)
{
    download_file_part_info *pMiddleTempNode = *pstTempNode;
    while (pMiddleTempNode)
    {
        if (pMiddleTempNode->part_num > partNode->part_num)
        {
            partNode->next = pMiddleTempNode;
            partNode->prev = pMiddleTempNode->prev;
            pMiddleTempNode->prev->next = partNode;
            pMiddleTempNode->prev = partNode;
            return -1;
        }
        else
        {
            if (pMiddleTempNode->next != NULL)
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

void addDownloadPartNodeToList(download_file_part_info **listToAdd, download_file_part_info *partNode)
{
    download_file_part_info * pstTempNode = *listToAdd;
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
    if (addDownloadPartNodeToListMiddle(&pstTempNode, partNode)) {
        return;
    }
    if ((pstTempNode != NULL) && (pstTempNode->next == NULL))
    {
        pstTempNode->next = partNode;
        partNode->prev = pstTempNode;
    }
}

void GetDownloadPartListToProcessCount(download_file_part_info *pstTempNodeNotDone,
    int nodeCoutNotDone, int *partCountOut, int task_num)
{
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
}

int GetDownloadPartListToProcess(download_file_part_info **listDone, download_file_part_info ** listNotDones,
    int partCountIn, int * partCountOut, int task_num)
{
    int i = 0;
    int nodeCoutNotDone = 0;
    download_file_part_info * pstTempNodeNotDone = *listNotDones;
    download_file_part_info * pstTempNodeNotDoneNext = *listNotDones;

    for (i = 0; i < partCountIn; i++)
    {
        if (pstTempNodeNotDone)
        {
            pstTempNodeNotDoneNext = pstTempNodeNotDone->next;
            addDownloadPartNodeToList(listDone, pstTempNodeNotDone);

            pstTempNodeNotDone = pstTempNodeNotDoneNext;
            if (pstTempNodeNotDone)
            {
                pstTempNodeNotDoneNext = pstTempNodeNotDone->next;
            }
        }
    }
    *listNotDones = pstTempNodeNotDone;
    pstTempNodeNotDone = *listNotDones;
    GetDownloadPartListToProcessCount(pstTempNodeNotDone, nodeCoutNotDone, partCountOut, task_num);
    return 0;
}


static obs_status DownloadPartCompletePropertiesCallback
(const obs_response_properties *properties, void *callback_data)
{
    download_file_callback_data * cbd = (download_file_callback_data *)callback_data;

    if (properties->etag)
    {
        errno_t err = strcpy_s(cbd->pstDownloadFilePartInfo->etag, MAX_SIZE_ETAG, properties->etag);
        CheckAndLogNoneZero(err, "strcpy_s", __FUNCTION__, __LINE__);
    }
    if (cbd->enableCheckPoint)
    {
        char pathToUpdate[1024];
        if (properties->etag)
        {
            int ret = sprintf_s(pathToUpdate, ARRAY_LENGTH_1024, "%s%d/%s", "downloadinfo/partsinfo/part",
                cbd->pstDownloadFilePartInfo->part_num + 1, "etag");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
#if defined(WIN32)
            EnterCriticalSection((CRITICAL_SECTION *)cbd->xmlWriteMutex);
#endif

#if defined __GNUC__ || defined LINUX
            pthread_mutex_lock((pthread_mutex_t *)cbd->xmlWriteMutex);
#endif
            ret = updateCheckPoint(pathToUpdate, properties->etag, cbd->checkpointFilename);
            if (ret == -1) {
                COMMLOG(OBS_LOGWARN, "Failed to update checkpoint file in function: %s.", __FUNCTION__);
            }
#if defined(WIN32)
            LeaveCriticalSection((CRITICAL_SECTION *)cbd->xmlWriteMutex);
#endif

#if defined __GNUC__ || defined LINUX
            pthread_mutex_unlock((pthread_mutex_t *)cbd->xmlWriteMutex);
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

static void  downloadPartCompleteCallback(obs_status status,
    const obs_error_details *error,
    void *callback_data)
{
    download_file_callback_data * cbd = (download_file_callback_data *)callback_data;

    if (status == OBS_STATUS_OK)
    {
        cbd->pstDownloadFilePartInfo->downloadStatus = DOWNLOAD_SUCCESS;
    }
    else
    {
        cbd->pstDownloadFilePartInfo->downloadStatus = DOWNLOAD_FAILED;
    }

    if (cbd->enableCheckPoint)
    {
        char pathToUpdate[1024];
        char contentToSet[32];

        int ret = sprintf_s(pathToUpdate, ARRAY_LENGTH_1024, "%s%d/%s", "downloadinfo/partsinfo/part",
            cbd->pstDownloadFilePartInfo->part_num + 1, "downloadStatus");
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        if (status == OBS_STATUS_OK)
        {
            ret = sprintf_s(contentToSet, ARRAY_LENGTH_32, "%s", "DOWNLOAD_SUCCESS");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        }
        else
        {
            ret = sprintf_s(contentToSet, ARRAY_LENGTH_32, "%s", "DOWNLOAD_FAILED");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        }

        //must ensure do close tempfile before updateCheckPoint
        if (cbd->fdStorefile != -1)
        {
            close(cbd->fdStorefile);
            cbd->fdStorefile = -1;
        }

#if defined(WIN32)
        EnterCriticalSection((CRITICAL_SECTION *)cbd->xmlWriteMutex);
#endif

#if defined __GNUC__ || defined LINUX
        pthread_mutex_lock((pthread_mutex_t *)cbd->xmlWriteMutex);
#endif

        ret = updateCheckPoint(pathToUpdate, contentToSet, cbd->checkpointFilename);
        if (ret == -1) {
            COMMLOG(OBS_LOGWARN, "Failed to update checkpoint file in function: %s.", __FUNCTION__);
        }
#if defined(WIN32)
        LeaveCriticalSection((CRITICAL_SECTION *)cbd->xmlWriteMutex);
#endif

#if defined __GNUC__ || defined LINUX
        pthread_mutex_unlock((pthread_mutex_t *)cbd->xmlWriteMutex);
#endif

    }

    if (cbd->respHandler->complete_callback)
    {
        (cbd->respHandler->complete_callback)(status,
            error, cbd->callbackDataIn);
    }
    return;
}

static obs_status getObjectPartDataCallback(int buffer_size, const char *buffer,
    void *callback_data)
{
    download_file_callback_data * cbd = (download_file_callback_data *)callback_data;

    int fd = cbd->fdStorefile;

    size_t wrote = write(fd, buffer, buffer_size);

    return ((wrote < (size_t)buffer_size) ?
        OBS_STATUS_AbortedByCallback : OBS_STATUS_OK);
}

#if defined (WIN32)
unsigned __stdcall DownloadThreadProc_win32(void* param)
{
    download_file_proc_data * pstPara = (download_file_proc_data *)param;
    char * storeFileName = pstPara->pstDownloadParams->fileNameStore;
    uint64_t part_size = pstPara->pstDownloadFilePartInfo->part_size;
    int part_num = pstPara->pstDownloadFilePartInfo->part_num;
    server_side_encryption_params * pstEncrypParam = NULL;
    char strPartNum[16] = { 0 };
    download_file_callback_data  data;
    data.fdStorefile = -1;
    int fd = -1;
    char * fileNameTemp = (char*)malloc(1024);

    if (fileNameTemp == NULL)
    {
        COMMLOG(OBS_LOGWARN, "DownloadThreadProc_win32: malloc failed!\n");
    }

    int ret = sprintf_s(fileNameTemp, 1024, "%s.%d", storeFileName, part_num);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);

    (void)_sopen_s(&fd, fileNameTemp, _O_BINARY | _O_RDWR | _O_CREAT,
        _SH_DENYNO, _S_IREAD | _S_IWRITE);

    free(fileNameTemp);
    fileNameTemp = NULL;

    if (fd == -1)
    {
        COMMLOG(OBS_LOGWARN, "DownloadThreadProc_win32 open upload file failed, partnum[%d]\n", part_num);
    }
    else
    {
        obs_get_object_handler getObjectHandler =
        {
            {&DownloadPartCompletePropertiesCallback,
            &downloadPartCompleteCallback},
            &getObjectPartDataCallback
        };

        sprintf_s(strPartNum, ARRAY_LENGTH_16, "%d", part_num + 1);
        memset_s(&data, sizeof(download_file_callback_data), 0, sizeof(download_file_callback_data));

        data.bytesRemaining = part_size;
        data.totalBytes = part_size;
        data.callbackDataIn = pstPara->callBackData;
        data.checkpointFilename = pstPara->pstDownloadParams->fileNameCheckpoint;
        data.enableCheckPoint = pstPara->pstDownloadParams->enable_check_point;
        data.fdStorefile = fd;
        data.respHandler = pstPara->pstDownloadParams->response_handler;
        data.taskHandler = 0;
        data.pstDownloadFilePartInfo = pstPara->pstDownloadFilePartInfo;
        data.xmlWriteMutex = pstPara->xmlWriteMutex;

        pstEncrypParam = pstPara->pstDownloadParams->pstServerSideEncryptionParams;

        if (data.enableCheckPoint == 1)
        {
            char pathToUpdate[1024];
            char contentToSet[32];

            ret = sprintf_s(pathToUpdate, ARRAY_LENGTH_1024, "%s%s/%s", "downloadinfo/partsinfo/part", strPartNum, "downloadStatus");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            ret = sprintf_s(contentToSet, ARRAY_LENGTH_32, "%s", "DOWNLOADING");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            EnterCriticalSection((CRITICAL_SECTION *)pstPara->xmlWriteMutex);
            updateCheckPoint(pathToUpdate, contentToSet, pstPara->pstDownloadParams->fileNameCheckpoint);
            LeaveCriticalSection((CRITICAL_SECTION *)pstPara->xmlWriteMutex);

        }
        obs_object_info object_info;
        memset_s(&object_info, sizeof(obs_object_info), 0, sizeof(obs_object_info));

        object_info.key = pstPara->pstDownloadParams->objectName;
        object_info.version_id = pstPara->pstDownloadParams->version_id;
        pstPara->pstDownloadFilePartInfo->downloadStatus = DOWNLOADING;

        obs_get_conditions get_conditions = *(pstPara->pstDownloadParams->get_conditions);
        get_conditions.start_byte = pstPara->pstDownloadFilePartInfo->start_byte;
        get_conditions.byte_count = part_size;
        COMMLOG(OBS_LOGINFO, "get_object partnum[%d] start:%ld size:%ld", part_num, get_conditions.start_byte, get_conditions.byte_count);
        get_object(pstPara->pstDownloadParams->options, &object_info, &get_conditions,
            pstPara->pstDownloadParams->pstServerSideEncryptionParams, &getObjectHandler, &data);
    }

    if (data.fdStorefile != -1)
    {
        close(data.fdStorefile);
        data.fdStorefile = -1;
    }

    return 1;
}
#endif

#if defined __GNUC__ || defined LINUX
void * DownloadThreadProc_linux(void* param)
{
    download_file_proc_data * pstPara = (download_file_proc_data *)param;
    char * storeFileName = pstPara->pstDownloadParams->fileNameStore;
    uint64_t part_size = pstPara->pstDownloadFilePartInfo->part_size;
    int part_num = pstPara->pstDownloadFilePartInfo->part_num;
    char strPartNum[16] = { 0 };
    download_file_callback_data  data;
    data.fdStorefile = -1;
    int fd = -1;
    char * fileNameTemp = (char*)malloc(1024);

    if (fileNameTemp == NULL)
    {
        COMMLOG(OBS_LOGWARN, "DownloadThreadProc_linux: malloc failed!\n");
    }

    int ret = sprintf_s(fileNameTemp, 1024, "%s.%d", storeFileName, part_num);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);

    fd = open(fileNameTemp, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    free(fileNameTemp);
    fileNameTemp = NULL;

    if (fd == -1)
    {
        COMMLOG(OBS_LOGERROR, "open store file failed, partnum[%d]\n", part_num);
        return NULL;
    }
    else
    {
        obs_get_object_handler getObjectHandler =
        {
            {&DownloadPartCompletePropertiesCallback,
            &downloadPartCompleteCallback},
            &getObjectPartDataCallback
        };

        ret = sprintf_s(strPartNum, ARRAY_LENGTH_16, "%d", part_num + 1);
        memset_s(&data, sizeof(download_file_callback_data), 0, sizeof(download_file_callback_data));

        data.bytesRemaining = part_size;
        data.totalBytes = part_size;
        data.callbackDataIn = pstPara->callBackData;
        data.checkpointFilename = pstPara->pstDownloadParams->fileNameCheckpoint;
        data.enableCheckPoint = pstPara->pstDownloadParams->enable_check_point;
        data.fdStorefile = fd;
        data.respHandler = pstPara->pstDownloadParams->response_handler;
        data.taskHandler = 0;
        data.pstDownloadFilePartInfo = pstPara->pstDownloadFilePartInfo;
        data.xmlWriteMutex = pstPara->xmlWriteMutex;


        if (data.enableCheckPoint == 1)
        {
            char pathToUpdate[1024];
            char contentToSet[32];

            ret = sprintf_s(pathToUpdate, ARRAY_LENGTH_1024, "%s%s/%s", "downloadinfo/partsinfo/part", strPartNum, "downloadStatus");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            ret = sprintf_s(contentToSet, ARRAY_LENGTH_32, "%s", "DOWNLOADING");
            CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
            pthread_mutex_lock((pthread_mutex_t *)pstPara->xmlWriteMutex);
            updateCheckPoint(pathToUpdate, contentToSet, pstPara->pstDownloadParams->fileNameCheckpoint);
            pthread_mutex_unlock((pthread_mutex_t *)pstPara->xmlWriteMutex);

        }

        obs_object_info object_info;
        memset_s(&object_info, sizeof(obs_object_info), 0, sizeof(obs_object_info));

        object_info.key = pstPara->pstDownloadParams->objectName;
        object_info.version_id = pstPara->pstDownloadParams->version_id;
        pstPara->pstDownloadFilePartInfo->downloadStatus = DOWNLOADING;

        obs_get_conditions get_conditions = *(pstPara->pstDownloadParams->get_conditions);
        get_conditions.start_byte = pstPara->pstDownloadFilePartInfo->start_byte;
        get_conditions.byte_count = part_size;
        COMMLOG(OBS_LOGINFO, "get_object partnum[%d] start:%ld size:%ld", part_num, get_conditions.start_byte, get_conditions.byte_count);
        get_object(pstPara->pstDownloadParams->options, &object_info, &get_conditions,
            pstPara->pstDownloadParams->pstServerSideEncryptionParams, &getObjectHandler, &data);
    }

    if (data.fdStorefile != -1)
    {
        close(data.fdStorefile);
        data.fdStorefile = -1;
    }

    return NULL;
}
#endif

#ifdef WIN32
void startDownloadThreadsWin32(HANDLE *arrHandle, download_file_proc_data *downloadFileProcDataList,
    unsigned uiThread2ID, DWORD dwExitCode, int partCount, download_file_part_info *pstOnePartInfo)
{
    int i = 0;
    for (i = 0; i < partCount; i++)
    {
        arrHandle[i] = (HANDLE)_beginthreadex(NULL, 0, DownloadThreadProc_win32,
            &downloadFileProcDataList[i], CREATE_SUSPENDED, &uiThread2ID);
        if (arrHandle[i] == 0)
        {
            GetExitCodeThread(arrHandle[i], &dwExitCode);
            COMMLOG(OBS_LOGERROR, "create thread i[%d] failed exit code = %u \n", i, dwExitCode);
        }
        pstOnePartInfo = pstOnePartInfo->next;
    }
    for (i = 0; i < partCount; i++)
    {
        ResumeThread(arrHandle[i]);
    }
    for (i = 0; i < partCount; i++)
    {
        WaitForSingleObject(arrHandle[i], INFINITE);
    }
    for (i = 0; i < partCount; i++)
    {
        CloseHandle(arrHandle[i]);
    }
    if (arrHandle)
    {
        free(arrHandle);
        arrHandle = NULL;
    }

}
#endif

#if defined __GNUC__ || defined LINUX
void startDownloadThreadsLinux(download_file_proc_data *downloadFileProcDataList,
    pthread_t *arrThread, int partCount, int err)
{
    int i = 0;
    for (i = 0; i < partCount; i++)
    {
        err = pthread_create(&arrThread[i], NULL, DownloadThreadProc_linux, (void*)&downloadFileProcDataList[i]);
        if (err != 0)
        {
            COMMLOG(OBS_LOGWARN, "startDownloadThreads create thread failed i[%d]\n", i);
        }
    }

    for (i = 0; i < partCount; i++)
    {
        err = pthread_join(arrThread[i], NULL);
        if (err != 0)
        {
            COMMLOG(OBS_LOGWARN, "startDownloadThreads join thread failed i[%d]\n", i);
        }
    }
    if (arrThread)
    {
        free(arrThread);
        arrThread = NULL;
    }

}
#endif

void startDownloadThreads(download_params * pstDownloadParams,
    download_file_part_info * downloadFilePartInfoList,
    int partCount, void* callback_data, void *xmlwrite_mutex)
{

    int i = 0;
    download_file_proc_data * downloadFileProcDataList =
        (download_file_proc_data *)malloc(sizeof(download_file_proc_data)*partCount);
    if (downloadFileProcDataList == NULL)
    {
        COMMLOG(OBS_LOGWARN, "startDownloadThreads: downloadFileProcDataList malloc failed\n");
        if (pstDownloadParams->response_handler->complete_callback) {
            (pstDownloadParams->response_handler->complete_callback)(OBS_STATUS_InternalError, 0, callback_data);
        }
        return;
    }

    download_file_proc_data * pstDownloadFileProcData = downloadFileProcDataList;
    download_file_part_info *pstOnePartInfo = downloadFilePartInfoList;
#ifdef WIN32
    HANDLE * arrHandle = (HANDLE *)malloc(sizeof(HANDLE)*partCount);
    unsigned  uiThread2ID;
    DWORD   dwExitCode;

#endif

#if defined __GNUC__ || defined LINUX
    pthread_t * arrThread = (pthread_t *)malloc(sizeof(pthread_t)*partCount);

    if (arrThread == NULL)
    {
        COMMLOG(OBS_LOGWARN, "startDownloadThreads: arrThread malloc failed\n", i);
        if (pstDownloadParams->response_handler->complete_callback) {
            (pstDownloadParams->response_handler->complete_callback)(OBS_STATUS_InternalError, 0, callback_data);
        }
        return;
    }

    int err = 0;
#endif
    memset_s(downloadFileProcDataList, sizeof(download_file_proc_data)*partCount, 0, sizeof(download_file_proc_data)*partCount);

    for (i = 0; i < partCount; i++)
    {
        pstDownloadFileProcData->pstDownloadParams = pstDownloadParams;
        pstDownloadFileProcData->pstDownloadFilePartInfo = pstOnePartInfo;
        pstDownloadFileProcData->callBackData = callback_data;
        pstDownloadFileProcData->xmlWriteMutex = xmlwrite_mutex;
        pstOnePartInfo = pstOnePartInfo->next;
        pstDownloadFileProcData++;
    }
    pstOnePartInfo = downloadFilePartInfoList;
#ifdef WIN32    
    startDownloadThreadsWin32(arrHandle, downloadFileProcDataList, uiThread2ID, dwExitCode, partCount, pstOnePartInfo);
#endif
#if defined __GNUC__ || defined LINUX
    startDownloadThreadsLinux(downloadFileProcDataList, arrThread, partCount, err);
#endif

    if (downloadFileProcDataList)
    {
        free(downloadFileProcDataList);
        downloadFileProcDataList = NULL;
    }
}

int isAllDownLoadPartSuccessPrev(download_file_part_info *ptrDownloadPartPrev)
{
    while (ptrDownloadPartPrev)
    {
        if ((ptrDownloadPartPrev->downloadStatus != DOWNLOAD_SUCCESS) && (ptrDownloadPartPrev->downloadStatus != COMBINE_SUCCESS))
        {
            return 0;
        }
        ptrDownloadPartPrev = ptrDownloadPartPrev->prev;
    }
    return 1;
}

int isAllDownLoadpartSuccessNext(download_file_part_info *ptrDownloadPartNext)
{
    while (ptrDownloadPartNext)
    {
        if ((ptrDownloadPartNext->downloadStatus != DOWNLOAD_SUCCESS) && (ptrDownloadPartNext->downloadStatus != COMBINE_SUCCESS))
        {
            return 0;
        }
        ptrDownloadPartNext = ptrDownloadPartNext->next;
    }
    return 1;
}

int isAllDownLoadPartsSuccess(download_file_part_info * downloadPartNode)
{
    download_file_part_info * ptrDownloadPartPrev = downloadPartNode;
    download_file_part_info * ptrDownloadPartNext = downloadPartNode;

    if (downloadPartNode == NULL)
    {
        return 0;
    }
    if (!isAllDownLoadPartSuccessPrev(ptrDownloadPartPrev))
    {
        return 0;
    }
    if (!isAllDownLoadpartSuccessNext(ptrDownloadPartNext))
    {
        return 0;
    }
    return 1;
}

int combinePartsFileRead(uint64_t remain_bytes, int bytesToRead, int bytesReadOut,
    int bytesWritten, int fdDest,
    int fdsrc, char *buff, int writeSuccess)
{
    while (remain_bytes)
    {
#ifdef WIN32
        Sleep(0);
#endif // WIN32
#if defined __GNUC__ || defined LINUX
        sleep(0);
#endif
        bytesToRead = (int)((remain_bytes > MAX_READ_ONCE) ? MAX_READ_ONCE : remain_bytes);
        bytesReadOut = read(fdsrc, buff, bytesToRead);
        if (bytesReadOut < 0)
        {
            COMMLOG(OBS_LOGWARN, "combinePartsFile: bytesReadOut is negative");
            writeSuccess = 0;
            break;
        }
        bytesWritten = write(fdDest, buff, bytesReadOut);
        if (bytesWritten < bytesReadOut)
        {
            writeSuccess = 0;
            break;
        }
        remain_bytes = remain_bytes - bytesWritten;
    }
    return writeSuccess;
}

void combinePartsFileSuccess(download_file_part_info *partNode, const char *check_point_file,
    char *pathToUpdate, char *contentToSet, void *xmlwrite_mutex, int ret,
    char *fileNameTemp)
{
    partNode->downloadStatus = COMBINE_SUCCESS;
    if (check_point_file)
    {
        ret = sprintf_s(pathToUpdate, LENGTH_1024, "%s%d/%s", "downloadinfo/partsinfo/part",
            partNode->part_num + 1, "downloadStatus");
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        ret = sprintf_s(contentToSet, LENGTH_32, "%s", "COMBINE_SUCCESS");
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
#if defined WIN32
        EnterCriticalSection((CRITICAL_SECTION *)xmlwrite_mutex);
#endif // defined WIN32
#if defined __GNUC__ || defined LINUX
        pthread_mutex_lock((pthread_mutex_t *)xmlwrite_mutex);
#endif // defined __GNUC__ || defined LINUX
        updateCheckPoint(pathToUpdate, contentToSet, check_point_file);
#if defined WIN32
        LeaveCriticalSection((CRITICAL_SECTION *)xmlwrite_mutex);
#endif // defined WIN32
#if defined __GNUC__ || defined LINUX
        pthread_mutex_unlock((pthread_mutex_t *)xmlwrite_mutex);
#endif // defined __GNUC__ || defined LINUX
        //must resure do remove tempfile after updateCheckPoint(COMBINE_SUCCESS)
    }
    remove(fileNameTemp);
}

int combinePartsFile(const char * fileName, download_file_part_info * downloadPartList, const char * check_point_file, void *xmlwrite_mutex)
{
    download_file_part_info * partNode = downloadPartList;
    char fileNameTemp[1024] = { 0 };
    int fdDest = -1;
    int fdSrc = -1;
    uint64_t remain_bytes = 0;
    int bytesToRead = 0;
    int bytesReadOut = 0;
    int bytesWritten = 0;
    char * buff = NULL;
    int writeSuccess = 1;
    int is_true = 0;

    is_true = ((fileName == NULL) || (downloadPartList == NULL));
    if (is_true)
    {
        return -1;
    }
#if defined WIN32
    _sopen_s(&fdDest, fileName, _O_BINARY | _O_WRONLY | _O_CREAT,
        _SH_DENYNO, _S_IREAD | _S_IWRITE);
#endif

#if defined __GNUC__ || defined LINUX
    fdDest = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
#endif
    if (fdDest == -1)
    {
        COMMLOG(OBS_LOGERROR, "%s open file[%s] failed\n", "combinePartsFile", fileName);
        return -1;
    }

    buff = (char *)malloc(MAX_READ_ONCE);
    if (!buff)
    {
        COMMLOG(OBS_LOGERROR, "malloc failed.");
        close(fdDest);
        fdDest = -1;
        return -1;
    }

    while (partNode)
    {
        char pathToUpdate[1024];
        char contentToSet[32];
        writeSuccess = 1;

        if (partNode->downloadStatus == COMBINE_SUCCESS)
        {
#ifdef WIN32
            if (_lseeki64(fdDest, (long long int)partNode->part_size, SEEK_CUR) == -1)
            {
                free(buff);
                buff = NULL;
                return -1;
            }
#else
            if (lseek(fdDest, (long long int)partNode->part_size, SEEK_CUR) == -1)
            {
                free(buff);
                buff = NULL;
                return -1;
            }
#endif
            partNode = partNode->next;
            continue;
        }

        int ret = sprintf_s(fileNameTemp, ARRAY_LENGTH_1024, "%s.%d", fileName, partNode->part_num);
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);

#if defined WIN32
        Sleep(0);
        _sopen_s(&fdSrc, fileNameTemp, _O_BINARY | _O_RDONLY,
            _SH_DENYWR, _S_IREAD);
#endif

#if defined __GNUC__ || defined LINUX
        sleep(0);
        fdSrc = open(fileNameTemp, O_RDONLY);
#endif
        if (fdSrc == -1)
        {
            COMMLOG(OBS_LOGERROR, "%s open file[%s] failed\n", "combinePartsFile", fileNameTemp);
            close(fdDest);
            fdDest = -1;
            free(buff);
            buff = NULL;
            return -1;
        }

        remain_bytes = partNode->part_size;
        writeSuccess = combinePartsFileRead(remain_bytes, bytesToRead, bytesReadOut, bytesWritten, fdDest, fdSrc, buff, writeSuccess);
        close(fdSrc);
        fdSrc = -1;
        if (writeSuccess == 1)
        {
            combinePartsFileSuccess(partNode, check_point_file, pathToUpdate,
                contentToSet, xmlwrite_mutex, ret, fileNameTemp);

        }
        else
        {
            if (check_point_file == NULL)//break-point-continue down, is not enabled
            {
                removeTempFiles(fileName, downloadPartList, 0);
            }
            break;
        }
        partNode = partNode->next;
    }



    close(fdDest);
    fdDest = -1;
    free(buff);
    buff = NULL;
    //here, remove the last file, it may contains some parts combined in
    is_true = ((writeSuccess == 0) && (check_point_file == NULL));
    if (is_true)
    {
        (void)remove(fileName);
    }

    return 0;
}

int setDownloadReturnPartList(download_file_part_info * partListIntern,
    obs_download_file_part_info **partListReturn, int partCount)
{
    int i = 0;
    download_file_part_info * partInfoNode = partListIntern;
    obs_download_file_part_info * partListReturnTemp =
        (obs_download_file_part_info *)malloc(sizeof(obs_download_file_part_info)*partCount);
    if (partListReturnTemp == NULL)
    {
        return -1;
    }
    (*partListReturn) = partListReturnTemp;
    for (i = 0; i < partCount; i++)
    {
        partListReturnTemp->part_num = partInfoNode->part_num + 1;
        partListReturnTemp->part_size = partInfoNode->part_size;
        partListReturnTemp->start_byte = partInfoNode->start_byte;
        partListReturnTemp->status_return = partInfoNode->downloadStatus;
        partInfoNode = partInfoNode->next;
        partListReturnTemp++;
    }
    return 0;
}

int isObjectChanged(download_file_summary * infoNew, download_file_summary * infoOld)
{
    if ((infoNew->lastModify != infoOld->lastModify)
        || (infoNew->objectLength != infoOld->objectLength)
        || (infoNew->storage_class != infoOld->storage_class))
    {
        return 1;
    }

    if (strcmp(infoNew->etag, infoOld->etag))
    {
        return 1;
    }
    return 0;
}

int checkDownloadPartsInfo(download_file_part_info * downloadPartList)
{
    download_file_part_info * partNode = downloadPartList;
    download_file_part_info * partNodePrev = NULL;
    int isValid = 1;
    while (partNode)
    {
#if defined WIN32
        Sleep(0);
#endif

#if defined __GNUC__ || defined LINUX
        sleep(0);
#endif
        if (partNode->prev)
        {
            partNodePrev = partNode->prev;
            if ((partNodePrev->start_byte + partNodePrev->part_size) != partNode->start_byte)
            {
                isValid = 0;
                break;
            }
        }
        partNode = partNode->next;
    }
    return isValid;
}

int get_download_isfirst_time_setFile(obs_download_file_configuration *download_file_config,
    char *storeFile, int is_true, int retVal, const char *key, char *checkpointFile,
    int isFirstTime)
{
    is_true = ((download_file_config->downLoad_file == NULL)
        || (!strlen(download_file_config->downLoad_file)));
    if (is_true)
    {
        errno_t err = EOK;
        err = memcpy_s(storeFile, ARRAY_LENGTH_1024, key, strlen(key) + 1);
        CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    }
    else
    {
        errno_t err = EOK;
        err = memcpy_s(storeFile, ARRAY_LENGTH_1024, download_file_config->downLoad_file,
            strlen(download_file_config->downLoad_file) + 1);
        CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    }
    is_true = ((download_file_config->check_point_file != NULL)
        && (strlen(download_file_config->check_point_file) != 0));
    if (is_true)
    {
        errno_t err = EOK;
        err = memcpy_s(checkpointFile, ARRAY_LENGTH_1024, download_file_config->check_point_file,
            strlen(download_file_config->check_point_file) + 1);
        if (err != EOK)
        {
            COMMLOG(OBS_LOGWARN, "get_download_isfirst_time: memcpy_s failed!\n");
        }
    }
    else
    {
        memset_s(checkpointFile, LENGTH_1024, 0, LENGTH_1024);
    }
    if (download_file_config->enable_check_point)
    {
        retVal = setCheckPointFile(storeFile, checkpointFile, &isFirstTime, DOWNLOAD_FILE_INFO);
        if (retVal == -1)
        {
            download_file_config->enable_check_point = 0;
            isFirstTime = 1;
        }
    }
    return isFirstTime;
}

int get_download_isfirst_time_read(download_file_summary *downloadFileInfoOld,
    download_file_part_info** pstDownloadFilePartInfoList, int *partCount,
    download_file_summary *pdownLoadFileInfo, int isObjectModified,
    int isPatsInfoValid, char *checkpointFile, char *storeFile,
    int retVal, int is_true, int isFirstTime)
{
    retVal = readCheckpointFile_Download(downloadFileInfoOld,
        pstDownloadFilePartInfoList, partCount, checkpointFile);
    if (retVal == -1)
    {
        isFirstTime = 1;
        if (*pstDownloadFilePartInfoList != NULL)
        {
            cleanDownloadList(*pstDownloadFilePartInfoList);
            *pstDownloadFilePartInfoList = NULL;
        }
    }
    else
    {
        isObjectModified = isObjectChanged(pdownLoadFileInfo, downloadFileInfoOld);
        isPatsInfoValid = checkDownloadPartsInfo(*pstDownloadFilePartInfoList);
        is_true = ((isObjectModified) || (!isPatsInfoValid));
        if (is_true)
        {
            removeTempFiles(storeFile, *pstDownloadFilePartInfoList, 1);
            isFirstTime = 1;
            if (*pstDownloadFilePartInfoList != NULL)
            {
                cleanDownloadList(*pstDownloadFilePartInfoList);
                pstDownloadFilePartInfoList = NULL;
            }
        }
    }
    return isFirstTime;
}

static int get_download_isfirst_time(obs_download_file_configuration * download_file_config, char *storeFile,
    const char *key, char *checkpointFile, download_file_summary *pdownLoadFileInfo,
    download_file_part_info** pstDownloadFilePartInfoList, int *partCount)
{
    int isFirstTime = 1;
    int retVal = -1;
    int isObjectModified = 0;
    int isPatsInfoValid = 0;
    int is_true = 0;
    download_file_summary downLoadFileInfoOld;

    //2,set the file to store the object, and the checkpoint file
    isFirstTime = get_download_isfirst_time_setFile(download_file_config, storeFile, is_true,
        retVal, key, checkpointFile, isFirstTime);

    //3, read the content of the checkpoint file
    if (!download_file_config->enable_check_point)
    {
        return isFirstTime;
    }

    isFirstTime = get_download_isfirst_time_read(&downLoadFileInfoOld, pstDownloadFilePartInfoList,
        partCount, pdownLoadFileInfo, isObjectModified, isPatsInfoValid,
        checkpointFile, storeFile, retVal, is_true, isFirstTime);

    return isFirstTime;
}

void download_complete_handle_success(obs_download_file_configuration *download_file_config,
    download_file_part_info *pstDownloadFilePartInfoList, char *checkpointFile,
    obs_download_file_response_handler *handler, void *callback_data,
    const char *storeFile, int retVal, void *xmlwrite_mutex)
{
    char *pstCheckPoint = download_file_config->enable_check_point ? checkpointFile : NULL;
    COMMLOG(OBS_LOGINFO, "%s all parts download success\n", "DownloadFile");
    retVal = combinePartsFile(storeFile, pstDownloadFilePartInfoList,
        pstCheckPoint, xmlwrite_mutex);
    if (retVal == 0)
    {
        char strReturn[1024] = { 0 };
        int ret = sprintf_s(strReturn, ARRAY_LENGTH_1024, "DownloadFile %s success\n", storeFile);
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        COMMLOG(OBS_LOGINFO, "DonwloadFlie combine success\n");
        if (handler->download_file_callback)
        {
            handler->download_file_callback(OBS_STATUS_OK, strReturn, 0, NULL, callback_data);
        }
        remove(checkpointFile);
    }
    else
    {
        COMMLOG(OBS_LOGERROR, "DownloadFile combine failed\n");
        if (download_file_config->enable_check_point == 0)
        {
            removeTempFiles(storeFile, pstDownloadFilePartInfoList, 1);
        }
        if (handler->download_file_callback)
        {
            handler->download_file_callback(OBS_STATUS_InternalError,
                "DownloadFile combine failed\n", 0, NULL, callback_data);
        }
    }
}

void download_complete_handle_noSuccess(obs_download_file_configuration *download_file_config,
    download_file_part_info *pstDownloadFilePartInfoList, const char *storeFile,
    obs_download_file_response_handler *handler, void *callback_data,
    int partCount, int retVal)
{
    obs_download_file_part_info *partListReturn = NULL;
    COMMLOG(OBS_LOGERROR, "%s Not all parts download success \n", "DownloadFile");
    retVal = setDownloadReturnPartList(pstDownloadFilePartInfoList, &partListReturn, partCount);
    if ((retVal == 0) && (handler->download_file_callback))
    {
        handler->download_file_callback(OBS_STATUS_InternalError, "DownloadFile Not all parts download success\n",
            partCount, partListReturn, callback_data);
    }
    if (download_file_config->enable_check_point == 0)
    {
        removeTempFiles(storeFile, pstDownloadFilePartInfoList, 1);
    }
    if (partListReturn)
    {
        free(partListReturn);
        partListReturn = NULL;
    }
}

void download_complete_handle(download_file_part_info * pstPartInfoListDone,
    obs_download_file_configuration * download_file_config,
    char *checkpointFile, const char *storeFile,
    obs_download_file_response_handler *handler, void *callback_data,
    int partCount, void *xmlwrite_mutex)
{
    int retVal = -1;
    download_file_part_info * pstDownloadFilePartInfoList = NULL;

    pstDownloadFilePartInfoList = pstPartInfoListDone;
    if (isAllDownLoadPartsSuccess(pstDownloadFilePartInfoList))
    {
        download_complete_handle_success(download_file_config, pstDownloadFilePartInfoList,
            checkpointFile, handler, callback_data, storeFile, retVal, xmlwrite_mutex);
    }
    else
    {
        download_complete_handle_noSuccess(download_file_config, pstDownloadFilePartInfoList,
            storeFile, handler, callback_data, partCount, retVal);
    }

    if (pstDownloadFilePartInfoList)
    {
        cleanDownloadList(pstDownloadFilePartInfoList);
        pstDownloadFilePartInfoList = NULL;
    }

    return;
}

#ifdef WIN32
int download_file_win32(obs_download_file_configuration *download_file_config,
    download_file_part_info *pstPartInfoListDone, download_file_part_info *pstPartInfoListNotDone,
    download_params stDownloadParams, int partCountToProc, char* checkpointFile,
    obs_download_file_response_handler *handler, void *callback_data,
    char *storeFile, int partCount)
{
    CRITICAL_SECTION  mutexThreadCheckpoint;
    if (download_file_config->enable_check_point)
    {
        InitializeCriticalSection(&mutexThreadCheckpoint);
    }
    while (pstPartInfoListNotDone)
    {
        int ret = GetDownloadPartListToProcess(&pstPartInfoListDone, &pstPartInfoListNotDone,
            partCountToProc, &partCountToProc, download_file_config->task_num);
        if (ret < 0) {
            COMMLOG(OBS_LOGWARN, "GetDownloadPartListToProcess returns %d.", ret);
        }
        if (partCountToProc > 0)
        {
            startDownloadThreads(&stDownloadParams, pstPartInfoListNotDone, partCountToProc, callback_data, &mutexThreadCheckpoint);
        }
    }
    download_complete_handle(pstPartInfoListDone, download_file_config, checkpointFile, storeFile,
        handler, callback_data, partCount, &mutexThreadCheckpoint);
    if (download_file_config->enable_check_point)
    {
        DeleteCriticalSection(&mutexThreadCheckpoint);
    }
    return partCountToProc;
}
#endif 

#if defined __GNUC__ || defined LINUX
int download_file_linux(obs_download_file_configuration *download_file_config,
    download_file_part_info *pstPartInfoListDone, download_file_part_info *pstPartInfoListNotDone,
    download_params stDownloadParams, int partCountToProc, char* checkpointFile,
    obs_download_file_response_handler *handler, void *callback_data,
    char *storeFile, int partCount)
{
    pthread_mutex_t mutexThreadCheckpoint;
    if (download_file_config->enable_check_point)
    {
        pthread_mutex_init(&mutexThreadCheckpoint, NULL);
    }
    while (pstPartInfoListNotDone)
    {
        int ret = GetDownloadPartListToProcess(&pstPartInfoListDone, &pstPartInfoListNotDone,
            partCountToProc, &partCountToProc, download_file_config->task_num);
        if (ret < 0) {
            COMMLOG(OBS_LOGWARN, "GetDownloadPartListToProcess returns %d.", ret);
        }
        if (partCountToProc > 0)
        {
            startDownloadThreads(&stDownloadParams, pstPartInfoListNotDone, partCountToProc, callback_data, &mutexThreadCheckpoint);
        }
    }
    download_complete_handle(pstPartInfoListDone, download_file_config, checkpointFile, storeFile,
        handler, callback_data, partCount, &mutexThreadCheckpoint);
    if (download_file_config->enable_check_point)
    {
        pthread_mutex_destroy(&mutexThreadCheckpoint);
    }
    return partCountToProc;
}
#endif 


void download_file(const obs_options *options, char *key, char* version_id,
    obs_get_conditions *get_conditions,
    server_side_encryption_params *encryption_params,
    obs_download_file_configuration * download_file_config,
    obs_download_file_response_handler *handler, void *callback_data)
{
    download_file_summary downLoadFileInfo;
    int retVal = -1;
    char storeFile[1024] = { 0 };
    int isFirstTime = 1;
    download_file_part_info * pstDownloadFilePartInfoList = NULL;
    download_file_part_info * pstPartInfoListDone = NULL;
    download_file_part_info * pstPartInfoListNotDone = NULL;
    int partCount = 0;
    char checkpointFile[1024];
    int partCountToProc = 0;
    uint64_t part_size = 0;
    int is_true = 0;


    download_params stDownloadParams;
    COMMLOG(OBS_LOGERROR, "in DownloadFile download_file_config: partsize=%d ", download_file_config->part_size);

    memset_s(&downLoadFileInfo, sizeof(download_file_summary), 0, sizeof(download_file_summary));
    //get the info of the object
    obs_status ret_status = getObjectInfo(&downLoadFileInfo, options, key, version_id, encryption_params);
    if (OBS_STATUS_OK != ret_status)
    {
        COMMLOG(OBS_LOGERROR, "in DownloadFile Get object metadata failed(%d),bucket=%s, key=%s,version_id=%s",
            ret_status, options->bucket_options.bucket_name,
            key,
            version_id);
        (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
        return;
    }
    //if the storage is glacier, restore the object firstly
    if (downLoadFileInfo.storage_class == OBS_STORAGE_CLASS_GLACIER)
    {
        ret_status = restoreGlacierObject(options, key, version_id);
        if (OBS_STATUS_OK != ret_status)
        {
            COMMLOG(OBS_LOGERROR, "in DownloadFile restoreGlacierObject failed(%d).", ret_status);
            (void)(*(handler->response_handler.complete_callback))(ret_status, 0, callback_data);
            return;
        }
    }

    //2,set the file to store the object, and the checkpoint file
    isFirstTime = get_download_isfirst_time(download_file_config, storeFile, key, checkpointFile,
        &downLoadFileInfo, &pstDownloadFilePartInfoList, &partCount);

    is_true = ((download_file_config->part_size <= 0)
        || (download_file_config->part_size > MAX_PART_SIZE));
    part_size = is_true ? DEFAULT_PART_SIZE : download_file_config->part_size;
    part_size = part_size > downLoadFileInfo.objectLength ? downLoadFileInfo.objectLength : part_size;

    //set down load part list
    is_true = ((isFirstTime == 1) || (download_file_config->enable_check_point == 0));
    if (is_true)
    {
        retVal = setDownloadpartList(&downLoadFileInfo, part_size, &pstDownloadFilePartInfoList, &partCount);
        if (retVal == -1)
        {
            if (download_file_config->enable_check_point)
            {
                remove(checkpointFile);
            }
            return;
        }
    }

    is_true = ((isFirstTime == 1) && (download_file_config->enable_check_point == 1));
    if (is_true)
    {
        (void)writeCheckpointFile_Download(&downLoadFileInfo,
            pstDownloadFilePartInfoList, partCount, checkpointFile);
    }

    //divid the list
    (void)DividDownloadPartList(pstDownloadFilePartInfoList, &pstPartInfoListDone, &pstPartInfoListNotDone);

    //start thread to download
    memset_s(&stDownloadParams, sizeof(download_params), 0, sizeof(download_params));
    stDownloadParams.callBackData = callback_data;
    stDownloadParams.enable_check_point = download_file_config->enable_check_point;
    stDownloadParams.fileNameCheckpoint = checkpointFile;
    stDownloadParams.fileNameStore = storeFile;
    stDownloadParams.objectName = key;
    stDownloadParams.options = options;
    stDownloadParams.version_id = version_id;
    stDownloadParams.pstServerSideEncryptionParams = encryption_params;
    stDownloadParams.response_handler = &(handler->response_handler);
    stDownloadParams.get_conditions = get_conditions;
    download_file_config->task_num = download_file_config->task_num == 0 ? MAX_THREAD_NUM :
        download_file_config->task_num;
    partCountToProc = 0;

#ifdef WIN32
    partCountToProc = download_file_win32(download_file_config, pstPartInfoListDone, pstPartInfoListNotDone,
        stDownloadParams, partCountToProc, checkpointFile, handler, callback_data, storeFile, partCount);
#endif 

#if defined __GNUC__ || defined LINUX
    partCountToProc = download_file_linux(download_file_config, pstPartInfoListDone, pstPartInfoListNotDone,
        stDownloadParams, partCountToProc, checkpointFile, handler, callback_data, storeFile, partCount);
#endif
}