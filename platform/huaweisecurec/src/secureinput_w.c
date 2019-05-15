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
#include <stddef.h>

#if !(defined(SECUREC_VXWORKS_PLATFORM))
#include <wchar.h> /*lint !e322 !e7*/
#include <wctype.h>
#endif

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WEOF
    #define WEOF ((wchar_t)-1)
#endif
typedef wchar_t _TCHAR;
typedef wchar_t _TUCHAR;

#if defined(SECUREC_VXWORKS_PLATFORM) && !defined(__WINT_TYPE__)
typedef wchar_t wint_t;
#endif

typedef wint_t _TINT;

#include "input.inl"


