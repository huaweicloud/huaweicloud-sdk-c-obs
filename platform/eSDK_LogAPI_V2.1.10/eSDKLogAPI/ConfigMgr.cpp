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
#include <windows.h>
#include <Shlwapi.h>
#endif
#include "ConfigMgr.h"
#include "eSDKTool.h"


namespace eSDK {

	ConfigMgr::ConfigMgr(void)
		: m_uiLogSize_Interface(0)
		, m_uiLogSize_Operation(0)
		, m_uiLogSize_Run(0)
		, m_uiLogNum_Interface(0)
		, m_uiLogNum_Operation(0)
		, m_uiLogNum_Run(0)
		, m_uiLogLevel_Interface(0)
		, m_uiLogLevel_Operation(0)
		, m_uiLogLevel_Run(0)
		, m_strProductName("")
		, m_strLogPath("")
		, m_uiLogFilePermission(0640)
	{
	}

	ConfigMgr::~ConfigMgr(void)
	{
		try
		{
			m_uiLogSize_Interface = 0;
			m_uiLogSize_Operation = 0;
			m_uiLogSize_Run = 0;
			m_uiLogNum_Interface = 0;
			m_uiLogNum_Operation = 0;
			m_uiLogNum_Run = 0;
			m_uiLogLevel_Interface = 0;
			m_uiLogLevel_Operation = 0;
			m_uiLogLevel_Run = 0;
			m_strProductName = "";
			m_strLogPath = "";
		}
		catch (...)
		{
			
		}
	}

	ConfigMgr& ConfigMgr::Instance(void)
	{
		static ConfigMgr s_ConfigMgr;
		return s_ConfigMgr;
	}
    
    void ConfigMgr::InitWithDefaultVal()
    {
        m_uiLogSize_Interface = 10240;
        m_uiLogSize_Operation = 10240;
        m_uiLogSize_Run = 10240;
        m_uiLogNum_Interface = 10;
        m_uiLogNum_Operation = 10;
        m_uiLogNum_Run = 10;
        m_uiLogLevel_Interface = 0;
        m_uiLogLevel_Operation = 0;
        m_uiLogLevel_Run = 0;
		m_strLogPath = "";
		m_uiLogFilePermission = 0640;
    }

	// 初始化
#if defined ANDROID
	bool ConfigMgr::Init(const std::string& iniInfo, const std::string& product)
	{
		// 校验配置文件内容是否为空
		if (iniInfo.empty())
		{//没有配置文件时用默认的
			//return false;
			InitWithDefaultVal();
			m_strProductName = product;
			return true;
		}

		// out param
		std::string tempValue("");

		// 获取LogSize
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogSize_Interface", iniInfo.c_str(), tempValue);
		m_uiLogSize_Interface = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogSize_Operation", iniInfo.c_str(), tempValue);
		m_uiLogSize_Operation = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogSize_Run", iniInfo.c_str(), tempValue);
		m_uiLogSize_Run = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();

		// 获取LogNum
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogNum_Interface", iniInfo.c_str(), tempValue);
		m_uiLogNum_Interface = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogNum_Operation", iniInfo.c_str(), tempValue);
		m_uiLogNum_Operation = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogNum_Run", iniInfo.c_str(), tempValue);
		m_uiLogNum_Run = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();

		// 获取LogLevel
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogLevel_Interface", iniInfo.c_str(), tempValue);
		m_uiLogLevel_Interface = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogLevel_Operation", iniInfo.c_str(), tempValue);
		m_uiLogLevel_Operation = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogLevel_Run", iniInfo.c_str(), tempValue);
		m_uiLogLevel_Run = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();

		// 获取LogNum
		eSDKTool::GetIniSectionItem(SECTION_NAME_PRODUCT, "sdkname", iniInfo.c_str(), tempValue);
		m_strProductName = tempValue;
		tempValue.clear();

		// 获取LogPath
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOGPATH, "LogPath", iniInfo.c_str(), tempValue);
		m_strLogPath = tempValue;
		tempValue.clear();

		return true;
	}
