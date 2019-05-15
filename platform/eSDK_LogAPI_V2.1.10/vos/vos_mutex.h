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
#ifndef VOS_MUTEX_H
#define VOS_MUTEX_H
#ifndef WIN32
#include <pthread.h>
#endif
namespace VPP
{
	typedef struct tagVOSMutex
	{
	#if VOS_APP_OS == VOS_OS_WIN32
	  HANDLE  mutex;
	#else
	  pthread_mutex_t  mutex;
	#endif
	}VOS_Mutex;


	VOS_Mutex *VOS_CreateMutex( );
	ULONG VOS_DestroyMutex( VOS_Mutex *pstMutex );
	ULONG VOS_MutexLock( VOS_Mutex *pstMutex );
	ULONG VOS_MutexUnlock( VOS_Mutex *pstMutex );
}//end namespace

#endif /* __VOS_MUTEX_H__ */
