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
// logManager.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdarg.h"
#include "LoggerMgr.h"
#include "eSDKTool.h"
#include "eSDKLog.h"
#include "./vos/vos.h"
#include "eSDKLogAPI.h"

#if defined __GNUC__ || defined LINUX
#include "securec.h"
#endif

using namespace eSDK;

// snprintf定义
#if defined _MSC_VER  || defined WIN32
# define SNPRINTF  _vsnprintf_s
#endif 

#if defined __GNUC__ || defined LINUX
# define SNPRINTF  vsnprintf_s
#endif

#ifndef WIN64
// 导出接口预处理
#pragma comment(linker, "/EXPORT:LogFini=_LogFini@4")
#pragma comment(linker, "/EXPORT:LogInit=_LogInit@16")
#pragma comment(linker, "/EXPORT:Log_Run_Debug=_Log_Run_Debug@8")
#pragma comment(linker, "/EXPORT:Log_Run_Error=_Log_Run_Error@8")
#pragma comment(linker, "/EXPORT:Log_Run_Info=_Log_Run_Info@8")
#pragma comment(linker, "/EXPORT:Log_Run_Warn=_Log_Run_Warn@8")
// 导出接口预处理
#endif

// 安卓读取无法读取ini文件，故直接传送ini文件内容
#if defined ANDROID
int _STD_CALL_ LogInitForAndroid(const char* product, const char* iniInfo, unsigned int logLevel[LOG_CATEGORY], const char* logPath)
{
	CheckPointerReturnCode(product,RET_NULL_POINTER);
	CheckPointerReturnCode(iniInfo,RET_NULL_POINTER);
	CheckPointerReturnCode(logPath,RET_NULL_POINTER);

	if(RET_SUCCESS != LOGMGRINSTANE().initAndroid(product, iniInfo, logLevel, logPath))
	{
		return RET_INVALID_PARA;
	}

	return RET_SUCCESS;
}
// 安卓读取无法读取ini文件，故直接传送ini文件内容
#else

int _STD_CALL_ LogInit(const char* product, const char* iniFile, unsigned int logLevel[LOG_CATEGORY], const char* logPath)
{
	
	CheckPointerReturnCode(product,RET_NULL_POINTER);
	CheckPointerReturnCode(iniFile,RET_NULL_POINTER);
	CheckPointerReturnCode(logPath,RET_NULL_POINTER);

	if(RET_SUCCESS != LOGMGRINSTANE().init(product, iniFile, logLevel, logPath))
	{
		return RET_INVALID_PARA;
	}

	return RET_SUCCESS;
}
#endif

int _STD_CALL_ LogFini(const char* product)
{
	CheckPointerReturnCode(product,RET_NULL_POINTER);

	if(RET_SUCCESS != LOGMGRINSTANE().uninit(product))
	{
		return RET_INVALID_PARA;
	}

	return RET_SUCCESS;
}

//lint -e438
void _STD_CALL_ Log_Interface_Info(
   const char* product,
   const char* interfaceType,
   const char* protocolType,
   const char* interfaceName,
   const char* TransactionID,
   const char* reqTime,
   const char* RespTime,
   const char* resultCode,
   const char* params,
   ...
   )
{
	CheckPointerReturn(product);
	CheckPointerReturn(interfaceType);
	CheckPointerReturn(protocolType);
	CheckPointerReturn(interfaceName);
	CheckPointerReturn(TransactionID);
	CheckPointerReturn(reqTime);
	CheckPointerReturn(RespTime);
	CheckPointerReturn(resultCode);
	CheckPointerReturn(params);

	std::string strInfoContent(product);
	strInfoContent.append("|");
	strInfoContent.append(interfaceType);
	strInfoContent.append("|");
	strInfoContent.append(protocolType);
	strInfoContent.append("|");
	strInfoContent.append(interfaceName);
	strInfoContent.append("|");
	strInfoContent.append(TransactionID);
	strInfoContent.append("|");
	strInfoContent.append(reqTime);
	strInfoContent.append("|");
	strInfoContent.append(RespTime);
	strInfoContent.append("|");

	char para[1024] = {0};
	va_list args;
	va_start(args, params);
	(void)SNPRINTF(para, sizeof(para), sizeof(para) - 1, params, args);
	va_end(args);
	strInfoContent.append(para);

	strInfoContent.append("|");
	strInfoContent.append(resultCode);
	strInfoContent.append("|");

	(void)LOGMGRINSTANE().printInterfacelog(product,INFO_LEVEL,strInfoContent);
}

