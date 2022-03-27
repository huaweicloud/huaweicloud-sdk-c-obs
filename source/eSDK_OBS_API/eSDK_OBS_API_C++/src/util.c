/** **************************************************************************
 *
 * Copyright 2008 Bryan Ischo <bryan@ischo.com>
 *
 * This file is part of libs3.
 *
 * libs3 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, version 3 of the License.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of this library and its programs with the
 * OpenSSL library, and distribute linked combinations including the two.
 *
 * libs3 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with libs3, in a file named COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 ************************************************************************** **/

/*lint -e506*/ 
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include <ctype.h>
#include <string.h>
#include "util.h"
#include "securec.h"
#include "log.h"
#include "pcre.h"

#ifdef WIN32
#include <windows.h>
#else
#include <locale.h>
#include <iconv.h>
#endif


#define OVECCOUNT 100

static int checkString(const char *str, const char *format) 
{
	if (format == NULL || str == NULL) {
        COMMLOG(OBS_LOGERROR, "input of checkString is NULL.");
		return 0;
	}
	while (*format) {
		char s = *str;
		char f = *format;
		if (!s) {
			return 0;
		}
		if (f == 'd') {
			if (!isdigit(s)) {
				return 0;
			}
		}
		else if (s != f) {
			return 0;
		}
		++str;
		++format;
	}
	return 1;
}

#ifdef WIN32
char* string_To_UTF8(const char* strSource)
{
    int nLen = 0;
    char* pBuf = NULL;

    int nwLen = MultiByteToWideChar(CP_ACP, 0, strSource, -1, NULL, 0);

    wchar_t * pwBuf = (wchar_t *)malloc(sizeof(wchar_t) * (nwLen+1));
    if (NULL == pwBuf)
    {
        COMMLOG(OBS_LOGERROR, "Malloc pwBuf failed!");
        return NULL;
    }
    memset_s(pwBuf, sizeof(wchar_t) * (nwLen+1), 0,  sizeof(wchar_t) * (nwLen+1));

    MultiByteToWideChar(CP_ACP, 0, strSource, -1, pwBuf, nwLen);
    nLen = WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, 0, NULL, NULL);

    pBuf = (char *)malloc(sizeof(char) * (nLen + 1));
    if (NULL == pBuf)
    {
        COMMLOG(OBS_LOGERROR, "Malloc pBuf failed!");
        free(pwBuf);     
        pwBuf=NULL;
        return NULL;
    }
    memset_s(pBuf, sizeof(char) * (nLen + 1), 0, nLen + 1);

    WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, pBuf, nLen, NULL, NULL);
    if (pwBuf)
    {
        free(pwBuf);
        pwBuf = NULL;
    }

    return pBuf;
}

char* UTF8_To_String(const char* strSource)
{
    char *str = NULL;
    int nLen = MultiByteToWideChar(CP_UTF8, 0, strSource, -1, NULL, 0);

    wchar_t* wstr = (wchar_t*)malloc(sizeof(wchar_t) * (nLen + 1));
    if (NULL == wstr)
    {
        COMMLOG(OBS_LOGERROR, "Malloc wstr failed!");
        return NULL;
    }
    memset_s(wstr, sizeof(wchar_t) * (nLen + 1), 0, nLen * 2 + 2);
    MultiByteToWideChar(CP_UTF8, 0, strSource, -1, wstr, nLen);
    nLen = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);

    str = (char*)malloc(sizeof(char) * (nLen + 1));
    if (NULL == str)
    {
        COMMLOG(OBS_LOGERROR, "Malloc str failed!");
        free(wstr);        
        wstr=NULL;
        return NULL;
    }
    memset_s(str, sizeof(char) * (nLen + 1), 0, nLen + 1);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, nLen, NULL, NULL);

    if(wstr) 
    {
        free(wstr);
        wstr = NULL;
    }

    return str;
}
#else
char* string_To_UTF8(const char* pSource)
{
    size_t i_length = strlen(pSource);
    size_t o_length = (i_length + 1) * 2;
    char* pmbs = (char*)malloc(o_length);
	
	if (pmbs == NULL)
	{
		COMMLOG(OBS_LOGERROR, "string_To_UTF8: Malloc str failed!");
		return NULL;
	}
	
    memset_s(pmbs, o_length, 0, o_length);
    char* result = pmbs; 
    size_t retsize;
    iconv_t cd;
    cd = iconv_open("UTF-8", "GBK");
    if((iconv_t)-1 == cd) { 
        perror("iconv_open error"); 
        free(pmbs);
        pmbs = NULL;
        iconv_close(cd);
        return NULL; 
    }

    retsize = iconv(cd, (char**)&pSource, &i_length, (char**)&pmbs, &o_length);
    if((size_t)-1 == retsize) {
        perror("iconv error"); 
        free(pmbs);
        pmbs = NULL;
        iconv_close(cd);
        return NULL;    
    }

    iconv_close(cd);
    return result;
}

