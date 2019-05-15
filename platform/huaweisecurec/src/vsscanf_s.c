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
#include "securecutil.h"
#include <string.h>
#if !defined(SECUREC_SYSAPI4VXWORKS)
#include <ctype.h>
#endif

/*******************************************************************************
 * <NAME>
 *    vsscanf_s
 *
 * <SYNOPSIS>
 *    int vsscanf_s(const char* buffer, const char* format, va_list arglist)
 *
 * <FUNCTION DESCRIPTION>
 *    The vsscanf_s function reads data from buffer into the location given by
 *    each argument. Every argument must be a pointer to a variable with a type
 *    that corresponds to a type specifier in format. The format argument controls
 *    the interpretation of the input fields and has the same form and function
 *    as the format argument for the scanf function.
 *    If copying takes place between strings that overlap, the behavior is undefined.
 *
 * <INPUT PARAMETERS>
 *    buffer                Stored data
 *    format                Format control string, see Format Specifications.
 *    arglist               pointer to list of arguments
 *
 * <OUTPUT PARAMETERS>
 *    arglist               the converted value stored in user assigned address
 *
 * <RETURN VALUE>
 *    Each of these functions returns the number of fields successfully converted
 *    and assigned; the return value does not include fields that were read but
 *    not assigned. A return value of 0 indicates that no fields were assigned.
 *    The return value is SCANF_EINVAL(-1) for an error or if the end of the
 *    string is reached before the first conversion.
 *******************************************************************************
*/

int vsscanf_s(const char* buffer, const char* format, va_list arglist)
{
    SEC_FILE_STREAM fStr = INIT_SEC_FILE_STREAM;
    int retval = -1;
    size_t count = 0;

    /* validation section */
    if (buffer == NULL || format == NULL)
    {
        SECUREC_ERROR_INVALID_PARAMTER("vsscanf_s");
        return SCANF_EINVAL;
    }
    count = strlen(buffer);
    if (count == 0 || count > SECUREC_STRING_MAX_LEN)
    {
        clearDestBuf(buffer,format, arglist);
        SECUREC_ERROR_INVALID_PARAMTER("vsscanf_s");
        return SCANF_EINVAL;
    }
#ifdef SECUREC_VXWORKS_PLATFORM
    /*
       in vxworks platform when buffer is white string ,will set first %s argument tu zero.like following useage:
       sscanf("   \v\f\t\r\n","%s",str,strSize);
    */
    if(isspace((int)buffer[0]) && isspace((int)buffer[count - 1])) /* do not check all character ,just first and last character then consider it is white string */
    {
        clearDestBuf(buffer,format, arglist);
    }
#endif
    fStr._flag = MEM_STR_FLAG;
    fStr._base = fStr._ptr =  (char*) buffer;
    fStr._cnt =  (int)count;
    /* coverity[var_deref_model] */
    retval = securec_input_s(&fStr, format, arglist);
    if (retval < 0 )
    {
        SECUREC_ERROR_INVALID_PARAMTER("vsscanf_s");
        return SCANF_EINVAL;
    }

    return retval;
}


