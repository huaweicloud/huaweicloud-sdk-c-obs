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
#ifdef WIN32
#include <shlwapi.h>
#pragma comment( lib, "shlwapi.lib")
#endif
#include "FileMgr.h"
#include "eSDKLog.h"

namespace eSDK
{

	FileMgr::FileMgr(void)
	{
	}

	FileMgr::~FileMgr(void)
	{
	}

	FileMgr& FileMgr::Instance(void)
	{
		static FileMgr inst;
		return inst;
	}
#ifndef WIN32
	bool FileMgr::pathFindExtension(char* destStr, char* path)const
	{
		char* posExtension= strrchr(path, '.');
		if(NULL == posExtension)
		{
			return false;
		}
		else
		{
			while((*destStr++ = *posExtension++) != '\0');
			return true;
		}
	}
#endif

	//删除文件
	int FileMgr::DeleteFile(const std::string& strFilePathName, eSDKLog* peSDKLog)const
	{
#ifdef WIN32
		if(!::DeleteFile(strFilePathName.c_str()))
#else
		if(remove(strFilePathName.c_str()))
#endif
		{
			std::string strLog = "delete ";
			strLog.append(strFilePathName + " failed.");
			if (NULL != peSDKLog)
			{
				peSDKLog->printOptWarnlog(strLog) ;
			}
			return RET_INVALID_PARA;
		}
		
		return RET_SUCCESS;
	}
}


