@echo off

::Usage: build.bat packageName release|debug
::
::obs dir
:: obs
:: |-bin
:: |-demo
:: |-include
::install package same to build

SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

set "G_MSVC_VERSION=vc100"
set "G_sln_NAME=obs"

set "G_BUILD_DIR=%~dp0"
set "G_sln_DIR=%G_BUILD_DIR%..\sln\%G_MSVC_VERSION%\"
set "G_MSBUILD_DIR=C:\Windows\Microsoft.NET\Framework\v4.0.30319\"
set "G_OBS_ROOT=%G_BUILD_DIR%..\"
set "G_OBS_3rd_depend_DIR=%G_OBS_ROOT%..\..\..\"
set "G_TARGET_PLATFORM=WIN32"
set "G_SECUREC_DIR=%G_sln_DIR%\Release"
set "G_3rd_PLATFORM=win32_x86_msvc\release"

::------------------第三方库文件编译-------------------

set  "G_SMIS_PROVIDER_DIR=%cd%\..\..\..\..\build\script\Provider\"
cd "%G_SMIS_PROVIDER_DIR%"

::call %cd%\build_openssl.bat win32

::call %cd%\build_zlib.bat win32

::call %cd%\build_libiconv.bat win32

::call %cd%\build_pcre.bat win32

::call %cd%\build_libssh2.bat win32

::call %cd%\build_curl.bat win32

::call %cd%\build_libxml2.bat win32

cd "%cd%\..\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\build"


:: -------------------- main --------------------------

::echo compile eSDK_OBS_API_C++ for windows.
echo.

if not exist %G_MSBUILD_DIR% (
    echo "error: .NET Framework should be installed."
    
    ENDLOCAL&exit /b 1
)

if /i not "%~2" == "debug" (
    if /i not "%~2" == "release" (
        call :USAGE
        
        ENDLOCAL&exit /b 1
    )
)

set "G_BUILD_OPT=%~2"
set "G_BUILD_OUTPUT_DIR=%G_BUILD_DIR%%G_MSVC_VERSION%\%G_BUILD_OPT%\"

set "G_PKG_NAME=%~1"

call :toLowerCase G_BUILD_OPT

set /a L_retValue=0
call :compileProvider %G_BUILD_OPT% L_retValue
if %L_retValue% NEQ 0 (
    echo error:build eSDK_OBS_API_C++ failed.
    
   ENDLOCAL&exit /b 1
)

set /a L_retValue=0
call :package %G_PKG_NAME% L_retValue
if %L_retValue% NEQ 0 (
    echo error:package failed.
    
    ENDLOCAL&exit /b 1
)

echo complete.

ENDLOCAL
:: ------------------ normal script execute flow end -----------------------
goto:EOF

:: -------------------- function ----------------------

:: ****************************************************************************
:: Function Name: compileProvider
:: Description: 
:: Parameter: %1 release|debug  %2 L_retValue 
:: Return: none
:: ****************************************************************************
:compileProvider
  
    set "L_sln_File=%G_sln_DIR%\%G_sln_NAME%.sln"
    if not exist %L_sln_File% (
        echo error: the sln is not exist.
        call :USAGE
        set /a %~2=1
        exit /b 1
    )
    
    if exist %G_BUILD_OUTPUT_DIR% (
        echo delete old output dir.
        echo.
        rmdir /s /q %G_BUILD_OUTPUT_DIR%
    )

    %G_MSBuild_DIR%MSBuild.exe %L_sln_File% /t:rebuild /p:Configuration=%1;Platform=%G_TARGET_PLATFORM%
    set /a %~2=%ERRORLEVEL%

goto:EOF

