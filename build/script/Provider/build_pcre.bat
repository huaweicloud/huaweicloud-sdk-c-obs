@echo off

rem 设置变量
set pcre_version=pcre-8.39
SET PCRE_PATH=%cd%\..\..\..\third_party_groupware\eSDK_Storage_Plugins\%pcre_version%\source
SET SOLUTION="%PCRE_PATH%\sln\pcre_839\pcre_839.sln"
if {x64} == {%~1} (
SET SOLUTION_CONFIG="Release|X64"
SET OUTPUT_TAG=win64_x64_msvc
SET PLATFORM=x64
) else if {win32} == {%~1} (
SET SOLUTION_CONFIG="Release|Win32"
SET OUTPUT_TAG=win32_x86_msvc
SET PLATFORM=x86
) else (
echo "the first parameter should be x64 or win32"
echo "Usage: build_openssl.bat x64 or build_openssl.bat win32"
goto Exit
)

SET ACTION=Rebuild

echo -----------start to compile prce-----------

rem 编译系统工程
"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv" %SOLUTION% /%ACTION% %SOLUTION_CONFIG%


xCOPY "%PCRE_PATH%\sln\pcre_839\%PLATFORM%\Release\*.dll" "%PCRE_PATH%\..\..\build\%pcre_version%\%OUTPUT_TAG%\bin\" /y
xCOPY "%PCRE_PATH%\sln\pcre_839\%PLATFORM%\Release\*.lib" "%PCRE_PATH%\..\..\build\%pcre_version%\%OUTPUT_TAG%\lib\" /y
xCOPY "%PCRE_PATH%\include\*.h" "%PCRE_PATH%\..\..\build\%pcre_version%\include\" /y


xCOPY "%PCRE_PATH%\sln\pcre_839\%PLATFORM%\Release\*.dll" "%PCRE_PATH%\..\bin\%OUTPUT_TAG%\release\" /y
xCOPY "%PCRE_PATH%\sln\pcre_839\%PLATFORM%\Release\*.lib" "%PCRE_PATH%\..\lib\%OUTPUT_TAG%\release\" /y
xCOPY "%PCRE_PATH%\include\*.h" "%PCRE_PATH%\..\include\" /y

xCOPY "%PCRE_PATH%\sln\pcre_839\%PLATFORM%\Release\*.dll" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\bin\%OUTPUT_TAG%\release\" /y
xCOPY "%PCRE_PATH%\sln\pcre_839\%PLATFORM%\Release\*.lib" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\lib\%OUTPUT_TAG%\release\" /y
xCOPY "%PCRE_PATH%\include\*.h" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\include\" /y

:Exit