char* UTF8_To_String(const char* pSource)
{
    size_t i_length = strlen(pSource);
    size_t o_length = (i_length + 1) * 2;
    char* pmbs = (char*)malloc(o_length);
	
	if (pmbs == NULL)
	{
		COMMLOG(OBS_LOGERROR, "UTF8_To_String: Malloc str failed!");
		return NULL;
	}
	
    memset_s(pmbs, o_length, 0, o_length);
    char* result = pmbs;  
    size_t retsize;
    iconv_t cd;
    cd = iconv_open("GBK", "UTF-8");
    if((iconv_t)-1 == cd) { 
        perror("iconv_open error"); 
        free(pmbs);
        pmbs = NULL;
        iconv_close(cd);
        return NULL; 
    }

    retsize = iconv(cd, (char**)&pSource, &i_length, (char**)&pmbs, &o_length);
    if((size_t)-1 == retsize) {
        perror("iconv error"); 
        free(pmbs);
        pmbs = NULL;
        iconv_close(cd);
        return NULL;    
    }

    iconv_close(cd);
    return result;
}
#endif

void changeTimeFormat(const char* strInTime, char* strOutTime)
{
    int ret = 0;
    if (!checkString(strInTime, "ddddddddT")) {
        ret = sprintf_s(strOutTime,ARRAY_LENGTH_50, "%s", strInTime);
        CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);
        return;
    }

    struct tm tmTool;
    ret = sscanf_s(strInTime, "%4d%2d%2dT", &tmTool.tm_year, &tmTool.tm_mon, &tmTool.tm_mday);
    if (ret != 3) {
        COMMLOG(OBS_LOGWARN, "sscanf_s failed in %s.(%d)", __FUNCTION__, __LINE__);
    }
    ret = sprintf_s(strOutTime,ARRAY_LENGTH_50, "%04d-%02d-%02dT00:00:00Z", tmTool.tm_year, tmTool.tm_mon, tmTool.tm_mday);
    CheckAndLogNeg(ret, "sprintf_s", __FUNCTION__, __LINE__);

    return;
}

int getTimeZone()
{
#ifdef WIN32
    SYSTEMTIME localTime;
    GetLocalTime(&localTime);

    FILETIME localFileTime;
    SystemTimeToFileTime(&localTime, &localFileTime);

    FILETIME gmFileTime;
    LocalFileTimeToFileTime(&localFileTime, &gmFileTime);

    SYSTEMTIME gmTime = {0};
    FileTimeToSystemTime(&gmFileTime, &gmTime);

    int time_zone = localTime.wHour - gmTime.wHour;
#else
    time_t time_utc;  
    struct tm tm_local;  

    time(&time_utc);  
    localtime_r(&time_utc, &tm_local);  

    struct tm tm_gmt;  

    gmtime_r(&time_utc, &tm_gmt); 

    int time_zone = tm_local.tm_hour - tm_gmt.tm_hour;  
#endif

    if (time_zone < -12) {  
        time_zone += 24;   
    } else if (time_zone > 12) {  
        time_zone -= 24;  
    }

    return time_zone;
}

int urlEncode(char *dest, const char *src, int maxSrcSize, char ignoreChar)
{
    if (dest == NULL) {
        COMMLOG(OBS_LOGERROR, "dest for urlEncode is NULL.");
        return -1;
    }
    if (src == NULL) {
        COMMLOG(OBS_LOGWARN, "src for urlEncode is NULL.");
        *dest = 0;
        return 1;
    }
    int len = 0;
    while (*src) {
        if (++len > maxSrcSize) {
            *dest = 0;
            return 0;
        }
        unsigned char c = *src;
        if (isalnum(c) || (c == '.') || (c == '-')
            || (c == '_') || (c == '~')
            || (c == ignoreChar))
        {
            *dest++ = c;
        }
        else {
            *dest++ = '%';
            *dest++ = "0123456789ABCDEF"[c >> 4];
            *dest++ = "0123456789ABCDEF"[c & 15];
        }
        ++src;
    }

    *dest = 0;
    return 1;
}

