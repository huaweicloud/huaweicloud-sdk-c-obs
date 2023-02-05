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
#define LOG_CONF_MESSAGELEN 1024


static char OBS_LOG_PATH[OBS_LOG_PATH_LEN]={0};
static bool ONLY_SET_LOGCONF = true;

int set_obs_log_path(const char *log_path, bool only_set_log_conf)
{
    if( log_path == NULL || strlen(log_path)> OBS_LOG_PATH_LEN)
    {
        return 0;
    }
    memset_s(OBS_LOG_PATH,OBS_LOG_PATH_LEN,0,OBS_LOG_PATH_LEN);
    errno_t err = EOK;
    err = memcpy_s(OBS_LOG_PATH,OBS_LOG_PATH_LEN,log_path,strlen(log_path));
    if (err != EOK)
    {
        return 0;
    }
    ONLY_SET_LOGCONF = only_set_log_conf;
    return 1;
}

#ifdef WIN32
# pragma warning (disable:4127)
#endif
#if defined __GNUC__ || defined LINUX
int ReadModeleFile(FILE* fp , char* sPath,char* pTmpFullDir, char* pTmpModuleDir,
	char *sLine, int sLineLen,const char *sModuleName, unsigned int unSize)
{
	int iRet = -1;
	while (0 == feof(fp))
	{
		if (NULL == fgets(sLine, sLineLen, fp))
		{
			continue;
		}
		pTmpFullDir = strchr(sLine, '/');
		if (NULL == strstr(sLine, "r-xp") || NULL == pTmpFullDir || NULL == strstr(sLine, sModuleName))
		{
			continue;
		}
		pTmpModuleDir = strrchr(pTmpFullDir, '/');
		if (pTmpModuleDir == pTmpFullDir)
		{
			break;
		}
		*pTmpModuleDir = '\0';
		if (pTmpModuleDir == pTmpFullDir)
		{
			break;
		}
		iRet = 0;
		int ret = strncpy_s(sPath, unSize, pTmpFullDir, strlen(pTmpFullDir) + 1);
		if (ret) {
			return ret;
		}
		break;
	}
	return iRet;
}
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
	iRet = ReadModeleFile(fp, sPath, pTmpFullDir, pTmpModuleDir, sLine, 1024,sModuleName, unSize);
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
    errno_t err = strcpy_s(strPath, MAX_MSG_SIZE, buf);
    if (err != EOK) {
        NULLLOG();
    }
    return ;
}
#endif

int SetSectionForSearch(char* Sect, size_t Sectlen, const char* Section, char* linebuf) 
{
	int ret = strcpy_s(Sect, Sectlen, "[");
	if (ret != 0) {
		CHECK_NULL_FREE(linebuf);
		return ret;
	}
	ret = strcat_s(Sect, Sectlen, Section);
	if (ret != 0) {
		CHECK_NULL_FREE(linebuf);
		return ret;
	}
	ret = strcat_s(Sect, Sectlen, "]");
	if (ret != 0) {
		CHECK_NULL_FREE(linebuf);
		return ret;
	}
	return 0;
}

void SearchSection(char* linebuf, char* Sect, const char* Item, char* iniValue, FILE* inifp) 
{
	int isSection = 0;
	char* iniItem = NULL;
	while (NULL != fgets(linebuf, MAX_MSG_SIZE, inifp))
	{
		linebuf[MAX_MSG_SIZE - 1] = '\0';
		
		if ('[' == linebuf[0])
		{
			if (strstr(linebuf, Sect))
			{
				isSection = 1;
			}else {
				isSection = 0;
			}
			continue;
		}

		if (!isSection || ';' == linebuf[0] || !strstr(linebuf, Item))
		{
			continue;
		}

		if ((iniItem = strchr(linebuf, '=')) != NULL)
		{
			iniItem++;
			while ('\n' != *iniItem && '\r' != *iniItem && '\0' != *iniItem)
			{
				*iniValue = *iniItem;
				iniItem++;
				iniValue++;
			}
			break;
		}
	}
	*iniValue = '\0';
}

void GetIniSectionItem(const char* Section, const char* Item, const char* FileName, char* iniValue)
{
    char Sect[30] = {0};
    char* linebuf = NULL;
    FILE* inifp = NULL;

    linebuf = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == linebuf)
    {
        return;
    }
    linebuf[0] = '\0';

	int ret = SetSectionForSearch(Sect, sizeof(Sect), Section, linebuf);
	if (ret)
	{
		return;
	}

    inifp = fopen(FileName, "rb");
    if(NULL == inifp)
    {
        *iniValue = '\0';
        CHECK_NULL_FREE(linebuf);
        return;
    }

	SearchSection(linebuf, Sect, Item, iniValue, inifp);

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

