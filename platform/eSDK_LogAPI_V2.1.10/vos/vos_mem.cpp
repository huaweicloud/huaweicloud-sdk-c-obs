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
#include "string.h"
//lint -e438
namespace VPP
{  
	VOID *VOS_malloc( LONG size )
	{
		CHAR *buf = VOS_NULL ;

		buf = ( CHAR *)malloc( (ULONG)size );//lint !e838
		if( NULL == buf )
		{
			return VOS_NULL ;
		}
		memset( buf , 0x00 , (ULONG)size );

		return buf ;
	}

	VOID VOS_free( VOID *buff )
	{
		if (NULL != buff)
		{
			free( buff );
			buff = VOS_NULL ;
		}

		return ;
	}
}//end namespace

//lint +e438

