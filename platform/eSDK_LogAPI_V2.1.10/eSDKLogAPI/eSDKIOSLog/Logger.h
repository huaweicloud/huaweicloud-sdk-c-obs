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
#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "LoggerMgr.h"
#include "eSDKTool.h"
#include "eSDKLog.h"
#include "vos/vos.h"
#include "eSDKLogDataType.h"

using namespace eSDK;

// snprintf∂®“Â
#if defined _MSC_VER  || defined WIN32
# define SNPRINTF  _vsnprintf_s
#endif

#if defined __GNUC__ || defined LINUX
# define SNPRINTF  vsnprintf_s
#endif

class CLogger
{
public:
	CLogger(void);
	~CLogger(void);


    int LogInit(const char* product, const char* iniFile, unsigned int logLevel[LOG_CATEGORY], const char* logPath);
    int LogFini(const char* product);

void Log_Interface_Info(
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
   );

void Log_Interface_Error(
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
	);

void Log_Operate_Debug(	
	const char* product,
	const char* moduleName,
	const char* userName,
	const char* clientFlag,
	const char* resultCode,
	const char* keyInfo,
	const char* params,
	...
	);

void Log_Operate_Info(
	const char* product,
	const char* moduleName,
	const char* userName,
	const char* clientFlag,
	const char* resultCode,
	const char* keyInfo,
	const char* params,
	...
);

void Log_Operate_Warn(
	const char* product,
	const char* moduleName,
	const char* userName,
	const char* clientFlag,
	const char* resultCode,
	const char* keyInfo,
	const char* params,
	...
);

void Log_Operate_Error(	
	const char* product,
	const char* moduleName,
	const char* userName,
	const char* clientFlag,
	const char* resultCode,
	const char* keyInfo,
	const char* params,
	...
);

void Log_Run_Debug(const char* product, const char* param);
void Log_Run_Info(const char* product, const char* param);
void Log_Run_Warn(const char* product, const char* param);
void Log_Run_Error(const char* product, const char* param);

int setLogPropertyEx(const char* product, unsigned int logSize[LOG_CATEGORY], unsigned int logNum[LOG_CATEGORY]);
};

#endif
