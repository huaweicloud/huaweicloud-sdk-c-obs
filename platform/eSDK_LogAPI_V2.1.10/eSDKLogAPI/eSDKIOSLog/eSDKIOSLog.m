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

#import "eSDKIOSLog.h"
#import "LoggerAdapter.h"

@implementation eSDKIOSLog
@synthesize objLogger;
__strong static eSDKIOSLog *instance = nil;
#pragma mark Singleton Methods

+ (id)getInstance
{
    @synchronized(self)
    {
        if (nil == instance) {
            instance = [[self alloc] init];
        }
    }
    
    return instance;
}

+(id)allocWithZone:(NSZone *)zone
{
    @synchronized(self)
    {
        if (nil == instance) {
            instance = [super allocWithZone:zone];
        }
    }
    
    return instance;
}

- (id)copyWithZone:(NSZone *)zone
{
    return self;
}

- (id)init
{
    if (self = [super init])
    {
        self.objLogger = [[LoggerAdaptor alloc] init];
    }
    
    return self;
}

-(void)dealloc
{
#if __has_feature(objc_arc)
#else
    [self.objLogger delloc];
    [super dealloc];
#endif
}

-(int)logInit:(NSString*)product iniFile:(NSString*)iniFile logLevel:(unsigned int [])level logPath:(NSString*)path
{
    return [objLogger logInit:[product UTF8String] iniFile:[iniFile UTF8String] logLevel:level logPath:[path UTF8String]];
}

-(int)logFini:(NSString*)product
{
    return [objLogger logFini:[product UTF8String]];
}

-(void)logInterfaceInfo:(NSString*)product
          interfaceType:(NSString*)interfaceType
           protocolType:(NSString*)protocolType
          interfaceName:(NSString*)interfaceName
          transactionID:(NSString*)transactionID
                reqTime:(NSString*)reqTime
               respTime:(NSString*)respTime
             resultCode:(NSString*)resultCode
                 params:(NSString*)params,...
{
    va_list list;
    va_start(list, params);
    [objLogger logInterfaceInfo:[product UTF8String]
                  interfaceType:[interfaceType UTF8String]
                   protocolType:[protocolType UTF8String]
                  interfaceName:[interfaceName UTF8String]
                  transactionID:[transactionID UTF8String]
                        reqTime:[reqTime UTF8String]
                       respTime:[respTime UTF8String]
                     resultCode:[resultCode UTF8String]
                         params:[params UTF8String], list];
    va_end(list);
}

-(void)logInterfaceError:(NSString*)product
           interfaceType:(NSString*)interfaceType
            protocolType:(NSString*)protocolType
           interfaceName:(NSString*)interfaceName
           transactionID:(NSString*)transactionID
                 reqTime:(NSString*)reqTime
                respTime:(NSString*)respTime
              resultCode:(NSString*)resultCode
                  params:(NSString*)params,...
{
    va_list list;
    va_start(list, params);
    [objLogger logInterfaceError:[product UTF8String]
                   interfaceType:[interfaceType UTF8String]
                    protocolType:[protocolType UTF8String]
                   interfaceName:[interfaceName UTF8String]
                   transactionID:[transactionID UTF8String]
                         reqTime:[reqTime UTF8String]
                        respTime:[respTime UTF8String]
                      resultCode:[resultCode UTF8String]
                          params:[params UTF8String], list];
    va_end(list);
}

-(void) logOperateDebug:(NSString*) product
             moduleName:(NSString*) moduleName
               userName:(NSString*) userName
             clientFlag:(NSString*) clientFlag
             resultCode:(NSString*) resultCode
                keyInfo:(NSString*) keyInfo
                 params:(NSString*) params,...
{
    va_list list;
    va_start(list, params);
    [objLogger logOperateDebug:[product UTF8String]
                    moduleName:[moduleName UTF8String]
                      userName:[userName UTF8String]
                    clientFlag:[clientFlag UTF8String]
                    resultCode:[resultCode UTF8String]
                       keyInfo:[keyInfo UTF8String]
                        params:[params UTF8String], list];
    va_end(list);
}

-(void) logOperateInfo:(NSString*) product
            moduleName:(NSString*) moduleName
              userName:(NSString*) userName
            clientFlag:(NSString*) clientFlag
            resultCode:(NSString*) resultCode
               keyInfo:(NSString*) keyInfo
                params:(NSString*) params,...
{
    va_list list;
    va_start(list, params);
    [objLogger logOperateInfo:[product UTF8String]
                   moduleName:[moduleName UTF8String]
                     userName:[userName UTF8String]
                   clientFlag:[clientFlag UTF8String]
                   resultCode:[resultCode UTF8String]
                      keyInfo:[keyInfo UTF8String]
                       params:[params UTF8String], list];
    va_end(list);
}

-(void) logOperateWarn:(NSString*) product
            moduleName:(NSString*) moduleName
              userName:(NSString*) userName
            clientFlag:(NSString*) clientFlag
            resultCode:(NSString*) resultCode
               keyInfo:(NSString*) keyInfo
                params:(NSString*) params,...
{
    va_list list;
    va_start(list, params);
    [objLogger logOperateWarn:[product UTF8String]
                   moduleName:[moduleName UTF8String]
                     userName:[userName UTF8String]
                   clientFlag:[clientFlag UTF8String]
                   resultCode:[resultCode UTF8String]
                      keyInfo:[keyInfo UTF8String]
                       params:[params UTF8String], list];
    va_end(list);
    
}

-(void) logOperateError:(NSString*) product
             moduleName:(NSString*) moduleName
               userName:(NSString*) userName
             clientFlag:(NSString*) clientFlag
             resultCode:(NSString*) resultCode
                keyInfo:(NSString*) keyInfo
                 params:(NSString*) params,...
{
    va_list list;
    va_start(list, params);
    [objLogger logOperateError:[product UTF8String]
                    moduleName:[moduleName UTF8String]
                      userName:[userName UTF8String]
                    clientFlag:[clientFlag UTF8String]
                    resultCode:[resultCode UTF8String]
                       keyInfo:[keyInfo UTF8String]
                        params:[params UTF8String], list];
    va_end(list);
    
}


-(void) logRunDebug:(NSString*) product param:(NSString*) param
{
    [objLogger logRunDebug:[product UTF8String] param:[param UTF8String]];
}

-(void) logRunWarn:(NSString*) product param:(NSString*) param
{
    [objLogger logRunWarn:[product UTF8String] param:[param UTF8String]];
}

-(void) logRunError:(NSString*) product param:(NSString*) param
{
    [objLogger logRunError:[product UTF8String] param:[param UTF8String]];
}

-(void) logRunInfo:(NSString*) product param:(NSString*) param
{
    [objLogger logRunInfo:[product UTF8String] param:[param UTF8String]];
}

-(int) setLogPropertyEx:(NSString*)product logSize:(unsigned int [])logSize logNum:(unsigned int [])logNum
{
    return [objLogger setLogPropertyEx:[product UTF8String] logSize:logSize logNum:logNum];
}
@end