void _STD_CALL_ Log_Interface_Error(
	const char* product,
	const char* interfaceType,
	const char* protocolType,
	const char* interfaceName,
	const char* TransactionID,
	const char* reqTime,
	const char* RespTime,
	const char* resultCode,
	const char* params,
	...
	)
{
	CheckPointerReturn(product);
	CheckPointerReturn(interfaceType);
	CheckPointerReturn(protocolType);
	CheckPointerReturn(interfaceName);
	CheckPointerReturn(TransactionID);
	CheckPointerReturn(reqTime);
	CheckPointerReturn(RespTime);
	CheckPointerReturn(resultCode);
	CheckPointerReturn(params);

	std::string strErrContent(product);
	strErrContent.append("|");
	strErrContent.append(interfaceType);
	strErrContent.append("|");
	strErrContent.append(protocolType);
	strErrContent.append("|");
	strErrContent.append(interfaceName);
	strErrContent.append("|");
	strErrContent.append(TransactionID);
	strErrContent.append("|");
	strErrContent.append(reqTime);
	strErrContent.append("|");
	strErrContent.append(RespTime);
	strErrContent.append("|");
	

	char para[1024] = {0};
	va_list args;
	va_start(args, params);
	(void)SNPRINTF(para, sizeof(para), sizeof(para) - 1, params, args);
	va_end(args);
	strErrContent.append(para);

	strErrContent.append("|");
	strErrContent.append(resultCode);
	strErrContent.append("|");

	(void)LOGMGRINSTANE().printInterfacelog(product,ERROR_LEVEL,strErrContent);
}

void _STD_CALL_ Log_Operate_Debug(	
	const char* product,
	const char* moduleName,
	const char* userName,
	const char* clientFlag,
	const char* resultCode,
	const char* keyInfo,
	const char* params,
	...
	)
{
	CheckPointerReturn(product);
	CheckPointerReturn(moduleName);
	CheckPointerReturn(userName);
	CheckPointerReturn(clientFlag);
	CheckPointerReturn(resultCode);
	CheckPointerReturn(keyInfo);

	std::string strDebugContent(moduleName);
	strDebugContent.append("|");
	strDebugContent.append(userName);
	strDebugContent.append("|");
	strDebugContent.append(clientFlag);
	strDebugContent.append("|");
	strDebugContent.append(resultCode);
	strDebugContent.append("|");
	strDebugContent.append(keyInfo);
	strDebugContent.append("|");

	char para[1024] = {0};
	va_list args;
	va_start(args, params);
	(void)SNPRINTF(para, sizeof(para), sizeof(para) - 1, params, args);
	va_end(args);

	strDebugContent.append(para);

	(void)LOGMGRINSTANE().printOperationlog(product,DEBUG_LEVEL,strDebugContent);
}

void _STD_CALL_ Log_Operate_Info(
	const char* product,
	const char* moduleName,
	const char* userName,
	const char* clientFlag,
	const char* resultCode,
	const char* keyInfo,
	const char* params,
	...
)
{
	CheckPointerReturn(product);
	CheckPointerReturn(moduleName);
	CheckPointerReturn(userName);
	CheckPointerReturn(clientFlag);
	CheckPointerReturn(resultCode);
	CheckPointerReturn(keyInfo);

	std::string strInfoContent(moduleName);
	strInfoContent.append("|");
	strInfoContent.append(userName);
	strInfoContent.append("|");
	strInfoContent.append(clientFlag);
	strInfoContent.append("|");
	strInfoContent.append(resultCode);
	strInfoContent.append("|");
	strInfoContent.append(keyInfo);
	strInfoContent.append("|");

	char para[1024] = {0};
	va_list args;
	va_start(args, params);
	(void)SNPRINTF(para, sizeof(para), sizeof(para) - 1, params, args);
	va_end(args);

	strInfoContent.append(para);

	(void)LOGMGRINSTANE().printOperationlog(product,INFO_LEVEL,strInfoContent);
}

