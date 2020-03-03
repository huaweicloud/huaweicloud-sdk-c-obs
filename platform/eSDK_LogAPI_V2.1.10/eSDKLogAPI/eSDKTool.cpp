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
#include <Wincrypt.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif
#include "eSDKTool.h"
#include "eSDKLogDataType.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "securec.h"

#include <sstream>

eSDKTool::eSDKTool(void)
{
}

eSDKTool::~eSDKTool(void)
{
}

std::string eSDKTool::GetDllPath()
{
#ifdef WIN32
	HMODULE hModule = GetModuleHandle("eSDKLogAPI.dll");
	char path[MAX_PATH] = {0};
	GetModuleFileName(hModule, path, MAX_PATH);
	std::string strPath(path);
	strPath = strPath.substr(0, strPath.rfind("\\")+1);
	return strPath;
#elif defined(TARGET_OS_IPHONE)||defined(TARGET_OS_MAC)
    // 获取程序Documents目录路径
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    return [documentsDirectory UTF8String];
#else
	const char separator = '/';
	char buf[1024] = {0};
	int nRet = -1;
	nRet = GetModuleFilePath("libeSDKLogAPI.so", buf, sizeof(buf));
	if(0 != nRet)
	{
		// print log;
		return "";
	}
	strcat_s(buf, 1024, "/");
	char* pch = strrchr(buf, separator);
	if(NULL == pch)
	{
		return "";
	}
	if(pch == buf)
	{
		return "";
	}
	std::string strPath(buf);
	if ('/' != strPath[strPath.size()-1])
	{
		strPath += "/";
	}
	return strPath;
;
#endif
}

std::string eSDKTool::GetAppPath()
{
#ifdef WIN32
	char path[MAX_PATH] = {0};
	GetModuleFileName(NULL, path, MAX_PATH);
	std::string strPath(path);
	strPath = strPath.substr(0, strPath.rfind("\\")+1);
	return strPath;
#elif defined(TARGET_OS_IPHONE)||defined(TARGET_OS_MAC)
    // 获取程序Documents目录路径
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    return [documentsDirectory UTF8String];
#else
    return "";
#endif
}

bool eSDKTool::IsPathFileExist(const std::string& path)
{
#ifdef WIN32
    if (PathFileExists(path.c_str()))
	{
		return true;
	}
    
#elif defined(TARGET_OS_IPHONE)||defined(TARGET_OS_MAC)
    NSFileManager *manager = [NSFileManager defaultManager];
    
    BOOL isDir;
    if ([manager fileExistsAtPath:[NSString stringWithUTF8String:path.c_str()] isDirectory:&isDir])
    {
        // 存在
        return true;
    }
#else
	if (0 == access(path.c_str(), 0))
	{
		return true;
	}
#endif
    return false;
}

bool eSDKTool::IsRelativePath(const std::string& path)
{
#ifdef WIN32
	return (PathIsRelative(path.c_str()) ? true : false);
#elif defined(TARGET_OS_IPHONE)||defined(TARGET_OS_MAC)
    NSString* strPath = [[NSString alloc] initWithUTF8String:path.c_str()];
    BOOL bAbsPath = [strPath isAbsolutePath];
    return !bAbsPath;
#else
	char temp = path.at(0);
	if ('/' == temp ) {
		return false;	// absolute
	}
    return true;	// relative
#endif
}

