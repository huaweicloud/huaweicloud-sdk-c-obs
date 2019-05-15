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
#ifndef DATA_TYPE_H_
#define DATA_TYPE_H_

//接口返回错误码
enum RetCode
{
	RET_SUCCESS				=	0,
	RET_FAILED				=	1,
	RET_NotInit				=	2,
	RET_INVALID_PARA		=	3,
	RET_NULL_POINTER		=	4,
}; 

// 日志级别
enum LogLevel
{
	DEBUG_LEVEL				=	0,
	INFO_LEVEL				=	1,
	WARN_LEVEL				=	2,
	ERROR_LEVEL				=	3
};

// 日志种类
enum LogCategory
{
	LOG_CATEGORY_INTERFACE	= 0,
	LOG_CATEGORY_OPERATION	= 1,
	LOG_CATEGORY_RUN		= 2
};

#define LOG_CATEGORY		3
#define LOG_LEVELS          4

// 非法日志级别
#define INVALID_LOG_LEVEL	((unsigned int)(-1))
#define INVALID_LOG_PATH	("")

/**
	*wifi连接状态
	* 
	*根据该回调函数，获得wifi连接状态
	*@return		0	未连接
	*@return		非0	连接
	*@attention	在wifi状态下上传日志，则需要实现该接口
	*@par			无
**/
typedef int (*WifiInfoCallBack)();

#endif

