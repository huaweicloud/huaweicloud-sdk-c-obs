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
#include "vos_config.h"
namespace VPP{  
ULONG  VOS_CreateThread( VOS_THREAD_FUNC pfnThread, VOID *args, VOS_Thread **pstVosThread,ULONG ulStackSize)
{
    VOS_Thread *pstThread = VOS_NULL ;
#ifdef HIKVISION
    struct sched_param param;
    memset(&param, 0x0, sizeof(param));
    PUMW_LOG(VPP::PUMW_LOG_INFO, "VOS_CreateThread start.\n");
#endif
    pstThread = (VOS_Thread*)VOS_malloc(sizeof(VOS_Thread));//lint !e838
    if( VOS_NULL == pstThread )
    {
        //PUMW_LOG(VPP::PUMW_LOG_ERROR, "VOS_malloc fail.\n");
        return VOS_ERR_MEM; 
    }
    
#if (VOS_APP_OS == VOS_OS_LINUX) || (VOS_APP_OS == VOS_OS_VXWORKS) || (VOS_APP_OS == VOS_OS_ANDROID)
    if ( pthread_attr_init(&pstThread->attr) != 0 )
    {
        //PUMW_LOG(VPP::PUMW_LOG_ERROR, "pthread_attr_init fail.\n");
        VOS_free(pstThread);        
        return VOS_ERR ;
    }    

    pthread_attr_setdetachstate(&pstThread->attr, PTHREAD_CREATE_JOINABLE );

    if( 0 == ulStackSize )
    {
        ulStackSize = VOS_DEFAULT_STACK_SIZE;
    }    
    if (pthread_attr_setstacksize(&pstThread->attr, (size_t)ulStackSize))
    {
        //PUMW_LOG(PUMW_LOG_ERROR, "pthread_attr_setstacksize fail.\n");
        VOS_free(pstThread);    
        return VOS_ERR ;
    }    

#ifdef HIKVISION
    PUMW_LOG(PUMW_LOG_INFO,"pthread_attr_getschedparam.\n");
    if (pthread_attr_getschedparam(&pstThread->attr, &param))
    {
        PUMW_LOG(PUMW_LOG_ERROR, "pthread_attr_getschedparam fail.\n");
        VOS_free(pstThread);    
        return VOS_ERR ;    
    }

    PUMW_LOG(PUMW_LOG_INFO, "pthread_attr_setschedpolicy.\n");
    if(pthread_attr_setschedpolicy(&pstThread->attr,SCHED_RR))
    {
        PUMW_LOG(PUMW_LOG_ERROR, "pthread_attr_setschedpolicy fail.\n");
        VOS_free(pstThread);    
        return VOS_ERR ;   
    }

    PUMW_LOG(PUMW_LOG_INFO, "pthread_attr_setschedparam.\n");
    param.sched_priority = VOS_THREAD_PRIOTITY;
    if (pthread_attr_setschedparam(&pstThread->attr,&param))
    {
        PUMW_LOG(PUMW_LOG_ERROR, "pthread_attr_setschedparam fail.\n");
        VOS_free(pstThread);    
        return VOS_ERR ;   
    }
#endif
  
    if ( pthread_create(&pstThread->pthead, &pstThread->attr, pfnThread, args) != 0 ) 
    {
        //PUMW_LOG(PUMW_LOG_ERROR, "pthread_create fail.\n");
        VOS_free(pstThread);
        return VOS_ERR ;
    }

#elif VOS_APP_OS == VOS_OS_WIN32
    pstThread->pthead = CreateThread(NULL,ulStackSize,pfnThread,args,0,&pstThread->ptheadID);
    if (NULL == pstThread->pthead)
    {
        VOS_free(pstThread);
        return VOS_ERR ;
    }

#endif
    *pstVosThread = pstThread ;

    return VOS_OK;
}

LONG VOS_ThreadJoin(const VOS_Thread *pstVosThread )
{   
#if VOS_APP_OS == VOS_OS_WIN32
    ULONG ulResult = WaitForSingleObject(pstVosThread->pthead,5*1000);//µÈ´ý5Ãë
    if(WAIT_OBJECT_0 != ulResult)//lint !e835
    {
		(void)TerminateThread(pstVosThread->pthead,0);
    }

    CloseHandle(pstVosThread->pthead);
    return VOS_OK;
#else
    return pthread_join(pstVosThread->pthead, 0);
#endif 
}

VOID  VOS_DeleteThread( VOS_Thread *pstVosThread )
{
    (void)VOS_ThreadJoin(pstVosThread);
    VOS_free(pstVosThread);
}

VOID  VOS_pthread_exit(void *retval)
{
#if VOS_APP_OS == VOS_OS_WIN32
    ExitThread(0);
#else
    pthread_exit(retval);
#endif
    //PUMW_LOG(VPP::PUMW_LOG_WARNING, "VOS_pthread_exit.\n");
};//lint !e818

#if VOS_APP_OS == VOS_OS_WIN32
HANDLE  VOS_pthread_self()
{
    return GetCurrentThread();
};

#else

pthread_t  VOS_pthread_self()
{
    return pthread_self();
};
#endif

}//end namespace