int urlDecode(char *dest, const char *src, int maxSrcSize)
{
    int len = 0;
    char strOne[4] = {0};
    int charGot = 0;

    if (src) while (*src) {
        if (++len > maxSrcSize) {
            *dest = 0;
            return 0;
        }
        unsigned char c = *src;
        if(c=='%'){
            src ++;
            errno_t err = memmove_s(strOne,ARRAY_LENGTH_4,src,2);
            if (err != EOK)
            {
                COMMLOG(OBS_LOGWARN, "%s(%d): memmove_s failed!(%d)", __FUNCTION__, __LINE__, err);
            }
            int ret = sscanf_s(strOne, "%02x",&charGot);
            if (ret != 1) {
                COMMLOG(OBS_LOGWARN, "%s(%d): sscanf_s failed!(%d)", __FUNCTION__, __LINE__);
            }
            if (ret = memset_s(strOne, ARRAY_LENGTH_4, 0, 4))
            {
                COMMLOG(OBS_LOGERROR, "in %s line %s memset_s error, code is %d.", __FUNCTION__, __LINE__, ret);
                return OBS_STATUS_InternalError;
            }
            src ++;

            *dest++ = (char)charGot;            
        }
        else
        {
            *dest++ = c;
        }
        src++;
    }

    *dest = 0;

    return 1;
}

int64_t parseIso8601Time(const char *str)
{
	if (!checkString(str, "dddd-dd-ddTdd:dd:dd")) {
		return -1;
	}

#define getnum() (*str - '0')

	struct tm stm;
	memset_s(&stm, sizeof(stm), 0, sizeof(stm));
	stm.tm_year = getnum() * 1000;
	++str;
	stm.tm_year += getnum() * 100;
	++str;
	stm.tm_year += getnum() * 10;
	++str;
	stm.tm_year += getnum() - 1900;
	str += 2;
	
	stm.tm_mon = getnum() * 10;
	++str;
	stm.tm_mon += getnum() - 1;
	str += 2;

	stm.tm_mday = getnum() * 10;
	++str;
	stm.tm_mday += getnum();
	str += 2;

	stm.tm_hour = getnum() * 10;
	++str;
	stm.tm_hour += getnum();
	str += 2;

	stm.tm_min = getnum() * 10;
	++str;
	stm.tm_min += getnum();
	str += 2;

	stm.tm_sec = getnum() * 10;
	++str;
	stm.tm_sec += getnum();
	++str;

	stm.tm_isdst = -1;
	int64_t ret = mktime(&stm);

	if (*str == '.') {
		str++;
		while (isdigit(*str)) {
			str++;
		}
	}

	if (checkString(str, "-dd:dd") || checkString(str, "+dd:dd")) {
		int flag = 1;
		if (*str == '+') {
			flag = -1;
		}
		++str;

		int hours = getnum() * 10;
		++str;
		hours += getnum();
		str += 2;

		int minutes = getnum() * 10;
		++str;
		minutes += getnum();
		ret += (flag * (((hours * 60) + minutes) * 60));
	}

	return ret;
}

uint64_t parseUnsignedInt(const char *str)
{
	uint64_t ret = 0;

	while (is_blank(*str)) {
		++str;
	}

	while (isdigit(*str)) {
		ret *= 10;
		ret += (*str - '0');
		++str;
	}
	return ret;
}


int base64Encode(const unsigned char *in, int inLen, char *out)
{
	BIO * bmem = NULL;
	BIO * b64 = NULL;
	BUF_MEM * bptr = NULL;

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, in, inLen);
	(void)BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

    errno_t err = EOK;
    err = memcpy_s(out, bptr->length, bptr->data, bptr->length);
    if (err != EOK)
    {
        COMMLOG(OBS_LOGWARN, "%s(%d): memcpy_s failed!", __FUNCTION__, __LINE__);
    }
    out[bptr->length] = 0;

    size_t result = bptr->length;
    BIO_free_all(b64);
    return result;
}