:: ****************************************************************************
:: Function Name: package
:: Description: 
:: Parameter: $1 packageName $2 retValue
:: Return: none
:: ****************************************************************************
:package
   set "L_TMP_PACKAGE_DIR=%G_BUILD_DIR%obs\"
   if exist %L_TMP_PACKAGE_DIR% (
       rmdir /q /s %L_TMP_PACKAGE_DIR%
   )
   
   mkdir %L_TMP_PACKAGE_DIR%
   mkdir %L_TMP_PACKAGE_DIR%demo
   mkdir %L_TMP_PACKAGE_DIR%include
   mkdir %L_TMP_PACKAGE_DIR%bin
   mkdir %L_TMP_PACKAGE_DIR%demo\huaweisecurec
   mkdir %L_TMP_PACKAGE_DIR%demo\huaweisecurec\lib
   mkdir %L_TMP_PACKAGE_DIR%demo\huaweisecurec\include

   if %ERRORLEVEL% NEQ 0 (
      echo make dir %L_TMP_PACKAGE_DIR% failed.
      set /a %~2=1
      exit /b 1
   )
   
   echo copy 3rd files...
   set "L_TMP_3rd_LIB_PATH=%G_OBS_3rd_depend_DIR%third_party_groupware\eSDK_Storage_Plugins\"   
   if not exist "%L_TMP_3rd_LIB_PATH%" (
       echo %L_TMP_3rd_LIB_PATH% not exist.
       set /a %~2=1
       exit /b 1
   )
   
   pushd %L_TMP_3rd_LIB_PATH%
     
   xcopy /c /Y curl-7.64.0\bin\%G_3rd_PLATFORM%\*.dll %L_TMP_PACKAGE_DIR%bin
   xcopy /c /Y curl-7.64.0\bin\%G_3rd_PLATFORM%\*.exe %L_TMP_PACKAGE_DIR%bin
   xcopy /c /Y curl-7.64.0\bin\%G_3rd_PLATFORM%\*.manifest %L_TMP_PACKAGE_DIR%bin

   
   xcopy /c /Y openssl-1.0.2n\bin\%G_3rd_PLATFORM%\*.dll %L_TMP_PACKAGE_DIR%bin
   xcopy /c /Y openssl-1.0.2n\bin\%G_3rd_PLATFORM%\*.exe %L_TMP_PACKAGE_DIR%bin
   xcopy /c /Y openssl-1.0.2n\bin\%G_3rd_PLATFORM%\*.manifest %L_TMP_PACKAGE_DIR%bin 

   
   xcopy /c /Y pcre-8.39\bin\%G_3rd_PLATFORM%\*.dll %L_TMP_PACKAGE_DIR%bin
   xcopy /c /Y pcre-8.39\bin\%G_3rd_PLATFORM%\*.exe %L_TMP_PACKAGE_DIR%bin
   xcopy /c /Y pcre-8.39\bin\%G_3rd_PLATFORM%\*.manifest %L_TMP_PACKAGE_DIR%bin
    
   popd
   
   
   xcopy /c /Y %G_OBS_3rd_depend_DIR%platform\eSDK_LogAPI_V2.1.10\C\release\*.dll %L_TMP_PACKAGE_DIR%bin
   
   
   echo "copy vc crt..."    
   set "L_TMP_VC_CRT_DIR=%G_OBS_3rd_depend_DIR%third_party\smis\win32_x86_msvc\Microsoft.VC100.CRT\"
   echo %L_TMP_VC_CRT_DIR%
   xcopy /c /Y %L_TMP_VC_CRT_DIR%*.dll %L_TMP_PACKAGE_DIR%bin
   
   
   echo copy files...
   for /f %%i in ('dir /b %G_BUILD_OUTPUT_DIR%\*.dll') do (
      xcopy /c /Y %G_BUILD_OUTPUT_DIR%%%i %L_TMP_PACKAGE_DIR%bin
   )
   
   for /f %%i in ('dir /b %G_BUILD_OUTPUT_DIR%\*.lib') do (
      xcopy /c /Y %G_BUILD_OUTPUT_DIR%%%i %L_TMP_PACKAGE_DIR%bin
   )
   :: open huaweisecurec
   xcopy /c /Y %G_OBS_3rd_depend_DIR%platform\huaweisecurec\include\*.h   %L_TMP_PACKAGE_DIR%demo\huaweisecurec\include
   xcopy /c /Y %G_BUILD_OUTPUT_DIR%huaweisecurec.lib %L_TMP_PACKAGE_DIR%demo\huaweisecurec\lib
   
   xcopy /c /Y %G_BUILD_DIR%..\inc\eSDKOBS.h %L_TMP_PACKAGE_DIR%include
   xcopy /c /Y %G_BUILD_DIR%..\build\OBS.ini %L_TMP_PACKAGE_DIR%bin
   
   xcopy /c /Y %G_BUILD_OUTPUT_DIR%libeSDKOBS.lib %L_TMP_PACKAGE_DIR%demo
   xcopy /c /Y %G_BUILD_DIR%..\inc\eSDKOBS.h %L_TMP_PACKAGE_DIR%demo
   xcopy /c /Y %G_BUILD_DIR%..\cert\client.crt %L_TMP_PACKAGE_DIR%demo
   xcopy /c /Y %G_BUILD_DIR%..\cert\server.jks %L_TMP_PACKAGE_DIR%demo
   xcopy /c /Y %G_OBS_3rd_depend_DIR%source\eSDK_OBS_API\eSDK_OBS_API_C++_Demo\README.txt %L_TMP_PACKAGE_DIR%demo
   xcopy /c /Y %G_OBS_3rd_depend_DIR%source\eSDK_OBS_API\eSDK_OBS_API_C++_Demo\set.bat %L_TMP_PACKAGE_DIR%demo
   xcopy /c /Y %G_OBS_3rd_depend_DIR%source\eSDK_OBS_API\eSDK_OBS_API_C++_Demo\demo_windows.cpp %L_TMP_PACKAGE_DIR%demo
   xcopy /c /Y %G_OBS_3rd_depend_DIR%source\eSDK_OBS_API\eSDK_OBS_API_C++_Demo\sln\*.sln %L_TMP_PACKAGE_DIR%demo\sln\
   xcopy /c /Y %G_OBS_3rd_depend_DIR%source\eSDK_OBS_API\eSDK_OBS_API_C++_Demo\sln\*.vcxproj %L_TMP_PACKAGE_DIR%demo\sln\
   xcopy /c /Y %G_OBS_3rd_depend_DIR%source\eSDK_OBS_API\eSDK_OBS_API_C++_Demo\sln\*.filters %L_TMP_PACKAGE_DIR%demo\sln\
   xcopy /c /Y %G_OBS_3rd_depend_DIR%source\eSDK_OBS_API\eSDK_OBS_API_C++_Demo\build_demo.bat %L_TMP_PACKAGE_DIR%demo
   xcopy /c /Y %G_BUILD_DIR%..\readme.txt %L_TMP_PACKAGE_DIR%

   
   cd %G_BUILD_DIR%
   
   :: package
   if not "%~1" == "all" (
       7z a -tzip %1.zip %L_TMP_PACKAGE_DIR%
       if !ERRORLEVEL! NEQ 0 (
           echo error:package faild.
           set /a %~2=0
           exit /b 1
       )
   )
   
   set /a %~2=0
