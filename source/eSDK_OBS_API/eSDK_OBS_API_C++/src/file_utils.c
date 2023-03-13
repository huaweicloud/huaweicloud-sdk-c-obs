/*********************************************************************************
* Copyright 2022 Huawei Technologies Co.,Ltd.
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
#include "eSDKOBS.h"
#include "log.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <securec.h>
#include <wchar.h>
#include <libxml/tree.h>
#include <errno.h>

#if defined (WIN32)
#include <io.h>
#endif
#define TEMP_RANDOM_NAME_LEN 4

static file_path_code file_path_code_schemes = ANSI_CODE;

#define ERROR_MESSAGE_BUFFER_SIZE 256
#if defined (WIN32)
errno_t file_sopen_s(int* pfh, const char *filename, int oflag, int shflag, int pmode)
{
	errno_t ret = -1;
	char* openFunc = "default open";
	if (file_path_code_schemes == ANSI_CODE)
	{
		openFunc = "_sopen_s";
		ret = _sopen_s(pfh, filename, oflag, shflag, pmode);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		openFunc = "_wsopen_s";
		ret = _wsopen_s(pfh, (wchar_t*)filename, oflag, shflag, pmode);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return -1;
	}
	if (ret != 0) {
		char errorMsg[ERROR_MESSAGE_BUFFER_SIZE] = { 0 };
		(void)strerror_s(errorMsg, ERROR_MESSAGE_BUFFER_SIZE, errno);
		errorMsg[ERROR_MESSAGE_BUFFER_SIZE - 1] = '\0';
		COMMLOG(OBS_LOGERROR, "%s failed in function %s, ret is %d, errno is %d, strerror_s is : %s."
			, openFunc, __FUNCTION__, ret, errno, errorMsg);
	}
	return ret;
}

int file_stati64(const char *path, struct __stat64 *buffer)
{
	int ret = -1;
	char* statFunc = "default stat";
	if (file_path_code_schemes == ANSI_CODE)
	{
		statFunc = "_stati64";
		ret = _stati64(path, buffer);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		statFunc = "_wstati64";
		ret = _wstati64((wchar_t*)path, buffer);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return -1;
	}
	if (ret != 0) {
		char errorMsg[ERROR_MESSAGE_BUFFER_SIZE] = { 0 };
		(void)strerror_s(errorMsg, ERROR_MESSAGE_BUFFER_SIZE, errno);
		errorMsg[ERROR_MESSAGE_BUFFER_SIZE - 1] = '\0';
		COMMLOG(OBS_LOGERROR, "%s failed in function %s, ret is %d, errno is %d, strerror_s is : %s."
			, statFunc, __FUNCTION__, ret, errno, errorMsg);
	}
	return ret;
}

void set_file_path_code(file_path_code code) {
	file_path_code_schemes = code;
}

file_path_code get_file_path_code() {
	return file_path_code_schemes;
}

char* getCharFromWchar(const wchar_t* wstr) {
	//bufferSize is size of char buffer ,include '\0' 
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	if (bufferSize <= 0) {
		COMMLOG(OBS_LOGERROR, "%s failed, bufferSize is 0", __FUNCTION__);
		return NULL;
	}
	bufferSize *= sizeof(char);
	char* char_str = (char*)malloc(bufferSize); //no need to malloc(bufferSize * sizeof(char)+1);
	if (char_str == NULL) {
		COMMLOG(OBS_LOGERROR, "malloc failed in function: %s,line %d", __FUNCTION__, __LINE__);
		return char_str;
	}
	errno_t err = memset_s(char_str, bufferSize, 0, bufferSize);
	if (err != EOK) {
		CheckAndLogNoneZero(err, "memset_s", __FUNCTION__, __LINE__);
		CHECK_NULL_FREE(char_str);
		return NULL;
	}
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, char_str, bufferSize, NULL, NULL);
	return char_str;
}

#endif

int file_path_cmp(char const* path1, char const* path2) {
	if (file_path_code_schemes == ANSI_CODE)
	{
		return strcmp(path1, path2);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		return wcscmp((wchar_t const*)path1, (wchar_t const*)path2);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return -1;
	}
}

int remove_file(const char* filename)
{
	int ret = -1;
	char* removeFunc = "default remove func";
	if (file_path_code_schemes == ANSI_CODE)
	{
		removeFunc = "remove";
		ret = remove(filename);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
#if defined __GNUC__ || defined LINUX
		removeFunc = "remove";
		ret = remove(filename);
#endif
#if defined (WIN32)
		removeFunc = "_wremove";
		ret = _wremove((const wchar_t*)filename);
#endif
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return -1;
	}
	if (ret != 0) {
		char errorMsg[ERROR_MESSAGE_BUFFER_SIZE] = { 0 };
		char *strerrorFunc = "strerror_r";
#if defined (WIN32)
		(void)strerror_s(errorMsg, ERROR_MESSAGE_BUFFER_SIZE, errno);
		strerrorFunc = "strerror_s";
#else
		(void)strerror_r(errno, errorMsg, ERROR_MESSAGE_BUFFER_SIZE);
#endif
		errorMsg[ERROR_MESSAGE_BUFFER_SIZE - 1] = '\0';
		COMMLOG(OBS_LOGERROR, "%s failed in function %s, ret is %d, errno is %d, %s is : %s."
			, removeFunc, __FUNCTION__, ret, errno, strerrorFunc, errorMsg);
	}
	return ret;
}

errno_t file_path_append(char* destination, size_t destinationSize)
{

	if (file_path_code_schemes == ANSI_CODE)
	{
		return strcat_s(destination, destinationSize, ".xmltmp");
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		return wcscat_s((wchar_t*)destination, destinationSize, L".xmltmp");
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return -1;
	}
}

errno_t  path_copy(void* const destination, size_t const destinationSize,
	void const* const source, size_t const sourceSize) 
{
	if (file_path_code_schemes == ANSI_CODE)
	{
		return memcpy_s(destination, destinationSize * sizeof(char)
			, source, sourceSize * sizeof(char));
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		return memcpy_s(destination, destinationSize * sizeof(wchar_t)
			, source, sourceSize * sizeof(wchar_t));
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return -1;
	}
}

char *getPathBuffer(size_t bufferLen)
{
	uint64_t unitLen = 0;
	char* pathBuffer = NULL;
	if (bufferLen == 0) {
		COMMLOG(OBS_LOGWARN, "%s failed, bufferLen is 0", __FUNCTION__);
		return pathBuffer;
	}
	else if (file_path_code_schemes == ANSI_CODE) {
		unitLen = sizeof(char);
	}
	else if (file_path_code_schemes == UNICODE_CODE) {
		unitLen = sizeof(wchar_t);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return pathBuffer;
	}
	size_t bufferSize = bufferLen * unitLen;
	pathBuffer = (char*)malloc(bufferSize);

	if (pathBuffer == NULL) {
		COMMLOG(OBS_LOGERROR, "malloc failed in function: %s,line %d", __FUNCTION__, __LINE__);
		return pathBuffer;
	}
	memset_s(pathBuffer, bufferSize, 0, bufferSize);
	return pathBuffer;
}

int checkpoint_file_path_printf(char* const path_buffer
	, size_t const path_buffer_count, char const* uploadFileName)
{
	int ret = -1;
	if (file_path_code_schemes == ANSI_CODE)
	{
		ret = sprintf_s(path_buffer, path_buffer_count, "%s%s", uploadFileName, ".xmltmp");
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		ret = swprintf_s((wchar_t* const)path_buffer, path_buffer_count
			, L"%s%s", (wchar_t const* const)uploadFileName, L".xmltmp");
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		ret = -1;
	}
	return ret;
}

size_t  file_path_strlen(char const* filePath)
{
	if (filePath == NULL) 
	{
		COMMLOG(OBS_LOGERROR, "try to strlen a nullptr, function %s failed", __FUNCTION__);
		return 0;
	}
	else if (file_path_code_schemes == ANSI_CODE)
	{
		return strlen(filePath);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		return wcslen((wchar_t const*)filePath);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return 0;
	}
}

int checkPointFileSave(const char *filename, xmlDocPtr doc) 
{
#if defined __GNUC__ || defined LINUX
	return xmlSaveFile(filename, doc);
#endif
#if defined (WIN32)
	int ret = -1;
	if (file_path_code_schemes == ANSI_CODE)
	{
		ret = xmlSaveFile(filename, doc);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		char* tempName = getCharFromWchar((const wchar_t*)filename);
		if (tempName == NULL) {
			COMMLOG(OBS_LOGERROR, "getCharFromWchar failed in function %s, line:%d", __FUNCTION__, __LINE__);
			return -1;
		}
		ret = xmlSaveFile(tempName, doc);
		CHECK_NULL_FREE(tempName);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		ret = -1;
	}
	return ret;
#endif
}

xmlDocPtr checkPointFileRead(const char *filename, const char *encoding, int options) {
#if defined __GNUC__ || defined LINUX
	return xmlReadFile(filename, encoding, options);
#endif
#if defined (WIN32)
	if (file_path_code_schemes == ANSI_CODE)
	{
		return xmlReadFile(filename, encoding, options);
}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		char* tempName = getCharFromWchar((const wchar_t*)filename);
		if (tempName == NULL) {
			COMMLOG(OBS_LOGERROR, "getCharFromWchar failed in function %s, line:%d", __FUNCTION__, __LINE__);
			return NULL;
		}
		xmlDocPtr readDoc = xmlReadFile(tempName, encoding, options);
		CHECK_NULL_FREE(tempName);
		return readDoc;
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return NULL;
	}
#endif
}