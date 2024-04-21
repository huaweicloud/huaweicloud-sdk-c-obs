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
#include "file_utils.h"

#if defined (WIN32)
#include <io.h>
#endif
#define TEMP_RANDOM_NAME_LEN 4

static file_path_code file_path_code_schemes = ANSI_CODE;
void checkAndLogStrError(const char* failedFuncName, const char* funcName, int lineNum);
#define ERROR_MESSAGE_BUFFER_SIZE 256
#if defined (WIN32)
int file_sopen_s(int* pfh, const char *filename, int oflag, int shflag, int pmode)
{
	int ret = -1;
	char* openFunc = "default open";
	if (file_path_code_schemes == ANSI_CODE)
	{
		openFunc = SYMBOL_NAME_STR(_sopen_s);
		ret = _sopen_s(pfh, filename, oflag, shflag, pmode);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		openFunc = SYMBOL_NAME_STR(_wsopen_s);
		ret = _wsopen_s(pfh, (wchar_t*)filename, oflag, shflag, pmode);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return -1;
	}
	if (ret != 0) {
		checkAndLogStrError(openFunc, __FUNCTION__, __LINE__);
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
		checkAndLogStrError(statFunc, __FUNCTION__, __LINE__);
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
	int err = memset_s(char_str, bufferSize, 0, bufferSize);
	if (err != EOK) {
		CheckAndLogNoneZero(err, "memset_s", __FUNCTION__, __LINE__);
		CHECK_NULL_FREE(char_str);
		return NULL;
	}
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, char_str, bufferSize, NULL, NULL);
	return char_str;
}

wchar_t *GetWcharFromChar(const char *char_str)
{
	const size_t char_str_len = strlen(char_str);
	const size_t wchar_str_size = char_str_len + 1;
	wchar_t *wchar_str = (wchar_t *)malloc(sizeof(wchar_t) * wchar_str_size);
	if (wchar_str == NULL) {
		COMMLOG(OBS_LOGERROR, "malloc failed in function: %s,line %d", __FUNCTION__, __LINE__);
		return wchar_str;
	}
	memset_s(wchar_str, sizeof(wchar_t) * wchar_str_size, 0, sizeof(wchar_t) * wchar_str_size);
	size_t converted = 0;
	int ret = mbstowcs_s(&converted, wchar_str, wchar_str_size, char_str, char_str_len);
	if (ret != 0) {
		CHECK_NULL_FREE(wchar_str);
	}
	else {
		wchar_str[char_str_len] = L'\0';
	}
	
	return wchar_str;
}

#endif

int needCheckStrError = true;
void checkAndLogStrError(const char* failedFuncName, const char* funcName, int lineNum) {
	int err = errno;
	if (needCheckStrError && err != EOK) {
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
		if(errorMsg == NULL || errorMsgBuffer[0] != '\0'){
			errorMsgBuffer[ERROR_MESSAGE_BUFFER_SIZE - 1] = '\0';
			errorMsg = errorMsgBuffer;
		}
#define STRERRORFUNCNAME "strerror_r(GNU-specific version)"
#endif
		COMMLOG(OBS_LOGERROR, 
			"Function :%s failed in function %s, line %d"
			", errno is : %d, %s is : %s.", failedFuncName,
			funcName, lineNum, err, STRERRORFUNCNAME, errorMsg);
	}
}

char* file_path_fgets(char* _Buffer, int _MaxCount, FILE* _Stream) {
	char* fgets_ret = NULL;
	char* fgetsName = "default fgets";
#ifdef WIN32
	if (file_path_code_schemes == ANSI_CODE)
	{
		fgets_ret = fgets(_Buffer, _MaxCount, _Stream);
		fgetsName = SYMBOL_NAME_STR(fgets);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		fgets_ret = (char*)fgetws((wchar_t*)_Buffer, _MaxCount, _Stream);
		fgetsName = SYMBOL_NAME_STR(fgetws);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
	}
#else
	fgets_ret = fgets(_Buffer, _MaxCount, _Stream);
	fgetsName = SYMBOL_NAME_STR(fgets);
#endif // WIN32
	if (fgets_ret == NULL) {
		checkAndLogStrError(fgetsName, __FUNCTION__, __LINE__);
	}
	return fgets_ret;
}

