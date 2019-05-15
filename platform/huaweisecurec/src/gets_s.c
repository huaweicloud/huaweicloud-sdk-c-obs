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
#include <string.h>

/*******************************************************************************
 * <NAME>
 *    gets_s
 *
 * <SYNOPSIS>
 *    char gets_s(char* buffer, size_t numberOfElements);
 *
 * <FUNCTION DESCRIPTION>
 *    The gets_s function reads a line from the standard input stream stdin and 
 *    stores it in buffer. The line consists of all characters up to and including
 *    the first newline character ('\n'). gets_s then replaces the newline 
 *    character with a null character ('\0') before returning the line.
 *    If the first character read is the end-of-file character, a null character
 *    is stored at the beginning of buffer and NULL is returned.
 *
 * <INPUT PARAMETERS>
 *    buffer                         Storage location for input string.
 *    numberOfElements               The size of the buffer.
 *
 * <OUTPUT PARAMETERS>
 *    buffer                         is updated
 *
 * <RETURN VALUE>
 *    buffer                         Successful operation
 *    NULL                           Buffer == NULL, numberOfElements == 0, 
 *                                   numberOfElements > SECUREC_STRING_MAX_LEN, 
 *                                   or read fail
 *******************************************************************************
 */

char* gets_s(char* buffer, size_t numberOfElements)
{
    int len = 0;
    int i = 1;
#ifdef COMPATIBLE_WIN_FORMAT
    if (buffer == NULL || numberOfElements == 0 || (numberOfElements > SECUREC_STRING_MAX_LEN && numberOfElements != (size_t)-1))
#else
    if (buffer == NULL || numberOfElements == 0 || numberOfElements > SECUREC_STRING_MAX_LEN)
#endif
    {
        SECUREC_ERROR_INVALID_PARAMTER("gets_s");
        return NULL;
    }
    
#ifdef COMPATIBLE_WIN_FORMAT
    if (fgets(buffer, (int)((numberOfElements == (size_t)-1)?SECUREC_STRING_MAX_LEN:numberOfElements), stdin) == NULL )
#else
    if (fgets(buffer, (int)numberOfElements, stdin) == NULL )
#endif
    {
        return NULL;
    }
    else
    {
        len = (int)strlen(buffer);

        if (len > 0)
        {
            if (buffer[len - 1] == '\r' || buffer[len - 1] == '\n')
            {
                /* remove ending carriage return and linefeed */
                while (i <= len && (buffer[len - i] ==  '\r' || buffer[len - i] == '\n') )
                {
                    ++i;
                }

                buffer[(len - i) + 1] = '\0';
            }
        }

        return buffer;
    }

}


