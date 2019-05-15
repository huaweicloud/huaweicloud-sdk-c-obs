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

#import "Logger.h"

@interface LoggerAdaptor : NSObject {
@private
    CLogger *objLogger;
}

-(int)logInit:(const char*)product iniFile:(const char*)iniFile logLevel:(unsigned int [])level logPath:(const char*)path;
-(int)logFini:(const char*)product;

-(void)logInterfaceInfo:(const char*)product
          interfaceType:(const char*)interfaceType
           protocolType:(const char*)protocolType
          interfaceName:(const char*)interfaceName
          transactionID:(const char*)transactionID
                reqTime:(const char*)reqTime
               respTime:(const char*)respTime
             resultCode:(const char*)resultCode
                 params:(const char*)params,
...;

-(void)logInterfaceError:(const char*)product
           interfaceType:(const char*)interfaceType
            protocolType:(const char*)protocolType
           interfaceName:(const char*)interfaceName
           transactionID:(const char*)transactionID
                 reqTime:(const char*)reqTime
                respTime:(const char*)respTime
              resultCode:(const char*)resultCode
                  params:(const char*)params,
...;

-(void) logOperateDebug:(const char*) product
             moduleName:(const char*) moduleName
               userName:(const char*) userName
             clientFlag:(const char*) clientFlag
             resultCode:(const char*) resultCode
                keyInfo:(const char*) keyInfo
                 params:(const char*) params,
...;

-(void) logOperateInfo:(const char*) product
            moduleName:(const char*) moduleName
              userName:(const char*) userName
            clientFlag:(const char*) clientFlag
            resultCode:(const char*) resultCode
               keyInfo:(const char*) keyInfo
                params:(const char*) params,
...;

-(void) logOperateWarn:(const char*) product
            moduleName:(const char*) moduleName
              userName:(const char*) userName
            clientFlag:(const char*) clientFlag
            resultCode:(const char*) resultCode
               keyInfo:(const char*) keyInfo
                params:(const char*) params,
...;

-(void) logOperateError:(const char*) product
             moduleName:(const char*) moduleName
               userName:(const char*) userName
             clientFlag:(const char*) clientFlag
             resultCode:(const char*) resultCode
                keyInfo:(const char*) keyInfo
                 params:(const char*) params,
...;


-(void) logRunDebug:(const char*) product param:(const char*) param;
-(void) logRunWarn:(const char*) product param:(const char*) param;
-(void) logRunError:(const char*) product param:(const char*) param;
-(void) logRunInfo:(const char*) product param:(const char*) param;

-(int) setLogPropertyEx:(const char*)product logSize:(unsigned int [])logSize logNum:(unsigned int [])logNum;
@end
