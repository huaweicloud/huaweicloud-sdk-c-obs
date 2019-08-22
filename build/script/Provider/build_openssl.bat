@echo off

set OPENSSL_VERSION=openssl-1.0.2r

set OPENSSL_PATH=%cd%\..\..\..\third_party_groupware\eSDK_Storage_Plugins\%OPENSSL_VERSION%

if {x64} == {%~1} (
set VCVARS="%VS100COMNTOOLS%\..\..\VC\bin\amd64\vcvars64.bat"
set OPENSSL_CONFIG=VC-WIN64A
set OPENSSL_CONFIG_BAT="ms/do_win64a.bat"
set OUTPUT_TAG=win64_x64_msvc
) else if {win32} == {%~1} (
set VCVARS="%VS100COMNTOOLS%\..\..\VC\bin\vcvars32.bat"
set OPENSSL_CONFIG=VC-WIN32
set OPENSSL_CONFIG_BAT="ms/do_ms.bat"
set OUTPUT_TAG=win32_x86_msvc
) else (
echo "the first parameter should be x64 or win32"
echo "Usage: build_openssl.bat x64 or build_openssl.bat win32"
goto exit
)

echo .
echo  -----------start to compile openssl-----------
echo .
cd %OPENSSL_PATH%

call %VCVARS%
perl Configure %OPENSSL_CONFIG% no-asm --prefix=..\openssl
call %OPENSSL_CONFIG_BAT%

nmake -f ms/ntdll.mak clean
nmake -f ms/ntdll.mak
nmake -f ms/ntdll.mak install

::xCOPY "%OPENSSL_PATH%\..\..\openssl\" "%OPENSSL_PATH%\..\..\build\openssl-1.0.2k\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\bin\*.dll" "%OPENSSL_PATH%\..\build\%OPENSSL_VERSION%\bin\%OUTPUT_TAG%\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\lib\*.lib" "%OPENSSL_PATH%\..\build\%OPENSSL_VERSION%\lib\%OUTPUT_TAG%\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\bin\*.exe" "%OPENSSL_PATH%\..\build\%OPENSSL_VERSION%\bin\%OUTPUT_TAG%\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\lib\engines\*.dll" "%OPENSSL_PATH%\..\build\%OPENSSL_VERSION%\lib\%OUTPUT_TAG%\engines\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\include\openssl\*.h" "%OPENSSL_PATH%\..\build\%OPENSSL_VERSION%\include\openssl\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\include\openssl\*.c" "%OPENSSL_PATH%\..\build\%OPENSSL_VERSION%\include\openssl\" /s /e /y


xCOPY "%OPENSSL_PATH%\..\openssl\bin\*.dll" "%OPENSSL_PATH%\bin\%OUTPUT_TAG%\release\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\lib\*.lib" "%OPENSSL_PATH%\lib\%OUTPUT_TAG%\release\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\include\openssl\*.h" "%OPENSSL_PATH%\include\openssl\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\include\openssl\*.c" "%OPENSSL_PATH%\include\openssl\" /s /e /y

xCOPY "%OPENSSL_PATH%\..\openssl\bin\*.dll" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\bin\%OUTPUT_TAG%\release\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\include\openssl\*.h" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\include\openssl\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\include\openssl\*.c" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\include\openssl\" /s /e /y
xCOPY "%OPENSSL_PATH%\..\openssl\lib\*.lib" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\lib\%OUTPUT_TAG%\release\" /s /e /y

cd "%cd%\..\..\..\build\script\Provider"
:exit