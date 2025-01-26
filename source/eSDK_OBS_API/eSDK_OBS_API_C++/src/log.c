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
#include "file_utils.h"
#include "securec.h"
#include <stdlib.h>

#ifdef WIN32
#define strcasecmp _stricmp
#define strncasecmp  _strnicmp
#else
#include <strings.h>
#include <sys/time.h>
#include <pthread.h>
#endif
#include "eSDKOBS.h"

#define SECTION_PATH     "LogPath"
#define PATH_VALUE       "LogPath"
#define OBS_LOG_PATH_LEN   257
#define LOG_CONF_MESSAGELEN 1024

static char USER_SET_OBS_LOG_PATH[OBS_LOG_PATH_LEN]={0};
static bool ONLY_SET_LOGCONF = true;
#if defined (WIN32)
#include <fcntl.h>
#include <io.h>
static wchar_t USER_SET_OBS_LOG_PATH_W[OBS_LOG_PATH_LEN] = { 0 };

int set_obs_log_path_w(const wchar_t *log_path, bool only_set_log_conf)
{
	if (log_path == NULL || wcslen(log_path) > OBS_LOG_PATH_LEN)
	{
		return 0;
	}
	memset_s(USER_SET_OBS_LOG_PATH_W, OBS_LOG_PATH_LEN * sizeof(wchar_t), 0, OBS_LOG_PATH_LEN * sizeof(wchar_t));
	errno_t err = EOK;
	err = memcpy_s(USER_SET_OBS_LOG_PATH_W, OBS_LOG_PATH_LEN * sizeof(wchar_t), log_path, wcslen(log_path) * sizeof(wchar_t));
	if (err != EOK)
	{
		return 0;
	}
	ONLY_SET_LOGCONF = only_set_log_conf;
	return 1;
}
#endif

int set_obs_log_path_c(const char *log_path, bool only_set_log_conf)
{
	if (log_path == NULL || strlen(log_path) > OBS_LOG_PATH_LEN)
	{
		return 0;
	}
	memset_s(USER_SET_OBS_LOG_PATH, OBS_LOG_PATH_LEN, 0, OBS_LOG_PATH_LEN);
	errno_t err = EOK;
	err = memcpy_s(USER_SET_OBS_LOG_PATH, OBS_LOG_PATH_LEN, log_path, strlen(log_path));
	if (err != EOK)
	{
		return 0;
	}
	ONLY_SET_LOGCONF = only_set_log_conf;
	return 1;
}

int set_obs_log_path(const char *log_path, bool only_set_log_conf)
{
#if defined (WIN32)
	if (get_file_path_code() == UNICODE_CODE) {
		return set_obs_log_path_w((const wchar_t *)log_path, only_set_log_conf);
	}
	else if(get_file_path_code() == ANSI_CODE) {
		return set_obs_log_path_c(log_path, only_set_log_conf);
	} 
	else {
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return 0;
	}
#else
	return set_obs_log_path_c(log_path, only_set_log_conf);
#endif
}

