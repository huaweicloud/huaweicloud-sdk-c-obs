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
#include <time.h>
#endif
#include "eSDKLog.h"
#include "eSDKTool.h"
#include "ConfigMgr.h"
#include "LoggerMgr.h"
#include "securec.h"
#ifdef WIN32
#define SPDLOG_WCHAR_FILENAMES
#include <io.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"    // support for basic file logging
#include "spdlog/sinks/rotating_file_sink.h" // support for rotating file logging
#include <mutex>

extern bool g_bCloseInterfaceLog;

#define D_IF_PATH_EMPTY(strPath)\
	if (strPath.empty())\
	{\
		return false;\
	}

eSDKLog::eSDKLog(void)
	: m_InstanceInterfaceName("")
	, m_InstanceOperationName("")
	, m_InstanceRunName("")
	, m_logPath("")
	, m_nInterfaceLevel(0)
{
	m_IntMutex = VOS_CreateMutex();
	m_OptMutex = VOS_CreateMutex();
	m_RunMutex = VOS_CreateMutex();
	m_IntBackupMutex = VOS_CreateMutex();
}

eSDKLog::~eSDKLog(void)
{
	try
    {
		m_InstanceInterfaceName = "";
		m_InstanceOperationName = "";
		m_InstanceRunName = "";

		if(VOS_NULL != m_IntMutex)
		{
			(void)VOS_DestroyMutex(m_IntMutex);
			m_IntMutex = VOS_NULL;
		}
		if(VOS_NULL != m_OptMutex)
		{
			(void)VOS_DestroyMutex(m_OptMutex);
			m_OptMutex = VOS_NULL;
		}
		if(VOS_NULL != m_RunMutex)
		{
			(void)VOS_DestroyMutex(m_RunMutex);
			m_RunMutex = VOS_NULL;
		}
		if (VOS_NULL != m_IntBackupMutex)
		{
			(void)VOS_DestroyMutex(m_IntBackupMutex);
			m_IntBackupMutex = VOS_NULL;
		}
	}
	catch(...)
	{
    }
}

wchar_t *GetWcharFromChar(const char *char_str)
{
	const size_t char_str_len = strlen(char_str);
	const size_t wchar_str_size = char_str_len + 1;
	wchar_t *wchar_str = (wchar_t *)malloc(sizeof(wchar_t) * wchar_str_size);
	if (wchar_str == NULL) {
		return wchar_str;
	}
	memset_s(wchar_str, sizeof(wchar_t) * wchar_str_size, 0, sizeof(wchar_t) * wchar_str_size);
	int ret = mbstowcs(wchar_str, char_str, char_str_len);
	if (ret != 0) {
		free(wchar_str);
		wchar_str = 0;
	}
	else {
		wchar_str[char_str_len] = L'\0';
	}

	return wchar_str;
}

static std::mutex locker;
static std::shared_ptr<spdlog::logger> runLogger;
static std::shared_ptr<spdlog::logger> operationLogger;
static std::shared_ptr<spdlog::logger> interfaceLogger;

