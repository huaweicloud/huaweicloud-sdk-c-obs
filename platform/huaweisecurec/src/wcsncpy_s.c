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
 *    wcsncpy_s
 *
 * <SYNOPSIS>
 *    errno_t wcsncpy_s(wchar_t* strDest, size_t destMax, const wchar_t* strSrc, size_t count);
 *
 * <FUNCTION DESCRIPTION>
 *    wcsncpy_s is the wide-character version of strncpy_s. the wcsncpy_s function
 *    copy the first D characters of strSrc to strDest, where D is the lesser of
 *    count and the length of strSrc.
 *
 * <INPUT PARAMETERS>
 *    strDest              Destination string.
 *    destMax              The size of the destination string, in characters.
 *    strSrc               Source string.
 *    count                Number of characters to be copied.
 *
 * <OUTPUT PARAMETERS>
 *    strDest              is updated
 *
 * <RETURN VALUE>
 *    EOK(0)               success
 *    EINVAL               strDest == NULL or strSrc == NULL
 *    ERANGE               destMax is zero or greater than SECUREC_WCHAR_STRING_MAX_LEN,
 *                         or count > SECUREC_WCHAR_STRING_MAX_LEN, or destMax
 *                         is too small
 *    EOVERLAP_AND_RESET   dest buffer and source buffer are overlapped
 *
 *    If there is a runtime-constraint violation, then if strDest is not a null
 *    pointer and destMax is greater than zero and not greater than
 *    SECUREC_WCHAR_STRING_MAX_LEN, then wcsncpy_s sets strDest[0] to the null
 *    character.
 *******************************************************************************
*/

errno_t wcsncpy_s(wchar_t* strDest,    size_t destMax, const wchar_t* strSrc, size_t count)
{
    wchar_t*  pHeader = strDest;
    size_t maxSize = destMax;
    IN_REGISTER const wchar_t* overlapGuard = NULL;

    if ( destMax == 0 || destMax > SECUREC_WCHAR_STRING_MAX_LEN )
    {
        SECUREC_ERROR_INVALID_RANGE("wcsncpy_s");
        return ERANGE;
    }

    if (strDest == NULL || strSrc == NULL )
    {
        SECUREC_ERROR_INVALID_PARAMTER("wcsncpy_s");
        if (strDest != NULL)
        {
            pHeader[0] = '\0';
            return EINVAL_AND_RESET;
        }
        return EINVAL;
    }
#ifdef  COMPATIBLE_WIN_FORMAT
    if (count > SECUREC_WCHAR_STRING_MAX_LEN && count !=(size_t)-1)
#else
    if (count > SECUREC_WCHAR_STRING_MAX_LEN)
#endif    
    {
        pHeader[0] = '\0'; /*clear dest string*/
        SECUREC_ERROR_INVALID_RANGE("wcsncpy_s");
        return ERANGE_AND_RESET;
    }

    if (count == 0)
    {
        pHeader[0] = '\0';
        return EOK;
    }

    if (strDest < strSrc)
    {
        overlapGuard = strSrc;
        while ((*(strDest++) = *(strSrc++)) != '\0' && --maxSize > 0 && --count > 0)
        {
            if ( strDest == overlapGuard)
            {
                pHeader[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("wcsncpy_s");
                return EOVERLAP_AND_RESET;
            }
        }
    }
    else
    {
        overlapGuard = strDest;
        while ((*(strDest++) = *(strSrc++)) != '\0' && --maxSize > 0 && --count > 0)
        {
            if ( strSrc == overlapGuard)
            {
                pHeader[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("wcsncpy_s");
                return EOVERLAP_AND_RESET;
            }
        }
    }
    if (count == 0)
    {
        *strDest = '\0';
    }

    if (maxSize == 0)
    {
        pHeader[0] = '\0';
        SECUREC_ERROR_INVALID_RANGE("wcsncpy_s");
        return ERANGE_AND_RESET;
    }

    return EOK;
}