int file_fopen_s(FILE** _Stream, const char *filename, const char *mode) {
	int ret = -1;
	char* fopenFuncName = "defaultOpen";
#ifdef WIN32
	if (file_path_code_schemes == ANSI_CODE)
	{
		ret = fopen_s(_Stream, filename, mode);
		fopenFuncName = SYMBOL_NAME_STR(fopen_s);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		wchar_t* mode_w = GetWcharFromChar(mode);
		if (mode_w == NULL) {
			COMMLOG(OBS_LOGERROR, "GerWcharFromChar failed, function %s failed", __FUNCTION__);
			return -2;
		}
		ret = _wfopen_s(_Stream, (const wchar_t*)filename, mode_w);
		fopenFuncName = SYMBOL_NAME_STR(_wfopen_s);
		CHECK_NULL_FREE(mode_w);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return -2;
	}
#else
	*_Stream = fopen(filename, mode);
	fopenFuncName = SYMBOL_NAME_STR(fopen);
	ret = errno;
#endif // WIN32
	if (ret != 0) {
		checkAndLogStrError(fopenFuncName, __FUNCTION__, __LINE__);
	}
	return ret;
}

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
		removeFunc = SYMBOL_NAME_STR(remove);
		ret = remove(filename);
#endif
#if defined (WIN32)
		removeFunc = SYMBOL_NAME_STR(_wremove);
		ret = _wremove((const wchar_t*)filename);
#endif
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return -1;
	}
	if (ret != 0) {
		checkAndLogStrError(removeFunc, __FUNCTION__, __LINE__);
	}
	return ret;
}

int file_path_append(char* destination, size_t destinationSize)
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

uint64_t getPathUnitLen(){
#ifdef WIN32
	if (file_path_code_schemes == ANSI_CODE) {
		return sizeof(char);
	}
	else if (file_path_code_schemes == UNICODE_CODE) {
		return sizeof(wchar_t);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		return sizeof(char);
	}
#else
	return sizeof(char);
#endif // WIN32
}

int path_copy(void* const destination, size_t const destinationSize,
	void const* const source, size_t const sourceSize) 
{
	uint64_t unitLen = getPathUnitLen();
	if (destinationSize == 0) {
		COMMLOG(OBS_LOGWARN, "%s failed, bufferLen is 0", __FUNCTION__);
		return -1;
	}
	return memcpy_s(destination, destinationSize * unitLen, source, sourceSize * unitLen);
}


char *getPathBuffer(size_t bufferLen)
{
	uint64_t unitLen = getPathUnitLen();
	char* pathBuffer = NULL;
	if (bufferLen == 0) {
		COMMLOG(OBS_LOGWARN, "%s failed, bufferLen is 0", __FUNCTION__);
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

int temp_part_file_path_printf(char * fileNameTempBuffer, 
	size_t const fileNameTempBufferCount, const char * storeFileName, int part_num) {
	int ret = -1;
	if (file_path_code_schemes == ANSI_CODE)
	{
		ret = sprintf_s(fileNameTempBuffer, fileNameTempBufferCount, "%s.%d", storeFileName, part_num);
	}
	else if (file_path_code_schemes == UNICODE_CODE)
	{
		ret = swprintf_s((wchar_t*)fileNameTempBuffer, fileNameTempBufferCount
			, L"%s.%d", (wchar_t const*)storeFileName, part_num);
	}
	else
	{
		COMMLOG(OBS_LOGERROR, "unkown encoding scheme, function %s failed", __FUNCTION__);
		ret = -1;
	}
	return ret;
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

size_t file_path_strlen(char const* filePath)
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

#else
	return xmlSaveFile(filename, doc);
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