bool eSDKTool::CreateMultipleDirectory(const std::string& path)
{
#ifdef WIN32
	std::string strPath(path);

	// ≈–???’
	if (strPath.empty())
	{
		return false;
	}
	// ?áπ?“—?≠￥ê‘?∑μa?≥…π?
	if (IsPathFileExist(strPath.c_str()))
	{
		if (!IsDir(path))
		{
			return false;
		}
		return true;
	}
	else
	{
		// ≥???Ω·?≤μ?"\\"
		while ('\\' == strPath.back())
		{
			strPath = strPath.substr(0, strPath.rfind("\\"));
		}

		// aò?°…?o????o
		std::string prePath = strPath.substr(0, strPath.rfind("\\"));

		// ≤a￥ê‘?μ????o
		if (prePath == strPath)
		{
			return false;
		}

		if (IsPathFileExist(prePath.c_str()))
		{
			BOOL bRet = CreateDirectoryA(strPath.c_str(), NULL);
			if (bRet == TRUE)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			// ?áπ??∞√êμ????o≤a￥ê‘?￡¨‘ú??￥￥Ω??∞√êμ????o
			if (CreateMultipleDirectory(prePath))
			{
				// ?∞√ê???o￥￥Ω?∫√?à“‘∫?￡¨‘ú‘?￥￥Ω?∫?√êμ????o
				return CreateMultipleDirectory(strPath);
			}
			return false;
		}
	}
#elif defined(TARGET_MAC_OS)||defined(TARGET_OS_IPHONE)
    std::string strPath(path);
	if (strPath.empty())
	{
		return false;
	}
    
    NSError *error;
    NSFileManager *manager = [NSFileManager defaultManager];
    
    if (IsPathFileExist(strPath.c_str()))
    {
        // 存在
        return true;
    }
    
    return [manager createDirectoryAtPath:[NSString stringWithUTF8String:strPath.c_str()] withIntermediateDirectories:YES attributes:nil error:&error];
#else
	std::string strPath(path);

	// ≈–???’
	if (strPath.empty())
	{
		return false;
	}
	// ?áπ?“—?≠￥ê‘?∑μa?≥…π?
	if (IsPathFileExist(strPath.c_str()))
	{
		return true;
	}
	else
	{
		// ≥???Ω·?≤μ?"\\"
		while ('/' == strPath[strPath.length()-1])
		{
			strPath = strPath.substr(0, strPath.rfind("/"));
		}

		// aò?°…?o????o
		std::string prePath = strPath.substr(0, strPath.rfind("/"));

		// ≤a￥ê‘?μ????o
		if (prePath == strPath)
		{
			return false;
		}

		if (IsPathFileExist(prePath.c_str()))
		{
			int makeID = mkdir(strPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP);
			if (0 == makeID)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			// ?áπ??∞√êμ????o≤a￥ê‘?￡¨‘ú??￥￥Ω??∞√êμ????o
			if (CreateMultipleDirectory(prePath))
			{
				// ?∞√ê???o￥￥Ω?∫√?à“‘∫?￡¨‘ú‘?￥￥Ω?∫?√êμ????o
				return CreateMultipleDirectory(strPath);
			}
			return false;
		}
	}
#endif
}

void eSDKTool::getCurrentPath(std::string& strPath)
{
#ifdef WIN32
	HMODULE hModule = GetModuleHandle("eSDKLogAPI.dll");
	char path[MAX_PATH+1] = {0};
	::GetModuleFileNameA(hModule, path, MAX_PATH);
	std::string strModulePath = path;
	unsigned int loc = strModulePath.find_last_of("\\");
	if( loc != string::npos )
	{
		strPath = strModulePath.substr(0,loc);
	}
#else
	// Linux get current path
	const char separator = '/';
	char buf[1024] = {0};
	int nRet = RET_INVALID_PARA;
	nRet = GetModuleFilePath("libeSDKLogAPI.so", buf, sizeof(buf));
	if(RET_SUCCESS != nRet)
	{
		// print log;
		return ;
	}
	strcat_s(buf, 1024, "/");
	char* pch = strrchr(buf, separator);
	if(NULL == pch)
	{
		return ;
	}
	if(pch == buf)
	{
		return ;
	}
	*pch = '\0';
	strPath = buf;
	return ;
#endif
}


