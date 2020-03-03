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
#include "eSDKLog.h"
#include "eSDKTool.h"
#include "ConfigMgr.h"
#include "LoggerMgr.h"
#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

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

//lint -e429
bool eSDKLog::InitLog4cpp(const std::string& product, unsigned int logLevel[LOG_CATEGORY], const std::string& logPath, int mode)
{
	m_logPath = logPath;
	m_nInterfaceLevel = logLevel[LOG_CATEGORY_INTERFACE];

	// 初始化日志路径
	std::string strInterfacePath = GetLog4cppPath(logPath, LOG_INTERFACE_FILE);
	// 判断路径是否为空，如果为空，则返回false
	D_IF_PATH_EMPTY(strInterfacePath);

	std::string strOperationPath = GetLog4cppPath(logPath, LOG_OPERATE_FILE);
	D_IF_PATH_EMPTY(strOperationPath);

	std::string strRunPath = GetLog4cppPath(logPath, LOG_RUN_FILE);
	D_IF_PATH_EMPTY(strRunPath);

	m_InstanceInterfaceName = product + LOG_INTERFACE_INSTANCE;
	m_InstanceOperationName = product + LOG_OPERATE_INSTANCE;
	m_InstanceRunName = product + LOG_RUN_INSTANCE;

	// log4cpp相关的对象new之后不需要手动释放，内存管理交由log4cpp
	//interface log file
	log4cpp::PatternLayout* pInterfaceLayout = new log4cpp::PatternLayout();
	pInterfaceLayout->setConversionPattern(LOG_INTERFACE_PATTERN);
	log4cpp::RollingFileAppender* rollfileAppenderInterface = new log4cpp::RollingFileAppender(
		m_InstanceInterfaceName,
		strInterfacePath,//必须使用绝对路径
		ConfigMgrInstance().GetLogSize_Interface()*1024,//单位KB
		ConfigMgrInstance().GetLogNum_Interface(),
		true,
		mode);
	rollfileAppenderInterface->setLayout(pInterfaceLayout);
	log4cpp::Category& logInterfaceCategory = log4cpp::Category::getInstance(m_InstanceInterfaceName);
	logInterfaceCategory.setAdditivity(false); 
	logInterfaceCategory.addAppender(rollfileAppenderInterface);
	logInterfaceCategory.setPriority(GetLog4cppLevel(logLevel[LOG_CATEGORY_INTERFACE], LOG_INTERFACE_INSTANCE));

	//operation log file
	log4cpp::PatternLayout* pOperationLayout = new log4cpp::PatternLayout();
	pOperationLayout->setConversionPattern(LOG_OPERATION_PATTERN);
	log4cpp::RollingFileAppender* rollfileAppenderOperation = new log4cpp::RollingFileAppender(
		m_InstanceOperationName,
		strOperationPath,//必须使用绝对路径
		ConfigMgrInstance().GetLogSize_Operation()*1024,//单位KB
		ConfigMgrInstance().GetLogNum_Operation(),
		true,
		mode);
	rollfileAppenderOperation->setLayout(pOperationLayout);
	log4cpp::Category& logOperationCategory = log4cpp::Category::getInstance(m_InstanceOperationName);
	logOperationCategory.setAdditivity(false); 
	logOperationCategory.addAppender(rollfileAppenderOperation);
	logOperationCategory.setPriority(GetLog4cppLevel(logLevel[LOG_CATEGORY_OPERATION], LOG_OPERATE_INSTANCE));

	//run log file
	log4cpp::PatternLayout* pRunLayout = new log4cpp::PatternLayout();
	pRunLayout->setConversionPattern(LOG_RUN_PATTERN);
	log4cpp::RollingFileAppender* rollfileAppenderRun = new log4cpp::RollingFileAppender(
		m_InstanceRunName,
		strRunPath,//必须使用绝对路径
		ConfigMgrInstance().GetLogSize_Run()*1024,//单位KB
		ConfigMgrInstance().GetLogNum_Run(),
		true,
		mode);
	rollfileAppenderRun->setLayout(pRunLayout);
	log4cpp::Category& logRunCategory = log4cpp::Category::getInstance(m_InstanceRunName);
	logRunCategory.setAdditivity(false); 
	logRunCategory.addAppender(rollfileAppenderRun);
	logRunCategory.setPriority(GetLog4cppLevel(logLevel[LOG_CATEGORY_RUN], LOG_RUN_INSTANCE));
	/*lint -restore*/

	logRunCategory.critStream() << "==============log start============";	

	return true;
}
//lint +e429