#else
	bool ConfigMgr::Init(const std::string& iniFile, const std::string& product)
	{
		// 校验配置文件路径
		if (!eSDKTool::IsPathFileExist(iniFile.c_str()))
		{//没有配置文件时用默认的
			//return false;
            InitWithDefaultVal();
            m_strProductName = product;
            return true;
		}

		// out param
		std::string tempValue("");

		// 获取LogSize
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogSize_Interface", iniFile.c_str(), tempValue);
		m_uiLogSize_Interface = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogSize_Operation", iniFile.c_str(), tempValue);
		m_uiLogSize_Operation = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogSize_Run", iniFile.c_str(), tempValue);
		m_uiLogSize_Run = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();

		// 获取LogNum
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogNum_Interface", iniFile.c_str(), tempValue);
		m_uiLogNum_Interface = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogNum_Operation", iniFile.c_str(), tempValue);
		m_uiLogNum_Operation = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogNum_Run", iniFile.c_str(), tempValue);
		m_uiLogNum_Run = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();

		// 获取LogLevel
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogLevel_Interface", iniFile.c_str(), tempValue);
		m_uiLogLevel_Interface = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogLevel_Operation", iniFile.c_str(), tempValue);
		m_uiLogLevel_Operation = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogLevel_Run", iniFile.c_str(), tempValue);
		m_uiLogLevel_Run = (unsigned int)atoi(tempValue.c_str());
		tempValue.clear();

		// 获取sdkname
		eSDKTool::GetIniSectionItem(SECTION_NAME_PRODUCT, "sdkname", iniFile.c_str(), tempValue);
		m_strProductName = tempValue;
		tempValue.clear();

		// 获取LogPath
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOGPATH, "LogPath", iniFile.c_str(), tempValue);
		m_strLogPath = tempValue;
		tempValue.clear();

		// 获取FilePermission
		eSDKTool::GetIniSectionItem(SECTION_NAME_LOG, "LogFilePermission", iniFile.c_str(), tempValue);
		std::string strPermission = tempValue;
		m_uiLogFilePermission = eSDKTool::StringToOCT(strPermission);
		tempValue.clear();

		return true;
	}
#endif

	// 去初始化
	bool ConfigMgr::Uninit(void)
	{
		m_uiLogSize_Interface = 0;
		m_uiLogSize_Operation = 0;
		m_uiLogSize_Run = 0;
		m_uiLogNum_Interface = 0;
		m_uiLogNum_Operation = 0;
		m_uiLogNum_Run = 0;
		m_uiLogLevel_Interface = 0;
		m_uiLogLevel_Operation = 0;
		m_uiLogLevel_Run = 0;
		m_strProductName = "";
		return true;
	}

	// 获取日志参数
	unsigned int ConfigMgr::GetLogSize_Interface(void) const
	{
		return m_uiLogSize_Interface;
	}
	unsigned int ConfigMgr::GetLogSize_Operation(void) const
	{
		return m_uiLogSize_Operation;
	}
	unsigned int ConfigMgr::GetLogSize_Run(void) const
	{
		return m_uiLogSize_Run;
	}
	unsigned int ConfigMgr::GetLogNum_Interface(void) const
	{
		return m_uiLogNum_Interface;
	}
	unsigned int ConfigMgr::GetLogNum_Operation(void) const
	{
		return m_uiLogNum_Operation;
	}
	unsigned int ConfigMgr::GetLogNum_Run(void) const
	{
		return m_uiLogNum_Run;
	}
	unsigned int ConfigMgr::GetLogLevel_Interface(void) const
	{
		return m_uiLogLevel_Interface;
	}
	unsigned int ConfigMgr::GetLogLevel_Operation(void) const
	{
		return m_uiLogLevel_Operation;
	}
	unsigned int ConfigMgr::GetLogLevel_Run(void) const
	{
		return m_uiLogLevel_Run;
	}
	const std::string& ConfigMgr::GetLogProduct(void) const
	{
		return m_strProductName;
	}
	std::string ConfigMgr::GetLogPath()
	{
		return m_strLogPath;
	}
	// 日志文件读取权限 add by cwx298983 2016.06.29 Start
	unsigned int ConfigMgr::GetFilePermission(void) const
	{
		if (0 != (m_uiLogFilePermission & 0xFE00) || 0 == m_uiLogFilePermission)
		{
			return 0640;
		}
		else
		{
			return m_uiLogFilePermission;
		}		
	}
	// 日志文件读取权限 add by cwx298983 2016.06.29 End

	// 设置日志参数
	void ConfigMgr::SetLogSize_Interface(const unsigned int nSize)
	{
		m_uiLogSize_Interface = nSize;
	}
	void ConfigMgr::SetLogSize_Operation(const unsigned int nSize)
	{
		m_uiLogSize_Operation = nSize;
	}
	void ConfigMgr::SetLogSize_Run(const unsigned int nSize)
	{
		m_uiLogSize_Run = nSize;
	}
	void ConfigMgr::SetLogNum_Interface(const unsigned int nNum)
	{
		m_uiLogNum_Interface = nNum;
	}
	void ConfigMgr::SetLogNum_Operation(const unsigned int nNum)
	{
		m_uiLogNum_Operation = nNum;
	}
	void ConfigMgr::SetLogNum_Run(const unsigned int nNum)
	{
		m_uiLogNum_Run = nNum;
	}
	void ConfigMgr::SetLogLevel_Interface(const unsigned int nLevel)
	{
		m_uiLogLevel_Interface = nLevel;
	}
	void ConfigMgr::SetLogLevel_Operation(const unsigned int nLevel)
	{
		m_uiLogLevel_Operation = nLevel;
	}
	void ConfigMgr::SetLogLevel_Run(const unsigned int nLevel)
	{
		m_uiLogLevel_Run = nLevel;
	}
	void ConfigMgr::SetLogPath(const std::string& strLogPath)
	{
		m_strLogPath = strLogPath;
	}
}

