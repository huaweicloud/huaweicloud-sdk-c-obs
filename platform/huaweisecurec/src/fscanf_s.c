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
 *    fscanf_s
 *
 * <SYNOPSIS>
 *    int fscanf_s(FILE *stream, const char *format, ...)
 *
 * <FUNCTION DESCRIPTION>
 *    The fscanf function reads data from the current position of stream into 
 *    the locations given by argument (if any). Each argument must be a pointer 
 *    to a variable of a type that corresponds to a type specifier in format.
 *    format controls the interpretation of the input fields and has the same 
 *    form and function as the format argument for scanf; see scanf for a 
 *    description of format.
 *
 * <INPUT PARAMETERS>
 *    stream              Pointer to FILE structure.
 *    format              Format control string, see Format Specifications.
 *    ...                 Optional arguments.
 *
 * <OUTPUT PARAMETERS>
 *    ...                 The convered value stored in user assigned address
 *
 * <RETURN VALUE>
 *    Each of these functions returns the number of fields successfully converted
 *    and assigned; the return value does not include fields that were read but 
 *    not assigned. A return value of 0 indicates that no fields were assigned. 
 *    If an error occurs, or if the end of the file stream is reached before the 
 *    first conversion, the return value is EOF for fscanf and fwscanf.
 *******************************************************************************
*/

int fscanf_s(FILE* stream, const char* format, ...)
{
    int ret = 0;
    va_list arglist;

    va_start(arglist, format);
    ret = vfscanf_s(stream, format, arglist);
    va_end(arglist);

    return ret;
}