#ifdef WIN32
bool eSDKLog::InitSPDLOG(const std::string& product, unsigned int logLevel[LOG_CATEGORY], const std::wstring& logPath, int mode) {
	
	m_logPath_w = logPath;
	m_nInterfaceLevel = logLevel[LOG_CATEGORY_INTERFACE];
	bool initiateSuccessfully = false;
	locker.lock();
	m_InstanceInterfaceName = product + LOG_INTERFACE_INSTANCE;
	m_InstanceOperationName = product + LOG_OPERATE_INSTANCE;
	m_InstanceRunName = product + LOG_RUN_INSTANCE;
	wchar_t* interfaceName_w = GetWcharFromChar(m_InstanceInterfaceName.c_str());
	wchar_t* operationName_w = GetWcharFromChar(m_InstanceOperationName.c_str());
	wchar_t* runName_w = GetWcharFromChar(m_InstanceRunName.c_str());
	wstring instanceInterfaceName_W;
	wstring instanceOperationName_W;
	wstring instanceRunName_W;

	if (interfaceName_w == nullptr) {
		instanceInterfaceName_W = L"obs-sdk-c.interface";
	} else {
		instanceInterfaceName_W = interfaceName_w;
		free(interfaceName_w);
		interfaceName_w = nullptr;
	}

	if (operationName_w == nullptr) {
		instanceOperationName_W = L"obs-sdk-c.operation";
	}
	else {
		instanceOperationName_W = operationName_w;
		free(operationName_w);
		operationName_w = nullptr;
	}

	if (runName_w == nullptr) {
		instanceRunName_W = L"obs-sdk-c.run";
	}
	else {
		instanceRunName_W = runName_w;
		free(runName_w);
		runName_w = nullptr;
	}

	try {
		runLogger = spdlog::rotating_logger_mt(m_InstanceRunName, logPath + L"/" + instanceRunName_W + L".log"
			, ConfigMgrInstance().GetLogSize_Run() * 1024, ConfigMgrInstance().GetLogNum_Run());
		runLogger->set_level(spdlog::level::level_enum(spdlog::level::debug + ConfigMgrInstance().GetLogLevel_Run()));
		//设置日志格式为 时间（精确到毫秒） 线程号 日志名 日志级别 自定义信息
		runLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [tid:%t] [%n] [%l] %v");
		initiateSuccessfully = true;
	}
	catch (const spdlog::spdlog_ex &ex) {
		initiateSuccessfully = false;
	}

	if (initiateSuccessfully) {
		try {
			operationLogger = spdlog::rotating_logger_mt(m_InstanceOperationName, logPath + L"/" + instanceOperationName_W + L".log"
				, ConfigMgrInstance().GetLogSize_Run() * 1024, ConfigMgrInstance().GetLogNum_Run());
			operationLogger->set_level(spdlog::level::level_enum(spdlog::level::debug + ConfigMgrInstance().GetLogLevel_Operation()));
			//设置日志格式为 时间（精确到毫秒） 线程号 日志名 日志级别 自定义信息
			operationLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [tid:%t] [%n] [%l] %v");
			initiateSuccessfully = true;
		}
		catch (const spdlog::spdlog_ex &ex) {
			initiateSuccessfully = false;
		}
	}

	if (initiateSuccessfully) {
		try {
			interfaceLogger = spdlog::rotating_logger_mt(m_InstanceInterfaceName, logPath + L"/" + instanceInterfaceName_W + L".log"
				, ConfigMgrInstance().GetLogSize_Run() * 1024, ConfigMgrInstance().GetLogNum_Run());
			interfaceLogger->set_level(spdlog::level::level_enum(spdlog::level::debug + ConfigMgrInstance().GetLogLevel_Interface()));
			//设置日志格式为 时间（精确到毫秒） 线程号 日志名 日志级别 自定义信息
			interfaceLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [tid:%t] [%n] [%l] %v");
			initiateSuccessfully = true;
		}
		catch (const spdlog::spdlog_ex &ex) {
			initiateSuccessfully = false;
		}
	}
	locker.unlock();
	return initiateSuccessfully;
}
#else
bool eSDKLog::InitSPDLOG(const std::string& product, unsigned int logLevel[LOG_CATEGORY], const std::string& logPath, int mode){

	m_logPath = logPath;
	m_nInterfaceLevel = logLevel[LOG_CATEGORY_INTERFACE];
	bool initiateSuccessfully = false;
	locker.lock();
	m_InstanceInterfaceName = product + LOG_INTERFACE_INSTANCE;
	m_InstanceOperationName = product + LOG_OPERATE_INSTANCE;
	m_InstanceRunName = product + LOG_RUN_INSTANCE;

	try {
		runLogger = spdlog::rotating_logger_mt(m_InstanceRunName, logPath + "/" + m_InstanceRunName + ".log"
			, ConfigMgrInstance().GetLogSize_Run() * 1024, ConfigMgrInstance().GetLogNum_Run());
		runLogger->set_level(spdlog::level::level_enum(spdlog::level::debug + ConfigMgrInstance().GetLogLevel_Run()));
		//设置日志格式为 时间（精确到毫秒） 线程号 日志名 日志级别 自定义信息
		runLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [tid:%t] [%n] [%l] %v");
		initiateSuccessfully = true;
	}
	catch (const spdlog::spdlog_ex &ex) {
		initiateSuccessfully = false;
	}

	if (initiateSuccessfully) {
		try {
			operationLogger = spdlog::rotating_logger_mt(m_InstanceOperationName, logPath + "/" + m_InstanceOperationName + ".log"
				, ConfigMgrInstance().GetLogSize_Run() * 1024, ConfigMgrInstance().GetLogNum_Run());
			operationLogger->set_level(spdlog::level::level_enum(spdlog::level::debug + ConfigMgrInstance().GetLogLevel_Operation()));
			//设置日志格式为 时间（精确到毫秒） 线程号 日志名 日志级别 自定义信息
			operationLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [tid:%t] [%n] [%l] %v");
			initiateSuccessfully = true;
		}
		catch (const spdlog::spdlog_ex &ex) {
			initiateSuccessfully = false;
		}
	}

	if (initiateSuccessfully) {
		try {
			interfaceLogger = spdlog::rotating_logger_mt(m_InstanceInterfaceName, logPath + "/" + m_InstanceInterfaceName + ".log"
				, ConfigMgrInstance().GetLogSize_Run() * 1024, ConfigMgrInstance().GetLogNum_Run());
			interfaceLogger->set_level(spdlog::level::level_enum(spdlog::level::debug + ConfigMgrInstance().GetLogLevel_Interface()));
			//设置日志格式为 时间（精确到毫秒） 线程号 日志名 日志级别 自定义信息
			interfaceLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [tid:%t] [%n] [%l] %v");
			initiateSuccessfully = true;
		}
		catch (const spdlog::spdlog_ex &ex) {
			initiateSuccessfully = false;
		}
	}
	locker.unlock();
	return initiateSuccessfully;
}
#endif

