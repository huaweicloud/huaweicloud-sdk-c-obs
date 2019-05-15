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
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifndef WEOF
/*if some platforms don't have wchar.h, dont't include it*/
#if !(defined(SECUREC_VXWORKS_PLATFORM))
#include <wchar.h>  /*lint !e322 !e7*/
#endif

#ifndef WEOF
    #define WEOF ((wchar_t)-1)
#endif
#endif  /* WEOF */

#include "secureprintoutput.h"

/***************************/
#define _XXXUNICODE
#define TCHAR wchar_t
#define _T(x) L ## x
#define write_char write_char_w
#define write_multi_char write_multi_char_w
#define write_string write_string_w
/***************************/

/*extern const UINT8T securec__lookuptable_s[];*/

/*put a wchar to output stream */
/*LSD change "unsigned short" to wchar_t*/

static wchar_t __putwc_nolock (wchar_t _c, SECUREC_XPRINTF_STREAM* _stream)
{
    wchar_t wcRet = 0;
    if (((_stream)->_cnt -= (int)WCHAR_SIZE ) >= 0 )
    {
        *(wchar_t*)(_stream->_ptr)  = (wchar_t)_c; /*lint !e826*/
        _stream->_ptr += sizeof (wchar_t);
        /* remove LSD wcRet =  (unsigned short) (0xffff & (wchar_t)_c); */
        wcRet = _c;
    }
    else
    {
        wcRet = (wchar_t)WEOF;
    }
    return wcRet;
}

static void write_char_w(wchar_t ch, SECUREC_XPRINTF_STREAM* f, int* pnumwritten)
{
    if (__putwc_nolock(ch, f) == (wchar_t)WEOF)
    {
        *pnumwritten = -1;
    }
    else
    {
        ++(*pnumwritten);
    }
}

static void write_multi_char_w(wchar_t ch, int num, SECUREC_XPRINTF_STREAM* f, int* pnumwritten)
{
    while (num-- > 0)
    {
        write_char_w(ch, f, pnumwritten);
        if (*pnumwritten == -1)
        {
            break;
        }
    }
}

static void write_string_w (wchar_t* string, int len, SECUREC_XPRINTF_STREAM* f, int* pnumwritten)
{
    while (len-- > 0)
    {
        write_char_w(*string++, f, pnumwritten);
        if (*pnumwritten == -1)
        {
            /*
            if (errno == EILSEQ)
                write_char(_T('?'), f, pnumwritten);
            else
                break;
            */
            break;
        }
    }
}

#include "output.inl"


