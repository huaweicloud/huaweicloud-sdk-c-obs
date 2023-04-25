@echo off

REM 编译obs的windows版本，并将.lib与.h .c拷贝到当前目录下，运行此demo前请执行该脚本，以保证demo为最新版本。


SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

set "G_MSVC_VERSION=vc100"
set "G_sln_NAME=obs"

set "G_CUR_DIR=%~dp0"
set "G_BUILD_DIR=%G_CUR_DIR%..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\build\"
set "G_sln_DIR=%G_BUILD_DIR%..\sln\%G_MSVC_VERSION%\"
set "G_MSBUILD_DIR=C:\Windows\Microsoft.NET\Framework\v4.0.30319\"
set "G_OBS_ROOT=%G_BUILD_DIR%..\"
set "G_OBS_3rd_depend_DIR=%G_OBS_ROOT%..\..\..\"
set "G_TARGET_PLATFORM=WIN32"
set "G_3rd_PLATFORM=win32_x86_msvc"
:: -------------------- main --------------------------

echo compile eSDK_OBS_API_C++ for windows.
echo.

if not exist %G_MSBUILD_DIR% (
    echo "error: .NET Framework should be installed."
    
    ENDLOCAL&exit /b 1
)


set "G_BUILD_OPT=release"
set "G_BUILD_OUTPUT_DIR=%G_BUILD_DIR%%G_MSVC_VERSION%\%G_BUILD_OPT%\"


set /a L_retValue=0
call :compileProvider %G_BUILD_OPT% L_retValue
if %L_retValue% NEQ 0 (
    echo error:build eSDK_OBS_API_C++ failed.
    
   ENDLOCAL&exit /b 1
)

xcopy /c /Y %G_BUILD_DIR%%G_MSVC_VERSION%\release\*.lib %G_CUR_DIR%
xcopy /c /Y %G_BUILD_DIR%..\src\s3.c %G_CUR_DIR%
xcopy /c /Y %G_BUILD_DIR%..\inc\eSDKOBSS3.h %G_CUR_DIR%
xcopy /c /Y %G_BUILD_DIR%..\cert\*.crt %G_CUR_DIR%
xcopy /c /Y %G_BUILD_DIR%..\cert\*.jks %G_CUR_DIR%


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

@echo on
