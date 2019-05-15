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
#ifndef VOS_COND_H
#define VOS_COND_H

namespace VPP{
typedef struct
{
#if VOS_APP_OS == VOS_OS_WIN32
    HANDLE  event;
#else
    pthread_cond_t cond;
    pthread_mutex_t  mutex;
#endif
}VOS_COND;


    LONG VOS_CondInit(VOS_COND* pCond);
    LONG VOS_CondWait(VOS_COND* pCond);
    LONG VOS_CondTimeWait(VOS_COND* pCond,LONG lSeconds);
    LONG VOS_CondSignal(VOS_COND* pCond);
    LONG VOS_CondDestroy(VOS_COND* pCond);

}//end namespace

#endif /* __VOS_COND_H__ */
