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

#include "securec.h"
#include "securecutil.h"

/*******************************************************************************
 * <NAME>
 *    wcscpy_s
 *
 * <SYNOPSIS>
 *    errno_t wcscpy_s(wchar_t* strDest, size_t destMax, const wchar_t* strSrc);
 *
 * <FUNCTION DESCRIPTION>
 *    wcscpy_s is wide-character version of strcpy_s.

 * <INPUT PARAMETERS>
 *    strDest                   Location of destination string buffer
 *    destMax                   Size of the destination string buffer.
 *    strSrc                    Null-terminated source string buffer.
 *
 * <OUTPUT PARAMETERS>
 *    strDest                   is updated.
 *
 * <RETURN VALUE>
 *    0                         success
 *    EINVAL                    strDest == NULL or strSrc == NULL
 *    ERANGE                    destination buffer is NOT enough,  or size of 
 *                              buffer is zero or greater than SECUREC_STRING_MAX_LEN
 *    EOVERLAP_AND_RESET        dest buffer and source buffer are overlapped
 *
 *    If there is a runtime-constraint violation, then if strDest is not a null 
 *    pointer and destMax is greater than zero and not greater than 
 *    SECUREC_WCHAR_STRING_MAX_LEN, then wcscpy_s sets strDest[0] to the null
 *    character.
 *******************************************************************************
*/

errno_t wcscpy_s(wchar_t* strDest, size_t destMax, const wchar_t* strSrc)
{
    wchar_t* pHeader = strDest;
    size_t maxSize = destMax;
    IN_REGISTER const wchar_t* overlapGuard = NULL;

    if (destMax == 0 || destMax > SECUREC_WCHAR_STRING_MAX_LEN)
    {
        SECUREC_ERROR_INVALID_RANGE("wcscpy_s");
        return ERANGE;
    }
    if (strDest == NULL || strSrc == NULL)
    {
        SECUREC_ERROR_INVALID_PARAMTER("wcscpy_s");
        if (strDest != NULL)
        {
            pHeader[0] = '\0';
            return EINVAL_AND_RESET;
        }


        return EINVAL;
    }

    if (strDest == strSrc)
    {
        return EOK;
    }

    if (strDest < strSrc)
    {
        overlapGuard = strSrc;
        while ((*(strDest++) = *(strSrc++)) != '\0'  && --maxSize > 0)
        {
            if ( strDest == overlapGuard)
            {
                pHeader[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("strcpy_s");
                return EOVERLAP_AND_RESET;
            }
        }
    }
    else
    {
        overlapGuard = strDest;
        while ((*(strDest++) = *(strSrc++)) != '\0'  && --maxSize > 0)
        {
            if ( strSrc == overlapGuard)
            {
                pHeader[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("strcpy_s");
                return EOVERLAP_AND_RESET;
            }
        }
    }

    if (maxSize == 0)
    {
        pHeader[0] = '\0';
        SECUREC_ERROR_INVALID_RANGE("wcscpy_s");
        return ERANGE;
    }

    return EOK;
}


