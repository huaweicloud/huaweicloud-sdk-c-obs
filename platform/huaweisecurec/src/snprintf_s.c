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
#include <stdarg.h>

/*******************************************************************************
 * <NAME>
 *    snprintf_s
 *
 * <SYNOPSIS>
 *    int snprintf_s(char* strDest, size_t destMax, size_t count, const char* format, ...);
 *
 * <FUNCTION DESCRIPTION>
 *    The snprintf_s function formats and stores count or fewer characters in 
 *    strDest and appends a terminating null. Each argument (if any) is converted
 *    and output according to the corresponding format specification in format.
 *    The formatting is consistent with the printf family of functions; If copying
 *    occurs between strings that overlap, the behavior is undefined.
 *
 * <INPUT PARAMETERS>
 *    strDest                 Storage location for the output.
 *    destMax                 The size of the storage location for output. Size 
 *                            in bytes for snprintf_s or size in words for snwprintf_s.
 *    count                   Maximum number of character to store.
 *    format                  Format-control string.
 *
 * <OUTPUT PARAMETERS>
 *    strDest                 is updated
 *
 * <RETURN VALUE>
 *    snprintf_s returns the number of characters stored in strDest, not counting
 *    the terminating null character.
 *    If the storage required to store the data and a terminating null exceeds 
 *    destMax, the function set strDest to an empty string, and return -1.
 *    If strDest or format is a NULL pointer, or if count is less than or equal
 *    to zero, the function return -1.
 *******************************************************************************
*/

int snprintf_s (char* strDest, size_t destMax, size_t count, const char* format, ...)
{
    int ret = 0;
    va_list arglist;

    va_start(arglist, format);
    ret = vsnprintf_s(strDest, destMax, count, format, arglist);
    va_end(arglist);

    return ret;
}


