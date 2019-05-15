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
#ifndef LOGGER_MGR_H_
#define LOGGER_MGR_H_

#include "./vos/vos.h"
#include "eSDKLogDataType.h"

#ifdef WIN32
#include <Windows.h>
#endif

#include <string>
#include <map>

// 产品结构体
class eSDKLog;
//lint -e1554 -e1540
class PRODUCT_DATA
{
public:
	eSDKLog* peSDKLog;
	std::string iniFile;
	std::string logPath;
	std::string productname;
	unsigned int filePermission;

	PRODUCT_DATA()
		: peSDKLog(NULL)
		, iniFile("")
		, logPath("")
		, productname("")
		, filePermission(0)
	{
	}
	PRODUCT_DATA(const PRODUCT_DATA& data)
		: peSDKLog(data.peSDKLog)
		, iniFile(data.iniFile)
		, logPath(data.logPath)
		, productname(data.productname)
		, filePermission(data.filePermission)
	{
	}
	~PRODUCT_DATA()
	{
	}
};
//lint +e1554 +e1540
typedef std::map<std::string, PRODUCT_DATA> PRODUCT_MAP;

#define LOGMGRINSTANE() (eSDK::LoggerMgr::Instance())

namespace eSDK
{
	//日志管理单实例
	class LoggerMgr
	{
	private:
		LoggerMgr(void);
		~LoggerMgr(void);
	public:
		static LoggerMgr& Instance(void);
	public:
#if defined ANDROID
		int initAndroid(const std::string& product, const std::string& iniInfo, unsigned int logLevel[LOG_CATEGORY], const std::string& logPath);
#else
		int init(const std::string& product, const std::string& iniFile, unsigned int logLevel[LOG_CATEGORY], const std::string& logPath);
#endif
		int printInterfacelog(const std::string& product,int iLevel,const std::string& strcontent);
		int printOperationlog(const std::string& product,int iLevel,const std::string& strcontent);
		int printRunlog(const std::string& product,int iLevel,const std::string& strcontent);
		int uninit(const std::string& product);

	public:
		void rollover(const std::string& product);

#if defined(ANDROID) || defined(TARGET_MAC_OS) || defined(TARGET_OS_IPHONE)
		int setLogPropertyEx(const char* product, unsigned int logSize[LOG_CATEGORY], unsigned int logNum[LOG_CATEGORY]);
#endif

	private:
		std::string getLogPathByCondition(const std::string& strIniLogPath);

	private:
		PRODUCT_MAP m_ProductMap;
		VPP::VOS_Mutex* m_mutex;

#if defined(ANDROID) || defined(TARGET_MAC_OS) || defined(TARGET_OS_IPHONE)
		pthread_t m_upStartId;
#endif
	};
}

#endif

