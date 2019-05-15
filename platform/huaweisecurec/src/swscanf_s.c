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
#include "secinput.h"

/*******************************************************************************
 * <NAME>
 *    swscanf_s
 *
 * <SYNOPSIS>
 *    int swscanf_s (const wchar_t* buffer, const wchar_t* format, ...)
 *
 * <FUNCTION DESCRIPTION>
 *    The swscanf_s function reads data from buffer into the location given by
 *    each argument. Every argument must be a pointer to a variable with a type
 *    that corresponds to a type specifier in format. The format argument controls
 *    the interpretation of the input fields and has the same form and function 
 *    as the format argument for the scanf function. If copying takes place between
 *    strings that overlap, the behavior is undefined.
 *
 * <INPUT PARAMETERS>
 *    buffer                 Stored data.
 *    format                 Format control string, see Format Specifications.
 *    ...                    Optional arguments.
 *
 * <OUTPUT PARAMETERS>
 *    ...                    the converted value stored in user assigned address
 *
 * <RETURN VALUE>
 *    Each of these functions returns the number of fields successfully converted
 *    and assigned; The return value does not include fields that were read but not
 *    assigned.
 *    A return value of 0 indicates that no fields were assigned.
 *    The return value is SCANF_EINVAL(-1) for an error or if the end of the string
 *    is reached before the first conversion.
 *******************************************************************************
*/

int swscanf_s(const wchar_t* buffer, const wchar_t* format, ...)
{
    int ret = 0;
    va_list arglist;

    va_start(arglist, format);
    ret = vswscanf_s(buffer, format, arglist);
    va_end(arglist);

    return ret;
}


