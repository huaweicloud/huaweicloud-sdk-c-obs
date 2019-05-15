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
#ifndef FILE_MGR_H_
#define FILE_MGR_H_

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>
#include "LoggerMgr.h"
#include "eSDKLogDataType.h"

#define FILEMGRINSTANE() (eSDK::FileMgr::Instance())

namespace eSDK
{
	class FileMgr
	{
#ifndef WIN32
	private:
		bool pathFindExtension(char* destStr,char* path)const;
#endif
	public:
		FileMgr(void);
		~FileMgr(void);
	public:
		static FileMgr& Instance(void);
	public:
		//É¾³ýÎÄ¼þ
		int DeleteFile(const std::string& fileName, eSDKLog* peSDKLog)const;
	};
}

#endif