void eSDKTool::GetIPPort(const std::string& server,std::string& ip,unsigned short& port)
{
	ip.clear();
	std::string instr = server;
	string::size_type pos = instr.find(':');
	if( pos != string::npos )
	{
		ip = instr.substr(0,pos);
		std::string outPort = instr.substr(pos+1);
		port = (unsigned short)atoi(outPort.c_str());
	}
	else
	{
		ip = "";
		port = 0;
	}


	return;
}
//o?≤?IPV4??–?μ?IPμ?÷∑??∑ò’??∑
bool eSDKTool::CheckIP4Valid(const std::string &ip)
{
	if (ip.empty())
	{
		return false;
	}

	const unsigned int DOT_NUM = 3;
	const unsigned int MAX_IP = 255;
	unsigned int auIpTemp[4] = {0};
	unsigned int iNum = 0;
	int iValue = 0;
	bool bFlag = false;
	unsigned int iIndex = 0;
	for (;;)
	{
		iValue = ip[iIndex];
		iIndex++;
		if ((iValue >= '0') && (iValue <= '9'))
		{
			bFlag = true;
			auIpTemp[iNum] = auIpTemp[iNum] * 10 + (unsigned int)iValue - '0';
			if (auIpTemp[iNum] > MAX_IP)
			{
				return false;
			}
		}
		else if ('.' == iValue)
		{
			if (!bFlag)
			{
				return false;
			}

			if (DOT_NUM == iNum)
			{
				return false;
			}

			iNum++;
			bFlag = false;
		}
		else if (('\0' == iValue) && (DOT_NUM == iNum) && bFlag)
		{
			break;
		}
		else
		{
			return false;
		}					

	}

	return true;
}
void eSDKTool::GetFileNameByPath(const std::string& strPath,std::string& strFileName)
{
	strFileName.clear();
	std::string instr = strPath;
#ifdef WIN32
	string::size_type pos = instr.rfind('\\');
#else
	string::size_type pos = instr.rfind('/');
#endif
	if( pos != string::npos )
	{
		strFileName = instr.substr(pos+1);
	}
	else
	{
		strFileName = strPath;
	}

}