int GET_LOG_PATH(char *logPath, const char *tempLogPath) {
    errno_t err = EOK;
    if (0 != strlen(tempLogPath))
    {
        if(tempLogPath[0] != '.'){
            size_t logPathLen = strlen(logPath);
            memset_s(logPath, logPathLen, 0, logPathLen);
        }else{
            
        }
        err = strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, tempLogPath);
    }
    else
    {
        err = strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, "logs");
    }
    return err;
}

int copy_file(char *source, char *target)
{
    int ret = 0;
    FILE *fp_src = fopen(source, "r");
    if (fp_src == NULL)
    {
        return -1;
    }
    FILE *fp_tar = fopen(target, "w");
    if (fp_tar == NULL)
    {
        return -1;
    }
    char* temp_arr = (char*)malloc(sizeof(char)*LOG_CONF_MESSAGELEN);

    while (fgets(temp_arr, LOG_CONF_MESSAGELEN, fp_src))
    {
        if(strstr(temp_arr, "LogPath=") != NULL)
        {
            ret = snprintf_s(temp_arr, LOG_CONF_MESSAGELEN, LOG_CONF_MESSAGELEN - 1, "LogPath=\.\/");
            CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
        }
            fputs(temp_arr, fp_tar);
    }
    fclose(fp_src);
    fclose(fp_tar);
    free(temp_arr);
    return ret;
}

int MoveConf(const char * buf)
{
    errno_t err = EOK;
    int ret = 0;
    char* source_conf = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    char* target_conf = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if ((source_conf == NULL) || (target_conf == NULL))
    {
        return -1;
    }
    err = memset_s(source_conf, MAX_MSG_SIZE, 0, MAX_MSG_SIZE);
    CHECK_ERR_RETURN(err);
    err = memset_s(target_conf, MAX_MSG_SIZE, 0, MAX_MSG_SIZE);
    CHECK_ERR_RETURN(err);
#ifdef WIN32
    GetModuleFileNameA(NULL, source_conf, MAX_MSG_SIZE - 1);
    char* chr = strrchr(source_conf, '\\');
    if (NULL != chr) {
        *(chr + 1) = '\0';
    }
    ret = snprintf_s(target_conf, MAX_MSG_SIZE, MAX_MSG_SIZE - 1, "%s\\OBS.ini", buf);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
    err = strcat_s(source_conf, MAX_MSG_SIZE, "OBS.ini");
    CHECK_ERR_RETURN(err);
#else
    getCurrentPath(source_conf);
    ret = snprintf_s(target_conf, MAX_MSG_SIZE, MAX_MSG_SIZE - 1, "%s\/OBS.ini", buf);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
    err = strcat_s(source_conf, MAX_MSG_SIZE, "\/OBS.ini");
    CHECK_ERR_RETURN(err);
#endif
    copy_file(source_conf, target_conf);
    free(source_conf);
    free(target_conf);
    return 0;
}

int GetConfPath(char *buf)
{
    errno_t err = EOK;
    bool log_path = (OBS_LOG_PATH[0] != 0) ? true : false;
    if (log_path) {
        err = memcpy_s(buf, sizeof(char)*MAX_MSG_SIZE, OBS_LOG_PATH, MAX_MSG_SIZE);
        CHECK_ERR_RETURN(err);
    }
    else
    {
#if defined __GNUC__ || defined LINUX
        getCurrentPath(buf);
#else
        GetModuleFileNameA(NULL, buf, MAX_MSG_SIZE - 1);
#endif
    }
    if (log_path && (!ONLY_SET_LOGCONF))
    {
        MoveConf(buf);
    }
}

int SetConfPath(char* currentPath, char* buf, char* confPath, char* logPath, char* tempLogPath) 
{
	errno_t err = EOK;
	if (NULL == currentPath)
	{
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		return -1;
	}
	memset_s(currentPath, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE * sizeof(char));

	err = memcpy_s(currentPath, sizeof(char)*MAX_MSG_SIZE, buf, MAX_MSG_SIZE);
	if (err != EOK)
	{
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		CHECK_NULL_FREE(currentPath);
		return -1;
	}
	char* chr = strrchr(currentPath, '\\');
	if (NULL != chr) {
		*(chr + 1) = '\0';
	}
	err = memcpy_s(logPath, sizeof(char)*MAX_MSG_SIZE, currentPath, MAX_MSG_SIZE);
	if (err != EOK)
	{
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		CHECK_NULL_FREE(currentPath);
		return -1;
	}
	err = memcpy_s(confPath, sizeof(char)*MAX_MSG_SIZE, currentPath, MAX_MSG_SIZE);
	if (err != EOK)
	{
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		CHECK_NULL_FREE(currentPath);
		return -1;
	}
	int ret = strcat_s(confPath, sizeof(char)*MAX_MSG_SIZE, "OBS.ini");
	if (ret != 0) {
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		CHECK_NULL_FREE(currentPath);
		return -1;
	}

	GetIniSectionItem(SECTION_PATH, PATH_VALUE, confPath, tempLogPath);
	tempLogPath[MAX_MSG_SIZE - 1] = '\0';

	ret = GET_LOG_PATH(logPath, tempLogPath);
	if (ret != 0) {
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		CHECK_NULL_FREE(currentPath);
		return -1;
	}

	return 0;
}