char * base64Decode(const char *base64Char, const long base64CharSize, char *originChar, long originCharSize) 
{

	BIO * b64 = NULL;
	BIO * bmem = NULL;

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_new_mem_buf(base64Char, base64CharSize);
	bmem = BIO_push(b64, bmem);
	BIO_read(bmem, originChar, originCharSize);

	BIO_free_all(bmem);

	return originChar;
}


void HMAC_SHA1(unsigned char hmac[20], const unsigned char *key, int key_len,
               const unsigned char *message, int message_len)
{
	unsigned int resultLen = 20;
	const EVP_MD* engine = NULL;
	engine = EVP_sha1();
	

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	HMAC_CTX ctx;
	HMAC_CTX_init(&ctx);

	HMAC_Init_ex(&ctx, key, key_len, engine, NULL);
	HMAC_Update(&ctx, message, message_len);
	HMAC_Final(&ctx, hmac, &resultLen);
	HMAC_CTX_cleanup(&ctx);
#else
	HMAC_CTX *ctx;
	ctx = HMAC_CTX_new();
	if (NULL == ctx)
	{
		COMMLOG(OBS_LOGERROR, "HMAC_CTX_new failed!");
		return;
	}
	HMAC_CTX_reset(ctx);
	HMAC_Init_ex(ctx, key, key_len, engine, NULL);
	HMAC_Update(ctx, message, message_len);
	HMAC_Final(ctx, hmac, &resultLen);
	HMAC_CTX_free(ctx);
#endif

}

void HMAC_SHA256(unsigned char hmac[32], const unsigned char *key, int key_len,
               const unsigned char *message, int message_len)
{
    const EVP_MD* engine = NULL;
    engine = EVP_sha256();

    unsigned char* temp = (unsigned char*)malloc(32);
    if (NULL == temp)
    {
        COMMLOG(OBS_LOGERROR, "Malloc temp failed!");
        return;
    }
    memset_s(temp,32, 0, 32);
    unsigned int tempLength = 0;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, key, key_len, engine, NULL);
    HMAC_Update(&ctx, message, message_len);
    HMAC_Final(&ctx, temp, &tempLength);
    HMAC_CTX_cleanup(&ctx);
#else
    HMAC_CTX *ctx;
    ctx = HMAC_CTX_new();
    if (NULL == ctx)
    {
        COMMLOG(OBS_LOGERROR, "HMAC_CTX_new failed!");
        free(temp);
        temp = NULL;
        return;
    }
    HMAC_CTX_reset(ctx);
    HMAC_Init_ex(ctx, key, key_len, engine, NULL);
    HMAC_Update(ctx, message, message_len);
    HMAC_Final(ctx, temp, &tempLength);
    HMAC_CTX_free(ctx);
#endif
    int ret = 0;
    if (ret = memset_s(hmac, ARRAY_LENGTH_32, 0, 32))
    {
        COMMLOG(OBS_LOGERROR, "in %s line %s memset_s error, code is %d.", __FUNCTION__, __LINE__, ret);
    }
    unsigned int i;
    for(i=0; i<tempLength; i++)
    {
        hmac[i] = temp[i];
    }
    free(temp);
    temp = NULL;
}

void SHA256Hash(unsigned char sha[32], const unsigned char *message, int message_len)
{
    const EVP_MD *md = NULL;
    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname("sha256");
    if(NULL == md)
    {
        return ;
    }
    unsigned char *temp = (unsigned char*)malloc(32);
    if (NULL == temp)
    {
        COMMLOG(OBS_LOGERROR, "Malloc temp failed!");
        return;
    }
    memset_s(temp, 32, 0, 32);
    unsigned int tempLength = 0;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX mdctx;
    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, md, NULL);
    EVP_DigestUpdate(&mdctx, message, message_len);
    EVP_DigestFinal_ex(&mdctx, temp, &tempLength);
    EVP_MD_CTX_cleanup(&mdctx);
#else
    EVP_MD_CTX *mdctx;
    mdctx = EVP_MD_CTX_new();
    if (NULL == mdctx)
    {
        COMMLOG(OBS_LOGERROR, "EVP_MD_CTX_new failed!");
        free(temp);
        temp = NULL;
        return;
    }
    EVP_MD_CTX_init(mdctx);
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, message, message_len);
    EVP_DigestFinal_ex(mdctx, temp, &tempLength);
    EVP_MD_CTX_free(mdctx);