#if defined ANDROID
void eSDKTool::GetIniSectionItem(const char* Section, const char* Item, const char* FileInfo, std::string& iniValue)
{
	const int LENGTH = 1024;
	char* tchValue = new char[LENGTH];
	memset_s(tchValue, LENGTH, 0, LENGTH*sizeof(char));

	char* iniItem = NULL;
	char Sect[30] = {0};
	char posChar = ' ';
	char linebuf[LENGTH] = {0};
	char oldline[LENGTH] = {0};

	if (NULL == FileInfo)
	{
		delete[] tchValue;
		tchValue = NULL;
		return;
	}

	std::stringstream fileStream(FileInfo);

	memcpy_s(Sect, 30, "[", sizeof("["));
	strcat_s(Sect, 30, Section);
	strcat_s(Sect, 30, "]");

	while(fileStream.get(posChar))
	{
		if('[' == posChar)
		{
			fileStream.unget();
			fileStream.getline(linebuf, LENGTH);
			if(strstr(linebuf, Sect))
			{
				while(fileStream.get(posChar))
				{
					if ('[' == posChar)
					{
						break;
					}

					// ;作为注释，跳过该行
					if (';' == posChar)
					{
						fileStream.unget();
						fileStream.getline(linebuf, LENGTH);
						continue;
					}
					// ;作为注释，跳过该行
					fileStream.unget();
					fileStream.getline(linebuf, LENGTH);
					if(strstr(linebuf, Item))
					{
						std::string strTmp(Item);

						if(iniItem = strchr(linebuf, '='))
						{
							iniItem++;
							if((*iniItem) == '\n')
							{
								delete[] tchValue;
								tchValue = NULL;
								return ;
							}
							iniItem[strlen(iniItem)] = 0;
							memcpy_s(tchValue, LENGTH, iniItem, strlen(iniItem));
							iniValue = std::string(tchValue);
							delete[] tchValue;
							tchValue = NULL;
							return ;
						}
					}
				}
			}
		}
		else
		{
			fileStream.unget();
			fileStream.getline(linebuf, LENGTH);
		}
	}

	delete[] tchValue;
	tchValue = NULL;
	return ;
}
#else
//lint -e438
void eSDKTool::GetIniSectionItem(const char* Section, const char* Item, const char* FileName, std::string& iniValue)
{
	const int LENGTH = 1024;
	char* tchValue = new char[LENGTH];
	memset_s(tchValue, LENGTH, 0, LENGTH*sizeof(char));
#ifdef WIN32
	(void)::GetPrivateProfileString(Section, Item, "", tchValue, LENGTH-1, FileName);
	iniValue = tchValue;
#elif defined(TARGET_MAC_OS)||defined(TARGET_OS_IPHONE)//to support ios, get from plist file
    NSMutableDictionary* dict =  [ [ NSMutableDictionary alloc ] initWithContentsOfFile:[NSString stringWithUTF8String:FileName]];
    NSString* value = [ dict objectForKey:[NSString stringWithUTF8String:Item]];
    iniValue = [value UTF8String];
#else
	char* iniItem = NULL;
	char Sect[30] = {0};
	char posChar = ' ';
	char linebuf[LENGTH] = {0};
	char oldline[LENGTH] = {0};
	FILE* inifp;
	memcpy_s(Sect, 30, "[", sizeof("["));
	strcat_s(Sect, 30, Section);
	strcat_s(Sect, 30, "]");
	inifp = fopen(FileName, "rb");
	if(NULL == inifp)
	{
		delete[] tchValue;
		tchValue = NULL;
		return ;
	}
	while((posChar = fgetc(inifp))!=EOF)
	{
		if('[' == posChar)
		{
			ungetc(posChar, inifp);
			fgets(linebuf,LENGTH, inifp);
			if(strstr(linebuf, Sect))
			{
				while((posChar = fgetc(inifp))!='[' && posChar != EOF)
				{
					// ;作为注释，跳过该行
					if (';' == posChar)
					{
						ungetc(posChar, inifp);
						fgets(linebuf, LENGTH, inifp);
						continue;
					}
					// ;作为注释，跳过该行
					ungetc(posChar, inifp);
					fgets(linebuf, LENGTH, inifp);
					if(strstr(linebuf, Item))
					{
						if(iniItem= strchr(linebuf, '='))
						{
							iniItem++;
							fclose(inifp);
							if((*iniItem) == '\n')
							{
								delete[] tchValue;
								tchValue = NULL;
								return ;
							}
							iniItem[strlen(iniItem)-1] = 0; 
							memcpy_s(tchValue, LENGTH, iniItem, strlen(iniItem));
							iniValue = std::string(tchValue);
							delete[] tchValue;
							tchValue = NULL;
							return ;
						}
					}
				}
				if(EOF == posChar)
				{
					break;
				}
				ungetc(posChar, inifp);
			}
		}
		else
		{
			ungetc(posChar, inifp);
			fgets(linebuf, LENGTH, inifp);
		}
	}
	fclose(inifp);

#endif
	delete[] tchValue;
	tchValue = NULL;
	return ;
}
//lint +e438
#endif

#ifndef WIN32
int eSDKTool::GetModuleFilePath(const char* sModuleName, char* sPath, unsigned int unSize)
{
	int iRet = RET_INVALID_PARA;
	char sLine[1024] = {0};
	FILE *fp = NULL;
	char *pTmpFullDir = NULL;
	char *pTmpModuleDir = NULL;
	
	fp = fopen ("/proc/self/maps", "r");
	if (NULL == fp)
	{
		return iRet;
	}
	while (0 == feof(fp))
	{
		if (NULL == fgets(sLine, sizeof(sLine), fp))											
		{
			continue;
		}
		pTmpFullDir = strchr(sLine, '/');
		if (NULL == strstr(sLine, "r-xp") ||  NULL == pTmpFullDir || NULL == strstr(sLine, sModuleName))
		{
			continue;
		}
		//
		pTmpModuleDir = strrchr(pTmpFullDir, '/');   
		if (pTmpModuleDir == pTmpFullDir)
		{	
			break;		
		}
		*pTmpModuleDir = '\0';
		if (strlen(pTmpFullDir) >= unSize)		
		{				
			break;	
		}										
		iRet = RET_SUCCESS;
		strncpy_s(sPath, unSize, pTmpFullDir, strlen(pTmpFullDir) + 1);		
		break;																									
	}
	fclose(fp);
	return iRet;
}