void eSDKLog::InvokeIntLogRolling(void)
{
}

void eSDKLog::InvokeOptLogRolling(void)
{
}

void eSDKLog::InvokeRunLogRolling(void)
{
}

void eSDKLog::UninitSPDLog(void)
{
	if (!m_InstanceInterfaceName.empty() && runLogger != nullptr)
	{
		interfaceLogger->critical("==============log end==============");
		interfaceLogger.reset();
	}
	if (!m_InstanceOperationName.empty() && runLogger != nullptr)
	{
		operationLogger->critical("==============log end==============");
		operationLogger.reset();
	}
	if (!m_InstanceRunName.empty() && runLogger != nullptr)
	{
		runLogger->critical("==============log end==============");
		runLogger.reset();
	}
}

void eSDKLog::ShutDownSPDLog(void)
{
	if (!m_InstanceRunName.empty() && runLogger != nullptr)
	{
		runLogger->critical("==============log end==============");
		runLogger.reset();
	}  
}

void eSDKLog::printIntInfolog(const std::string& strcontent)
{
	if(interfaceLogger == nullptr){
		return;
	}
	interfaceLogger->info(strcontent);
}

void eSDKLog::printIntErrorlog(const std::string& strcontent)
{
	if(interfaceLogger == nullptr){
		return;
	}
	interfaceLogger->error(strcontent);
}

void eSDKLog::printOptDebuglog(const std::string& strcontent)
{
	if(operationLogger == nullptr){
		return;
	}
	operationLogger->debug(strcontent);
}

void eSDKLog::printOptInfolog(const std::string& strcontent)
{
	if(operationLogger == nullptr){
		return;
	}
	operationLogger->info(strcontent);
}

void eSDKLog::printOptWarnlog(const std::string& strcontent)
{
	if(operationLogger == nullptr){
		return;
	}
	operationLogger->warn(strcontent);
}

void eSDKLog::printOptErrorlog(const std::string& strcontent)
{
	if(operationLogger == nullptr){
		return;
	}
	operationLogger->error(strcontent);
}

void eSDKLog::printRunDebuglog(const std::string& strcontent)
{
	if(runLogger == nullptr){
		return;
	}
	runLogger->debug(strcontent);
}

void eSDKLog::printRunInfolog(const std::string& strcontent)
{
	if(runLogger == nullptr){
		return;
	}
	runLogger->info(strcontent);
}

void eSDKLog::printRunWarnlog(const std::string& strcontent)
{
	if(runLogger == nullptr){
		return;
	}
	runLogger->warn(strcontent);
}

void eSDKLog::printRunErrorlog(const std::string& strcontent)
{
	if(runLogger == nullptr){
		return;
	}
	runLogger->error(strcontent);
}

std::string eSDKLog::GetLogPath(const std::string& logPath, const std::string& strLogType)
{
	std::string logPathC("");

	// 使用默认日志路径
	if (INVALID_LOG_PATH == logPath)
	{
		logPathC = eSDKTool::GetAppPath();
		logPathC.append("log");
		if (eSDKTool::CreateMultipleDirectory(logPathC.c_str()))
		{
#ifdef WIN32
			logPathC.append("\\");
#else
			logPathC.append("/");
#endif
			logPathC.append(ConfigMgrInstance().GetLogProduct());
			logPathC.append(".");
			logPathC.append(strLogType);
		}
		else
		{
			logPathC = "";
		}
	}
	// 创建日志路径
	else
	{
		// 相对目录判断
		if (eSDKTool::IsRelativePath(logPath))
		{
			return logPathC;
		}

		if (eSDKTool::CreateMultipleDirectory(logPath.c_str()))
		{
			logPathC = logPath;
#ifdef WIN32
            if ('\\' != logPath.back())
			{
				logPathC.append("\\");
			}
#else			
            if ('/' != logPath[logPath.size() - 1])
			{
				logPathC.append("/");
			}
#endif
			logPathC.append(ConfigMgrInstance().GetLogProduct());
			logPathC.append(".");
			logPathC.append(strLogType);

			// 先判断文件夹下是否有与日志文件同名的文件夹 modify by cwx298983 2016.02.29 Start
			if (eSDKTool::IsDir(logPathC))
			{
				logPathC = "";
			}
			// 先判断文件夹下是否有与日志文件同名的文件夹 modify by cwx298983 2016.02.29 End
		}
	}

	return logPathC;
}