#endif
    int ret = 0;
    if (ret = memset_s(sha, ARRAY_LENGTH_32, 0, 32))
    {
        COMMLOG(OBS_LOGERROR, "in %s line %s memset_s error, code is %d.", __FUNCTION__, __LINE__, ret);
    }
    unsigned int i = 0;
    for(i = 0; i < tempLength; i++)
    {
        sha[i] = temp[i];
    }
    free(temp);
    temp = NULL;
}

int is_blank(char c)
{
    return ((c == ' ') || (c == '\t'));
}

void uchar_to_hexes(unsigned char ucIn, unsigned char* szOut)
{
    static const unsigned char Number[] = 
    {
        '0', '1', '2', '3', '4', '5', '6', '7', 
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'  
    };

    szOut[0] = Number[(ucIn & 0xF0) >> 4];
    szOut[1] = Number[(ucIn & 0x0F)];
}

void ustr_to_hexes(unsigned char* szIn,unsigned int inlen, unsigned char* szOut)
{
    unsigned int i;
    for (i = 0; i < inlen; ++i)
    {
        uchar_to_hexes(szIn[i], szOut + 2 * i);
    }
}

int pcre_replace_start(char *dest, int offset, int count,
	const char *src, int src_len, int *ovector)
{
	for (int i = 0; i<count; i++)
	{
		int retVal = 0;
		if (i == 0)
		{
			int ret = strncpy_s(dest + offset, src_len + count * 6 - offset, src, ovector[i * 2]);
			CheckAndLogNoneZero(ret, "strncpy_s", __FUNCTION__, __LINE__);
			offset = ovector[i * 2];
		}
		else
		{
			int ret = strncpy_s(dest + offset, src_len + count * 6 - offset, 
				src + ovector[i * 2 - 1], ovector[i * 2] - ovector[i * 2 - 1]);
			CheckAndLogNoneZero(ret, "strncpy_s", __FUNCTION__, __LINE__);
			offset += ovector[i * 2] - ovector[i * 2 - 1];
		}
		if (src[ovector[i * 2]] == '<')
		{
			retVal = strcat_s(dest, sizeof(char)*(src_len + count * 6), "&lt;");
			if (retVal != 0)
			{
				COMMLOG(OBS_LOGWARN, "strcat_s failed in %s.(%d)", __FUNCTION__, __LINE__);
			}
			offset += 4;
		}
		if (src[ovector[i*2]] == '>')
		{
			retVal = strcat_s(dest, sizeof(char)*(src_len + count * 6), "&gt;");
			if (retVal != 0)
			{
				COMMLOG(OBS_LOGWARN, "strcat_s failed in %s.(%d)", __FUNCTION__, __LINE__);
			}
			offset += 4;
		}
		if (src[ovector[i*2]] == '&')
		{
			retVal = strcat_s(dest, sizeof(char)*(src_len + count * 6), "&amp;");
			if (retVal != 0)
			{
				COMMLOG(OBS_LOGWARN, "strcat_s failed in %s.(%d)", __FUNCTION__, __LINE__);
			}
			offset += 5;
		}
		if (src[ovector[i*2]] == '\'')
		{
			retVal = strcat_s(dest, sizeof(char)*(src_len + count * 6), "&apos;");
			if (retVal != 0)
			{
				COMMLOG(OBS_LOGWARN, "strcat_s failed in %s.(%d)", __FUNCTION__, __LINE__);
			}
			offset += 6;
		}
		if (src[ovector[i * 2]] == '\"')
		{
			retVal = strcat_s(dest, sizeof(char)*(src_len + count * 6), "&quot;");
			if (retVal != 0) {
				COMMLOG(OBS_LOGWARN, "strcat_s failed in %s.(%d)", __FUNCTION__, __LINE__);
			}
			offset += 6;
		}
	}
	return count;
}

