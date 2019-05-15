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
#ifndef VOS_BASETYPE_H
#define VOS_BASETYPE_H

#ifndef  ULONGLONG

#if (VOS_APP_OS == VOS_OS_WIN32)
typedef  unsigned __int64       ULONGLONG;
#else
typedef  unsigned long long     ULONGLONG ;
#endif

#endif /* ULONGLONG */

#ifndef  LONGLONG
#if (VOS_APP_OS == VOS_OS_WIN32)
typedef  __int64                LONGLONG;
#else
typedef  long long              LONGLONG;
#endif

#endif /* ULONGLONG */

#ifndef  ULONG
typedef  unsigned long          ULONG ;
#endif

#ifndef  LONG
typedef  long                   LONG;
#endif

#ifndef  USHORT
typedef  unsigned short         USHORT ;
#endif

#ifndef  SHORT
typedef  short                  SHORT ;
#endif

#ifndef  UCHAR
typedef  unsigned char          UCHAR ;
#endif

#ifndef  CHAR
typedef  char                   CHAR ;
#endif

#ifndef  VOID
typedef  void                   VOID ;
#endif

#ifndef  UINT
typedef  unsigned int           UINT ;
#endif

#ifndef  VOS_BOOLEAN
enum VOS_BOOLEAN
{
    VOS_TRUE = 1,
    VOS_FALSE = 0
};
#endif

#ifndef  VOS_NULL
#define  VOS_NULL               NULL
#endif

#ifndef  VOS_SUCCESS
#define  VOS_SUCCESS             0
#endif

#ifndef  VOS_FAIL
#define  VOS_FAIL               -1
#endif

#if VOS_BYTE_ORDER == VOS_LITTLE_ENDIAN
#define VOS_ntohl(x)            ((((x) & 0x000000ff)<<24)|(((x) & 0x0000ff00) << 8) |(((x) & 0x00ff0000)>>8) |(((x) & 0xff000000) >>24))
#define VOS_htonl(x)            (VOS_ntohl(x))
#else
#define VOS_ntohl(x)            (x)
#define VOS_htonl(x)            (VOS_ntohl(x))
#endif

#if VOS_BYTE_ORDER == VOS_LITTLE_ENDIAN
#define VOS_ntohs(x)            ((((x) & 0x00ff) << 8) |(((x) & 0xff00) >> 8))
#define VOS_htons(x)            (VOS_ntohs(x))
#else
#define VOS_ntohs(x)          (x)
#define VOS_htons(x)          (VOS_ntohs(x))
#endif

#if VOS_BYTE_ORDER == VOS_LITTLE_ENDIAN
#define  VOS_ntohll( x )        (((VOS_ntohl( ((x) & 0xFFFFFFFF)))<< 32) | (VOS_ntohl(((x)>>32)&0xFFFFFFFF)))
#define  VOS_htonll( x )        (VOS_ntohll(x))
#else
#define  VOS_ntohll( x )        (x)
#define  VOS_htonll( x )        (VOS_ntohll(x))
#endif

#define QUEUE_MODE_NOWAIT  0
#define QUEUE_MODE_WAIT    1

#endif /*VOS_BASETYPE_H_INCLUDE  */