void eSDKLog::InvokeIntLogRolling(void)
{
	log4cpp::RollingFileAppender* pInfaceRollApper = dynamic_cast<log4cpp::RollingFileAppender*>(log4cpp::RollingFileAppender::getAppender(m_InstanceInterfaceName));
	if(NULL != pInfaceRollApper)
	{
		(void)VOS_MutexLock(m_IntMutex);
		pInfaceRollApper->rollOver();
		(void)VOS_MutexUnlock(m_IntMutex);
	}
}

void eSDKLog::InvokeOptLogRolling(void)
{
	log4cpp::RollingFileAppender* pOperRollApper = dynamic_cast<log4cpp::RollingFileAppender*>(log4cpp::RollingFileAppender::getAppender(m_InstanceOperationName));
	if(NULL != pOperRollApper)
	{
		(void)VOS_MutexLock(m_OptMutex);
		pOperRollApper->rollOver();
		(void)VOS_MutexUnlock(m_OptMutex);
	}
}

void eSDKLog::InvokeRunLogRolling(void)
{
	log4cpp::RollingFileAppender* pRunRollApper = dynamic_cast<log4cpp::RollingFileAppender*>(log4cpp::RollingFileAppender::getAppender(m_InstanceRunName));
	if(NULL != pRunRollApper)
	{
		(void)VOS_MutexLock(m_RunMutex);
		pRunRollApper->rollOver();
		(void)VOS_MutexUnlock(m_RunMutex);
	}
}

void eSDKLog::UninitLog4cpp(void)
{
	try
	{
		if (!m_InstanceInterfaceName.empty() && !m_InstanceOperationName.empty() && !m_InstanceRunName.empty())
		{
			log4cpp::Category& logInterfaceCategory = log4cpp::Category::getInstance(m_InstanceInterfaceName);
			log4cpp::Category& logOperationCategory = log4cpp::Category::getInstance(m_InstanceOperationName);
			log4cpp::Category& logRunCategory = log4cpp::Category::getInstance(m_InstanceRunName);

			logRunCategory.critStream() << "==============log end==============";

			logInterfaceCategory.removeAllAppenders();
			logOperationCategory.removeAllAppenders();
			logRunCategory.removeAllAppenders();
		}	
	}
	catch (...)
	{
	}
}

void eSDKLog::ShutDownLog4cpp(void)
{
    try
    {
		if (!m_InstanceRunName.empty())
		{
			log4cpp::Category& logRunCategory = log4cpp::Category::getInstance(m_InstanceRunName);

			logRunCategory.critStream() << "==============log end==============";

			log4cpp::Category::shutdown();
		}       
    }
    catch (...)
    {
    }
}

void eSDKLog::printIntInfolog(const std::string& strcontent)
{
	log4cpp::Category& logInterfaceCategory = log4cpp::Category::getInstance(m_InstanceInterfaceName);
	(void)VOS_MutexLock(m_IntMutex);
	logInterfaceCategory.infoStream() << strcontent;
	(void)VOS_MutexUnlock(m_IntMutex);
}

void eSDKLog::printIntErrorlog(const std::string& strcontent)
{
	log4cpp::Category& logInterfaceCategory = log4cpp::Category::getInstance(m_InstanceInterfaceName);
	(void)VOS_MutexLock(m_IntMutex);
	logInterfaceCategory.errorStream() << strcontent;
	(void)VOS_MutexUnlock(m_IntMutex);
}

void eSDKLog::printOptDebuglog(const std::string& strcontent)
{
	log4cpp::Category& logOperateCategory = log4cpp::Category::getInstance(m_InstanceOperationName);
	(void)VOS_MutexLock(m_OptMutex);
	logOperateCategory.debugStream() << strcontent;
	(void)VOS_MutexUnlock(m_OptMutex);
}

void eSDKLog::printOptInfolog(const std::string& strcontent)
{
	log4cpp::Category& logOperateCategory = log4cpp::Category::getInstance(m_InstanceOperationName);
	(void)VOS_MutexLock(m_OptMutex);
	logOperateCategory.infoStream() << strcontent;
	(void)VOS_MutexUnlock(m_OptMutex);
}

void eSDKLog::printOptWarnlog(const std::string& strcontent)
{
	log4cpp::Category& logOperateCategory = log4cpp::Category::getInstance(m_InstanceOperationName);
	(void)VOS_MutexLock(m_OptMutex);
	logOperateCategory.warnStream() << strcontent;
	(void)VOS_MutexUnlock(m_OptMutex);
}