int pcre_replace(const char* src,char ** destOut)
{
    pcre  *re = NULL;
    const char *error = NULL;
    int src_len = 0;
    int  erroffset = 0;
    int  ovector[OVECCOUNT]={0};
    if (src == NULL) {
        COMMLOG(OBS_LOGERROR, "src for pcre_replace is NULL.");
        return 0;
    }
    src_len = strlen(src);
    re = pcre_compile("[&\'\"<>]", 0, &error, &erroffset, NULL);
    if (re == NULL)
    {
        return 0;
    }
    int count = 0;
    int offset = 0;
    while( pcre_exec(re,NULL,src,src_len,offset,0,&ovector[count*2],OVECCOUNT) > 0)
    {
        offset=ovector[count*2 + 1];
        count++;
    }
    CHECK_NULL_FREE(re);
    if(count == 0)
    {
        return 0;
    }

    char* dest = (char *)malloc(sizeof(char)*(src_len + count*6));
    if(dest==NULL)
    {
        COMMLOG(OBS_LOGERROR, "Malloc dest failed !");
        return 0;
    }
    memset_s(dest, sizeof(char)*(src_len + count*6), 0,(src_len + count*6));
    offset = 0;
	pcre_replace_start(dest, offset, count, src, src_len, ovector);
    *destOut = dest;
    return count;
}

int add_xml_element(char * buffOut, int * lenth,const char * elementName, const char * content, 
        eFormalizeChoice needFormalize, xmlAddType addType)
{
    return add_xml_element_in_bufflen(buffOut, lenth, elementName, content, needFormalize, addType, MAX_XML_LEN);
}

int add_xml_element_headOrTail(char *buffOut, int *lenth, const char *elementName, 
	xmlAddType addType, int tmplen, int buff_len)
{
	if (addType == ADD_HEAD_ONLY)
	{
		tmplen = snprintf_s(buffOut + *lenth, buff_len - *lenth,
			_TRUNCATE, "<%s>", elementName);
	}
	else
	{
		tmplen = snprintf_s(buffOut + *lenth, buff_len - *lenth,
			_TRUNCATE, "</%s>", elementName);
	}
	if (tmplen < 0)
	{
		COMMLOG(OBS_LOGERROR, "snprintf_s error xmlElementName:%s!", elementName);
		return -1;
	}
	*lenth += tmplen;
	return 0;
}

int add_xml_element_name(char *buffOut, int *lenth, const char *elementName, const char *content, 
	eFormalizeChoice needFormalize, const char *pstrContent, char *afterFormalize, 
	int mark , int is_true, int tmplen, int buff_len)
{
	is_true = (content == NULL || '\0' == content[0]);
	if (is_true)
	{
		COMMLOG(OBS_LOGERROR, "xml element content is NULL!");
		return -1;
	}
	if (needFormalize == NEED_FORMALIZE)
	{
		mark = pcre_replace(content, &afterFormalize);
		pstrContent = mark ? afterFormalize : content;
	}
	else
	{
		pstrContent = content;
	}
	tmplen = snprintf_s(buffOut + *lenth, buff_len - *lenth, _TRUNCATE,
		"<%s>%s</%s>", elementName, pstrContent, elementName);
	if (tmplen < 0) 
	{
		COMMLOG(OBS_LOGERROR, "snprintf_s error xmlElementName:%s, xmlElementContent:%s!",
			elementName, content);
		return -1;
	}
	*lenth += tmplen;
	if ((needFormalize) && (mark))
	{
		free(afterFormalize);
		afterFormalize = NULL;
	}
	return 0;
}

int add_xml_element_in_bufflen(char * buffOut, int * lenth,const char * elementName, const char * content, 
        eFormalizeChoice needFormalize, xmlAddType addType, int buff_len)
{
    int mark = 0;
    char*  afterFormalize = 0;
    const char * pstrContent = NULL;
    int tmplen = 0;   
    int is_true = 0;

    is_true = ((buffOut == NULL) || (elementName == NULL));
    if(is_true)
    {
        return -1;
    }

    is_true = ((addType == ADD_HEAD_ONLY) || (addType == ADD_TAIL_ONLY));
    if(is_true)
    {
		if (add_xml_element_headOrTail(buffOut, lenth, elementName, addType, tmplen, buff_len))
		{
			return -1;
		}
    }
    else if(addType  == ADD_NAME_CONTENT)
    {
		if (add_xml_element_name(buffOut, lenth, elementName, content, needFormalize,
			pstrContent, afterFormalize, mark, is_true, tmplen, buff_len)) 
		{
			return -1;
		}
    }
    else
    {
        COMMLOG(OBS_LOGERROR, "xml add type is invalid!");
        return -1;
    }      
    
    return 0;     
}
/*lint restore*/


