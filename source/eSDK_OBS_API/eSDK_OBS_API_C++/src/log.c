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
#include "log.h"
#include "securec.h"
#include <stdlib.h>

#ifdef WIN32
#define strcasecmp _stricmp
#define strncasecmp  _strnicmp
#else
#include <strings.h>
#endif
#include "eSDKOBS.h"

#define SECTION_PATH     "LogPath"
#define PATH_VALUE       "LogPath"
#define OBS_LOG_PATH_LEN   257


static char OBS_LOG_PATH[OBS_LOG_PATH_LEN]={0};

int set_obs_log_path(char *log_path)
{
    if( log_path == NULL || strlen(log_path)> OBS_LOG_PATH_LEN)
    {
        return 0;
    }
    memset_s(OBS_LOG_PATH,OBS_LOG_PATH_LEN,0,OBS_LOG_PATH_LEN);
    memcpy_s(OBS_LOG_PATH,OBS_LOG_PATH_LEN,log_path,strlen(log_path));
    return 1;
}

#ifdef WIN32
# pragma warning (disable:4127)
#endif
#if defined __GNUC__ || defined LINUX
int GetModuleFilePath(const char* sModuleName, char* sPath, unsigned int unSize)
{
    int iRet = -1;
    char sLine[1024] = {0};
    FILE *fp = NULL;
    char *pTmpFullDir = NULL;
    char *pTmpModuleDir = NULL;

    fp = fopen ("/proc/self/maps", "r");
    if (NULL == fp)
    {
        return iRet;
    }
    while (0 == feof(fp))
    {
        if (NULL == fgets(sLine, sizeof(sLine), fp))
        {
            continue;
        }
        
        pTmpFullDir = strchr(sLine, '/');
        if (NULL == strstr(sLine, "r-xp") ||  NULL == pTmpFullDir || NULL == strstr(sLine, sModuleName))
        {
            continue;
        }

        pTmpModuleDir = strrchr(pTmpFullDir, '/');   
        if (pTmpModuleDir == pTmpFullDir)
        {
        break;
        }
        *pTmpModuleDir = '\0';
        if (strlen(pTmpFullDir) >= unSize)
        {
            break;
        }
        iRet = 0;
        strncpy_sec(sPath, unSize, pTmpFullDir, strlen(pTmpFullDir) + 1);
        break;
    }
    fclose(fp);
    return iRet;
}

void getCurrentPath(char *strPath)
{
    const char separator = '/';
    char buf[1024] = {0};
    int nRet = -1;
    nRet = GetModuleFilePath("libeSDKLogAPI.so", buf, sizeof(buf));
    if(0 != nRet)
    {
        // print log;
        return ;
    }

    strcat_s(buf, sizeof(buf), "/");
    char* pch = strrchr(buf, separator);
    if(NULL == pch)
    {
        return ;
    }
    if(pch == buf)
    {
        return ;
    }
    *pch = '\0';
    strcpy_s(strPath, MAX_MSG_SIZE, buf);
    return ;
}
#endif

void GetIniSectionItem(const char* Section, const char* Item, const char* FileName, char* iniValue)
{
    char* iniItem = NULL;
    char Sect[30] = {0};
    int isSection = 0;
    char* linebuf = NULL;
    FILE* inifp = NULL;

    linebuf = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == linebuf)
    {
        return;
    }
    linebuf[0] = '\0';

    strcpy_s(Sect, sizeof(Sect), "[");
    strcat_s(Sect, sizeof(Sect), Section);
    strcat_s(Sect, sizeof(Sect), "]");

    inifp = fopen(FileName, "rb");
    if(NULL == inifp)
    {
        *iniValue = '\0';
        CHECK_NULL_FREE(linebuf);
        return;
    }

    while(NULL != fgets(linebuf, MAX_MSG_SIZE, inifp))
    {
        linebuf[MAX_MSG_SIZE-1] = '\0';

        if('[' == linebuf[0])
        {
            if(strstr(linebuf, Sect))
            {
                isSection = 1;
            } else {
                isSection = 0;
            }
            continue;
        }

        if(!isSection || ';' == linebuf[0] || !strstr(linebuf, Item))
        {
            continue;
        }

        if((iniItem = strchr(linebuf, '=')) != NULL)
        {
            iniItem++;
            while('\n' != *iniItem && '\r' != *iniItem && '\0' != *iniItem) 
            {
                *iniValue = *iniItem;
                iniItem++;
                iniValue++;
            }
            break;
        }
    }

    *iniValue = '\0';
    fclose(inifp);
    CHECK_NULL_FREE(linebuf);

    return;
}
#if defined __GNUC__ || defined LINUX
void itoa(int i, char*string)
{
    int power;
    int j;

    j=i;
    for(power=1; j>=10; j /= 10)
    {
        power*=10;
    }

    for(;power>0;power/=10)
    {
        *string++= '0' + i/power;
        i%=power;
    }

    *string='\0';
}
#endif