void LogError(const char* name, const char* funcName, unsigned long line, OBS_LOGLEVEL logLevel) {
	COMMLOG(logLevel, "%s failed in function: %s, line (%ld)!", name, funcName, line);
}
int CheckAndLogNULL(const void* ptr, const char* ptrName, const char* name, const char* funcName, unsigned long line) {
	if (ptr == NULL) {
		COMMLOG(OBS_LOGERROR, "%s is NULL!", ptrName);
		LogError(name, funcName, line, OBS_LOGERROR);
		return 0;
	}
	else {
		return 1;
	}
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
		if (pTmpModuleDir == pTmpFullDir || pTmpModuleDir == NULL)
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
		checkAndLogStrError(SYMBOL_NAME_STR(fopen), __FUNCTION__, __LINE__);
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
void SearchSection_C(char* linebuf, char* Sect, const char* Item, char* iniValue, FILE* inifp) 
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
#ifdef WIN32
void SearchSection_W(wchar_t* linebuf, wchar_t* Sect, const wchar_t* Item, wchar_t* iniValue, FILE* inifp) {

	int isSection = 0;
	wchar_t* iniItem = NULL;

	int oldMode = _setmode(_fileno(inifp), _O_U8TEXT);
	while (NULL != fgetws(linebuf, MAX_MSG_SIZE, inifp))
	{
		linebuf[MAX_MSG_SIZE - 1] = L'\0';

		if (L'[' == linebuf[0])
		{
			if (wcsstr(linebuf, Sect))
			{
				isSection = 1;
			}
			else {
				isSection = 0;
			}
			continue;
		}

		if (!isSection || L';' == linebuf[0] || !wcsstr(linebuf, Item))
		{
			continue;
		}

		if ((iniItem = wcschr(linebuf, L'=')) != NULL)
		{
			iniItem++;
			while (L'\n' != *iniItem && L'\r' != *iniItem && L'\0' != *iniItem)
			{
				*iniValue = *iniItem;
				iniItem++;
				iniValue++;
			}
			break;
		}
	}
	*iniValue = L'\0';
	(void)_setmode(_fileno(inifp), oldMode);
}
#endif

void SearchSection(char* linebuf, char* Sect, const char* Item, char* iniValue, FILE* inifp) {
#ifdef WIN32
	if (get_file_path_code() == ANSI_CODE)
	{
		SearchSection_C(linebuf, Sect, Item, iniValue, inifp);
	}
	else if (get_file_path_code() == UNICODE_CODE)
	{
		wchar_t* Sect_W = GetWcharFromChar(Sect);
		wchar_t* Item_W = GetWcharFromChar(Item);
		if (Sect_W == NULL || Item_W == NULL) {
			COMMLOG(OBS_LOGERROR, "GerWcharFromChar failed, function %s failed", __FUNCTION__);
			CHECK_NULL_FREE(Sect_W);
			CHECK_NULL_FREE(Item_W);
			return;
		}
		SearchSection_W((wchar_t*)linebuf, Sect_W, Item_W, (wchar_t*)iniValue, inifp);
		CHECK_NULL_FREE(Sect_W);
		CHECK_NULL_FREE(Item_W);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
	}
#else
	SearchSection_C(linebuf, Sect, Item, iniValue, inifp);
#endif // WIN32
}

void GetIniSectionItem(const char* Section, const char* Item, const char* FileName, char* iniValue)
{
    char Sect[30] = {0};
    char* linebuf = NULL;
    FILE* inifp = NULL;

	linebuf = getPathBuffer(MAX_MSG_SIZE);
    if (NULL == linebuf)
    {
        return;
    }

	errno_t ret = SetSectionForSearch(Sect, sizeof(Sect), Section, linebuf);
	if (ret)
	{
		return;
	}
	ret = file_fopen_s(&inifp, FileName, "r");
    if(NULL == inifp || ret != 0)
    {
        *iniValue = '\0';
        CHECK_NULL_FREE(linebuf);
		if (NULL != inifp) {
			fclose(inifp);
		}
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
int GET_LOG_PATH_C(char *logPath, const char *tempLogPath) {
	errno_t err = EOK;
	if (0 != strlen(tempLogPath))
	{
		if (tempLogPath[0] != '.') {
			size_t logPathLen = strlen(logPath);
			memset_s(logPath, logPathLen, 0, logPathLen);
		}
		else {

		}
		err = strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, tempLogPath);
	}
	else
	{
		err = strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, "logs");
	}
	return err;
}
int GET_LOG_PATH_W(wchar_t *logPath, const wchar_t *tempLogPath) {
	errno_t err = EOK;
	if (0 != wcslen(tempLogPath))
	{
		if (tempLogPath[0] != L'.') {
			size_t logPathLen = wcslen(logPath);
			memset_s(logPath, logPathLen * sizeof(wchar_t), 0, logPathLen * sizeof(wchar_t));
		}
		else {

		}
		err = wcscat_s(logPath, sizeof(char)*MAX_MSG_SIZE, tempLogPath);
	}
	else
	{
		err = wcscat_s(logPath, sizeof(char)*MAX_MSG_SIZE, L"logs");
	}
	return err;
}
int GET_LOG_PATH(char *logPath, const char *tempLogPath) {
    errno_t err = EOK;
#ifdef WIN32
	if (get_file_path_code() == ANSI_CODE) {
		err = GET_LOG_PATH_C(logPath, tempLogPath);
	}
	else if (get_file_path_code() == UNICODE_CODE) {
		err = GET_LOG_PATH_W((wchar_t*)logPath, (const wchar_t*)tempLogPath);
	}
#else
	err = GET_LOG_PATH_C(logPath, tempLogPath);
#endif // WIN32

    return err;
}

int copy_file(char *source, char *target)
{
    int ret = 0; 
	FILE *fp_src = NULL;
	file_fopen_s(&fp_src, source, "r");
    if (fp_src == NULL)
    {
        return -1;
    }
	FILE *fp_tar = NULL;
	file_fopen_s(&fp_tar, target, "w");
    if (fp_tar == NULL)
    {
    	fclose(fp_src);
        return -1;
    }
    char* temp_arr = (char*)malloc(sizeof(char)*LOG_CONF_MESSAGELEN);
    if (!CheckAndLogNULL(temp_arr, SYMBOL_NAME_STR(temp_arr), SYMBOL_NAME_STR(malloc), __FUNCTION__, __LINE__))
    {
    	fclose(fp_src);
    	fclose(fp_tar);
        return -1;
    }

    while (fgets(temp_arr, LOG_CONF_MESSAGELEN, fp_src))
    {
        if(strstr(temp_arr, "LogPath=") != NULL)
        {
            ret = snprintf_s(temp_arr, LOG_CONF_MESSAGELEN, LOG_CONF_MESSAGELEN - 1, "LogPath=./");
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
    char* source_conf = getPathBuffer(MAX_MSG_SIZE);
    char* target_conf = getPathBuffer(MAX_MSG_SIZE);
    if ((source_conf == NULL) || (target_conf == NULL))
    {
        CHECK_NULL_FREE(source_conf);
		CHECK_NULL_FREE(target_conf);
        return -1;
    }
	char* strcat_name=SYMBOL_NAME_STR(strcat_s);
#ifdef WIN32
	if (get_file_path_code() == ANSI_CODE) {
		GetModuleFileNameA(NULL, source_conf, MAX_MSG_SIZE - 1);
		char* chr = strrchr(source_conf, '\\');
		if (NULL != chr) {
			*(chr + 1) = '\0';
		}
		ret = snprintf_s(target_conf, MAX_MSG_SIZE, MAX_MSG_SIZE - 1, "%s\\OBS.ini", buf);
		CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
		err = strcat_s(source_conf, MAX_MSG_SIZE, "OBS.ini");
	}
	else if (get_file_path_code() == UNICODE_CODE) {
		GetModuleFileNameW(NULL, (wchar_t*)source_conf, MAX_MSG_SIZE - 1);
		wchar_t* chr = wcsrchr((wchar_t*)source_conf, L'\\');
		if (NULL != chr) {
			*(chr + 1) = L'\0';
		}
		ret = _snwprintf_s((wchar_t*)target_conf, MAX_MSG_SIZE, MAX_MSG_SIZE - 1, L"%s\\OBS.ini", (wchar_t*)buf);
		CheckAndLogNeg(ret, "_snwprintf_s", __FUNCTION__, __LINE__);
		err = wcscat_s((wchar_t*)source_conf, MAX_MSG_SIZE, L"OBS.ini");
		strcat_name=SYMBOL_NAME_STR(wcscat_s);
	}
	else {
		return -1;
	}
#else
    getCurrentPath(source_conf);
    ret = snprintf_s(target_conf, MAX_MSG_SIZE, MAX_MSG_SIZE - 1, "%s/OBS.ini", buf);
    CheckAndLogNeg(ret, "snprintf_s", __FUNCTION__, __LINE__);
    err = strcat_s(source_conf, MAX_MSG_SIZE, "/OBS.ini");
#endif
    if(checkIfErrorAndLogStrError(strcat_name, __FUNCTION__, __LINE__, err)) {
		CHECK_NULL_FREE(source_conf);
		CHECK_NULL_FREE(target_conf);
		return -1;
	} else {
    	copy_file(source_conf, target_conf);
		CHECK_NULL_FREE(source_conf);
		CHECK_NULL_FREE(target_conf);
    	return 0;
	}
}

bool isUserSetLogPath() {
#if defined (WIN32)
	if (get_file_path_code() == ANSI_CODE)
	{
		return strlen(USER_SET_OBS_LOG_PATH) != 0;
	}
	else if (get_file_path_code() == UNICODE_CODE)
	{
		return wcslen(USER_SET_OBS_LOG_PATH_W) != 0;
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return false;
	}
#else
	return strlen(USER_SET_OBS_LOG_PATH) != 0;
#endif
}

int GetConfPath(char *buf)
{
    errno_t err = EOK;
	bool ifUserSetLogPath = isUserSetLogPath();
    if (ifUserSetLogPath) {
#ifdef WIN32
		if (get_file_path_code() == UNICODE_CODE) {
			err = path_copy(buf, OBS_LOG_PATH_LEN, USER_SET_OBS_LOG_PATH_W, OBS_LOG_PATH_LEN);
		}
		else if (get_file_path_code() == ANSI_CODE) {
			err = path_copy(buf, OBS_LOG_PATH_LEN, USER_SET_OBS_LOG_PATH, OBS_LOG_PATH_LEN);
		}
		else {
			COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
			err = -1;
		}
#else
		err = path_copy(buf, OBS_LOG_PATH_LEN, USER_SET_OBS_LOG_PATH, OBS_LOG_PATH_LEN);
#endif
        CHECK_ERR_RETURN(err);
    }
    else
    {
#if defined __GNUC__ || defined LINUX
        getCurrentPath(buf);
#elif defined (WIN32)
		if (get_file_path_code() == ANSI_CODE) {
			GetModuleFileNameA(NULL, buf, MAX_MSG_SIZE - 1);
		}
		else if (get_file_path_code() == UNICODE_CODE) {
			GetModuleFileNameW(NULL, (wchar_t*)buf, MAX_MSG_SIZE - 1);
		}
		else {
			return -2;
		}
#endif
    }
    if (ifUserSetLogPath && (!ONLY_SET_LOGCONF))
    {
        MoveConf(buf);
    }
    return EOK;
}

#ifdef WIN32
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
	err = path_copy(currentPath, MAX_MSG_SIZE, buf, MAX_MSG_SIZE);
	if (err != EOK)
	{
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		CHECK_NULL_FREE(currentPath);
		return -1;
	}
	if (get_file_path_code() == ANSI_CODE) {
		char* chr = strrchr(currentPath, '\\');
		if (NULL != chr) {
			*(chr + 1) = '\0';
		}
	}
	else if (get_file_path_code() == UNICODE_CODE) {
		wchar_t* chr = wcsrchr((wchar_t*)currentPath, L'\\');
		if (NULL != chr) {
			*(chr + 1) = L'\0';
		}
	}
	err = path_copy(logPath, MAX_MSG_SIZE, currentPath, MAX_MSG_SIZE);
	if (err != EOK)
	{
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		CHECK_NULL_FREE(currentPath);
		return -1;
	}
	err = path_copy(confPath, MAX_MSG_SIZE, currentPath, MAX_MSG_SIZE);
	if (err != EOK)
	{
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		CHECK_NULL_FREE(currentPath);
		return -1;
	}
	int ret = -1;
	if (get_file_path_code() == ANSI_CODE) {
		ret = strcat_s(confPath, MAX_MSG_SIZE, "OBS.ini");
	}
	else if (get_file_path_code() == UNICODE_CODE) {
		ret = wcscat_s((wchar_t*)confPath, MAX_MSG_SIZE, L"OBS.ini");
	}
	if (ret != 0) {
		CHECK_NULL_FREE(buf);
		CHECK_NULL_FREE(confPath);
		CHECK_NULL_FREE(logPath);
		CHECK_NULL_FREE(tempLogPath);
		CHECK_NULL_FREE(currentPath);
		return -1;
	}

	GetIniSectionItem(SECTION_PATH, PATH_VALUE, confPath, tempLogPath);
	if (get_file_path_code() == ANSI_CODE) {
		tempLogPath[MAX_MSG_SIZE - 1] = '\0';
	}
	else if (get_file_path_code() == UNICODE_CODE) {
		((wchar_t*)tempLogPath)[MAX_MSG_SIZE - 1] = L'\0';
	}

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
#endif // WIN32

void LOG_LEVEL_INIT(unsigned int *logLevel, uint64_t logLevelLen)
{
    for (uint64_t i = 0; i < logLevelLen; ++i) {
        logLevel[i] = INVALID_LOG_LEVEL;
    }
}

int LOG_INIT(void)
{
    unsigned int logLevel[LOG_CATEGORY] = { 0 };
	LOG_LEVEL_INIT(logLevel, LOG_CATEGORY);

	char* buf = getPathBuffer(MAX_MSG_SIZE);

    char* confPath = getPathBuffer(MAX_MSG_SIZE);

    char* logPath = getPathBuffer(MAX_MSG_SIZE);

    char* tempLogPath = getPathBuffer(MAX_MSG_SIZE);

    if (NULL == tempLogPath || NULL == buf 
		|| NULL == confPath || NULL == logPath)
    {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        return -1;
    }

	errno_t err = EOK;
#if defined __GNUC__ || defined LINUX
    GetConfPath(buf);
    if(USER_SET_OBS_LOG_PATH[0] != 0 ) {
        err = memcpy_s(buf, sizeof(char)*OBS_LOG_PATH_LEN, USER_SET_OBS_LOG_PATH, OBS_LOG_PATH_LEN);
		CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    }
    else {        
        getCurrentPath(buf);
    }
    err = memcpy_s(confPath, sizeof(char)*MAX_MSG_SIZE, buf, MAX_MSG_SIZE);
	CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);
    strcat_s(confPath, sizeof(char)*MAX_MSG_SIZE, "/OBS.ini");
	
	FILE* confPathFile = fopen(confPath, "r");
    if(NULL == confPathFile) 
    {
		checkAndLogStrError(SYMBOL_NAME_STR(fopen), __FUNCTION__, __LINE__);
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        CHECK_NULL_FREE(tempLogPath);
        return -1;
    }
    fclose(confPathFile);
	
    GetIniSectionItem(SECTION_PATH, PATH_VALUE, confPath, tempLogPath);
    tempLogPath[MAX_MSG_SIZE - 1] = '\0';
    err = memcpy_s(logPath, sizeof(char)*MAX_MSG_SIZE, buf, MAX_MSG_SIZE);
	CheckAndLogNoneZero(err, "memcpy_s", __FUNCTION__, __LINE__);

    err = EOK;
    err = strcat_s(logPath, sizeof(char)*MAX_MSG_SIZE, "/");
    err = GET_LOG_PATH(logPath, tempLogPath);
    if (err != EOK) {
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        CHECK_NULL_FREE(tempLogPath);
        return -1;
    }
#elif defined WIN32
	err = EOK;
    GetConfPath(buf);

    char* currentPath = getPathBuffer(MAX_MSG_SIZE);
	int ret = SetConfPath(currentPath, buf, confPath, logPath, tempLogPath);
	if (ret)
	{
        CHECK_NULL_FREE(buf);
        CHECK_NULL_FREE(confPath);
        CHECK_NULL_FREE(logPath);
        CHECK_NULL_FREE(tempLogPath);
        CHECK_NULL_FREE(currentPath);
		return ret;
	}
    CHECK_NULL_FREE(currentPath);

#endif

  //  tempLogPath[0] = '\0';
  //  GetIniSectionItem("ProductConfig", "support_API", confPath, tempLogPath);
#if defined ANDROID
    int iRet = LogInitForAndroid(PRODUCT, confPath, logLevel, LOG_CATEGORY, logPath);
#elif defined WIN32

	int iRet = 0;
	if (get_file_path_code() == ANSI_CODE) {
		wchar_t* confPathW = GetWcharFromChar(confPath);
		wchar_t* logPathW = GetWcharFromChar(logPath);
		if (confPathW == NULL || logPathW == NULL) {
			CHECK_NULL_FREE(confPathW);
			CHECK_NULL_FREE(logPathW);
			iRet = -1;
		}
		else {
			iRet = LogInit_W(PRODUCT, confPathW, logLevel, LOG_CATEGORY, logPathW);
			CHECK_NULL_FREE(confPathW);
			CHECK_NULL_FREE(logPathW);
		}
		
	}
	else if (get_file_path_code() == UNICODE_CODE) {
		iRet = LogInit_W(PRODUCT, (const wchar_t*)confPath, logLevel, LOG_CATEGORY, (const wchar_t*)logPath);
	}
#else
    int iRet = LogInit(PRODUCT, confPath, logLevel, LOG_CATEGORY, logPath);
#endif
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

void LOG_EXIT(void)
{
    LogFini(PRODUCT);
    return ;
}
void print_current_time_ms(char* buffer, int bufferLen) {
	// buffer是24个字符的时间字符串，包含毫秒

#ifdef _WIN32
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf_s(buffer, bufferLen, "%04d-%02d-%02d %02d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
	struct timeval tv;
	struct tm* tm_info;
	gettimeofday(&tv, NULL);
	tm_info = localtime(&tv.tv_sec);
	strftime(buffer, bufferLen, "%Y-%m-%d %H:%M:%S", tm_info);
	size_t timeStrLen = strlen(buffer);
	sprintf_s(buffer + timeStrLen, bufferLen - timeStrLen, ".%03ld", tv.tv_usec / 1000);
#endif

}

unsigned long thread_id(void)
{
	unsigned long ret;

#if defined __GNUC__ || defined LINUX
	ret = (unsigned long)pthread_self();
#else
	ret = (unsigned long)GetCurrentThreadId();
#endif

	return(ret);
}

char* getLevelStr(OBS_LOGLEVEL level) {
	switch (level)
	{
	case OBS_LOGDEBUG:
		return SYMBOL_NAME_STR(OBS_LOGDEBUG);
	case OBS_LOGINFO:
		return SYMBOL_NAME_STR(OBS_LOGINFO);
	case OBS_LOGWARN:
		return SYMBOL_NAME_STR(OBS_LOGWARN);
	case OBS_LOGERROR:
		return SYMBOL_NAME_STR(OBS_LOGERROR);
	default:
		return SYMBOL_NAME_STR(OBS_LOGERROR);
	}
}
#define OBS_LOG_TIME_LEN 24
void OBSLogPrintf(OBS_LOGLEVEL level, char* acMsg, size_t acMsgLen) {
	char time_ms[OBS_LOG_TIME_LEN] = { 0 };
	print_current_time_ms(time_ms, OBS_LOG_TIME_LEN);
	long currentTid = thread_id();
	char* levelStr = getLevelStr(level);
	printf("[%s] [tid:%ld] [obs-sdk-c.printLog] [%s] |%s\n", time_ms, currentTid, levelStr, acMsg);
}

OBSLogCallBack userCustomLog = NULL;

void setUserCustomLog(OBSLogCallBack userCustomLogNew) {
	userCustomLog = userCustomLogNew;
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

	if (userCustomLog) {
		(*userCustomLog)(level, acMsg, sizeof(acMsg));
	}
    else if(level == OBS_LOGDEBUG)
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

static OBS_LOGLEVEL obsSdkLogLevel = OBS_LOGERROR;
OBS_LOGLEVEL getRunLogLevel() {
	return obsSdkLogLevel;
}

void setRunLogLevel(void) {
	unsigned int level = GetRunLogLevel();
	switch (level)
	{
	case 0:
		obsSdkLogLevel = OBS_LOGDEBUG;
		break;
	case 1:
		obsSdkLogLevel = OBS_LOGINFO;
		break;
	case 2:
		obsSdkLogLevel = OBS_LOGWARN;
		break;
	case 3:
		obsSdkLogLevel = OBS_LOGERROR;
		break;
	default:
		obsSdkLogLevel = OBS_LOGERROR;
		break;
	}
}

void NULLLOG() {
    return;
}


void CheckAndLogNoneZero(int ret, const char* name, const char* funcName, unsigned long line) {
    if (ret != 0) {
		LogError(name, funcName, line, OBS_LOGWARN);
    }
}

void CheckAndLogNeg(int ret, const char* name, const char* funcName, unsigned long line) {
    if (ret < 0) {
		LogError(name, funcName, line, OBS_LOGWARN);
    }
}

#define ERROR_MESSAGE_BUFFER_SIZE 256
void checkAndLogStrError(const char* failedFuncName, const char* funcName, int lineNum) {
	int err = errno;
	checkIfErrorAndLogStrError(failedFuncName, funcName, lineNum, err);
}

bool checkIfErrorAndLogStrError(const char* failedFuncName, const char* funcName, int lineNum, int err) {
	if (err != EOK) {
		char errorMsgBuffer[ERROR_MESSAGE_BUFFER_SIZE] = { 0 };
		char* errorMsg = "default error";
#if defined (WIN32)
		(void)strerror_s(errorMsgBuffer, ERROR_MESSAGE_BUFFER_SIZE, err);
		errorMsgBuffer[ERROR_MESSAGE_BUFFER_SIZE - 1] = '\0';
		errorMsg = errorMsgBuffer;
#define STRERRORFUNCNAME "strerror_s"
#elif (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE
		(void)strerror_r(err, errorMsgBuffer, ERROR_MESSAGE_BUFFER_SIZE);
		errorMsgBuffer[ERROR_MESSAGE_BUFFER_SIZE - 1] = '\0';
		errorMsg = errorMsgBuffer;
#define STRERRORFUNCNAME "strerror_r(XSI-compliant version)"
#else
		errorMsg = strerror_r(err, errorMsgBuffer, ERROR_MESSAGE_BUFFER_SIZE);
		if (errorMsg == NULL || errorMsgBuffer[0] != '\0') {
			errorMsgBuffer[ERROR_MESSAGE_BUFFER_SIZE - 1] = '\0';
			errorMsg = errorMsgBuffer;
	}
#define STRERRORFUNCNAME "strerror_r(GNU-specific version)"
#endif
		COMMLOG(OBS_LOGERROR,
			"Function :%s failed in function %s, line %d"
			", errno is : %d, %s is : %s.", failedFuncName,
			funcName, lineNum, err, STRERRORFUNCNAME, errorMsg);
		return true;
	}
	return false;
}

void check_before_complete(obs_response_complete_callback *complete_callback, const obs_status status,
	const obs_error_details *error_details, void *callback_data, const char* function, int line) {
	if (complete_callback == NULL) {
		COMMLOG(OBS_LOGERROR, "failed to call %s cause it's NULL in %s, line:%d",
			SYMBOL_NAME_STR(complete_callback), function, line);
	}
	else {
		(void)(*(complete_callback))(status, error_details, callback_data);
	}
}