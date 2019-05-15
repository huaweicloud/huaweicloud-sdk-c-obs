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

/*******************************************************************************
 * <NAME>
 *    wcstok_s
 *
 * <SYNOPSIS>
 *    wchar_t * wcstok_s(wchar_t* strToken, const wchar_t* strDelimit, wchar_t** context);
 *
 * <FUNCTION DESCRIPTION>
 *    wcstok_s is the wide-character versions of strtok_s.
 *
 * <INPUT PARAMETERS>
 *    strToken               String containing token or tokens.
 *    strDelimit             Set of delimiter characters.
 *    context                Used to store position information between calls to
 *                           strtok_s.
 *
 * <OUTPUT PARAMETERS>
 *
 * <RETURN VALUE>
 *    Returns a pointer to the next token found in strToken.
 *    They return NULL when no more tokens are found.
 *    Each call modifies strToken by substituting a NULL character for the first
 *    delimiter that occurs after the returned token.
 *
 *    return value          condition
 *    NULL                  context == NULL, strDelimit == NULL, strToken == NULL
 *                          && (*context) == NULL, or no token is found.
 *******************************************************************************
*/

wchar_t* wcstok_s(wchar_t* strToken, const wchar_t* strDelimit, wchar_t** context)
{
    wchar_t* token = NULL;
    const wchar_t* ctl = NULL;

    /* validation section */
    if (context == NULL || strDelimit == NULL)
    {
        return NULL;
    }
    if (strToken == NULL && (*context) == NULL)
    {
        return NULL;
    }

    /* If string==NULL, continue with previous string */
    if (!strToken)
    {
        strToken = *context;
    }

    /* Find beginning of token (skip over leading delimiters). Note that
    * there is no token if this loop sets string to point to the terminal null. 
    */
    for ( ; *strToken != 0 ; strToken++)
    {
        for (ctl = strDelimit; *ctl != 0 && *ctl != *strToken; ctl++)
            ;
        if (*ctl == 0)
        {
            break;
        }
    }

    token = strToken;

    /* Find the end of the token. If it is not the end of the string,
    * put a null there. 
    */
    for ( ; *strToken != 0 ; strToken++)
    {
        for (ctl = strDelimit; *ctl != 0 && *ctl != *strToken; ctl++)
            ;
        if (*ctl != 0)
        {
            *strToken++ = 0;
            break;
        }
    }

    /* Update the context */
    *context = strToken;

    /* Determine if a token has been found. */
    if (token == strToken)
    {
        return NULL;
    }
    else
    {
        return token;
    }
}