void _STD_CALL_ Log_Operate_Warn(
	const char* product,
	const char* moduleName,
	const char* userName,
	const char* clientFlag,
	const char* resultCode,
	const char* keyInfo,
	const char* params,
	...
)
{
	CheckPointerReturn(product);
	CheckPointerReturn(moduleName);
	CheckPointerReturn(userName);
	CheckPointerReturn(clientFlag);
	CheckPointerReturn(resultCode);
	CheckPointerReturn(keyInfo);

	std::string strWarnContent(moduleName);
	strWarnContent.append("|");
	strWarnContent.append(userName);
	strWarnContent.append("|");
	strWarnContent.append(clientFlag);
	strWarnContent.append("|");
	strWarnContent.append(resultCode);
	strWarnContent.append("|");
	strWarnContent.append(keyInfo);
	strWarnContent.append("|");

	char para[1024] = {0};
	va_list args;
	va_start(args, params);
	(void)SNPRINTF(para, sizeof(para), sizeof(para) - 1, params, args);
	va_end(args);

	strWarnContent.append(para);

	(void)LOGMGRINSTANE().printOperationlog(product,WARN_LEVEL,strWarnContent);
}

void _STD_CALL_ Log_Operate_Error(	
	const char* product,
	const char* moduleName,
	const char* userName,
	const char* clientFlag,
	const char* resultCode,
	const char* keyInfo,
	const char* params,
	...
)
{
	CheckPointerReturn(product);
	CheckPointerReturn(moduleName);
	CheckPointerReturn(userName);
	CheckPointerReturn(clientFlag);
	CheckPointerReturn(resultCode);
	CheckPointerReturn(keyInfo);

	std::string strErrorContent(moduleName);
	strErrorContent.append("|");
	strErrorContent.append(userName);
	strErrorContent.append("|");
	strErrorContent.append(clientFlag);
	strErrorContent.append("|");
	strErrorContent.append(resultCode);
	strErrorContent.append("|");
	strErrorContent.append(keyInfo);
	strErrorContent.append("|");

	char para[1024] = {0};
	va_list args;
	va_start(args, params);
	(void)SNPRINTF(para, sizeof(para), sizeof(para) - 1, params, args);
	va_end(args);

	strErrorContent.append(para);

	(void)LOGMGRINSTANE().printOperationlog(product,ERROR_LEVEL,strErrorContent);
}
//lint +e438

void _STD_CALL_ Log_Run_Debug(const char* product, const char* param)
{
	CheckPointerReturn(product);
	CheckPointerReturn(param);
	std::string strDebugContent(param);
	strDebugContent = "|" + strDebugContent;

	(void)LOGMGRINSTANE().printRunlog(product,DEBUG_LEVEL,strDebugContent);
}

void _STD_CALL_ Log_Run_Info(const char* product, const char* param)
{
	CheckPointerReturn(product);
	CheckPointerReturn(param);
	std::string strInfoContent(param);
	strInfoContent = "|" + strInfoContent;

	(void)LOGMGRINSTANE().printRunlog(product,INFO_LEVEL,strInfoContent);
}

void _STD_CALL_ Log_Run_Warn(const char* product, const char* param)
{
	CheckPointerReturn(product);
	CheckPointerReturn(param);
	std::string strWarnContent(param);
	strWarnContent = "|" + strWarnContent;

	(void)LOGMGRINSTANE().printRunlog(product,WARN_LEVEL,strWarnContent);
}

void _STD_CALL_ Log_Run_Error(const char* product, const char* param)
{
	CheckPointerReturn(product);
	CheckPointerReturn(param);
	std::string strErrorContent(param);
	strErrorContent = "|" + strErrorContent;

	(void)LOGMGRINSTANE().printRunlog(product,ERROR_LEVEL,strErrorContent);
}

// 移动端ISV初始化接口
#if defined(ANDROID) || defined(TARGET_MAC_OS) || defined(TARGET_OS_IPHONE)

eSDK_LOG_API int _STD_CALL_ setLogPropertyEx(const char* product, unsigned int logSize[LOG_CATEGORY], unsigned int logNum[LOG_CATEGORY])
{
	CheckPointerReturnCode(product, RET_NULL_POINTER);
	CheckPointerReturnCode(logSize, RET_NULL_POINTER);
	CheckPointerReturnCode(logNum, RET_NULL_POINTER);

	return LOGMGRINSTANE().setLogPropertyEx(product, logSize, logNum);
}

#endif
// 移动端ISV初始化接口

