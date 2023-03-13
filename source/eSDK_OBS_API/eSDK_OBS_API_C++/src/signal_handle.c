/*********************************************************************************
* Copyright 2022 Huawei Technologies Co.,Ltd.
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
#include "signal_handle.h"
#include "log.h"
#if defined __GNUC__ || defined LINUX
#include <pthread.h>
#include <signal.h>

static void handle_SIGPIPE(int signal, siginfo_t *siginfo, void *u_contxt)
{
    COMMLOG(OBS_LOGERROR,"received signal:%d!", signal);
	if(siginfo == NULL){
        COMMLOG(OBS_LOGERROR,"NULL siginfo!");
	}else {
        COMMLOG(OBS_LOGERROR,"si_signo:%d", siginfo->si_signo);
        COMMLOG(OBS_LOGERROR,"si_errno:%d", siginfo->si_errno);
        COMMLOG(OBS_LOGERROR,"si_code:%d", siginfo->si_code);
        COMMLOG(OBS_LOGERROR,"si_pid:%d", siginfo->si_pid);
        COMMLOG(OBS_LOGERROR,"si_uid:%d", siginfo->si_uid);
        COMMLOG(OBS_LOGERROR,"si_status:%d", siginfo->si_status);
	}
	return;
}
void set_sigaction_for_sigpipe(){
    struct sigaction sigact;
	sigact.sa_sigaction = handle_SIGPIPE;
   	sigact.sa_flags = SA_SIGINFO;
   	int sigemptysetResult = memset_s(&sigact.sa_mask,sizeof(sigact.sa_mask),0,sizeof(sigact.sa_mask));
	int sigactionResult = sigaction(SIGPIPE, &sigact, NULL);
    COMMLOG(OBS_LOGINFO,"Result of sigemptyset:%d", sigemptysetResult);
    COMMLOG(OBS_LOGINFO,"Result of sigaction:%d", sigactionResult);
}

void unset_sigaction_for_sigpipe(){
    struct sigaction sigact;
	sigact.sa_sigaction = NULL;
   	sigact.sa_flags = SA_SIGINFO;
   	int sigemptysetResult = memset_s(&sigact.sa_mask,sizeof(sigact.sa_mask),0,sizeof(sigact.sa_mask));
	int sigactionResult = sigaction(SIGPIPE, &sigact, NULL);
    COMMLOG(OBS_LOGINFO,"Result of sigemptyset:%d", sigemptysetResult);
    COMMLOG(OBS_LOGINFO,"Result of sigaction:%d", sigactionResult);
}
#endif