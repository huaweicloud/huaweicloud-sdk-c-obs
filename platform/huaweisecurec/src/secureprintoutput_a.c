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
#include "secureprintoutput.h"
#include <stdio.h>

/***************************/
/*remove this def style #define TCHAR char*/
typedef char TCHAR;
#define _T(x) x
#define write_char write_char_a
#define write_multi_char write_multi_char_a
#define write_string write_string_a
/***************************/

/*extern const UINT8T securec__lookuptable_s[];*/

#include "output.inl"


