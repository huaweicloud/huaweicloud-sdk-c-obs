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
#include <curl/curl.h>
#include <stdlib.h>
#include <sys/select.h>
#include "request.h"
#include "request_context.h"
#include "securec.h"

#if defined __GNUC__ || defined LINUX
#include <unistd.h>
#endif

#ifdef WIN32
# pragma warning (disable:4127)
#endif


obs_status obs_get_request_context_fdsets(obs_request_context *request_context,
    fd_set *readFdSet, fd_set *writeFdSet, fd_set *exceptFdSet, int *maxFd)
{
    return ((curl_multi_fdset(request_context->curlm, readFdSet, writeFdSet,
    exceptFdSet, maxFd) == CURLM_OK) ? OBS_STATUS_OK : OBS_STATUS_InternalError);
}

int64_t obs_get_request_context_timeout(obs_request_context *request_context)
{
    long timeout = 0;

    if (curl_multi_timeout(request_context->curlm, &timeout) != CURLM_OK) {
        timeout = 0;
    }

    return timeout;
}

obs_status obs_runonce_request_context(obs_request_context *request_context, 
    int *requestsRemainingReturn)
{
    CURLMcode status = CURLM_OK;

    do {
        status = curl_multi_perform(request_context->curlm,
        requestsRemainingReturn);
        switch (status) {
            case CURLM_OK:
            case CURLM_CALL_MULTI_PERFORM:
                break;
            case CURLM_OUT_OF_MEMORY:
                return OBS_STATUS_OutOfMemory;
            default:
                return OBS_STATUS_InternalError;
        }
        CURLMsg *msg = NULL;
        int junk = 0;
        while ((msg = curl_multi_info_read(request_context->curlm, &junk)) != NULL) 
        {
            if (msg->msg != CURLMSG_DONE) {
                return OBS_STATUS_InternalError;
            }
            http_request *request = NULL;
            if (curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, 
                (char **) (char *) &request) != CURLE_OK) 
            {
                return OBS_STATUS_InternalError;
            }
            
            if (request->prev == request->next) {
                request_context->requests = 0;
            }
            else {
                request_context->requests = request->next;
                request->prev->next = request->next;
                request->next->prev = request->prev;
            }
            if ((msg->data.result != CURLE_OK) &&
                    (request->status == OBS_STATUS_OK)) 
            {
                request->status = request_curl_code_to_status
                                (msg->data.result);
            }
            
            if (curl_multi_remove_handle(request_context->curlm, 
                     msg->easy_handle) != CURLM_OK) 
            {
                return OBS_STATUS_InternalError;
            }
            
            request_finish(request);
            status = CURLM_CALL_MULTI_PERFORM;
        }
    } while (status == CURLM_CALL_MULTI_PERFORM);

    return OBS_STATUS_OK;
}

obs_status obs_create_request_context(obs_request_context **request_context_return)
{
    *request_context_return = 
        (obs_request_context *) malloc(sizeof(obs_request_context));
    
    if (!*request_context_return) {
        return OBS_STATUS_OutOfMemory;
    }

    memset_s(*request_context_return, sizeof(obs_request_context), 0, sizeof(obs_request_context));

    if (((*request_context_return)->curlm = curl_multi_init()) ==NULL) {
        free(*request_context_return);
        *request_context_return = NULL;
        return OBS_STATUS_OutOfMemory;
    }

    (*request_context_return)->requests = 0;

    return OBS_STATUS_OK;
}


void obs_destroy_request_context(obs_request_context *request_context)
{
    curl_multi_cleanup(request_context->curlm);
    http_request *r = request_context->requests, *rFirst = r;
    
    if (r) do {
        r->status = OBS_STATUS_Interrupted;
        http_request *rNext = r->next;
        request_finish(r);
        r = rNext;
    } while (r != rFirst);

    free(request_context);
    request_context = NULL;
}

obs_status obs_runall_request_context(obs_request_context *request_context)
{
    int requestsRemaining = 0;
    do {
        fd_set readfds, writefds, exceptfds;
        memset_s(&readfds, sizeof(readfds), 0, sizeof(readfds));
        memset_s(&writefds, sizeof(writefds), 0, sizeof(writefds));
        memset_s(&exceptfds, sizeof(exceptfds), 0, sizeof(exceptfds));

        int maxfd = 0;
        obs_status status = obs_get_request_context_fdsets
            (request_context, &readfds, &writefds, &exceptfds, &maxfd);
        if (status != OBS_STATUS_OK) {
            return status;
        }
#ifdef WIN32
        Sleep(0);
#else
        sleep(0);
#endif

        if (maxfd != -1) {
            int64_t timeout = obs_get_request_context_timeout(request_context);
            struct timeval tv = { (long)timeout / 1000, ((long)timeout % 1000) * 1000 };

            int selectResult = select(maxfd + 1, &readfds, &writefds, &exceptfds,(timeout == -1) ? 0 : &tv);
            switch(selectResult)
            {
                case 0:
                    COMMLOG(OBS_LOGERROR, "select timeout!");
                    break;
                case -1:
                    COMMLOG(OBS_LOGERROR, "select error!");
                    break;
                default:
                    break;
            }

        }
        status = obs_runonce_request_context(request_context, &requestsRemaining);
        if (status != OBS_STATUS_OK) {
            return status;
        }
    } while (requestsRemaining);
    
    return OBS_STATUS_OK;
}
