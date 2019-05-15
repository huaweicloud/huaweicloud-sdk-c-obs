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

#import <Foundation/Foundation.h>
#import "eSDKLogDataType.h"
@class LoggerAdaptor;
@interface eSDKIOSLog : NSObject
@property (nonatomic, retain) LoggerAdaptor *objLogger;

//Sington instance
+ (id)getInstance;

//Rewrite for sington purpose
+(id)allocWithZone:(NSZone *)zone;

//Logger init
-(int)logInit:(NSString*)sdkname iniFile:(NSString*)iniFile logLevel:(unsigned int [])level logPath:(NSString*)path;
//Logger uninit
-(int)logFini:(NSString*)sdkname;

/*
 * Below is logger's operations
 */
-(void)logInterfaceInfo:(NSString*)sdkname
          interfaceType:(NSString*)interfaceType
           protocolType:(NSString*)protocolType
          interfaceName:(NSString*)interfaceName
          transactionID:(NSString*)transactionID
                reqTime:(NSString*)reqTime
               respTime:(NSString*)respTime
             resultCode:(NSString*)resultCode
                 params:(NSString*)params,...;

-(void)logInterfaceError:(NSString*)sdkname
           interfaceType:(NSString*)interfaceType
            protocolType:(NSString*)protocolType
           interfaceName:(NSString*)interfaceName
           transactionID:(NSString*)transactionID
                 reqTime:(NSString*)reqTime
                respTime:(NSString*)respTime
              resultCode:(NSString*)resultCode
                  params:(NSString*)params,...;

-(void) logOperateDebug:(NSString*) sdkname
             moduleName:(NSString*) moduleName
               userName:(NSString*) userName
             clientFlag:(NSString*) clientFlag
             resultCode:(NSString*) resultCode
                keyInfo:(NSString*) keyInfo
                 params:(NSString*) params,...;

-(void) logOperateInfo:(NSString*) sdkname
            moduleName:(NSString*) moduleName
              userName:(NSString*) userName
            clientFlag:(NSString*) clientFlag
            resultCode:(NSString*) resultCode
               keyInfo:(NSString*) keyInfo
                params:(NSString*) params,...;

-(void) logOperateWarn:(NSString*) sdkname
            moduleName:(NSString*) moduleName
              userName:(NSString*) userName
            clientFlag:(NSString*) clientFlag
            resultCode:(NSString*) resultCode
               keyInfo:(NSString*) keyInfo
                params:(NSString*) params,...;

-(void) logOperateError:(NSString*) sdkname
             moduleName:(NSString*) moduleName
               userName:(NSString*) userName
             clientFlag:(NSString*) clientFlag
             resultCode:(NSString*) resultCode
                keyInfo:(NSString*) keyInfo
                 params:(NSString*) params,...;


-(void) logRunDebug:(NSString*) sdkname param:(NSString*) param;
-(void) logRunWarn:(NSString*) sdkname param:(NSString*) param;
-(void) logRunError:(NSString*) sdkname param:(NSString*) param;
-(void) logRunInfo:(NSString*) sdkname param:(NSString*) param;

-(int) setLogPropertyEx:(NSString*)sdkname logSize:(unsigned int [])logSize logNum:(unsigned int [])logNum;

@end

//Log instance define
#define IOSLogInstance [eSDKIOSLog getInstance]
