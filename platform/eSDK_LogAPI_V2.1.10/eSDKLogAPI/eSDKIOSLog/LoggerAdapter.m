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

#import "LoggerAdapter.h"

@implementation LoggerAdaptor

- (id) init {
    if (self = [super init]) {
        objLogger = new CLogger();
    }
    
    return self;
}

- (void) dealloc {
    if (objLogger != NULL) {
        delete objLogger;
        objLogger = NULL;
    }
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

-(int)logInit:(const char*)product iniFile:(const char*)iniFile logLevel:(unsigned int [])level logPath:(const char*)path
{
    if(objLogger == NULL)
    {
        return 0;
    }
    
    return objLogger->LogInit(product, iniFile, level, path);
}

-(int)logFini:(const char*)product
{
    if(objLogger == NULL)
    {
        return 0;
    }
    
    return objLogger->LogFini(product);
}

-(void)logInterfaceInfo:(const char*)product
          interfaceType:(const char*)interfaceType
           protocolType:(const char*)protocolType
          interfaceName:(const char*)interfaceName
          transactionID:(const char*)transactionID
                reqTime:(const char*)reqTime
               respTime:(const char*)respTime
             resultCode:(const char*)resultCode
                 params:(const char*)params,
...
{
    if(objLogger == NULL)
    {
        return;
    }
    
    va_list list;
    va_start(list, params);
    objLogger->Log_Interface_Info(product
                                  , interfaceType
                                  , protocolType
                                  , interfaceName
                                  , transactionID
                                  , reqTime
                                  , respTime
                                  , resultCode
                                  , params
                                  , list);
    va_end(list);
}

-(void)logInterfaceError:(const char*)product
           interfaceType:(const char*)interfaceType
            protocolType:(const char*)protocolType
           interfaceName:(const char*)interfaceName
           transactionID:(const char*)transactionID
                 reqTime:(const char*)reqTime
                respTime:(const char*)respTime
              resultCode:(const char*)resultCode
                  params:(const char*)params,
...
{
    if(objLogger == NULL)
    {
        return;
    }
    
    va_list list;
    va_start(list, params);
    objLogger->Log_Interface_Error(product
                                   , interfaceType
                                   , protocolType
                                   , interfaceName
                                   , transactionID
                                   , reqTime
                                   , respTime
                                   , resultCode
                                   , params
                                   , list);
    va_end(list);
}

-(void) logOperateDebug:(const char*) product
             moduleName:(const char*) moduleName
               userName:(const char*) userName
             clientFlag:(const char*) clientFlag
             resultCode:(const char*) resultCode
                keyInfo:(const char*) keyInfo
                 params:(const char*) params,
...
{
    if(objLogger == NULL)
    {
        return;
    }
    
    va_list list;
    va_start(list, params);
    objLogger->Log_Operate_Debug(product, moduleName, userName, clientFlag, resultCode, keyInfo, params, list);
    va_end(list);
}

-(void) logOperateInfo:(const char*) product
            moduleName:(const char*) moduleName
              userName:(const char*) userName
            clientFlag:(const char*) clientFlag
            resultCode:(const char*) resultCode
               keyInfo:(const char*) keyInfo
                params:(const char*) params,
...
{
    if(objLogger == NULL)
    {
        return;
    }
    
    va_list list;
    va_start(list, params);
    objLogger->Log_Operate_Info(product, moduleName, userName, clientFlag, resultCode, keyInfo, params, list);
    va_end(list);
}

-(void) logOperateWarn:(const char*) product
            moduleName:(const char*) moduleName
              userName:(const char*) userName
            clientFlag:(const char*) clientFlag
            resultCode:(const char*) resultCode
               keyInfo:(const char*) keyInfo
                params:(const char*) params,
...
{
    if(objLogger == NULL)
    {
        return;
    }
    
    va_list list;
    va_start(list, params);
    objLogger->Log_Operate_Warn(product, moduleName, userName, clientFlag, resultCode, keyInfo, params, list);
    va_end(list);
}

-(void) logOperateError:(const char*) product
             moduleName:(const char*) moduleName
               userName:(const char*) userName
             clientFlag:(const char*) clientFlag
             resultCode:(const char*) resultCode
                keyInfo:(const char*) keyInfo
                 params:(const char*) params,
...
{
    if(objLogger == NULL)
    {
        return;
    }
    
    va_list list;
    va_start(list, params);
    objLogger->Log_Operate_Error(product, moduleName, userName, clientFlag, resultCode, keyInfo, params, list);
    va_end(list);
}

-(void) logRunDebug:(const char*) product param:(const char*) param
{
    if(objLogger == NULL)
    {
        return;
    }
    
    objLogger->Log_Run_Debug(product, param);
}

-(void) logRunWarn:(const char*) product param:(const char*) param
{
    if(objLogger == NULL)
    {
        return;
    }
    
    objLogger->Log_Run_Warn(product, param);
}

-(void) logRunError:(const char*) product param:(const char*) param
{
    if(objLogger == NULL)
    {
        return;
    }
    
    objLogger->Log_Run_Error(product, param);
}

-(void) logRunInfo:(const char*) product param:(const char*) param
{
    if(objLogger == NULL)
    {
        return;
    }
    
    objLogger->Log_Run_Info(product, param);
}

-(int) setLogPropertyEx:(const char *)product logSize:(unsigned int [])logSize logNum:(unsigned int [])logNum
{
    if (objLogger == NULL)
    {
        return RET_NULL_POINTER;
    }
    
    return objLogger->setLogPropertyEx(product, logSize, logNum);
}
@end