goto:EOF

:: ****************************************************************************
:: Function Name: toUpperCase
:: Description: 
:: Parameter: $1 string in ASCII
:: Return: none
:: ****************************************************************************
:toUpperCase
    for %%a in ("a=A" "b=B" "c=C" "d=D" "e=E" "f=F" "g=G" "h=H" "i=I" "j=J" "k=K" "l=L" "m=M" "n=N" "o=O" "p=P" "q=Q" "r=R" "s=S" "t=T" "u=U" "v=V" "w=W" "x=X" "y=Y" "z=Z") do ( 
        call set "%1=%%%1:%%~a%%"
    )
goto:EOF

:: ****************************************************************************
:: Function Name: toLowerCase
:: Description: 
:: Parameter: $1 string in ASCII
:: Return: none
:: ****************************************************************************
:toLowerCase
    for %%a in ("A=a" "B=b" "C=c" "D=d" "E=e" "F=f" "G=g" "H=h" "I=i" "J=j" "K=k" "L=l" "M=m" "N=n" "O=o" "P=p" "Q=q" "R=r" "S=s" "T=t" "U=u" "V=v" "W=w" "X=x" "Y=y" "Z=z") do ( 
        call set "%1=%%%1:%%~a%%"
    )
goto:EOF

:USAGE
    echo "Usage: %~nx0 pkgName debug|release"
goto:EOF

@echo on
