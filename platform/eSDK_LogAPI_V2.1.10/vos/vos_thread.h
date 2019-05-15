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
#ifndef VOS_THREAD_H
#define VOS_THREAD_H
#include "vos_config.h"
namespace VPP{

typedef struct tagVOSThread
{
#if VOS_APP_OS == VOS_OS_WIN32
    ULONG ptheadID;
    HANDLE pthead; 
#else
    pthread_attr_t attr;
    pthread_t pthead;
#endif

}VOS_Thread;

#if VOS_APP_OS == VOS_OS_WIN32

typedef  ULONG(__stdcall * VOS_THREAD_FUNC)(VOID *)  ;

#else

typedef  VOID* ( * VOS_THREAD_FUNC)(VOID *)  ;

#endif

ULONG VOS_CreateThread( VOS_THREAD_FUNC pfnThread, VOID *args, 
          VOS_Thread **pstVosThread,ULONG ulStackSize);

VOID  VOS_pthread_exit(void *retval);

LONG  VOS_ThreadJoin(const VOS_Thread *pstVosThread );

VOID  VOS_DeleteThread( VOS_Thread *pstVosThread );

#if VOS_APP_OS == VOS_OS_WIN32
HANDLE  VOS_pthread_self();
#else
pthread_t  VOS_pthread_self();
#endif

}//end namespace

#endif /* __VOS_THREAD_H__ */
