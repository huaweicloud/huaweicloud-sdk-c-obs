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
#ifndef WIN32
#include "unistd.h"
#endif

namespace VPP{  
    
ULONG VOS_GetTicks()
{
    ULONG ticks = 0 ;
#if VOS_APP_OS == VOS_OS_LINUX
    struct timeval now;
    gettimeofday(&now, VOS_NULL);
    ticks = (ULONG)(now.tv_sec * 1000 + now.tv_usec / 1000);
#elif VOS_APP_OS == VOS_OS_VXWORKS
    ticks = (ULONG)(tickGet() * 1000 / sysClkRateGet());
#elif VOS_APP_OS == VOS_OS_WIN32
    ticks = GetTickCount();  //lint !e838
#endif
    return( ticks );
}

VOID VOS_Delay(ULONG ulDelayTimeMs)
{
    LONG was_error;

    struct timeval tv;
    ULONG then, now, elapsed;
    then = VOS_GetTicks();

    do 
    {
        errno = 0;
        /* Calculate the time interval left (in case of interrupt) */
        now = VOS_GetTicks();
        elapsed = (now-then);
        then = now;
        if ( elapsed >= ulDelayTimeMs )
        {
            break;
        }

        ulDelayTimeMs -= elapsed;
        tv.tv_sec = (long)(ulDelayTimeMs/1000);
        tv.tv_usec = (long)((ulDelayTimeMs%1000)*1000);

        was_error = select(0, VOS_NULL, VOS_NULL, VOS_NULL, &tv);

    } while ( was_error && (errno == EINTR) );
}

VOID VOS_Sleep( ULONG ulMs )
{
#if VOS_APP_OS == VOS_OS_WIN32
    Sleep(ulMs);
#else
    usleep(ulMs * 1000);
#endif

    return ;
}
}//end namespace

