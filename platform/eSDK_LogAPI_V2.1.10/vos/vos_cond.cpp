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
#include "vos.h"
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
namespace VPP{  
#if VOS_APP_OS == VOS_OS_WIN32

LONG VOS_CondInit(VOS_COND* pCond)
{
    pCond->event = CreateEvent(NULL,FALSE,FALSE,NULL); 
    
    if(NULL == pCond->event)
    {
        return VOS_ERR;
    }

    return VOS_OK;
}
LONG VOS_CondWait(VOS_COND* pCond)
{
    DWORD ret = WaitForSingleObject(pCond->event,INFINITE);
    if(WAIT_OBJECT_0 != ret)//lint !e835
    {
        return VOS_ERR;
    }
    return VOS_OK;
}//lint !e818

LONG VOS_CondTimeWait(VOS_COND* pCond,LONG lSeconds)
{
    DWORD ret = WaitForSingleObject(pCond->event,(unsigned long)lSeconds*1000);
    if(WAIT_TIMEOUT == ret)
    {
        return VOS_ERR_COND_TIMEOUT;
    }
    if(WAIT_FAILED == ret)
    {
        return VOS_ERR;
    }

    return VOS_OK;
}//lint !e818

LONG VOS_CondSignal(VOS_COND* pCond)
{
    if(0 == SetEvent(pCond->event))
        return VOS_ERR;

    return VOS_OK;
}//lint !e818

LONG VOS_CondDestroy(VOS_COND* pCond)
{
    CloseHandle(pCond->event);
    return VOS_OK;
}//lint !e818

#else

    LONG VOS_CondInit(VOS_COND* pCond)
    {
		
	if(0 != pthread_mutex_init(&pCond->mutex, NULL))
	{
		return VOS_ERR;
	}
	if(0 != pthread_cond_init(&pCond->cond, NULL))
	{
		pthread_mutex_destroy(&pCond->mutex);
		return VOS_ERR;
	}
	
        return VOS_OK;
    }
    LONG VOS_CondWait(VOS_COND* pCond)
    {
	    pthread_mutex_lock(&pCond->mutex);   
	    pthread_cond_wait(&pCond->cond,&pCond->mutex);   
	    pthread_mutex_unlock(&pCond->mutex);
        return VOS_OK;
    }

    LONG VOS_CondTimeWait(VOS_COND* pCond,LONG lSeconds)
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        
    	timespec ts ={0};
        ts.tv_sec = now.tv_sec+lSeconds;
        ts.tv_nsec = now.tv_usec*1000;

	pthread_mutex_lock(&pCond->mutex);   
	int ret = pthread_cond_timedwait(&pCond->cond,&pCond->mutex,&ts);   
	pthread_mutex_unlock(&pCond->mutex);

        if(0 == ret)
        {
            return VOS_OK;
        }
        
	if(ETIMEDOUT == ret)
	{
	    return VOS_ERR_COND_TIMEOUT;
	}
	
        return VOS_ERR;
    }

    LONG VOS_CondSignal(VOS_COND* pCond)
    {
        pthread_mutex_lock(&pCond->mutex);   
        pthread_cond_signal(&pCond->cond);
        pthread_mutex_unlock(&pCond->mutex);
        		
        return VOS_OK;
    }

    LONG VOS_CondDestroy(VOS_COND* pCond)
    {
        pthread_mutex_destroy(&pCond->mutex);
        pthread_cond_destroy(&pCond->cond);
        return VOS_OK;
    }


#endif
}//end namespace
