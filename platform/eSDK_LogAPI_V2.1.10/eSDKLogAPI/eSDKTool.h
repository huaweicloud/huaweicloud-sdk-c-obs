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
#ifndef _ESDKTOOL_H
#define _ESDKTOOL_H

#ifdef WIN32
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include <string>
#include <string.h>
#include <stdlib.h>
using namespace std;

#if defined(ANDROID) || defined(TARGET_MAC_OS) || defined(TARGET_OS_IPHONE)
#include "zlib.h"
#endif

#define CheckPointerReturn(para) if(NULL == para)\
{\
	return;\
}
#define CheckPointerReturnCode(para,retcode) if(NULL == para)\
{\
	return retcode;\
}

//工具类
class eSDKTool
{
private:
	eSDKTool(void);
	~eSDKTool(void);
#ifndef WIN32
	static int GetModuleFilePath(const char* sModuleName, char* sPath, unsigned int unSize);
#endif

public:
	static std::string GetDllPath();
	static std::string GetAppPath();
	static bool IsRelativePath(const std::string& path);
	static bool CreateMultipleDirectory(const std::string& path);
	static void getCurrentPath(std::string& strPath);
	static void GetIPPort(const std::string& server,std::string& ip,unsigned short& port);//输入“10.10.10.10:10”，解析出“10.10.10.10”和10
	static bool CheckIP4Valid(const std::string &ip);//检测IP地址是否正确
	static void GetFileNameByPath(const std::string& strPath,std::string& strFile);//输入“D:\\123.txt”，得出“123.txt”
	static void GetSrandNum(std::string& sRandNum); //获取随机字符串作分隔符
#if defined ANDROID
	static void GetIniSectionItem(const char* Section, const char* Item, const char* FileInfo, std::string& iniValue);
#else
	static void GetIniSectionItem(const char* Section, const char* Item, const char* FileName, std::string& iniValue);
#endif
    static bool IsPathFileExist(const std::string& path);

	// 判断是否为目录
	static bool IsDir(const std::string& path);

#if defined(ANDROID) || defined(TARGET_MAC_OS) || defined(TARGET_OS_IPHONE)
	static int gzcompress(Bytef *data, uLong ndata, Bytef *zdata, uLong *nzdata);
#endif

	// std::string to Octal(int)
	static unsigned int StringToOCT(const std::string& strNumber);
	// std::string to Octal(int)
};

//工具方法

#endif