void eSDKLog::printOptErrorlog(const std::string& strcontent)
{
	log4cpp::Category& logOperateCategory = log4cpp::Category::getInstance(m_InstanceOperationName);
	(void)VOS_MutexLock(m_OptMutex);
	logOperateCategory.errorStream() << strcontent;
	(void)VOS_MutexUnlock(m_OptMutex);
}

void eSDKLog::printRunDebuglog(const std::string& strcontent)
{
	log4cpp::Category& logRunCategory = log4cpp::Category::getInstance(m_InstanceRunName);
	(void)VOS_MutexLock(m_RunMutex);
	logRunCategory.debugStream() << strcontent;
	(void)VOS_MutexUnlock(m_RunMutex);
}

void eSDKLog::printRunInfolog(const std::string& strcontent)
{
	log4cpp::Category& logRunCategory = log4cpp::Category::getInstance(m_InstanceRunName);
	(void)VOS_MutexLock(m_RunMutex);
	logRunCategory.infoStream() << strcontent;
	(void)VOS_MutexUnlock(m_RunMutex);
}

void eSDKLog::printRunWarnlog(const std::string& strcontent)
{
	log4cpp::Category& logRunCategory = log4cpp::Category::getInstance(m_InstanceRunName);
	(void)VOS_MutexLock(m_RunMutex);
	logRunCategory.warnStream() << strcontent;
	(void)VOS_MutexUnlock(m_RunMutex);
}

void eSDKLog::printRunErrorlog(const std::string& strcontent)
{
	log4cpp::Category& logRunCategory = log4cpp::Category::getInstance(m_InstanceRunName);
	(void)VOS_MutexLock(m_RunMutex);
	logRunCategory.errorStream() << strcontent;
	(void)VOS_MutexUnlock(m_RunMutex);
}

std::string eSDKLog::GetLog4cppPath(const std::string& logPath, const std::string& strLogType)
{
	std::string log4cppPath("");

	// 使用默认日志路径
	if (INVALID_LOG_PATH == logPath)
	{
		log4cppPath = eSDKTool::GetAppPath();
		log4cppPath.append("log");
		if (eSDKTool::CreateMultipleDirectory(log4cppPath.c_str()))
		{
#ifdef WIN32
			log4cppPath.append("\\");
#else
            log4cppPath.append("/");
#endif
			log4cppPath.append(ConfigMgrInstance().GetLogProduct());
			log4cppPath.append(".");
			log4cppPath.append(strLogType);
		}
		else
		{
			log4cppPath = "";
		}
	}
	// 创建日志路径
	else
	{
		// 相对目录判断
		if (eSDKTool::IsRelativePath(logPath))
		{
			return log4cppPath;
		}

		if (eSDKTool::CreateMultipleDirectory(logPath.c_str()))
		{
			log4cppPath = logPath;
#ifdef WIN32
            if ('\\' != logPath.back())
			{
				log4cppPath.append("\\");
			}
#else			
            if ('/' != logPath[logPath.size() - 1])
			{
				log4cppPath.append("/");
			}
#endif
			log4cppPath.append(ConfigMgrInstance().GetLogProduct());
			log4cppPath.append(".");
			log4cppPath.append(strLogType);

			// 先判断文件夹下是否有与日志文件同名的文件夹 modify by cwx298983 2016.02.29 Start
			if (eSDKTool::IsDir(log4cppPath))
			{
				log4cppPath = "";
			}
			// 先判断文件夹下是否有与日志文件同名的文件夹 modify by cwx298983 2016.02.29 End
		}
	}

	return log4cppPath;
}

log4cpp::Priority::PriorityLevel eSDKLog::GetLog4cppLevel(unsigned int uiLogLevel, const std::string& strLogType)
{
	if (LOG_INTERFACE_INSTANCE == strLogType)
	{
		uiLogLevel = ConfigMgrInstance().GetLogLevel_Interface();
	}
	else if (LOG_OPERATE_INSTANCE == strLogType)
	{
		uiLogLevel = ConfigMgrInstance().GetLogLevel_Operation();
	}
	else if (LOG_RUN_INSTANCE == strLogType)
	{
		uiLogLevel = ConfigMgrInstance().GetLogLevel_Run();
	}

	switch (uiLogLevel)
	{
	case DEBUG_LEVEL:
		return log4cpp::Priority::DEBUG;
	case INFO_LEVEL:
		return log4cpp::Priority::INFO;
	case WARN_LEVEL:
		return log4cpp::Priority::WARN;
	case ERROR_LEVEL:
		return log4cpp::Priority::ERROR;
	default:
		return log4cpp::Priority::INFO;
	}
}
