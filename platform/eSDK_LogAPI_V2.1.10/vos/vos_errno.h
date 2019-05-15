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
#ifndef VOS_ERRNO_H
#define VOS_ERRNO_H

typedef  enum
{
    VOS_OK                  =      0x0,
    VOS_ERR                 =      0x1,
    VOS_ERR_PARAM           =      0x2,    
    VOS_ERR_MEM             =      0x3,
    VOS_ERR_SYS             =      0x4,    
    
    VOS_ERR_QUE_EMPTY       =      0x1000,    
    VOS_ERR_QUE_LEN         =      0x1001,
    VOS_ERR_QUE_TIMEOUT     =      0x1002,
    VOS_ERR_MUTEX_TIMEOUT   =      0x1003,    
    VOS_ERR_SEMA_TIMEOUT    =      0x1004,    
    VOS_ERR_LIST_EMPTY      =      0x1005, 
    VOS_ERR_DATANOTENOUGH   =      0x1006,
    VOS_ERR_COND_TIMEOUT    =      0x1007,

    VOS_SYS_ERR_BUTT
    
}VOS_SYS_ERR;

#endif

