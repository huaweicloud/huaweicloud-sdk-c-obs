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

/*******************************************************************************
 * <NAME>
 *    vfscanf_s
 *
 * <SYNOPSIS>
 *    int vfscanf_s (FILE* stream, const char* format, va_list arglist)
 *
 * <FUNCTION DESCRIPTION>
 *    The vfscanf_s function reads data from the current position of stream into
 *    the locations given by argument (if any). Each argument must be a pointer
 *    to a variable of a type that corresponds to a type specifier in format.
 *    format controls the interpretation of the input fields and has the same
 *    form and function as the format argument for scanf.
 *
 * <INPUT PARAMETERS>
 *    stream               Pointer to FILE structure.
 *    format               Format control string, see Format Specifications.
 *    arglist              pointer to list of arguments
 *
 * <OUTPUT PARAMETERS>
 *    arglist              the converted value stored in user assigned address
 *
 * <RETURN VALUE>
 *    Each of these functions returns the number of fields successfully converted
 *    and assigned; the return value does not include fields that were read but
 *    not assigned. A return value of 0 indicates that no fields were assigned.
 *    If an error occurs, or if the end of the file stream is reached before the
 *    first conversion, the return value is SCANF_EINVAL(-1).
 *******************************************************************************
*/

int vfscanf_s(FILE* stream, const char* format, va_list arglist)
{
    int retval = 0;
    SEC_FILE_STREAM fStr = INIT_SEC_FILE_STREAM;

    if ((stream == NULL) || (format == NULL) )
    {
        SECUREC_ERROR_INVALID_PARAMTER("vfscanf_s");
        return SCANF_EINVAL;
    }

    SECUREC_LOCK_FILE(stream);
    _VALIDATE_STREAM_ANSI_SETRET(stream, EINVAL, retval, EOF);
    fStr.pf = stream;
    fStr._flag = FILE_STREAM_FLAG;
    fStr.oriFilePos = UNINITIALIZED_FILE_POS;

    retval = securec_input_s(&fStr, format,  arglist);

    SECUREC_UNLOCK_FILE(stream);
    if (retval < 0)
    {
        /*MaskCoverityID31811*/ /*coverity[RESOURCE_LEAK]*/
        SECUREC_ERROR_INVALID_PARAMTER("vfscanf_s");
        /* coverity[leaked_storage] */
        return SCANF_EINVAL;
    }

    return retval;
}


