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

@class LoggerAdaptor;
@interface eSDKIOSLog : NSObject
@property (nonatomic, retain) LoggerAdaptor *objLogger;

+ (id)getInstance;
+(id)allocWithZone:(NSZone *)zone;

-(int)logInit:(NSString*)product iniFile:(NSString*)iniFile logLevel:(unsigned int [])level logPath:(NSString*)path;
-(int)logFini:(NSString*)product;

-(void)logInterfaceInfo:(NSString*)product
          interfaceType:(NSString*)interfaceType
           protocolType:(NSString*)protocolType
          interfaceName:(NSString*)interfaceName
             sourceAddr:(NSString*)sourceAddr
             targetAddr:(NSString*)targetAddr
          transactionID:(NSString*)transactionID
                reqTime:(NSString*)reqTime
               respTime:(NSString*)respTime
             resultCode:(NSString*)resultCode
                 params:(NSString*)params,...;

-(void)logInterfaceError:(NSString*)product
           interfaceType:(NSString*)interfaceType
            protocolType:(NSString*)protocolType
           interfaceName:(NSString*)interfaceName
              sourceAddr:(NSString*)sourceAddr
              targetAddr:(NSString*)targetAddr
           transactionID:(NSString*)transactionID
                 reqTime:(NSString*)reqTime
                respTime:(NSString*)respTime
              resultCode:(NSString*)resultCode
                  params:(NSString*)params,...;

-(void) logOperateDebug:(NSString*) product
             moduleName:(NSString*) moduleName
               userName:(NSString*) userName
             clientFlag:(NSString*) clientFlag
             resultCode:(NSString*) resultCode
                keyInfo:(NSString*) keyInfo
                 params:(NSString*) params,...;

-(void) logOperateInfo:(NSString*) product
            moduleName:(NSString*) moduleName
              userName:(NSString*) userName
            clientFlag:(NSString*) clientFlag
            resultCode:(NSString*) resultCode
               keyInfo:(NSString*) keyInfo
                params:(NSString*) params,...;

-(void) logOperateWarn:(NSString*) product
            moduleName:(NSString*) moduleName
              userName:(NSString*) userName
            clientFlag:(NSString*) clientFlag
            resultCode:(NSString*) resultCode
               keyInfo:(NSString*) keyInfo
                params:(NSString*) params,...;

-(void) logOperateError:(NSString*) product
             moduleName:(NSString*) moduleName
               userName:(NSString*) userName
             clientFlag:(NSString*) clientFlag
             resultCode:(NSString*) resultCode
                keyInfo:(NSString*) keyInfo
                 params:(NSString*) params,...;


-(void) logRunDebug:(NSString*) product param:(NSString*) param;
-(void) logRunWarn:(NSString*) product param:(NSString*) param;
-(void) logRunError:(NSString*) product param:(NSString*) param;

@end
