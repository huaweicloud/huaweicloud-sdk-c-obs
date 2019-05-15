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
#include "Logger.h"
#include "securec.h"

CLogger::CLogger(void)
{
}

CLogger::~CLogger(void)
{
}

int CLogger::LogInit(const char* product, const char* iniFile, unsigned int logLevel[LOG_CATEGORY], const char* logPath)
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

int CLogger::LogFini(const char* product)
{
	CheckPointerReturnCode(product,RET_NULL_POINTER);

	if(RET_SUCCESS != LOGMGRINSTANE().uninit(product))
	{
		return RET_INVALID_PARA;
	}

	return RET_SUCCESS;
}

void CLogger::Log_Interface_Info(
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

void CLogger::Log_Interface_Error(
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

void CLogger::Log_Operate_Debug(	
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

void CLogger::Log_Operate_Info(
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

void CLogger::Log_Operate_Warn(
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

void CLogger::Log_Operate_Error(	
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

void CLogger::Log_Run_Debug(const char* product, const char* param)
{
	CheckPointerReturn(product);
	CheckPointerReturn(param);
	std::string strDebugContent(param);
	(void)LOGMGRINSTANE().printRunlog(product,DEBUG_LEVEL,strDebugContent);
}

void CLogger::Log_Run_Info(const char* product, const char* param)
{
	CheckPointerReturn(product);
	CheckPointerReturn(param);
	std::string strInfoContent(param);
	strInfoContent = "|" + strInfoContent;
	(void)LOGMGRINSTANE().printRunlog(product,INFO_LEVEL,strInfoContent);
}

void CLogger::Log_Run_Warn(const char* product, const char* param)
{
	CheckPointerReturn(product);
	CheckPointerReturn(param);
	std::string strWarnContent(param);
	strWarnContent = "|" + strWarnContent;
	(void)LOGMGRINSTANE().printRunlog(product,WARN_LEVEL,strWarnContent);
}

void CLogger::Log_Run_Error(const char* product, const char* param)
{
	CheckPointerReturn(product);
	CheckPointerReturn(param);
	std::string strErrorContent(param);
	strErrorContent = "|" + strErrorContent;
	(void)LOGMGRINSTANE().printRunlog(product,ERROR_LEVEL,strErrorContent);
}

int CLogger::setLogPropertyEx(const char *product, unsigned int *logSize, unsigned int *logNum)
{
    CheckPointerReturnCode(product, RET_NULL_POINTER);
    CheckPointerReturnCode(logSize, RET_NULL_POINTER);
    CheckPointerReturnCode(logNum, RET_NULL_POINTER);
    
    int nRet = LOGMGRINSTANE().setLogPropertyEx(product, logSize, logNum);
    return nRet;
}

