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
#ifndef VOS_H
#define VOS_H

#include "vos_config.h"
#include "vos_basetype.h"
#include "vos_errno.h"
#include "vos_mem.h"
#include "vos_mutex.h"
#include "vos_sema.h"
#include "vos_thread.h"
#include "vos_time.h"
#include "vos_cond.h"

#ifndef  INFINITE
#define  INFINITE               0xFFFFFFFF
#endif

#define  VOS_DEFAULT_STACK_SIZE  (256*1024)
#define  VOS_THREAD_PRIOTITY     (75)

#define  VOS_MUTEX_MAXWAIT      INFINITE

#define MS_IN_SECONDS  1000
#define US_IN_MS       1000 
#define BITS_IN_BYTE   8

using namespace VPP;


#endif /* VOS_H_INCLUDE  */

