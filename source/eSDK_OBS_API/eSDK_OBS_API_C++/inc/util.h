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

#ifndef UTIL_H
#define UTIL_H

#include <curl/curl.h>
#include <curl/multi.h>
#include <stdint.h>
#include "eSDKOBS.h"

//#define vsnprintf_sec vsnprintf_s
//#define snprintf_sec snprintf_s
//#define sprintf_sec sprintf_s
//#define strncpy_sec strncpy_s

#define ACS_URL "http://acs.amazonaws.com/groups/"

#define ACS_GROUP_ALL_USERS     ACS_URL "global/AllUsers"
#define ACS_GROUP_AWS_USERS     ACS_URL "global/AuthenticatedUsers"
#define ACS_GROUP_LOG_DELIVERY  ACS_URL "s3/LogDelivery"
#define MAX_XML_LEN (1024*100)
/* _TRUNCATE */
#define _TRUNCATE (SECUREC_STRING_MAX_LEN - 1)

#ifdef _MSC_VER
#define snprintf_s _snprintf_s
#endif

#define COMPACTED_METADATA_BUFFER_SIZE \
    (OBS_MAX_METADATA_COUNT * sizeof(OBS_METADATA_HEADER_NAME_PREFIX "n: v"))


#define MAX_URLENCODED_KEY_SIZE (3 * OBS_MAX_KEY_SIZE)


#define MAX_URI_SIZE \
    ((sizeof("https:///") - 1) + OBS_MAX_HOSTNAME_SIZE + 255 + 1 +       \
     MAX_URLENCODED_KEY_SIZE + (sizeof("?torrent") - 1) + 1)

#define MAX_CANONICALIZED_RESOURCE_SIZE \
        (1 + 255 + 1 + MAX_URLENCODED_KEY_SIZE + (sizeof("?torrent") - 1) + 1)

#define MAX_ACTUAL_HEADERS_SIZE  2048


#define CHECK_NULL_FREE(x) \
do{\
    if (NULL != x)\
    {\
        free(x);\
        x = NULL;\
    }\
}while(0)

#define obscase case

typedef enum
{
    ADD_HEAD_ONLY,
    ADD_TAIL_ONLY,
    ADD_NAME_CONTENT,
    INVALID_ADD_TYPE
}xmlAddType;

typedef enum
{
   NOT_NEED_FORMALIZE,
   NEED_FORMALIZE   
}eFormalizeChoice;



int urlEncode(char *dest, const char *src, int maxSrcSize, char ignoreChar);
    
int urlDecode(char *dest, const char *src, int maxSrcSize);

char* string_To_UTF8(const char* strSource);

char* UTF8_To_String(const char* strSource);

int getTimeZone();

void changeTimeFormat(const char* strInTime, char* strOutTime);

int64_t parseIso8601Time(const char *str);

uint64_t parseUnsignedInt(const char *str);

int base64Encode(const unsigned char *in, int inLen, char *out);

char * base64Decode(const char *base64Char, const long base64CharSize, char *originChar, long originCharSize) ;

void HMAC_SHA1(unsigned char hmac[20], const unsigned char *key, int key_len,
                const unsigned char *message, int message_len);

void HMAC_SHA256(unsigned char hmac[32], const unsigned char *key, int key_len,
                    const unsigned char *message, int message_len);

void SHA256Hash(unsigned char sha[32], const unsigned char *message, int message_len);

uint64_t hash(const unsigned char *k, int length);

int is_blank(char c);

void ustr_to_hexes(unsigned char* szIn,unsigned int inlen, unsigned char* szOut);

int pcre_replace(const char* src,char ** destOut);
int add_xml_element(char * buffOut, int * lenth,const char * elementName, const char * content, 
        eFormalizeChoice needFormalize, xmlAddType addType);
int add_xml_element_in_bufflen(char * buffOut, int * lenth,const char * elementName, const char * content, 
        eFormalizeChoice needFormalize, xmlAddType addType, int buff_len);


#endif /* UTIL_H */


