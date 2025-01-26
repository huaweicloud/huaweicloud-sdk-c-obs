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
#ifndef FILE_UTILS
#define FILE_UTILS

#include <sys/stat.h>
#include <libxml/tree.h>
int checkPointFileSave(const char *filename, xmlDocPtr doc);
xmlDocPtr checkPointFileRead(const char *filename, const char *encoding, int options);
int file_path_cmp(char const* path1, char const* path2);
int remove_file(const char* filename);
int file_path_append(char* destination, size_t destinationSize);
int path_copy(void* const destination, size_t const destinationSize,
	void const* const source, size_t const sourceSize);
char *getPathBuffer(size_t bufferLen);
int temp_part_file_path_printf(char * fileNameTempBuffer,
	size_t const fileNameTempBufferCount, const char * storeFileName, int part_num);
int checkpoint_file_path_printf(char* const path_buffer
	, size_t const path_buffer_count, char const* uploadFileName);
size_t  file_path_strlen(char const* filePath);
int file_fopen_s(FILE** _Stream, const char *filename, const char *mode);
char* file_path_fgets(char* _Buffer, int _MaxCount, FILE* _Stream);
#if defined (WIN32)
wchar_t *GetWcharFromChar(const char *char_str);
int file_sopen_s(int* pfh, const char *filename, int oflag, int shflag, int pmode);
int file_stati64(const char *path, struct __stat64 *buffer);
#endif

#endif