int LOG_INIT()
{
    unsigned int *logLevel = (unsigned int*)malloc(sizeof(unsigned int)*LOG_CATEGORY);
    if (NULL == logLevel)
    {
        return -1;
    }else {
        for (size_t i = 0; i < LOG_CATEGORY; ++i) {
            logLevel[i] = INVALID_LOG_LEVEL;
        }
    }
    char* buf = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == buf)
    {
        CHECK_NULL_FREE(logLevel);
        return -1;
    }
    memset_s(buf, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE*sizeof(char));

    char* confPath = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == confPath)
    {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(logLevel);
        return -1;
    }
    memset_s(confPath, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE*sizeof(char));

    char* logPath = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == logPath)
    {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logLevel);
        return -1;
    }
    memset_s(logPath, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE*sizeof(char));

    char* tempLogPath = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
    if (NULL == tempLogPath)
    {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        CHECK_NULL_FREE(logLevel);
        return -1;
    }
    memset_s(tempLogPath, sizeof(char)*MAX_MSG_SIZE, 0, MAX_MSG_SIZE*sizeof(char));

#if defined __GNUC__ || defined LINUX
    GetConfPath(buf);
    if( OBS_LOG_PATH[0] != 0 ) {
        memcpy_s(buf, sizeof(char)*MAX_MSG_SIZE, OBS_LOG_PATH, MAX_MSG_SIZE);
    }
    else {        
        getCurrentPath(buf);
    }
    memcpy_s(confPath, sizeof(char)*MAX_MSG_SIZE, buf, MAX_MSG_SIZE);
    strcat_s(confPath, sizeof(char)*MAX_MSG_SIZE, "/OBS.ini");
	
	FILE* confPathFile = fopen(confPath, "r");
    if(NULL == confPathFile) 
    {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        CHECK_NULL_FREE(logLevel);
        CHECK_NULL_FREE(tempLogPath);
        return -1;
    }
    fclose(confPathFile);
	
    GetIniSectionItem(SECTION_PATH, PATH_VALUE, confPath, tempLogPath);
    tempLogPath[MAX_MSG_SIZE - 1] = '\0';
    memcpy_s(logPath, sizeof(char)*MAX_MSG_SIZE, buf, MAX_MSG_SIZE);

    errno_t err = EOK;
    err = strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, "/");
    err = GET_LOG_PATH(logPath, tempLogPath);
    if (err != EOK) {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        CHECK_NULL_FREE(logLevel);
        CHECK_NULL_FREE(tempLogPath);
        return -1;
    }
#else
	errno_t err =EOK;
    GetConfPath(buf);

    char* currentPath = (char*)malloc(sizeof(char)*MAX_MSG_SIZE);
	int ret = SetConfPath(currentPath, buf, confPath, logPath, tempLogPath);
	if (ret)
	{
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        CHECK_NULL_FREE(logLevel);
        CHECK_NULL_FREE(tempLogPath);
        CHECK_NULL_FREE(currentPath);
		return ret;
	}
    CHECK_NULL_FREE(currentPath);

#endif

  //  tempLogPath[0] = '\0';
  //  GetIniSectionItem("ProductConfig", "support_API", confPath, tempLogPath);
    int iRet = LogInit(PRODUCT, confPath, logLevel, logPath);
    CHECK_NULL_FREE(buf);
    CHECK_NULL_FREE(confPath);
    CHECK_NULL_FREE(logPath);
    CHECK_NULL_FREE(logLevel);
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
    int ret = vsnprintf_s(acMsg, sizeof(acMsg), MAX_LOG_SIZE - 1, pszFormat, pszArgp);
    va_end(pszArgp);
    if (ret < 0) {
        return;
    }

    if(level == OBS_LOGDEBUG)
    {
        (void)Log_Run_Debug(PRODUCT,acMsg, sizeof(acMsg));
    }
    else if(level == OBS_LOGINFO)
    {
        (void)Log_Run_Info(PRODUCT,acMsg, sizeof(acMsg));
    }
    else if(level == OBS_LOGWARN)
    {
        (void)Log_Run_Warn(PRODUCT,acMsg, sizeof(acMsg));
    }
    else if(level == OBS_LOGERROR)
    {
        (void)Log_Run_Error(PRODUCT,acMsg, sizeof(acMsg));
    }
}

void NULLLOG() {
    return;
}

void CheckAndLogNoneZero(int ret, const char* name, const char* funcName, unsigned long line) {
    if (ret != 0) {
        COMMLOG(OBS_LOGWARN, "%s failed in %s.(%ld)", name, funcName, line);
    }
}

void CheckAndLogNeg(int ret, const char* name, const char* funcName, unsigned long line) {
    if (ret < 0) {
        COMMLOG(OBS_LOGWARN, "%s failed in %s.(%ld)", name, funcName, line);
    }
}