int LOG_INIT()
{
    unsigned int logLevel[LOG_CATEGORY] = {INVALID_LOG_LEVEL, 
    INVALID_LOG_LEVEL,
    INVALID_LOG_LEVEL };
    char* buf = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == buf)
    {
        return -1;
    }
    memset_s(buf, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE*sizeof(char));

    char* confPath = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == confPath)
    {
        CHECK_NULL_FREE(buf);
        return -1;
    }
    memset_s(confPath, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE*sizeof(char));

    char* logPath = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == logPath)
    {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        return -1;
    }
    memset_s(logPath, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE*sizeof(char));

    char* tempLogPath = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == tempLogPath)
    {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        return -1;
    }
    memset_s(tempLogPath, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE*sizeof(char));

#if defined __GNUC__ || defined LINUX
    if( OBS_LOG_PATH[0] != 0 ) {
        memcpy_s(buf, sizeof(char)*MAX_MSG_SIZE, OBS_LOG_PATH, MAX_MSG_SIZE);
    }
    else {        
        getCurrentPath(buf);
    }
    memcpy_s(confPath, sizeof(char)*MAX_MSG_SIZE, buf, MAX_MSG_SIZE);
    strcat_s(confPath, sizeof(char)*MAX_MSG_SIZE, "/OBS.ini");
    if(NULL == fopen(confPath, "r")) {
        return -1;
    }
    GetIniSectionItem(SECTION_PATH, PATH_VALUE, confPath, tempLogPath);
    tempLogPath[MAX_MSG_SIZE - 1] = '\0';
    memcpy_s(logPath, sizeof(char)*MAX_MSG_SIZE, buf, MAX_MSG_SIZE);

    strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, "/");
    if(0 != strlen(tempLogPath))
    {
        strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, tempLogPath);
    }
    else
    {
        strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, "logs");
    }
#else
    if( OBS_LOG_PATH[0] != 0 )  {
        memcpy_s(buf, sizeof(char)*MAX_MSG_SIZE, OBS_LOG_PATH, MAX_MSG_SIZE);
    }
    else {
        GetModuleFileNameA(NULL,buf,MAX_MSG_SIZE-1);
    }

    char* currentPath = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == currentPath)
    {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        CHECK_NULL_FREE(tempLogPath);
        return -1;
    }
    memset_s(currentPath, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE*sizeof(char));

    memcpy_s(currentPath, sizeof(char)*MAX_MSG_SIZE, buf, MAX_MSG_SIZE);
    if(NULL != strrchr(currentPath, '\\')){ 
        *(strrchr(currentPath, '\\') + 1) = '\0'; 
    }
    memcpy_s(logPath, sizeof(char)*MAX_MSG_SIZE, currentPath, MAX_MSG_SIZE);
    memcpy_s(confPath, sizeof(char)*MAX_MSG_SIZE, currentPath, MAX_MSG_SIZE);
    strcat_s(confPath, sizeof(char)*MAX_MSG_SIZE, "OBS.ini");

    GetIniSectionItem(SECTION_PATH, PATH_VALUE, confPath, tempLogPath);
    tempLogPath[MAX_MSG_SIZE - 1] = '\0';

    if(0 != strlen(tempLogPath))
    {
        strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, tempLogPath);
    }
    else
    {
        strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, "logs");
    }
    CHECK_NULL_FREE(currentPath);

#endif

  //  tempLogPath[0] = '\0';
  //  GetIniSectionItem("ProductConfig", "support_API", confPath, tempLogPath);
    int iRet = LogInit(PRODUCT, confPath, logLevel, logPath);
    CHECK_NULL_FREE(buf);
    CHECK_NULL_FREE(confPath);
    CHECK_NULL_FREE(logPath);
    CHECK_NULL_FREE(tempLogPath);

    if(iRet)
    {
        return -1;
    }
    return 0;
}

void LOG_EXIT()
{
    LogFini(PRODUCT);
    return ;
}

void COMMLOG(OBS_LOGLEVEL level, const char *pszFormat, ...)
{
    va_list pszArgp;
    const char *tempFormat = pszFormat;
    if (NULL == tempFormat)
    {
        return;
    }
    va_start(pszArgp, pszFormat);
    char acMsg[MAX_LOG_SIZE] = {0};
    vsnprintf_sec(acMsg, sizeof(acMsg), MAX_LOG_SIZE - 1, pszFormat, pszArgp);
    va_end(pszArgp);

    if(level == OBS_LOGDEBUG)
    {
        (void)Log_Run_Debug(PRODUCT,acMsg);
    }
    else if(level == OBS_LOGINFO)
    {
        (void)Log_Run_Info(PRODUCT,acMsg);
    }
    else if(level == OBS_LOGWARN)
    {
        (void)Log_Run_Warn(PRODUCT,acMsg);
    }
    else if(level == OBS_LOGERROR)
    {
        (void)Log_Run_Error(PRODUCT,acMsg);
    }
}


