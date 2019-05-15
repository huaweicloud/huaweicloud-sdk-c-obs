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

/*******************************************************************************
 * <NAME>
 *    vswscanf_s
 *
 * <SYNOPSIS>
 *    int vswscanf_s(const wchar_t* buffer, const wchar_t* format, va_list arglist)
 *
 * <FUNCTION DESCRIPTION>
 *    The vsscanf_s function reads data from buffer into the location given by
 *    each argument. Every argument must be a pointer to a variable with a type
 *    that corresponds to a type specifier in format.
 *    The format argument controls the interpretation of the input fields and
 *    has the same form and function as the format argument for the scanf function.
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

static size_t mywcslen(const wchar_t *wcs)
{
    const wchar_t* eos = wcs;

    while( *eos++ ) ;

    return( (size_t)(eos - wcs - 1) );
}

int vswscanf_s(const wchar_t* buffer, const wchar_t* format, va_list arglist)
{
    SEC_FILE_STREAM fStr = INIT_SEC_FILE_STREAM;
    int retval = -1;
    size_t count;

    /* validation section */
    if (buffer == NULL || format == NULL)
    {
        SECUREC_ERROR_INVALID_PARAMTER("vswscanf_s");
        return SCANF_EINVAL;
    }
    count = mywcslen(buffer);
    if (count == 0 || count > SECUREC_WCHAR_STRING_MAX_LEN )
    {
        clearwDestBuf(buffer,format, arglist);
        SECUREC_ERROR_INVALID_PARAMTER("vswscanf_s");
        return SCANF_EINVAL;
    }

    fStr._flag = MEM_STR_FLAG;
    fStr._base = fStr._ptr = (char*) buffer;
    fStr._cnt =  (int)count * ((int)WCHAR_SIZE);
    /* coverity[var_deref_model] */
    retval = securec_winput_s(&fStr, format, arglist);
    if (retval < 0 )
    {
        SECUREC_ERROR_INVALID_PARAMTER("vswscanf_s");
        return SCANF_EINVAL;
    }
    return retval;
}


