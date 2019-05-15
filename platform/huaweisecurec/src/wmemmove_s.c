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
 *    wmemmove_s
 *
 * <SYNOPSIS>
 *    errno_t wmemmove_s(wchar_t* dest, size_t destMax, const wchar_t* src, size_t count);
 *
 * <FUNCTION DESCRIPTION>
 *    wmemmove_s copies count wide character(two bytes) from src to dest
 *
 * <INPUT PARAMETERS>
 *    dest                     destination object.
 *    destMax                  Size of the destination buffer.
 *    src                      Source object.
 *    count                    Number of bytes or character to copy.
 *
 * <OUTPUT PARAMETERS>
 *    dest                  is updated.
 *
 * <RETURN VALUE>
 *     EOK                     success
 *     EINVAL                  dest == NULL or src == NULL
 *     ERANGE                  count > destMax or destMax > SECUREC_WCHAR_MEM_MAX_LEN
 *                             or destMax == 0
 *
 *     If an error occured, dest will NOT be filled with 0.
 *     If some regions of the source area and the destination overlap, wmemmove_s
 *     ensures that the original source bytes in the overlapping region are copied
 *     before being overwritten
 ********************************************************************************
*/

errno_t wmemmove_s(wchar_t* dest, size_t destMax, const wchar_t* src, size_t count)
{
    if (destMax == 0 || destMax > SECUREC_WCHAR_MEM_MAX_LEN )
    {
        SECUREC_ERROR_INVALID_PARAMTER("wmemmove_s");
        return ERANGE;
    }
    if ( count > destMax)
    {
        SECUREC_ERROR_INVALID_PARAMTER("wmemmove_s");
        if (dest != NULL)
        {
            (void)memset(dest, 0, destMax * WCHAR_SIZE);
            return ERANGE_AND_RESET;
        }
        return ERANGE;
    }
    return memmove_s(dest, destMax * WCHAR_SIZE, src, count * WCHAR_SIZE);
}