#endif
//lint -e438
void eSDKTool::GetSrandNum(std::string& sRandNum)
{
#ifdef WIN32
	DWORD dSrandNum = 0;
	void *lpGoop =&dSrandNum;
	DWORD cbGoop = sizeof(dSrandNum);
	HCRYPTPROV hCryptProv = NULL ;
	CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);  
	if (!CryptGenRandom(hCryptProv, cbGoop, reinterpret_cast <LPBYTE>(lpGoop))  )
	{
		/* Handle error */
		CryptReleaseContext(hCryptProv,0);   
		return ;
	} 
	else 
	{
		char* tempStr = new char[256];
		memset_s(tempStr, 256, 0, 256*sizeof(char));
		_ultoa_s(dSrandNum, tempStr, 255, 10);
		sRandNum = std::string(tempStr);
		delete[] tempStr;
		tempStr = NULL;
		CryptReleaseContext(hCryptProv, 0);   
		return ;
	}
#else
	unsigned long dSrandNum = 0;
	int fd = 0;
	fd = open("/dev/random", O_RDONLY);
	if(fd > 0)
	{
		read(fd, &dSrandNum, sizeof(unsigned long));
		close(fd);
		char* tempStr = new char[256];
		memset_s(tempStr, 256, 0, 256*sizeof(char));
		sprintf_s(tempStr, 256, "%ld", dSrandNum);
		sRandNum = std::string(tempStr);
		delete[] tempStr;
		tempStr = NULL;
		return ;
	}
	else
	{
		close(fd);
		return ;
	}
#endif
}
//lint +e438

#if defined(ANDROID) || defined(TARGET_MAC_OS) || defined(TARGET_OS_IPHONE)
/* Compress gzip data */
/* data 原数据 ndata 原数据长度 zdata 压缩后数据 nzdata 压缩后长度 */
int eSDKTool::gzcompress(Bytef *data, uLong ndata, Bytef *zdata, uLong *nzdata)
{
	z_stream c_stream;
	int err = 0;

	if(data && ndata > 0)
	{
		c_stream.zalloc = NULL;
		c_stream.zfree = NULL;
		c_stream.opaque = NULL;

		//只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer
		if(deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
			MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
		{
			return -1;
		}

		c_stream.next_in  = data;
		c_stream.avail_in  = ndata;
		c_stream.next_out = zdata;
		c_stream.avail_out  = *nzdata;

		while(c_stream.avail_in != 0 && c_stream.total_out < *nzdata)
		{
			if(deflate(&c_stream, Z_NO_FLUSH) != Z_OK)
			{
				return -1;
			}
		}
		if(c_stream.avail_in != 0) return c_stream.avail_in;
		for(;;)
		{
			if((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END)
			{
				break;
			}
			if(err != Z_OK)
			{
				return -1;
			}
		}
		if(deflateEnd(&c_stream) != Z_OK)
		{
			return -1;
		}
		*nzdata = c_stream.total_out;

		return 0;
	}
	return -1;
}
#endif

// 判断是否为目录
bool eSDKTool::IsDir(const std::string& path)
{
	bool bRet = false;
#ifdef WIN32
	if (FILE_ATTRIBUTE_DIRECTORY == PathIsDirectory(path.c_str()))
	{
		bRet = true;
	}
#else
	struct stat sb;

	if (stat(path.c_str(), &sb) == -1)
	{
		return bRet;
	}

	if (S_IFDIR == (sb.st_mode & S_IFMT))
	{
		bRet = true;
	}
#endif

	return bRet;
}

// std::string to Octal(int)
unsigned int eSDKTool::StringToOCT(const std::string& strNumber)
{
	unsigned int nRet = 0;

	for (unsigned int i=0; i<strNumber.length(); ++i)
	{
		if (strNumber[i] < '0' || strNumber[i] > '9')
		{
			return 0;
		}

		nRet = 8 * nRet + (strNumber[i] - '0');
	}

	return nRet;
}
// std::string to Octal(int)
